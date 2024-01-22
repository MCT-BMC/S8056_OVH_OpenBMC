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

enum IcReg
{
    CMD_DEVICE_ID = 0xAD,
    CMD_DEVICE_REV = 0xAE,
};

namespace VrUpdateUtil
{

void update_flash_progress(int pecent, const char *version_id, int progress, std::string additionalInfo="");

std::vector<uint8_t> hexToNumByteArray(const std::string &srcStr);
    
std::string byteArrayToString(std::vector<uint8_t> byteArray);

int8_t writeReadPmbusCmd(uint8_t bus, uint8_t addr, std::vector<uint8_t> writeData, 
                         uint8_t readDateSize, std::vector<uint8_t> *resData);

int8_t writePmbusCmd(uint8_t bus, uint8_t addr, std::vector<uint8_t> writeData);

int8_t sendBlockWriteRead(uint8_t bus, uint8_t addr, uint8_t cmd,
                          std::vector<uint8_t> *rBuf);

int8_t getMfrId(uint8_t bus, uint8_t addr, std::string &mfrId);

int8_t getMfrModel(uint8_t bus, uint8_t addr, std::string &mfrModel);

} // namespace VrUpdateUtil