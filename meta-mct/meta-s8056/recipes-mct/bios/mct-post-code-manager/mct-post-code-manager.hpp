#pragma once

#include <iostream>
#include <unistd.h>
#include <boost/asio.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/exception.hpp>

constexpr auto MAX_COLLECTION_POST_CODE_SIZE = 20;

boost::asio::io_service io;
std::shared_ptr<sdbusplus::asio::connection> systemBus;
std::shared_ptr<sdbusplus::asio::dbus_interface> iface;

std::vector<uint64_t> collectedPostCode;

namespace postCode
{
    static constexpr const char* busName = "xyz.openbmc_project.mct.PostCode";
    static constexpr const char* path = "/xyz/openbmc_project/mct/PostCode";
    static constexpr const char* interface = "xyz.openbmc_project.mct.PostCode";
    static constexpr const char* getPostCodeMethod = "GetPostCodes";
} // namespace postCode
