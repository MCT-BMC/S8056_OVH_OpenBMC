#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <gpiod.h>
#include <openbmc/libobmcdbus.hpp>
#include <openbmc/libmisc.h>

#include "cpld-updater.hpp"

constexpr auto discretePathPrefix = "/xyz/openbmc_project/sensors/discrete/";

void usage ()
{
    std::cout << "Usage: cpld-updater [file name] : update cpld firmware\n";
}

int main(int argc, char **argv)
{

    if (argc != 2) {
        usage();
        return -1;
    }

    if (access(argv[1], F_OK) < 0)
    {
        std::cerr << "Missing CPLD FW file.\n";
        return -1;
    }

    struct stat buf;

    if (stat(argv[1], &buf) < 0)
    {
        std::cerr << "Failed to get CPLD FW file info.\n";
        return -1;
    }

    if (!S_ISREG(buf.st_mode))
    {
        std::cerr << "Invalid CPLD FW file type.\n";
        return -1;
    }

    int ret = -1;
    ret = Sem_Acquired(SEM_CPLDUPDATELOCK);
    if (ret < 0)
    {
        printf("semaphor acquired error : ret : %d \n", ret);
        return -1;
    }

    CpldUpdateManager cpld_updater;

    std::string cpld_path = argv[1];

    /*  JTAG MUX OE#, its default is high.No connect
        H:Disable (Default)
        L:Enable */
    gpiod_ctxless_set_value("gpiochip0", gpioJtagBmcOeN, 0, false, "JTAG_BMC_OE_N" ,NULL, NULL);     

    /*  JTAG MUX select
        0: BMC JTAG To CPU (default)
        1: BMC JTAG To CPLD */
    gpiod_ctxless_set_value("gpiochip0", gpioBmcJtagSel, 1, false, "BMC_JTAG_SEL", NULL, NULL);  

    std::string message = "CPLD start to update\n";
    std::string objPath = discretePathPrefix + std::string("Version_Change");

    /*  Event Data 1:
        01h - Firmware or software change detected with associated Entity.
        Event Data 2:
        82h - CPLD firmware change
        Event Data 3:
        07h - oob REST */
    std::vector<uint8_t> eventData = {0x1, 0x82, 0x7};
    ipmiSelAdd(message, objPath, eventData, true);

    // Open jtag device
    ret = cpld_updater.cpldUpdateDevOpen();
    if (ret < 0)
    {
        std::cerr << "Failed to open jtag device.\n";
        cpld_updater.cpldUpdateCloseAll();
        goto error_handler;
    }

    // Check CPLD ID
    ret = cpld_updater.cpldUpdateCheckId();
    if (ret < 0)
    {
        std::cerr << "Failed to check CPLD ID.\n";
        cpld_updater.cpldUpdateCloseAll();
        goto error_handler;
    }

    // Open CPLD FW File
    ret = cpld_updater.cpldUpdateFileOpen(cpld_path);
    if (ret < 0)
    {
        std::cerr << "Failed to open CPLD FW file [" << cpld_path.c_str() <<"]\n";
        cpld_updater.cpldUpdateCloseAll();
        goto error_handler;
    }

    // Get Update Data Size
    ret = cpld_updater.cpldUpdateGetUpdateDataSize();
    if (ret < 0)
    {
        std::cerr << "Failed to get update data size.\n";
        cpld_updater.cpldUpdateCloseAll();
        goto error_handler;
    }

    // Get Update File Parsed
    ret = cpld_updater.cpldUpdateJedFileParser();
    if (ret < 0)
    {
        std::cerr << "Failed to parse CPLD JED File.\n";
        cpld_updater.cpldUpdateCloseAll();
        goto error_handler;
    }

    // Enter transparent mode
    ret = cpld_updater.cpldUpdateCpldStart();
    if (ret < 0)
    {
        std::cerr << "Failed to enter Transparent mode.\n";
        cpld_updater.cpldUpdateCloseAll();
        goto error_handler;
    }

    // Erase stage
    std::cout << "Erasing...\n";
    ret = cpld_updater.cpldUpdateCpldErase();
    if (ret < 0)
    {
        std::cerr << "Failed in the Erase stage.\n";
        cpld_updater.cpldUpdateCloseAll();
        goto error_handler;
    }

    // Program stage
    std::cout << "Programming...\n";
    ret = cpld_updater.cpldUpdateCpldProgram();
    if (ret < 0)
    {
        std::cerr << "Failed in the Program stage.\n";
        cpld_updater.cpldUpdateCloseAll();
        goto error_handler;
    }

    // Verify stage
    std::cout << "Verifying...\n";
    ret = cpld_updater.cpldUpdateCpldVerify();
    if (ret < 0)
    {
        std::cerr << "Failed in the Verify stage.\n";
        cpld_updater.cpldUpdateCloseAll();
        goto error_handler;
    }

    // Exit transparent mode
    ret = cpld_updater.cpldUpdateCpldEnd();
    if (ret < 0)
    {
        std::cerr << "Failed to exit Transparent mode.\n";
        cpld_updater.cpldUpdateCloseAll();
        goto error_handler;
    }

    cpld_updater.cpldUpdateCloseAll();

    /*  Event Data 1:
        07h - Software or F/W Change detected with associated Entity was successful.
    */
    eventData.at(0) = 0x7;
    ipmiSelAdd(message, objPath, eventData, true);

    std::cout << "=== CPLD FW update finished ===\n";
error_handler:

    gpiod_ctxless_set_value("gpiochip0", gpioJtagBmcOeN, 1, false, "JTAG_BMC_OE_N" ,NULL, NULL);     
    gpiod_ctxless_set_value("gpiochip0", gpioBmcJtagSel, 0, false, "BMC_JTAG_SEL", NULL, NULL);
    ret = Sem_Released(SEM_CPLDUPDATELOCK);
    if (ret < 0)
    {
        printf("semaphor release error : %s, ret : %d \n", __func__, ret);
        return ret;
    }

    return 0;
}
