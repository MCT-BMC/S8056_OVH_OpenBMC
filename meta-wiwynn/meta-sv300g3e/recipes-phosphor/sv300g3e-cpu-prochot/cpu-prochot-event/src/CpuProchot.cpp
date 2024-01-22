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

#include "CpuProchot.hpp"

#include <unistd.h>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <fstream>
#include <limits>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <string>
#include <systemd/sd-journal.h>

static std::chrono::seconds pollInterval(1);
static uint32_t checkPoint1 = 10;    // 10 seconds
static uint32_t checkPoint2 = 60;    // 60 seconds

CpuProchotObject::CpuProchotObject(boost::asio::io_service& io,
    std::shared_ptr<sdbusplus::asio::connection>& conn,
    gpiod_line* line, gpiod_line_request_config& config,
    std::string sensorName):
    gpioLine(line), gpioConfig(config), gpioEventDescriptor(io),
    conn(conn), sensorName(sensorName), waitTimer(io),
    isProchotAssert(false), isProchotAssertAgain(false), checkStage(clsAssertFlag),
    counter1(0), counter2(0)
{
    int res = requestCpuProchotEvent();

    if(0 == res)
    {
        setupTimer();
    }
}

CpuProchotObject::~CpuProchotObject()
{
    gpiod_line_release(gpioLine);
    waitTimer.cancel();
}

bool CpuProchotObject::getPGoodProperty()
{
    std::variant<int> state;
    auto method = conn->new_method_call(powerStateService, powerStatePath,
                                        propertyInterface, "Get");
    method.append(powerStateInterface, "pgood");
    try
    {
        auto reply = conn->call(method);
        reply.read(state);

        if( 1 != std::get<int>(state))
        {
            return false;
        }
        else
        {
            return true;
        }
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        std::cerr << "Error in " << __func__ << "\n";
        return false;
    }
}

void CpuProchotObject::waitCpuProchotEvent()
{
    gpioEventDescriptor.async_wait(
        boost::asio::posix::stream_descriptor::wait_read,
        [this](const boost::system::error_code& ec) {
            if (ec)
            {
                std::cerr << "GPIO event read error: "<< sensorName <<"\n";
                return;
            }
            cpuProchotEventHandler();
        });
}

void CpuProchotObject::cpuProchotEventHandler()
{
    gpiod_line_event gpioLineEvent;
    if (gpiod_line_event_read_fd(gpioEventDescriptor.native_handle(),
                                 &gpioLineEvent) < 0)
    {
        std::cerr << "Failed to read gpioLineEvent from fd: "<< sensorName <<"\n";
    }

    if (isProchotAssert)
    {
        // std::cerr << "Contineous Assert!: "<< sensorName <<"\n";
        isProchotAssertAgain = true;
        waitCpuProchotEvent();
        return;
    }

    bool on1 = ::isAcpiPowerOn();
    bool on2 = ::isSysPwrOk();

    if (!on1 || !on2)
    {
        // std::cerr << "It is a false alert!: "<< sensorName <<"\n";
        waitCpuProchotEvent();
        return;
    }

    // std::cerr << "It is a real event!: "<< sensorName <<"\n";
    isProchotAssert = true;
    addCpuThrottleSEL(true);
    waitCpuProchotEvent();

    return;
}

int CpuProchotObject::requestCpuProchotEvent()
{
    if (gpiod_line_request(gpioLine, &gpioConfig, 0) < 0)
    {
        std::cerr << "Failed to request prochot line: "<< sensorName <<"\n";
        return -1;
    }

    int gpioLineFd = gpiod_line_event_get_fd(gpioLine);
    if (gpioLineFd < 0)
    {
        std::cerr << "Failed to get line fd: "<< sensorName <<"\n";
        return -1;
    }

    gpioEventDescriptor.assign(gpioLineFd);

    waitCpuProchotEvent();

    return 0;
}

void CpuProchotObject::runProchotEventAlgorithm()
{
    if (!isProchotAssert)
    {
        counter1 = 0;
        counter2 = 0;
        checkStage = clsAssertFlag;
        return;
    }

    bool on = ::isAcpiPowerOn();
    if (!on)
    {
        addCpuThrottleSEL(false);

        counter1 = 0;
        counter2 = 0;
        checkStage = clsAssertFlag;
        isProchotAssert = false;
        isProchotAssertAgain = false;

        return;
    }

    switch (checkStage)
    {
        case clsAssertFlag:
        {
            counter1++;
            checkStage = clsAssertFlag;

            if (counter1 >= checkPoint1)
            {
                counter1 = 0;
                isProchotAssertAgain = false;
                checkStage = waitAndChkAgn;
            }

            break;
        }

        case waitAndChkAgn:
        {
            counter2++;
            checkStage = waitAndChkAgn;

            if (counter2 >= checkPoint2)
            {
                counter2 = 0;
                int pinValue = gpiod_line_get_value(gpioLine);

                if ((!isProchotAssertAgain) &&
                    (1 == pinValue))
                {
                    addCpuThrottleSEL(false);
                    isProchotAssert = false;
                }
                checkStage = clsAssertFlag;
            }

             break;
        }

        default:
            break;
    }

    return;
}

void CpuProchotObject::setupTimer()
{
    runProchotEventAlgorithm();

    waitTimer.expires_from_now(std::chrono::steady_clock::duration(pollInterval));
    waitTimer.async_wait([&](const boost::system::error_code& ec) {
    if (ec == boost::asio::error::operation_aborted)
    {
        return; // we're being canceled
    }
        setupTimer();
    });
}

void CpuProchotObject::addCpuThrottleSEL(bool isAssert)
{
    uint16_t genId = 0x20;
    bool assert = isAssert;
    std::string dbusPath = sensorPathPrefix + sensorName;
    std::vector<uint8_t> eventData(3, 0xFF);
    eventData.at(0) = 0xA; // Processor Automatically Throttled

    sdbusplus::message::message selWrite = conn->new_method_call(
            ipmiSELService, ipmiSELPath, ipmiSELAddInterface, "IpmiSelAdd");
    selWrite.append(ipmiSELAddMessage, dbusPath, eventData, assert, genId);

    try
    {
        conn->call_noreply(selWrite);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        std::cerr << "Error in " << __func__ << "\n";
    }

    return;
}
