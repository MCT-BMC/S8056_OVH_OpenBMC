/* Copyright 2020 MCT
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include "mct-special-mode-manager.hpp"

static constexpr bool DEBUG = false;

void dbusServiceInitialize()
{
    bus->request_name(obmcSpMode::busName);
    sdbusplus::asio::object_server objServer(bus);
    iface=objServer.add_interface(obmcSpMode::path,obmcSpMode::interface);

    iface->register_property(
        obmcSpMode::prop, secCtrl::convertForMessage(specialMode),
        // Ignore set
        [&](const std::string& req, std::string& propertyValue) {
            secCtrl::SpecialMode::Modes mode =
                secCtrl::SpecialMode::convertModesFromString(req);
            specialMode = mode;
            propertyValue = req;
            return 0;
        },
        // Override get
        [&](const std::string& mode) {
            return secCtrl::convertForMessage(specialMode);
        });

    iface->initialize();
}

int main(int argc, char *argv[])
{
    boost::asio::io_context io;
    bus = std::make_shared<sdbusplus::asio::connection>(io);

    dbusServiceInitialize();

    io.run();
    return 0;
}