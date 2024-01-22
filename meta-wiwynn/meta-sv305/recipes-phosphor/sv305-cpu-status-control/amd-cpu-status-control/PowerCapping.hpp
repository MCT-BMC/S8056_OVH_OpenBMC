#pragma once
#include <stdio.h>
#include <stdlib.h>

#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/streambuf.hpp>
#include <openbmc/libobmcdbus.hpp>
#include <sdbusplus/asio/object_server.hpp>

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <variant>
#include <unistd.h>
#include <time.h>
extern "C"
{
#include "esmi_common.h"
#include "esmi_cpuid_msr.h"
#include "esmi_mailbox.h"
#include "esmi_rmi.h"
#include "esmi_tsi.h"
}

namespace fs = std::filesystem;

#define DEFAULT_FIFO_BUFFER_SIZE 5
#define MAX_POWER_LIMIT 180000
#define MIN_POWER_LIMIT 100000
#define POWER_LIMIT_THRESHOLD 175000
#define RATE_OF_DECLINE_IN_MWATTS 5000
#define FAST_APPLY_POWER_CAP 40000
#define SLOW_RELEASE_POWER_CAP 20000
#define FAST_RELEASE_POWER_CAP 80000

const static constexpr char* serviceName =
    "xyz.openbmc_project.AMD_CPU.Control";
const static constexpr char* objPathName =
    "/xyz/openbmc_project/AMD_CPU/Control";
const static constexpr char* powerControlIntfName =
    "xyz.openbmc_project.PowerStatus.Control";
const static constexpr char* parameterIntfName =
    "xyz.openbmc_project.AMD_CPU.Control.Parameters";
const static constexpr char* operationalIntfName =
    "xyz.openbmc_project.PowerCapping.OperationalStatus";

const static constexpr char *amdCpuPowerCapPath =
    "/var/lib/amd-cpu-power-cap/";
const static constexpr char *powerCapName =
    "PowerCapStatus.json";

typedef struct qnode
{
    double data;
    struct qnode *prev, *next;
} QNODE;

typedef struct queue
{
    struct qnode *front, *rear;
    int size;
    int entries;
} QUEUE;

class PowerCapping
{
  public:
    PowerCapping(boost::asio::io_service& io, int pollingMs,
                 sdbusplus::asio::object_server& objectServer,
                 std::shared_ptr<sdbusplus::asio::connection>& connection);
    ~PowerCapping();

  private:
    void psuQueueInit(int size);
    int psuQueueDelNode();
    void psuQueueAddNode(double data);
    int stopSystemPowerCapping(int sockId);
    int setParameters(int sockId, uint32_t power);
    void checkAndCapPower();
    void calcMovingAvg();
    int readPsuPin();
    int startSystemPowerCapping(void);
    std::tuple<int, uint32_t> getCpuPowerLimit(int sockId);
    std::tuple<int, uint32_t> getCpuPowerConsumption(int sockId);
    int setCpuPowerLimit(int sockId, uint32_t power);
    int setDefaultCpuPowerLimit(int sockId);
    bool isCrashdump();
    sdbusplus::asio::object_server& objServer;
    std::shared_ptr<sdbusplus::asio::connection>& conn;
    std::shared_ptr<sdbusplus::asio::dbus_interface> operationalInterface = nullptr;
    std::shared_ptr<sdbusplus::asio::dbus_interface> parameterInterface = nullptr;
    QUEUE* psuQueue;
    boost::asio::steady_timer waitTimer;
    int pollingMs;
    uint32_t cpuSocketId = 0;
    uint32_t systemPowerLimit;
    uint8_t fastReleaseTimeout = 20;
    uint8_t slowReleaseTimeout = 20;
    double psuPIN;
    double psuPINMovingAvg;
    uint32_t cpuPowerLimit;
    std::string isPowerCapEnabled = "OFF";
    bool isAsserted = false;
    bool debugModeEnabled = false;
    std::timed_mutex esmiMutex;
    int esmiMutexTimout = 3000; // 3 sec
};
