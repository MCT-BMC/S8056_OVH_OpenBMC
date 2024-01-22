#include "utils.hpp"

#include <iostream>

GetSubTreeType
    getSubTree(const std::shared_ptr<sdbusplus::asio::connection>& conn,
               const std::string& path, const int depth,
               const std::vector<std::string>& interfaces)
{
    GetSubTreeType subTree;
    try
    {
        auto message = conn->new_method_call(mapperService, mapperPath,
                                             mapperInterface, "GetSubTree");
        message.append(path, depth, interfaces);
        auto reply = conn->call(message);
        reply.read(subTree);
    }
    catch (const sdbusplus::exception_t& e)
    {
        std::cerr << "Failed to GetSubTree. path = " << path
                  << ", error = " << e.what() << "\n";
    }

    return subTree;
}

std::string
    getNameOwner(const std::shared_ptr<sdbusplus::asio::connection>& conn,
                 const std::string& name)
{
    std::string owner;
    try
    {
        auto message = conn->new_method_call(dbusService, dbusPath,
                                             dbusInterface, "GetNameOwner");
        message.append(name);
        auto reply = conn->call(message);
        reply.read(owner);
    }
    catch (const sdbusplus::exception_t& e)
    {
        std::cerr << "Failed to GetNameOwner. name = " << name
                  << ", error = " << e.what() << "\n";
    }

    return owner;
}

DbusPropertyMap getAll(const std::shared_ptr<sdbusplus::asio::connection>& conn,
                       const std::string& service, const std::string& path,
                       const std::string& interface)
{
    DbusPropertyMap properties;
    try
    {
        auto message = conn->new_method_call(service.c_str(), path.c_str(),
                                             propertiesInterface, "GetAll");
        message.append(interface);
        auto reply = conn->call(message);
        reply.read(properties);
    }
    catch (const sdbusplus::exception_t& e)
    {
        std::cerr << "Failed to GetManagedObjects. service = " << service
                  << ", path = " << path
                  << ", interface = " << interface << ", error = " << e.what()
                  << "\n";
    }

    return properties;
}
