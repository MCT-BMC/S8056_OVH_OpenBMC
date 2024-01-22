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

#include <PatternMatch.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/container/flat_set.hpp>
#include <filesystem>
#include <fstream>
#include <regex>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <systemd/sd-journal.h>
#include <sys/stat.h>
#include <unistd.h>

bool createDefaultPatternFile()
{
    std::ofstream patternFileOut(patternFilePath.c_str());

    if(!patternFileOut)
    {
        return false;
    }
    else
    {
        patternFileOut << defaultPattern.dump(4);
        patternFileOut.close();
    }

    return true;
}

bool createPatternSensors(
    boost::asio::io_service& io, sdbusplus::asio::object_server& objectServer,
    boost::container::flat_map<std::string, std::unique_ptr<SolPatternSensor>>& sensors,
    std::shared_ptr<sdbusplus::asio::connection>& dbusConnection)
{
    // 1. Check if the pattern file exists.
    struct stat buf;
    if(0 != stat(patternFilePath.c_str(), &buf))
    {
        // If the pattern file does not exit, create a new one with default value.
        // sd_journal_print(LOG_CRIT, "[SOL] Pattern File doesn't exist.\n");
        std::cerr << "[sol] pattern file does not exist.\n";
        if(true != createDefaultPatternFile())
        {
            return false;
        }
    }

    // 2. SOL Pattern JSON Parse
    nlohmann::json patternData;
    std::ifstream patternFileIn(patternFilePath.c_str());
    if(patternFileIn)
    {
        patternData = nlohmann::json::parse(patternFileIn, nullptr, false);

        if (patternData.is_discarded())
        {
            std::cerr << "[sol] pattern file syntax error \n";
            if(true != createDefaultPatternFile())
            {
                return false;
            }

            patternData = defaultPattern;
        }

        patternFileIn.close();

    }
    else
    {
        std::cerr << "[sol] failed to open pattern file \n";
        if(true != createDefaultPatternFile())
        {
            return false;
        }
        
        patternData = defaultPattern;
    }
    
    // 2.2 JSON Parsing
    std::string ptnName;
    std::string ptnType;
    std::string ptnString;

    for(int idx=0; idx<maxPatternNum; idx++)
    {
        ptnName = "Pattern" + std::to_string((idx+1));
        ptnType = "pattern/";

        if(patternData.find(ptnName.c_str()) != patternData.end())
        {
            ptnString = patternData.at(ptnName.c_str()).get<std::string>();
        }
        else
        {
            ptnString = "";
        }
        
        // Print out the pattern string and pattern length 
        sensors[ptnName] = std::make_unique<SolPatternSensor>(
            objectServer, dbusConnection, io, ptnType, ptnName, ptnString);
    }

    return true;
}

int main(int argc, char *argv[])
{
    boost::asio::io_service io;
    auto systemBus = std::make_shared<sdbusplus::asio::connection>(io);
    systemBus->request_name("xyz.openbmc_project.SolPatternSensor");
    sdbusplus::asio::object_server objectServer(systemBus);
    boost::container::flat_map<std::string, std::unique_ptr<SolPatternSensor>> sensors;
    
    io.post([&]() {
        createPatternSensors(io, objectServer, sensors, systemBus);
    });

    // boost::asio::steady_timer filterTimer(io);

    io.run();

    return 0;
}
