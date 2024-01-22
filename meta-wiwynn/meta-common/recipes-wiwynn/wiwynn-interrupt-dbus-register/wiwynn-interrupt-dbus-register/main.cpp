#include "config.h"

#include <boost/asio/io_context.hpp>
#include <exception>
#include <fstream>
#include <gpiod.h>
#include <iostream>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/server.hpp>
#include <string>
#include <vector>

using json = nlohmann::json;
using namespace std;

json loadJson(const std::string& path)
{
    std::ifstream jsonFile(path);
    if (jsonFile.is_open() == false)
    {
        throw std::runtime_error("Failed to open json file");
    }

    auto data = json::parse(jsonFile, nullptr, false);
    if (data.is_discarded())
    {
        throw std::runtime_error("Failed to parse json - invalid format");
    }

    return data;
}

void registerProperty(string& name, string& signature, json& value,
                      shared_ptr<sdbusplus::asio::dbus_interface> interface)
{
    if ((signature == "y") || (signature == "UINT8"))
    {
        interface->register_property(name, (uint8_t)value,
            sdbusplus::asio::PropertyPermission::readWrite);
    }
    else if ((signature == "b") || (signature == "BOOL"))
    {
        interface->register_property(name, (bool)value,
            sdbusplus::asio::PropertyPermission::readWrite);
    }
    else if ((signature == "n") || (signature == "INT16"))
    {
        interface->register_property(name, (int16_t)value,
            sdbusplus::asio::PropertyPermission::readWrite);
    }
    else if ((signature == "q") || (signature == "UINT16"))
    {
        interface->register_property(name, (uint16_t)value,
            sdbusplus::asio::PropertyPermission::readWrite);
    }
    else if ((signature == "i") || (signature == "INT32"))
    {
        interface->register_property(name, (int32_t)value,
            sdbusplus::asio::PropertyPermission::readWrite);
    }
    else if ((signature == "u") || (signature == "UINT32"))
    {
        interface->register_property(name, (uint32_t)value,
            sdbusplus::asio::PropertyPermission::readWrite);
    }
    else if ((signature == "x") || (signature == "INT64"))
    {
        interface->register_property(name, (int64_t)value,
            sdbusplus::asio::PropertyPermission::readWrite);
    }
    else if ((signature == "t") || (signature == "UINT64"))
    {
        interface->register_property(name, (uint64_t)value,
            sdbusplus::asio::PropertyPermission::readWrite);
    }
    else if ((signature == "d") || (signature == "DOUBLE"))
    {
        interface->register_property(name, (double)value,
            sdbusplus::asio::PropertyPermission::readWrite);
    }
    else if ((signature == "s") || (signature == "STRING"))
    {
        interface->register_property(name, (string)value,
            sdbusplus::asio::PropertyPermission::readWrite);
    }
    else
    {
        cerr << "Unknown Signature: " << signature << "\n";
    }

    return;
}
int main(int argc, char **argv)
{
    json jsonData;
    try
    {
        jsonData = loadJson(std::string(GPIO_INTERRUPT_DBUS_PATH));
    }
    catch (const std::exception& e)
    {
        std::cerr << "Failed to load json file: " << e.what() << "\n";
        return 0;
    }

    // Async io context for operation.
    boost::asio::io_context io;

    vector<shared_ptr<sdbusplus::asio::connection>> buses;
    vector<shared_ptr<sdbusplus::asio::dbus_interface>> interfaces;

    for (auto &serv : jsonData)
    {
        auto servNameFind = serv.find("ServiceName");
        auto servInfoFind = serv.find("ServiceInfo");
        if ((servNameFind != serv.end()) && (servInfoFind != serv.end()))
        {
            auto bus = std::make_shared<sdbusplus::asio::connection>(io);
            string serviceName = *servNameFind;
            bus->request_name(serviceName.c_str());
            buses.push_back(bus);

            for (auto &obj : *servInfoFind)
            {
                auto objNameFind = obj.find("ObjPathName");
                auto objInfoFind = obj.find("ObjPathInfo");
                if ((objNameFind != obj.end()) && (objInfoFind != obj.end()))
                {
                    auto server = sdbusplus::asio::object_server(bus);
                    string objPathName = *objNameFind;

                    for (auto &intf : *objInfoFind)
                    {
                         auto intfNameFind = intf.find("InterfaceName");
                         auto intfInfoFind = intf.find("InterfaceInfo");
                         if ((intfNameFind != intf.end()) && (intfInfoFind != intf.end()))
                         {
                            string intfName = *intfNameFind;
                            auto interface = server.add_interface(objPathName, intfName);
                            interfaces.push_back(interface);
                            for (auto &prop : *intfInfoFind)
                            {
                                auto propNameFind = prop.find("Name");
                                auto propTypeFind = prop.find("Type");
                                auto propSignatureFind = prop.find("Signature");
                                auto propChipIdFind = prop.find("ChipId");
                                auto propGpioNumFind = prop.find("GpioNum");
                                auto propValueFind = prop.find("Value");
                                string name;
                                string type;
                                if ((propNameFind != prop.end()) &&
                                    (propTypeFind != prop.end()))
                                {
                                    name = *propNameFind;
                                    type = *propTypeFind;
                                }
                                else
                                {
                                    continue;
                                }

                                if (type == "property")
                                {
                                    if (propSignatureFind != prop.end())
                                    {
                                        string signature = *propSignatureFind;
                                        if ((propChipIdFind != prop.end()) && (propGpioNumFind != prop.end()))
                                        {
                                            string chipId = *propChipIdFind;
                                            int gpioNum = *propGpioNumFind;
                                            int value = gpiod_ctxless_get_value(chipId.c_str(), gpioNum, false, argv[0]);
                                            if (value < 0)
                                            {
                                                std::cerr << "Failed to get gpio value. chipId: " << chipId << " gpioNum: " << gpioNum << " return: " << value << "\n";
                                                continue;
                                            }
                                            prop["Value"] = value;
                                        }
                                        else if(propValueFind == prop.end())
                                        {
                                            std::cerr << "Missing Value or ChipId/GpioNum for property: " << name << "\n";
                                            continue;
                                        }
                                        registerProperty(name, signature, prop.at("Value"), interface);
                                    }
                                }
                                else if (type == "signal")
                                {
                                    // TODO: register signal
                                }
                            }
                            interface->initialize();
                         }
                    }
                }
            }
        }
    }

    io.run();

    return 0;
}


