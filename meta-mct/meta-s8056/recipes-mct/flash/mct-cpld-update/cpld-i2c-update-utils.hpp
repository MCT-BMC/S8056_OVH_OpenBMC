#pragma once

#include <stdint.h>
#include <vector>
#include <string>
#include <fstream>
#include <openbmc/libobmci2c.h>

constexpr uint8_t maxRetry = 60;
constexpr uint8_t busyFlagBit = 0x80;
constexpr uint8_t statusRegBusy = 0x10;
constexpr uint8_t statusRegFail = 0x20;
constexpr unsigned long waitBusyTime = 500000; //ms

constexpr unsigned int LatticeInsLength = 0x08;
constexpr int LatticeColSize = 128; // Bits

constexpr char TAG_QF[] = "QF";
constexpr char TAG_CF_START[] = "L000";
constexpr char TAG_UFM[] = "NOTE TAG DATA";
constexpr char TAG_ROW[] = "NOTE FEATURE";
constexpr char TAG_CHECKSUM[] = "C";
constexpr char TAG_USERCODE[] = "NOTE User Electronic";

enum cpldCmd
{
    CMD_ERASE_FLASH = 0x0E,
    CMD_DISALBE_CONFIG_INTERFACE = 0x26,
    CMD_READ_STATUS_REG = 0x3C,
    CMD_RESET_CONFIG_FLASH = 0x46,
    CMD_RESET_USER_FLASH = 0x47,
    CMD_PROGRAM_DONE = 0x5E,
    CMD_PROGRAM_PAGE = 0x70,
    CMD_READ_INCR_NV = 0x73,
    CMD_ENABLE_CONFIG_MODE = 0x74,
    CMD_READ_USERCODE = 0xC0,
    CMD_PROGRAM_USERCODE = 0xC2,
    CMD_READ_DEVICE_ID = 0xE0,
    CMD_READ_BUSY_FLAG = 0xF0,
};

typedef struct
{
  unsigned long int QF;
  unsigned int *UFM;
  unsigned int Version;
  unsigned int CheckSum;
  std::vector<uint8_t> cfgData;
  std::vector<uint8_t> ufmData;
} CPLDInfo;

class CpldUpdateManager
{
public:
    CpldUpdateManager(uint8_t bus, uint8_t addr, std::string path);
    ~CpldUpdateManager();
    uint8_t bus;
    uint8_t addr;
    std::string jedFilePath;
    std::vector<uint8_t> fwData;
    CPLDInfo fwInfo;

    int8_t jedFileParser();
    int8_t fwUpdate();
    int8_t fwVerify();
    int8_t fwDone();
    int8_t getJedFileUserCode(unsigned int& userCode);
private:
    bool startWith(const char *str, const char *ptn);
    int indexof(const char *str, const char *ptn);
    int shiftData(char *data, int len, std::vector<uint8_t> *cpldData);
    uint8_t reverse_bit(uint8_t b);

    int8_t readDeviceId();
    int8_t enableProgramMode();
    int8_t eraseFlash();
    int8_t resetConfigFlash();
    int8_t resetUserFlash();
    int8_t writeProgramPage();
    int8_t writeUserCode();
    int8_t verifyUserCode();
    int8_t verifyProgramPage();
    int8_t programDone();
    int8_t disableConfigInterface();

    bool waitBusyAndVerify();
    int8_t readBusyFlag(uint8_t *busyFlag);
    int8_t readStatusReg(uint8_t *statusReg);

    int8_t i2cWriteCmd(std::vector<uint8_t> cpldCmd);
    int8_t i2cWriteReadCmd(std::vector<uint8_t> cpldCmd,
                           std::vector<uint8_t> *readData);
};
