#pragma once

#include <iostream>
#include <string>
#include <fcntl.h>
#include <regex>
#include <fstream>
#include <sdbusplus/bus.hpp>
#include <openbmc/libmisc.h>
#include <mtd/mtd-user.h>
#include <openbmc/obmc-i2c.h>
#include <ipmid/utils.hpp>

#include "common-utils.hpp"

extern "C" {
    #include <linux/i2c-dev.h>
    #include <i2c/smbus.h>
}

namespace fs = std::filesystem;

static constexpr bool DEBUG = false;

constexpr auto SPI_MASTER_MODE = 0;
constexpr auto SPI_PASS_THROUGH_MODE = 1;

constexpr auto SPI_DEV = "1e630000.spi";
constexpr auto SPI_PATH = "/sys/bus/platform/drivers/aspeed-smc/";

constexpr auto BIND=1;
constexpr auto UNBIND=0;

constexpr auto HW_STRAP_REGISTER = 0x1E6E2070;
constexpr auto HW_STRAP_CLEAR_REGISTER = 0x1E6E207C;
constexpr auto SPI_MODE_bit_1 = 0x00001000;
constexpr auto SPI_MODE_bit_2 = 0x00002000;

namespace dualBios
{
    static constexpr const char* busName = "xyz.openbmc_project.mct.DualBios";
    static constexpr const char* path = "/xyz/openbmc_project/mct/DualBios";
    static constexpr const char* interface = "xyz.openbmc_project.mct.DualBios";
    static constexpr const char* propCurrentBios = "SetCurrentBootBios";
    static constexpr const char* propBiosSelect = "BiosSelect";
    static constexpr const char* propBiosAction = "BiosAction";
} // namespace dualBios

namespace version
{

static const std::string modeNameDelta = "DELTA";
static const std::string modeNameChicony = "CHICONY POWER";
static const std::string failedToFindFwVersion = "";

enum SystemVersion
{
    ACTIVE_BMC = 0x00,
    BACKUP_BMC = 0x01,
    ACTIVE_BIOS = 0x02,
    BACKUP_BIOS = 0x03,
    DC_SCM_CPLD = 0x04,
    PSU1 = 0x05,
    PSU2 = 0x06,
    MB_CPLD = 0x07,
};

static std::vector<uint8_t> failedToReadFwVersion
{
    0x46, //F
    0x61, //a
    0x69, //i
    0x6C, //l
    0x65, //e
    0x64  //d
};

bool isFailedToReadFwVersion(std::vector<uint8_t> input);

void setSpiMode(int mode);

int setBiosMtdDevice(uint8_t state);

int getCurrentUsingBios(int& currentUsingBios);

int selectUpdateBios(int select);

std::string readMtdDevice(char* mtdDevice, uint64_t seekAddress,
                          uint64_t seekSize, char* versionHead, char* versionEnd);

std::vector<uint8_t> getFirmwareVersion(std::string& version);

std::vector<uint8_t> readActiveBmcVersion();

std::vector<uint8_t> readBackupBmcVersion();

std::vector<uint8_t> readBiosVersion(uint8_t selectBios);

int getRegsInfoByte(uint8_t busId, uint8_t address, uint8_t regs, uint8_t* data);

std::vector<uint8_t> getCpldVersion(uint8_t selectFirmware);

std::vector<uint8_t> sendBlockWriteRead(uint8_t busId, uint8_t slaveAddr, uint8_t cmd);

int8_t getMfrId(uint8_t bus, uint8_t addr, std::string &mfrId);

int8_t getChiconyPsuVersion(uint8_t bus, uint8_t addr, std::vector<uint8_t> *version);

int8_t getDeltaPsuVersion(uint8_t bus, uint8_t addr, std::vector<uint8_t> *version);

std::vector<uint8_t> getPsuVersion(uint8_t select);

} // namespace version