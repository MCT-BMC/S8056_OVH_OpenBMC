#include "Utils.hpp"

#include <openbmc/libobmci2c.h>
#include <unistd.h>

#include <NICSensor.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <iostream>
#include <limits>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <string>
#include <systemd/sd-journal.h>

static constexpr bool DEBUG = false;
static constexpr const char* sensorPathPrefix = "/xyz/openbmc_project/sensors/";

static constexpr unsigned int sensorPollMs = 1000;
static constexpr unsigned int sensorScaleFactor = 1000;
static constexpr size_t warnAfterErrorCount = 10;

static constexpr int bmcSlaveAddr = 0x10; // 7-bit bmc slave address
static std::vector<uint8_t> mctpGetTempCmd1 = {0x0F, 0x0A, 0x21,
										       0x01, 0x00, 0x60, 0xCF,
										       0x00, 0x85, 0x01, 0x00,
										       0x01, 0x20};
static constexpr const int mctpGetTempCmd1ResLen = 16;

static std::vector<uint8_t> mctpGetTempCmd2 = {0x0F, 0x1A, 0x21,
										       0x01, 0x01, 0x60, 0xCA,
										       0x02, 0x00, 0x01, 0x00,
										       0x09, 0x00, 0x00, 0x00,
										       0x00, 0x00, 0x00, 0x00,
										       0x00, 0x00, 0x00, 0x00,
										       0x00, 0x00, 0x00, 0x00,
										       0x00, 0x93};
static constexpr const int mctpGetTempCmd2ResLen = 34;

static std::vector<uint8_t> mctpGetTempCmd3 = {0x0F, 0x22, 0x21,
										       0x01, 0x01, 0x60, 0xC9,
										       0x02, 0x00, 0x01, 0x00,
										       0x35, 0x50, 0x00, 0x00,
										       0x05, 0x00, 0x00, 0x00,
										       0x00, 0x00, 0x00, 0x00,
										       0x00, 0x00, 0x00, 0x01,
										       0x57, 0x4B, 0x00, 0x00,
										       0x00, 0x00, 0x00, 0x00,
										       0x00, 0x92};
static constexpr const int mctpGetTempCmd3ReqLen = 37;
static constexpr const int mctpGetTempCmd3ResLen = 42;

NICSensor::NICSensor(const std::string& path, const std::string& objectType,
                       sdbusplus::asio::object_server& objectServer,
                       std::shared_ptr<sdbusplus::asio::connection>& conn,
                       boost::asio::io_service& io, const std::string& sensorName,
                       std::vector<thresholds::Threshold>&& _thresholds,
                       const std::string& sensorConfiguration,
                       std::string& sensorTypeName, const double MaxValue,
                       const double MinValue, const uint8_t busId,
                       const uint8_t slaveAddr, const uint8_t cmdCode,
                       const uint8_t readMode, const double scaleVal,
                       const double offsetVal, const int tctlMax):
    Sensor(escapeName(sensorName),
           std::move(_thresholds), sensorConfiguration, objectType, false, false, MaxValue,
           MinValue, conn, PowerState::on), std::enable_shared_from_this<NICSensor>(),
    path(path), objServer(objectServer), waitTimer(io), errCount(0),
    senValue(0), busId(busId), slaveAddr(slaveAddr), cmdCode(cmdCode),
    readMode(readMode), sensorType(sensorTypeName), scaleVal(scaleVal), mctpIID(0x1),
    offsetVal(offsetVal), tctlMax(tctlMax), thresholdTimer(io), mctpCmd12Set(false)
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
    setupPowerMatch(conn);
    setupRead();
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
    std::weak_ptr<NICSensor> weakRef = weak_from_this();
    bool res = false;

    if (!readingStateGood())
    {
        mctpCmd12Set = false;
        markAvailable(false);
        res = true;
    }
    else
    {
        res = visitNICReg();

        if (res != true)
        {
            res = mctpGetTemp();
            if (res != true)
            {
                incrementError();
            }
        }
    }

    if (res == true)
    {
        handleResponse();
    }

    waitTimer.expires_from_now(boost::asio::chrono::milliseconds(sensorPollMs));
    waitTimer.async_wait([weakRef](const boost::system::error_code & ec) {
        if (ec == boost::asio::error::operation_aborted)
        {
            return; // we're being canceled
        }
        std::shared_ptr<NICSensor> self = weakRef.lock();
        if (!self)
        {
            return;
        }
        self->setupRead();
    });
}

bool NICSensor::visitNICReg(void)
{
    int fd = -1;
    int res = -1;
    std::vector<char> filename;
    filename.assign(20, 0);
/*
    if (!readingStateGood())
    {
        markAvailable(false);
        return true;
    }
*/
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

bool NICSensor::mctpGetTemp(void)
{
    std::vector<char> filename(32, 0);
    int outFd = -1;
    outFd =  open_i2c_dev(busId, filename.data(), filename.size(), 0);
    if (outFd < 0)
    {
        if constexpr (DEBUG)
        {
            std::cerr << "[MCTP] Error in open MCTP I2C device.\n";
        }
        return false;
    }

    int inFd = -1;
    inFd = open_i2c_slave_dev(busId, bmcSlaveAddr);
    if (inFd < 0)
    {
        if constexpr (DEBUG)
        {
            std::cerr << "[MCTP] Error in open slave mqueue device.\n";
        }
        close_i2c_dev(outFd);
        return false;
    }

    // Step1
    int res = -1;
    uint8_t mctpAddr = 0x32;
    uint8_t mctpAddrPec = 0x3b;
    std::vector<uint8_t> mctpResp(255,0);

    if (mctpCmd12Set == false)
    {
        i2c_slave_clear_buffer(inFd);
        res = i2c_master_write(outFd, mctpAddr, mctpGetTempCmd1.size(), mctpGetTempCmd1.data());
        if (res < 0)
        {
            std::cerr << "[MCTP] Error in master wirte in Cmd1.\n";
            close_i2c_dev(inFd);
            close_i2c_dev(outFd);
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        res = i2c_slave_read(inFd, mctpResp.data());
        if (res <= 0)
        {
            std::cerr << "[MCTP] Error in reading data from mqueue for Cmd1.\n";
            close_i2c_dev(inFd);
            close_i2c_dev(outFd);
            return false;
        }

        if (res != mctpGetTempCmd1ResLen)
        {
            std::cerr << "[MCTP] Error in resLen of Cmd1.\n";
            close_i2c_dev(inFd);
            close_i2c_dev(outFd);
            return false;
        }

        if (DEBUG)
        {
            for (int j=0; j<res; j++)
            {
                sd_journal_print(LOG_INFO, "[CMD1][%x]",
                                            mctpResp.at(j));
            }
        }

        uint8_t cmd1_pec = i2c_smbus_pec(0, mctpResp.data(), (res-1));
        if (cmd1_pec != mctpResp.at(res-1))
        {
            std::cerr << "[MCTP] Error in PEC of Cmd1.\n";
            close_i2c_dev(inFd);
            close_i2c_dev(outFd);
            return false;
        }

        i2c_slave_clear_buffer(inFd);
        res = i2c_master_write(outFd, mctpAddr, mctpGetTempCmd2.size(), mctpGetTempCmd2.data());
        if (res < 0)
        {
            std::cerr << "[MCTP] Error in master wirte in Cmd2.\n";
            close_i2c_dev(inFd);
            close_i2c_dev(outFd);
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        res = i2c_slave_read(inFd, mctpResp.data());
        if (res <= 0)
        {
            std::cerr << "[MCTP] Error in reading data from mqueue for Cmd2.\n";
            close_i2c_dev(inFd);
            close_i2c_dev(outFd);
            return false;
        }

        if (res != mctpGetTempCmd2ResLen)
        {
            std::cerr << "[MCTP] Error in resLen of Cmd2.\n";
            close_i2c_dev(inFd);
            close_i2c_dev(outFd);
            return false;
        }

        uint8_t cmd2_pec = i2c_smbus_pec(0, mctpResp.data(), (res-1));
        if (cmd2_pec != mctpResp.at(res-1))
        {
            std::cerr << "[MCTP] Error in PEC of Cmd2.\n";
            close_i2c_dev(inFd);
            close_i2c_dev(outFd);
            return false;
        }
    }
    mctpCmd12Set = true;

    // Get Temperature
    // Specify the IID

    mctpGetTempCmd3.at(11) = mctpIID;
    uint8_t cmd3req_pec = i2c_smbus_pec(mctpAddrPec, mctpGetTempCmd3.data(),
                                       (mctpGetTempCmd3ReqLen-1));
    mctpGetTempCmd3.at(mctpGetTempCmd3ReqLen-1) = cmd3req_pec;

    i2c_slave_clear_buffer(inFd);
    res = i2c_master_write(outFd, mctpAddr, mctpGetTempCmd3.size(), mctpGetTempCmd3.data());
    if (res < 0)
    {
        std::cerr << "[MCTP] Error in master wirte in Cmd3.\n";
        close_i2c_dev(inFd);
        close_i2c_dev(outFd);
        return false;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    res = i2c_slave_read(inFd, mctpResp.data());
    if (res <= 0)
    {
        std::cerr << "[MCTP] Error in reading data from mqueue for Cmd3.\n";
        close_i2c_dev(inFd);
        close_i2c_dev(outFd);
        return false;
    }

    if (res != mctpGetTempCmd3ResLen)
    {
        std::cerr << "[MCTP] Error in resLen of Cmd3.\n";
        close_i2c_dev(inFd);
        close_i2c_dev(outFd);
        return false;
    }

    uint8_t cmd3_pec = i2c_smbus_pec(0, mctpResp.data(), (res-1));
    if (cmd3_pec != mctpResp.at(res-1))
    {
        std::cerr << "[MCTP] Error in PEC of Cmd3.\n";
        close_i2c_dev(inFd);
        close_i2c_dev(outFd);
        return false;
    }

    senValue = mctpResp.at(36);
    if (DEBUG)
    {
        sd_journal_print(LOG_CRIT, "[MCTP Temp][Max %d][Curr %d]\n",
                         mctpResp.at(35), mctpResp.at(36));
    }

    close_i2c_dev(inFd);
    close_i2c_dev(outFd);

    // Increase the IID
    if (0xFF == mctpIID)
    {
        mctpIID = 0x1;
    }
    else
    {
        mctpIID++;
    }

    return true;
}

void NICSensor::handleResponse()
{
    if (readingStateGood())
    {
        if (static_cast<double>(senValue) != value)
        {
            updateValue(senValue);
        }
    }
}

void NICSensor::checkThresholds(void)
{
    if (!readingStateGood())
    {
        markAvailable(false);
        return;
    }

    thresholds::checkThresholdsPowerDelay(weak_from_this(), thresholdTimer);
}
