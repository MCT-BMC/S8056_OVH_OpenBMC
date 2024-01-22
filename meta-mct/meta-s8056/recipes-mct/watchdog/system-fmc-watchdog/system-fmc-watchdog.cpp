#include <iostream>
#include <variant>
#include <openbmc/libmisc.h>
#include <cstdlib>
#include <boost/asio.hpp>

#define FMC_BASE 0x1e620000
#define FMC_WDT2_CTRL 0x64
#define FMC_WDT2_RELOAD_VAL 0x68
#define FMC_WDT2_RESTART 0x6C

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        std::cerr << "Unexpected parameters" << std::endl;
        return 0;
    }

    boost::asio::io_context io;
    static boost::asio::steady_timer waitTimer(io);
    uint32_t registerBufferFMC = 0;

    uint8_t watchdogTimer = atoi(argv[1]);
    uint8_t interval = atoi(argv[2]);

    write_register((FMC_BASE + FMC_WDT2_RELOAD_VAL), watchdogTimer*10);

    while(true){
        write_register((FMC_BASE + FMC_WDT2_CTRL), 0x1);
        waitTimer.expires_from_now(boost::asio::chrono::seconds(interval));
        waitTimer.wait();
        write_register((FMC_BASE + FMC_WDT2_RESTART), 0x4755);
    }

    return 0;
}
