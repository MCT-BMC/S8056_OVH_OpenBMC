#pragma once
#include "sensor.hpp"

#include <boost/asio/steady_timer.hpp>
#include <boost/container/flat_map.hpp>
#include <chrono>
#include <limits>
#include <vector>
#include <fstream>

struct VirtualSensor : public Sensor
{
    VirtualSensor(std::shared_ptr<sdbusplus::asio::connection>& conn,
                  boost::asio::io_service& io, const std::string& name,
                  const std::string& sensorConfiguration,
                  sdbusplus::asio::object_server& objectServer,
                  std::vector<thresholds::Threshold>&& thresholds,
                  double maxReading, double minReading,
                  size_t pollTime, int selectMode, std::string sensorUnit,
                  std::vector<std::string> inputParameter,
                  std::vector<std::string> inputSensorUnit,
                  std::vector<double> inputScaleFactor,
                  std::vector<std::string> inputExpression);
    ~VirtualSensor();

    void checkThresholds(void) override;
    void read(void);
    void init(void);
    int getParameterValue(double& value, std::string path, double scaleFactor);

    int selectMode;
    size_t pollTime;
    std::string sensorUnit;
    std::vector<std::string> inputParameter;
    std::vector<std::string> inputSensorUnit;
    std::vector<double> inputScaleFactor;
    std::vector<std::string> inputExpression;
    std::vector<int> devErr{std::vector<int>(10,0)};

  private:
    sdbusplus::asio::object_server& objectServer;
    std::shared_ptr<sdbusplus::asio::connection> dbusConnection;
    boost::asio::steady_timer waitTimer;
};