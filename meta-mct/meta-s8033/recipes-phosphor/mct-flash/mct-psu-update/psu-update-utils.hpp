#pragma once

#include <iostream>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <openbmc/libobmci2c.h>

const std::string modeNameDelta = "DELTA";
const std::string modeNameChicony = "CHICONY POWER";

enum PmbusReg
{
    CMD_MFR_ID = 0x99,
    CMD_MFR_MODEL = 0x9A,
    CMD_MFR_DEVICE_CODE = 0xD0,
    CMD_ISP_KEY = 0xD1,
    CMD_ISP_STATUS = 0xD2,
    CMD_ISP_MEMORY = 0xD4,
    CMD_FWUPLOAD_MODE = 0xD6,
    CMD_FWUPLOAD = 0xD7,
    CMD_FWUPLOAD_STATUS = 0xD8,
    CMD_FWUPLOAD_CAPABILITY = 0xF2,
    CMD_FWUPDATE_STATUS = 0xF3,
};

namespace PsuUpdateUtil
{

void update_flash_progress(int pecent, const char *version_id, int progress);

std::vector<uint8_t> hexToNumByteArray(const std::string &srcStr);
    
std::string byteArrayToString(std::vector<uint8_t> byteArray);

int8_t writeReadPmbusCmd(uint8_t bus, uint8_t addr, std::vector<uint8_t> writeData, 
                         uint8_t readDateSize, std::vector<uint8_t> *resData);

int8_t writePmbusCmd(uint8_t bus, uint8_t addr, std::vector<uint8_t> writeData);

int8_t sendBlockWriteRead(uint8_t bus, uint8_t addr, uint8_t cmd,
                          std::vector<uint8_t> *rBuf);

int8_t getMfrId(uint8_t bus, uint8_t addr, std::string &mfrId);

int8_t getMfrModel(uint8_t bus, uint8_t addr, std::string &mfrModel);

} // namespace PsuUpdateUtil