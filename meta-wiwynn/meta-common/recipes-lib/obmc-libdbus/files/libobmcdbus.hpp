#pragma once

#include <chrono>
#include <sdbusplus/bus.hpp>
#include <variant>
#include "xyz/openbmc_project/Software/Version/server.hpp"

constexpr uint16_t selBMCGenID = 0x0020;
constexpr char const* IPMI_SEL_SERVICE = "xyz.openbmc_project.Logging.IPMI";
constexpr char const* IPMI_SEL_PATH = "/xyz/openbmc_project/Logging/IPMI";
constexpr char const* IPMI_SEL_INTERFACE = "xyz.openbmc_project.Logging.IPMI";

int ipmiSelAdd(std::string message, std::string object_path,
               std::vector<uint8_t> event_data, bool is_assert);

/* isPgoodOn
 *
 * Query pgood to check host power status
 * @out: true - host power is on
 *       false - host power is off
 **/
bool isPgoodOn();

/* isPostComplete
 *
 * Query postcomplete to check bios post status
 * @out: true - post complete
 *       false - during bios post
 **/
bool isPostComplete();

