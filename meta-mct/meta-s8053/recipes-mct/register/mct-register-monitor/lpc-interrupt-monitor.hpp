#pragma once

#include <iostream>
#include <vector>
#include <variant>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <climits>
#include <unistd.h>
#include <sys/inotify.h>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/server.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/log.hpp>

#define BUF_LEN (10 * (sizeof(struct inotify_event) + NAME_MAX + 1))

namespace fs = std::filesystem;
using namespace phosphor::logging;

static constexpr auto MILLI_OFFSET = 1000;
static constexpr auto MICRO_OFFSET = 1000000;

static constexpr auto lpc_rst = "lpc-rst";
static constexpr auto interrupt = "interrupt";

static std::vector<std::string> registerMatch;
static std::vector<std::string> findPath;


/** @brief Initialize the property for match LPC interrupt register
 *
 */
void propertyInitialize();


/** @brief Find the monitor register path for LPC interrupt.
 *
 *  @return On success returns path with stored LPC interrupt.
 */
std::vector<std::string> findRegisterPath();

/**
 * @brief Handler the interrupt for input path
 *
 * @param[in] lpcPath - Path for LPC reset register.
 * @param[in] interruptPath - Path for LPC interrupt.
 */
void interruptHandler(std::string lpcPath,std::string interruptPath);

/**
 * @brief Get current timestamp in milliseconds.
 * 
 * @param[in] io - io context.
 * @param[in] delay - Delay time in micro second.
 */
void registerHandler(boost::asio::io_context& io,double delay);

/**
 * @brief Action for interrupt occurred.
 *
 * @param[in] lpcPath - Path for LPC reset register.
 * @param[in] status - Status for handler getting.
 */
void interruptAction(std::string lpcPath, int status);
