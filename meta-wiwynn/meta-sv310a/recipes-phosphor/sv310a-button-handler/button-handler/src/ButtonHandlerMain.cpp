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
#include <boost/container/flat_set.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>

#include <filesystem>
#include <fstream>
#include <regex>

static bool acpiPowerStateOn = false;
static std::unique_ptr<sdbusplus::bus::match::match> acpiPowerMatcher = nullptr;

static std::map<std::string, int> polarityMap = {
    {"FALLING", GPIOD_LINE_REQUEST_EVENT_FALLING_EDGE},
    {"RISING", GPIOD_LINE_REQUEST_EVENT_RISING_EDGE},
    {"BOTH", GPIOD_LINE_REQUEST_EVENT_BOTH_EDGES}};

bool isAcpiPowerOn(void)
{
    return acpiPowerStateOn;
}

bool startAcpiPowerMonitor(
    const std::shared_ptr<sdbusplus::asio::connection>& conn)
{
    static boost::asio::steady_timer timer(conn->get_io_context());

    // 1. Initialize acpiPowerStateOn
    std::variant<int> state;
    auto method = conn->new_method_call(powerStateService, powerStatePath,
                                        propertyInterface, "Get");
    method.append(powerStateInterface, "pgood");
    try
    {
        auto reply = conn->call(method);
        reply.read(state);

        if (1 != std::get<int>(state))
        {
            acpiPowerStateOn = false;
        }
        else
        {
            acpiPowerStateOn = true;
        }
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        log<level::ERR>("Failed to get pgood property");
        return false;
    }

    // 2. Initialize acpiPowerStateOn
    acpiPowerMatcher = std::make_unique<sdbusplus::bus::match::match>(
        static_cast<sdbusplus::bus::bus&>(*conn),
        "type='signal',interface='org.freedesktop.DBus.Properties',"
        "member='PropertiesChanged',path='/org/openbmc/control/power0',"
        "arg0='org.openbmc.control.Power'",
        [](sdbusplus::message::message& message) {
            boost::container::flat_map<std::string, std::variant<int>> values;
            std::string objectName;
            message.read(objectName, values);

            auto findPgoodState = values.find("pgood");
            if (findPgoodState != values.end())
            {
                int PgoodState = std::get<int>(findPgoodState->second);
                if (1 != PgoodState)
                {
                    timer.cancel();
                    acpiPowerStateOn = false;
                    return;
                }

                // To avoid the power button glitch issue at early power-on
                // stage.
                timer.expires_after(std::chrono::seconds(2));
                timer.async_wait([](boost::system::error_code ec) {
                    if (ec)
                    {
                        return;
                    }
                    acpiPowerStateOn = true;
                });
            }
        });

    if (!acpiPowerMatcher)
    {
        log<level::ERR>("Failed to creat Power Matcher");
        return false;
    }

    return true;
}

bool createButtonObjects(boost::asio::io_service& io,
                         sdbusplus::asio::object_server& objectServer,
                         std::vector<std::unique_ptr<ButtonObject>>& btnObjs)
{
    std::ifstream gpioConfigFile(gpioConfigPath);

    if (!gpioConfigFile)
    {
        log<level::ERR>("Failed to open Button GPIO config file");
        return false;
    }

    auto data = nlohmann::json::parse(gpioConfigFile, nullptr, false);
    if (data.is_discarded())
    {
        log<level::ERR>("Failed to parse gpio Configure File");
        return false;
    }

    int idx = 0;
    while (!data[idx].is_null())
    {
        if ((!data[idx]["Name"].is_null() && !data[idx]["GpioNum"].is_null()))
        {
            int gpioNumber = static_cast<int>(data[idx]["GpioNum"]);
            std::string buttonName = data[idx]["Name"];
            std::string triggerType("BOTH");

            if (!data[idx]["EventMon"].is_null())
            {
                triggerType = data[idx]["EventMon"];
            }

            gpiod_line* line = nullptr;
            struct gpiod_line_request_config config
            {
                "button-handler", GPIOD_LINE_REQUEST_EVENT_BOTH_EDGES, 0
            };

            line = gpiod_line_get("0", gpioNumber);
            auto findEvent = polarityMap.find(triggerType);
            if (findEvent != polarityMap.end())
            {
                config.request_type = findEvent->second;
            }
            btnObjs.push_back(std::make_unique<ButtonObject>(
                objectServer, io, line, config, buttonName));
        }
        idx++;
    }
    return true;
}

int main(int argc, char* argv[])
{
    boost::asio::io_service io;
    auto systemBus = std::make_shared<sdbusplus::asio::connection>(io);
    systemBus->request_name("xyz.openbmc_project.ButtonHandler");
    sdbusplus::asio::object_server objectServer(systemBus);
    std::vector<std::unique_ptr<ButtonObject>> btnObjs;

    if (!startAcpiPowerMonitor(systemBus))
    {
        log<level::ERR>("Failed to start ACPI power monitor");
        return -1;
    }

    if (!createButtonObjects(io, objectServer, btnObjs))
    {
        log<level::ERR>("Failed to create Button objects");
        return -1;
    }
    io.run();

    return 0;
}
