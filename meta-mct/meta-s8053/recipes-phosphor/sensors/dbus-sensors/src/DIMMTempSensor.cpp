#include "DIMMTempSensor.hpp"

#include "Utils.hpp"
#include "MUXUtils.hpp"
#include "VariantVisitors.hpp"

#include <math.h>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <chrono>
#include <iostream>
#include <limits>
#include <numeric>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <vector>

extern "C" {
#include <i2c/smbus.h>
#include <linux/i2c-dev.h>
}

constexpr const bool debug = false;

constexpr const char* supportClass = "MUX";
constexpr const char* configInterface =
    "xyz.openbmc_project.Configuration.DIMMTempSensor";
static constexpr double TempMaxReading = 0xFF;
static constexpr double TempMinReading = 0;

static constexpr double DimmAddress[] =
{
    0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
};

static constexpr uint8_t DIMM_CURRENT_TEMP_REG = 0x05;

static constexpr uint8_t MAX_UC_RETRY = 3;

boost::container::flat_map<std::string, std::unique_ptr<DIMMTempSensor>> sensors;

DIMMTempSensor::DIMMTempSensor(std::shared_ptr<sdbusplus::asio::connection>& conn,
                             boost::asio::io_service& io,
                             const std::string& sensorClass,
                             const std::string& muxName,
                             const std::string& sensorName,
                             const std::string& sensorConfiguration,
                             sdbusplus::asio::object_server& objectServer,
                             std::vector<thresholds::Threshold>&& thresholdData,
                             PowerState readState,
                             uint8_t muxBusId, uint8_t muxAddress,
                             uint8_t dimmAddress, uint8_t tempReg,
                             uint8_t offset,uint8_t dimmGroupCounter,
                             uint8_t delayTime) :
    Sensor(escapeName(sensorName),
           std::move(thresholdData), sensorConfiguration,
           "xyz.openbmc_project.Configuration.DIMMTemp", false, false,
           TempMaxReading,TempMinReading,conn, readState),
    objectServer(objectServer), dbusConnection(conn), waitTimer(io),
    muxName(muxName), muxBusId(muxBusId), muxAddress(muxAddress), dimmAddress(dimmAddress),
    tempReg(tempReg),offset(offset),dimmGroupCounter(dimmGroupCounter),delayTime(delayTime),
    retry(0)
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

    if(sensorClass == supportClass)
    {
        enableMux = true;
    }
    else
    {
        enableMux = false;
    }

    PowerDelayCounter = delayTime;

    setupPowerMatch(conn);
    setupOwnerMatch(conn);

    upperCritical = std::numeric_limits<double>::max();
    for (const auto& threshold : thresholds)
    {
        if(threshold.level == thresholds::Level::CRITICAL &&
           threshold.direction == thresholds::Direction::HIGH)
        {
            upperCritical = threshold.value;
            break;
        }
    }
}

DIMMTempSensor::~DIMMTempSensor()
{
    waitTimer.cancel();
    for (const auto& iface : thresholdInterfaces)
    {
        objectServer.remove_interface(iface);
    }
    objectServer.remove_interface(sensorInterface);
    objectServer.remove_interface(association);
}

void DIMMTempSensor::init(void)
{
    setInitialProperties(sensor_paths::unitDegreesC);
    read();
}

void DIMMTempSensor::checkThresholds(void)
{
    thresholds::checkThresholds(this);
}

void DIMMTempSensor::selectMux()
{
    switch(resolveMuxList(muxName))
    {
        case PCA9544:
            static constexpr uint8_t PCA9544TempBase = 0x04;
            switchPCA9544(muxBusId,muxAddress,PCA9544TempBase+((offset*dimmGroupCounter)/(sizeof(DimmAddress)/ sizeof(double))));
            break;
        default:
            std::cerr << "Not support MUX type\n";
            break;
    }

}

int DIMMTempSensor::getDimmRegsInfoWord(uint8_t dimmAddress,uint8_t regs, int32_t* data, double* rawValue)
{
    std::string i2cBus = "/dev/i2c-" + std::to_string(muxBusId);
    int fd = open(i2cBus.c_str(), O_RDWR);

    if (fd < 0)
    {
        std::cerr << " unable to open i2c device" << i2cBus << "  err=" << fd
                  << "\n";
        return -1;
    }

    if (ioctl(fd, I2C_SLAVE_FORCE, dimmAddress) < 0)
    {
        std::cerr << " unable to set device address\n";
        close(fd);
        return -1;
    }

    unsigned long funcs = 0;
    if (ioctl(fd, I2C_FUNCS, &funcs) < 0)
    {
        std::cerr << " not support I2C_FUNCS\n";
        close(fd);
        return -1;
    }

    if (!(funcs & I2C_FUNC_SMBUS_READ_WORD_DATA))
    {
        std::cerr << " not support I2C_FUNC_SMBUS_READ_WORD_DATA\n";
        close(fd);
        return -1;
    }

    *data = i2c_smbus_read_word_data(fd, regs);
    *rawValue = *data;

    close(fd);

    if (*data < 0)
    {
        if constexpr (debug)
        {
            std::cerr << " read word data failed at " << static_cast<int>(regs) << "\n";
        }
        return -1;
    }

    uint32_t tempLSB = ((*data & 0x0000ff00) >> 8) >> 2;
    uint32_t tempMSB = (((*data & 0x000000ff) << 8)&0x00001f00) >> 2;

    *data = (tempMSB+tempLSB)*0.25;

    return 0;
}

void DIMMTempSensor::read(void)
{
    static constexpr size_t pollTime = 1; // in seconds

    waitTimer.expires_from_now(boost::posix_time::seconds(pollTime));
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

        if (!isPowerOn())
        {
            markAvailable(false);
            PowerDelayCounter = 0;
            read();
            return;
        }

        if (checkOwnerStatus() == 0x00)
        {
            markAvailable(false);
            PowerDelayCounter = 0;
            read();
            return;
        }

        if(PowerDelayCounter < delayTime)
        {
            markAvailable(false);
            PowerDelayCounter = PowerDelayCounter + 1;
            read();
            return;
        }

        int32_t temp = 0;
        int32_t tempSum = 0;
        bool i2cSuccess = false;
        if(enableMux)
        {
            selectMux();
        }
        for(int i=0;i<dimmGroupCounter;i++)
        {
            int ret = getDimmRegsInfoWord(dimmAddress+i, tempReg, &temp, &rawValue);
            if(ret >=0)
            {
                if(tempSum<=temp)
                {
                    tempSum = temp;
                }
                i2cSuccess=true;
            }
        }
        if (i2cSuccess)
        {
            double v = static_cast<double>(tempSum);
            if constexpr (debug)
            {
                std::cerr << "Value update to " << (double)v << " raw reading "
                          << static_cast<int>(tempSum) << "\n";
            }
            if (v >= upperCritical)
            {
                if(retry < MAX_UC_RETRY){
                    std::cerr << name << " sensor reading UC for " << retry << " times, rawValue:" << rawValue << std::endl;
                    retry++;
                }
            }
            else
            {
                retry = 0;
            }

            if ((v < upperCritical) || (retry >= MAX_UC_RETRY))
            {
                markAvailable(true);
                updateValue(v);
            }
        }
        else
        {
            if constexpr (debug)
            {
                std::cerr << "Invalid read getDimmRegsInfoWord\n";
            }
            markAvailable(false);
        }
        read();
    });
}

void createSensors(
    boost::asio::io_service& io, sdbusplus::asio::object_server& objectServer,
    boost::container::flat_map<std::string, std::unique_ptr<DIMMTempSensor>>&
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

                    std::string sensorClass =
                        loadVariant<std::string>(entry.second, "Class");

                    std::string muxName =
                        loadVariant<std::string>(entry.second, "Name");

                    uint8_t muxBusId = loadVariant<uint8_t>(entry.second, "Bus");

                    uint8_t muxAddress =
                        loadVariant<uint8_t>(entry.second, "Address");

                    uint8_t dimmSlotNum = loadVariant<uint8_t>(entry.second, "DimmSlotNum");

                    uint8_t dimmGroupCounter = loadVariant<uint8_t>(entry.second, "DimmGroupCounter");

                    uint8_t delayTime = loadVariant<uint8_t>(entry.second, "DelayTime");

                    PowerState readState = PowerState::always;
                    std::string powerState = loadVariant<std::string>(entry.second, "PowerState");
                    setReadState(powerState, readState);

                    std::vector<thresholds::Threshold> sensorThresholds;
                    if (!parseThresholdsFromConfig(pathPair.second,
                                                   sensorThresholds))
                    {
                        std::cerr << "error populating thresholds for " << muxName
                                  << "\n";
                    }

                    for(int i=0;i<dimmSlotNum/dimmGroupCounter;i++)
                    {
                        std::string sensorName = "Name"+  std::to_string(i);
                        std::string dimmName =
                            loadVariant<std::string>(entry.second, sensorName);
                        uint8_t selectDimmAddress = DimmAddress[(i*dimmGroupCounter)%(sizeof(DimmAddress)/ sizeof(double))];
                        std::vector<thresholds::Threshold> dimmThresholds(sensorThresholds);

                        if constexpr (debug)
                        {
                            std::cerr
                                << "Configuration parsed for \n\t" << entry.first
                                << "\n"
                                << "with\n"
                                << "\tSensor Class: " << sensorClass << "\n"
                                << "\tMUX Name: " << muxName << "\n"
                                << "\tMUX Bus: " << static_cast<int>(muxBusId) << "\n"
                                << "\tMUX BUS Address: " << static_cast<int>(muxAddress)
                                << "\n"
                                << "\tDIMM Name: " << dimmName << "\n"
                                << "\tDIMM Address: " << static_cast<int>(selectDimmAddress) << "\n"
                                << "\tDIMM Reg: " << static_cast<int>(DIMM_CURRENT_TEMP_REG) << "\n";
                        }

                        auto& sensor = sensors[dimmName];

                        sensor = std::make_unique<DIMMTempSensor>(
                            dbusConnection, io, sensorClass, muxName, dimmName, pathPair.first, objectServer,
                            std::move(dimmThresholds), readState, muxBusId, muxAddress,selectDimmAddress,
                            DIMM_CURRENT_TEMP_REG,i,dimmGroupCounter,delayTime);

                        sensor->init();
                    }
                }
            }
        },
        entityManagerName, "/", "org.freedesktop.DBus.ObjectManager",
        "GetManagedObjects");
}

int main(int argc, char** argv)
{
    boost::asio::io_service io;
    auto systemBus = std::make_shared<sdbusplus::asio::connection>(io);
    systemBus->request_name("xyz.openbmc_project.DIMMTempSensor");
    sdbusplus::asio::object_server objectServer(systemBus);

    io.post([&]() { createSensors(io, objectServer, sensors, systemBus); });

    boost::asio::deadline_timer configTimer(io);

    std::function<void(sdbusplus::message::message&)> eventHandler =
        [&](sdbusplus::message::message& message) {
            configTimer.expires_from_now(boost::posix_time::seconds(1));
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
