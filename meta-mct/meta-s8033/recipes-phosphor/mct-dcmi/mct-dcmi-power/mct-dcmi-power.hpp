#pragma once

#include <iostream>
#include <chrono>
#include <ctime>
#include <algorithm>
#include <fstream>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <boost/format.hpp>
#include <boost/circular_buffer.hpp>

#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/asio/sd_event.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/exception.hpp>
#include <sdbusplus/server.hpp>
#include <sdbusplus/timer.hpp>
#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/log.hpp>
#include <xyz/openbmc_project/State/Host/server.hpp>


//#include <amd/libapmlpower.hpp> not mature enough 
extern "C"
{
#include "esmi_common.h"
#include "esmi_cpuid_msr.h"
#include "esmi_mailbox.h"
#include "esmi_rmi.h"
#include "esmi_tsi.h"
}


namespace fs = std::filesystem;
using namespace phosphor::logging;
using namespace sdbusplus::xyz::openbmc_project::State::server;

constexpr auto FAST_APPLY_POWER_CAP = 40000;
constexpr auto RATE_OF_DECLINE_IN_MWATTS = 5000;
constexpr auto MAX_POWER_LIMIT = 240000;
constexpr auto MIN_POWER_LIMIT = 100000;
constexpr auto SLOW_RELEASE_POWER_CAP = 20000;
constexpr auto FAST_RELEASE_POWER_CAP = 80000;

constexpr auto mW = 1000;

constexpr auto MILLI_SECONDS = 1000;
constexpr auto MICRO_SECONDS = 1000000;
constexpr auto MAX_COLLECTION_POWER_SIZE = 65535;

constexpr auto DCMI_SERVICE = "xyz.openbmc_project.DCMI";
constexpr auto DCMI_POWER_PATH = "/xyz/openbmc_project/DCMI/Power";
constexpr auto DCMI_POWER_INTERFACE = "xyz.openbmc_project.DCMI.Value";
constexpr auto PCAP_PATH = "/xyz/openbmc_project/control/host0/power_cap";
constexpr auto PCAP_INTERFACE = "xyz.openbmc_project.Control.Power.Cap";

constexpr char const* powerlimitSensorPath= "/xyz/openbmc_project/sensors/chassis/SYS_PWR_CAPPING";

constexpr auto PERIOD_MAX_PROP = "PeriodMaxValue";
constexpr auto PERIOD_MIN_PROP = "PeriodMinValue";
constexpr auto PERIOD_AVERAGE_PROP = "PeriodAverageValue";
constexpr auto POWER_CAP_PROP = "PowerCap";
constexpr auto POWER_CAP_ENABLE_PROP = "PowerCapEnable";
constexpr auto EXCEPTION_ACTION_PROP = "ExceptionAction";
constexpr auto CORRECTION_TIME_PROP = "CorrectionTime";
constexpr auto SAMPLING_PERIOD_PROP = "SamplingPeriod";

#ifdef DBUS_MODE
static constexpr const char* POWER_SERVICE = "xyz.openbmc_project.VirtualSensor";
static constexpr const char* POWER_PATH = "/xyz/openbmc_project/sensors/power/SYSTEM_POWER";
static constexpr const char* POWER_INTERFACE = "xyz.openbmc_project.Sensor.Value";
#endif

constexpr std::chrono::microseconds DBUS_TIMEOUT = std::chrono::microseconds(5*1000000);

typedef struct{
    double time = 0;
    double value = 0;
}Power;

typedef struct{
    double max=0;
    double min=0;
    double average=0;
    bool powerCapEnable=true;
    bool actionEnable=true;
    uint16_t samplingPeriod=0;
    uint32_t correctionTime=0;
    uint32_t correctionTimeout=0;
    uint32_t powerCap=0;
    std::string exceptionAction="";
    std::string powerPath="";
    std::vector<Power> collectedPower;
    boost::circular_buffer<double> cb {5}; //todo, make it dynamic to support dcmi sampling period setting
}PowerStore;

class PSUProperty
{
  public:
    PSUProperty(std::string name, double max, double min, unsigned int factor) :
        labelTypeName(name), maxReading(max), minReading(min),
        sensorScaleFactor(factor)
    {
    }
    ~PSUProperty() = default;

    std::string labelTypeName;
    double maxReading;
    double minReading;
    unsigned int sensorScaleFactor;
};

static boost::container::flat_map<std::string,PSUProperty> labelMatch;

#ifdef SYSFS_MODE
/** @brief Read the value from hardware monitor path.
 *
 *  @param[in] path - hardware monitor path for power.
 */
int readPower(std::string path);
#endif

#ifdef DBUS_MODE
/** @brief Read the value from dbus path.
 *
 *  @param[in] path - dbus path for power.
 */
double readDbusPower(std::string path);
#endif

/** @brief Update current power value to dbus property.
 *
 *  @param[in] property - Dbus property.
 *  @param[in] newValue - Current power value.
 *  @param[in] oldValue - Stored power value.
 */
void updatePowerValue(std::string property,const double& newValue,double& oldValue);

/** @brief Initialize the property for match power label and power which stored at dbus.
 *
 *  @param[in] powerStore - Stored power struct.
 */
void propertyInitialize(PowerStore& powerStore);

/** @brief Initialize the dbus Service for dcmi power.
 *
 *  @param[in] powerStore - Stored power struct.
 */
void dbusServiceInitialize(PowerStore& powerStore);

#ifdef SYSFS_MODE
/** @brief Find the hardware monitor path for power.
 *
 *  @return On success returns path with stored power.
 */
std::string findPowerPath();
#endif

#if 0
/**
 * @brief Handle the DCMI power action
 *
 * @param[in] io - io context.
 * @param[in] delay - Delay time in micro second.
 */
void powerHandler(boost::asio::io_context& io,PowerStore& powerStore,double delay);
#endif

/** @brief Handle the correction timeout for power limit.
 *
 *  @param[in] currentPower - Current power with time and value.
 *  @param[in] powerStore - Stored power struct.
 */
void timeoutHandle(Power currentPower,PowerStore& powerStore);

class PowerHandler
{
  public:
    PowerHandler(boost::asio::io_service& io,PowerStore& powerStore, 
                std::shared_ptr<sdbusplus::asio::connection>& connection, int polling);
     ~PowerHandler();
  void cappingHandle(void); 

  private:
    std::shared_ptr<sdbusplus::asio::connection>& conn;
   
    int updateMovingAverage();
    std::tuple<int, uint32_t> getCpuPowerLimit(int sockId);
    std::tuple<int, uint32_t> getCpuPowerConsumption(int sockId);
    int setCpuPowerLimit(int sockId, uint32_t power);
    int setDefaultCpuPowerLimit(int sockId);
    void checkAndCapPower(void);
    bool pgood;
    bool cappingAssert;
    bool running = false;
    PowerStore& powerStore;
    boost::asio::steady_timer waitTimer;
    int polling;
    bool debugModeEnabled = false;
	double psuPINMovingAvg;
	double psuPIN;
	uint32_t cpuSocketId = 0;
	uint32_t cpuPowerLimit;
    std::timed_mutex esmiMutex;
    int esmiMutexTimout = 3000; // 3 sec
    uint8_t fastReleaseTimeout = 20;
    uint8_t slowReleaseTimeout = 20;
     
};


//esmi-library
//===========================================================================================
std::tuple<int, uint32_t> PowerHandler::getCpuPowerLimit(int sockId)
{
    oob_status_t ret;
    uint32_t power = 0;
    if (!esmiMutex.try_lock_for(boost::asio::chrono::milliseconds(esmiMutexTimout)))
    {
        if (debugModeEnabled)
        {
            std::cerr << "[Debug Mode]"
                      << "esmiMutex is still lock\n";
        }
        esmiMutex.unlock();
        esmiMutex.lock();
    }
    esmi_oob_init(sockId);

    /* Get the PowerLimit for a given socket index */
    ret = read_socket_power_limit(sockId, &power);
    if (ret != OOB_SUCCESS)
    {
        fprintf(stderr, "Failed: to get socket[%d] powerlimit, Err[%d]: %s\n",
                sockId, ret, esmi_get_err_msg(ret));
    }
    esmi_oob_exit();
    esmiMutex.unlock();

    if (debugModeEnabled)
    {
        std::cerr << "[Debug Mode]"
                  << "Get SockID[" << sockId << "] CPU Power Limit[" << power
                  << "]\n";
    }
    return std::make_tuple(ret, power);
}

//===========================================================================================
std::tuple<int, uint32_t> PowerHandler::getCpuPowerConsumption(int sockId)
{
    oob_status_t ret;
    uint32_t power = 0;

    if (!esmiMutex.try_lock_for(boost::asio::chrono::milliseconds(esmiMutexTimout)))
    {
        if (debugModeEnabled)
        {
            std::cerr << "[Debug Mode]"
                      << "esmiMutex is still lock\n";
        }
        esmiMutex.unlock();
        esmiMutex.lock();
    }
    esmi_oob_init(sockId);

    ret = read_socket_power(sockId, &power);
    if (ret != OOB_SUCCESS)
    {
        fprintf(stderr, "Failed: to get socket[%d] power, Err[%d]: %s\n",
                sockId, ret, esmi_get_err_msg(ret));
    }
    esmi_oob_exit();
    esmiMutex.unlock();

    if (debugModeEnabled)
    {
        std::cerr << "[Debug Mode]"
                  << "Get SockID[" << sockId << "] CPU Power Consumption["
                  << power << "]\n";
    }
    return std::make_tuple(ret, power);
}

//===========================================================================================

int PowerHandler::setCpuPowerLimit(int sockId, uint32_t power)
{
    oob_status_t ret;
    uint32_t max_power = 0;

    if (!esmiMutex.try_lock_for(boost::asio::chrono::milliseconds(esmiMutexTimout)))
    {
        if (debugModeEnabled)
        {
            std::cerr << "[Debug Mode]"
                      << "esmiMutex is still lock\n";
        }
        esmiMutex.unlock();
        esmiMutex.lock();
    }
    esmi_oob_init(sockId);

    ret = read_max_socket_power_limit(sockId, &max_power);
    if ((ret == OOB_SUCCESS) && (power > max_power))
    {
        fprintf(stderr,
                "Input power is more than max limit,"
                " So It set's to default max %.3f Watts\n",
                (double)max_power / 1000);
        power = max_power;
    }

    ret = write_socket_power_limit(sockId, power);
    if (ret != OOB_SUCCESS)
    {
        fprintf(stderr, "Failed: to set socket[%d] power_limit, Err[%d]: %s\n",
                sockId, ret, esmi_get_err_msg(ret));
    }
    esmi_oob_exit();
    esmiMutex.unlock();

    if (debugModeEnabled)
    {
        std::cerr << "[Debug Mode]"
                  << "Set SockID[" << sockId << "] CPU Power Limit[" << power
                  << "]\n";
    }
    return ret;
}
//===========================================================================================

int PowerHandler::setDefaultCpuPowerLimit(int sockId)
{
    oob_status_t ret;
    uint32_t max_power;

    if (!esmiMutex.try_lock_for(boost::asio::chrono::milliseconds(esmiMutexTimout)))
    {
        if (debugModeEnabled)
        {
            std::cerr << "[Debug Mode]"
                      << "esmiMutex is still lock\n";
        }
        esmiMutex.unlock();
        esmiMutex.lock();
    }
    esmi_oob_init(sockId);

    ret = read_max_socket_power_limit(sockId, &max_power);
    if (ret != OOB_SUCCESS)
    {
        fprintf(
            stderr,
            "Failed: to get socket[%d] default max power limit, Err[%d]: %s\n",
            sockId, ret, esmi_get_err_msg(ret));
        goto error_handler;
    }

    ret = write_socket_power_limit(sockId, max_power);
    if (ret != OOB_SUCCESS)
    {
        fprintf(stderr,
                "Failed: to set socket[%d] default power_limit, Err[%d]: %s\n",
                sockId, ret, esmi_get_err_msg(ret));
        goto error_handler;
    }
    esmi_oob_exit();
    esmiMutex.unlock();

    if (debugModeEnabled)
    {
        std::cerr << "[Debug Mode]"
                  << "Set SockID[" << sockId << "] Default CPU Power Limit["
                  << max_power << "]\n";
    }
    return OOB_SUCCESS;

error_handler:
    esmi_oob_exit();
    esmiMutex.unlock();
    return ret;
}
//===========================================================================================


