#include "I2cNVMeSensor.hpp"

#include "Utils.hpp"
#include "VariantVisitors.hpp"

#include <math.h>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/container/flat_map.hpp>
#include <chrono>
#include <functional>
#include <iostream>
#include <limits>
#include <memory>
#include <numeric>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/bus/match.hpp>
#include <string>
#include <vector>

extern "C" {
#include <i2c/smbus.h>
#include <linux/i2c-dev.h>
}

constexpr const bool debug = false;

constexpr const char* configInterface =
    "xyz.openbmc_project.Configuration.I2cNVMe";
static constexpr double maxReading = 127;
static constexpr double minReading = -128;


boost::container::flat_map<std::string, std::unique_ptr<I2cNvmeSensor>> sensors;

I2cNvmeSensor::I2cNvmeSensor(std::shared_ptr<sdbusplus::asio::connection>& conn,
                             boost::asio::io_service& io,
                             const std::string& sensorName,
                             const std::string& sensorConfiguration,
                             sdbusplus::asio::object_server& objectServer,
                             std::vector<thresholds::Threshold>&& thresholdData,
                             uint8_t busId, uint8_t address) :
    Sensor(escapeName(sensorName),
           std::move(thresholdData), sensorConfiguration,
           "xyz.openbmc_project.Configuration.I2cNVMe", false, false, maxReading,
           minReading,conn),
    busId(busId), address(address),
    objectServer(objectServer), dbusConnection(conn), waitTimer(io)
{
    sensorInterface = objectServer.add_interface(
        "/xyz/openbmc_project/sensors/temperature/" + name,
        "xyz.openbmc_project.Sensor.Value");

    for (const auto& threshold : thresholds)
    {
        std::string interface = thresholds::getInterface(threshold.level);
        thresholdInterfaces[static_cast<size_t>(threshold.level)] =
            objectServer.add_interface(
                "/xyz/openbmc_project/sensors/temperature/" + name, interface);
    }
    association = objectServer.add_interface(
        "/xyz/openbmc_project/sensors/temperature/" + name,
        association::interface);
}

I2cNvmeSensor::~I2cNvmeSensor()
{
    waitTimer.cancel();
    for (const auto& iface : thresholdInterfaces)
    {
        objectServer.remove_interface(iface);
    }
    objectServer.remove_interface(sensorInterface);
    objectServer.remove_interface(association);
}

void I2cNvmeSensor::init(void)
{
    setInitialProperties(sensor_paths::unitDegreesC);
    read();
}

void I2cNvmeSensor::checkThresholds(void)
{
    thresholds::checkThresholds(this);
}

#define I2C_DATA_MAX 256

static inline __u8 i2c_8bit_addr_from_msg(const struct i2c_msg* msg)
{
    return (msg->addr << 1) | (msg->flags & I2C_M_RD ? 1 : 0);
}

/* Since I2C_M_RD not implement PEC in the linux driver layer,
   so copy some driver PEC check functions here */
#define POLY (0x1070U << 3)
static inline __u8 crc8(__u16 data)
{
    int i;

    for (i = 0; i < 8; i++)
    {
        if (data & 0x8000)
            data = data ^ POLY;
        data = data << 1;
    }
    return (__u8)(data >> 8);
}

/* Incremental CRC8 over count bytes in the array pointed to by p */
static inline __u8 i2c_smbus_pec(__u8 crc, __u8* p, size_t count)
{
    int i;

    for (i = 0; i < (int)count; i++)
        crc = crc8((crc ^ p[i]) << 8);
    return crc;
}

/* Assume a 7-bit address, which is reasonable for SMBus */
static inline __u8 i2c_smbus_msg_pec(__u8 pec, struct i2c_msg* msg)
{
    /* The address will be sent first */
    __u8 addr = i2c_8bit_addr_from_msg(msg);
    pec = i2c_smbus_pec(pec, &addr, 1);

    /* The data buffer follows */
    return i2c_smbus_pec(pec, (__u8*)msg->buf, msg->len);
}

/* Return <0 on CRC error
   If there was a write before this read (most cases) we need to take the
   partial CRC from the write part into account.
   Note that this function does modify the message (we need to decrease the
   message length to hide the CRC byte from the caller). */
static inline int i2c_smbus_check_pec(__u8 cpec, struct i2c_msg* msg)
{
    __u8 rpec = msg->buf[--msg->len];
    cpec = i2c_smbus_msg_pec(cpec, msg);
    if (rpec != cpec)
    {
        printf("Error: Bad PEC 0x%02x vs. 0x%02x\n", rpec, cpec);
        return -1;
    }
    return 0;
}

static inline __s32 i2c_read_after_write(int file, __u8 command,
                                         __u8 slave_addr, __u8 tx_len,
                                         const __u8* tx_buf, int rx_len,
                                         const __u8* rx_buf)
{
    struct i2c_rdwr_ioctl_data msgst;
    struct i2c_msg msg[2];
    int ret;
    int status;
    __u8 partial_pec = 0;

    msg[0].addr = slave_addr & 0xFF;
    msg[0].flags = 0;
    msg[0].buf = (unsigned char*)tx_buf;
    msg[0].len = tx_len;

    msg[1].addr = slave_addr & 0xFF;
    msg[1].flags = I2C_M_RD | I2C_M_RECV_LEN;
    msg[1].buf = (unsigned char*)rx_buf;
    msg[1].len = rx_len;

    msgst.msgs = msg;
    msgst.nmsgs = 2;

    ret = ioctl(file, I2C_RDWR, &msgst);
    if (ret < 0)
        return ret;
    else if (command)
    {
        partial_pec = i2c_smbus_msg_pec(0, &msg[0]);
        msg[1].len = rx_buf[0] + 1;
        status = i2c_smbus_check_pec(partial_pec, &msg[1]);
        return status;
    }
    return ret;
}

int I2cNvmeSensor::getNVMeTemp(uint8_t* pu8data)
{
    int res;
    unsigned char Rx_buf[I2C_DATA_MAX] = {0};
    unsigned char tx_data = 0;  //command code
    std::string i2cBus = "/dev/i2c-" + std::to_string(busId);
    int fd = open(i2cBus.c_str(), O_RDWR);

    if (fd < 0)
    {
        std::cerr << " unable to open i2c device" << i2cBus << "  err=" << fd
                  << "\n";
        return -1;
    }

    Rx_buf[0] = 1;
    res = i2c_read_after_write(fd, 0, address, 1,
                               &tx_data, I2C_DATA_MAX,
                               (const unsigned char*)Rx_buf);

    if (res < 0)
    {
        if constexpr (debug)
        {
            std::cerr << "Error: block write read failed\n";
        }
        close(fd);
        return -1;
    }

    if constexpr (debug)
    {
        std::cerr << "Block data read \n\t";
        for (int i=0; i<32; i++)
        {
            std::cerr << std::to_string(Rx_buf[i]) << "  ";
        }
        std::cerr << "\n";
    }

    *pu8data = Rx_buf[3];
    
    close(fd);

    if (*pu8data <= 0x7E)
    {
        return 0;
    }
    else if (*pu8data == 0x7F)
    {
        std::cerr << "Temperature is 127 or higher\n";
        return -1;
    }
    else if (*pu8data == 0x80)
    {
        if constexpr (debug)
        {
            std::cerr << "No temperature data or temperature data is more than 5 seconds old\n";
        }
        return -1;
    }
    else if (*pu8data == 0x81)
    {
        std::cerr << "Temperature sensor failure\n";
        return -1;
    }
    else if (*pu8data == 0xC4)
    {
        std::cerr << "Temperature is -60 or lower\n";
        return -1;
    }
    else if (*pu8data >= 0xC5)
    {
        std::cerr << "Temperature measured in degrees Celsius is represented\n in twos complement(-1 to -59)\n";
        return -1;
    }
    else
    {
        return -1;
    }
}

void I2cNvmeSensor::read(void)
{
    static constexpr size_t pollTime = 1; // in seconds

    waitTimer.expires_from_now(boost::asio::chrono::seconds(pollTime));
    waitTimer.async_wait([this](const boost::system::error_code& ec) {
        if (ec == boost::asio::error::operation_aborted)
        {
            return; // we're being cancelled
        }
        // read timer error
        else if (ec)
        {
            std::cerr << "timer error\n";
            return;
        }
        uint8_t temp;
        int ret = getNVMeTemp(&temp);
        if (ret >= 0)
        {
            double v = static_cast<double>(temp);
            if constexpr (debug)
            {
                std::cerr << "Value update to " << v << "\n";
            }
            markAvailable(true);
            updateValue(v);
        }
        else
        {
            if constexpr (debug)
            {
                std::cerr << "Invalid read getNVMeTemp\n";
            }
            markAvailable(false);
        }
        read();
    });
}

void createSensors(
    boost::asio::io_service& io, sdbusplus::asio::object_server& objectServer,
    boost::container::flat_map<std::string, std::unique_ptr<I2cNvmeSensor>>&
        sensors,
    std::shared_ptr<sdbusplus::asio::connection>& dbusConnection)
{
    if (!dbusConnection)
    {
        std::cerr << "Connection not created\n";
        return;
    }

    dbusConnection->async_method_call(
        [&io, &objectServer, &dbusConnection, &sensors](
            boost::system::error_code ec, const ManagedObjectType& resp) {
            if (ec)
            {
                std::cerr << "Error contacting entity manager\n";
                return;
            }
            for (const auto& pathPair : resp)
            {
                for (const auto& entry : pathPair.second)
                {
                    if (entry.first != configInterface)
                    {
                        continue;
                    }
                    std::string name =
                        loadVariant<std::string>(entry.second, "Name");

                    std::vector<thresholds::Threshold> sensorThresholds;
                    if (!parseThresholdsFromConfig(pathPair.second,
                                                   sensorThresholds))
                    {
                        std::cerr << "error populating thresholds for " << name
                                  << "\n";
                    }

                    uint8_t busId = loadVariant<uint8_t>(entry.second, "Bus");

                    uint8_t address =
                        loadVariant<uint8_t>(entry.second, "Address");

                    if constexpr (debug)
                    {
                        std::cerr
                            << "Configuration parsed for \n\t" << entry.first
                            << "\n"
                            << "with\n"
                            << "\tName: " << name << "\n"
                            << "\tBus: " << static_cast<int>(busId) << "\n"
                            << "\tAddress: " << static_cast<int>(address)
                            << "\n";
                    }

                    auto& sensor = sensors[name];

                    sensor = std::make_unique<I2cNvmeSensor>(
                        dbusConnection, io, name, pathPair.first, objectServer,
                        std::move(sensorThresholds), busId, address);

                    sensor->init();
                }
            }
        },
        entityManagerName, "/", "org.freedesktop.DBus.ObjectManager",
        "GetManagedObjects");
}

int main()
{
    boost::asio::io_service io;
    auto systemBus = std::make_shared<sdbusplus::asio::connection>(io);
    systemBus->request_name("xyz.openbmc_project.I2cNvmeSensor");
    sdbusplus::asio::object_server objectServer(systemBus);

    io.post([&]() { createSensors(io, objectServer, sensors, systemBus); });

    boost::asio::steady_timer configTimer(io);

    std::function<void(sdbusplus::message::message&)> eventHandler =
        [&](sdbusplus::message::message&) {
            configTimer.expires_from_now(boost::asio::chrono::seconds(1));
            // create a timer because normally multiple properties change
            configTimer.async_wait([&](const boost::system::error_code& ec) {
                if (ec == boost::asio::error::operation_aborted)
                {
                    return; // we're being canceled
                }
                // config timer error
                else if (ec)
                {
                    std::cerr << "timer error\n";
                    return;
                }
                createSensors(io, objectServer, sensors, systemBus);
                if (sensors.empty())
                {
                    std::cout << "Configuration not detected\n";
                }
            });
        };

    sdbusplus::bus::match::match configMatch(
        static_cast<sdbusplus::bus::bus&>(*systemBus),
        "type='signal',member='PropertiesChanged',"
        "path_namespace='" +
            std::string(inventoryPath) +
            "',"
            "arg0namespace='" +
            configInterface + "'",
        eventHandler);

    io.run();
}