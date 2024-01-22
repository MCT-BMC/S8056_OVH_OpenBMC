#pragma once

#include <iostream>
#include <vector>
#include <stdlib.h>
#include <stdio.h>
#include <sdbusplus/bus.hpp>
#include <cpld-i2c-update-utils.hpp>
#include <cpld-lattice.hpp>

namespace ipmi
{
static constexpr const char* busName = "xyz.openbmc_project.Logging.IPMI";
static constexpr const char* path = "/xyz/openbmc_project/Logging/IPMI";
static constexpr const char* interface = "xyz.openbmc_project.Logging.IPMI";
static constexpr const char* request = "IpmiSelAdd";
} // namespace ipmi

typedef struct {
    int program;                    /* enable/disable program  */
    int debug;                      /* enable/disable debug flag */
    int progress;                   /* progress update, use the version id */
    int bus;                        /* cpld device i2c bus*/
    int address;                    /* cpld device i2c address*/
    int jed_info;                   /* Read JED file user code flag*/
    unsigned int version;           /* cpld JED file user cocde*/
    char version_id[256];           /* version id */
}cpld_t;

namespace CpldUpdateUtil
{

void update_flash_progress(int pecent, const char *version_id, int progress);

void setUpdateProgress(cpld_t* cpld, char* input, uint8_t base, uint8_t divisor);

void logSelEvent(std::string enrty, std::string path ,
                        uint8_t eventData0, uint8_t eventData1, uint8_t eventData2);

int i2cLatticeCpldFwUpdate(cpld_t& cpld, std::string cpldFilePath);

int i2cLatticeCpldJedFileUserCode(std::string cpldFilePath, unsigned int& userCode);

} // namespace CpldUpdateUtil