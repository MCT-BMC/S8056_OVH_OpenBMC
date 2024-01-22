/*
// Copyright (c) 2021 MiTAC Corporation
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
#include <PatternSensor.hpp>
#include "Utils.hpp"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/container/flat_set.hpp>
#include <boost/container/flat_map.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/bus/match.hpp>

#include <functional>
#include <fstream>
#include <memory>
#include <string>
#include <variant>
#include <vector>

constexpr const bool debug = false;

constexpr const char* configInterface =
    "xyz.openbmc_project.Configuration.SolPatternSensor";

void createSensors(
    boost::asio::io_service& io, sdbusplus::asio::object_server& objectServer,
    boost::container::flat_map<std::string, std::unique_ptr<SolPatternSensor>>&
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
                    uint8_t maxSensor =
                        loadVariant<uint8_t>(entry.second, "MaxSensor");
                    double max =
                        loadVariant<double>(entry.second, "MaxValue");
                    double min =
                        loadVariant<double>(entry.second, "MinValue");

                    std::vector<thresholds::Threshold> sensorThresholds;
                    if (!parseThresholdsFromConfig(pathPair.second,
                                                   sensorThresholds))
                    {
                        std::cerr << "error populating thresholds for " << name
                                  << "\n";
                    }

                    if constexpr (debug)
                    {
                        std::cerr
                            << "Configuration parsed for \n\t" << entry.first
                            << "\n"
                            << "with\n"
                            << "\tName: " << name << "\n"
                            << "\tMax Sensor: " << static_cast<int>(maxSensor) << "\n";
                    }

                    nlohmann::json patternInfo;

                    std::ifstream patternFileInput(patternFilePath);
                    if(!patternFileInput)
                    {
                        // Create the default pattern file wheb file does not exist.
                        for(int i=0; i<maxSensor; i++)
                        {
                            std::string sensorName = name + std::to_string((i+1));
                            patternInfo[sensorName] = "";
                        }
                        std::ofstream patternFileOutput(patternFilePath);
                        patternFileOutput << patternInfo << std::endl;
                        patternFileOutput.close();
                    }
                    else
                    {
                        patternInfo = nlohmann::json::parse(patternFileInput, nullptr, false);
                    }

                    for(int i=0; i<maxSensor; i++)
                    {
                        std::string sensorName = name + std::to_string((i+1));
                        auto& sensor = sensors[sensorName];
                        sensor = std::make_unique<SolPatternSensor>(
                            dbusConnection, io, sensorName, pathPair.first, objectServer,
                            std::move(sensorThresholds), max, min, patternInfo[sensorName]);
                        sensor->updatePaternHandler();
                    }
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
    systemBus->request_name("xyz.openbmc_project.SolPatternSensor");
    sdbusplus::asio::object_server objectServer(systemBus);
    boost::container::flat_map<std::string, std::unique_ptr<SolPatternSensor>> sensors;

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
