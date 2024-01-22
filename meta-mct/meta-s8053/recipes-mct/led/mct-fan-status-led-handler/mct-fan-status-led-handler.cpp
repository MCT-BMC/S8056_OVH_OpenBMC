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

#include "mct-fan-status-led-handler.hpp"


namespace fs = std::filesystem;

static constexpr bool DEBUG = false;

int GetNodeID(boost::asio::io_context& io)
{
    int8_t node_ID;
    std::variant<std::uint8_t> result;

    auto bus = std::make_shared<sdbusplus::asio::connection>(io);

    auto method = bus->new_method_call(node::busName, node::path,
                                      PROPERTY_INTERFACE, "Get");

    method.append(node::interface, node::prop);

    try
    {
        auto reply = bus->call(method);
        reply.read(result);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        std::cerr << "Failed to get CurrentNodeID" << std::endl;
        return -1;
    }

    node_ID = std::get<std::uint8_t>(result);

    return node_ID;

}

//Get nct7362 hwmon number
bool getHwmonID()
{
    std::smatch match;
    std::regex inputRegex(R"(hwmon(\d+))");
    fs::path namePath;
    int i = 0;

    for (i=0 ; i<2 ; i++) // 2 nct7362afhimrs
    {
        fs::path hwmonPath = "/sys/bus/i2c/drivers/nct7362/16-00" + std::to_string(i+22) + "/hwmon";
        std::ifstream nameFile(hwmonPath);

        if (nameFile.good())
        {
            for (const auto & file : fs::directory_iterator(hwmonPath))
            {
                namePath = file.path();            
                std::ifstream nameFile(namePath);
                if (nameFile.good())
                {
                    std::string pathStr = namePath.string();
                    std::regex_search(pathStr, match, inputRegex);
                    std::string HWMonStr = *(match.begin());

                    if (i == 0)
                    {
                        hwmonN1ID = std::stoi(HWMonStr.substr(5));
                    }
                    else
                    {
                        hwmonN2ID = std::stoi(HWMonStr.substr(5));
                        return true; // get both nct7362 hwmon id
                    }
                }
            }
        }
    }
    return false;
}

int readFile(std::string senorPath)
{
    int val;
    std::ifstream fs(senorPath);
    
    if (fs.is_open() == false)
    {
        std::cerr << "Read " << senorPath << " fail" << std::endl;
        return -1;
    }

    fs >> val;

    fs.close();
    return val;
}

bool writeFile(std::string senorPath, int value)
{
    std::ofstream fs(senorPath);
    
    if (fs.is_open() == false)
    {
        std::cerr << "Write " << senorPath << " fail" << std::endl;
        return false;
    }

    fs << value;

    fs.close();

    int ret = readFile(senorPath);

    return true;
}

int readFanMinAlarm(int fanNum, std::string hwmon_path)
{
    int ret = 0;
    std::string path;
    path = hwmon_path + "/fan" + std::to_string(fanNum) + "_min_alarm";
    ret = readFile(path);

    return ret;
}

int monitorFanMinFault(int node)
{
    int rc = 0;
    int i = 0;
    int j = 0;
    int count = 0;
    bool fanfault = false;
    int fan_num_start = 0;
    std::string hwmon_path;
    int hwmonID = 0;
    int setValue = 0;
    int alarmVal = 0;
    char gpioName[30];
    gpiod::line line;
    std::string path;

    if (node == 1)
    {
        fan_num_start = 1;
        hwmonID = hwmonN1ID;
    }
    else
    {
        fan_num_start = 4;
        hwmonID = hwmonN2ID;
    }

    hwmon_path = "/sys/class/hwmon/hwmon" + std::to_string(hwmonID);

    for (i=1, j=fan_num_start ; i<=7 ; i=i+3, j++)
    {
        memset(gpioName, 0, sizeof(gpioName));
        sprintf(gpioName, "FAN%d_FAULT_LED", j);

        line = gpiod::find_line(gpioName);
        if (!line)
        {
            std::cerr << "Failed to find the " << gpioName << " line" << std::endl;
            return -1;
        }

        setValue = 1; // LED off
        for (count=0 ; count <2 ; count++)
        {
            // Get fan low speed status
            alarmVal = readFanMinAlarm(i+count, hwmon_path);

            if (alarmVal == 1)
            {
                // LED on
                fanfault = true;
                setValue = 0;
            }
            if (!dualRotorFan)
            {
                break;
            }
        }

        try
        {
            line.request({"mct-fan-status-led-handler", gpiod::line_request::DIRECTION_OUTPUT, setValue});
        }
            catch (std::exception&)
        {
            std::cerr << "Failed to request output for " << gpioName << std::endl;
            return -1;
        }

        //line.set_value(setValue);
        line.release();
    }

    if (!fanfault)
    {
        //clear fan fault status (interrupt pin)
        path = hwmon_path + "/fan" + std::to_string(fan_num_start) + "_alarm";
        alarmVal = readFile(path);
        std::cerr << "No fan fault, clear fan fault status\n";
        return 0;
    }

    return 1;
}


int ReadfanCtrlFail(const std::string& name, int node)
{
    int retVal = 0;
    gpiod::line gpioLine;

    gpioLine = gpiod::find_line(name);
    if (!gpioLine)
    {
        std::cerr << "Failed to find the " << name << " line" << std::endl;
        return -1;
    }

    try
    {
        gpioLine.request({"mct-fan-status-led-handler", gpiod::line_request::DIRECTION_INPUT, 0});
    }
    catch (std::exception&)
    {
        std::cerr << "Failed to request input for " << name << std::endl;
        return -1;
    }

    retVal = gpioLine.get_value();
    gpioLine.release();

    if (retVal == 0)
    {
        return 1; //fan fail
    }
    else
    {
        return 0; // fan normal
    }

}

// Check if need to monitor this node
bool NeedToMonitor(int nodeID)
{
    if (currentNodeId != nodeID)
        return false;
    // TODO: if other node crash or not present
    else
        return true;

}

void StartFanCtrl1LEDTimer()
{

   if (!NeedToMonitor(NODE_1))
   {
      return;
   }

   if (fanCtrl1Fail == false)
   {
      fanCtrl1FailVal = ReadfanCtrlFail(fanCtrl1FailName, NODE_1);

      if (fanCtrl1FailVal == FAN_FAIL) // fan fail interrupt occur
      {
         fanCtrl1Fail = true;
      }
   }

   if (fanCtrl1Fail == true)
   {
      if (monitorFanMinFault(NODE_1) == FAN_NORMAL) // Monitor fans alarm in fan ctrl 1
      {
         fanCtrl1Fail = false; // All fans are normal
      }
   }

}

void StartFanCtrl2LEDTimer()
{

   if (!NeedToMonitor(NODE_2))
   {
      return;
   }

   if (fanCtrl2Fail == false)
   {
      fanCtrl2FailVal = ReadfanCtrlFail(fanCtrl2FailName, NODE_2);
      if (fanCtrl2FailVal == FAN_FAIL)
      {
         fanCtrl2Fail = true;
      }
   }

   if (fanCtrl2Fail == true)
   {
      if (monitorFanMinFault(NODE_2) == FAN_NORMAL)
      {
         fanCtrl2Fail = false;
      }
   }

}

void MonitorFanFault(boost::asio::io_context& io)
{
    static boost::asio::steady_timer timer(io);

    timer.expires_after(std::chrono::seconds(1));

    timer.async_wait([&io](const boost::system::error_code& ec) 
    {
        StartFanCtrl1LEDTimer();
        StartFanCtrl2LEDTimer();
        MonitorFanFault(io);
    });
}

// Initial the dual rotot fan min threshold.
bool initSecFanThres(int threshold)
{
    std::string hwmon_path;
    bool ret = true;
    int hwmonID = 0;
    int i = 0;

    if (currentNodeId == 1)
    {
        hwmonID = hwmonN1ID;
    }
    else
    {
        hwmonID = hwmonN2ID;
    }

    for (i=2 ; i <=8 ; i=i+3)
    {
        hwmon_path = "/sys/class/hwmon/hwmon" + std::to_string(hwmonID) + "/fan" + std::to_string(i) + "_min";
        ret = writeFile(hwmon_path, threshold);
        if (!ret)
        {
            std::cerr << "Write threshold to " << hwmon_path <<  " failed\n";
        }
    }

    return true;
}


int main(int argc, char *argv[])
{
    std::cerr << "MCT Fan status LED handler start to monitor." << std::endl;

    boost::asio::io_service io;
    currentNodeId = GetNodeID(io);

    if (!getHwmonID())
    {
        return 0;
    }

    if (!initSecFanThres(0))
    {
        return 0;
    }

    // To check all fan fail status at least once
    fanCtrl1Fail = true;
    fanCtrl2Fail = true;

    io.post([&]() { MonitorFanFault(io); });

    io.run();

    return 0;
}
