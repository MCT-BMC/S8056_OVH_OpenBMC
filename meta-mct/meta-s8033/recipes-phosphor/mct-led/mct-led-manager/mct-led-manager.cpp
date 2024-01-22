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

#include "mct-led-manager.hpp"

static constexpr bool DEBUG = false;

static void setProperty(std::shared_ptr<sdbusplus::asio::connection>& bus,
                        const std::string& service, const std::string& path, const std::string& interface,
                        const std::string& property, const std::string& value)
{
    auto method = bus->new_method_call(service.c_str(), path.c_str(), PROPERTY_INTERFACE, "Set");

    try
    {
        method.append(interface, property, std::variant<std::string>(value));
        bus->call_noreply(method);
    }
    catch (std::exception& e)
    {
        std::cerr << "Failed to set property:" << e.what() << "\n";
        setProperty(bus,service,path,interface,property,value);
        sleep(3);
    }
}

void setFaultLed(int enable)
{
    if(enable > 0)
    {
        setProperty(bus, sysLed::busName, sysLed::path, sysLed::interface, "State", LED_ENABLE);
        setProperty(bus, hwLed::busName, hwLed::path, hwLed::interface, "State", LED_ENABLE);
    }
    else
    {
        setProperty(bus, sysLed::busName, sysLed::path, sysLed::interface, "State", LED_DISABLE);
        setProperty(bus, hwLed::busName, hwLed::path, hwLed::interface, "State", LED_DISABLE);
    }

}

int increaseFaultLed(void)
{
    faultLed++;
    iface->set_property(led::faultLedprop, faultLed);
    setFaultLed(faultLed);
    return faultLed;
}

int decreaseFaultLed(void)
{
    if(faultLed > 0){
        faultLed--;
        iface->set_property(led::faultLedprop, faultLed);
    }
    setFaultLed(faultLed);
    return faultLed;
}

void dbusServiceInitialize()
{
    bus->request_name(led::busName);
    sdbusplus::asio::object_server objServer(bus);
    iface=objServer.add_interface(led::path,led::interface);
    iface->register_method(led::increaseFaultLedMethod, increaseFaultLed);
    iface->register_method(led::decreaseFaultLedMethod, decreaseFaultLed);
    iface->register_property(led::faultLedprop, faultLed,
                             sdbusplus::asio::PropertyPermission::readWrite);
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