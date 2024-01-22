#include "general-cmd-overlay.hpp"
#include <iostream>
#include <sys/ioctl.h>
#include <linux/rtc.h>

#include <ipmid/api.hpp>
#include <ipmid/utils.hpp>
#include <host-ipmid/ipmid-api.h>

#include <sdbusplus/bus.hpp>
#include <sdbusplus/message.hpp>
#include <phosphor-logging/log.hpp>

#include "tyan-utils.hpp"
#include "common-utils.hpp"

const static constexpr char* RTC_PATH = "/dev/rtc0";

using namespace phosphor::logging;

namespace ipmi
{

void register_netfn_ipmi_overlay() __attribute__((constructor));

bool setRtcTime(uint32_t selTime)
{
    int fd;
    struct rtc_time rtcTime;
    time_t t = selTime;
    struct tm *currentTime = gmtime(&t);

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

//===============================================================
/* Get Device GUID Command
 * Override the general IPMI command
NetFun: 0x06
Cmd : 0x08
Request:

Response:
    Byte 1 : Completion Code
    Byte 2-17 : GUID bytes 1 through 16.
*/
ipmi::RspType<std::vector<uint8_t>> ipmiAppGetDeviceGuid()
{
    ipmi::Cc ret;
    uint8_t busId, slaveAddr;
    uint8_t readCount = 0x01;
    uint8_t offsetMSB, offsetLSB;
    uint8_t selctField = 0x02; // 2h-Device GUID, 16 Bytes

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
/* Get System GUID Command
 * Override the general IPMI command
NetFun: 0x06
Cmd : 0x37
Request:

Response:
    Byte 1 : Completion Code
    Byte 2-17 : GUID bytes 1 through 16.
*/
ipmi::RspType<std::vector<uint8_t>> ipmiAppGetSystemGuid()
{
    ipmi::Cc ret;
    uint8_t busId, slaveAddr;
    uint8_t readCount = 0x01;
    uint8_t offsetMSB, offsetLSB;
    uint8_t selctField = 0x03; // 3h-System GUID, 16 Bytes

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

void register_netfn_ipmi_overlay()
{
    ipmi::registerHandler(ipmi::prioMax, ipmi::netFnApp, ipmi::app::cmdGetDeviceGuid, ipmi::Privilege::Admin, ipmiAppGetDeviceGuid);
    ipmi::registerHandler(ipmi::prioMax, ipmi::netFnApp, ipmi::app::cmdGetSystemGuid, ipmi::Privilege::Admin, ipmiAppGetSystemGuid);
}
}
