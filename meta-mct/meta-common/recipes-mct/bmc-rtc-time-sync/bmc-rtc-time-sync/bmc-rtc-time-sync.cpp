#include <iostream>
#include <linux/rtc.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

const static constexpr char* RTC_PATH = "/dev/rtc0";

// will update bmc time if the time difference beyond this value
static constexpr uint8_t timeDiffAllowedSecond = 3;

uint32_t rtc_read_time()
{
    int fd;
    struct rtc_time rtcTime;

    fd = open(RTC_PATH, O_RDWR);
    if (fd ==  -1) {
        return 0;
    }

    int ret = ioctl(fd, RTC_RD_TIME, &rtcTime);
    if (ret == -1) {
            return 0;
    }

    close(fd);

    struct tm currentTime = {0};
    currentTime.tm_year = rtcTime.tm_year;
    currentTime.tm_mday = rtcTime.tm_mday;
    currentTime.tm_mon = rtcTime.tm_mon;
    currentTime.tm_hour = rtcTime.tm_hour;
    currentTime.tm_min = rtcTime.tm_min;
    currentTime.tm_sec = rtcTime.tm_sec;
    uint32_t selTime = static_cast<uint32_t>(mktime(&currentTime));

    std::cerr << "Current RTC time is "
              << +(rtcTime.tm_year + 1900) << "-"
              << +(rtcTime.tm_mon + 1) << "-"
              << +(rtcTime.tm_mday) << ", "
              << +(rtcTime.tm_hour) << ":"
              << +(rtcTime.tm_min) << ":"
              << +(rtcTime.tm_sec) << std::endl;
    return selTime;
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

int main(int argc, char *argv[])
{
    time_t bmcTimeSeconds = 0;

    if(!getBmcTime(bmcTimeSeconds))
    {
        std::cerr <<"Failed to get BMC Time" << std::endl;
        return 0;
    }

    uint32_t selTime =  rtc_read_time();

    timespec setTime;
    setTime.tv_sec = selTime;
    if (std::abs(setTime.tv_sec - bmcTimeSeconds) > timeDiffAllowedSecond)
    {
        if(clock_settime(CLOCK_REALTIME, &setTime) < 0)
        {
            std::cerr <<"Failed to set SEL Time" << std::endl;
        }
    }

    return 0;
}