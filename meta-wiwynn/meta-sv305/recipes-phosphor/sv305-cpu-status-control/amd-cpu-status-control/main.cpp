#include "PowerCapping.hpp"

#include <stdio.h>

#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/server.hpp>

#include <memory>
#include <variant>

static int pollingMs = 1000;

int main(int argc, char** argv)
{

    boost::asio::io_service io;
    auto systemBus = std::make_shared<sdbusplus::asio::connection>(io);
    systemBus->request_name(serviceName);
    sdbusplus::asio::object_server server(systemBus);

    PowerCapping powerCapping(io, pollingMs, server, systemBus);

    io.run();

    return 0;
}
