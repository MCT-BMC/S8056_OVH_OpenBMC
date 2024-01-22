/*
// Copyright (c) 2020 Wiwynn Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

#pragma once

#include <systemd/sd-journal.h>
#include <unistd.h>

#include <boost/asio/io_service.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/chrono/chrono_io.hpp>
#include <boost/process.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/asio/object_server.hpp>

#include <iostream>
#include <variant>

static const std::string dimmObjPathPrefix =
    "/xyz/openbmc_project/leaky_bucket/dimm_slot/";
static const std::string thresholdObjPath =
    "/xyz/openbmc_project/leaky_bucket/threshold";
static const std::string eccErrorInterface =
    "xyz.openbmc_project.DimmEcc.Count";
static const std::string thresholdInterface =
    "xyz.openbmc_project.LeakyBucket.Threshold";

static constexpr char const* ipmiSELService =
    "xyz.openbmc_project.Logging.IPMI";
static constexpr char const* ipmiSELPath = "/xyz/openbmc_project/Logging/IPMI";
static constexpr char const* ipmiSELAddInterface =
    "xyz.openbmc_project.Logging.IPMI";
static const std::string ipmiSELAddMessage = "SEL Entry";

static const std::string dimmConfigPath = "/etc/leaky-bucket-dimm.json";
static const std::string thresholdConfigPath =
    "/etc/leaky-bucket-threshold.json";

static const uint8_t maxThresholdNum = 3;

static const uint16_t defaultT1 = 1;
static const uint16_t defaultT2 = 0;
static const uint16_t defaultT3 = 65535;

extern uint16_t thresholdT1;
extern uint16_t thresholdT2;
extern uint16_t thresholdT3;

static const nlohmann::json defaultThreshold = {{"Threshold1", defaultT1},
                                                {"Threshold2", defaultT2},
                                                {"Threshold3", defaultT3}};

class LeakyBucketObject
{
  public:
    LeakyBucketObject(sdbusplus::asio::object_server& objectServer,
                      boost::asio::io_service& io, const uint8_t& slotNumber,
                      const std::string& slotName);
    ~LeakyBucketObject();

  private:
    sdbusplus::asio::object_server& objServer;
    std::shared_ptr<sdbusplus::asio::connection> busConn;
    boost::asio::steady_timer reduceTimer;
    std::shared_ptr<sdbusplus::asio::dbus_interface> dimmObjInterface;

    std::string slotName;
    uint8_t sensorNumber;
    uint32_t corrEccTotalValue;
    uint32_t corrEccRelativeValue;
    uint32_t dimmEccSelNumber;
    std::vector<uint8_t> OemMcaStatusEventData;
    bool hadLogCorrectableEccSel;

    uint16_t lbaThreshold1;
    uint16_t lbaThreshold2;
    uint16_t lbaThreshold3;

    bool setTotalEccCount(const uint32_t& newValue, uint32_t& oldValue);
    bool updateTotalEccCount(const uint32_t& newValue);
    bool setRelativeEccCount(const uint32_t& newValue, uint32_t& oldValue);
    bool updateRelativeEccCount(const uint32_t& newValue);
    bool setOemMcaStatusEventData(const std::vector<uint8_t>& newValue,
                                  std::vector<uint8_t>& oldValue);
    bool accumulateTotalEccCount();

    void updateThresholds(void);
    void startReduceTimer(void);
    void logSelCorrectableMemEcc(const uint8_t eventData1);
    void logSelOemMcaStatus(const std::vector<uint8_t> eventData);
};
