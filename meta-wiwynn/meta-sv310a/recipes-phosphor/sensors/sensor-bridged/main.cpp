#include "utils.hpp"

#include <boost/asio/io_context.hpp>

#include <iostream>

struct SensorPtr
{
    std::shared_ptr<sdbusplus::asio::dbus_interface> interface;
    std::unique_ptr<sdbusplus::bus::match::match> match;
};

struct Sender
{
    std::string uniqueName;
    std::unique_ptr<sdbusplus::bus::match::match> match;
};

const std::string sensorPathPrefix = "/xyz/openbmc_project/sensors";
constexpr auto valueInterface = "xyz.openbmc_project.Sensor.Value";
const std::set<std::string> ignoreInterfaces = {
    "org.freedesktop.DBus.Introspectable", "org.freedesktop.DBus.Peer",
    "org.freedesktop.DBus.Properties",
    "xyz.openbmc_project.Association.Definitions"};

boost::asio::io_context io;
auto conn = std::make_shared<sdbusplus::asio::connection>(io);
sdbusplus::asio::object_server objectServer(conn);

std::unordered_map<DbusObjectPath, std::map<DbusInterface, SensorPtr>> sensors;
std::unordered_map<DbusObjectPath, Sender> sensorSenders;

std::string getMapperUniqueName()
{
    static std::string uniqueName;
    if (uniqueName.empty())
    {
        uniqueName = getNameOwner(conn, mapperService);
    }

    return uniqueName;
}

bool checkSender(const std::string& sender, const DbusObjectPath& path)
{
    auto findSender = sensorSenders.find(path);
    if (findSender == sensorSenders.end())
    {
        auto match = std::make_unique<sdbusplus::bus::match::match>(
            static_cast<sdbusplus::bus::bus&>(*conn),
            sdbusplus::bus::match::rules::nameOwnerChanged(sender),
            [path](sdbusplus::message::message&) {
                for (const auto& [interface, ptr] : sensors[path])
                {
                    objectServer.remove_interface(ptr.interface);
                }
                sensors.erase(path);
                sensorSenders.erase(path);
            });
        sensorSenders.emplace(path, Sender{sender, std::move(match)});
    }
    else if (findSender->second.uniqueName != sender)
    {
        return false;
    }

    return true;
}

void propertiesChangedMatches(const std::string& sender,
                              const DbusObjectPath& path,
                              const std::vector<std::string>& interfaces)
{
    for (const auto& interface : interfaces)
    {
        if (ignoreInterfaces.find(interface) != ignoreInterfaces.end())
        {
            continue;
        }

        auto& match = sensors[path][interface].match;
        match = std::make_unique<sdbusplus::bus::match::match>(
            static_cast<sdbusplus::bus::bus&>(*conn),
            sdbusplus::bus::match::rules::sender(sender) +
                sdbusplus::bus::match::rules::propertiesChanged(path,
                                                                interface),
            [](sdbusplus::message::message& message) {
                DbusObjectPath path = message.get_path();
                DbusInterface interface;
                DbusPropertyMap properties;
                message.read(interface, properties);

                auto& ptr = sensors[path][interface];
                for (const auto& [name, value] : properties)
                {
                    auto d = std::get_if<double>(&value);
                    if (d)
                    {
                        ptr.interface->set_property(name, *d);
                        continue;
                    }

                    auto b = std::get_if<bool>(&value);
                    if (b)
                    {
                        ptr.interface->set_property(name, *b);
                        continue;
                    }

                    auto s = std::get_if<std::string>(&value);
                    if (s)
                    {
                        ptr.interface->set_property(name, *s);
                        continue;
                    }

                    auto u = std::get_if<uint32_t>(&value);
                    if (u)
                    {
                        ptr.interface->set_property(name, *u);
                        continue;
                    }

                    std::cerr << "Unknown property type. path = " << path
                              << ", name = " << name << "\n";
                }
            });
    }
}

void registerInterfaces(const DbusObjectPath& path,
                        const DbusInterfaceMap& interfaces)
{
    for (const auto& [interface, properties] : interfaces)
    {
        if (ignoreInterfaces.find(interface) != ignoreInterfaces.end())
        {
            continue;
        }

        auto& intf = sensors[path][interface].interface;
        intf = objectServer.add_interface(path, interface);
        for (const auto& [name, value] : properties)
        {
            auto d = std::get_if<double>(&value);
            if (d)
            {
                intf->register_property(name, *d);
                continue;
            }

            auto b = std::get_if<bool>(&value);
            if (b)
            {
                intf->register_property(name, *b);
                continue;
            }

            auto s = std::get_if<std::string>(&value);
            if (s)
            {
                intf->register_property(name, *s);
                continue;
            }

            auto u = std::get_if<uint32_t>(&value);
            if (u)
            {
                intf->register_property(name, *u);
                continue;
            }

            std::cerr << "Unknown property type. path = " << path
                      << ", name = " << name << "\n";
        }
        intf->initialize();
    }
}

void interfacesAddedMatch()
{
    static auto match = std::make_shared<sdbusplus::bus::match::match>(
        static_cast<sdbusplus::bus::bus&>(*conn),
        sdbusplus::bus::match::rules::interfacesAdded("/") +
            sdbusplus::bus::match::rules::argNpath(0, sensorPathPrefix + "/"),
        [](sdbusplus::message::message& message) {
            DbusService sender = message.get_sender();
            // Ignore the signal from object mapper and me.
            if ((sender == conn->get_unique_name()) ||
                (sender == getMapperUniqueName()))
            {
                return;
            }

            sdbusplus::message::object_path path;
            DbusInterfaceMap interfaceMap;
            message.read(path, interfaceMap);

            // Check whether the incoming sender matches the reference sender
            // for the specified D-Bus path.
            if (!checkSender(sender, path.str))
            {
                return;
            }

            // Though the interfacesAdded signal provide property values, but
            // we still need to register PropertiesChanged match first and then
            // get property values again. If the property is changed before
            // we monitor the propertiesChanged signal, the property value will
            // be wrong before the next propertiesChanged signal comes.
            std::vector<std::string> interfaces;
            std::transform(interfaceMap.begin(), interfaceMap.end(),
                           std::back_inserter(interfaces),
                           [](std::pair<DbusInterface, DbusPropertyMap> p)
                               -> DbusInterface { return p.first; });
            propertiesChangedMatches(sender, path.str, interfaces);

            for (auto& [interface, properties] : interfaceMap)
            {
                // If GetAll is failed we still need to register all properties
                // on the D-Bus, but the value might be NaN before the next
                // PropertyChanged signal comes.
                auto newProperties = getAll(conn, sender, path.str, interface);
                if (!newProperties.empty())
                {
                    properties = newProperties;
                }
            }
            registerInterfaces(path.str, interfaceMap);
        });
}

void interfacesRemovedMatch()
{
    static auto match = std::make_shared<sdbusplus::bus::match::match>(
        static_cast<sdbusplus::bus::bus&>(*conn),
        sdbusplus::bus::match::rules::interfacesRemoved("/") +
            sdbusplus::bus::match::rules::argNpath(0, sensorPathPrefix + "/"),
        [](sdbusplus::message::message& message) {
            DbusService sender = message.get_sender();
            // Ignore the signal from object mapper and me.
            if ((sender == conn->get_unique_name()) ||
                (sender == getMapperUniqueName()))
            {
                return;
            }

            sdbusplus::message::object_path path;
            std::vector<DbusInterface> interfaces;
            message.read(path, interfaces);

            // Check whether the incoming sender matches the reference sender
            // for the specified D-Bus path.
            if (!checkSender(sender, path.str))
            {
                return;
            }

            auto& sensor = sensors[path.str];
            for (const auto& interface : interfaces)
            {
                if (sensor.find(interface) == sensor.end())
                {
                    continue;
                }

                objectServer.remove_interface(sensor[interface].interface);
                sensor.erase(interface);
            }
        });
}

void getSensors()
{
    auto subTree = getSubTree(conn, sensorPathPrefix, 0,
                              std::vector<std::string>{valueInterface});
    for (const auto& [path, objects] : subTree)
    {
        for (const auto& [service, interfaces] : objects)
        {
            // Convert well-knwon service name to D-Bus unique name.
            auto uniqueName = getNameOwner(conn, service);
            if (uniqueName.empty())
            {
                continue;
            }

            if (!checkSender(uniqueName, path))
            {
                continue;
            }

            propertiesChangedMatches(uniqueName, path, interfaces);

            DbusInterfaceMap interfaceMap;
            for (const auto& interface : interfaces)
            {
                interfaceMap[interface] =
                    getAll(conn, service, path, interface);
            }
            registerInterfaces(path, interfaceMap);
        }
    }
}

int main()
{
    conn->request_name("xyz.openbmc_project.Sensor.Bridged");

    interfacesAddedMatch();
    interfacesRemovedMatch();
    getSensors();

    io.run();
}
