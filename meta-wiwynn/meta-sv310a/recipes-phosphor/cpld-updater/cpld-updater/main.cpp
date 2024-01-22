#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <gpiod.h>
#include <openbmc/libobmcdbus.hpp>
#include <openbmc/libmisc.h>

#include "cpld-i2c-updater.hpp"
#include "cpld-updater.hpp"

constexpr auto discretePathPrefix = "/xyz/openbmc_project/sensors/discrete/";

void usage ()
{
    std::cout << "Usage : cpld-updater {update type} [i2c bus] [i2c address] {file path} : update cpld firmware\n";
    std::cout << "        update type : Enter \"--i2c\" or \"--jtag\" \n";
    std::cout << "        If update type is [--i2c], you must enter i2c bus and i2c (7-bit) address\n";
    std::cout << "        file path   : the file path of CPLD firmware.\n";
    std::cout << "Example :\n";
    std::cout << "        Update CPLD via jtag : cpld-updater --jtag cpld_fw.jed\n";
    std::cout << "        Update CPLD via i2c  : cpld-updater --i2c 7 0x40 cpld_fw.jed\n";
}

int main(int argc, char **argv)
{
    if (argc != 3 && argc != 5) {
        usage();
        return -1;
    }

    std::string message = "CPLD start to update\n";
    std::string objPath = discretePathPrefix + std::string("Version_Change");

    std::string updateType = argv[1];
    if (argc == 3 && updateType == "--jtag")
    {
        std::cout << "update CPLD via jtag\n";

        std::string cpld_path = argv[2];

        if (access(cpld_path.c_str(), F_OK) < 0)
        {
            std::cerr << "Missing CPLD FW file.\n";
            return -1;
        }

        struct stat buf;

        if (stat(cpld_path.c_str(), &buf) < 0)
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

        /*  JTAG MUX OE#, its default is high.No connect
            H:Disable (Default)
            L:Enable */
        gpiod_ctxless_set_value("gpiochip0", gpioJtagBmcOeN, 0, false, "JTAG_BMC_OE_N" , NULL, NULL);

        /*  JTAG MUX select
            0: BMC JTAG To CPU (default)
            1: BMC JTAG To CPLD */
        gpiod_ctxless_set_value("gpiochip0", gpioBmcJtagSel, 1, false, "BMC_JTAG_SEL", NULL, NULL);

        /*  Event Data 1:
            A1h - Firmware or software change detected with associated Entity.
            Event Data 2:
            82h - CPLD firmware change
            Event Data 3:
            07h - oob REST */
        std::vector<uint8_t> eventData = {0xA1, 0x82, 0x7};
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
            std::cerr << "Failed to open CPLD FW file [" << cpld_path.c_str() << "]\n";
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
            A7h - Software or F/W Change detected with associated Entity was successful.
        */
        eventData.at(0) = 0xA7;
        ipmiSelAdd(message, objPath, eventData, true);

        std::cout << "=== CPLD FW update finished ===\n";
error_handler:

        gpiod_ctxless_set_value("gpiochip0", gpioJtagBmcOeN, 1, false, "JTAG_BMC_OE_N" , NULL, NULL);
        gpiod_ctxless_set_value("gpiochip0", gpioBmcJtagSel, 0, false, "BMC_JTAG_SEL", NULL, NULL);
        ret = Sem_Released(SEM_CPLDUPDATELOCK);
        if (ret < 0)
        {
            printf("semaphor release error : %s, ret : %d \n", __func__, ret);
            return ret;
        }
    }
    else if (argc == 5 && updateType == "--i2c")
    {
        uint8_t bus = std::strtoul(argv[2], NULL, 0);
        uint8_t addr = std::strtoul(argv[3], NULL, 0);

        std::cout << "update CPLD via i2c\n";

        std::cout << "i2c bus: " << unsigned(bus) << ",i2c addr : " << unsigned(addr) << "\n";

        if (access(argv[4], F_OK) < 0)
        {
            std::cerr << "Missing CPLD CFG file.\n";
            return -1;
        }

        std::string jedFilePath = argv[4];

        CpldI2cUpdateManager cpldpdateManager(bus, addr, jedFilePath);

        int ret = cpldpdateManager.jedFileParser();
        if (ret < 0)
        {
            std::cerr << "JED file parsing failed\n";
            return -1;
        }

        std::string cpldI2cSemaphore = SEM_CPLDUPDATELOCK;
        cpldI2cSemaphore = cpldI2cSemaphore + "_" + std::to_string(bus) + "_" + std::to_string(addr);

        ret = Sem_Acquired(cpldI2cSemaphore.c_str());
        if (ret < 0)
        {
            std::cerr << "Semaphor acquired error. ret = " << ret << "\n";
            return -1;
        }

        /*  Event Data 1:
            A1h - Firmware or software change detected with associated Entity.
            Event Data 2:
            82h - CPLD firmware change
            Event Data 3:
            07h - oob REST */
        std::vector<uint8_t> eventData = {0xA1, 0x82, 0x7};
        ipmiSelAdd(message, objPath, eventData, true);

        ret = cpldpdateManager.fwUpdate();
        if (ret < 0)
        {
            std::cerr << "CPLD update failed\n";

            ret = Sem_Released(cpldI2cSemaphore.c_str());
            if (ret < 0)
            {
                std::cerr << "Semaphor release error. ret = " << ret << "\n";
                return -1;
            }
            return -1;
        }

        /*  Event Data 1:
            A7h - Software or F/W Change detected with associated Entity was successful.
        */
        eventData.at(0) = 0xA7;
        ipmiSelAdd(message, objPath, eventData, true);

        ret = Sem_Released(cpldI2cSemaphore.c_str());
        if (ret < 0)
        {
            std::cerr << "Semaphor release error. ret = " << ret << "\n";
            return ret;
        }
    }
    else
    {
        usage();
        return -1;
    }

    return 0;
}
