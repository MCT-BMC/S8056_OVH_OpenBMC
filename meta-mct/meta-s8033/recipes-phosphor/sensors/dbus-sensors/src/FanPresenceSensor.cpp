#include "FanPresenceSensor.hpp"

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
    "xyz.openbmc_project.Configuration.FanPresenceSensor";
static constexpr double MaxReading = 0xFF;
static constexpr double MinReading = 0;

const std::string logSelInitPath = "/run/fanpresencesensor/";

boost::container::flat_map<std::string, std::unique_ptr<FanPresenceSensor>> sensors;

FanPresenceSensor::FanPresenceSensor(std::shared_ptr<sdbusplus::asio::connection>& conn,
                             boost::asio::io_service& io,
                             const std::string& sensorName,
                             const std::string& sensorConfiguration,
                             sdbusplus::asio::object_server& objectServer,
                             std::vector<thresholds::Threshold>&& thresholdData,
                             std::string gpioName, std::string path,
                             std::string polarity, std::string eventMon,
                             PowerState readState) :
    Sensor(escapeName(sensorName),
           std::move(thresholdData), sensorConfiguration,
           "xyz.openbmc_project.Configuration.FanPresenceSensor", false, false, MaxReading,
           MinReading, conn, readState),
    objectServer(objectServer), dbusConnection(conn), waitTimer(io),
    gpioName(gpioName), path(path), polarity(polarity), eventMon(eventMon)
{
    sensorInterface = objectServer.add_interface(
        "/xyz/openbmc_project/sensors/" + path + "/" + name,
        "xyz.openbmc_project.Sensor.Value");

    association = objectServer.add_interface(
        "/xyz/openbmc_project/sensors/" + path + "/" + name,
        "org.openbmc.Associations");
    
    sensorPath = "/xyz/openbmc_project/sensors/" + path + "/" + name;
}

FanPresenceSensor::~FanPresenceSensor()
{
    waitTimer.cancel();
    objectServer.remove_interface(sensorInterface);
    objectServer.remove_interface(association);
}

void FanPresenceSensor::setFaultLed(std::string setPolarity, bool init)
{
    std::string dbusMethod;

    if(setPolarity == polarity)
    {
        dbusMethod = "increaseFaultLed";
    }
    else
    {
        dbusMethod = "decreaseFaultLed";
    }

    if(dbusMethod == "decreaseFaultLed" && init)
    {
        return;
    }

    try
    {
        auto method = dbusConnection->new_method_call("xyz.openbmc_project.mct.led",
                                                  "/xyz/openbmc_project/mct/led",
                                                  "xyz.openbmc_project.mct.led", dbusMethod.c_str());
        dbusConnection->call_noreply(method);
    }
    catch (std::exception& e)
    {
        std::cerr << "Failed to set fault LED status:" << e.what() << "\n";
    }

}

void FanPresenceSensor::logSELEvent(std::string enrty, std::string path ,
                     uint8_t eventData0, uint8_t eventData1, uint8_t eventData2)
{
    std::vector<uint8_t> eventData(3, 0xFF);
    eventData[0] = eventData0;
    eventData[1] = eventData1;
    eventData[2] = eventData2;
    uint16_t genid = 0x20;

    sdbusplus::message::message writeSEL = dbusConnection->new_method_call(
        "xyz.openbmc_project.Logging.IPMI", "/xyz/openbmc_project/Logging/IPMI",
        "xyz.openbmc_project.Logging.IPMI", "IpmiSelAdd");
    writeSEL.append(enrty, path, eventData, true, (uint16_t)genid);
    try
    {
        dbusConnection->call(writeSEL);
    }
    catch (std::exception& e)
    {
        std::cerr << "Failed to log the button event:" << e.what() << "\n";
    }
}

void FanPresenceSensor::gpioValueHandler(bool value, bool init)
{
    bool enableLog = true;
    if(init)
    {
        std::ifstream initfile(logSelInitPath+name);
        if(initfile.good())
        {
            enableLog = false;
        }
        initfile.close();
    }

    uint8_t deviceState = discrete::deviceState::Present;

    if (value)
    {
        if(polarity == "HIGH")
        {
            //Device not presnet wiht GPIO high
            if(enableLog){
                logSELEvent("Presence Sensor Gpio HIGH with device absent",
                            sensorPath, 0x00, 0xFF, 0xFF);
            }
            deviceState = discrete::deviceState::Absent;
        }
        else
        {
            //Device presnet wiht GPIO high
            if(enableLog && !init){
                logSELEvent("Presence Sensor Gpio HIGH with device present",
                            sensorPath, 0x01, 0xFF, 0xFF);
            }
        }
        setFaultLed("HIGH", init);
    }
    else
    {
        if(polarity == "LOW")
        {
            //Device not presnet with GPIO low
            if(enableLog){
                logSELEvent("Presence Sensor Gpio LOW with device absent",
                    sensorPath, 0x00, 0xFF, 0xFF);
            }
            deviceState = discrete::deviceState::Absent;
        }
        else
        {
            //Device presnet with GPIO low
            if(enableLog && !init){
                logSELEvent("Presence Sensor Gpio LOW with device present",
                    sensorPath, 0x01, 0xFF, 0xFF);
            }
        }
        setFaultLed("LOW", init);
    }

    std::ofstream outfile(logSelInitPath+name, std::ofstream::out);
    outfile << +deviceState;
    outfile.close();

    updateValue(deviceState);
}

void FanPresenceSensor::gpioHandler(void)
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

        bool newGpioValue = gpioLine.get_value();

        if(newGpioValue == gpioValue)
        {
            gpioHandler();
            return;
        }

        gpioValue = newGpioValue;
        gpioValueHandler(gpioValue, false);

        gpioHandler();
    });
}

void FanPresenceSensor::init(void)
{
    setInitialProperties(sensor_paths::unitPercent);

    gpioLine = gpiod::find_line(gpioName);
    if (!gpioLine)
    {
        std::cerr << "Failed to find the " << gpioName << " line" << std::endl;
        return;
    }

    try
    {
        gpioLine.request(
            {"presence-sensor", gpiod::line_request::DIRECTION_INPUT, 0});
    }
    catch (std::exception&)
    {
        std::cerr << "Failed to request events for " << gpioName << std::endl;
        return;
    }

    gpioValue = gpioLine.get_value();
    gpioValueHandler(gpioValue, true);

    gpioHandler();
    
}

void FanPresenceSensor::checkThresholds(void)
{
    thresholds::checkThresholds(this);
}

void createSensors(
    boost::asio::io_service& io, sdbusplus::asio::object_server& objectServer,
    boost::container::flat_map<std::string, std::unique_ptr<FanPresenceSensor>>&
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
                    
                    std::string gpioName =
                        loadVariant<std::string>(entry.second, "GpioName");

                    std::vector<thresholds::Threshold> sensorThresholds;
                    if (!parseThresholdsFromConfig(pathPair.second,
                                                   sensorThresholds))
                    {
                        std::cerr << "error populating thresholds for " << name
                                  << "\n";
                    }

                    std::string path = loadVariant<std::string>(entry.second, "Path");

                    std::string polarity = loadVariant<std::string>(entry.second, "Polarity");

                    std::string eventMon = loadVariant<std::string>(entry.second, "EventMon");

                    PowerState readState = PowerState::always;
                    std::string powerState = loadVariant<std::string>(entry.second, "PowerState");
                    setReadState(powerState, readState);

                    if constexpr (debug)
                    {
                        std::cerr
                            << "Configuration parsed for \n\t" << entry.first
                            << "\n"
                            << "with\n"
                            << "\tName: " << name << "\n"
                            << "\tGpioName: " << gpioName << "\n"
                            << "\tpath: " << path << "\n"
                            << "\tPolarity: " << polarity << "\n"
                            << "\tEventMon: " << eventMon << "\n";
                    }

                    auto& sensor = sensors[name];

                    sensor = std::make_unique<FanPresenceSensor>(
                        dbusConnection, io, name, pathPair.first, objectServer,
                        std::move(sensorThresholds), gpioName , path, polarity, eventMon,
                        readState);

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
    systemBus->request_name("xyz.openbmc_project.FanPresenceSensor");
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

    io.run();
}
