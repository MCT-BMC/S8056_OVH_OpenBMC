#pragma once
#include <sensor.hpp>

#include <boost/asio/deadline_timer.hpp>
#include <boost/container/flat_map.hpp>
#include <chrono>
#include <limits>
#include <memory>
#include <string>
#include <vector>

struct EventSensor : public Sensor
{
    EventSensor(std::shared_ptr<sdbusplus::asio::connection>& conn,
                  const std::string& objectType,
                  const std::string& name,
                  const std::string& sensorType,
                  const std::string& sensorConfiguration,
                  sdbusplus::asio::object_server& objectServer,
                  std::vector<thresholds::Threshold>&& thresholds,
                  const std::string& startUnit
                  );
    ~EventSensor() override;

    void checkThresholds(void) override;
    void startInitService(void);
    void init(void);

  private:
    sdbusplus::asio::object_server& objectServer;
    std::string sensorType;
    std::string startUnit;
};
