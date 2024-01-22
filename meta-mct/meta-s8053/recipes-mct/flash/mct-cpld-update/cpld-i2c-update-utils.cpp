#include <iostream>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <iomanip>
#include <cstring>

#include <cpld-i2c-update-utils.hpp>

static constexpr bool DEBUG = false;

CpldUpdateManager::CpldUpdateManager(uint8_t bus, uint8_t addr, std::string path) :
    bus(bus), addr(addr), jedFilePath(path)
{
    fwInfo = {0};
}

CpldUpdateManager::~CpldUpdateManager()
{
}


bool CpldUpdateManager::startWith(const char *str, const char *ptn)
{
    int len = std::strlen(ptn);

    for (int i = 0; i < len; i++)
    {
        if ( str[i] != ptn[i] )
        {
            return false;
        }
    }

    return true;
}

int CpldUpdateManager::indexof(const char *str, const char *ptn)
{
    char *ptr = std::strstr(const_cast<char*>(str), ptn);
    int index = 0;

    if (ptr != NULL)
    {
        index = ptr - str;
    }
    else
    {
        index = -1;
    }

    return index;
}

int CpldUpdateManager::shiftData(char *data, int len, std::vector<uint8_t> *cpldData)
{
    int resultIdx = 0;
    int dataIdx = 7;
    uint8_t byteData = 0x0;

    for (int i = 0; i < len; i++)
    {
        data[i] = data[i] - 0x30;
        byteData |= ((unsigned char)data[i] << dataIdx);
        dataIdx--;

        if (0 == ((i + 1) % 8))
        {
            cpldData->emplace_back(byteData);
            byteData = 0x0;
            dataIdx = 7;
            resultIdx++;
        }
    }

    return 0;
}

uint8_t CpldUpdateManager::reverse_bit(uint8_t b) {
    b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
    b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
    b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
    return b;
}

int8_t CpldUpdateManager::jedFileParser()
{
    bool CFStart = false;
    bool UFMStart = false;
    bool VersionStart = false;
    bool ChecksumStart = false;

    /* data + \r\n */
    int readLineSize = LatticeColSize + 2;
    char data_buf[LatticeColSize];
    int copySize = 0;

    std::string line;
    std::ifstream ifs(jedFilePath, std::ifstream::in);
    if (!ifs.good())
    {
        std::cerr << "Error in " << __FUNCTION__ << " in " << __LINE__ << "\n";
        return -1;
    }


    // Parsing JED file
    while(getline(ifs, line))
    {
        if (startWith(line.c_str(), TAG_QF) == true)
        {
            copySize = indexof(line.c_str(), "*") - indexof(line.c_str(), "F") - 1;
            if (copySize < 0)
            {
                std::cerr << "Error in " << __FUNCTION__ << " in " << __LINE__ << "\n";
                ifs.close();
                return -1;
            }

            std::memset(data_buf, 0, sizeof(data_buf));
            std::memcpy(data_buf, &line.c_str()[2], copySize);
            fwInfo.QF = std::atol(data_buf);

            std::printf("QF Size = %ld\n", fwInfo.QF);
        }
        else if (startWith(line.c_str(), TAG_CF_START) == true)
        {
            CFStart = true;
        }
        else if (startWith(line.c_str(), TAG_UFM) == true)
        {
            UFMStart = true;
        }
        else if (startWith(line.c_str(), TAG_USERCODE) == true)
        {
            VersionStart = true;
        }
        else if (startWith(line.c_str(), TAG_CHECKSUM) == true)
        {
            ChecksumStart = true;
        }

        if (CFStart == true)
        {
            if ((startWith(line.c_str(), TAG_CF_START) == false) &&
                    (std::strlen(line.c_str()) != 1))
            {
                if ((startWith(line.c_str(), "0") == true) ||
                        (startWith(line.c_str(), "1") == true))
                {
                    std::memset(data_buf, 0, sizeof(data_buf));
                    std::memcpy(data_buf, line.c_str(), LatticeColSize);

                    // Convert string to byte data
                    shiftData(data_buf, LatticeColSize, &(fwInfo.cfgData));
                }
                else
                {   
                    std::cerr << "CF Size = " << fwInfo.cfgData.size() << "\n";
                    CFStart = false;
                }
            }
        }
        else if ((ChecksumStart == true) &&
                 (std::strlen(line.c_str()) != 1))
        {
            ChecksumStart = false;

            copySize = indexof(line.c_str(), "*") - indexof(line.c_str(), "C") - 1;
            if (copySize < 0)
            {
                std::cerr << "Error in " << __FUNCTION__ << " in " << __LINE__ << "\n";
                ifs.close();
                return -1;
            }

            std::memset(data_buf, 0, sizeof(data_buf));
            std::memcpy(data_buf, &line.c_str()[1], copySize);
            fwInfo.CheckSum = std::strtoul(data_buf, NULL, 16);

            std::cerr << "Checksum = 0x" << std::hex << fwInfo.CheckSum << "\n";
        }
        else if (VersionStart == true)
        {
            if ((startWith(line.c_str(), TAG_USERCODE) == false) &&
                    (std::strlen(line.c_str()) != 1))
            {
                VersionStart = false;

                if (startWith(line.c_str(), "UH") == true)
                {
                    copySize = indexof(line.c_str(), "*") - indexof(line.c_str(), "H") - 1;
                    if (copySize < 0)
                    {
                        std::cerr << "Error in " << __FUNCTION__ << " in " << __LINE__ << "\n";
                        ifs.close();
                        return -1;
                    }

                    std::memset(data_buf, 0, sizeof(data_buf));
                    std::memcpy(data_buf, &line.c_str()[2], copySize);
                    fwInfo.Version = strtoul(data_buf, NULL, 16);

                    std::cerr << "UserCode = 0x" << std::hex << fwInfo.Version << "\n";
                }
                else if (startWith(line.c_str(), "U") == true)
                {
                    copySize = indexof(line.c_str(), "*") - indexof(line.c_str(), "U") - 1;
                    if (copySize < 0)
                    {
                        std::cerr << "Error in " << __FUNCTION__ << " in " << __LINE__ << "\n";
                        ifs.close();
                        return -1;
                    }

                    std::memset(data_buf, 0, sizeof(data_buf));
                    std::memcpy(data_buf, &line.c_str()[2], copySize);
                    fwInfo.Version = strtoul(data_buf, NULL, 2);

                    std::cerr << "UserCode = 0x" << std::hex << fwInfo.Version << "\n";
                }
            }
        }
        else if (UFMStart == true)
        {
            if ((startWith(line.c_str(), TAG_UFM) == false) &&
                    (startWith(line.c_str(), "L") == false) &&
                    (std::strlen(line.c_str()) != 1))
            {
                if ((startWith(line.c_str(), "0") == true) ||
                        (startWith(line.c_str(), "1") == true))
                {
                    std::memset(data_buf, 0, sizeof(data_buf));
                    std::memcpy(data_buf, line.c_str(), LatticeColSize);

                    // Convert string to byte data
                    shiftData(data_buf, LatticeColSize, &(fwInfo.ufmData));
                }
                else
                {
                    std::cerr << "UFM size = " << fwInfo.ufmData.size() << "\n";
                    UFMStart = false;
                }
            }
        }
    }

    // Compute check sum
    unsigned int jedFileCheckSum = 0;
    for (unsigned i = 0; i < fwInfo.cfgData.size(); i ++)
    {
        jedFileCheckSum += reverse_bit(fwInfo.cfgData.at(i));
    }

    jedFileCheckSum = jedFileCheckSum & 0xffff;


    if ((fwInfo.CheckSum != jedFileCheckSum) ||
            (fwInfo.CheckSum == 0))
    {
        std::cerr << "CPLD JED File CheckSum Error - " << std::hex << jedFileCheckSum << "\n";
        ifs.close();
        return -1;
    }

    std::cerr << "JED File CheckSum = 0x" << std::hex << jedFileCheckSum << "\n";

    ifs.close();
    return 0;
}

int8_t CpldUpdateManager::getJedFileUserCode(unsigned int& userCode)
{
    userCode = fwInfo.Version;
    return 0;
}

int8_t CpldUpdateManager::i2cWriteCmd(std::vector<uint8_t> cmdData)
{
    int res = -1;
    std::vector<char> filename;
    filename.assign(20, 0);

    int fd = open_i2c_dev(bus, filename.data(), filename.size(), 0);
    if (fd < 0)
    {
        std::cerr << "Fail to open I2C device: " << unsigned(bus) << "\n";
        return -1;
    }

    res = i2c_master_write(fd, addr, cmdData.size(), cmdData.data());
    if (res < 0)
    {
        std::cerr << "Fail to r/w I2C device: " << unsigned(bus)
                  << ", Addr: " << unsigned(addr) << "\n";
        close_i2c_dev(fd);
        return -1;
    }

    close_i2c_dev(fd);
    return 0;
}

int8_t CpldUpdateManager::i2cWriteReadCmd(std::vector<uint8_t> cmdData,
        std::vector<uint8_t> *readData)
{
    int res = -1;
    std::vector<char> filename;
    filename.assign(20, 0);

    int fd = open_i2c_dev(bus, filename.data(), filename.size(), 0);
    if (fd < 0)
    {
        std::cerr << "Fail to open I2C device: " << unsigned(bus) << "\n";
        return -1;
    }

    std::vector<uint8_t> resData;
    resData.assign(readData->size(), 0);

    res = i2c_master_write_read(fd, addr, cmdData.size(), cmdData.data(),
                                resData.size(), resData.data());
    if (res < 0)
    {
        std::cerr << "Fail to r/w I2C device: " << unsigned(bus)
                  << ", Addr: " << unsigned(addr) << "\n";
        close_i2c_dev(fd);
        return -1;
    }

    *readData = resData;
    close_i2c_dev(fd);
    return 0;
}

int8_t CpldUpdateManager::readDeviceId()
{
    std::vector<uint8_t> cmd = {CMD_READ_DEVICE_ID, 0x0, 0x0, 0x0};
    std::vector<uint8_t> readData;
    uint8_t resSize = 4;

    readData.assign(resSize, 0);

    int ret = i2cWriteReadCmd(cmd, &readData);
    if (ret < 0)
    {
        return -1;
    }

    std::cerr << "Device ID = ";
    for (int i = 0; i < readData.size(); i++)
    {
        std::cerr << std::hex << std::setfill('0') << std::setw(2)
                  << unsigned(readData.at(i)) << " ";
    }
    std::cerr << "\n";
    return 0;
}

int8_t CpldUpdateManager::enableProgramMode()
{
    std::vector<uint8_t> cmd = {CMD_ENABLE_CONFIG_MODE, 0x8, 0x0};

    int ret = i2cWriteCmd(cmd);
    if (ret < 0)
    {
        return -1;
    }

    if (!waitBusyAndVerify())
    {
        std::cerr << "Wait busy and verify fail\n";
        return -1;
    }

    return 0;
}

int8_t CpldUpdateManager::eraseFlash()
{
    std::vector<uint8_t> cmd = {CMD_ERASE_FLASH, 0xC, 0x0, 0x0};

    if (!waitBusyAndVerify())
    {
        std::cerr << "Wait busy and verify fail before erase\n";
        return -1;
    }

    int ret = i2cWriteCmd(cmd);
    if (ret < 0)
    {
        return -1;
    }

    if (!waitBusyAndVerify())
    {
        std::cerr << "Wait busy and verify fail\n";
        std::cerr << "Disable config interface.\n";
        ret = disableConfigInterface();
        if (ret < 0)
        {
            std::cerr << "Disable Config Interface fail. ret =" << ret << "\n";
            std::cerr << "Please do not AC system and wait for FA.\n";
            return -1;
        }
        return -1;
    }

    return 0;
}

int8_t CpldUpdateManager::resetConfigFlash()
{
    std::vector<uint8_t> cmd = {CMD_RESET_CONFIG_FLASH, 0x0, 0x0, 0x0};
    if constexpr (DEBUG)
    {
        std::cerr << "Reset config flash.\n";
    }
    int ret = i2cWriteCmd(cmd);
    if (ret < 0)
    {
        std::cerr << "Reset config flash fail. ret =" << ret << "\n";
        return -1;
    }

    return 0;
}

int8_t CpldUpdateManager::resetUserFlash()
{
    std::vector<uint8_t> cmd = {CMD_RESET_USER_FLASH, 0x0, 0x0, 0x0};
    if constexpr (DEBUG)
    {
        std::cerr << "Reset user flash.\n";
    }
    int ret = i2cWriteCmd(cmd);
    if (ret < 0)
    {
        std::cerr << "Reset user flash fail. ret =" << ret << "\n";
        return -1;
    }

    return 0;
}


int8_t CpldUpdateManager::writeProgramPage()
{
    std::vector<uint8_t> cmd = {CMD_PROGRAM_PAGE, 0x0, 0x0, 0x0};
    uint8_t iterSize = 16;

    if (resetConfigFlash() < 0)
    {
        return -1;
    }

    for (unsigned i = 0; i < fwInfo.cfgData.size(); i += iterSize)
    {

        double progressRate = ((double(i) / double(fwInfo.cfgData.size())) * 100);
        std::cout << "Update :" << std::fixed << std::dec
                  << std::setprecision(2) << progressRate << "% \r";

        uint8_t len = ((i + iterSize) < fwInfo.cfgData.size())
                      ? iterSize : (fwInfo.cfgData.size() - i);
        std::vector<uint8_t> data = cmd;
        std::vector<uint8_t>::iterator dataIter = data.end();

        data.insert(dataIter, fwInfo.cfgData.begin() + i, fwInfo.cfgData.begin() + i + len);

        int ret = i2cWriteCmd(data);
        if (ret < 0)
        {
            return -1;
        }

        if (!waitBusyAndVerify())
        {
            std::cerr << "Wait busy and verify fail\n";
            return -1;
        }

        data.clear();
    }

#ifdef CPLD_I2C_UFM_MODE
    if (resetUserFlash() < 0)
    {
        return -1;
    }

    for (unsigned i = 0; i < fwInfo.ufmData.size(); i += iterSize)
    {

        double progressRate = ((double(i) / double(fwInfo.ufmData.size())) * 100);
        std::cout << "Update :" << std::fixed << std::dec
                  << std::setprecision(2) << progressRate << "% \r";

        uint8_t len = ((i + iterSize) < fwInfo.ufmData.size())
                      ? iterSize : (fwInfo.ufmData.size() - i);
        std::vector<uint8_t> data = cmd;
        std::vector<uint8_t>::iterator dataIter = data.end();

        data.insert(dataIter, fwInfo.ufmData.begin() + i, fwInfo.ufmData.begin() + i + len);

        int ret = i2cWriteCmd(data);
        if (ret < 0)
        {
            return -1;
        }

        if (!waitBusyAndVerify())
        {
            std::cerr << "Wait busy and verify fail\n";
            return -1;
        }

        data.clear();
    }
#endif

    return 0;
}

int8_t CpldUpdateManager::verifyProgramPage() {
    std::vector<uint8_t> cmd = {CMD_READ_INCR_NV, 0x00, 0x00, 0x01};
    uint8_t cmdSize = cmd.size();
    uint8_t iterSize = 16;

    if (resetConfigFlash() < 0)
    {
        return -1;
    }

    for (unsigned i = 0; i < fwInfo.cfgData.size(); i += iterSize)
    {

        double progressRate = ((double(i) / double(fwInfo.cfgData.size())) * 100);
        std::cout << "Verify :" << std::fixed << std::dec
                  << std::setprecision(2) << progressRate << "% \r";

        uint8_t len = ((i + iterSize) < fwInfo.cfgData.size())
                      ? iterSize : (fwInfo.cfgData.size() - i);
        std::vector<uint8_t> data = cmd;
        std::vector<uint8_t>::iterator dataIter = data.end();

        data.insert(dataIter, fwInfo.cfgData.begin() + i, fwInfo.cfgData.begin() + i + len);

        std::vector<uint8_t> readData;
        readData.assign(iterSize, 0);
        int ret = i2cWriteReadCmd(cmd, &readData);
        if (ret < 0)
        {
            return -1;
        }

        for(int i=0; i < cmdSize; i++)
        {
            data.erase(data.begin());
        }

        if constexpr (DEBUG)
        {
            std::printf( "Compare data: ");
            for(int i=0; i < data.size(); i++) 
            {
                std::printf("%02X ",data[i]);
            }
            std::printf("\n");

            std::printf( "Read data: ");
            for(int i=0; i < readData.size(); i++) 
            {
                std::printf("%02X ",readData[i]);
            }
            std::printf("\n");
        }

        if(data != readData)
        {
            std::cerr << "Data verify fail\n";
            return -1;
        }

        if (!waitBusyAndVerify())
        {
            std::cerr << "Wait busy and verify fail\n";
            return -1;
        }

        data.clear();
    }

    return 0;
}

int8_t CpldUpdateManager::writeUserCode()
{
    std::vector<uint8_t> cmd = {CMD_PROGRAM_USERCODE, 0x0, 0x0, 0x0};

    cmd.insert(cmd.end(), fwInfo.Version >> 24);
    cmd.insert(cmd.end(), fwInfo.Version >> 16);
    cmd.insert(cmd.end(), fwInfo.Version >> 8);
    cmd.insert(cmd.end(), fwInfo.Version);

    if (resetConfigFlash() < 0)
    {
        return -1;
    }

    int ret = i2cWriteCmd(cmd);
    if (ret < 0)
    {
        return -1;
    }

    if (!waitBusyAndVerify())
    {
        std::cerr << "Wait busy and verify fail\n";
        return -1;
    }

    return 0;
}

int8_t CpldUpdateManager::verifyUserCode()
{
    bool verifyFailed = false;
    std::vector<uint8_t> cmd = {CMD_READ_USERCODE, 0x0, 0x0, 0x0};
    std::vector<uint8_t> readData;
    uint8_t readSize = 4;
    readData.assign(readSize, 0);

    if (resetConfigFlash() < 0)
    {
        return -1;
    }

    int ret = i2cWriteReadCmd(cmd, &readData);
    if (ret < 0)
    {
        return -1;
    }

    std::printf( "Read user code: ");
    for(int i=0; i < readData.size(); i++) {
        std::printf("%02X ",readData[i]);
        if(readData[i] != (fwInfo.Version >> ((readData.size() - i - 1)*8)))
        {
            verifyFailed = true;
        }
    }
    std::printf("\n");

    if(verifyFailed)
    {
        return -1;
    }

    if (!waitBusyAndVerify())
    {
        std::cerr << "Wait busy and verify fail\n";
        return -1;
    }

    return 0;
}

int8_t CpldUpdateManager::programDone()
{
    std::vector<uint8_t> cmd = {CMD_PROGRAM_DONE, 0x0, 0x0, 0x0};

    if (resetConfigFlash() < 0)
    {
        return -1;
    }

    int ret = i2cWriteCmd(cmd);
    if (ret < 0)
    {
        return -1;
    }

    if (!waitBusyAndVerify())
    {
        std::cerr << "Wait busy and verify fail\n";
        return -1;
    }

    return 0;
}

int8_t CpldUpdateManager::disableConfigInterface()
{
    std::vector<uint8_t> cmd = {CMD_DISALBE_CONFIG_INTERFACE, 0x0, 0x0};

    int ret = i2cWriteCmd(cmd);
    if (ret < 0)
    {
        return -1;
    }

    return 0;
}

bool CpldUpdateManager::waitBusyAndVerify()
{
    uint8_t retry = 0;
    int8_t ret = 0;

    // Check busy flag
    while (retry <= maxRetry)
    {
        uint8_t busyFlag = 0xff;

        ret = readBusyFlag(&busyFlag);
        if (ret < 0)
        {
            std::cerr << "Fail to read busy flag. ret = "
                      << unsigned(ret) << "\n";
            return false;
        }

        if (busyFlag & busyFlagBit)
        {
            usleep(waitBusyTime);
            retry++;
            if (retry > maxRetry)
            {
                std::cerr << "Busy flag Reg : " << std::to_string(busyFlag)  << " Busy!\n";
                return false;
            }
        }
        else
        {
            break;
        }
    }

    retry = 0;

    // Check status register
    while (retry <= maxRetry)
    {
        uint8_t statusReg = 0xff;

        ret = readStatusReg(&statusReg);
        if (ret < 0)
        {
            std::cerr << "Fail to read status register. ret = "
                      << unsigned(ret) << "\n";
            return false;
        }

        if (statusReg & statusRegFail)
        {
            std::cout << "Status Reg : Fail!\n";
            return false;
        }
        else if (statusReg & statusRegBusy)
        {
            usleep(waitBusyTime);
            retry++;
            if (retry > maxRetry)
            {
                std::cerr << "Status Reg : " << std::to_string(statusRegBusy) << " Busy!\n";
                return false;
            }
        }
        else
        {
            break;
        }
    }

    return true;
}

int8_t CpldUpdateManager::readBusyFlag(uint8_t *busyFlag)
{
    std::vector<uint8_t> cmd = {CMD_READ_BUSY_FLAG, 0x0, 0x0, 0x0};
    std::vector<uint8_t> readData;
    uint8_t resSize = 1;

    readData.assign(resSize, 0);

    int ret = i2cWriteReadCmd(cmd, &readData);
    if (ret < 0)
    {
        return -1;
    }

    if (readData.size() != resSize)
    {
        return - 1;
    }
    else
    {
        *busyFlag = readData.at(0);
    }
    return 0;
}

int8_t CpldUpdateManager::readStatusReg(uint8_t *statusReg)
{
    std::vector<uint8_t> cmd = {CMD_READ_STATUS_REG, 0x0, 0x0, 0x0};
    std::vector<uint8_t> readData;
    uint8_t resSize = 4;
    readData.assign(resSize, 0);

    int ret = i2cWriteReadCmd(cmd, &readData);
    if (ret < 0)
    {
        return -1;
    }

    if (readData.size() != resSize)
    {
        return - 1;
    }
    else
    {
        *statusReg = readData.at(2);
    }
    return 0;
}

int8_t CpldUpdateManager::fwUpdate()
{
    int ret = -1;

    ret = readDeviceId();
    if (ret < 0)
    {
        std::cout << "Read device Id fail. ret = " << ret << "\n";
        return -1;
    }

    std::cerr << "Starts to update ...\n";
    std::cerr << "Enable program mode.\n";

    ret = enableProgramMode();
    if (ret < 0)
    {
        std::cout << "Enable program mode. ret = " << ret << "\n";
        return -1;
    }

    std::cerr << "Erase flash.\n";
    ret = eraseFlash();
    if (ret < 0)
    {
        std::cerr << "Erase flash fail. ret =" << ret << "\n";
        return -1;
    }

    std::cerr << "Write Program Page ...\n";
    ret = writeProgramPage();
    if (ret < 0)
    {
        std::cerr << "Write program page fail. ret =" << ret << "\n";
        return -1;
    }

    std::cerr << "Write User Code ...\n";
    ret = writeUserCode();
    if (ret < 0)
    {
        std::cerr << "Write user codr fail. ret =" << ret << "\n";
        return -1;
    }

    return 0;
}

int8_t CpldUpdateManager::fwVerify()
{
    int ret = -1;

    std::cerr << "Verify User Code ...\n";
    ret = verifyUserCode();
    if (ret < 0)
    {
        std::cerr << "Verify user codr fail. ret =" << ret << "\n";
        return -1;
    }

    std::cerr << "Verify Program Page ...\n";
    ret = verifyProgramPage();
    if (ret < 0)
    {
        std::cerr << "Verify program page fail. ret =" << ret << "\n";
        return -1;
    }

    return 0;
}

int8_t CpldUpdateManager::fwDone()
{
    int ret = -1;

    std::cerr << "Write Program Page Done.\n";

    std::cerr << "Program done.\n";
    ret = programDone();
    if (ret < 0)
    {
        std::cerr << "Program fone fail. ret =" << ret << "\n";
        return -1;
    }

    std::cerr << "Disable config interface.\n";
    ret = disableConfigInterface();
    if (ret < 0)
    {
        std::cerr << "Disable Config Interface fail. ret =" << ret << "\n";
        return -1;
    }

    std::cerr << "Update completed!\n";

    return 0;
}