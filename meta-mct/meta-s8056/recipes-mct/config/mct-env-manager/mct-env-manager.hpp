#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <cstdio>
#include <nlohmann/json.hpp>

constexpr auto KEY = "Key";
constexpr auto VALUE = "Value";

const static std::string envConfigPath = "/usr/share/mct-env-manager/env-config.json";