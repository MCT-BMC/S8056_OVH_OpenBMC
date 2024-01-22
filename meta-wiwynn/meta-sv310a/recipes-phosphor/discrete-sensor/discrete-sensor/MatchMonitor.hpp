#pragma once

#include <openbmc/libobmcdbus.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/server.hpp>

#include <functional>
#include <iostream>
#include <map>
#include <string>

void cpuTempMonitor(std::shared_ptr<sdbusplus::asio::connection> conn,
                    std::string objPath, double& upperThrottledThres,
                    double& lowerThrottledThres)
{
    static bool isAsserted = false;
    auto cpuTempMatcherCallback =
        [conn, objPath, upperThrottledThres,
         lowerThrottledThres](sdbusplus::message::message& msg) {
            boost::container::flat_map<std::string, std::variant<double, int>>
                propertiesChanged;
            std::string objName;

            if (msg.is_method_error())
            {
                return;
            }

            // Check host is power and bios post status
            if (!isPgoodOn() || !isPostComplete())
            {
                return;
            }

            msg.read(objName, propertiesChanged);
            auto findValue = propertiesChanged.find("Value");
            if (findValue == propertiesChanged.end())
            {
                return;
            }
            double* sensorValue = std::get_if<double>(&findValue->second);
            std::vector<uint8_t> eventData(3, 0xFF);

            eventData[0] = static_cast<uint8_t>(0x0A);
            std::string message;

            if (!isAsserted && *sensorValue >= upperThrottledThres)
            {
                isAsserted = true;
                message = "CPU Status - Throttled Assert";
                if (ipmiSelAdd(message, objPath, eventData, isAsserted) != 0)
                {
                    std::cerr << "Fail to add CPU Status SEL \n";
                }
            }
            if (isAsserted && *sensorValue < lowerThrottledThres)
            {
                isAsserted = false;
                message = "CPU Status - Throttled Dessert";
                if (ipmiSelAdd(message, objPath, eventData, isAsserted) != 0)
                {
                    std::cerr << "Fail to add CPU Status SEL \n";
                }
            }
        };

    static sdbusplus::bus::match::match cpuStatusMatcher(
        static_cast<sdbusplus::bus::bus&>(*conn),
        sdbusplus::bus::match::rules::propertiesChanged(
            "/xyz/openbmc_project/sensors/temperature/CPU_Temp",
            "xyz.openbmc_project.Sensor.Value"),
        std::move(cpuTempMatcherCallback));
}

void batteryMonitor(std::shared_ptr<sdbusplus::asio::connection> conn,
                    std::string& monitorsensor, std::string& monitorevent,
                    std::string objPath)
{
    auto batteryLowMatcherCallback =
        [conn, monitorsensor, objPath](sdbusplus::message::message& msg) {
            // Get the event type and assertion details from the message
            std::string sensorName;
            std::string thresholdInterface;
            std::string event;
            bool assert;
            double assertValue;
            try
            {
                msg.read(sensorName, thresholdInterface, event, assert,
                         assertValue);
            }
            catch (sdbusplus::exception_t&)
            {
                std::cerr << "error getting assert signal data from "
                          << msg.get_path() << "\n";
                return;
            }

            // Check the asserted events to determine if we should log this
            // event
            std::pair<std::string, std::string> pathAndEvent(
                std::string(msg.get_path()), event);

            std::vector<uint8_t> eventData(3, 0xFF);
            std::string message = "Battery Assert";

            static bool logged = false;
            if (!logged && (sensorName == monitorsensor) &&
                (assert && (event.find("AlarmLow") != std::string::npos)))
            {
                eventData[0] = static_cast<uint8_t>(0x0);

                if (ipmiSelAdd(message, objPath, eventData, true) != 0)
                {
                    std::cerr << "Fail to add Battery SEL \n";
                }
                else
                {
                    logged = true;
                }
            }
        };
    if (monitorevent == "BatteryLow")
    {
        static sdbusplus::bus::match::match batteryLowMatcher(
            static_cast<sdbusplus::bus::bus&>(*conn),
            "type='signal', member='ThresholdAsserted'",
            std::move(batteryLowMatcherCallback));
    }
}
