/*
// Copyright (c) 2019 Wiwynn Corporation
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
#include <PatternMatch.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <iostream>
#include <fstream>
#include <limits>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <string>
#include <systemd/sd-journal.h>

static const std::string solLogFile = "/var/log/obmc-console.log";
static constexpr unsigned int sensorPollSec = 5;
static constexpr size_t warnAfterErrorCount = 10;

SolPatternSensor::SolPatternSensor(
    sdbusplus::asio::object_server& objectServer,
    std::shared_ptr<sdbusplus::asio::connection>& conn,
    boost::asio::io_service& io, const std::string& sensorType,
    const std::string& sensorName, const std::string& ptnString):
    objServer(objectServer), waitTimer(io), patternName(sensorName),
    patternString(ptnString), hitCount(0), newHitCount(0), errCount(0)
{
    std::string dbusPath = sensorPathPrefix + sensorType + sensorName;

    sensorInterface = objectServer.add_interface(
        dbusPath, solValueInterface.c_str());
    
    unsigned int maxValue = 255;
    unsigned int minValue = 0;
    bool invalidValue = false;
    sensorInterface->register_property("InvalidValue", invalidValue);
    sensorInterface->register_property("MaxValue", maxValue);
    sensorInterface->register_property("MinValue", minValue);

    // Pattern Hit value
    sensorInterface->register_property("Value", hitCount,
    [&](const unsigned int& newValue, unsigned int& oldValue) {
        return setHitCount(newValue, oldValue);
    });

    if (!sensorInterface->initialize())
    {
        std::cerr << "error initializing value interface\n";
    }

    // Pattern String
    patternInterface = objectServer.add_interface(
        dbusPath, solPatternInterface.c_str());
    
    patternInterface->register_property("Pattern", patternString,
    [&](const std::string& newValue, std::string& oldValue) {
        return setPattern(newValue, oldValue);
    });

    if (!patternInterface->initialize())
    {
        std::cerr << "error initializing value interface\n";
    }

    // Matching command
    matchingCmd = "egrep -c -m 255 \"" + patternString + "\" " + solLogFile;

    setupMatching();
}

SolPatternSensor::~SolPatternSensor()
{
    waitTimer.cancel();
    objServer.remove_interface(sensorInterface);
    objServer.remove_interface(patternInterface);
}

bool SolPatternSensor::setHitCount(const unsigned int& newValue, 
                                         unsigned int& oldValue)
{
     oldValue = newValue;
     hitCount = newValue;

     return true;
}

bool SolPatternSensor::setPattern(const std::string& newValue, 
                                        std::string& oldValue)
{
     oldValue = newValue;
     patternString = newValue;
     matchingCmd = "egrep -c -m 255 \"" + patternString + "\" " + solLogFile;
     
     // Update the Pattern JSON
     nlohmann::json patternData;
     std::ifstream patternFileIn(patternFilePath.c_str());
     if(patternFileIn)
     {
        patternData = nlohmann::json::parse(patternFileIn, nullptr, false);
        patternFileIn.close();
     }

     if ((!patternData.is_null()) &&
         (!patternData.is_discarded()))
     {
        if(patternData.find(patternName.c_str()) != patternData.end())
        {
            // Modify the pattern string and update the pattern json
            patternData.at(patternName.c_str()) = newValue;

            std::ofstream patternFileOut(patternFilePath.c_str());
            if(patternFileOut)
            {
                patternFileOut << patternData.dump(4);
                patternFileOut.close();
            }
        }
     }

     return true;
}

bool SolPatternSensor::updateHitCount(const unsigned int& newValue)
{
    sensorInterface->set_property("Value", newValue);

    return true;
}

void SolPatternSensor::setupMatching(void)
{
    bool res = runMatching();
    if( res != true)
    {
        errCount++;
    }
    else
    {
       errCount = 0;
    }

    handleResponse();
}

bool SolPatternSensor::runMatching(void)
{
    // if it is a null string
    if(0 == patternString.size())
    {
        newHitCount = 0;
        return true;
    }

    boost::process::ipstream ips;
    boost::process::child c(matchingCmd.c_str(), boost::process::std_out > ips);
    c.wait();

    if(!c.exit_code())
    {
        std::string matchingRes;
        ips >> matchingRes;
        newHitCount = static_cast<unsigned int>(strtol(matchingRes.c_str(), NULL, 10));
    }
    else
    {
        return false;
    }

    return true;
}

void SolPatternSensor::handleResponse()
{
    if (errCount >= warnAfterErrorCount)
    {
        updateHitCount(0);
    }
    else
    {
        if (newHitCount != hitCount)
        {
            updateHitCount(newHitCount);
        }
    }

    waitTimer.expires_from_now(boost::asio::chrono::seconds(sensorPollSec));
    waitTimer.async_wait([&](const boost::system::error_code& ec) {
        if (ec == boost::asio::error::operation_aborted)
        {
            return; // we're being canceled
        }
        setupMatching();
    });
}
