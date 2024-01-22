#pragma once

#include <iostream>
#include <chrono>
#include <ctime>
#include <algorithm>
#include <fstream>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/format.hpp>

#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/asio/sd_event.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/exception.hpp>
#include <sdbusplus/server.hpp>
#include <sdbusplus/timer.hpp>

#include <systemd/sd-journal.h>

static const std::string LEAKLY_BUCKET_SERVICE = "xyz.openbmc_project.leaklyBucket";
static const std::string HOST_DIMM_ECC_Path = "/xyz/openbmc_project/leakyBucket/HOST_DIMM_ECC";
static const std::string VALUE_INTERFACE = "xyz.openbmc_project.Sensor.Value";

static const std::string LEAKLY_BUCKET_CONFIG_PATH = "/var/lib/ipmi/lbConfig";

namespace ipmi
{
static constexpr const char* busName = "xyz.openbmc_project.Logging.IPMI";
static constexpr const char* path = "/xyz/openbmc_project/Logging/IPMI";
static constexpr const char* interface = "xyz.openbmc_project.Logging.IPMI";
static constexpr const char* request = "IpmiSelAddOem";
} // namespace ipmi

struct leakyBucketSensor 
{
    leakyBucketSensor(const std::string& name, const uint8_t& sensorNum,
                      std::shared_ptr<sdbusplus::asio::connection>& conn,
                      boost::asio::io_service& io, sdbusplus::asio::object_server& objectServer);
    ~leakyBucketSensor();

    void read(void);
    void init(void);
    bool addEcc(void);

    std::string name;
    uint8_t sensorNum;
    uint8_t count = 0;
    uint32_t limitCount = 0;
    std::unique_ptr<sdbusplus::bus::match::match> lpcResetMatch;
    std::shared_ptr<sdbusplus::asio::dbus_interface> intf;

  private:
    bool enableLimitSel = true;
    sdbusplus::asio::object_server& objectServer;
    std::shared_ptr<sdbusplus::asio::connection> dbusConnection;
    boost::asio::steady_timer waitTimer;
};


