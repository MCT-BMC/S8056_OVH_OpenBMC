#pragma once

#include <iostream>
#include <sdbusplus/bus.hpp>


static uint32_t lpcResetStatus = 0x00;
std::unique_ptr<sdbusplus::bus::match::match> lpcResetMatch = nullptr;;

void setLpcResetStatus(uint32_t status)
{
    lpcResetStatus = status;
}

uint32_t checkLpcResetStatus(void)
{
    if (!lpcResetMatch)
    {
        return 0;
    }
    return lpcResetStatus;
}

void setupLpcResetMatch(const std::shared_ptr<sdbusplus::asio::connection>& conn)
{
    static boost::asio::steady_timer timer(conn->get_io_context());

    if (lpcResetMatch)
    {
        return;
    }

    lpcResetMatch = std::make_unique<sdbusplus::bus::match::match>(
        static_cast<sdbusplus::bus::bus&>(*conn),
        sdbusplus::bus::match::rules::type::signal() +
        sdbusplus::bus::match::rules::member(std::string("LpcReset")),
        [](sdbusplus::message::message& message) {
            message.read(lpcResetStatus);
        });
}