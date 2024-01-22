#pragma once

#include <Thresholds.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sensor.hpp>
#include <boost/asio/steady_timer.hpp>

enum class ReadMode
{
    READ_BYTE = 0
};

class NICSensor : public Sensor, public std::enable_shared_from_this<NICSensor>
{
public:
    NICSensor(const std::string& path, const std::string& objectType,
               sdbusplus::asio::object_server& objectServer,
               std::shared_ptr<sdbusplus::asio::connection>& conn,
               boost::asio::io_service& io, const std::string& sensorName,
               std::vector<thresholds::Threshold>&& thresholds,
               const std::string& sensorConfiguration,
               std::string& sensorTypeName, const double MaxValue,
               const double MinValue, const uint8_t busId,
               const uint8_t slaveAddr, const uint8_t cmdCode,
               const uint8_t readMode, const double scaleVal,
               const double offsetVal, const int tctlMax);
    ~NICSensor();
private:
    sdbusplus::asio::object_server& objServer;
    boost::asio::steady_timer waitTimer;
    std::string path;
    std::string& sensorType;
    int errCount;
    uint8_t busId;
    uint8_t slaveAddr;
    uint8_t cmdCode;
    uint8_t readMode;
    float senValue;
    double scaleVal;
    double offsetVal;
    int tctlMax;
    thresholds::ThresholdTimer thresholdTimer;
    int powerondelaytime;
    uint8_t mctpIID;
    bool mctpCmd12Set;
    void setupRead(void);
    bool visitNICReg(void);
    bool mctpGetTemp(void);
    void handleResponse(void);
    void checkThresholds(void) override;
};
