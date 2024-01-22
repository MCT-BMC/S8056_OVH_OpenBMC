#include "VirtualSensor.hpp"

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

constexpr const bool debug = false;

constexpr const char* configInterface =
    "xyz.openbmc_project.Configuration.Virtual";

static constexpr double MAX_INPUNT_PARAMETER = 10;

enum mode
{
    dbus = 0,
    sysfs,
};

std::map<std::string, int> supportMode = {
    {"dbus", mode::dbus},
    {"sysfs", mode::sysfs},
};

boost::container::flat_map<std::string, std::unique_ptr<VirtualSensor>> sensors;

int getService(std::string& service,std::shared_ptr<sdbusplus::asio::connection>& bus, const std::string& intf,
                       const std::string& path)
{
    auto mapperCall =
        bus->new_method_call("xyz.openbmc_project.ObjectMapper",
                            "/xyz/openbmc_project/object_mapper",
                            "xyz.openbmc_project.ObjectMapper", "GetObject");

    mapperCall.append(path);
    mapperCall.append(std::vector<std::string>({intf}));
    try
    {
        auto mapperResponseMsg = bus->call(mapperCall);
        if (mapperResponseMsg.is_method_error())
        {
            return -1;
        }
        std::map<std::string, std::vector<std::string>> mapperResponse;
        mapperResponseMsg.read(mapperResponse);
        if (mapperResponse.begin() == mapperResponse.end())
        {
            return -1;
        }
        service = mapperResponse.begin()->first;
    }
    catch (const sdbusplus::exception::SdBusError& e){
        return -1;
    }
    return 0;
}

VirtualSensor::VirtualSensor(std::shared_ptr<sdbusplus::asio::connection>& conn,
                             boost::asio::io_service& io,
                             const std::string& sensorName,
                             const std::string& sensorConfiguration,
                             sdbusplus::asio::object_server& objectServer,
                             std::vector<thresholds::Threshold>&& thresholdData,
                             double maxReading, double minReading,
                             size_t pollTime, int selectMode, std::string sensorUnit,
                             std::vector<std::string> inputParameter,
                             std::vector<std::string> inputSensorUnit,
                             std::vector<double> inputScaleFactor,
                             std::vector<std::string> inputExpression) :
    Sensor(escapeName(sensorName),
           std::move(thresholdData), sensorConfiguration,
           "xyz.openbmc_project.Configuration.Virtual", false, false, maxReading,
           minReading, conn),
    objectServer(objectServer), dbusConnection(conn), waitTimer(io),
    pollTime(pollTime), selectMode(selectMode), sensorUnit(sensorUnit),
    inputParameter(inputParameter), inputSensorUnit(inputSensorUnit),
    inputScaleFactor(inputScaleFactor), inputExpression(inputExpression)
{
    sensorInterface = objectServer.add_interface(
        "/xyz/openbmc_project/sensors/" + sensorUnit + "/" + name,
        "xyz.openbmc_project.Sensor.Value");

    for (const auto& threshold : thresholds)
    {
        std::string interface = thresholds::getInterface(threshold.level);
        thresholdInterfaces[static_cast<size_t>(threshold.level)] =
            objectServer.add_interface(
                "/xyz/openbmc_project/sensors/" + sensorUnit + "/" + name, interface);
    }
    association = objectServer.add_interface(
        "/xyz/openbmc_project/sensors/" + sensorUnit +"/" + name,
        association::interface);
}

VirtualSensor::~VirtualSensor()
{
    waitTimer.cancel();
    for (const auto& iface : thresholdInterfaces)
    {
        objectServer.remove_interface(iface);
    }
    objectServer.remove_interface(sensorInterface);
    objectServer.remove_interface(association);
}

void VirtualSensor::init(void)
{
    if(sensorUnit == "power"){
        setInitialProperties(sensor_paths::unitWatts);
    }
    else if(sensorUnit == "temperature")
    {
        setInitialProperties(sensor_paths::unitDegreesC);
    }
    else if(sensorUnit == "voltage")
    {
        setInitialProperties(sensor_paths::unitVolts);
    }
    else if(sensorUnit == "current")
    {
        setInitialProperties(sensor_paths::unitAmperes );
    }
    else{
        std::cerr << "Sensor unit error\n";
        return;
    }

    read();
}

void VirtualSensor::checkThresholds(void)
{
    thresholds::checkThresholds(this);
}

int VirtualSensor::getParameterValue(double& value, std::string path, double scaleFactor)
{

    if(selectMode == mode::dbus)
    {
        std::variant<double> valueBuf;
        std::string service;

        int ret = getService(service, dbusConnection, "xyz.openbmc_project.Sensor.Value", path);

        if(ret < 0)
        {
            return -1;
        }

        auto method = dbusConnection->new_method_call(service.c_str(), path.c_str(), "org.freedesktop.DBus.Properties", "Get");
        method.append("xyz.openbmc_project.Sensor.Value","Value");

        try
        {
            auto reply=dbusConnection->call(method);
            if (reply.is_method_error())
            {
                return -1;
            }
            reply.read(valueBuf);
        }
        catch (const sdbusplus::exception::SdBusError& e){
            return -1;
        }
        value = std::get<double>(valueBuf) / scaleFactor;
    }
    else if(selectMode == mode::sysfs)
    {
        std::string line;
        std::string command = "cat " + path + " 2> /dev/null";
        std::array<char, 128> buffer;

        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe)
        {
            return -1;
        }
        while (fgets(buffer.data(), 128, pipe) != NULL) {
            line += buffer.data();
        }
        pclose(pipe);

        try
        {
            value = std::stoi(line) / scaleFactor;
        }
        catch (std::exception &e)
        {
            return 0;
        }
    }

    if constexpr (debug)
    {
        std::cerr << "Path : " << path << ", Value : " << std::to_string(value) << std::endl;
    }

    return 0;
}

void VirtualSensor::read(void)
{
    waitTimer.expires_from_now(boost::asio::chrono::seconds(pollTime));
    waitTimer.async_wait([this](const boost::system::error_code& ec) {
        if (ec == boost::asio::error::operation_aborted)
        {
            return; // we're being cancelled
        }
        // read timer error
        else if (ec)
        {
            return;
        }

        double value = 0;
        double readFailed = 0;
        int ret = 0;
        for(unsigned int i = 0; i < inputParameter.size(); i++){
            if(devErr[i]){
                devErr[i]--; 
                continue;
            }
            double valueTmp = 0;

            std::string sensorPath;

            switch(selectMode)
            {
                case mode::dbus :
                    sensorPath = "/xyz/openbmc_project/sensors/"+ inputSensorUnit[i] +"/"+ inputParameter[i];
                    break;
                case mode::sysfs :
                    sensorPath = inputParameter[i];
                    break;
            }

            if(i==0){
                ret = getParameterValue(value, sensorPath, inputScaleFactor[i]);
            }
            else
            {
                ret = getParameterValue(valueTmp, sensorPath, inputScaleFactor[i]);
            }

            if(ret < 0)
            {
                devErr[i]+=10;
                readFailed++;
            }
            else
            {
                if(i>0)
                {
                    if(inputExpression[i-1] == "+")
                    {
                        value = value + valueTmp;
                    }
                    else if(inputExpression[i-1] == "-")
                    {
                        value = value - valueTmp;
                    }
                    else if(inputExpression[i-1] == "*")
                    {
                        value = value * valueTmp;
                    }
                    else if(inputExpression[i-1] == "/")
                    {
                        value = value / valueTmp;
                    }
                }
            }
        }
        if(readFailed >= inputParameter.size())
        {
            markAvailable(false);
        }
        else
        {
            markAvailable(true);
            updateValue(value);
        }
        read();
    });
}

void createSensors(
    boost::asio::io_service& io, sdbusplus::asio::object_server& objectServer,
    boost::container::flat_map<std::string, std::unique_ptr<VirtualSensor>>&
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
                    std::string sensorUnit =
                        loadVariant<std::string>(entry.second, "sensorUnit");
                    std::string mode =
                        loadVariant<std::string>(entry.second, "Class");

                    int selectMode = mode::dbus;

                    if (supportMode.find(mode) != supportMode.end())
                    {
                        selectMode =  supportMode.find(mode)->second;
                    }

                    double maxReading =
                        loadVariant<double>(entry.second, "MaxReading");
                    double minReading =
                        loadVariant<double>(entry.second, "MinReading");

                    size_t pollTime = loadVariant<size_t>(entry.second, "PullTime");

                    std::vector<thresholds::Threshold> sensorThresholds;
                    if (!parseThresholdsFromConfig(pathPair.second,
                                                   sensorThresholds))
                    {
                        std::cerr << "error populating thresholds for " << name
                                  << "\n";
                    }

                    std::vector<std::string> inputParameter;
                    std::vector<std::string> inputSensorUnit;
                    std::vector<double> inputScaleFactor;
                    std::vector<std::string> inputExpression;

                    for (int i=0; i < MAX_INPUNT_PARAMETER; i++)
                    {
                        if (entry.second.find("parameter" + std::to_string(i)) == entry.second.end() ||
                            entry.second.find("sensorUnit" + std::to_string(i)) == entry.second.end() ||
                            entry.second.find("scaleFactor" + std::to_string(i)) == entry.second.end())
                        {
                            break;
                        }

                        inputParameter.push_back(loadVariant<std::string>(entry.second,"parameter" + std::to_string(i)));
                        inputSensorUnit.push_back(loadVariant<std::string>(entry.second, "sensorUnit" + std::to_string(i)));
                        inputScaleFactor.push_back(loadVariant<double>(entry.second, "scaleFactor" + std::to_string(i)));

                        if(i>0)
                        {
                            if (entry.second.find("expression" + std::to_string(i-1)) == entry.second.end())
                            {
                                break;
                            }
                            inputExpression.push_back(loadVariant<std::string>(entry.second, "expression" + std::to_string(i-1)));
                        }
                    }
                    

                    if constexpr (debug)
                    {
                        std::cerr
                            << "Configuration parsed for \n\t" << entry.first
                            << "\n"
                            << "with\n"
                            << "\tName: " << name << "\n"
                            << "\tselectMode: " << selectMode << "\n"
                            << "\tsensorUnit: " << sensorUnit << "\n"
                            << "\tpollTime: " << pollTime << "\n"
                            << "\n";
                    }

                    auto& sensor = sensors[name];

                    sensor = std::make_unique<VirtualSensor>(
                        dbusConnection, io, name, pathPair.first, objectServer,
                        std::move(sensorThresholds), maxReading, minReading, pollTime, selectMode, sensorUnit,
                         inputParameter, inputSensorUnit, inputScaleFactor, inputExpression);

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
    systemBus->request_name("xyz.openbmc_project.VirtualSensor");
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