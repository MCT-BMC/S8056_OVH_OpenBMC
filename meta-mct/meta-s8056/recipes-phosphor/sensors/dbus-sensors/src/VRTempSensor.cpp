#include "VRTempSensor.hpp"

#include "Utils.hpp"
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

constexpr const char* configInterface =
    "xyz.openbmc_project.Configuration.VRTempSensor";
static constexpr double TempMaxReading = 0xFF;
static constexpr double TempMinReading = 0;

boost::container::flat_map<std::string, std::unique_ptr<VRTempSensor>> sensors;

VRTempSensor::VRTempSensor(std::shared_ptr<sdbusplus::asio::connection>& conn,
                             boost::asio::io_service& io,
                             const std::string& sensorName,
                             const std::string& sensorConfiguration,
                             sdbusplus::asio::object_server& objectServer,
                             std::vector<thresholds::Threshold>&& thresholdData,
                             uint8_t busId, uint8_t address,
                             uint8_t tempReg) :
    Sensor(escapeName(sensorName),
           std::move(thresholdData), sensorConfiguration,
           "xyz.openbmc_project.Configuration.VrTemp", false, false, TempMaxReading,
           TempMinReading, conn),
    objectServer(objectServer), dbusConnection(conn), waitTimer(io),
    busId(busId), address(address), tempReg(tempReg)
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

VRTempSensor::~VRTempSensor()
{
    waitTimer.cancel();
    for (const auto& iface : thresholdInterfaces)
    {
        objectServer.remove_interface(iface);
    }
    objectServer.remove_interface(sensorInterface);
    objectServer.remove_interface(association);
}

void VRTempSensor::init(void)
{
    setInitialProperties(sensor_paths::unitDegreesC);
    read();
}

void VRTempSensor::checkThresholds(void)
{
    thresholds::checkThresholds(this);
}

int VRTempSensor::getRegsInfoWord(uint8_t regs, int16_t* pu16data)
{
    std::string i2cBus = "/dev/i2c-" + std::to_string(busId);
    int fd = open(i2cBus.c_str(), O_RDWR);

    if (fd < 0)
    {
        std::cerr << " unable to open i2c device" << i2cBus << "  err=" << fd
                  << "\n";
        return -1;
    }

    if (ioctl(fd, I2C_SLAVE_FORCE, address) < 0)
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

    *pu16data = i2c_smbus_read_word_data(fd, regs);
    close(fd);

    if (*pu16data < 0)
    {
        if constexpr (debug)
        {
            std::cerr << " read word data failed at " << static_cast<int>(regs)
                      << "\n";
        }
        return -1;
    }

    return 0;
}

void VRTempSensor::read(void)
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
        int16_t temp;
        int ret = getRegsInfoWord(tempReg, &temp);
        if (ret >= 0)
        {
            double v = static_cast<double>(temp);
            if constexpr (debug)
            {
                std::cerr << "Value update to " << (double)v << "raw reading "
                          << static_cast<int>(temp) << "\n";
            }
            markAvailable(true);
            if(v >= 0XFF)
            {
                std::cerr << "Invalid reading:" <<v << "\n";
            }
            else
            {
                updateValue(v);
            }
        }
        else
        {
            if constexpr (debug)
            {
                std::cerr << "Invalid read getRegsInfoWord\n";
            }
            markAvailable(false);
        }
        read();
    });
}

void createSensors(
    boost::asio::io_service& io, sdbusplus::asio::object_server& objectServer,
    boost::container::flat_map<std::string, std::unique_ptr<VRTempSensor>>&
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

                    uint8_t tempReg = loadVariant<uint8_t>(entry.second, "Reg");

                    if constexpr (debug)
                    {
                        std::cerr
                            << "Configuration parsed for \n\t" << entry.first
                            << "\n"
                            << "with\n"
                            << "\tName: " << name << "\n"
                            << "\tBus: " << static_cast<int>(busId) << "\n"
                            << "\tAddress: " << static_cast<int>(address)
                            << "\n"
                            << "\tReg: " << static_cast<int>(tempReg) << "\n";
                    }

                    auto& sensor = sensors[name];

                    sensor = std::make_unique<VRTempSensor>(
                        dbusConnection, io, name, pathPair.first, objectServer,
                        std::move(sensorThresholds), busId, address,
                        tempReg);

                    sensor->init();
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
    systemBus->request_name("xyz.openbmc_project.VRTempSensor");
    sdbusplus::asio::object_server objectServer(systemBus);

    io.post([&]() { createSensors(io, objectServer, sensors, systemBus); });

    boost::asio::steady_timer configTimer(io);

    std::function<void(sdbusplus::message::message&)> eventHandler =
        [&](sdbusplus::message::message& message) {
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

    setupManufacturingModeMatch(*systemBus);
    io.run();
}