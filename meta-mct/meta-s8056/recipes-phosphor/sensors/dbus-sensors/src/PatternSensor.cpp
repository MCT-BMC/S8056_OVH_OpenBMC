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

#include <unistd.h>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>

#include <iostream>
#include <fstream>
#include <limits>
#include <string>

static constexpr size_t warnAfterErrorCount = 10;

SolPatternSensor::SolPatternSensor(std::shared_ptr<sdbusplus::asio::connection>& conn,
                             boost::asio::io_service& io,
                             const std::string& sensorName,
                             const std::string& sensorConfiguration,
                             sdbusplus::asio::object_server& objectServer,
                             std::vector<thresholds::Threshold>&& thresholdData,
                             double max, double min,std::string patternInfoString) :
    Sensor(escapeName(sensorName),
           std::move(thresholdData), sensorConfiguration,
           "xyz.openbmc_project.Configuration.SolPatternSensor", true, false,
           max, min, conn),
    objServer(objectServer), waitTimer(io),patternInfoString(patternInfoString)
{
    sensorInterface = objectServer.add_interface(
        "/xyz/openbmc_project/sensors/pattern/" + name,
        "xyz.openbmc_project.Sensor.Value");

    for (const auto& threshold : thresholds)
    {
        std::string interface = thresholds::getInterface(threshold.level);
        thresholdInterfaces[static_cast<size_t>(threshold.level)] =
            objectServer.add_interface(
                "/xyz/openbmc_project/sensors/pattern/" + name, interface);
    }
    association = objectServer.add_interface(
        "/xyz/openbmc_project/sensors/pattern/" + name,
        association::interface);
    
    sensorInterface->register_property("Pattern", patternInfoString,
        [&](const std::string& newValue, std::string& oldValue) {
            return setPattern(newValue, oldValue);
        });

    setInitialProperties(sensor_paths::unitPercent);
}

SolPatternSensor::~SolPatternSensor()
{
    waitTimer.cancel();
    for (const auto& iface : thresholdInterfaces)
    {
        objServer.remove_interface(iface);
    }
    objServer.remove_interface(sensorInterface);
    objServer.remove_interface(association);
}

void SolPatternSensor::checkThresholds(void)
{
    thresholds::checkThresholds(this);
}

void SolPatternSensor::updatePaternHandler(void)
{
    auto bus = sdbusplus::bus::new_default();
    auto msg = bus.new_signal("/", "org.freedesktop.DBus", "SolPatternUpdate");
    bool parameter = true;
    msg.append(parameter);
    msg.signal_send();
}

bool SolPatternSensor::setPattern(const std::string& newValue, std::string& oldValue)
{
    oldValue = newValue;
    patternInfoString = newValue;

    std::ifstream patternFileInput(patternFilePath);
    if(!patternFileInput)
    {
        return false;
    }

    nlohmann::json patternInfo = nlohmann::json::parse(patternFileInput, nullptr, false);
    patternFileInput.close();

    if(patternInfo.find(name.c_str()) != patternInfo.end())
    {
        patternInfo[name] = newValue;
        std::ofstream patternFileOutput(patternFilePath);
        if(patternFileOutput)
        {
            patternFileOutput << patternInfo << std::endl;;
            patternFileOutput.close();
        }
    }

    updatePaternHandler();

     return true;
}
