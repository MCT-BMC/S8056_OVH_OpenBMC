#include "oem-cmd.hpp"
#include <iostream>
#include <fstream>
#include <variant>
#include <sstream>
#include <gpiod.hpp>
#include <cstdlib>

#include <ipmid/api.hpp>
#include <ipmid/utils.hpp>
#include <host-ipmid/ipmid-api.h>

#include <sdbusplus/bus.hpp>
#include <sdbusplus/message.hpp>
#include <phosphor-logging/log.hpp>
#include <nlohmann/json.hpp>
#include <amd/libapmlpower.hpp>
#include "xyz/openbmc_project/Control/Power/RestoreDelay/server.hpp"

#include "version-utils.hpp"
#include "common-utils.hpp"

using namespace phosphor::logging;
using namespace sdbusplus::xyz::openbmc_project::Control::Power::server;

const static constexpr char* SYS_LED_PATH = "/sys/class/leds/sys_fault/brightness";

namespace ipmi
{

void register_netfn_mct_oem() __attribute__((constructor));

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

bool biosRecoveryMode()
{
    uint8_t biosRecovery = 0x02;

    auto bus = sdbusplus::bus::new_default();
    auto method = bus.new_method_call(dualBios::busName, dualBios::path,
                                      dualBios::interface, dualBios::propBiosAction);
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

static bool defaultCounterLoaded = false;
static uint8_t defaultDimmEccCounter = 0;

static std::string dimmArray[12][4] = {
    {"A0", "A1", "2", "0xC000216A"},
    {"B0", "B1", "4", "0xC000215A"},
    {"C0", "C1", "0", "0xC000215A"},
    {"D0", "D1", "4", "0xC000216A"},
    {"E0", "E1", "0", "0xC000216A"},
    {"F0", "F1", "2", "0xC000215A"},
    {"G0", "G1", "8", "0xC000216A"},
    {"H0", "H1", "10", "0xC000215A"},
    {"I0", "I1", "6", "0xC000215A"},
    {"J0", "J1", "10", "0xC000216A"},
    {"K0", "K1", "6", "0xC000216A"},
    {"L0", "L1", "8", "0xC000215A"},
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
            ret = common::setBmcService("yaap.service","StartUnit","EnableUnitFiles");
        }
        else
        {
            ret = common::setBmcService("yaap.service","StopUnit","DisableUnitFiles");
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
            ret = common::setBmcService("start-ipkvm.service","StartUnit","EnableUnitFiles");
        }
        else
        {
            ret = common::setBmcService("start-ipkvm.service","StopUnit","DisableUnitFiles");
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

        ret = common::setBmcService("bmcweb.service","RestartUnit","Null");

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

    constexpr auto postCodeInterface = "xyz.openbmc_project.mct.PostCode";
    constexpr auto postCodeObjPath   = "/xyz/openbmc_project/mct/PostCode";
    constexpr auto postCodeService   = "xyz.openbmc_project.mct.PostCode";

    std::shared_ptr<sdbusplus::asio::connection> bus = getSdBus();

    std::vector<uint64_t> postCodeBuffer;
    std::vector<uint8_t> dataInfo;
    uint16_t currentBootCycle = 0;
    int postCodeIndex = 0;

    try
    {
        /* Set the argument for method call */
        auto msg = bus->new_method_call(postCodeService, postCodeObjPath,
                                         postCodeInterface, "GetPostCodes");

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
            dataInfo.push_back(postCodeBuffer[postCodeIndex]);
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
    std::string solPatternObjPath = sol::path + std::to_string((patternNum));
    std::string s(patternData.begin(), patternData.end());
    std::shared_ptr<sdbusplus::asio::connection> dbus = getSdBus();
    try
    {
        ipmi::setDbusProperty(*dbus, sol::busName, solPatternObjPath,
                               sol::interface, sol::request, s);
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
    std::string solPatternObjPath = sol::path + std::to_string((patternNum));
    std::string patternData;

    std::shared_ptr<sdbusplus::asio::connection> dbus = getSdBus();
    try
    {
        auto value = ipmi::getDbusProperty(*dbus, sol::busName, solPatternObjPath,
                               sol::interface, sol::request);
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
    static const std::string sysFmcWDTEndFilePath = "/run/sysFmcWDTEnd";

    uint8_t bootSource = 0;
    uint32_t registerBuffer = 0;
    uint32_t switchMask1 = 0x49504D49; // ASCII IPMI
    uint32_t switchMask2 = 0xD1000000; // checksum

    uint32_t bSramControlAddress = 0x1e6ef000;
    uint32_t bSramKey = 0xdba078e2;
    uint32_t bSramStoreBSAddress = 0x1e6ef120;

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
                ret = common::setBmcService("system-fmc-watchdog.service","StopUnit","Null");
                if(ret < 0)
                {
                    return ipmi::responseUnspecifiedError();
                }

                while (true)
                {
                    if (access(sysFmcWDTEndFilePath.c_str(), F_OK ) != -1 )
                    {
                        break;
                    }
                    sleep(1);
                }

                write_register(bSramControlAddress, bSramKey);
                // ASCII IPMI to SRAM
                write_register(bSramStoreBSAddress, 0x49504D49);
                // Checksum to SRAM
                write_register(bSramStoreBSAddress + 4, 0xD1000000);
                write_register(bSramControlAddress, 0x0);

                write_register(0x1E620068, 0x00000064); // Timeout limit setting to 10s. 
                sleep(1);
                write_register(0x1E62006C, 0x00004755);
                sleep(1);
                write_register(0x1E620064, 0x00000001);
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
    if(!common::isPowerOn() || !common::hasBiosPost())
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

    if(!common::isPowerOn() || !common::hasBiosPost())
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

    if(!common::isPowerOn() || !common::hasBiosPost())
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

        auto method = bus->new_method_call(dualBios::busName, dualBios::path,
                                           dualBios::interface, dualBios::propCurrentBios);
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
        auto method = bus->new_method_call(dualBios::busName, dualBios::path, PROPERTY_INTERFACE, "Get");
        method.append(dualBios::interface, dualBios::propBiosSelect);
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
    if (!inputSysLedFile.good())
    {
        std::cerr << "File not exist\n";
        return ipmi::responseUnspecifiedError();
    }
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
            08h-DIMM Channel I ECC count
            09h-DIMM Channel J ECC count
            0Ah-DIMM Channel K ECC count
            0Bh-DIMM Channel L ECC count
Response:
        Byte 1 : Completion Code
        Byte 2 : Correctable ECC Count
*/
ipmi::RspType<uint8_t> ipmiGetHostEccCount(uint8_t channelNum)
{
    uint8_t count;

    if(!common::isPowerOn() || !common::hasBiosPost())
    {
        return ipmi::responseCommandNotAvailable();
    }

    if(!common::isPowerOn())
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
            04h-DC-SCM CPLD
            05h-PSU1
            06h-PSU2
            07h-MB CPLD
Response:
        Byte 1 : Completion Code
        Byte 2-N : Firmware revision
        Active/Backup BMC :
            Byte 2 : Major firmware revision
            Byte 3 : Minor firmware revision
        Active/Backup BIOS :
            Byte 2-N : Firmware revision with with ASCII format
        MB/DC-SCM CPLD :
            Byte 2 : Major firmware revision
            Byte 3 : Minor firmware revision
        PSU1/PSU2 :
            Byte 2 : Major firmware revision
            Byte 3 : Minor firmware revision
            Byte 4 : Maintain firmware Version
*/
ipmi::RspType<std::vector<uint8_t>> ipmiGetSysFirmwareVersion(uint8_t selectFirmware)
{
    std::vector<uint8_t> firmwareVersion;

    //It's not allow to read BIOS version when BIOS still posting.
    if(selectFirmware == version::ACTIVE_BIOS || selectFirmware == version::BACKUP_BIOS)
    {
        if(common::isPowerOn() && !common::hasBiosPost())
        {
            return ipmi::responseCommandNotAvailable();
        }
    }

    switch(selectFirmware)
    {
        case version::ACTIVE_BMC:
            firmwareVersion = version::readActiveBmcVersion();
            break;
        case version::BACKUP_BMC:
            firmwareVersion = version::readBackupBmcVersion();
            break;
        case version::ACTIVE_BIOS:
            firmwareVersion = version::readBiosVersion(0);
            break;
        case version::BACKUP_BIOS:
            firmwareVersion = version::readBiosVersion(1);
            break;
        case version::MB_CPLD:
            firmwareVersion = version::getCpldVersion(version::MB_CPLD);
            break;
        case version::DC_SCM_CPLD:
            firmwareVersion = version::getCpldVersion(version::DC_SCM_CPLD);
            break;
        // case version::PSU1:
        //     firmwareVersion = version::getPsuVersion(1);
        //     break;
        // case version::PSU2:
        //     firmwareVersion = version::getPsuVersion(2);
        //     break;
        default:
            return ipmi::responseInvalidFieldRequest();
    }

    if(version::isFailedToReadFwVersion(firmwareVersion))
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
        00h : Disable AMD MBIST feature and log MBIST SEL once (This command only recommend BIOS internal used.)
        01h : Disable AMD MBIST feature
        02h : Enable Algorithm #1: Write-Pause-Read (This test will take ~11 seconds per memory rank per pattern).
        03h : Enable Algorithm #2: Write-Pause-Read, short tWR (This test will take ~26 seconds per memory rank per pattern).
        04h : Enable Algorithm #3: Row Victim Aggressor (This test will take ~22 seconds per memory rank per pattern).
        05h : Enable Algorithm #4: Write-Pause-Read, Closed Page (This test will take ~19 seconds per memory rank per pattern).
        06h : Enable Algorithm #5: Closed Page Stress Test (This test will take ~29 seconds per memory rank per pattern).
        07h : Enable Algorithm #6: Double Row Victim Aggressor (This test will take ~18 seconds per memory rank per pattern).
        08h : Enable Algorithm #7: Open page Checkerboard RMW (This test will take ~47 seconds per memory rank per pattern).
        09h : Enable Algorithm #8: Closed page tWR Stress (This test will take ~28 seconds per memory rank per pattern).
        0Ah : Enable Algorithm #9: Open Page Static Refresh Test (This test will take ~6 seconds per memory rank per pattern)
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
            case 0:
            case 1:
            case 2:
            case 3:
            case 4:
            case 5:
            case 6:
            case 7:
            case 8:
            case 9:
            case 10:
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
/* BMC restore default command
NetFun: 0x30
Cmd : 0xB4
Request:

Response:
    Byte 1 : Completion Code
*/

ipmi::RspType<> ipmiBmcRestoreDefault()
{
    int ret;
    constexpr auto service = "xyz.openbmc_project.Software.BMC.Updater";
    constexpr auto path = "/xyz/openbmc_project/software";
    constexpr auto interface = "xyz.openbmc_project.Common.FactoryReset";
    auto bus = sdbusplus::bus::new_default();
    auto method = bus.new_method_call(service, path, interface, "Reset");

    try
    {
        bus.call_noreply(method);

    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        return ipmi::responseUnspecifiedError();
    }

    ret = common::setBmcService("delay-bef-bmc-reboot@10.service", "StartUnit", "Null");
    if (ret < 0)
    {
        return ipmi::responseUnspecifiedError();
    }

    return ipmi::responseSuccess();
}

//===============================================================
/* Get current Node ID
NetFun: 0x30
Cmd : 0x24
Request:

Response:
    Byte 1 : Completion Code
    Byte 2 : Current Node ID
*/

ipmi::RspType<uint8_t> ipmiGetCurrentNodeId()
{
    uint8_t nodeId;

    int ret = common::getCurrentNode(nodeId);
    if(ret != 0)
    {
        return ipmi::responseUnspecifiedError();
    }

    return ipmi::responseSuccess(nodeId);
}

//===============================================================
/* Set special mode
NetFun: 0x30
Cmd : 0x25
Request:
    Byte 1 : Setting special mode
        00h - Normal mode
        01h - Manufacturing mode
        02h - ValidationUnsecure mode
Response:
    Byte 1 : Completion Code
*/

ipmi::RspType<> ipmiSetSpecialMode(uint8_t mode)
{
    secCtrl::SpecialMode::Modes specialMode;

    switch(mode)
    {
        case 0:
            specialMode = secCtrl::SpecialMode::Modes::None;
            break;
        case 1:
            specialMode = secCtrl::SpecialMode::Modes::Manufacturing;
            break;
        case 2:
            specialMode = secCtrl::SpecialMode::Modes::ValidationUnsecure;
            break;
    }

    int ret = common::setCurrentSpecialMode(specialMode);
    if(ret != 0)
    {
        return ipmi::responseUnspecifiedError();
    }

    return ipmi::responseSuccess();
}

//===============================================================
/* Get special mode
NetFun: 0x30
Cmd : 0x26
Request:

Response:
    Byte 1 : Completion Code
    Byte 2 : Current special mode
        00h - Normal mode
        01h - Manufacturing mode
        02h - ValidationUnsecure mode
*/

ipmi::RspType<uint8_t> ipmiGetSpecialMode()
{
    uint8_t mode;
    secCtrl::SpecialMode::Modes specialMode;

    int ret = common::getCurrentSpecialMode(specialMode);
    if(ret != 0)
    {
        return ipmi::responseUnspecifiedError();
    }

    switch(specialMode)
    {
        case secCtrl::SpecialMode::Modes::None:
            mode = 0;
            break;
        case secCtrl::SpecialMode::Modes::Manufacturing:
            mode = 1;
            break;
        case secCtrl::SpecialMode::Modes::ValidationUnsecure:
            mode = 2;
            break;
    }

    return ipmi::responseSuccess(mode);
}

//===============================================================
/* Set Aspeed OTP strap
NetFun: 0x30
Cmd : 0x27
Request:
    Byte 1 : Specified OTP strap offset
    Byte 2 : Specified OTP strap value
Response:
    Byte 1 : Completion Code
*/

ipmi::RspType<> ipmiSetOtpStrap(uint8_t offset, uint8_t value)
{
    std::string cmd = "yes YES | mct-aspeed-otp-tool pb strap " +
                      common::intToHexString(offset) + " " +
                      std::to_string(value);

    FILE* fp = popen(cmd.c_str(), "r");
    if (!fp)
    {
        return ipmi::responseUnspecifiedError();
    }

    if(pclose(fp) != 0)
    {
        return ipmi::responseUnspecifiedError();
    }

    return ipmi::responseSuccess();
}

//===============================================================
/* Get Aspeed OTP strap
NetFun: 0x30
Cmd : 0x28
Request:
    Byte 1 : Specified OTP strap offset
Response:
    Byte 1 : Completion Code
    Byte 2 : Current OTP strap value
*/

ipmi::RspType<uint8_t> ipmiGetOtpStrap(uint8_t offset)
{
    char buffer[100];
    std::string cmdSuccess = "BIT(hex)";
    std::string cmdValue = "0x";
    bool getOtpStrap = false;
    std::string cmdResult;

    std::string cmd = "mct-aspeed-otp-tool read strap " +
                      common::intToHexString(offset) + " 1";

    FILE* fp = popen(cmd.c_str(), "r");
    if (!fp)
    {
        return ipmi::responseUnspecifiedError();
    }

    while (fgets(buffer, sizeof(buffer), fp) != NULL)
    {
        if(getOtpStrap)
        {
            if(strstr(buffer, cmdValue.c_str()) != NULL)
            {
                cmdResult = buffer;
            }
        }
        if(strstr(buffer, cmdSuccess.c_str()) != NULL)
        {
            getOtpStrap = true;
        }
    }

    if(pclose(fp) != 0)
    {
        return ipmi::responseUnspecifiedError();
    }
    return ipmi::responseSuccess(stoi(cmdResult.substr (8,9)));
}

//=======================================================================
/* Get/Set BMC P2A status
NetFun: 0x30
Cmd : 0x29
Request:
    Byte 1 : Status (optional: To get BMC P2A status)
        00h : Disable BMC P2A feature
        01h : Enable BMC P2A feature
Response:
    Byte 1 : Completion Code
    Byte 2 : BMC P2A status
*/
ipmi::RspType<uint8_t> ipmiConfigBmcP2a(std::optional<uint8_t> configBmcP2a)
{
    uint8_t p2aResponse = 0;
    uint32_t registerBuffer = 0;
    uint32_t p2aRegiter = 0x1e6e20c8;

    constexpr auto service = "xyz.openbmc_project.Settings";
    constexpr auto path = "/xyz/openbmc_project/oem/ServiceStatus";
    constexpr auto interface = "xyz.openbmc_project.OEM.ServiceStatus";
    constexpr auto p2aService = "P2aService";

    auto bus = sdbusplus::bus::new_default();

    if(configBmcP2a)
    {
        if (read_register(p2aRegiter, &registerBuffer) < 0)
        {
            return ipmi::responseUnspecifiedError();
        }

        switch(configBmcP2a.value())
        {
            case 0:
                registerBuffer = registerBuffer | 0x00000001;
                break;
            case 1:
                registerBuffer = registerBuffer & (~0x00000001);
                break;
            default :
                return ipmi::responseInvalidFieldRequest();
        }


        write_register(p2aRegiter, registerBuffer);

        auto method = bus.new_method_call(service, path, PROPERTY_INTERFACE,"Set");
        method.append(interface, p2aService, std::variant<bool>((bool)(configBmcP2a.value()& 0x01)));
        bus.call_noreply(method);
        p2aResponse = configBmcP2a.value();
    }
    else
    {
        if (read_register(p2aRegiter, &registerBuffer) < 0)
        {
            return ipmi::responseUnspecifiedError();
        }

        if(registerBuffer&0x00000001)
        {
            p2aResponse = 0;
        }
        else
        {
            p2aResponse = 1;
        }
    }

    return ipmi::responseSuccess(p2aResponse);
}

//===============================================================
/* Disable hot swap control command
NetFun: 0x38
Cmd : 0x20
Request:

Request:
    Byte 1 : Completion Code
*/

ipmi::RspType<> ipmiDisableHsc()
{

    int ret = common::setBmcService("setting-hsc-register@DisableIPMI.service","StartUnit","Null");

    if(ret < 0)
    {
        return ipmi::responseUnspecifiedError();
    }

    return ipmi::responseSuccess();
}

//===============================================================
/* Hot swap control - AC cycle start command
NetFun: 0x38
Cmd : 0x21
Request:

Request:
    Byte 1 : Completion Code
*/

ipmi::RspType<> ipmiHscACStart()
{
    int ret = common::setBmcService("setting-hsc-register@HSCACStart.service","StartUnit","Null");

    if(ret < 0)
    {
        return ipmi::responseUnspecifiedError();
    }

    return ipmi::responseSuccess();
}

//===============================================================
/* Hot swap control - Config AC cycle delay command
NetFun: 0x38
Cmd : 0x22
Request:
    Byte 1 : Delay time setting (optional: if NULL, get the current delay time)
Request:
    Byte 1 : Completion Code
    Byte 2 : Current delay time
*/

ipmi::RspType<uint8_t> ipmiHscConfigACDelay(std::optional<uint8_t> ac_delay)
{
    uint8_t current_delay = 0;
    int ret;

    if (ac_delay)
    {
        char service[128] = {};
        snprintf(service, sizeof(service), "setting-hsc-register@SetDelay_0x%02X.service", ac_delay.value());
        int ret = common::setBmcService(service,"StartUnit","Null");

        if(ret < 0)
        {
            return ipmi::responseUnspecifiedError();
        }
        current_delay = ac_delay.value();
    }
    else
    {
        /*
        // TODO: Should be used after env-manager finished.
        char* hsc_bus_env_p = std::getenv("hsc_bus");
        if(hsc_bus_env_p != nullptr)
            std::cerr << "hsc_bus: " << hsc_bus_env_p << '\n';
        else
        {
            std::cerr << "Could not find env var: hsc_bus. \n";
            return ipmi::responseUnspecifiedError();
        }
        std::string hsc_bus(hsc_bus_env_p);

        char* hsc_address_env_p = std::getenv("hsc_address");
        if(hsc_address_env_p != nullptr)
            std::cerr << "hsc_address: " << hsc_address_env_p << '\n';
        else
        {
            std::cerr << "Could not find env var: hsc_address. \n";
            return ipmi::responseUnspecifiedError();
        }
        std::string hsc_address(hsc_address_env_p);*/

        std::shared_ptr<sdbusplus::asio::connection> busp = getSdBus();
        ipmi::Value result;
        uint8_t node_ID, hsc_bus = 16, hsc_address;

        ret = common::getCurrentNode(node_ID);
        if(ret != 0)
        {
            return ipmi::responseUnspecifiedError();
        }

        if (node_ID == 1)
            hsc_address = 0x10;
        else
            hsc_address = 0x12;

        std::string i2cBus = "/dev/i2c-" + std::to_string(hsc_bus);
        std::cerr << "hsc_bus: " << i2cBus << " /hsc_address: " << std::to_string(hsc_address) << '\n';

        std::vector<uint8_t> readBuf(1);
        std::vector<uint8_t> writeData{0xcc};

        ret = ipmi::i2cWriteRead(i2cBus, hsc_address, writeData, readBuf);

        current_delay = (uint8_t)readBuf[0];
    }
    return ipmi::responseSuccess(current_delay);
}


void register_netfn_mct_oem()
{
    ipmi::registerHandler(ipmi::prioMax, NETFUN_TWITTER_OEM, IPMI_CMD_RamdomDelayACRestorePowerON, ipmi::Privilege::Admin, ipmi_tyan_RamdomDelayACRestorePowerON);
    ipmi::registerHandler(ipmi::prioMax, NETFUN_TWITTER_OEM, IPMI_CMD_SetService, ipmi::Privilege::Admin, ipmi_SetService);
    ipmi::registerHandler(ipmi::prioMax, NETFUN_TWITTER_OEM, IPMI_CMD_GetService, ipmi::Privilege::Admin, ipmi_GetService);
    ipmi::registerHandler(ipmi::prioMax, NETFUN_TWITTER_OEM, IPMI_CMD_GetPostCode, ipmi::Privilege::Admin, ipmi_GetPostCode);
    ipmi::registerHandler(ipmi::prioMax, NETFUN_TWITTER_OEM, IPMI_CMD_SET_SOL_PATTERN, ipmi::Privilege::Admin, ipmiSetSolPattern);
    ipmi::registerHandler(ipmi::prioMax, NETFUN_TWITTER_OEM, IPMI_CMD_GET_SOL_PATTERN, ipmi::Privilege::Admin, ipmiGetSolPattern);
    ipmi::registerHandler(ipmi::prioMax, NETFUN_TWITTER_OEM, IPMI_CMD_BmcFlashDevice, ipmi::Privilege::Admin, ipmiBmcFlashDevice);
    ipmi::registerHandler(ipmi::prioMax, NETFUN_TWITTER_OEM, IPMI_CMD_GetCpuPower, ipmi::Privilege::Admin, ipmiGetCpuPower);
    ipmi::registerHandler(ipmi::prioMax, NETFUN_TWITTER_OEM, IPMI_CMD_GetCpuPowerLimit, ipmi::Privilege::Admin, ipmiGetCpuPowerLimit);
    ipmi::registerHandler(ipmi::prioMax, NETFUN_TWITTER_OEM, IPMI_CMD_SetCpuPowerLimit, ipmi::Privilege::Admin, ipmiSetCpuPowerLimit);
    ipmi::registerHandler(ipmi::prioMax, NETFUN_TWITTER_OEM, IPMI_CMD_SetDefaultCpuPowerLimit, ipmi::Privilege::Admin, ipmiSetDefaultCpuPowerLimit);
    ipmi::registerHandler(ipmi::prioMax, NETFUN_TWITTER_OEM, IPMI_CMD_SetSysPowerLimit, ipmi::Privilege::Admin, ipmiSetSysPowerLimit);
    ipmi::registerHandler(ipmi::prioMax, NETFUN_TWITTER_OEM, IPMI_CMD_GetSysPowerLimit, ipmi::Privilege::Admin, ipmiGetSysPowerLimit);
    ipmi::registerHandler(ipmi::prioMax, NETFUN_TWITTER_OEM, IPMI_CMD_EnableCpuPowerLimit, ipmi::Privilege::Admin, ipmiEnableSysPowerLimit);
    ipmi::registerHandler(ipmi::prioMax, NETFUN_TWITTER_OEM, IPMI_CMD_BiosRecovery, ipmi::Privilege::Admin, ipmiBiosRecovery);
    ipmi::registerHandler(ipmi::prioMax, NETFUN_TWITTER_OEM, IPMI_CMD_BiosFlashDevice, ipmi::Privilege::Admin, ipmiBiosFlashDevice);
    ipmi::registerHandler(ipmi::prioMax, NETFUN_TWITTER_OEM, IPMI_CMD_GetAmberLedStatus, ipmi::Privilege::Admin, ipmiGetAmberStatus);
    ipmi::registerHandler(ipmi::prioMax, NETFUN_TWITTER_OEM, IPMI_CMD_GetHostEccCount, ipmi::Privilege::Admin, ipmiGetHostEccCount);
    ipmi::registerHandler(ipmi::prioMax, NETFUN_TWITTER_OEM, IPMI_CMD_GetSysFirmwareVersion, ipmi::Privilege::Admin, ipmiGetSysFirmwareVersion);
    ipmi::registerHandler(ipmi::prioMax, NETFUN_TWITTER_OEM, IPMI_CMD_ConfigAmdMbist, ipmi::Privilege::Admin, ipmiConfigAmdMbist);
    ipmi::registerHandler(ipmi::prioMax, NETFUN_TWITTER_OEM, IPMI_CMD_BMC_RESTORE_DEFAULT, ipmi::Privilege::Admin, ipmiBmcRestoreDefault);
    ipmi::registerHandler(ipmi::prioMax, NETFUN_TWITTER_OEM, IPMI_CMD_GetCurrentNodeId, ipmi::Privilege::Admin, ipmiGetCurrentNodeId);
    ipmi::registerHandler(ipmi::prioMax, NETFUN_TWITTER_OEM, IPMI_CMD_SetSpecialMode, ipmi::Privilege::Admin, ipmiSetSpecialMode);
    ipmi::registerHandler(ipmi::prioMax, NETFUN_TWITTER_OEM, IPMI_CMD_GetSpecialMode, ipmi::Privilege::Admin, ipmiGetSpecialMode);
    ipmi::registerHandler(ipmi::prioMax, NETFUN_TWITTER_OEM, IPMI_CMD_SetOtpStrap, ipmi::Privilege::Admin, ipmiSetOtpStrap);
    ipmi::registerHandler(ipmi::prioMax, NETFUN_TWITTER_OEM, IPMI_CMD_GetOtpStrap, ipmi::Privilege::Admin, ipmiGetOtpStrap);
    ipmi::registerHandler(ipmi::prioMax, NETFUN_TWITTER_OEM, IPMI_CMD_ConfigBmcP2a, ipmi::Privilege::Admin, ipmiConfigBmcP2a);
    ipmi::registerHandler(ipmi::prioMax, NETFUN_TWITTER_OEM_38, IPMI_CMD_DisableHsc, ipmi::Privilege::Admin, ipmiDisableHsc);
    ipmi::registerHandler(ipmi::prioMax, NETFUN_TWITTER_OEM_38, IPMI_CMD_HscACStart, ipmi::Privilege::Admin, ipmiHscACStart);
    ipmi::registerHandler(ipmi::prioMax, NETFUN_TWITTER_OEM_38, IPMI_CMD_HscConfigACDelay, ipmi::Privilege::Admin, ipmiHscConfigACDelay);
}
}
