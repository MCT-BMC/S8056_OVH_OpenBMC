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

#include "mct-psu-status-led-handler.hpp"

static constexpr bool DEBUG = false;

void printVoltageLog(std::string name, std::string status)
{
    std::cerr << name << " voltage level:" << status << std::endl;
}

void setSystemAcLossGpio(uint8_t psu1AcLossValue, uint8_t  psu2AcLossValue, bool setGpio)
{
    if(pdbConfig)
    {
        if(psu2AcLossValue)
        {
            systemAcLossValue = 1;
        }
        else
        {
            systemAcLossValue = 0;
        }
    }
    else
    {
        if(psu1AcLossValue && psu2AcLossValue)
        {
            systemAcLossValue = 1;
        }
        else
        {
            systemAcLossValue = 0;
        }
    }

    if(setGpio)
    {
        systemAcLossLine.set_value(systemAcLossValue);
        printVoltageLog(systemAcLossName,std::to_string(systemAcLossValue));
    }
}

static void psu1AclossHandler()
{
    gpiod::line_event gpioLineEvent = psu1AcLossLine.event_read();

    if (gpioLineEvent.event_type == gpiod::line_event::FALLING_EDGE)
    {
        printVoltageLog(psu1AcLossName,"FALLING_EDGE");
        psu1AcLossValue = 0;
    }
    else if (gpioLineEvent.event_type == gpiod::line_event::RISING_EDGE)
    {
        printVoltageLog(psu1AcLossName,"RISING_EDGE");
        psu1AcLossValue = 1;
    }

    setSystemAcLossGpio(psu1AcLossValue,psu2AcLossValue,true);

    psu1AcLossEvent.async_wait(
        boost::asio::posix::stream_descriptor::wait_read,
        [&](const boost::system::error_code ec) {
            if (ec)
            {
                 std::cerr << "PSU1 AC loss handler error: " << ec.message();
                return;
            }
            psu1AclossHandler();
        });
}

static void psu2AclossHandler()
{
    gpiod::line_event gpioLineEvent = psu2AcLossLine.event_read();

    if (gpioLineEvent.event_type == gpiod::line_event::FALLING_EDGE)
    {
        printVoltageLog(psu2AcLossName,"FALLING_EDGE");
        psu2AcLossValue = 0;
    }
    else if (gpioLineEvent.event_type == gpiod::line_event::RISING_EDGE)
    {
        printVoltageLog(psu2AcLossName,"RISING_EDGE");
        psu2AcLossValue = 1;
    }

    setSystemAcLossGpio(psu1AcLossValue,psu2AcLossValue,true);

    psu2AcLossEvent.async_wait(
        boost::asio::posix::stream_descriptor::wait_read,
        [&](const boost::system::error_code ec) {
            if (ec)
            {
                 std::cerr << "PSU1 AC loss handler error: " << ec.message();
                return;
            }
            psu2AclossHandler();
        });
}

bool psu1AcLossGpioInit(const std::string& name)
{
    psu1AcLossLine = gpiod::find_line(name);
    if (!psu1AcLossLine)
    {
        std::cerr << "Failed to find the " << name << " line" << std::endl;
        return false;
    }

    try
    {
        psu1AcLossLine.request({"psu-status-led-handler", gpiod::line_request::DIRECTION_INPUT, 0});
    }
    catch (std::exception&)
    {
        std::cerr << "Failed to request input for " << name << std::endl;
        return false;
    }

    psu1AcLossValue = psu1AcLossLine.get_value();
    psu1AcLossLine.release();

    try
    {
        psu1AcLossLine.request({"psu-status-led-handler", gpiod::line_request::EVENT_BOTH_EDGES});
    }
    catch (std::exception&)
    {
        std::cerr << "Failed to request events for " << name << std::endl;
        return false;
    }

    int gpioLineFd = psu1AcLossLine.event_get_fd();
    if (gpioLineFd < 0)
    {
        std::cerr << "Failed to name " << name << " fd" << std::endl;
        return false;
    }

    psu1AcLossEvent.assign(gpioLineFd);

    psu1AcLossEvent.async_wait(boost::asio::posix::stream_descriptor::wait_read,
        [&](const boost::system::error_code ec) {
            if (ec)
            {
                std::cerr << name << " fd handler error: " << ec.message();
                return;
            }
            psu1AclossHandler();
    });

    return true;
}

bool psu2AcLossGpioInit(const std::string& name)
{
    psu2AcLossLine = gpiod::find_line(name);
    if (!psu2AcLossLine)
    {
        std::cerr << "Failed to find the " << name << " line" << std::endl;
        return false;
    }

    try
    {
        psu2AcLossLine.request({"psu-status-led-handler", gpiod::line_request::DIRECTION_INPUT, 0});
    }
    catch (std::exception&)
    {
        std::cerr << "Failed to request input for " << name << std::endl;
        return false;
    }

    psu2AcLossValue = psu2AcLossLine.get_value();
    psu2AcLossLine.release();

    try
    {
        psu2AcLossLine.request({"psu-status-led-handler", gpiod::line_request::EVENT_BOTH_EDGES});
    }
    catch (std::exception&)
    {
        std::cerr << "Failed to request events for " << name << std::endl;
        return false;
    }

    int gpioLineFd = psu2AcLossLine.event_get_fd();
    if (gpioLineFd < 0)
    {
        std::cerr << "Failed to name " << name << " fd" << std::endl;
        return false;
    }

    psu2AcLossEvent.assign(gpioLineFd);

    psu2AcLossEvent.async_wait(boost::asio::posix::stream_descriptor::wait_read,
        [&](const boost::system::error_code ec) {
            if (ec)
            {
                std::cerr << name << " fd handler error: " << ec.message();
                return;
            }
            psu2AclossHandler();
    });

    return true;
}

bool systemAcLossGpioInit(const std::string& name)
{
    systemAcLossLine = gpiod::find_line(name);
    if (!systemAcLossLine)
    {
        std::cerr << "Failed to find the " << name << " line" << std::endl;
        return false;
    }

    setSystemAcLossGpio(psu1AcLossValue,psu2AcLossValue,false);

    try
    {
        systemAcLossLine.request({"psu-status-led-handler", gpiod::line_request::DIRECTION_OUTPUT, 0});
    }
    catch (std::exception&)
    {
        std::cerr << "Failed to request input for " << name << std::endl;
        return false;
    }

    setSystemAcLossGpio(psu1AcLossValue,psu2AcLossValue,true);

    return true;
}


int main(int argc, char *argv[])
{
    std::cerr << "MCT PSU status LED handler start to monitor." << std::endl;

    if (std::filesystem::exists(pdbConfigPath))
    {
        pdbConfig = true;
    }

    psu1AcLossGpioInit(psu1AcLossName);
    psu2AcLossGpioInit(psu2AcLossName);
    systemAcLossGpioInit(systemAcLossName);
    
    io.run();
    return 0;
}