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

#include <unistd.h>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <fstream>
#include <limits>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <string>
#include <systemd/sd-journal.h>
#include <queue>
#include <chrono>
#include <variant>
#include <xyz/openbmc_project/State/Host/server.hpp>

// Phosphor Host State manager
namespace State = sdbusplus::xyz::openbmc_project::State::server;

static std::chrono::milliseconds pollInterval(500);
static int power_pin = 35;
static int reset_pin = 33;
static uint32_t power_off_times = 4;    // 2 seconds
static uint32_t power_force_times = 12;    // 6 seconds
static int pressEvent = 100;
static int releaseEvent = 0;
bool power_off_set = false;
std::queue<int> powerBtnEventQueue; // press=100,release=0

constexpr auto RESTART_CAUSE_SERVICE = "xyz.openbmc_project.State.Host";
constexpr auto RESTART_CAUSE_OBJ_PATH = "/xyz/openbmc_project/state/host0";
constexpr auto RESTART_CAUSE_INTERFACE = "xyz.openbmc_project.State.Host";
constexpr auto PROPERTY_INTERFACE = "org.freedesktop.DBus.Properties";
constexpr auto PROPERTY_SET_METHOD = "Set";
constexpr auto RESTART_CAUSE_PROPERTY = "RestartCause";
constexpr auto RESTART_CAUSE_RESET_BTN = "xyz.openbmc_project.State.Host.RestartCause.ResetButton";

enum event_type : uint8_t
{
    EVENT_RESET,
    EVENT_POWER_ON,
    EVENT_POWER_OFF,
    EVENT_POWER_FORCE,
    EVENT_POWER_NOACTION
};

enum gpio_line_event_type : uint8_t
{
    RISING_EDGE = 1,
    FALLING_EDGE
};

ButtonObject::ButtonObject(boost::asio::io_service& io,
    std::shared_ptr<sdbusplus::asio::connection>& conn,
    gpiod_line* line, gpiod_line_request_config& config,
    std::string sensorName):
    gpioLine(line), gpioConfig(config), gpioEventDescriptor(io),
    conn(conn), sensorName(sensorName), waitTimer(io)
{
    int res = requestButtonEvent();
    if(0 == res && sensorName == "POWER_BUTTON") // only power_button need to run algorithm
    {
        setupTimer();
    }
}

ButtonObject::~ButtonObject()
{
    gpiod_line_release(gpioLine);
    waitTimer.cancel();
}

void ButtonObject::waitButtonEvent()
{
    gpioEventDescriptor.async_wait(
        boost::asio::posix::stream_descriptor::wait_read,
        [this](const boost::system::error_code& ec) {
            if (ec)
            {
                std::cerr << "testtGPIO event read error: "<< sensorName <<"\n";
                return;
            }
            buttonEventHandler();
        });
}

void ButtonObject::buttonEventHandler()
{
    gpiod_line_event gpioLineEvent;
    int ress = gpiod_line_event_read_fd(gpioEventDescriptor.native_handle(), &gpioLineEvent);
    if (ress < 0)
    {
        std::cerr << "testtFailed to read gpioLineEvent from fd: "<< sensorName <<"\n";
    }

    /* Reset pin be set*/
    if (sensorName == "RESET_BUTTON")
    {
        addBtnSEL(EVENT_RESET);
        std::string resetService("host-powerreset.service");
        startSystemdUnit(resetService);

        std::string restartCauseStr =
            State::Host::convertRestartCauseToString(State::Host::RestartCause::ResetButton);
        setRestartCause(restartCauseStr);
    }
    /* Push power button event to queue*/
    else if (sensorName == "POWER_BUTTON")
    {

        if (gpioLineEvent.event_type == FALLING_EDGE )
        {
            powerBtnEventQueue.push(pressEvent);
        }
        else if (gpioLineEvent.event_type == RISING_EDGE )
        {
            powerBtnEventQueue.push(releaseEvent);
        }
    }

    waitButtonEvent();
    return;
}

int ButtonObject::requestButtonEvent()
{
    if (gpiod_line_request(gpioLine, &gpioConfig, 0) < 0)
    {
        std::cerr << "testtFailed to request powerbutton line: "<< sensorName <<"\n";
        return -1;
    }

    int gpioLineFd = gpiod_line_event_get_fd(gpioLine);
    if (gpioLineFd < 0)
    {
        std::cerr << "testtFailed to get line fd: "<< sensorName <<"\n";
        return -1;
    }

    gpioEventDescriptor.assign(gpioLineFd);

    waitButtonEvent();

    return 0;
}

void ButtonObject::runPowerButtonEventAlgorithm()
{
    int last_press = 0;
    int tmp_status = 0;
    bool hadpoweroff = false;

    bool on1 = ::isAcpiPowerOn();

    if (!on1) // off
    {
        bool hadPowerOn = false;
        while (!powerBtnEventQueue.empty())
        {
            tmp_status = powerBtnEventQueue.front();
            powerBtnEventQueue.pop();

            if (power_off_set == true)  // power off from force shutdown
            {
                power_off_set = false;
                addBtnSEL(EVENT_POWER_FORCE);
            }
            else // when power off bypass all signals
            {
                if (tmp_status >= pressEvent && hadPowerOn == false)
                {
                    gpiod_ctxless_set_value("0", power_pin, 0, false, "power-button-event", NULL, NULL);
                    addBtnSEL(EVENT_POWER_ON);
                    std::string restartCauseStr =
                        State::Host::convertRestartCauseToString(State::Host::RestartCause::PowerButton);
                    setRestartCause(restartCauseStr);
                    // Generate at lest 100ms pulse
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
                else if (tmp_status < pressEvent && hadPowerOn == false)
                {
                    gpiod_ctxless_set_value("0", power_pin, 1, false, "power-button-event", NULL, NULL);
                    hadPowerOn = true;
                    power_off_set = false;
                }
            }
        }
    }
    else // on
    {
        while (!powerBtnEventQueue.empty())
        {
            tmp_status = powerBtnEventQueue.front();
            powerBtnEventQueue.pop();
            int press_elapsed = 0;  // count elapse time from press till now

            if (tmp_status >= pressEvent) // press event
            {
                last_press = tmp_status;
                press_elapsed = tmp_status - pressEvent;

                if ((press_elapsed >= power_off_times) && (power_off_set == false))
                {
                    gpiod_ctxless_set_value("0", power_pin, 0, false, "power-button-event", NULL, NULL);
                    addBtnSEL(EVENT_POWER_OFF);
                    power_off_set = true;
                }
            }
            else  //release event
            {
                press_elapsed = last_press - tmp_status - pressEvent;
                if (press_elapsed < power_off_times)
                {
                    addBtnSEL(EVENT_POWER_NOACTION);
                    std::cerr << "testpassLess2s\n";
                }
                else
                {
                    gpiod_ctxless_set_value("0", power_pin, 1, false, "power-button-event", NULL, NULL);
                    power_off_set = false;
                    if (press_elapsed >= power_force_times)
                    {
                        addBtnSEL(EVENT_POWER_FORCE);
                    }
                    last_press = 0;
                    tmp_status = 0;
                }
            }
        }

        if (tmp_status != 0  ) // still press add back to queue
        {
            powerBtnEventQueue.push(tmp_status + 1);
        }
    }

    return;
}

void ButtonObject::setupTimer()
{
    runPowerButtonEventAlgorithm();

    waitTimer.expires_from_now(std::chrono::steady_clock::duration(pollInterval));
    waitTimer.async_wait([&](const boost::system::error_code& ec) {
    if (ec == boost::asio::error::operation_aborted)
    {
        return; // we're being canceled
    }
        setupTimer();
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

        case EVENT_POWER_ON:
            eventData[0] = 0x0;
        break;

        case EVENT_POWER_OFF:
            eventData[0] = 0x0;
        break;

        case EVENT_POWER_NOACTION:
            eventData[0] = 0x0;
        break;

        case EVENT_POWER_FORCE:
            dbusPath = sensorAcpiPath;
            eventData[0] = 0xA;
        break;

        default:
        break;
    }

    sdbusplus::message::message selWrite = conn->new_method_call(
            ipmiSELService, ipmiSELPath, ipmiSELAddInterface, "IpmiSelAdd");
    selWrite.append(ipmiSELAddMessage, dbusPath, eventData, assert, genId);

    try
    {
        conn->call_noreply(selWrite);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        std::cerr << "Error in " << __func__ << "\n";
    }

    return;
}

int ButtonObject::startSystemdUnit(const std::string sysUnit)
{
    constexpr auto SYSTEMD_SERVICE = "org.freedesktop.systemd1";
    constexpr auto SYSTEMD_OBJ_PATH = "/org/freedesktop/systemd1";
    constexpr auto SYSTEMD_INTERFACE = "org.freedesktop.systemd1.Manager";
    auto bus = sdbusplus::bus::new_default_system();
    try
    {
        auto method = bus.new_method_call(SYSTEMD_SERVICE, SYSTEMD_OBJ_PATH,
                                            SYSTEMD_INTERFACE, "StartUnit");
        method.append(sysUnit, "replace");
        bus.call_noreply(method);
    }
    catch (std::exception& e)
    {
        std::cerr << "Failed to start systemd unit" << "\n";
        return -1;
    }

    return 0;
}

int ButtonObject::setRestartCause(std::string cause)
{
    auto bus = sdbusplus::bus::new_default_system();
    auto method = bus.new_method_call(RESTART_CAUSE_SERVICE,
                                    RESTART_CAUSE_OBJ_PATH,
                                    PROPERTY_INTERFACE, 
                                    PROPERTY_SET_METHOD);
    method.append(RESTART_CAUSE_INTERFACE, RESTART_CAUSE_PROPERTY, std::variant<std::string>(cause));

    try
    {
        auto reply = bus.call(method);
    }
    catch (sdbusplus::exception_t& e)
    {
        std::cerr << "Fail to set restart cause:" << e.what() << "\n";
        return -1;
    }

    return 0;
}
