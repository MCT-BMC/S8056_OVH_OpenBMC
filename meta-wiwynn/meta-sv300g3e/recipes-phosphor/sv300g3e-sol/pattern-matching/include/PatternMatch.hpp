/*
// Copyright (c) 2019 Wiwynn Corporation
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

#pragma once

#include <unistd.h>
#include <variant>
#include <iostream>
#include <systemd/sd-journal.h>
#include <sdbusplus/asio/object_server.hpp>
#include <boost/process.hpp>
#include <nlohmann/json.hpp>
#include <boost/asio/steady_timer.hpp>

static const std::string sensorPathPrefix = "/xyz/openbmc_project/sensors/";
static const std::string solValueInterface = "xyz.openbmc_project.Sensor.Value";
static const std::string solPatternInterface = "xyz.openbmc_project.Sensor.SOLPattern";

static const std::string patternFilePath = "/etc/sol-pattern.json";
static const int maxPatternNum = 4;
static const nlohmann::json defaultPattern =
{
    {"Pattern1", ""},
    {"Pattern2", ""},
    {"Pattern3", ""},
    {"Pattern4", ""}
};

class SolPatternSensor
{
    public:
        SolPatternSensor(sdbusplus::asio::object_server& objectServer,
                        std::shared_ptr<sdbusplus::asio::connection>& conn,
                        boost::asio::io_service& io, const std::string& sensorType,
                        const std::string& sensorName, const std::string& ptnString);
        ~SolPatternSensor();

    private:
        sdbusplus::asio::object_server& objServer;
        boost::asio::steady_timer waitTimer;
        std::shared_ptr<sdbusplus::asio::dbus_interface> sensorInterface;
        std::shared_ptr<sdbusplus::asio::dbus_interface> patternInterface;
        std::string patternName;
        std::string patternString;
        std::string matchingCmd;
        unsigned int hitCount;
        unsigned int newHitCount;
        int errCount;

        bool setHitCount(const unsigned int& newValue, unsigned int& oldValue);
        bool setPattern(const std::string& newValue, std::string& oldValue);
        bool updateHitCount(const unsigned int& newValue);

        void setupMatching(void);
        bool runMatching(void);
        void handleResponse(void);
};
