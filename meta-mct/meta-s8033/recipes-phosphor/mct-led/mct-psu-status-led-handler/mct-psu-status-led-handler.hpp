#pragma once

#include <iostream>
#include <filesystem>
#include <unistd.h>
#include <boost/asio.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <gpiod.hpp>
#include <gpiod.hpp>

boost::asio::io_service io;

static std::string psu1AcLossName = "PSU1_AC_LOSS";
static gpiod::line psu1AcLossLine;
static boost::asio::posix::stream_descriptor psu1AcLossEvent(io);
int psu1AcLossValue;

static std::string psu2AcLossName = "PSU2_AC_LOSS";
static gpiod::line psu2AcLossLine;
static boost::asio::posix::stream_descriptor psu2AcLossEvent(io);
int psu2AcLossValue;

static std::string systemAcLossName = "SYS_AC_LOSS";
static gpiod::line systemAcLossLine;
int systemAcLossValue;

std::string pdbConfigPath = "/run/pdb-config";
bool pdbConfig= false;
