#pragma once

#include <iostream>
#include <filesystem>
#include <fstream>
#include <gpiod.hpp>
#include <unistd.h>
#include <regex>

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>

#include <boost/asio.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/algorithm/string/predicate.hpp>

namespace node
{
    static constexpr const char* busName = "xyz.openbmc_project.mct.node";
    static constexpr const char* path = "/xyz/openbmc_project/mct/node";
    static constexpr const char* interface = "xyz.openbmc_project.mct.node";
    static constexpr const char* prop = "CurrentNodeID";
}

#define PROPERTY_INTERFACE "org.freedesktop.DBus.Properties"
#define FAN_FAIL 1
#define FAN_NORMAL 0
#define NODE_1 1
#define NODE_2 2

static std::string fanCtrl1FailName = "FANCTRL1_FAIL";
bool fanCtrl1Fail = false;
int fanCtrl1FailVal;

static std::string fanCtrl2FailName = "FANCTRL2_FAIL";
bool fanCtrl2Fail = false;
int fanCtrl2FailVal;

uint8_t currentNodeId = 0;
int hwmonN1ID;
int hwmonN2ID;
bool dualRotorFan = false;

