#pragma once
#include "sensor.hpp"

#include <boost/asio/steady_timer.hpp>
#include <boost/container/flat_map.hpp>
#include <chrono>
#include <limits>
#include <memory>
#include <string>
#include <vector>

struct I2cNvmeSensor : public Sensor
{
    I2cNvmeSensor(std::shared_ptr<sdbusplus::asio::connection>& conn,
                  boost::asio::io_service& io, const std::string& name,
                  const std::string& sensorConfiguration,
                  sdbusplus::asio::object_server& objectServer,
                  std::vector<thresholds::Threshold>&& thresholds,
                  uint8_t busId, uint8_t address);
    ~I2cNvmeSensor();

    void checkThresholds(void) override;
    void read(void);
    void init(void);

    uint8_t busId;
    uint8_t address;

  private:
    int getNVMeTemp(uint8_t* pu8data);
    sdbusplus::asio::object_server& objectServer;
    std::shared_ptr<sdbusplus::asio::connection> dbusConnection;
    boost::asio::steady_timer waitTimer;
};