/* Copyright 2021 MCT
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include "mct-sol-handler.hpp"

static constexpr bool DEBUG = false;

int execmd(char* cmd,char* result) {
    char buffer[128];
    FILE* pipe = popen(cmd, "r");
    if (!pipe)
        return -1;

    while(!feof(pipe)) {
        if(fgets(buffer, 128, pipe)){
            strcat(result,buffer);
        }
    }
    pclose(pipe);
    return 0;
}

static bool setSolPatternSensorReading(int sensorNum,double value)
{
    std::string solPatternPath = SOL_PATTERN_PATH + std::to_string(sensorNum);

    sdbusplus::message::message method = bus->new_method_call(
        SOL_PATTERN_SERVICE, solPatternPath.c_str(),
        PROPERTY_INTERFACE, "Set");
    method.append(SOL_PATTERN_INTERFACE, "Value", std::variant<double>(value));
	
    try
    {
        bus->call_noreply(method);
    }
    catch (std::exception& e)
    {
        std::cerr << "Failed to set Sol pattern sensor "<< sensorNum <<" reading:" << e.what() << "\n";
        return false;
    }

    return true;
}


void patternInitializing()
{
    nlohmann::json patternInfo;

    solPatternMatching.clear();

    std::ifstream patternFileIn(patternFilePath.c_str());
    if(!patternFileIn)
    {
        return;
    }

    patternInfo = nlohmann::json::parse(patternFileIn, nullptr, false);

    if (patternInfo.is_discarded())
    {
        return;
    }

    patternFileIn.close();

    std::string sensorName;

    for(int i=0; i < maxPatternSensor; i++)
    {
        sensorName = "Pattern" + std::to_string((i+1));

        if(patternInfo.find(sensorName.c_str()) != patternInfo.end())
        {
            solPatternMatching.push_back(patternInfo.at(sensorName.c_str()).get<std::string>());
        }
        else
        {
            break;
        }
    }
}

inline static sdbusplus::bus::match::match setPatternUpdateHandler(
    std::shared_ptr<sdbusplus::asio::connection> conn)
{
    auto matcherCallback = [conn](sdbusplus::message::message &msg)
    {
        patternInitializing();
    };

    sdbusplus::bus::match::match matcher(
        static_cast<sdbusplus::bus::bus &>(*conn),
        "type='signal',member='SolPatternUpdate'",
        std::move(matcherCallback));
    return matcher;
}

void patternHandler(boost::asio::io_context& io,unsigned int delay)
{
    static boost::asio::steady_timer timer(io);

    timer.expires_after(std::chrono::seconds(delay));

    timer.async_wait([&io](const boost::system::error_code&) 
    {
        std::vector<double> solPattenMatch(solPatternMatching.size(),0);

        for(int i=0; i < solPatternMatching.size(); i++)
        {
            if(solPatternMatching[i] == "" )
            {
                solPattenMatch[i] = 0;
                continue;
            }

            char cmd[400]={0};
            char response[50]="";
            int rc=0;

            memset(cmd,0,sizeof(cmd));
            snprintf(cmd,sizeof(cmd),"egrep -m 255 -o \"%s\" %s | wc -l ",
                                      solPatternMatching[i].c_str(),solLogFile.c_str());
            memset(response,0,sizeof(response));
            rc=execmd(cmd,response);

            if(rc==0)
            {
                solPattenMatch[i] = static_cast<unsigned int>(strtol(response, NULL, 10));
            }
        }

        for(int i=0; i < solPatternMatching.size(); i++)
        {
            if constexpr (DEBUG)
            {
                std::cerr << "SOL patten " << i << " match:" << solPattenMatch[i] << std::endl;
            }
            setSolPatternSensorReading(i+1,solPattenMatch[i]);
        }
        patternHandler(io,sensorPollSec);
    });
}

int main(int argc, char *argv[])
{
    boost::asio::io_context io;
    bus = std::make_shared<sdbusplus::asio::connection>(io);
    
    sdbusplus::bus::match::match patternUpdateHandler = setPatternUpdateHandler(bus);

    patternInitializing();

    io.post(
        [&]() { patternHandler(io, 0); });
    
    io.run();
    return 0;
}