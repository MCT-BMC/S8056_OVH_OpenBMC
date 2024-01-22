/*
// Copyright (c) 2020 Wiwynn Corporation
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

#include <gpiod.h>
#include <unistd.h>
#include <variant>
#include <iostream>
#include <systemd/sd-journal.h>
#include <sdbusplus/asio/object_server.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/chrono/chrono_io.hpp>
#include <boost/process.hpp>
#include <nlohmann/json.hpp>

constexpr char const *propertyInterface = "org.freedesktop.DBus.Properties";
constexpr char const *powerStateService = "org.openbmc.control.Power";
constexpr char const *powerStatePath = "/org/openbmc/control/power0";
constexpr char const *powerStateInterface = "org.openbmc.control.Power";

constexpr char const *ipmiSELService = "xyz.openbmc_project.Logging.IPMI";
constexpr char const *ipmiSELPath = "/xyz/openbmc_project/Logging/IPMI";
constexpr char const *ipmiSELAddInterface = "xyz.openbmc_project.Logging.IPMI";
const static std::string ipmiSELAddMessage = "SEL Entry";

const static std::string gpioConfigPath = "/etc/cpu-prochot-gpio.json";
const static std::string sensorPathPrefix = "/xyz/openbmc_project/sensors/processor/";

enum prochot_check_stage : uint8_t
{
    clsAssertFlag = 0,
    waitAndChkAgn
};

bool isAcpiPowerOn(void);
bool isSysPwrOk(void);

class CpuProchotObject
{
    public:
    /** @brief Constructs CPU Prochot Monitoring object.
     *
     *  @param[in] io          - io service
     *  @param[in] conn        - dbus connection
     *  @param[in] line        - GPIO line from libgpiod
     *  @param[in] config      - configuration of line with event
     *  @param[in] sensorName  - CPU prochot sensor name
     */
    CpuProchotObject(boost::asio::io_service& io,
                    std::shared_ptr<sdbusplus::asio::connection>& conn,
                    gpiod_line* line, gpiod_line_request_config& config,
                    std::string sensorName);
    ~CpuProchotObject();

    private:
        gpiod_line* gpioLine;
        gpiod_line_request_config gpioConfig;
        std::string sensorName;
        boost::asio::posix::stream_descriptor gpioEventDescriptor;
        std::shared_ptr<sdbusplus::asio::connection>& conn;
        boost::asio::steady_timer waitTimer;
        bool isProchotAssert;
        bool isProchotAssertAgain;
        uint8_t checkStage;
        uint32_t counter1;
        uint32_t counter2;

        bool getPGoodProperty();
        void waitCpuProchotEvent();
        void cpuProchotEventHandler();
        int requestCpuProchotEvent();
        void runProchotEventAlgorithm();
        void setupTimer();
        void addCpuThrottleSEL(bool isAssert);
};
