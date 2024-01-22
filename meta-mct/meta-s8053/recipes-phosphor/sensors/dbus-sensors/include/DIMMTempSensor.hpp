#pragma once
#include "sensor.hpp"

#include <boost/asio/deadline_timer.hpp>
#include <boost/container/flat_map.hpp>
#include <chrono>
#include <limits>
#include <vector>

struct DIMMTempSensor : public Sensor
{
    DIMMTempSensor(std::shared_ptr<sdbusplus::asio::connection>& conn,
                  boost::asio::io_service& io,
                  const std::string& sensorClass,
                  const std::string& muxName,
                  const std::string& sensorName,
                  const std::string& sensorConfiguration,
                  sdbusplus::asio::object_server& objectServer,
                  std::vector<thresholds::Threshold>&& thresholds,
                  PowerState readState,
                  uint8_t muxBusId, uint8_t muxAddress,
                  uint8_t dimmAddress, uint8_t tempReg,
                  uint8_t offset,uint8_t dimmGroupCounter,
                  uint8_t delayTime);
    ~DIMMTempSensor();

    void checkThresholds(void) override;
    void read(void);
    void init(void);

    std::string muxName;
    uint8_t muxBusId;
    uint8_t muxAddress;
    uint8_t dimmAddress;
    uint8_t tempReg;
    uint8_t offset;
    uint8_t dimmGroupCounter;
    uint8_t delayTime;
    bool enableMux;

  private:
    int retry;
    double upperCritical;
    void selectMux();
    int getDimmRegsInfoWord(uint8_t dimmAddress, uint8_t regs, int32_t* pu16data, double* rawValue);
    sdbusplus::asio::object_server& objectServer;
    std::shared_ptr<sdbusplus::asio::connection> dbusConnection;
    boost::asio::deadline_timer waitTimer;
    uint8_t PowerDelayCounter;
};
