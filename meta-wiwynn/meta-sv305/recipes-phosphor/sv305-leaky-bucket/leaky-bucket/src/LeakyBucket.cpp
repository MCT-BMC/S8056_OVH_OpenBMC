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

#include <unistd.h>
#include <LeakyBucket.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <iostream>
#include <fstream>
#include <limits>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <string>
#include <systemd/sd-journal.h>

static std::chrono::seconds dimmPollSec(1);

LeakyBucketObject::LeakyBucketObject(sdbusplus::asio::object_server& objectServer,
    std::shared_ptr<sdbusplus::asio::connection>& conn, boost::asio::io_service& io,
    const uint8_t& slotNumber, const std::string& slotName):
    objServer(objectServer), busConn(conn), waitTimer(io), slotName(slotName), sensorNumber(slotNumber),
    corrEccTotalValue(0), corrEccRelativeValue(0), diffCount(0), dimmEccSelNumber(0), timer(0),
    lastEccTotalValue(0), lbaThreshold1(thresholdT1), lbaThreshold2(thresholdT2), lbaThreshold3(thresholdT3)
{
    std::string dbusPath = dimmObjPathPrefix + std::to_string(slotNumber);

    dimmObjInterface = objectServer.add_interface(
        dbusPath, eccErrorInterface.c_str());

    // Total correctable ECC event counter
    dimmObjInterface->register_property("TotalEccCount", corrEccTotalValue,
    [&](const uint32_t& newValue, uint32_t& oldValue) {
        return setTotalEccCount(newValue, oldValue);
    });

    // Relative correctable ECC event counter
    dimmObjInterface->register_property("RelativeEccCount", corrEccRelativeValue,
    [&](const uint32_t& newValue, uint32_t& oldValue) {
        return setRelativeEccCount(newValue, oldValue);
    });

    // Method to add correctable ECC event count
    dimmObjInterface->register_method("DimmEccAssert",
    [&]() {
        return accumulateTotalEccCount();
    });

    if (!dimmObjInterface->initialize())
    {
        std::cerr << "[lba] error initializing ECC error interface\n";
    }

    setupProcessing();
}

LeakyBucketObject::~LeakyBucketObject()
{
    waitTimer.cancel();
    objServer.remove_interface(dimmObjInterface);
}

bool LeakyBucketObject::setTotalEccCount(const uint32_t& newValue,
                                         uint32_t& oldValue)
{
     oldValue = newValue;
     return true;
}

bool LeakyBucketObject::updateTotalEccCount(const uint32_t& newValue)
{
    dimmObjInterface->set_property("TotalEccCount", newValue);
    return true;
}

bool LeakyBucketObject::setRelativeEccCount(const uint32_t& newValue,
                                            uint32_t& oldValue)
{
     oldValue = newValue;
     corrEccRelativeValue = newValue;
     return true;
}

bool LeakyBucketObject::updateRelativeEccCount(const uint32_t& newValue)
{
    dimmObjInterface->set_property("RelativeEccCount", newValue);
    return true;
}

bool LeakyBucketObject::accumulateTotalEccCount()
{
    corrEccTotalValue++;
    updateTotalEccCount(corrEccTotalValue);

    return true;
}

void LeakyBucketObject::setupProcessing(void)
{
    runLeakyBucketAlgorithm();

    waitTimer.expires_from_now(std::chrono::steady_clock::duration(dimmPollSec));
    waitTimer.async_wait([&](const boost::system::error_code& ec) {
    if (ec == boost::asio::error::operation_aborted)
    {
        std::cerr << "[lba] async_wait operation aborted!\n";
        return; // we're being canceled
    }
        setupProcessing();
    });
}

void LeakyBucketObject::runLeakyBucketAlgorithm(void)
{
    // 1. Update threshold value
    lbaThreshold1 = thresholdT1;
    lbaThreshold2 = thresholdT2;
    lbaThreshold3 = thresholdT3;

    // 2. Get the diff value
    if(corrEccTotalValue > lastEccTotalValue)
    {
        diffCount += corrEccTotalValue - lastEccTotalValue;
    }
    else if(corrEccTotalValue < lastEccTotalValue)
    {
        diffCount += (std::numeric_limits<uint32_t>::max() - lastEccTotalValue);
        diffCount += (corrEccTotalValue + 1);
    }
    lastEccTotalValue = corrEccTotalValue;

    // 3. Deal with the diff value
    if(0 != diffCount)
    {
        // 3.1 Start the timer
        timer++;

        // 3.2 Reduce by T2 for every 24 hours
        if(reduceEventTimeSec == timer)
        {
            if(diffCount <= lbaThreshold2)
            {
                diffCount = 0;
            }
            else
            {
                diffCount -= lbaThreshold2;
            }

            timer = 0;
        }

        if(0 == lbaThreshold1)
        {
            // Invalid T1 threshold. Set to default
            lbaThreshold1 = defaultT1;
        }

        // 3.3 Get the number of SEL we are going to issue
        while(diffCount >= lbaThreshold1)
        {
            diffCount -= lbaThreshold1;

            if(dimmEccSelNumber < lbaThreshold3)
            {
                uint8_t eventData1;

                dimmEccSelNumber++;

                if(dimmEccSelNumber == lbaThreshold3)
                {
                    eventData1 = 0x5; // Correctable ECC logging limit reached
                }
                else
                {
                    eventData1 = 0x0; // Correctable ECC Error
                }

                logSelCorrectableMemEcc(eventData1);
            }
        }

        if(0 == diffCount)
        {
            timer = 0;
        }
    }
    else
    {
        timer = 0;
    }

    if(diffCount != corrEccRelativeValue)
    {
        updateRelativeEccCount(diffCount);
    }
}

void LeakyBucketObject::logSelCorrectableMemEcc(const uint8_t eventData1)
{
    /* Sensor type: Memory (0xC)
       Sensor specific offset: 00h - Correctable ECC Error
                               05h - Correctable ECC logging limit reached
    */
    uint8_t recordType = 0x2;   // Record Type
    std::vector<uint8_t> selData(9, 0xFF);

    selData[0] = 0x21;          // Generator ID
    selData[1] = 0x00;          // Generator ID
    selData[2] = 0x04;          // EvM Rev
    selData[3] = 0x0C;          // Sensor Type - Memory
    selData[4] = sensorNumber;  // Sensor Number
    selData[5] = 0x6f;          // Event Dir | Event Type
    selData[6] = eventData1;    // Event Data 1
    selData[7] = 0xFF;          // Event Data 2
    selData[8] = 0xFF;          // Event Data 3

    sdbusplus::message::message writeSEL = busConn->new_method_call(
        ipmiSELService, ipmiSELPath, ipmiSELAddInterface, "IpmiSelAddOem");
    writeSEL.append(ipmiSELAddMessage, selData, recordType);

    try
    {
        auto ret = busConn->call(writeSEL);
    }
    catch (sdbusplus::exception_t& e)
    {
        std::cerr << "[lba][" << slotName << "] failed to add Memory correctable ECC SEL \n";
    }
}
