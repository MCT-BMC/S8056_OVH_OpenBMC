#pragma once

#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/bus/match.hpp>

#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

using DbusObjectPath = std::string;
using DbusService = std::string;
using DbusInterface = std::string;
using DbusProperty = std::string;

using DbusVariant = std::variant<std::string, bool, uint8_t, uint16_t, int16_t,
                                 uint32_t, int32_t, uint64_t, int64_t, double>;

using DbusPropertyMap = std::unordered_map<DbusProperty, DbusVariant>;

using DbusInterfaceMap = std::unordered_map<DbusInterface, DbusPropertyMap>;

using GetSubTreeType = std::unordered_map<
    DbusObjectPath,
    std::unordered_map<DbusService, std::vector<DbusInterface>>>;

constexpr auto propertiesInterface = "org.freedesktop.DBus.Properties";

constexpr auto dbusService = "org.freedesktop.DBus";
constexpr auto dbusPath = "/org/freedesktop/DBus";
constexpr auto dbusInterface = "org.freedesktop.DBus";

constexpr auto mapperService = "xyz.openbmc_project.ObjectMapper";
constexpr auto mapperPath = "/xyz/openbmc_project/object_mapper";
constexpr auto mapperInterface = "xyz.openbmc_project.ObjectMapper";

GetSubTreeType getSubTree(
    const std::shared_ptr<sdbusplus::asio::connection>& conn,
    const std::string& path, const int depth,
    const std::vector<std::string>& interfaces = std::vector<std::string>{});

std::string
    getNameOwner(const std::shared_ptr<sdbusplus::asio::connection>& conn,
                 const std::string& name);

DbusPropertyMap getAll(const std::shared_ptr<sdbusplus::asio::connection>& conn,
                       const std::string& service, const std::string& path,
                       const std::string& interface);
