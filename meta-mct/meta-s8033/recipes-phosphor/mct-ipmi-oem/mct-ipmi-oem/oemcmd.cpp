#include "oemcmd.hpp"
#include <host-ipmid/ipmid-api.h>
#include <fstream>
#include <stdio.h>
#include <string.h>
#include <systemd/sd-bus.h>
#include <linux/rtc.h>
#include <endian.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <iostream>
#include <variant>
#include <sstream>
#include <boost/algorithm/string.hpp>

#include <linux/types.h>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/message.hpp>
#include <sdbusplus/vtable.hpp>
#include <sdbusplus/server/interface.hpp>
#include <sdbusplus/timer.hpp>
#include <gpiod.hpp>

#include <ipmid/api.hpp>
#include <ipmid/utils.hpp>
#include <phosphor-logging/log.hpp>
#include <phosphor-logging/elog-errors.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/message/types.hpp>
#include <nlohmann/json.hpp>
#include <amd/libapmlpower.hpp>
#include "xyz/openbmc_project/Control/Power/RestoreDelay/server.hpp"

#include "version-utils.hpp"

#define FSC_SERVICE "xyz.openbmc_project.EntityManager"
#define FSC_OBJECTPATH "/xyz/openbmc_project/inventory/system/board/s8033_Fan_Table/Pid_"
#define PID_INTERFACE "xyz.openbmc_project.Configuration.Pid.Zone"
#define PROPERTY_INTERFACE "org.freedesktop.DBus.Properties"

const static constexpr char* SOL_PATTERN_SERVICE = "xyz.openbmc_project.SolPatternSensor";
const static constexpr char* SOL_PATTERN_OBJECTPATH = "/xyz/openbmc_project/sensors/pattern/Pattern";
const static constexpr char* VALUE_INTERFACE = "xyz.openbmc_project.Sensor.Value";

static constexpr const char* CHASSIS_SERVICE = "xyz.openbmc_project.State.Chassis";
static constexpr const char* CHASSIS_PATH = "/xyz/openbmc_project/state/chassis0";
static constexpr const char* CHASSIS_INTERFACE = "xyz.openbmc_project.State.Chassis";
static constexpr const char* CHASSIS_REQUEST = "CurrentPowerState";

const static constexpr char* POST_SERVICE = "xyz.openbmc_project.State.OperatingSystem";
const static constexpr char* POST_PATH = "/xyz/openbmc_project/state/os";
const static constexpr char* POST_INTERFACE = "xyz.openbmc_project.State.OperatingSystem.Status";
const static constexpr char* POST_REQUEST = "OperatingSystemState";

const static constexpr char* RTC_PATH = "/dev/rtc0";
const static constexpr char* SYS_LED_PATH = "/sys/class/leds/sys-fault/brightness";

using namespace phosphor::logging;
using namespace sdbusplus::xyz::openbmc_project::Control::Power::server;

namespace ipmi
{

void register_netfn_mct_oem() __attribute__((constructor));

static int getProperty(sdbusplus::bus::bus& bus, const std::string& path,
                 const std::string& property, double& value, const std::string service, const std::string interface)
{
    std::variant<double> valuetmp;
    auto method = bus.new_method_call(service.c_str(), path.c_str(), PROPERTY_INTERFACE, "Get");
    method.append(interface.c_str(),property);
    try
    {
        auto reply=bus.call(method);
        if (reply.is_method_error())
        {
            std::printf("Error looking up services, PATH=%s",interface.c_str());
            return -1;
        }
        reply.read(valuetmp);
    }
    catch (const sdbusplus::exception::SdBusError& e){
        return -1;
    }

    value = std::get<double>(valuetmp);
    return 0;
}

static void setProperty(sdbusplus::bus::bus& bus, const std::string& path,
                 const std::string& property, const double value)
{
    auto method = bus.new_method_call(FSC_SERVICE, path.c_str(),
                                      PROPERTY_INTERFACE, "Set");
    method.append(PID_INTERFACE, property, std::variant<double>(value));
	
    bus.call_noreply(method);

    return;
}

int logSelEvent(std::string enrty, std::string path ,
                     uint8_t eventData0, uint8_t eventData1, uint8_t eventData2)
{
    std::vector<uint8_t> eventData(3, 0xFF);
    eventData[0] = eventData0;
    eventData[1] = eventData1;
    eventData[2] = eventData2;
    uint16_t genid = 0x20;

    auto bus = sdbusplus::bus::new_default();

    sdbusplus::message::message writeSEL = bus.new_method_call(
        "xyz.openbmc_project.Logging.IPMI", "/xyz/openbmc_project/Logging/IPMI",
        "xyz.openbmc_project.Logging.IPMI", "IpmiSelAdd");
    writeSEL.append(enrty, path, eventData, true, (uint16_t)genid);
    try
    {
        bus.call(writeSEL);
    }
    catch (std::exception& e)
    {
        std::cerr << "Failed to log the button event:" << e.what() << "\n";
        return -1;
    }
    return 0 ;
}

int execmd(char* cmd,char* result) {
        char buffer[128];
        FILE* pipe = popen(cmd, "r");
        if (!pipe)
                return -1;

        while(!feof(pipe)) {
                if(fgets(buffer, 128, pipe)){
                        strcat(result,buffer);
                }
        }
        pclose(pipe);
        return 0;
}

int setBmcService(std::string service,std::string status, std::string setting)
{
    constexpr auto SYSTEMD_SERVICE = "org.freedesktop.systemd1";
    constexpr auto SYSTEMD_OBJ_PATH = "/org/freedesktop/systemd1";
    constexpr auto SYSTEMD_INTERFACE = "org.freedesktop.systemd1.Manager";

    auto bus = sdbusplus::bus::new_default();

    // start/stop servcie
    if(status != "Null"){
        auto method = bus.new_method_call(SYSTEMD_SERVICE, SYSTEMD_OBJ_PATH, SYSTEMD_INTERFACE, status.c_str());
        method.append(service, "replace");

        try
        {
            auto reply = bus.call(method);
        }
        catch (const sdbusplus::exception::SdBusError& e)
        {
            log<level::ERR>("Error in start/stop servcie",entry("ERROR=%s", e.what()));
            return -1;
        }
    }

    // enable/disable service
    if(setting != "Null"){
        auto method = bus.new_method_call(SYSTEMD_SERVICE, SYSTEMD_OBJ_PATH, SYSTEMD_INTERFACE,setting.c_str());

        std::array<std::string, 1> appendMessage = {service};

        if(setting == "EnableUnitFiles")
        {
            method.append(appendMessage, false, true);
        }
        else
        {
            method.append(appendMessage, false);
        }

        try
        {
            auto reply = bus.call(method);
        }
        catch (const sdbusplus::exception::SdBusError& e)
        {
            log<level::ERR>("Error in enable/disable service",entry("ERROR=%s", e.what()));
            return -1;
        }
    }

     return 0;
}

bool getOemServiceStatus(std::string service)
{
    constexpr auto SETTIMG_SERVICE = "xyz.openbmc_project.Settings";
    constexpr auto SERVICE_STATUS_PATH = "/xyz/openbmc_project/oem/ServiceStatus";
    constexpr auto SERVICE_STATUS_INTERFACE = "xyz.openbmc_project.OEM.ServiceStatus";

    auto bus = sdbusplus::bus::new_default();

    //Get web service status
    auto method = bus.new_method_call(SETTIMG_SERVICE, SERVICE_STATUS_PATH, PROPERTY_INTERFACE, "Get");
    method.append(SERVICE_STATUS_INTERFACE, service);

    std::variant<bool> result;
    try
    {
        auto reply = bus.call(method);
        reply.read(result);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        log<level::ERR>("Error in service status get",entry("ERROR=%s", e.what()));
    }

    return std::get<bool>(result);
}

bool cleadrCmos()
{
    int value = 1;

    auto line = gpiod::find_line("CLR_CMOS");

    if (!line)
    {
        std::cerr << "Error requesting gpio CLR_CMOS\n";
        return false;
    }

    try
    {
        line.request({"ipmid", gpiod::line_request::DIRECTION_OUTPUT,0}, value);
    }
    catch (std::system_error&)
    {
        std::cerr << "Error requesting gpio\n";
        return false;
    }

    line.set_value(0);
    sleep(3);
    line.set_value(1);

    line.release();
    return true;
}

bool setRtcTime(uint32_t selTime)
{
    int fd;
    struct rtc_time rtcTime;
    time_t t = selTime;
    struct tm *currentTime = gmtime(&t);

    std::string logMessage (300, 'x');


    fd = open(RTC_PATH, O_RDWR);
    if (fd ==  -1) {
        return false;
    }

    rtcTime.tm_year = currentTime->tm_year;
    rtcTime.tm_mday = currentTime->tm_mday;
    rtcTime.tm_mon = currentTime->tm_mon;
    rtcTime.tm_hour = currentTime->tm_hour;
    rtcTime.tm_min = currentTime->tm_min;
    rtcTime.tm_sec = currentTime->tm_sec;

    int ret = ioctl(fd, RTC_SET_TIME, &rtcTime);
    if (ret == -1) {
        return false;
    }
    close(fd);

    return true;
}

bool getBmcTime(time_t& timeSeconds)
{
    struct timespec sTime = {0};
    int ret = 0;
    ret = clock_gettime(CLOCK_REALTIME, &sTime);
    if (ret != 0)
    {
        return false;
    }

    timeSeconds = sTime.tv_sec;
    return true;
}

ipmi::Cc setManufactureFieledProperty(uint8_t selctField, uint8_t size,
                                      uint8_t& readCount, uint8_t& offsetMSB, uint8_t& offsetLSB)
{
    std::vector<uint8_t> validSize = {2, 1, 16, 16, 16, 6, 6, 32};

    if(size != validSize[selctField] && readCount==0)
    {
        return ipmi::ccReqDataLenInvalid;
    }

    readCount = validSize[selctField];

    switch(selctField)
    {
        case 0:
            // 0h-Product ID, 2 Bytes, LSB
            offsetMSB = 0x00;
            offsetLSB = 0x0d;
            break;
        case 1:
            // 1h-SKU ID, 1 Byte
            offsetMSB = 0x00;
            offsetLSB = 0x0f;
            break;
        case 2:
            // 2h-Device GUID, 16 Bytes
            offsetMSB = 0x00;
            offsetLSB = 0x10;
            break;
        case 3:
            // 3h-System GUID, 16 Bytes
            offsetMSB = 0x00;
            offsetLSB = 0x20;
            break;
        case 4:
            // 4h-Motherboard Serial Number, 16 Bytes
            offsetMSB = 0x00;
            offsetLSB = 0x30;
            break;
        case 5:
            //  5h-MAC 1 Address, 6 Bytes
            offsetMSB = 0x00;
            offsetLSB = 0x40;
            break;
        case 6:
            //  6h-MAC 2 Address, 6 Bytes
            offsetMSB = 0x00;
            offsetLSB = 0x46;
            break;
        case 7:
            // 7h-System Name, 32 Bytes
            offsetMSB = 0x00;
            offsetLSB = 0x4c;
            break;
        default:
            return ipmi::ccUnspecifiedError;
            break;
    }

    return ipmi::ccSuccess;
}

bool isPowerOn()
{
    auto bus = sdbusplus::bus::new_default();
    auto method = bus.new_method_call(CHASSIS_SERVICE, CHASSIS_PATH, PROPERTY_INTERFACE,"Get");
    method.append(CHASSIS_INTERFACE, CHASSIS_REQUEST);
    std::variant<std::string> result;
    try
    {
        auto reply = bus.call(method);
        reply.read(result);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        return false;
    }

    bool on = boost::ends_with(std::get<std::string>(result), "On");

    if(on)
    {
        return true;
    }

    return false;
}

bool hasBiosPost()
{
    auto bus = sdbusplus::bus::new_default();
    auto method = bus.new_method_call(POST_SERVICE, POST_PATH, PROPERTY_INTERFACE, "Get");
    method.append(POST_INTERFACE, POST_REQUEST);
    std::variant<std::string> result;
    try
    {
        auto reply = bus.call(method);
        reply.read(result);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        return false;
    }

    bool biosHasPost = std::get<std::string>(result) != "Inactive";

    if(biosHasPost)
    {
        return true;
    }

    return false;
}

bool biosRecoveryMode()
{
    uint8_t biosRecovery = 0x02;

    auto bus = sdbusplus::bus::new_default();
    auto method = bus.new_method_call(DBIOS_SERVICE, DBIOS_PATH, DBIOS_INTERFACE, "BiosAction");
    method.append(biosRecovery,(bool)true,(bool)true);
    try
    {
        auto reply=bus.call(method);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        return false;
    }
    return true;
}

static boost::container::flat_map<uint8_t, std::string> eccMap;
static void loadEccMap(void)
{
    //MCT ecc filter
    static constexpr const char* sdrFile = "/usr/share/ipmi-providers/sdr_event_only.json";
    constexpr static const uint16_t biosId = 0x3f;
    constexpr static const uint8_t sensorTypeMemory = 0xc;
    constexpr static const uint8_t eventDataMemCorrectableEcc = 0;

    static bool loaded = false;
    uint16_t ownerId;
    uint8_t sensorNum;
    uint8_t sensorType;
    std::string sensorName;

    if(loaded)
    {
        return;
    }
    loaded = true;

    eccMap.clear();
    std::ifstream sdrStream(sdrFile);
    if(!sdrStream.is_open())
    {
        std::cerr << "NO defined SDR found\n";
    }
    else
    {
        auto data = nlohmann::json::parse(sdrStream, nullptr, false);
        if (data.is_discarded())
        {
            std::cerr << "syntax error in " << sdrFile << "\n";
        }
        else
        {
            int idx = 0;
            while (!data[idx].is_null())
            {
                ownerId = std::stoul((std::string)data[idx]["ownerId"], nullptr, 16);
                sensorNum = std::stoul((std::string)data[idx]["sensorNumber"], nullptr, 16);
                sensorType = std::stoul((std::string)data[idx]["sensorType"], nullptr, 16);
                sensorName = data[idx]["sensorName"];
                if(biosId == ownerId && sensorTypeMemory == sensorType)
                {
                    eccMap[sensorNum] = sensorName;
                }
                idx++;
            }
        }
        sdrStream.close();
    }

    for(auto const& pair:eccMap)
    {
        std::cerr << (unsigned)pair.first << ":" << pair.second << '\n';
    }
}

static bool defaultCounterLoaded = false;
static uint8_t defaultDimmEccCounter = 0;

static std::string dimmArray[8][4] = {
    {"A0", "A1", "0", "0xC0002113"},
    {"B0", "B1", "0", "0xC0002123"},
    {"C0", "C1", "2", "0xC0002123"},
    {"D0", "D1", "2", "0xC0002113"},
    {"E0", "E1", "6", "0xC0002113"},
    {"F0", "F1", "6", "0xC0002123"},
    {"G0", "G1", "4", "0xC0002123"},
    {"H0", "H1", "4", "0xC0002113"},
};

static void loadDefaultDimmEccCounter(void)
{
    if(defaultCounterLoaded)
    {
        return;
    }
    defaultCounterLoaded = true;

    for(int i=0; i < sizeof(dimmArray) / sizeof(dimmArray[0]);i++)
    {
        uint64_t msrValue;
        uint32_t msrRegister;
        std::stringstream stringstreamMsrRegister;
        stringstreamMsrRegister << std::hex << dimmArray[i][3];
        stringstreamMsrRegister >> msrRegister;
        msrValue = getMsrValue(std::stoi(dimmArray[i][2]),msrRegister);
        msrValue = (msrValue & 0x00000fff00000000) >> 32;

        if(msrValue == 0)
        {
            continue;
        }

        uint8_t count = 0xfff - msrValue;

        if(count > defaultDimmEccCounter)
        {
            defaultDimmEccCounter = count;
        }
        std::cerr << "defaultDimmEccCounter : " << std::to_string(defaultDimmEccCounter) << std::endl;
    }

    if(defaultDimmEccCounter == 0)
    {
        defaultCounterLoaded = false;
    }
}

static sdbusplus::bus::match::match lpcResetAction(
    *getSdBus(),
    sdbusplus::bus::match::rules::type::signal() +
    sdbusplus::bus::match::rules::member(std::string("LpcReset")),
    [](sdbusplus::message::message& m) {
        uint32_t value;
        m.read(value);

        if(value == 0x01)
        {
            defaultCounterLoaded = false;
            defaultDimmEccCounter = 0;
        }
    });

//===============================================================
/* Set Fan Control Enable Command
NetFun: 0x2E
Cmd : 0x06
Request:
        Byte 1-3 : Tyan Manufactures ID (FD 19 00)
        Byte 4 :  00h - Fan Control Disable
                  01h - Fan Control Enable
                  ffh - Get Current Fan Control Status
Response:
        Byte 1 : Completion Code
        Byte 2-4 : Tyan Manufactures ID
        Byte 5 : Current Fan Control Status , present if FFh passed to Enable Fan Control in Request
*/
ipmi::RspType<uint8_t> ipmi_tyan_ManufactureMode(uint8_t mode)
{
    uint8_t currentStatus;
    auto bus = sdbusplus::bus::new_default();

    if (mode == 0x00)
    {
        // Disable Fan Control
        system("/usr/bin/env controlFan.sh manual-mode");
    }
    else if (mode == 0x01)
    {
        // Enable Fan Control
        system("/usr/bin/env controlFan.sh auto-mode");
    }
    else if (mode == 0xff)
    {
        // Get Current Fan Control Status
        std::ifstream fanModeFile("/run/fan-config");

        if(fanModeFile.good())
        {
            std::string readText;
            getline(fanModeFile, readText);
            currentStatus = std::stoi(readText);
        }
        else
        {
            currentStatus = 0x01;
        }

        return ipmi::responseSuccess(currentStatus);
    }
    else
    {
        return ipmi::responseUnspecifiedError();
    }

    return ipmi::responseSuccess();
}

//===============================================================

/* Set Fan Control PWM Duty Command
NetFun: 0x2E
Cmd : 0x05
Request:
        Byte 1-3 : Tyan Manufactures ID (FD 19 00)
        Byte 4 : PWM ID
            00h - Reserved
            01h - SYS PWM 1
            02h - SYS PWM 2
            03h - SYS PWM 3
            04h - SYS PWM 4
            05h - SYS PWM 5
            06h - SYS PWM 6

        Byte 5 : Duty Cycle
            0-64h - manual control duty cycle (0%-100%)
            FEh - Get current duty cycle
            FFh - Return to automatic control
Response:
        Byte 1 : Completion Code
        Byte 2-4 : Tyan Manufactures ID
        Byte 5 : Current Duty Cycle , present if 0xFE passed to Duty Cycle in Request
*/
ipmi::RspType<uint8_t> ipmi_tyan_FanPwmDuty(uint8_t pwmId, uint8_t duty)
{
    std::fstream file;
    int rc=0;
    char command[100];
    char temp[50];
    uint8_t responseDuty;
    uint8_t pwmValue = 0;
    char FSCStatus[100];
    uint8_t currentStatus;

    if (duty == 0xfe)
    {
        // Get current duty cycle
        switch (pwmId)
        {
            case 0:
                return ipmi::responseParmOutOfRange();
            case 1:
            case 2:
            case 3:
            case 4:
            case 5:
            case 6:
                    memset(command,0,sizeof(command));
                    sprintf(command, "cat /sys/class/hwmon/hwmon0/pwm%d",(7-pwmId));
                    break;
            default:
                    return ipmi::responseParmOutOfRange();
        }

        memset(temp, 0, sizeof(temp));
        rc = execmd((char *)command, temp);

        if (rc != 0)
        {
            return ipmi::responseUnspecifiedError();
        }

        pwmValue = strtol(temp,NULL,10);
        responseDuty = pwmValue*100/255;

        return ipmi::responseSuccess(responseDuty);
    }
    else if (duty == 0xff)
    {
        // Return to automatic control
        rc = system("/usr/bin/env controlFan.sh auto-mode");
    }
    else if (duty <= 0x64)
    {
        // Get Current Fan Control Status

        std::ifstream fanModeFile("/run/fan-config");

        if(fanModeFile.good())
        {
            std::string readText;
            getline(fanModeFile, readText);
            currentStatus = std::stoi(readText);
        }
        else
        {
            currentStatus = 0x01;
        }

        if(currentStatus == 0x00)
        {
            // control duty cycle (0%-100%)
            pwmValue = duty*255/100;

            switch (pwmId)
            {
                case 0:
                    return ipmi::responseParmOutOfRange();
                case 1:
                case 2:
                case 3:
                case 4:
                case 5:
                case 6:
                    memset(command,0,sizeof(command));
                    sprintf(command, "echo %d > /sys/class/hwmon/hwmon0/pwm%d", pwmValue, (7-pwmId));
                    break;
                default:
                    return ipmi::responseParmOutOfRange();
            }
            rc = system(command);
        }
        else
        {
            // fan control is Enable can't control fan duty
            return ipmi::responseUnspecifiedError();
        }
    }
    else
    {
        return ipmi::responseParmOutOfRange();
    }

    if (rc != 0)
    {
        return ipmi::responseUnspecifiedError();
    }

    return ipmi::responseSuccess();
}
//===============================================================
/* Config ECC Leaky Bucket Command
NetFun: 0x2E
Cmd : 0x1A
Request:
        Byte 1-3 : Tyan Manufactures ID (FD 19 00)
        Byte 4 : optional, set T1
        Byte 5 : optional, set T2
        Byte 6 : optional, set T3

Response:
        Byte 1 : Completion Code
        Byte 2-4 : Tyan Manufactures ID
        Byte 5 : Return current T1 if request length = 3.
        Byte 6 : Return current T2 if request length = 3.
        Byte 7 : Return current T3 if request length = 3.
*/
ipmi::RspType<std::optional<uint8_t>, // T1 
              std::optional<uint8_t>, // T2
              std::optional<uint8_t>  // T3
              >
    ipmi_tyan_ConfigEccLeakyBucket(std::optional<uint8_t> T1, std::optional<uint8_t> T2, std::optional<uint8_t> T3)
{

    constexpr const char* leakyBucktPath =
        "/xyz/openbmc_project/leakyBucket/HOST_DIMM_ECC";
    constexpr const char* leakyBucktIntf =
        "xyz.openbmc_project.Sensor.Value";
    std::shared_ptr<sdbusplus::asio::connection> busp = getSdBus();
    uint8_t t1;
    uint8_t t2;
    uint8_t t3;
    ipmi::Value result;

    auto service = ipmi::getService(*busp, leakyBucktIntf, leakyBucktPath);
    
    //get t1,t2,t3
    if(!T1) 
    {
        try
        {
            result = ipmi::getDbusProperty(
                    *busp, service, leakyBucktPath, leakyBucktIntf, "T1");    
            t1 = std::get<uint8_t>(result);

            result = ipmi::getDbusProperty(
                *busp, service, leakyBucktPath, leakyBucktIntf, "T2");
            t2 = std::get<uint8_t>(result);

            result = ipmi::getDbusProperty(
                *busp, service, leakyBucktPath, leakyBucktIntf, "T3");
            t3 = std::get<uint8_t>(result);
        
        }
        catch (const std::exception& e)
        {
            return ipmi::responseUnspecifiedError();
        }
        return ipmi::responseSuccess(t1,t2,t3);
    }

    //t1 found 
    try
    {
       ipmi::setDbusProperty(
                *busp, service, leakyBucktPath, leakyBucktIntf, "T1", T1.value());    
    }
    catch (const std::exception& e)
    {
        return ipmi::responseUnspecifiedError();
    }

    if(T2)
    {
        try
        {
            ipmi::setDbusProperty(
                    *busp, service, leakyBucktPath, leakyBucktIntf, "T2", T2.value());
        }
        catch (const std::exception& e)
        {
            return ipmi::responseUnspecifiedError();
        }
    }

    if(T3)
    {
        try
        {
            ipmi::setDbusProperty(
                    *busp, service, leakyBucktPath, leakyBucktIntf, "T3", T3.value());
        }
        catch (const std::exception& e)
        {
            return ipmi::responseUnspecifiedError();
        }
    }
    return ipmi::responseSuccess();
}

//===============================================================
/* Get DIMM BMC leaky bucket ECC Count
NetFun: 0x2E
Cmd : 0x1B
Request:
        Byte 1-3 : Tyan Manufactures ID (FD 19 00)
        Byte 4 : DIMM (Sensor Number for ECC event)

Response:
        Byte 1 : Completion Code
        Byte 2-4 : Tyan Manufactures ID
        Byte 5 : Correctable ECC Count
*/
ipmi::RspType<uint8_t> ipmiGetEccCount(uint8_t sensorNum)
{

    constexpr const char* leakyBucktPath =
        "/xyz/openbmc_project/leakyBucket/HOST_DIMM_ECC";
    constexpr const char* leakyBucktIntf =
        "xyz.openbmc_project.Sensor.Value";
    std::shared_ptr<sdbusplus::asio::connection> busp = getSdBus();
    ipmi::Value result;
    uint8_t count;
    loadEccMap();

    auto ecc = eccMap.find(sensorNum);
    //find ecc in eccMap
    if (ecc == eccMap.end())
    {
        return ipmi::responseInvalidFieldRequest();
    }

    std::string sensorPath = "/xyz/openbmc_project/leakyBucket/HOST_DIMM_ECC/"+ecc->second;

    auto service = ipmi::getService(*busp, leakyBucktIntf, leakyBucktPath);

    //get count
    try
    {
        result = ipmi::getDbusProperty(
                    *busp, service, sensorPath, leakyBucktIntf, "count");
        count = std::get<uint8_t>(result);
    }
    catch (const std::exception& e)
    {
        //ecc bucket is not created, return count as 0;
        return ipmi::responseSuccess(0);
    }
    return ipmi::responseSuccess(count);
}

//======================================================================================================
/* get gpio status Command (for manufacturing) 
NetFun: 0x2E
Cmd : 0x41
Request:
       Byte 1-3 : Tyan Manufactures ID (FD 19 00)
        Byte 4 : gpio number 
Response:
        Byte 1 : Completion Code
        Byte 2-4 : Tyan Manufactures ID
        Byte 5 : gpio direction 
        Byte 6 : gpio data
*/
ipmi::RspType<uint8_t,
              uint8_t>
    ipmi_tyan_getGpio(uint8_t gpioNo)
{
    auto chip = gpiod::chip("gpiochip0");
    auto line = chip.get_line(gpioNo);
    
    if (!line)
    {
        std::cerr << "Error requesting gpio\n";
        return ipmi::responseUnspecifiedError();
    }
    auto dir = line.direction();

    bool resp;
    try
    {
        line.request({"ipmid", gpiod::line_request::DIRECTION_INPUT,0});
        resp = line.get_value();
    }
    catch (std::system_error&)
    {
        std::cerr << "Error reading gpio: " << (unsigned)gpioNo << "\n";
        return ipmi::responseUnspecifiedError();
    }
  
    line.release();
    return ipmi::responseSuccess((uint8_t)dir, (uint8_t)resp);
}

//===============================================================
/* Get Firmware String Command
NetFun: 0x2E
Cmd : 0x10
Request:
    Byte 1-3 : Tyan Manufactures ID (FD 19 00)
Response:
    Byte 1 : Completion Code
    Byte 2-4 : Tyan Manufactures ID
    Byte 5-N : Firmware String
*/
ipmi::RspType<std::vector<uint8_t>> ipmi_getFirmwareString()
{
    std::vector<uint8_t>firmwareString(3,0);
    std::string osReleasePath = "/etc/os-release";
    std::string searchFirmwareString = "BMC_NAME";
    std::string readText;
    std::ifstream readFile(osReleasePath);

    while(getline(readFile, readText)) {
        std::size_t found = readText.find(searchFirmwareString);
        if (found!=std::string::npos){
            std::vector<std::string> readTextSplite;
            boost::split(readTextSplite, readText, boost::is_any_of( "\"" ) );
            firmwareString.assign(readTextSplite[1].length()+1, 0);
            std::copy(readTextSplite[1].begin(), readTextSplite[1].end(), firmwareString.begin());
        }
    }

    readFile.close();

    return ipmi::responseSuccess(firmwareString);
}

//===============================================================
/* Set Manufacture Data Command
NetFun: 0x2E
Cmd : 0x0D
Request:
    Byte 1-3 : Tyan Manufactures ID (FD 19 00)
    Byte 4 : Manufacture Data Parameter
                0h-Product ID, 2 Bytes, LSB
                1h-SKU ID, 1 Byte
                2h-Device GUID, 16 Bytes
                3h-System GUID, 16 Bytes
                4h-Motherboard Serial Number, 16 Bytes
                5h-MAC 1 Address, 6 Bytes
                6h-MAC 2 Address, 6 Bytes
                7h-System Name, 32 Bytes
    Byte 5-N : Data
Response:
    Byte 1 : Completion Code
    Byte 2-4 : Tyan Manufactures ID
*/
ipmi::RspType<> ipmi_setManufactureData(uint8_t selctField, std::vector<uint8_t> writeData)
{
    ipmi::Cc ret;
    uint8_t busId = 0x02;
    uint8_t slaveAddr = 0x50;
    uint8_t readCount = 0x00;
    uint8_t offsetMSB, offsetLSB;

    ret = setManufactureFieledProperty(selctField, writeData.size(), readCount, offsetMSB, offsetLSB);

    if(ret !=  ipmi::ccSuccess)
    {
        return ipmi::response(ret);
    }

    std::vector<uint8_t> writeDataWithOffset;
    writeDataWithOffset.assign(writeData.begin(), writeData.end());
    writeDataWithOffset.insert(writeDataWithOffset.begin(), offsetLSB);
    writeDataWithOffset.insert(writeDataWithOffset.begin(), offsetMSB);

    std::vector<uint8_t> readBuf(0x00);
    std::string i2cBus =
        "/dev/i2c-" + std::to_string(static_cast<uint8_t>(busId));

    ret = ipmi::i2cWriteRead(i2cBus, static_cast<uint8_t>(slaveAddr),
                                      writeDataWithOffset, readBuf);

    if(ret !=  ipmi::ccSuccess)
    {
        return ipmi::response(ret);
    }

    return ipmi::responseSuccess();
}

//===============================================================
/* Get Manufacture Data Command
NetFun: 0x2E
Cmd : 0x0E
Request:
    Byte 1-3 : Tyan Manufactures ID (FD 19 00)
    Byte 4 : Manufacture Data Parameter
                0h-Product ID, 2 Bytes, LSB
                1h-SKU ID, 1 Byte
                2h-Device GUID, 16 Bytes
                3h-System GUID, 16 Bytes
                4h-Motherboard Serial Number, 16 Bytes
                5h-MAC 1 Address, 6 Bytes
                6h-MAC 2 Address, 6 Bytes
                7h-System Name, 32 Bytes
Response:
    Byte 1 : Completion Code
    Byte 2-4 : Tyan Manufactures ID
    Byte 5-N : Data
*/
ipmi::RspType<std::vector<uint8_t>> ipmi_getManufactureData(uint8_t selctField)
{
    ipmi::Cc ret;
    uint8_t busId = 0x02;
    uint8_t slaveAddr = 0x50;
    uint8_t readCount = 0x01;
    uint8_t offsetMSB, offsetLSB;

    ret = setManufactureFieledProperty(selctField, 0x00, readCount, offsetMSB, offsetLSB);

    if(ret !=  ipmi::ccSuccess)
    {
        return ipmi::response(ret);
    }

    std::vector<uint8_t> readBuf(readCount);
    std::vector<uint8_t> writeDataWithOffset;
    writeDataWithOffset.insert(writeDataWithOffset.begin(), offsetLSB);
    writeDataWithOffset.insert(writeDataWithOffset.begin(), offsetMSB);

    std::string i2cBus =
        "/dev/i2c-" + std::to_string(static_cast<uint8_t>(busId));

    ret = ipmi::i2cWriteRead(i2cBus, static_cast<uint8_t>(slaveAddr),
                                      writeDataWithOffset, readBuf);

    return ipmi::responseSuccess(readBuf);
}

//===============================================================
/* Set/Get Ramdom Delay AC Restore Power ON Command
NetFun: 0x30
Cmd : 0x18
Request:
    Byte 1 : Op Code
        [7] : 0-Get 1-Set
        [6:0] :
            00h-Disable Delay
            01h-Enable Delay, Random Delay Time
            02h-Enable Delay, Fixed Delay Time
    Byte 2-3 : Delay Time, LSB first (Second base)
Response:
    Byte 1 : Completion Code
    Byte 2 :  Op Code
    Byte 3-4 : Delay Time, LSB first (Second base)
*/
ipmi::RspType<uint8_t, uint8_t, uint8_t> ipmi_tyan_RamdomDelayACRestorePowerON(uint8_t opCode, uint8_t delayTimeLSB,uint8_t delayTimeMSB)
{
    std::uint8_t opCodeResponse, delayTimeLSBResponse, delayTimeMSBResponse;;

    constexpr auto service = "xyz.openbmc_project.Settings";
    constexpr auto path = "/xyz/openbmc_project/control/power_restore_delay";
    constexpr auto powerRestoreInterface = "xyz.openbmc_project.Control.Power.RestoreDelay";
    constexpr auto alwaysOnPolicy = "PowerRestoreAlwaysOnPolicy";
    constexpr auto delay = "PowerRestoreDelay";

    auto bus = sdbusplus::bus::new_default();

    if(opCode & 0x80)
    {
        //Set
        opCodeResponse = opCode;
        delayTimeLSBResponse = delayTimeLSB;
        delayTimeMSBResponse = delayTimeMSB;
        try
        {
            auto method = bus.new_method_call(service, path, PROPERTY_INTERFACE,"Set");
            method.append(powerRestoreInterface, alwaysOnPolicy, std::variant<std::string>(RestoreDelay::convertAlwaysOnPolicyToString((RestoreDelay::AlwaysOnPolicy)(opCode & 0x7F))));
            bus.call_noreply(method);

            uint32_t delayValue = delayTimeLSB | (delayTimeMSB << 8);

            auto methodDelay = bus.new_method_call(service, path, PROPERTY_INTERFACE, "Set");
            methodDelay.append(powerRestoreInterface, delay, std::variant<uint32_t>(delayValue));
            bus.call_noreply(methodDelay);
        }
        catch (const sdbusplus::exception::SdBusError& e)
        {
            log<level::ERR>("Error in RamdomDelayACRestorePowerON Set",entry("ERROR=%s", e.what()));
            return ipmi::responseParmOutOfRange();
        }
    }
    else
    {
        //Get
        auto method = bus.new_method_call(service, path, PROPERTY_INTERFACE,"Get");
        method.append(powerRestoreInterface, alwaysOnPolicy);

        std::variant<std::string> result;
        try
        {
            auto reply = bus.call(method);
            reply.read(result);
        }
        catch (const sdbusplus::exception::SdBusError& e)
        {
            log<level::ERR>("Error in RamdomDelayACRestorePowerON Get",entry("ERROR=%s", e.what()));
            return ipmi::responseUnspecifiedError();
        }
        auto powerAlwaysOnPolicy = std::get<std::string>(result);

        auto methodDelay = bus.new_method_call(service, path, PROPERTY_INTERFACE, "Get");
        methodDelay.append(powerRestoreInterface, delay);

        std::variant<uint32_t> resultDelay;
        try
        {
            auto reply = bus.call(methodDelay);
            reply.read(resultDelay);
        }
        catch (const sdbusplus::exception::SdBusError& e)
        {
            log<level::ERR>("Error in PowerRestoreDelay Get",entry("ERROR=%s", e.what()));
            return ipmi::responseUnspecifiedError();
        }
        auto powerRestoreDelay = std::get<uint32_t>(resultDelay);

        opCodeResponse = (opCode & 0x80) | (uint8_t)RestoreDelay::convertAlwaysOnPolicyFromString(powerAlwaysOnPolicy);

        uint8_t *delayValue = (uint8_t *)&powerRestoreDelay;
        delayTimeLSBResponse = delayValue[0];
        delayTimeMSBResponse = delayValue[1];
    }

    return ipmi::responseSuccess(opCodeResponse,delayTimeLSBResponse,delayTimeMSBResponse);
}

//===============================================================
/* Set specified service enable or disable status
NetFun: 0x30
Cmd : 0x0D
Request:
    Byte 1 : Set service status
        [7-3] : reserved
        [2] :
            0h-Disable virtual media service
            1h-Enable virtual media service
        [1] :
            0h-Disable KVM service
            1h-Enable KVM service
        [0] :
            0h-Disable web service
            1h-Enable web service
Response:
    Byte 1 : Completion Code
*/
ipmi::RspType<> ipmi_SetService(uint8_t serviceSetting)
{
    constexpr auto SETTIMG_SERVICE = "xyz.openbmc_project.Settings";
    constexpr auto SERVICE_STATUS_PATH = "/xyz/openbmc_project/oem/ServiceStatus";
    constexpr auto SERVICE_STATUS_INTERFACE = "xyz.openbmc_project.OEM.ServiceStatus";

    constexpr auto WEB_SERVICE = "WebService";
    constexpr auto KVM_SERVICE = "KvmService";
    constexpr auto VM_SERVICE = "VmService";
    constexpr auto CRD_SERVICE = "RemoteDebugService";

    int ret;
    auto bus = sdbusplus::bus::new_default();


    //Set cpu remote debug service 
    try
    {
        if(serviceSetting & 0x08)
        {
            ret = setBmcService("yaap.service","StartUnit","EnableUnitFiles");
        }
        else
        {
            ret = setBmcService("yaap.service","StopUnit","DisableUnitFiles");
        }

        if(ret < 0)
        {
            return ipmi::responseUnspecifiedError();
        }
        auto method = bus.new_method_call(SETTIMG_SERVICE, SERVICE_STATUS_PATH, PROPERTY_INTERFACE,"Set");
        method.append(SERVICE_STATUS_INTERFACE, CRD_SERVICE, std::variant<bool>((bool)(serviceSetting & 0x08)));
        bus.call_noreply(method);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        log<level::ERR>("Error in web service set",entry("ERROR=%s", e.what()));
        return ipmi::responseUnspecifiedError();
    }
    

    //Set web service status
    try
    {
        auto method = bus.new_method_call(SETTIMG_SERVICE, SERVICE_STATUS_PATH, PROPERTY_INTERFACE,"Set");
        method.append(SERVICE_STATUS_INTERFACE, WEB_SERVICE, std::variant<bool>((bool)(serviceSetting & 0x01)));
        bus.call_noreply(method);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        log<level::ERR>("Error in web service set",entry("ERROR=%s", e.what()));
        return ipmi::responseUnspecifiedError();
    }

    //Set KVM service status
    try
    {
        if(serviceSetting & 0x02)
        {
            ret = setBmcService("start-ipkvm.service","StartUnit","EnableUnitFiles");
        }
        else
        {
            ret = setBmcService("start-ipkvm.service","StopUnit","DisableUnitFiles");
        }

        if(ret < 0)
        {
            return ipmi::responseUnspecifiedError();
        }

        auto method = bus.new_method_call(SETTIMG_SERVICE, SERVICE_STATUS_PATH, PROPERTY_INTERFACE,"Set");
        method.append(SERVICE_STATUS_INTERFACE, KVM_SERVICE, std::variant<bool>((bool)(serviceSetting & 0x02)));
        bus.call_noreply(method);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        log<level::ERR>("Error in KVM service set",entry("ERROR=%s", e.what()));
        return ipmi::responseUnspecifiedError();
    }

    //Set virtual media service status
    try
    {
        auto method = bus.new_method_call(SETTIMG_SERVICE, SERVICE_STATUS_PATH, PROPERTY_INTERFACE,"Set");
        method.append(SERVICE_STATUS_INTERFACE, VM_SERVICE, std::variant<bool>((bool)(serviceSetting & 0x04)));
        bus.call_noreply(method);

        ret = setBmcService("bmcweb.service","RestartUnit","Null");

        if(ret < 0)
        {
            return ipmi::responseUnspecifiedError();
        }

    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        log<level::ERR>("Error in web service set",entry("ERROR=%s", e.what()));
        return ipmi::responseUnspecifiedError();
    }

    return ipmi::responseSuccess();
}

//===============================================================
/* Get specified service enable or disable status
NetFun: 0x30
Cmd : 0x0E
Request:

Response:
    Byte 1 : Completion Code
    Byte 2 : Set service status
        [7-3] : reserved
        [2] :
            0h-Vitrual media service is disable
            1h-Vitrual media service is enable
        [1] :
            0h-KVM service is disable
            1h-KVM service is enable
        [0] :
            0h-Web service is disable
            1h-Web service is enable
*/
ipmi::RspType<uint8_t> ipmi_GetService()
{
    uint8_t serviceResponse = 0;
    try
    {
        serviceResponse = (((uint8_t)(getOemServiceStatus("WebService"))) << 0) +
                          (((uint8_t)(getOemServiceStatus("KvmService"))) << 1) +
                          (((uint8_t)(getOemServiceStatus("VmService"))) << 2) +
                          (((uint8_t)(getOemServiceStatus("RemoteDebugService"))) << 3);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        log<level::ERR>("Error in service set",entry("ERROR=%s", e.what()));
        return ipmi::responseUnspecifiedError();
    }

    return ipmi::responseSuccess(serviceResponse);
}

//===============================================================
/* Get the latest 20 BIOS post codes
NetFun: 0x30
Cmd : 0x10
Request:

Response:
    Byte 1 : Completion Code
    Byte 2-21 : latest 20 BIOS post codes
*/
ipmi::RspType<std::vector<uint8_t>> ipmi_GetPostCode()
{
    static const size_t MAX_POST_SIZE = 20;

    constexpr auto postCodeInterface = "xyz.openbmc_project.State.Boot.PostCode";
    constexpr auto postCodeObjPath   = "/xyz/openbmc_project/State/Boot/PostCode0";
    constexpr auto postCodeService   = "xyz.openbmc_project.State.Boot.PostCode0";

    std::shared_ptr<sdbusplus::asio::connection> bus = getSdBus();

    std::vector<std::tuple<uint64_t, std::vector<uint8_t>>> postCodeBuffer;
    std::vector<uint8_t> dataInfo;
    uint16_t currentBootCycle = 0;
    int postCodeIndex = 0;

    try
    {
        /* Get CurrentBootCycleIndex property */
        auto value = ipmi::getDbusProperty(*bus, postCodeService, postCodeObjPath,
                               postCodeInterface, "CurrentBootCycleCount");
        currentBootCycle = std::get<uint16_t>(value);

        /* Set the argument for method call */
        auto msg = bus->new_method_call(postCodeService, postCodeObjPath,
                                         postCodeInterface, "GetPostCodes");
        msg.append(currentBootCycle);

        /* Get the post code of CurrentBootCycleIndex */
        auto reply = bus->call(msg);
        reply.read(postCodeBuffer);

        int postCodeBufferSize = postCodeBuffer.size();

        // Set command return length to return the last 20 post code.
        if (postCodeBufferSize > MAX_POST_SIZE)
        {
            postCodeIndex = postCodeBufferSize - MAX_POST_SIZE;
        }
        else
        {
            postCodeIndex = 0;
        }

        /* Get post code data */
        for (int i = 0; ((i < MAX_POST_SIZE) && (postCodeIndex < postCodeBufferSize)); i++, postCodeIndex++)
        {
            dataInfo.push_back(std::get<0>(postCodeBuffer[postCodeIndex]));
        }
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        sd_journal_print(LOG_ERR,"IPMI GetPostCode Failed in call method, %s\n",e.what());
        return ipmi::responseUnspecifiedError();
    }
    return ipmi::responseSuccess(dataInfo);
}

//===============================================================
/* Set SOL pattern function
NetFun: 0x30
Cmd : 0xB2
Request:
    Byte 1 : Sensor Number for pattern sensor
    Byte 2-N : pattern data string
Response:
    Byte 1 : Completion Code
*/
ipmi::RspType<> ipmiSetSolPattern(uint8_t patternNum, std::vector<uint8_t> patternData)
{
    /* Pattern Number */
    if((patternNum < 1) || (patternNum > 4))
    {
        sd_journal_print(LOG_CRIT, "[%s] invalid pattern number %d\n",
                         __FUNCTION__, patternNum);
        return ipmi::responseParmOutOfRange();;
    }

    /* Set pattern to dbus */
    std::string solPatternObjPath = SOL_PATTERN_OBJECTPATH + std::to_string((patternNum));
    std::string s(patternData.begin(), patternData.end());
    std::shared_ptr<sdbusplus::asio::connection> dbus = getSdBus();
    try
    {
        ipmi::setDbusProperty(*dbus, SOL_PATTERN_SERVICE, solPatternObjPath,
                               VALUE_INTERFACE, "Pattern", s);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        return ipmi::responseUnspecifiedError();
    }

    return ipmi::responseSuccess();
}

//===============================================================
/* Get SOL pattern function
NetFun: 0x30
Cmd : 0xB3
Request:
    Byte 1 : Sensor Number for pattern sensor
Response:
    Byte 1 : Completion Code
    Byte 2-N : pattern data string
*/
ipmi::RspType<std::vector<uint8_t>> ipmiGetSolPattern(uint8_t patternNum)
{

    /* Pattern Number */
    if((patternNum < 1) || (patternNum > 4))
    {
        sd_journal_print(LOG_CRIT, "[%s] invalid pattern number %d\n",
                         __FUNCTION__, patternNum);
        return ipmi::responseParmOutOfRange();;
    }

    /* Get pattern to dbus */
    std::string solPatternObjPath = SOL_PATTERN_OBJECTPATH + std::to_string((patternNum));
    std::string patternData;

    std::shared_ptr<sdbusplus::asio::connection> dbus = getSdBus();
    try
    {
        auto value = ipmi::getDbusProperty(*dbus, SOL_PATTERN_SERVICE, solPatternObjPath,
                               VALUE_INTERFACE, "Pattern");
        patternData = std::get<std::string>(value);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        return ipmi::responseUnspecifiedError();
    }

    std::vector<uint8_t> v(patternData.begin(), patternData.end());
    return ipmi::responseSuccess(v);
}

//===============================================================
/* Clear CMOS command
NetFun: 0x30
Cmd : 0x3A
Request:

Response:
    Byte 1 : Completion Code
*/
ipmi::RspType<> ipmiClearCmos()
{
    int ret = logSelEvent("Clear CMOS SEL Entry","/xyz/openbmc_project/sensors/fru_state/CLEAR_CMOS",0x02,0x01,0xFF);

    if(ret < 0)
    {
        return ipmi::responseUnspecifiedError();
    }

    std::thread clearCmosThread(cleadrCmos);
    clearCmosThread.detach();

    return ipmi::responseSuccess();
}

//===============================================================
/* BMC flash device command
NetFun: 0x30
Cmd : 0x0F
Request:
    Byte 1 : Op Code
        [7:0] :
            00h-Get command
            01h-Set command
    Byte 2 : Switch using BMC flash immediately
        [7:0] :
            00h-Reserved
            01h-Enable switch action
Response:
    Byte 1 : Completion Code
    Byte 2 : Op Code
    Byte 3 : Current using BMC flash device
*/

ipmi::RspType<uint8_t,uint8_t> ipmiBmcFlashDevice(uint8_t opCode,uint8_t switchFlash)
{
    static const std::string bootSourceFilePath = "/run/boot-source";

    uint8_t bootSource = 0;
    uint32_t registerBuffer = 0;
    uint32_t bootSourceFlag = 0x02;
    uint32_t switchMask1 = 0x49504D49; // ASCII IPMI
    uint32_t switchMask2 = 0xD1000000; // checksum
    int ret;

    std::ifstream ifs(bootSourceFilePath);
    std::string line;
    getline(ifs, line);
    bootSource = std::stoi(line);

    if(opCode)
    {
        switch(switchFlash)
        {
            case 1:
                ret = setBmcService("system-watchdog.service","StopUnit","Null");
                if(ret < 0)
                {
                    return ipmi::responseUnspecifiedError();
                }
                // ASCII IPMI to SRAM
                system("devmem 0x1e723010 32 0x49504D49");
                // Checksum to SRAM
                system("devmem 0x1e723014 32 0xD1000000");

                system("devmem 0x1E785024 32 0x00989680");
                system("devmem 0x1E785028 32 0x00004755");
                system("devmem 0x1E78502C 32 0x00000093");
                break;
            default:
                break;
        }
    }

    return ipmi::responseSuccess(opCode,bootSource);
}

//===============================================================
/* Get CPU power command
NetFun: 0x30
Cmd : 0x11
Request:

Request:
    Byte 1 : Completion Code
    Byte 2-5 : Current CPU power consumption
*/

ipmi::RspType<uint32_t> ipmiGetCpuPower()
{
    uint32_t power = getCpuPowerConsumption(0);

    return ipmi::responseSuccess(power);
}

//===============================================================
/* Get CPU power limit command
NetFun: 0x30
Cmd : 0x12
Request:

Request:
    Byte 1 : Completion Code
    Byte 2-5 : Current CPU power limitation
*/

ipmi::RspType<uint32_t> ipmiGetCpuPowerLimit()
{

    if(!isPowerOn() || !hasBiosPost())
    {
        return ipmi::responseCommandNotAvailable();
    }

    uint32_t power = getCpuPowerLimit(0);

    return ipmi::responseSuccess(power);
}

//===============================================================
/* Set CPU power limit command
NetFun: 0x30
Cmd : 0x13
Request:
    Byte 1-4 : CPU power limitation
Request:
    Byte 1 : Completion Code
*/

ipmi::RspType<> ipmiSetCpuPowerLimit(uint32_t limit)
{

    if(!isPowerOn() || !hasBiosPost())
    {
        return ipmi::responseCommandNotAvailable();
    }

    int ret = setCpuPowerLimit(0,limit);

    if(ret < 0)
    {
        return ipmi::responseUnspecifiedError();
    }

    return ipmi::responseSuccess();
}

//===============================================================
/* Set default CPU power limit command
NetFun: 0x30
Cmd : 0x14
Request:

Request:
    Byte 1 : Completion Code
*/

ipmi::RspType<> ipmiSetDefaultCpuPowerLimit()
{

    if(!isPowerOn() || !hasBiosPost())
    {
        return ipmi::responseCommandNotAvailable();
    }

    int ret = setDefaultCpuPowerLimit(0);

    if(ret < 0)
    {
        return ipmi::responseUnspecifiedError();
    }

    return ipmi::responseSuccess();
}

//===============================================================
/* Set System power limit command
NetFun: 0x30
Cmd : 0x20
Request:
    Byte 1-4 : System power limitation
Request:
    Byte 1 : Completion Code
*/


constexpr auto sysPowerCapInt = "xyz.openbmc_project.Control.Power.Cap";
constexpr auto sysPowerCapObj = "/xyz/openbmc_project/control/host0/power_cap";
constexpr auto settingService = "xyz.openbmc_project.Settings";

ipmi::RspType<> ipmiSetSysPowerLimit(uint32_t limit)
{

    std::shared_ptr<sdbusplus::asio::connection> bus = getSdBus();

    try
    {
        ipmi::setDbusProperty(*bus, settingService, sysPowerCapObj,
                               sysPowerCapInt, "PowerCap", limit);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        return ipmi::responseUnspecifiedError();
    }

    return ipmi::responseSuccess();
}

//===============================================================
/* Get System power limit command
NetFun: 0x30
Cmd : 0x21
Request:

Request:
    Byte 1 : Completion Code
    Byte 2-5 : Current System power limitation
*/

ipmi::RspType<uint32_t> ipmiGetSysPowerLimit()
{
	std::shared_ptr<sdbusplus::asio::connection> bus = getSdBus();
	uint32_t powerLimit;
    try
    {
		auto value = ipmi::getDbusProperty(*bus, settingService, sysPowerCapObj,
                               sysPowerCapInt, "PowerCap");

    	powerLimit = std::get<uint32_t>(value);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        return ipmi::responseUnspecifiedError();
    }
	 return ipmi::responseSuccess(powerLimit);
}

//===============================================================
/* Enable system power limit
NetFun: 0x30
Cmd : 0x22
Request:
	Byte 1:  optional, bit[0] = 1b, enable system power limit

Request:
    Byte 1 : Completion Code
    Byte 2 : return current setting 
    
*/

ipmi::RspType<bool> ipmiEnableSysPowerLimit(std::optional<uint8_t> Enalbe)
{
    std::shared_ptr<sdbusplus::asio::connection> bus = getSdBus();
    bool enable;

    if(!Enalbe)
    {
        uint8_t value;
        try
        {
            auto v = ipmi::getDbusProperty(*bus, settingService, sysPowerCapObj,
                                   sysPowerCapInt, "PowerCapEnable");
        
            enable = std::get<bool>(v);
        }
        catch (const sdbusplus::exception::SdBusError& e)
        {
            return ipmi::responseUnspecifiedError();
        }         
    }else{

	    enable = Enalbe.value()? true:false;
        try
        {
            ipmi::setDbusProperty(*bus, settingService, sysPowerCapObj,
                               sysPowerCapInt, "PowerCapEnable", enable);
        }
        catch (const sdbusplus::exception::SdBusError& e)
        {
            return ipmi::responseUnspecifiedError();
        }
    }
    return ipmi::responseSuccess(enable);
}


//===============================================================
/* Disable hot swap control command
NetFun: 0x30
Cmd : 0x15
Request:

Request:
    Byte 1 : Completion Code
*/

ipmi::RspType<> ipmiDisableHsc()
{

    int ret = setBmcService("setting-hsc-register@DisableIPMI.service","StartUnit","Null");

    if(ret < 0)
    {
        return ipmi::responseUnspecifiedError();
    }

    return ipmi::responseSuccess();
}

//===============================================================
/* Set BIOS recovery related setting
NetFun: 0x30
Cmd : 0x16
Request:
    Byte 1 : BIOS recovery status
        [7:0] :
            00h-Normal mode
            01h-BIOS recovery mode
Response:
    Byte 1 : Completion Code
*/

ipmi::RspType<> ipmiBiosRecovery(uint8_t biosStatus)
{
    if(biosStatus == 0x00)
    {
        return ipmi::responseSuccess();
    }
    else if(biosStatus == 0x01)
    {
        std::thread biosRecoveryThread(biosRecoveryMode);
        biosRecoveryThread.detach();
    }
    else
    {
        return ipmi::responseParmOutOfRange();
    }

    return ipmi::responseSuccess();
}

//===============================================================
/* Setting BIOS flash device command
NetFun: 0x30
Cmd : 0x17
Request:
    Byte 1 : Op Code
        [7:0] :
            00h-Get command
            01h-Set command
    Byte 2 : Switch using BIOS flash immediately
        [7:0] :
            00h-Switch to main BIOS flash device
            01h-Switch to secondary BIOS flash device
Response:
    Byte 1 : Completion Code
    Byte 2 :  Op Code
    Byte 3 :  Current using BIOS flash device
*/

ipmi::RspType<uint8_t,uint8_t> ipmiBiosFlashDevice(uint8_t opCode,uint8_t switchFlash)
{
    std::shared_ptr<sdbusplus::asio::connection> bus = getSdBus();
    uint8_t currentBIOSFlash = 0;

    if(opCode)
    {
        switch(switchFlash)
        {
            case 0:
            case 1:
                break;
            default:
                return ipmi::responseParmOutOfRange();
                break;
        }

        auto method = bus->new_method_call(DBIOS_SERVICE, DBIOS_PATH, DBIOS_INTERFACE, "SetCurrentBootBios");
        method.append((int)switchFlash,true);
        try
        {
            auto reply=bus->call(method);
        }
        catch (const sdbusplus::exception::SdBusError& e)
        {
            return ipmi::responseUnspecifiedError();
        }
        currentBIOSFlash = (uint8_t)switchFlash;
    }
    else
    {
        auto method = bus->new_method_call(DBIOS_SERVICE, DBIOS_PATH, PROPERTY_INTERFACE, "Get");
        method.append(DBIOS_INTERFACE, "BiosSelect");
        std::variant<int> result;
        try
        {
            auto reply = bus->call(method);
            reply.read(result);
        }
        catch (const sdbusplus::exception::SdBusError& e)
        {
            return ipmi::responseUnspecifiedError();
        }

        currentBIOSFlash = (uint8_t)std::get<int>(result);
    }

    return ipmi::responseSuccess(opCode,currentBIOSFlash);
}

//===============================================================
/* Get amber LED status command
NetFun: 0x30
Cmd : 0x19
Request:

Response:
    Byte 1 : Completion Code
    Byte 2 : Amber LED status
        [7:0] :
            00h-Amber LED disable
            01h-Amber LED enable
*/

ipmi::RspType<uint8_t> ipmiGetAmberStatus()
{
    ipmi::Value result;
    std::string sysLedState;
    uint8_t state = 0x00;

    std::ifstream inputSysLedFile(SYS_LED_PATH, std::ifstream::in);
    getline(inputSysLedFile , sysLedState);

    if(sysLedState != "0")
    {
        state = 0x01;
    }

    return ipmi::responseSuccess(state);
}

//===============================================================
/* Get DIMM Host Ecc Count
NetFun: 0x30
Cmd : 0x1B
Request:
        Byte 1 : DIMM Channel
        [7:0] :
            00h-DIMM Channel A ECC count
            01h-DIMM Channel B ECC count
            02h-DIMM Channel C ECC count
            03h-DIMM Channel D ECC count
            04h-DIMM Channel E ECC count
            05h-DIMM Channel F ECC count
            06h-DIMM Channel G ECC count
            07h-DIMM Channel H ECC count
Response:
        Byte 1 : Completion Code
        Byte 2 : Correctable ECC Count
*/
ipmi::RspType<uint8_t> ipmiGetHostEccCount(uint8_t channelNum)
{
    uint8_t count;

    if(!isPowerOn() || !hasBiosPost())
    {
        return ipmi::responseCommandNotAvailable();
    }

    loadDefaultDimmEccCounter();

    if (channelNum >= sizeof(dimmArray)/sizeof(dimmArray[0]))
    {
        return ipmi::responseInvalidFieldRequest();
    }

    uint64_t msrValue;
    uint32_t msrRegister;
    std::stringstream stringstreamMsrRegister;
    stringstreamMsrRegister << std::hex << dimmArray[channelNum][3];
    stringstreamMsrRegister >> msrRegister;

    msrValue = getMsrValue(std::stoi(dimmArray[channelNum][2]),msrRegister);
    msrValue = (msrValue & 0x00000fff00000000) >> 32;

    if(msrValue == 0)
    {
        return ipmi::responseSuccess(0);
    }
    count = defaultDimmEccCounter - (0xfff - msrValue);

    return ipmi::responseSuccess(count);
}

//===============================================================
/* Get system firmware revision.
   BMC would response firmware revision directly.
   BIOS would response firmware revision with ASCII format.
   CPLD would response firmware revision directly.
   PSU would response firmware revision directly.
NetFun: 0x30
Cmd : 0x1C
Request:
        Byte 1 : Select read firmware
        [7:0] :
            00h-Active BMC
            01h-Backup BMC
            02h-Active BIOS
            03h-Backup BIOS
            04h-CPLD
            05h-PSU1
            06h-PSU2
Response:
        Byte 1 : Completion Code
        Byte 2-N : Firmware revision
        Active/Backup BMC :
            Byte 2 : Major firmware revision
            Byte 3 : Minor firmware revision
        Active/Backup BIOS :
            Byte 2-N : Firmware revision with with ASCII format
        CPLD :
            Byte 2 : Major firmware revision
            Byte 3 : Minor firmware revision
            Byte 4 : Maintain firmware Version
        PSU1/PSU2 :
            Byte 2 : Major firmware revision
            Byte 3 : Minor firmware revision
            Byte 4 : Maintain firmware Version
*/
ipmi::RspType<std::vector<uint8_t>> ipmiGetSysFirmwareVersion(uint8_t selectFirmware)
{
    std::vector<uint8_t> firmwareVersion;

    //It's not allow to read BIOS version when BIOS still posting.
    if(selectFirmware == ACTIVE_BIOS || selectFirmware == BACKUP_BIOS)
    {
        if(isPowerOn() && !hasBiosPost())
        {
            return ipmi::responseCommandNotAvailable();
        }
    }

    switch(selectFirmware)
    {
        case ACTIVE_BMC:
            firmwareVersion = readActiveBmcVersion();
            break;
        case BACKUP_BMC:
            firmwareVersion = readBackupBmcVersion();
            break;
        case ACTIVE_BIOS:
            firmwareVersion = readBiosVersion(0);
            break;
        case BACKUP_BIOS:
            firmwareVersion = readBiosVersion(1);
            break;
        case CPLD:
            firmwareVersion = getCpldVersion();
            break;
        case PSU1:
            firmwareVersion = getPsuVersion(1);
            break;
        case PSU2:
            firmwareVersion = getPsuVersion(2);
            break;
        default:
            return ipmi::responseInvalidFieldRequest();
    }

    if(isFailedToReadFwVersion(firmwareVersion))
    {
        return ipmi::responseCommandNotAvailable();
    }

    return ipmi::responseSuccess(firmwareVersion);
}

//=======================================================================
/* Get/Set AMD MBIST status
NetFun: 0x30
Cmd : 0x23
Request:
    Byte 1 : Status (optional: To get AMD MBIST status directly)
        00h : Reserved
        01h : Disable AMD MBIST feature
        02h : Enable AMD MBIST "Simple Write-Read Static Refresh test" feature (This test will take ~20 seconds per memory rank)
        03h : Enable AMD MBIST "Write/Read/Write data test" feature (This test will take ~100 seconds per memory rank)
        04h : Enable AMD MBIST "Aggressor Row stress test" feature (This test will take ~100 seconds per memory rank)
        05h : Enable AMD MBIST "Closed page test" feature (This test will take ~22 minutes per memory rank)
        06h : Enable AMD MBIST "Closed page stress test" feature (This test will take ~30 minutes per memory rank)
Response:
    Byte 1 : Completion Code
    Byte 2 : AMD MBIST status

*/
ipmi::RspType<uint8_t> ipmiConfigAmdMbist(std::optional<uint8_t> configAmdMbist)
{
    uint8_t amdMbistResponse = 0;

    constexpr auto service = "xyz.openbmc_project.Settings";
    constexpr auto path = "/xyz/openbmc_project/oem/HostStatus";
    constexpr auto hostStatusInterface = "xyz.openbmc_project.OEM.HostStatus";
    constexpr auto amdMbistStatus = "AmdMbistStatus";

    auto bus = sdbusplus::bus::new_default();

    if(configAmdMbist)
    {
        switch(configAmdMbist.value())
        {
            case 1:
            case 2:
            case 3:
            case 4:
            case 5:
            case 6:
                break;
            default :
                return ipmi::responseInvalidFieldRequest();
        }

        auto method = bus.new_method_call(service, path, PROPERTY_INTERFACE,"Set");
        method.append(hostStatusInterface, amdMbistStatus, std::variant<uint32_t>(configAmdMbist.value()));
        bus.call_noreply(method);
        amdMbistResponse = configAmdMbist.value();
    }
    else
    {
        auto method = bus.new_method_call(service, path, PROPERTY_INTERFACE, "Get");
        method.append(hostStatusInterface, amdMbistStatus);

        std::variant<uint32_t> result;
        try
        {
            auto reply = bus.call(method);
            reply.read(result);
        }
        catch (const sdbusplus::exception::SdBusError& e)
        {
            log<level::ERR>("Error in ConfigAmdMbist Get",entry("ERROR=%s", e.what()));
            return ipmi::responseUnspecifiedError();
        }
        amdMbistResponse = (uint8_t)(std::get<uint32_t>(result));
    }

    return ipmi::responseSuccess(amdMbistResponse);
}
//===============================================================
/* Set SEL time
 * Override the general IPMI command
NetFun: 0x0A
Cmd : 0x49
Request:
    Byte 1-4 : Set sel time
Response:
    Byte 1 : Completion Code
*/
ipmi::RspType<> ipmi_setSELTime(uint32_t selTime)
{
    static constexpr uint8_t timeDiffAllowedSecond = 3;

    try
    {
        time_t bmcTimeSeconds = 0;
        time_t settingTimeSeconds = selTime;

        if(!getBmcTime(bmcTimeSeconds))
        {
            std::cerr <<"Failed to get BMC Time" << std::endl;
            return ipmi::responseUnspecifiedError();
        }

        if (std::abs(settingTimeSeconds - bmcTimeSeconds) > timeDiffAllowedSecond)
        {
            timespec setTime;
            setTime.tv_sec = selTime;
            if(!setRtcTime(selTime))
            {
                std::cerr <<"Failed to set RTC Time" << std::endl;
                return ipmi::responseUnspecifiedError();
            }

            if(clock_settime(CLOCK_REALTIME, &setTime) < 0)
            {
                return ipmi::responseUnspecifiedError();
            }
        }
    }
    catch (const std::exception& e)
    {
        return ipmi::responseUnspecifiedError();
    }
    return ipmi::responseSuccess();
}

//===============================================================
/* Get Device GUID Command
 * Override the general IPMI command
NetFun: 0x0A
Cmd : 0x08
Request:

Response:
    Byte 1 : Completion Code
    Byte 2-17 : GUID bytes 1 through 16.
*/
ipmi::RspType<std::vector<uint8_t>> ipmiAppGetDeviceGuid()
{
    ipmi::Cc ret;
    uint8_t busId = 0x02;
    uint8_t slaveAddr = 0x50;
    uint8_t readCount = 0x01;
    uint8_t offsetMSB, offsetLSB;
    uint8_t selctField = 0x02; // 2h-Device GUID, 16 Bytes

    ret = setManufactureFieledProperty(selctField, 0x00, readCount, offsetMSB, offsetLSB);

    if(ret !=  ipmi::ccSuccess)
    {
        return ipmi::response(ret);
    }

    std::vector<uint8_t> readBuf(readCount);
    std::vector<uint8_t> writeDataWithOffset;
    writeDataWithOffset.insert(writeDataWithOffset.begin(), offsetLSB);
    writeDataWithOffset.insert(writeDataWithOffset.begin(), offsetMSB);

    std::string i2cBus =
        "/dev/i2c-" + std::to_string(static_cast<uint8_t>(busId));

    ret = ipmi::i2cWriteRead(i2cBus, static_cast<uint8_t>(slaveAddr),
                                      writeDataWithOffset, readBuf);

    return ipmi::responseSuccess(readBuf);
}

//===============================================================
/* Get System GUID Command
 * Override the general IPMI command
NetFun: 0x0A
Cmd : 0x37
Request:

Response:
    Byte 1 : Completion Code
    Byte 2-17 : GUID bytes 1 through 16.
*/
ipmi::RspType<std::vector<uint8_t>> ipmiAppGetSystemGuid()
{
    ipmi::Cc ret;
    uint8_t busId = 0x02;
    uint8_t slaveAddr = 0x50;
    uint8_t readCount = 0x01;
    uint8_t offsetMSB, offsetLSB;
    uint8_t selctField = 0x03; // 3h-System GUID, 16 Bytes

    ret = setManufactureFieledProperty(selctField, 0x00, readCount, offsetMSB, offsetLSB);

    if(ret !=  ipmi::ccSuccess)
    {
        return ipmi::response(ret);
    }

    std::vector<uint8_t> readBuf(readCount);
    std::vector<uint8_t> writeDataWithOffset;
    writeDataWithOffset.insert(writeDataWithOffset.begin(), offsetLSB);
    writeDataWithOffset.insert(writeDataWithOffset.begin(), offsetMSB);

    std::string i2cBus =
        "/dev/i2c-" + std::to_string(static_cast<uint8_t>(busId));

    ret = ipmi::i2cWriteRead(i2cBus, static_cast<uint8_t>(slaveAddr),
                                      writeDataWithOffset, readBuf);

    return ipmi::responseSuccess(readBuf);
}

void register_netfn_mct_oem()
{
    ipmi::registerOemHandler(ipmi::prioMax, IANA_TYAN, IPMI_CMD_FanPwmDuty, ipmi::Privilege::Admin, ipmi_tyan_FanPwmDuty);
    ipmi::registerOemHandler(ipmi::prioMax, IANA_TYAN, IPMI_CMD_ManufactureMode, ipmi::Privilege::Admin, ipmi_tyan_ManufactureMode);
    ipmi::registerOemHandler(ipmi::prioMax, IANA_TYAN, IPMI_CMD_ConfigEccLeakyBucket, ipmi::Privilege::Admin, ipmi_tyan_ConfigEccLeakyBucket);
    ipmi::registerOemHandler(ipmi::prioMax, IANA_TYAN, IPMI_CMD_GetEccCount, ipmi::Privilege::Admin, ipmiGetEccCount);
    ipmi::registerOemHandler(ipmi::prioMax, IANA_TYAN, IPMI_CMD_gpioStatus, ipmi::Privilege::Admin, ipmi_tyan_getGpio);
    ipmi::registerOemHandler(ipmi::prioMax, IANA_TYAN, IPMI_CMD_GetFirmwareString, ipmi::Privilege::Admin, ipmi_getFirmwareString);
    ipmi::registerOemHandler(ipmi::prioMax, IANA_TYAN, IPMI_CMD_SetManufactureData, ipmi::Privilege::Admin,ipmi_setManufactureData);
    ipmi::registerOemHandler(ipmi::prioMax, IANA_TYAN, IPMI_CMD_GetManufactureData, ipmi::Privilege::Admin,ipmi_getManufactureData);
    ipmi::registerHandler(ipmi::prioMax, NETFUN_TWITTER_OEM, IPMI_CMD_RamdomDelayACRestorePowerON, ipmi::Privilege::Admin, ipmi_tyan_RamdomDelayACRestorePowerON);
    ipmi::registerHandler(ipmi::prioMax, NETFUN_TWITTER_OEM, IPMI_CMD_SetService, ipmi::Privilege::Admin, ipmi_SetService);
    ipmi::registerHandler(ipmi::prioMax, NETFUN_TWITTER_OEM, IPMI_CMD_GetService, ipmi::Privilege::Admin, ipmi_GetService);
    ipmi::registerHandler(ipmi::prioMax, NETFUN_TWITTER_OEM, IPMI_CMD_GetPostCode, ipmi::Privilege::Admin, ipmi_GetPostCode);
    ipmi::registerHandler(ipmi::prioMax, NETFUN_TWITTER_OEM, IPMI_CMD_SET_SOL_PATTERN, ipmi::Privilege::Admin, ipmiSetSolPattern);
    ipmi::registerHandler(ipmi::prioMax, NETFUN_TWITTER_OEM, IPMI_CMD_GET_SOL_PATTERN, ipmi::Privilege::Admin, ipmiGetSolPattern);
    ipmi::registerHandler(ipmi::prioMax, NETFUN_TWITTER_OEM, IPMI_CMD_ClearCmos, ipmi::Privilege::Admin, ipmiClearCmos);
    ipmi::registerHandler(ipmi::prioMax, NETFUN_TWITTER_OEM, IPMI_CMD_BmcFlashDevice, ipmi::Privilege::Admin, ipmiBmcFlashDevice);
    ipmi::registerHandler(ipmi::prioMax, NETFUN_TWITTER_OEM, IPMI_CMD_GetCpuPower, ipmi::Privilege::Admin, ipmiGetCpuPower);
    ipmi::registerHandler(ipmi::prioMax, NETFUN_TWITTER_OEM, IPMI_CMD_GetCpuPowerLimit, ipmi::Privilege::Admin, ipmiGetCpuPowerLimit);
    ipmi::registerHandler(ipmi::prioMax, NETFUN_TWITTER_OEM, IPMI_CMD_SetCpuPowerLimit, ipmi::Privilege::Admin, ipmiSetCpuPowerLimit);
    ipmi::registerHandler(ipmi::prioMax, NETFUN_TWITTER_OEM, IPMI_CMD_SetDefaultCpuPowerLimit, ipmi::Privilege::Admin, ipmiSetDefaultCpuPowerLimit);
    ipmi::registerHandler(ipmi::prioMax, NETFUN_TWITTER_OEM, IPMI_CMD_SetSysPowerLimit, ipmi::Privilege::Admin, ipmiSetSysPowerLimit);
    ipmi::registerHandler(ipmi::prioMax, NETFUN_TWITTER_OEM, IPMI_CMD_GetSysPowerLimit, ipmi::Privilege::Admin, ipmiGetSysPowerLimit);
    ipmi::registerHandler(ipmi::prioMax, NETFUN_TWITTER_OEM, IPMI_CMD_EnableCpuPowerLimit, ipmi::Privilege::Admin, ipmiEnableSysPowerLimit);
    ipmi::registerHandler(ipmi::prioMax, NETFUN_TWITTER_OEM, IPMI_CMD_DisableHsc, ipmi::Privilege::Admin, ipmiDisableHsc);
    ipmi::registerHandler(ipmi::prioMax, NETFUN_TWITTER_OEM, IPMI_CMD_BiosRecovery, ipmi::Privilege::Admin, ipmiBiosRecovery);
    ipmi::registerHandler(ipmi::prioMax, NETFUN_TWITTER_OEM, IPMI_CMD_BiosFlashDevice, ipmi::Privilege::Admin, ipmiBiosFlashDevice);
    ipmi::registerHandler(ipmi::prioMax, NETFUN_TWITTER_OEM, IPMI_CMD_GetAmberLedStatus, ipmi::Privilege::Admin, ipmiGetAmberStatus);
    ipmi::registerHandler(ipmi::prioMax, NETFUN_TWITTER_OEM, IPMI_CMD_GetHostEccCount, ipmi::Privilege::Admin, ipmiGetHostEccCount);
    ipmi::registerHandler(ipmi::prioMax, NETFUN_TWITTER_OEM, IPMI_CMD_GetSysFirmwareVersion, ipmi::Privilege::Admin, ipmiGetSysFirmwareVersion);
    ipmi::registerHandler(ipmi::prioMax, NETFUN_TWITTER_OEM, IPMI_CMD_ConfigAmdMbist, ipmi::Privilege::Admin, ipmiConfigAmdMbist);
    ipmi::registerHandler(ipmi::prioMax, ipmi::netFnStorage, ipmi::storage::cmdSetSelTime, ipmi::Privilege::Admin,ipmi_setSELTime);
    ipmi::registerHandler(ipmi::prioMax, ipmi::netFnApp, ipmi::app::cmdGetDeviceGuid, ipmi::Privilege::Admin, ipmiAppGetDeviceGuid);
    ipmi::registerHandler(ipmi::prioMax, ipmi::netFnApp, ipmi::app::cmdGetSystemGuid, ipmi::Privilege::Admin, ipmiAppGetSystemGuid);
}
}
