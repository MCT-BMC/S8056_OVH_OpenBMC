#pragma once

#include "Thresholds.hpp"
#include "sensor.hpp"

#include <sdbusplus/asio/object_server.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/container/flat_map.hpp>

#include <iostream>
#include <unistd.h>

static constexpr const char* patternFilePath = "/etc/sol-pattern.json";

class SolPatternSensor : public Sensor
{
    public:
        SolPatternSensor(std::shared_ptr<sdbusplus::asio::connection>& conn,
                         boost::asio::io_service& io,
                         const std::string& sensorName,
                         const std::string& sensorConfiguration,
                         sdbusplus::asio::object_server& objectServer,
                         std::vector<thresholds::Threshold>&& thresholdData,
                         double max, double min, std::string patternInfoString);
        ~SolPatternSensor();

        void updatePaternHandler(void);

    private:
        sdbusplus::asio::object_server& objServer;
        boost::asio::steady_timer waitTimer;
        std::string patternInfoString;
        int errCount;
        void checkThresholds(void) override;
        bool setPattern(const std::string& newValue, std::string& oldValue);
};
