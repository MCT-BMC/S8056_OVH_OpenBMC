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

#include <LeakyBucket.hpp>
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

uint16_t thresholdT1 = defaultT1;
uint16_t thresholdT2 = defaultT2;
uint16_t thresholdT3 = defaultT3;

bool createDefaultThresholdFile()
{
    std::ofstream thresholdFileOut(thresholdConfigPath.c_str());

    if(!thresholdFileOut)
    {
        return false;
    }
    else
    {
        thresholdFileOut << defaultThreshold.dump(maxThresholdNum);
        thresholdFileOut.close();
    }

    return true;
}

bool updateThresholdJson(std::string thresholdName,
                         uint16_t Value)
{
    // Update the Threshold JSON
    nlohmann::json thresholdData;
    std::ifstream thresholdFileIn(thresholdConfigPath.c_str());

    // Open Threshold config JSON
    if(thresholdFileIn)
    {
        thresholdData = nlohmann::json::parse(thresholdFileIn, nullptr, false);
        thresholdFileIn.close();
    }
    else
    {
        if(!createDefaultThresholdFile())
        {
            return false;
        }
        thresholdData = defaultThreshold;
    }

    // Check JSON syntax
    if (thresholdData.is_null() ||
        thresholdData.is_discarded())
    {
        if(!createDefaultThresholdFile())
        {
            return false;
        }
        thresholdData = defaultThreshold;
    }

    // Update threshold value
    if(thresholdData.find(thresholdName.c_str()) != thresholdData.end())
    {
        thresholdData.at(thresholdName.c_str()) = Value;

        std::ofstream thresholdFileOut(thresholdConfigPath.c_str());
        if(thresholdFileOut)
        {
            thresholdFileOut << thresholdData.dump(maxThresholdNum);
            thresholdFileOut.close();
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    return true;
}

bool setThreshold1(const uint16_t& newValue,
           uint16_t& oldValue)
{
    if(!updateThresholdJson("Threshold1", newValue))
    {
        return false;
    }

    oldValue = newValue;
    thresholdT1 = newValue;

    return true;
}

bool setThreshold2(const uint16_t& newValue,
           uint16_t& oldValue)
{
    if(!updateThresholdJson("Threshold2", newValue))
    {
        return false;
    }

    oldValue = newValue;
    thresholdT2 = newValue;

    return true;
}

bool setThreshold3(const uint16_t& newValue,
           uint16_t& oldValue)
{
    if(!updateThresholdJson("Threshold3", newValue))
    {
        return false;
    }

    oldValue = newValue;
    thresholdT3 = newValue;

    return true;
}

bool createThresholdProperties(
    sdbusplus::asio::object_server& objectServer,
    std::shared_ptr<sdbusplus::asio::dbus_interface>& thresholdPropertyInterface)
{
    // 1. Check if the threshold config file exists.
    struct stat buf;
    if(0 != stat(thresholdConfigPath.c_str(), &buf))
    {
        // If the threshold config file does not exit, create a new one with default values.
        std::cerr << "[lba] threshold config file does not exist.\n";
        if(!createDefaultThresholdFile())
        {
            return false;
        }
    }

    // 2. Threshold JSON Parse
    nlohmann::json thresholdData;
    std::ifstream thresholdFileIn(thresholdConfigPath.c_str());
    if(thresholdFileIn)
    {
        thresholdData = nlohmann::json::parse(thresholdFileIn, nullptr, false);

        if (thresholdData.is_discarded())
        {
            std::cerr << "[lba] threshold config file syntax error \n";
            if(!createDefaultThresholdFile())
            {
                return false;
            }

            thresholdData = defaultThreshold;
        }

        thresholdFileIn.close();

    }
    else
    {
        std::cerr << "[lba] failed to open threshold config file \n";
        if(!createDefaultThresholdFile())
        {
            return false;
        }

        thresholdData = defaultThreshold;
    }
    
    // 2.2 JSON Parsing
    std::string thrdName;
    uint16_t thrdValue;

    for(int idx=0; idx<maxThresholdNum; idx++)
    {
        thrdName = "Threshold" + std::to_string((idx+1));

        if(thresholdData.find(thrdName.c_str()) != thresholdData.end())
        {
            thrdValue = thresholdData.at(thrdName.c_str()).get<uint16_t>();

            switch((idx+1))
            {
                case 1:
                    thresholdT1 = thrdValue;
                    break;
                case 2:
                    thresholdT2 = thrdValue;
                    break;
                case 3:
                    thresholdT3 = thrdValue;
                    break;
                default:
                    std::cerr << "[lba] invalid threshold index "<< idx << "\n";
                    break;
            }
        }
    }

    // 2.3 Set threshold as dbus properties
    thresholdPropertyInterface = objectServer.add_interface(
        thresholdObjPath, thresholdInterface.c_str());

    thresholdPropertyInterface->register_property("Threshold1", thresholdT1,
    [&](const uint16_t& newValue, uint16_t& oldValue) {
        return setThreshold1(newValue, oldValue);
    });

    thresholdPropertyInterface->register_property("Threshold2", thresholdT2,
    [&](const uint16_t& newValue, uint16_t& oldValue) {
        return setThreshold2(newValue, oldValue);
    });

    thresholdPropertyInterface->register_property("Threshold3", thresholdT3,
    [&](const uint16_t& newValue, uint16_t& oldValue) {
        return setThreshold3(newValue, oldValue);
    });

    if (!thresholdPropertyInterface->initialize())
    {
        std::cerr << "[lba] error initializing threshold interface\n";
        return false;
    }

    return true;
}

bool createDimmEccObjects(
    boost::asio::io_service& io, sdbusplus::asio::object_server& objectServer,
    boost::container::flat_map<std::string, std::unique_ptr<LeakyBucketObject>>& dimmobjs,
    std::shared_ptr<sdbusplus::asio::connection>& dbusConnection)
{
    std::ifstream dimmConfigFile(dimmConfigPath);
    
    if(!dimmConfigFile)
    {
        std::cerr << "[lba] failed to open DIMM config file: " << dimmConfigPath <<"\n";
    }
    else
    {
        auto data = nlohmann::json::parse(dimmConfigFile, nullptr, false);
        if(data.is_discarded())
        {
            std::cerr << "[lba] syntax error in " << dimmConfigPath << "\n";
        }
        else
        {
            int idx = 0;
            while(!data[idx].is_null())
            {
                if(!data[idx]["Number"].is_null() &&
                   !data[idx]["Name"].is_null())
                {
                    uint8_t slotNumber = static_cast<uint8_t>(data[idx]["Number"]);
                    std::string slotName = data[idx]["Name"];

                    // Create DIMM objects
                    dimmobjs[slotName] = std::make_unique<LeakyBucketObject>(
                        objectServer, dbusConnection, io, slotNumber, slotName);
                }

                idx++;
            }
        }
    }

    return true;
}

int main(int argc, char *argv[])
{
    boost::asio::io_service io;
    auto systemBus = std::make_shared<sdbusplus::asio::connection>(io);
    systemBus->request_name("xyz.openbmc_project.LeakyBucket");
    sdbusplus::asio::object_server objectServer(systemBus);
    boost::container::flat_map<std::string, std::unique_ptr<LeakyBucketObject>> dimmobjs;
    std::shared_ptr<sdbusplus::asio::dbus_interface> thresholdPropertyInterface;
    
    if(!createThresholdProperties(objectServer, thresholdPropertyInterface))
    {
        std::cerr << "[lba] failed to create threshold properties\n";
        return -1;
    }

    io.post([&]() {
        createDimmEccObjects(io, objectServer, dimmobjs, systemBus);
    });

    io.run();

    objectServer.remove_interface(thresholdPropertyInterface);
    return 0;
}
