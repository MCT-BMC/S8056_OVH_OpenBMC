#pragma once

#include <stdint.h>

constexpr uint32_t maxRetry = 5000;

static constexpr int gpioJtagBmcOeN = 2;
static constexpr int gpioBmcJtagSel = 148;

typedef struct
{
  unsigned long int QF;
  unsigned int *CF;
  unsigned int CF_Line;
  unsigned int *UFM;
  unsigned int UFM_Line;
  unsigned int Version;
  unsigned int CheckSum;
  unsigned int FEARBits;
  unsigned int FeatureRow;

} CPLDInfo;

enum : uint8_t
{
  CHECK_BUSY = 0,
  CHECK_STATUS = 1,
};

enum : uint8_t
{
  JTAG_STATE_TLRESET = 0,
  JTAG_STATE_IDLE,
  JTAG_STATE_SELECTDR,
  JTAG_STATE_CAPTUREDR,
  JTAG_STATE_SHIFTDR,
  JTAG_STATE_EXIT1DR,
  JTAG_STATE_PAUSEDR,
  JTAG_STATE_EXIT2DR,
  JTAG_STATE_UPDATEDR,
  JTAG_STATE_SELECTIR,
  JTAG_STATE_CAPTUREIR,
  JTAG_STATE_SHIFTIR,
  JTAG_STATE_EXIT1IR,
  JTAG_STATE_PAUSEIR,
  JTAG_STATE_EXIT2IR,
  JTAG_STATE_UPDATEIR
};

class CpldUpdateManager
{
public:
    CpldUpdateManager();
    ~CpldUpdateManager();

    int cpldUpdateDevOpen();
    void cpldUpdateDevClose();
    int cpldUpdateFileOpen(const std::string& cpld_path);
    void cpldUpdateFileClose();
    void cpldUpdateCloseAll();

    int cpldUpdateCheckId();
    int cpldUpdateGetUpdateDataSize();
    int cpldUpdateJedFileParser();

    int cpldUpdateCpldStart();
    int cpldUpdateCpldErase();
    int cpldUpdateCpldProgram();
    int cpldUpdateCpldVerify();
    int cpldUpdateCpldEnd();

private:
    int jtagFd;
    FILE *cpldFp;
    int cfSize;
    int ufmSize;
    int eraseType;
    CPLDInfo devInfo;

    // Search the index of char in string
    int indexof(char *str, const char *ptn);

    // Identify if the str starts with a specific pattern
    bool startWith(const char *str, const char *ptn);

    // Convert bit data to byte data
    int shiftData(char *data, unsigned int *result, int len);

    // Check device status
    int checkDeviceStatus(uint8_t mode, unsigned int &status);

    int SendCFdata();
    int SendUFMdata();
};
