#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <gpiod.h>
#include <nlohmann/json.hpp>

constexpr auto LINE_NAME = "LineName";
constexpr auto DIRECTION = "Direction";
constexpr auto VALUE = "Value";

const static std::string gpioInitialConfigPath = "/usr/share/gpio-initial/gpio-initial-config.json";

/** @brief Set the specificed pin direction to input.
 *
 *  @param[in] lineName - line name from GPIO in DTS.
 */
void setInput(std::string lineName);

/** @brief Set the specificed pin direction to output.
 *
 *  @param[in] lineName - line name from GPIO in DTS.
 *  @param[in] setValue - Setting output voltage.
 */
void setOutput(std::string lineName,int setValue);