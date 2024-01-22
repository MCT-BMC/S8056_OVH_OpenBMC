#pragma once

#include <iostream>
#include <string>
#include <fstream> 
#include <openbmc/libmisc.h>
#include <sdbusplus/asio/object_server.hpp>
#include <nlohmann/json.hpp>

/** @brief Log the IPMI SEL event
 *
 *  @param[in] enrty - String to journal log
 *  @param[in] path - Sensor path
 *  @param[in] eventData0 - IPMI SEL event data 0.
 *  @param[in] eventData1 - IPMI SEL event data 1.
 *  @param[in] eventData2 - IPMI SEL event data 2.
 * 
 *  @return true: log SEL success false: failed to log SEL
 */
static bool logSELEvent(std::string enrty, std::string path , uint8_t eventData0, uint8_t eventData1, uint8_t eventData2);

/** @brief Set AC lost setting
 * 
 *  @return null
 */
bool aclostSetting();

/** @brief Set BMC watchdog timeout setting
 * 
 *  @return null
 */
bool bmcWatchdogTimeoutSetting();