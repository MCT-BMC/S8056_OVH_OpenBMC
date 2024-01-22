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
#include <openbmc/libobmci2c.h>
#include <systemd/sd-journal.h>
#include <unistd.h>

#include <boost/asio/io_service.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/process.hpp>
#include <nlohmann/json.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/bus.hpp>
#include <xyz/openbmc_project/State/Host/server.hpp>

#include <iostream>
#include <variant>

static const std::string buttonInterfacePrefix = "xyz.openbmc_project.ButtonHandler.Buttons.";
static const std::string buttonObjPathPrefix = "/xyz/openbmc_project/ButtonHandler/Buttons/";

constexpr char const* propertyInterface = "org.freedesktop.DBus.Properties";
constexpr char const* powerStateService = "org.openbmc.control.Power";
constexpr char const* powerStatePath = "/org/openbmc/control/power0";
constexpr char const* powerStateInterface = "org.openbmc.control.Power";

constexpr char const* ipmiSELService = "xyz.openbmc_project.Logging.IPMI";
constexpr char const* ipmiSELPath = "/xyz/openbmc_project/Logging/IPMI";
constexpr char const* ipmiSELAddInterface = "xyz.openbmc_project.Logging.IPMI";
const static std::string ipmiSELAddMessage = "SEL Entry";

const static std::string gpioConfigPath = "/etc/button-gpio.json";
const static std::string sensorPathPrefix =
    "/xyz/openbmc_project/sensors/discrete/";

constexpr uint8_t HSC_BUS = 5;
constexpr uint8_t HSC_ADDR = 0x11; // 7-bit address
constexpr uint8_t CMD_OPERATION = 0x01;
constexpr uint8_t HCS_OUTPUT_DISABLE = 0x0;

// Phosphor Host State manager
namespace State = sdbusplus::xyz::openbmc_project::State::server;
using namespace phosphor::logging;

enum prochot_check_stage : uint8_t
{
    clsAssertFlag = 0,
    waitAndChkAgn
};

bool isAcpiPowerOn(void);

class ButtonObject
{
  public:
    /** @brief Constructs CPU Prochot Monitoring object.
     *
     *  @param[in] io          - io service
     *  @param[in] conn        - dbus connection
     *  @param[in] line        - GPIO line from libgpiod
     *  @param[in] config      - configuration of line with event
     *  @param[in] buttonName  - Power Buttton Name
     */
    ButtonObject(sdbusplus::asio::object_server& objectServer, boost::asio::io_service& io,
                 gpiod_line* line, gpiod_line_request_config& config,
                 std::string buttonName);
    ~ButtonObject();

  private:
    std::shared_ptr<sdbusplus::asio::connection> busConn;
    std::shared_ptr<sdbusplus::asio::dbus_interface> buttonObjInterface;
    gpiod_line* gpioLine;
    gpiod_line_request_config gpioConfig;
    std::string buttonName;
    boost::asio::posix::stream_descriptor gpioEventDescriptor;
    boost::asio::steady_timer acOffTimer;
    decltype(std::chrono::steady_clock::now()) pressedTime;
    bool cancelAcOff;
    bool isEnabled;

    bool setButtonState(const bool& newValue, bool& oldValue);
    void waitButtonEvent();
    void buttonEventHandler();
    int requestButtonEvent();
    void setupAcOffTimer();
    void addBtnSEL(int eventType);
    int initiateHostStateTransition(State::Host::Transition transition);
    int startSystemdUnit(const std::string sysUnit);
    int setRestartCause(std::string cause);
};
