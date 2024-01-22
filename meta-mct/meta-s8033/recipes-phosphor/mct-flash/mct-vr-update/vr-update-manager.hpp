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
#include <vr-update-utils.hpp>

#include <sstream>
#include <algorithm>
#include <cstdint>
#include <fstream>

typedef struct {
    uint8_t header;
    uint8_t byteCount;
    uint8_t pmbusAddress;
    uint8_t pmbusCommand;
    std::vector<uint8_t> data;
    uint8_t checksum;
}vr_hex_t;

enum hexDataType {
    DATA_RECORD = 0x00,
    HEADER_RECORD = 0x49,
    NO_OF_RECORD_TYPES = 0xFF,
};

constexpr int vrUpdateProcressBase = 30;

class VrUpdateManager
{
public:
    VrUpdateManager(uint8_t bus, uint8_t addr, std::string fwPath,
                    uint8_t devcieId, int progress,std::string versionId);
    ~VrUpdateManager();
    uint8_t bus;
    uint8_t addr;
    std::string fwPath;
    std::vector<vr_hex_t> hexStore;
    std::vector<uint8_t> hexDeviceId;
    std::vector<uint8_t> hexDeviceRev;
    uint8_t devcieId;
    int progress;
    std::string versionId;
    int delayUpdateProcress;

    int8_t fwUpdate();
private:
    int8_t writeReadPmbusCmd(std::vector<uint8_t> writeData,
                             uint8_t readDateSize, std::vector<uint8_t> *resData);
    int8_t writePmbusCmd(std::vector<uint8_t> writeData);
    int8_t sendBlockWriteRead(uint8_t cmd, std::vector<uint8_t> *rBuf);
    int8_t getDeviceId(std::vector<uint8_t>* deviceId);
    int8_t getDeviceRev(std::vector<uint8_t>* deviceRev);
    int8_t getAvailableNvmSlot(uint32_t& availableNvmSlot);
    int8_t getPollProgreammerStatus();
    int8_t getCurretVrFwCrc();
    int8_t getBankStatus(uint8_t bank);
    int8_t resetVrDevcie();
    int8_t VR_update_progress();
};