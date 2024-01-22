/*
// Copyright (c) 2019 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

#include <EventSensor.hpp>
#include <Utils.hpp>
#include <VariantVisitors.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/container/flat_map.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/bus/match.hpp>

#include <chrono>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <vector>

constexpr const bool debug = false;

static auto sensorTypes{
    std::to_array<const char*>({"xyz.openbmc_project.Configuration.EventSensor",
                                "xyz.openbmc_project.Configuration.DiscreteSensor"})};

constexpr const char* configInterface =
    "xyz.openbmc_project.Configuration.EventSensor";
static constexpr double maxReading = 0xFF;
static constexpr double minReading = 0;

namespace systemd
{
    static constexpr const char* busName = "org.freedesktop.systemd1";
    static constexpr const char* path = "/org/freedesktop/systemd1";
    static constexpr const char* interface = "org.freedesktop.systemd1.Manager";
    static constexpr const char* request = "StartUnit";
} // namespace systemd

boost::container::flat_map<std::string, std::unique_ptr<EventSensor>> sensors;

EventSensor::EventSensor(std::shared_ptr<sdbusplus::asio::connection>& conn,
                             const std::string& objectType,
                             const std::string& sensorName,
                             const std::string& sensorType,
                             const std::string& sensorConfiguration,
                             sdbusplus::asio::object_server& objectServer,
                             std::vector<thresholds::Threshold>&& thresholdData,
                             const std::string& startUnit
                             ) :
    Sensor(escapeName(sensorName), std::move(thresholdData),
           sensorConfiguration, objectType,
           true, false, maxReading, minReading, conn),
    objectServer(objectServer),sensorType(sensorType),startUnit(startUnit)
{
    sensorInterface = objectServer.add_interface(
        "/xyz/openbmc_project/sensors/" + sensorType + "/" + name,
        "xyz.openbmc_project.Sensor.Value");

    for (const auto& threshold : thresholds)
    {
        std::string interface = thresholds::getInterface(threshold.level);
        thresholdInterfaces[static_cast<size_t>(threshold.level)] =
            objectServer.add_interface(
                "/xyz/openbmc_project/sensors/" + sensorType + "/" + name, interface);
    }
    association = objectServer.add_interface(
        "/xyz/openbmc_project/sensors/" + sensorType + "/" + name,
        association::interface);
    updateValue(1);
}

EventSensor::~EventSensor()
{
    for (const auto& iface : thresholdInterfaces)
    {
        objectServer.remove_interface(iface);
    }
    objectServer.remove_interface(sensorInterface);
    objectServer.remove_interface(association);
}

void EventSensor::init(void)
{
    setInitialProperties(sensor_paths::unitDegreesC);
    startInitService();
}

void EventSensor::startInitService(void)
{
    if(startUnit == "na")
    {
        return;
    }

    try
    {
        auto method =
            dbusConnection->new_method_call(systemd::busName, systemd::path,
                                systemd::interface,systemd::request);
        method.append(startUnit);
        method.append("replace");
        dbusConnection->call_noreply(method);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        std::cerr << "Failed to start service. ERROR=" << e.what();
    }
}

void EventSensor::checkThresholds(void)
{
    thresholds::checkThresholds(this);
}

void createSensors(
    boost::asio::io_service& io, sdbusplus::asio::object_server& objectServer,
    boost::container::flat_map<std::string, std::unique_ptr<EventSensor>>&
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
                    if(std::find(sensorTypes.begin(), sensorTypes.end(), entry.first) == sensorTypes.end())
                    {
                        continue;
                    }

                    std::string name =
                        loadVariant<std::string>(entry.second, "Name");

                    std::vector<thresholds::Threshold> sensorThresholds;

                    std::string sensorType =
                        loadVariant<std::string>(entry.second, "SensorType");

                    std::string startUnit =
                        loadVariant<std::string>(entry.second, "StartUnit");

                    if constexpr (debug)
                    {
                        std::cerr
                            << "Configuration parsed for \n\t" << entry.first
                            << "\n"
                            << "with\n"
                            << "\tName: " << name << "\n"
                            << "\tSensorType: " << sensorType << "\n"
                            << "\tStartUnit: " << startUnit << "\n";
                    }

                    auto& sensor = sensors[name];

                    sensor = std::make_unique<EventSensor>(
                        dbusConnection, entry.first, name, sensorType, pathPair.first, objectServer,
                        std::move(sensorThresholds),startUnit);

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
    systemBus->request_name("xyz.openbmc_project.EventSensor");
    sdbusplus::asio::object_server objectServer(systemBus);
    std::vector<std::unique_ptr<sdbusplus::bus::match::match>> matches;

    io.post([&]() { createSensors(io, objectServer, sensors, systemBus); });

    boost::asio::deadline_timer configTimer(io);

    std::function<void(sdbusplus::message::message&)> eventHandler =
        [&](sdbusplus::message::message&) {
            configTimer.expires_from_now(boost::posix_time::seconds(1));
            // create a timer because normally multiple properties change
            configTimer.async_wait([&](const boost::system::error_code& ec) {
                if (ec == boost::asio::error::operation_aborted)
                {
                    return; // we're being canceled
                }
                // config timer error
                if (ec)
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

    for (const char* type : sensorTypes)
    {
        auto match = std::make_unique<sdbusplus::bus::match::match>(
            static_cast<sdbusplus::bus::bus&>(*systemBus),
            "type='signal',member='PropertiesChanged',path_namespace='" +
                std::string(inventoryPath) + "',arg0namespace='" + type + "'",
            eventHandler);
        matches.emplace_back(std::move(match));
    }

    setupManufacturingModeMatch(*systemBus);
    io.run();
    return 0;
}
