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

#include <systemd/sd-journal.h>
#include <unistd.h>

#include <LeakyBucket.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>

#include <fstream>
#include <iostream>
#include <limits>
#include <string>

static std::chrono::seconds reduceEventTimeSec(86400); // 24 hours

LeakyBucketObject::LeakyBucketObject(
    sdbusplus::asio::object_server& objectServer, boost::asio::io_service& io,
    const uint8_t& slotNumber, const std::string& slotName) :
    objServer(objectServer),
    busConn(std::make_shared<sdbusplus::asio::connection>(
        io, sdbusplus::bus::new_system().release())),
    reduceTimer(io), slotName(slotName), sensorNumber(slotNumber),
    corrEccTotalValue(0), corrEccRelativeValue(0), hadLogCorrectableEccSel(false), dimmEccSelNumber(0),
    lbaThreshold1(thresholdT1), lbaThreshold2(thresholdT2),
    lbaThreshold3(thresholdT3)
{
    std::string dbusPath = dimmObjPathPrefix + std::to_string(slotNumber);

    dimmObjInterface =
        objectServer.add_interface(dbusPath, eccErrorInterface.c_str());

    // Total correctable ECC event counter
    dimmObjInterface->register_property(
        "TotalEccCount", corrEccTotalValue,
        [&](const uint32_t& newValue, uint32_t& oldValue) {
            return setTotalEccCount(newValue, oldValue);
        });

    // Relative correctable ECC event counter
    dimmObjInterface->register_property("RelativeEccCount", corrEccRelativeValue,
    [&](const uint32_t& newValue, uint32_t& oldValue) {
        return setRelativeEccCount(newValue, oldValue);
    });

    // OEM MCA Status Event Data
    OemMcaStatusEventData.resize(13);
    dimmObjInterface->register_property(
        "OemMcaStatusEventData", OemMcaStatusEventData,
        [&](const std::vector<uint8_t>& newValue,
            std::vector<uint8_t>& oldValue) {
            return setOemMcaStatusEventData(newValue, oldValue);
        });

    // Method to add correctable ECC event count
    dimmObjInterface->register_method(
        "DimmEccAssert", [&]() { return accumulateTotalEccCount(); });

    if (!dimmObjInterface->initialize())
    {
        std::cerr << "[lba] error initializing ECC error interface\n";
    }

    startReduceTimer();
}

LeakyBucketObject::~LeakyBucketObject()
{
    reduceTimer.cancel();
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
     return true;
}

bool LeakyBucketObject::updateRelativeEccCount(const uint32_t& newValue)
{
    dimmObjInterface->set_property("RelativeEccCount", newValue);
    return true;
}

bool LeakyBucketObject::setOemMcaStatusEventData(
    const std::vector<uint8_t>& newValue, std::vector<uint8_t>& oldValue)
{
    oldValue = newValue;
    OemMcaStatusEventData = newValue;
    if (hadLogCorrectableEccSel)
    {
        hadLogCorrectableEccSel = false;

        logSelOemMcaStatus(OemMcaStatusEventData);
    }
    return true;
}

bool LeakyBucketObject::accumulateTotalEccCount()
{
    hadLogCorrectableEccSel = false;

    updateTotalEccCount(++corrEccTotalValue);
    corrEccRelativeValue++;
    updateThresholds();

    if ((corrEccRelativeValue % lbaThreshold1) == 0)
    {
        if (dimmEccSelNumber < lbaThreshold3)
        {
            uint8_t eventData1;

            dimmEccSelNumber++;

            if (dimmEccSelNumber == lbaThreshold3)
            {
                eventData1 = 0x5; // Correctable ECC logging limit reached
            }
            else
            {
                eventData1 = 0x0; // Correctable ECC Error
            }

            logSelCorrectableMemEcc(eventData1);
            hadLogCorrectableEccSel = true;
            corrEccRelativeValue = 0;
        }
    }

    updateRelativeEccCount(corrEccRelativeValue);

    return true;
}

void LeakyBucketObject::updateThresholds(void)
{
    lbaThreshold1 = thresholdT1;
    lbaThreshold2 = thresholdT2;
    lbaThreshold3 = thresholdT3;

    if(0 == lbaThreshold1)
    {
        // Invalid T1 threshold. Set to default
        lbaThreshold1 = defaultT1;
    }
}

void LeakyBucketObject::startReduceTimer(void)
{

    reduceTimer.expires_from_now(
        std::chrono::steady_clock::duration(reduceEventTimeSec));
    reduceTimer.async_wait([&](const boost::system::error_code& ec) {
        if (ec == boost::asio::error::operation_aborted)
        {
            std::cerr << "[lba] async_wait operation aborted!\n";
            return; // we're being canceled
        }
        updateThresholds();

        if (corrEccRelativeValue <= lbaThreshold2)
        {
            corrEccRelativeValue = 0;
        }
        else
        {
            corrEccRelativeValue -= lbaThreshold2;
        }

        updateRelativeEccCount(corrEccRelativeValue);

        startReduceTimer();
    });
}

void LeakyBucketObject::logSelCorrectableMemEcc(const uint8_t eventData1)
{
    /* Sensor type: Memory (0xC)
       Sensor specific offset: 00h - Correctable ECC Error
                               05h - Correctable ECC logging limit reached
    */
    uint8_t recordType = 0x2; // Record Type
    std::vector<uint8_t> selData(9, 0xFF);

    selData[0] = 0x21;         // Generator ID
    selData[1] = 0x00;         // Generator ID
    selData[2] = 0x04;         // EvM Rev
    selData[3] = 0x0C;         // Sensor Type - Memory
    selData[4] = sensorNumber; // Sensor Number
    selData[5] = 0x6f;         // Event Dir | Event Type
    selData[6] = eventData1;   // Event Data 1
    selData[7] = 0xFF;         // Event Data 2
    selData[8] = 0xFF;         // Event Data 3

    sdbusplus::message::message writeSEL = busConn->new_method_call(
        ipmiSELService, ipmiSELPath, ipmiSELAddInterface, "IpmiSelAddOem");
    writeSEL.append(ipmiSELAddMessage, selData, recordType);

    try
    {
        auto ret = busConn->call(writeSEL);
    }
    catch (sdbusplus::exception_t& e)
    {
        std::cerr << "[lba][" << slotName
                  << "] failed to add Memory correctable ECC SEL \n";
    }
}

void LeakyBucketObject::logSelOemMcaStatus(const std::vector<uint8_t> eventData)
{
    uint8_t recordType = 0xE0; // Record Type

    sdbusplus::message::message writeSEL = busConn->new_method_call(
        ipmiSELService, ipmiSELPath, ipmiSELAddInterface, "IpmiSelAddOem");
    writeSEL.append(ipmiSELAddMessage, eventData, recordType);

    try
    {
        auto ret = busConn->call(writeSEL);
    }
    catch (sdbusplus::exception_t& e)
    {
        std::cerr << "[lba][" << slotName
                  << "] failed to add OEM MCA Status SEL \n";
    }
}
