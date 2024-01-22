#include "tyan-oem-cmd.hpp"
#include <iostream>
#include <fstream>
#include <gpiod.hpp>

#include <ipmid/api.hpp>
#include <ipmid/utils.hpp>
#include <host-ipmid/ipmid-api.h>

#include <sdbusplus/bus.hpp>
#include <sdbusplus/message.hpp>
#include <phosphor-logging/log.hpp>
#include <nlohmann/json.hpp>

#include "common-utils.hpp"
#include "version-utils.hpp"
#include "tyan-utils.hpp"

using namespace phosphor::logging;

namespace ipmi
{

void register_netfn_tyan_oem() __attribute__((constructor));

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

constexpr auto fanService = "xyz.openbmc_project.FanSensor";
constexpr auto fanIntf = "xyz.openbmc_project.Control.FanPwm";

/* Return value: 01h for auto, 00h for manual */
uint8_t getFanControlMode()
{
    uint8_t currentStatus;
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

    return currentStatus;
}

// High active
bool clearCmos()
{
    int value = 0;

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

    line.set_value(1);
    sleep(3);
    line.set_value(0);

    line.release();
    return true;
}

//===============================================================
/* Set Fan Control Enable Command
NetFun: 0x2E
Cmd : 0x40
Request:
        Byte 1-3 : Tyan Manufactures ID (FD 19 00)
        Byte 4 :  00h - Disable Fan Control (manual)
                  01h - Enable Fan Control (auto)
Response:
        Byte 1 : Completion Code
        Byte 2-4 : Tyan Manufactures ID
*/
ipmi::RspType<> ipmi_tyan_SetFanControl(uint8_t mode)
{
    uint8_t currentStatus = getFanControlMode();

    // Set to manual and current in auto mode
    if (mode == 0x00 && currentStatus == 0x01)
    {
        system("/usr/bin/env controlFan.sh manual-mode");
    }
    // Set to auto and current in manual mode
    else if (mode == 0x01 && currentStatus == 0x00)
    {
        system("/usr/bin/env controlFan.sh auto-mode");
    }
    // Needn't change mode
    else if ((mode == 0x00 && currentStatus == 0x00) ||
             (mode == 0x01 && currentStatus == 0x01))
    {
        return ipmi::responseSuccess();
    }
    else
    {
        return ipmi::responseUnspecifiedError();
    }

    return ipmi::responseSuccess();
}

//===============================================================
/* Get Fan Control Enable Command
NetFun: 0x2E
Cmd : 0x41
Request:
        Byte 1-3 : Tyan Manufactures ID (FD 19 00)
Response:
        Byte 1 : Completion Code
        Byte 2-4 : Tyan Manufactures ID
        Byte 5 : Current Fan Control Status
            00h - Disable (manual)
            01h - Enable (auto)
*/
ipmi::RspType<uint8_t> ipmi_tyan_GetFanControl()
{
    uint8_t currentStatus = getFanControlMode();

    return ipmi::responseSuccess(currentStatus);
}

//===============================================================
/* Set Fan Control PWM Duty Command
NetFun: 0x2E
Cmd : 0x44
Request:
        Byte 1-3 : Tyan Manufactures ID (FD 19 00)
        Byte 4 : PWM ID
            00h - FAN1 PWM0 //hwmon pwm1
            01h - FAN2 PWM3 //hwmon pwm4
            02h - FAN3 PWM7 //hwmon pwm8
            03h - FAN4 PWM0 //hwmon pwm1
            04h - FAN5 PWM3 //hwmon pwm4
            05h - FAN6 PWM7 //hwmon pwm8

        Byte 5 : PWM mode
            00h - Auto control (Enable fan control)
            01h - Manual control (Disable fan control)

        Byte 6 : Duty Cycle //Only available in manual mode
            0-64h - manual control duty cycle (0%-100%)

Response:
        Byte 1 : Completion Code
        Byte 2-4 : Tyan Manufactures ID
*/
ipmi::RspType<> ipmi_tyan_SetFanPwmDuty(uint8_t pwmId, uint8_t mode, uint8_t duty)
{
    uint8_t pwmValue = 0;
    uint8_t control_mode;
    uint8_t nodeId;
    secCtrl::SpecialMode::Modes specialMode;

    int ret = common::getCurrentSpecialMode(specialMode);

    if(ret != 0)
    {
        return ipmi::responseUnspecifiedError();
    }

    if(specialMode == secCtrl::SpecialMode::Modes::None)
    {
        int ret = common::getCurrentNode(nodeId);
        if(ret != 0)
        {
            return ipmi::responseUnspecifiedError();
        }

        switch(nodeId)
        {
            case 1:
                switch(pwmId)
                {
                    case 0:
                    case 1:
                    case 2:
                        break;
                    default:
                        return ipmi::responseParmOutOfRange();
                }
                break;
            case 2:
                switch(pwmId)
                {
                    case 3:
                    case 4:
                    case 5:
                        break;
                    default:
                        return ipmi::responseParmOutOfRange();
                }
                break;

            default:
                return ipmi::responseParmOutOfRange();
        }
    }

    if (mode == 0x1)
    {
        if (duty <= 0x64)
        {
            control_mode = getFanControlMode();
            if (control_mode != 0x0) // 00h: manual
            {
                // Disable Fan Control
                system("/usr/bin/env controlFan.sh manual-mode");
            }

            // control duty cycle (0%-100%)
            pwmValue = duty * 255 / 100;

            std::shared_ptr<sdbusplus::asio::connection> bus = getSdBus();
            std::string object = "/xyz/openbmc_project/control/fanpwm/SYS_PWM_" + std::to_string(pwmId+1);

            try
            {
                ipmi::setDbusProperty(*bus, fanService, object,
                                       fanIntf, "Target", static_cast<uint64_t>(pwmValue));
            }
            catch (const sdbusplus::exception::SdBusError& e)
            {
                return ipmi::responseUnspecifiedError();
            }
        }
        else
        {
            return ipmi::responseParmOutOfRange();
        }
    }
    else if (mode == 0x0)
    {
        control_mode = getFanControlMode();
        if (control_mode != 0x1) //01h: manual
        {
            // Enable Fan Control
            system("/usr/bin/env controlFan.sh auto-mode");
        }
    }
    else
    {
        return ipmi::responseUnspecifiedError();
    }

    return ipmi::responseSuccess();
}

//===============================================================
/* Get Fan Control PWM Duty Command
NetFun: 0x2E
Cmd : 0x45
Request:
        Byte 1-3 : Tyan Manufactures ID (FD 19 00)
        Byte 4 : PWM ID
            00h - FAN1 PWM0 //hwmon pwm1
            01h - FAN2 PWM3 //hwmon pwm4
            02h - FAN3 PWM7 //hwmon pwm8
            03h - FAN4 PWM0 //hwmon pwm1
            04h - FAN5 PWM3 //hwmon pwm4
            05h - FAN6 PWM7 //hwmon pwm8

Response:
        Byte 1 : Completion Code
        Byte 2-4 : Tyan Manufactures ID
        Byte 5 : Current PWM mode
                 00h - Auto mode (Enable fan control)
                 01h - Manual mode (Disable fan control)
        Byte 6 : Current Duty Cycle
*/
ipmi::RspType<uint8_t, uint8_t> ipmi_tyan_GetFanPwmDuty(uint8_t pwmId)
{

    uint8_t responseStatus;
    uint8_t responseDuty;
    uint64_t pwmValue = 0;
    uint8_t currentStatus = getFanControlMode();

    responseStatus = ~currentStatus & 0x1; // 00h for auto and 01h for manual in tyan oem command spec

    std::string object = "/xyz/openbmc_project/control/fanpwm/SYS_PWM_" + std::to_string(pwmId+1);
    std::shared_ptr<sdbusplus::asio::connection> bus = getSdBus();

    try
    {
        auto value = ipmi::getDbusProperty(*bus, fanService, object, fanIntf, "Target");
        pwmValue = std::get<uint64_t>(value);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        return ipmi::responseUnspecifiedError();
    }

    responseDuty = pwmValue * 100 / 255;

    return ipmi::responseSuccess(responseStatus, responseDuty);
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
/* set gpio status Command (for manufacturing)
NetFun: 0x2E
Cmd : 0x3
Request:
       Byte 1-3 : Tyan Manufactures ID (FD 19 00)
       Byte 4 : gpio number
       Byte 5 : GPIO direction
                00h - Input
                01h - Output
       Byte 6 : GPIO data
                00h - Low
                01h - High
Response:
        Byte 1 : Completion Code
        Byte 2-4 : Tyan Manufactures ID
*/
ipmi::RspType<>ipmi_tyan_setGpio(uint8_t gpioNo, uint8_t dir, uint8_t data)
{
    if (dir > 1 || data > 1)
    {
        return ipmi::responseParmOutOfRange();
    }

    auto chip = gpiod::chip("gpiochip0");
    auto line = chip.get_line(gpioNo);
    if (!line)
    {
        std::cerr << "Error requesting gpio\n";
        return ipmi::responseUnspecifiedError();
    }

    try
    {
        if (dir == 0)
        {
            line.request({"ipmid", gpiod::line_request::DIRECTION_INPUT, {}});
        }
        else
        {
            line.request({"ipmid", gpiod::line_request::DIRECTION_OUTPUT, {}}, data);
        }
    }
    catch (std::system_error&)
    {
        std::cerr << "Error setting gpio: " << (unsigned)gpioNo << "\n";
        return ipmi::responseUnspecifiedError();
    }

    line.release();
    return ipmi::responseSuccess();
}


//======================================================================================================
/* get gpio status Command (for manufacturing) 
NetFun: 0x2E
Cmd : 0x4
Request:
       Byte 1-3 : Tyan Manufactures ID (FD 19 00)
        Byte 4 : gpio number 
Response:
        Byte 1 : Completion Code
        Byte 2-4 : Tyan Manufactures ID
        Byte 5 : gpio direction (1: output, 0:input)
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
    auto dir = line.direction(); //2: output, 1:input

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
    return ipmi::responseSuccess((uint8_t)(dir -1), (uint8_t)resp);
}

//===============================================================
/* Get Firmware String Command
NetFun: 0x2E
Cmd : 0x80
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
    uint8_t busId, slaveAddr;
    uint8_t readCount = 0x00;
    uint8_t offsetMSB, offsetLSB;

    if(common::getBaseboardFruAddress(busId, slaveAddr) < 0)
    {
        return ipmi::responseUnspecifiedError();
    }


    ret = tyan::setManufactureFieledProperty(selctField, writeData.size(), readCount, offsetMSB, offsetLSB);

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
    uint8_t busId, slaveAddr;
    uint8_t readCount = 0x01;
    uint8_t offsetMSB, offsetLSB;

    if(common::getBaseboardFruAddress(busId, slaveAddr) < 0)
    {
        return ipmi::responseUnspecifiedError();
    }

    ret = tyan::setManufactureFieledProperty(selctField, 0x00, readCount, offsetMSB, offsetLSB);

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
/* Master write read
NetFun: 0x2E
Cmd : 0x09
Request:
    Byte 1-3 : Tyan Manufactures ID (FD 19 00)
    Byte 4 : Bus Channel Number (0-based)
    Byte 5 : Slave Address (8 bit address)
    Byte 6 : Read Count
    Byte 7-N : Data to write
Response:
    Byte 1 : Completion Code
    Byte 2-4 : Tyan Manufactures ID
    Byte 5-N: Bytes Read
*/
ipmi::RspType<std::vector<uint8_t>> ipmi_tyan_MasterWriteRead(uint8_t busId, uint8_t slaveAddr,
                                                              uint8_t readCount, std::vector<uint8_t> writeData)
{
    ipmi::Cc ret;
    std::vector<uint8_t> readBuf(readCount);
    std::string i2cBus =
        "/dev/i2c-" + std::to_string(static_cast<uint8_t>(busId));

    ret = ipmi::i2cWriteRead(i2cBus, static_cast<uint8_t>(slaveAddr >> 1),
                             writeData, readBuf);

    if(ret !=  ipmi::ccSuccess)
    {
        std::cerr << "ipmi failed\n";
        return ipmi::response(ret);
    }


    return ipmi::responseSuccess(readBuf);
}

//===============================================================
/* Clear CMOS command
NetFun: 0x2E
Cmd : 0x65
Request:
    Byte 1-3 : Tyan Manufactures ID (FD 19 00)
Response:
    Byte 1 : Completion Code
    Byte 2-4 : Tyan Manufactures ID
*/
ipmi::RspType<> ipmiClearCmos()
{
    if(common::isPowerOn())
    {
        return ipmi::responseCommandNotAvailable();
    }

    int ret = common::logSelEvent("Clear CMOS SEL Entry",
                                  "/xyz/openbmc_project/sensors/fru_state/CLEAR_CMOS",
                                  0x02,0x01,0xFF);

    if(ret < 0)
    {
        return ipmi::responseUnspecifiedError();
    }

    std::thread clearCmosThread(clearCmos);
    clearCmosThread.detach();

    return ipmi::responseSuccess();
}

void register_netfn_tyan_oem()
{
    ipmi::registerOemHandler(ipmi::prioMax, IANA_TYAN, IPMI_CMD_SetFanControl, ipmi::Privilege::Admin, ipmi_tyan_SetFanControl);
    ipmi::registerOemHandler(ipmi::prioMax, IANA_TYAN, IPMI_CMD_GetFanControl, ipmi::Privilege::Admin, ipmi_tyan_GetFanControl);
    ipmi::registerOemHandler(ipmi::prioMax, IANA_TYAN, IPMI_CMD_SetFanPwmDuty, ipmi::Privilege::Admin, ipmi_tyan_SetFanPwmDuty);
    ipmi::registerOemHandler(ipmi::prioMax, IANA_TYAN, IPMI_CMD_GetFanPwmDuty, ipmi::Privilege::Admin, ipmi_tyan_GetFanPwmDuty);
    ipmi::registerOemHandler(ipmi::prioMax, IANA_TYAN, IPMI_CMD_ConfigEccLeakyBucket, ipmi::Privilege::Admin, ipmi_tyan_ConfigEccLeakyBucket);
    ipmi::registerOemHandler(ipmi::prioMax, IANA_TYAN, IPMI_CMD_GetEccCount, ipmi::Privilege::Admin, ipmiGetEccCount);
    ipmi::registerOemHandler(ipmi::prioMax, IANA_TYAN, IPMI_CMD_SetgpioStatus, ipmi::Privilege::Admin, ipmi_tyan_setGpio);
    ipmi::registerOemHandler(ipmi::prioMax, IANA_TYAN, IPMI_CMD_GetgpioStatus, ipmi::Privilege::Admin, ipmi_tyan_getGpio);
    ipmi::registerOemHandler(ipmi::prioMax, IANA_TYAN, IPMI_CMD_MasterWriteRead, ipmi::Privilege::Admin, ipmi_tyan_MasterWriteRead);
    ipmi::registerOemHandler(ipmi::prioMax, IANA_TYAN, IPMI_CMD_GetFirmwareString, ipmi::Privilege::Admin, ipmi_getFirmwareString);
    ipmi::registerOemHandler(ipmi::prioMax, IANA_TYAN, IPMI_CMD_SetManufactureData, ipmi::Privilege::Admin,ipmi_setManufactureData);
    ipmi::registerOemHandler(ipmi::prioMax, IANA_TYAN, IPMI_CMD_GetManufactureData, ipmi::Privilege::Admin,ipmi_getManufactureData);
    ipmi::registerOemHandler(ipmi::prioMax, IANA_TYAN, IPMI_CMD_ClearCmos, ipmi::Privilege::Admin,ipmiClearCmos);
}
}
