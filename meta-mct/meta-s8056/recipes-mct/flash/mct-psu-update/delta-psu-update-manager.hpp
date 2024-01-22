#pragma once

#include <iostream>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <vector>
#include <string>
#include <openbmc/libobmci2c.h>
#include <psu-update-utils.hpp>

#include <sstream>
#include <algorithm>
#include <cstdint>
#include <fstream>

constexpr int8_t delayTimeToSetUpMode = 1;

constexpr int8_t deltaPsuFwHeaderSize = 32;

constexpr uint32_t ispKey = 0x496E7372; // ACSII : Insr

constexpr int deltaDelayUpdateProcressTimes = 10;
constexpr int deltaUpdateProcressBase = 30;

enum DeltaIspMode
{
    CMD_CLEAR_STAT = 0x00,
    CMD_RESET_SEQ = 0x01,
    CMD_BOOT_ISP = 0x02,
    CMD_BOOT_PM = 0x03,
};

typedef struct {
    uint8_t byteCount;
    uint8_t address[2];
    uint8_t recordType;
    std::vector<uint8_t> data;
    uint8_t checksum;
}intel_hex_t;

enum intelhexRecordType {
    DATA_RECORD,                 // '00'
    END_OF_FILE_RECORD,          // '01'
    EXTENDED_SEGMENT_ADDRESS,    // '02'
    START_SEGMENT_ADDRESS,       // '03'
    EXTENDED_LINEAR_ADDRESS,     // '04'
    START_LINEAR_ADDRESS,        // '05'
    NO_OF_RECORD_TYPES
};

class DeltaPsuUpdateManager
{
public:
    DeltaPsuUpdateManager(uint8_t bus, uint8_t addr, std::string fwPath, 
                          int progress,std::string versionId);
    ~DeltaPsuUpdateManager();
    uint8_t bus;
    uint8_t addr;
    std::string fwPath;
    std::vector<intel_hex_t> intelHexStore;
    int progress;
    std::string versionId;
    int delayUpdateProcress;

    int8_t fwUpdate();
private:
    int8_t writeReadPmbusCmd(std::vector<uint8_t> writeData,
                             uint8_t readDateSize, std::vector<uint8_t> *resData);
    int8_t writePmbusCmd(std::vector<uint8_t> writeData);
    int8_t sendBlockWriteRead(uint8_t cmd, std::vector<uint8_t> *rBuf);
    int8_t getDeviceCode(std::vector<uint8_t> *deviceCode);
    int8_t setIspKey(uint32_t ispKey);
    int8_t setIspStatus(uint8_t mode);
    int8_t getIspStatus(int8_t *ispStatus);
    int8_t getFwUploadCapability(std::vector<uint8_t> *fwCapability);
    int8_t getFwUpdataStatus(std::vector<uint8_t> *fwUpdatStatus);

    int8_t writeDataToPsu(std::vector<uint8_t> hexData);
    int8_t sendImageToPsu();
};