#pragma once

#include <iostream>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <vector>
#include <string>
#include <fstream>
#include <iterator>
#include <openbmc/libobmci2c.h>
#include <psu-update-utils.hpp>

constexpr int psuFwHeaderSize = 32;
constexpr uint8_t psuFwUploadBlockSizeOffset = 0x1c;

constexpr int8_t exitFwUpdateMode = 0;
constexpr int8_t enterFwUpdateMode = 1;
constexpr int8_t delayTimeToSetUploadMode = 2;

constexpr int8_t maxRetry = 5;
constexpr int8_t retryDelay = 1;

constexpr int8_t chiconyDelayAfterUpdate = 2;
constexpr int8_t chiconyMaxRetry = 5;
constexpr int8_t chiconyRetryDelay = 1;

constexpr int8_t delayFinishUpload = 5;
constexpr int8_t delayFinishUpdate = 5;

constexpr int8_t flashHeaderDelayTime = 2;
constexpr int flashBlockDelayTimeUs = 50000;

constexpr uint8_t fullImageReceive = 1 << 0;
constexpr uint8_t BadImageSafeMode = 1 << 2;
constexpr uint8_t BadImage = 1 << 3;
constexpr uint8_t ImageNotSupport = 1 << 4;

constexpr uint8_t monitorBitActiveDelay = 3;

constexpr int chiconyDelayUpdateProcressTimes = 1;
constexpr int chiconyUpdateProcressBase = 30;

class ChiconyPsuUpdateManager
{
public:
    ChiconyPsuUpdateManager(uint8_t bus, uint8_t addr, std::string fwPath,
                            int progress,std::string versionId);
    ~ChiconyPsuUpdateManager();
    uint8_t bus;
    uint8_t addr;
    std::string fwPath;
    std::vector<uint8_t> fwData;
    int progress;
    std::string versionId;
    int delayUpdateProcress;

    int8_t fwUpdate();
private:
    int8_t writeReadPmbusCmd(std::vector<uint8_t> writeData,
                             uint8_t readDateSize, std::vector<uint8_t> *resData);
    int8_t writePmbusCmd(std::vector<uint8_t> writeData);
    int8_t sendBlockWriteRead(uint8_t cmd, std::vector<uint8_t> *rBuf);

    int8_t setFwUploadMode(uint8_t mode);
    int8_t getFwUploadMode(int8_t *mode);
    int8_t getFwUploadStatus(uint16_t *status);

    int8_t enterUploadMode();
    int8_t exitUploadMode();

    int8_t writeDataToPsu(int start, uint8_t size);
    int8_t checkFwUploadStatus(int currentIndex);
    int8_t sendImageToPsu();
};