#pragma once

#include <iostream>
#include <vector>
#include <fstream>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <nlohmann/json.hpp>

std::shared_ptr<sdbusplus::asio::connection> bus;

static const std::string solLogFile = "/var/lib/obmc-console/obmc-console.log";
static const std::string patternFilePath = "/etc/sol-pattern.json";

static const int maxPatternSensor = 255;
static constexpr unsigned int sensorPollSec = 10;

static constexpr char const* PROPERTY_INTERFACE = "org.freedesktop.DBus.Properties";

static constexpr char const* SOL_PATTERN_SERVICE = "xyz.openbmc_project.SolPatternSensor";
static constexpr char const* SOL_PATTERN_PATH = "/xyz/openbmc_project/sensors/pattern/Pattern";
static constexpr char const* SOL_PATTERN_INTERFACE = "xyz.openbmc_project.Sensor.Value";

std::vector<std::string> solPatternMatching;
