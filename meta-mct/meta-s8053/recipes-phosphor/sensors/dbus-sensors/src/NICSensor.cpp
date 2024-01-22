#include <unistd.h>

#include <openbmc/libobmci2c.h>

#include <NICSensor.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>

#include <iostream>
#include <istream>
#include <limits>
#include <memory>
#include <string>
#include <system_error>
#include <vector>

static constexpr bool DEBUG = false;
static constexpr const char* sensorPathPrefix = "/xyz/openbmc_project/sensors/";

static constexpr unsigned int sensorPollMs = 1000;
static constexpr unsigned int sensorScaleFactor = 1000;
static constexpr size_t warnAfterErrorCount = 10;

NICSensor::NICSensor(const std::string& path, const std::string& objectType,
                       sdbusplus::asio::object_server& objectServer,
                       std::shared_ptr<sdbusplus::asio::connection>& conn,
                       boost::asio::io_service& io, const std::string& sensorName,
                       std::vector<thresholds::Threshold>&& _thresholds,
                       const std::string& sensorConfiguration,
                       const PowerState& powerState,
                       std::string& sensorTypeName, const double MaxValue,
                       const double MinValue, const uint8_t busId,
                       const uint8_t slaveAddr, const uint8_t cmdCode,
                       const uint8_t readMode, const double scaleVal,
                       const double offsetVal, const int tctlMax):
    Sensor(escapeName(sensorName),
           std::move(_thresholds), sensorConfiguration, objectType, false, false, MaxValue,
           MinValue, conn, powerState),
    std::enable_shared_from_this<NICSensor>(), objServer(objectServer),
    path(path), waitTimer(io),
    busId(busId), slaveAddr(slaveAddr), cmdCode(cmdCode),
    readMode(readMode), sensorType(sensorTypeName), scaleVal(scaleVal),
    offsetVal(offsetVal), tctlMax(tctlMax), thresholdTimer(io)
{
    std::string dbusPath = sensorPathPrefix + sensorTypeName + name;

    sensorInterface = objectServer.add_interface(
                          dbusPath, "xyz.openbmc_project.Sensor.Value");
    if(tctlMax)
    {
        sensorInterface->register_property("TctlMAX", tctlMax);
    }

    for (const auto& threshold : thresholds)
    {
        std::string interface = thresholds::getInterface(threshold.level);
        thresholdInterfaces[static_cast<size_t>(threshold.level)] =
            objectServer.add_interface(dbusPath, interface);
    }
    association =
        objectServer.add_interface(dbusPath, association::interface);

    setInitialProperties(sensor_paths::unitDegreesC);
}

NICSensor::~NICSensor()
{
    // close the Timer to cancel async operations
    waitTimer.cancel();
    for (const auto& iface : thresholdInterfaces)
    {
        objServer.remove_interface(iface);
    }
    objServer.remove_interface(sensorInterface);
    objServer.remove_interface(association);
}

void NICSensor::setupRead(void)
{
    if (!readingStateGood())
    {
        markAvailable(false);
    }
    else
    {
        handleResponse();
    }

    std::weak_ptr<NICSensor> weakRef = weak_from_this();

    waitTimer.expires_from_now(boost::asio::chrono::milliseconds(sensorPollMs));
    waitTimer.async_wait([weakRef](const boost::system::error_code& ec) {
        if (ec == boost::asio::error::operation_aborted)
        {
            return; // we're being canceled
        }
        std::shared_ptr<NICSensor> self = weakRef.lock();
        if (self)
        {
            self->setupRead();
        }
    });
}

void NICSensor::handleResponse()
{
    double senValue;
    if (readNicReg(senValue))
    {
        updateValue(senValue);
    }
    else
    {
        incrementError();
    }
}

bool NICSensor::readNicReg(double& senValue)
{
    int fd = -1;
    int res = -1;
    std::vector<char> filename;
    filename.assign(20, 0);

    fd = open_i2c_dev(busId, filename.data(), filename.size(), 0);
    if (fd < 0)
    {
        std::cerr << "Fail to open I2C device: " << path << "\n";
        return false;
    }

    std::vector<uint8_t> cmdData;
    cmdData.assign(1, cmdCode);
    std::vector<uint8_t> readBuf;
    uint32_t raw_value;
    switch (readMode)
    {
        case static_cast<int>(ReadMode::READ_BYTE): // Byte
            readBuf.assign(1, 0x0);
            res = i2c_master_write_read(fd, slaveAddr, cmdData.size(), cmdData.data(),
                                        readBuf.size(), readBuf.data());
            raw_value = readBuf.at(0);
            break;
        default:
            res = -1;
    }

    if (res < 0)
    {
        if (DEBUG)
        {
            std::cerr << "Path: " << path
                      << ", Addr: " << slaveAddr << "\n";
        }
        close_i2c_dev(fd);
        return false;
    }

    if (raw_value != 0 )
    {
        senValue = raw_value * scaleVal + offsetVal;
    }

    close_i2c_dev(fd);
    return true;
}

void NICSensor::checkThresholds(void)
{
    if (!readingStateGood())
    {
        return;
    }

    thresholds::checkThresholdsPowerDelay(weak_from_this(), thresholdTimer);
}
