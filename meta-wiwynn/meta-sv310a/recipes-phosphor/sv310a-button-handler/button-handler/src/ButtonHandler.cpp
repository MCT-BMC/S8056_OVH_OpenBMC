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

#include "ButtonHandler.hpp"

#include <systemd/sd-journal.h>
#include <unistd.h>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>

#include <chrono>
#include <fstream>
#include <limits>
#include <queue>
#include <string>
#include <variant>

constexpr auto HOST_STATE_MANAGER_SERVICE = "xyz.openbmc_project.State.Host";
constexpr auto HOST_STATE_MANAGER_OBJ_PATH = "/xyz/openbmc_project/state/host0";
constexpr auto HOST_STATE_MANAGER_IFACE = "xyz.openbmc_project.State.Host";
constexpr auto DBUS_PROPERTY_IFACE = "org.freedesktop.DBus.Properties";
constexpr auto PROPERTY_SET_METHOD = "Set";
constexpr auto HOST_TRANSITIOIN_PROPERTY = "RequestedHostTransition";
constexpr auto RESTART_CAUSE_PROPERTY = "RestartCause";
constexpr auto RESTART_CAUSE_RESET_BTN =
    "xyz.openbmc_project.State.Host.RestartCause.ResetButton";

constexpr auto bootOptionService = "xyz.openbmc_project.BootOption";
constexpr auto bootOptionPath = "/xyz/openbmc_project/control/boot";
constexpr auto bootOptionValidIntf = "xyz.openbmc_project.Control.Valid";
// Indicate bit 0 of BMC boot flag valid bit clearing.
constexpr uint8_t dontClearButtonPowerUp = 0x01;
// Indicate bit 1 of BMC boot flag valid bit clearing.
constexpr uint8_t dontClearButtonReset = 0x02;

enum event_type : uint8_t
{
    EVENT_RESET,
    EVENT_POWER,
    EVENT_ACPI
};

enum gpio_line_event_type : uint8_t
{
    RISING_EDGE = 1,
    FALLING_EDGE
};

static void clearBootFlagsValid(uint8_t dontClearBit)
{
    auto bus = sdbusplus::bus::new_default_system();
    uint8_t bootFlagValidBitClearing = 0;
    try
    {
        auto msg = bus.new_method_call(bootOptionService, bootOptionPath,
                                       DBUS_PROPERTY_IFACE, "Get");
        msg.append(bootOptionValidIntf, "BootFlagValidBitClearing");
        auto reply = bus.call(msg);
        std::variant<uint8_t> variant;
        reply.read(variant);
        bootFlagValidBitClearing = std::get<uint8_t>(variant);
    }
    catch (const std::exception& e)
    {
        log<level::ERR>("Failed to get BootFlagValidBitClearing",
                        entry("ERROR=%s", e.what()));
    }

    // Clear Boot Flags Valid.
    if (!(bootFlagValidBitClearing & dontClearBit))
    {
        try
        {
            auto msg = bus.new_method_call(bootOptionService, bootOptionPath,
                                           DBUS_PROPERTY_IFACE, "Set");
            msg.append(bootOptionValidIntf, "BootFlagsValid",
                       std::variant<bool>(false));
            bus.call_noreply(msg);
        }
        catch (const std::exception& e)
        {
            log<level::ERR>("Failed to set BootFlagsValid",
                            entry("ERROR=%s", e.what()));
        }
    }
}

ButtonObject::ButtonObject(sdbusplus::asio::object_server& objectServer, boost::asio::io_service& io,
                           gpiod_line* line, gpiod_line_request_config& config,
                           std::string buttonName) :
    busConn(std::make_shared<sdbusplus::asio::connection>(
            io, sdbusplus::bus::new_system().release())),
    gpioLine(line),
    gpioConfig(config), gpioEventDescriptor(io),
    buttonName(buttonName), acOffTimer(io), cancelAcOff(false), isEnabled(true)
{
    std::string dbusPath = buttonObjPathPrefix + buttonName;
    std::string dbusIntf = buttonInterfacePrefix + buttonName;
    buttonObjInterface = objectServer.add_interface(
        dbusPath, dbusIntf);

    buttonObjInterface->register_property("Enabled", true,
    [&](const bool& newValue, bool& oldValue) {
        return setButtonState(newValue, oldValue);
    });

    if (!buttonObjInterface->initialize())
    {
        log<level::ERR>("Failed to initialize Enabled interface");
    }

    int res = requestButtonEvent();
    if (res < 0)
    {
        log<level::ERR>("Failed to Monitor GPIO,",
                        entry("BUTTONNAME=%s", buttonName.c_str()));
    }
}

ButtonObject::~ButtonObject()
{
    gpiod_line_release(gpioLine);
    acOffTimer.cancel();
}

bool ButtonObject::setButtonState(const bool& newValue, bool& oldValue)
{
     oldValue = newValue;
     isEnabled = newValue;
     return true;
}

void ButtonObject::waitButtonEvent()
{
    gpioEventDescriptor.async_wait(
        boost::asio::posix::stream_descriptor::wait_read,
        [this](const boost::system::error_code& ec) {
            if (ec)
            {
                return;
            }
            buttonEventHandler();
        });
}

void ButtonObject::buttonEventHandler()
{
    gpiod_line_event gpioLineEvent;
    int rc = gpiod_line_event_read_fd(gpioEventDescriptor.native_handle(),
                                      &gpioLineEvent);
    if (rc < 0)
    {
        log<level::ERR>("Failed to read GPIO event from file descriptor",
                        entry("BUTTONNAME=%s", buttonName.c_str()));
        return;
    }

    if (isEnabled)
    {
        if (buttonName == "RSTBTN")
        {
            if (gpioLineEvent.event_type == FALLING_EDGE)
            {
                addBtnSEL(EVENT_RESET);
            }
            else if (gpioLineEvent.event_type == RISING_EDGE)
            {
                if (::isAcpiPowerOn())
                {
                    initiateHostStateTransition(State::Host::Transition::ForceWarmReboot);

                    std::string restartCauseStr = State::Host::convertRestartCauseToString(
                        State::Host::RestartCause::ResetButton);
                    setRestartCause(restartCauseStr);
                    clearBootFlagsValid(dontClearButtonReset);
                }
            }
        }
        else if (buttonName == "PWRBTN")
        {
            if (gpioLineEvent.event_type == FALLING_EDGE)
            {
                addBtnSEL(EVENT_POWER);
                pressedTime = std::chrono::steady_clock::now();
                setupAcOffTimer();
            }
            else if (gpioLineEvent.event_type == RISING_EDGE)
            {
                if (::isAcpiPowerOn())
                {
                    auto now = std::chrono::steady_clock::now();
                    auto durationTime =
                        std::chrono::duration_cast<std::chrono::milliseconds>(
                            now - pressedTime);
                    if (durationTime < std::chrono::milliseconds(PWR_ACOFF_TIME_MS))
                    {
                        cancelAcOff = true;
                        acOffTimer.cancel();
                    }

                    if ((durationTime >=
                         std::chrono::milliseconds(PWR_SOFTOFF_TIME_MS)) &&
                        (durationTime <
                         std::chrono::milliseconds(PWR_HARDOFF_TIME_MS)))
                    {
                        initiateHostStateTransition(State::Host::Transition::SoftOff);
                    }
                    else if ((durationTime >=
                              std::chrono::milliseconds(PWR_HARDOFF_TIME_MS)) &&
                             (durationTime <
                              std::chrono::milliseconds(PWR_ACOFF_TIME_MS)))
                    {
                        addBtnSEL(EVENT_ACPI);
                        initiateHostStateTransition(State::Host::Transition::Off);
                    }
                }
                else
                {
                    cancelAcOff = true;
                    acOffTimer.cancel();
                    initiateHostStateTransition(State::Host::Transition::On);
                    std::string restartCauseStr =
                        State::Host::convertRestartCauseToString(
                            State::Host::RestartCause::PowerButton);
                    setRestartCause(restartCauseStr);
                    clearBootFlagsValid(dontClearButtonPowerUp);
                }
            }
        }
    }
    waitButtonEvent();
    return;
}

int ButtonObject::requestButtonEvent()
{
    if (gpiod_line_request(gpioLine, &gpioConfig, 0) < 0)
    {
        log<level::ERR>("Failed to request powerbutton line",
                         entry("BUTTONNAME=%s", buttonName.c_str()));
        return -1;
    }

    int gpioLineFd = gpiod_line_event_get_fd(gpioLine);
    if (gpioLineFd < 0)
    {
        log<level::ERR>("Failed to get the event file descriptor",
                         entry("BUTTONNAME=%s", buttonName.c_str()));
        gpiod_line_release(gpioLine);
        return -1;
    }

    gpioEventDescriptor.assign(gpioLineFd);

    waitButtonEvent();

    return 0;
}

void ButtonObject::setupAcOffTimer()
{
    acOffTimer.expires_from_now(std::chrono::steady_clock::duration(
        std::chrono::milliseconds(PWR_ACOFF_TIME_MS)));
    acOffTimer.async_wait([&](const boost::system::error_code& ec) {
        if (ec == boost::asio::error::operation_aborted)
        {
            return; // we're being canceled
        }
        if (!cancelAcOff)
        {
            // Send power off command to MB-HSC(ADM1278)
            int res = -1;
            std::vector<char> filename;
            filename.assign(20, 0);

            // Open I2C Device
            int fd = open_i2c_dev(HSC_BUS, filename.data(), filename.size(), 0);
            if (fd < 0)
            {
                log<level::ERR>("Fail to open I2C device");
                return;
            }

            uint8_t busAddr = HSC_ADDR << 1;
            std::vector<uint8_t> crcData = {busAddr, CMD_OPERATION,
                                            HCS_OUTPUT_DISABLE};
            uint8_t crcValue = i2c_smbus_pec(crcData.data(), crcData.size());

            // Send HSC Operation command (Hot swap output disabled)
            std::vector<uint8_t> cmd_data = {CMD_OPERATION, HCS_OUTPUT_DISABLE,
                                             crcValue};
            res = i2c_master_write(fd, HSC_ADDR, cmd_data.size(),
                                   cmd_data.data());
            if (res < 0)
            {
                log<level::ERR>("Fail to r/w I2C device");
                close_i2c_dev(fd);
                return;
            }
            close_i2c_dev(fd);
        }
        else
        {
            cancelAcOff = false;
        }
    });
}

void ButtonObject::addBtnSEL(int eventType)
{
    uint16_t genId = 0x20;
    bool assert = true;
    std::string dbusPath = sensorPathPrefix + "Button";
    std::vector<uint8_t> eventData(3, 0xFF);
    switch (eventType)
    {
        case EVENT_RESET:
            eventData[0] = 0x2;
            break;
        case EVENT_POWER:
            eventData[0] = 0x0;
            break;
        case EVENT_ACPI:
            dbusPath = sensorPathPrefix + "ACPI_Power_State";
            eventData[0] = 0xA;
            break;
        default:
            break;
    }

    sdbusplus::message::message writeSEL = busConn->new_method_call(
        ipmiSELService, ipmiSELPath, ipmiSELAddInterface, "IpmiSelAdd");
    writeSEL.append(ipmiSELAddMessage, dbusPath, eventData, assert, genId);

    try
    {
        busConn->call_noreply(writeSEL);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        log<level::ERR>("Error to Add Sel",
                        entry("BUTTON_NAME=%s", buttonName.c_str()),
                        entry("ERROR=%s", e.what()));
    }

    return;
}

int ButtonObject::initiateHostStateTransition(
    State::Host::Transition transition)
{
    auto bus = sdbusplus::bus::new_default_system();
    auto request = State::convertForMessage(transition);

    try
    {
        auto method = bus.new_method_call(
            HOST_STATE_MANAGER_SERVICE, HOST_STATE_MANAGER_OBJ_PATH,
            DBUS_PROPERTY_IFACE, PROPERTY_SET_METHOD);
        method.append(HOST_STATE_MANAGER_IFACE, HOST_TRANSITIOIN_PROPERTY,
                      std::variant<std::string>(request));

        bus.call(method);
    }
    catch (sdbusplus::exception::SdBusError& e)
    {
        log<level::ERR>("Failed to initiate transition",
                        entry("ERROR=%s", e.what()));
        return -1;
    }
    return 0;
}

int ButtonObject::setRestartCause(std::string cause)
{
    auto bus = sdbusplus::bus::new_default_system();
    auto method = bus.new_method_call(HOST_STATE_MANAGER_SERVICE,
                                      HOST_STATE_MANAGER_OBJ_PATH,
                                      DBUS_PROPERTY_IFACE, PROPERTY_SET_METHOD);
    method.append(HOST_STATE_MANAGER_IFACE, RESTART_CAUSE_PROPERTY,
                  std::variant<std::string>(cause));

    try
    {
        auto reply = bus.call(method);
    }
    catch (sdbusplus::exception_t& e)
    {
        log<level::ERR>("Fail to set restart cause",
                        entry("ERROR=%s", e.what()));
        return -1;
    }

    return 0;
}
