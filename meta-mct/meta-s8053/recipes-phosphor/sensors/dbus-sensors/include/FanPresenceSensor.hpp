#pragma once
#include "sensor.hpp"

#include <boost/asio/steady_timer.hpp>
#include <boost/container/flat_map.hpp>
#include <chrono>
#include <limits>
#include <vector>
#include <fstream>
#include <gpiod.hpp>

namespace discrete
{
enum deviceState
{
    reserved = 0,
    Absent,
    Present,
};
}// namespace discrete

struct FanPresenceSensor : public Sensor
{
    FanPresenceSensor(std::shared_ptr<sdbusplus::asio::connection>& conn,
                  boost::asio::io_service& io, const std::string& name,
                  const std::string& sensorConfiguration,
                  sdbusplus::asio::object_server& objectServer,
                  std::vector<thresholds::Threshold>&& thresholds,
                  std::string gpioName, std::string path, 
                  std::string polarity, std::string eventMon,
                  PowerState readState);
    ~FanPresenceSensor();

    void checkThresholds(void) override;
    void init(void);
    void gpioValueHandler(bool value, bool init);
    void gpioHandler(void);
    void logSELEvent(std::string enrty, std::string path ,
                     uint8_t eventData0, uint8_t eventData1, uint8_t eventData2);
    void setFaultLed(std::string setPolarity, bool init);

    std::string gpioName;
    std::string path;
    std::string polarity;
    std::string eventMon;
    std::string sensorPath;
    bool gpioValue;

  private:
    sdbusplus::asio::object_server& objectServer;
    std::shared_ptr<sdbusplus::asio::connection> dbusConnection;
    boost::asio::steady_timer waitTimer;
    gpiod::line gpioLine;
};