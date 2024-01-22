#include "version-utils.hpp"

namespace version
{

bool isFailedToReadFwVersion(std::vector<uint8_t> input)
{
    if(failedToReadFwVersion.size() != input.size())
    {
        return false;
    }

    for(int i=0; i<input.size(); i++)
    {
        if(failedToReadFwVersion[i] != input[i])
        {
            return false;
        }
    }
    return true;
}

void setSpiMode(int mode)
{
    uint32_t registerBuffer = 0;
    uint32_t acLostFlag = 0x01;

    switch(mode)
    {
        case SPI_MASTER_MODE:
            common::setAst2500Register(HW_STRAP_CLEAR_REGISTER,SPI_MODE_bit_2);
            common::setAst2500Register(HW_STRAP_REGISTER,SPI_MODE_bit_1);
            break;
        case SPI_PASS_THROUGH_MODE:
            common::setAst2500Register(HW_STRAP_REGISTER,(SPI_MODE_bit_1|SPI_MODE_bit_2));
            break;
    }
}

int setBiosMtdDevice(uint8_t state)
{
    std::string spi = SPI_DEV;
    std::string path = SPI_PATH;
    int fd;

    switch (state)
    {
    case UNBIND:
        path = path + "unbind";
        break;
    case BIND:
        path = path + "bind";
        break;
    default:
        std::cerr << "Fail to get state failed" << std::endl;
        return -1;
    }

    fd = open(path.c_str(), O_WRONLY);
    if (fd < 0)
    {
        std::cerr << "Fail in " << __func__ << std::endl;
        return -1;
    }

    write(fd, spi.c_str(), spi.size());
    close(fd);

    return 0;
}

int getCurrentUsingBios(int& currentUsingBios)
{
    auto bus = sdbusplus::bus::new_default_system();

    auto method = bus.new_method_call(dualBios::busName, dualBios::path, PROPERTY_INTERFACE, "Get");
    method.append(dualBios::interface, dualBios::propBiosSelect);
    std::variant<int> result;
    try
    {
        auto reply = bus.call(method);
        reply.read(result);
    }
    catch (const std::exception&  e)
    {
        return -1;
    }

    currentUsingBios = (uint8_t)std::get<int>(result);

    return 0;
}

int selectUpdateBios(int select)
{
    auto bus = sdbusplus::bus::new_default_system();

    auto method = bus.new_method_call(dualBios::busName, dualBios::path,
                                      dualBios::interface, dualBios::propCurrentBios);
    method.append(select,false);
    try
    {
        auto reply=bus.call(method);
    }
    catch (const std::exception&  e)
    {
        return -1;
    }

    return 0;
}

std::string readMtdDevice(char* mtdDevice, uint64_t seekAddress,
                          uint64_t seekSize, char* versionHead, char* versionEnd)
{
    uint64_t maxSeekSize =  0x200000;
    uint64_t setSeekSize =  seekSize;
    mtd_info_t mtd_info;
    unsigned char* read_buf = NULL;
    bool getVersionHead = true;
    std::stringstream version;

    int fd = open(mtdDevice, O_RDWR);
    ioctl(fd, MEMGETINFO, &mtd_info);
    if(seekSize != 0)
    {
        read_buf = (unsigned char*)malloc(seekSize * sizeof(unsigned char));
    }
    else
    {
        if(mtd_info.size > maxSeekSize){
            read_buf = (unsigned char*)malloc(maxSeekSize * sizeof(unsigned char));
            setSeekSize = maxSeekSize;
        }
        else
        {
            read_buf = (unsigned char*)malloc(mtd_info.size * sizeof(unsigned char));
            setSeekSize = mtd_info.size;
        }
    }

    lseek(fd, seekAddress, SEEK_SET);
    read(fd, read_buf, setSeekSize);

    for(int i = 0; i <= setSeekSize; i++)
    {
        if (i == setSeekSize)
        {
            return failedToFindFwVersion;
        }

        int versionCheck = 0;
        if(getVersionHead){
            for(int j = 0; j < strlen(versionHead) ; j++) {
                if(versionHead[j] != read_buf[i+j])
                {
                    break;
                }
                versionCheck++;
            }
            
            if(versionCheck != strlen(versionHead))
            {
                continue;
            }
            getVersionHead = false;
        }

        versionCheck = 0;

        for(int k = 0; k < strlen(versionEnd); k++) {
            if(versionEnd[k] != read_buf[i+k])
            {
                break;
            }
            versionCheck++;
        }

        if(versionCheck == strlen(versionEnd))
        {
            break;
        }
        if constexpr (DEBUG)
        {
            std::cerr << "read_buf[" << i << "]" << std::to_string(read_buf[i]) << std::endl;
        }
        version << read_buf[i];
    }

    std::cerr << version.str() << std::endl;

    free(read_buf);
    close(fd);

    return version.str();
}

std::vector<uint8_t> getFirmwareVersion(std::string& version)
{
    std::vector<uint8_t> formatVersion;

    try {
        version.erase(remove(version.begin(), version.end(), '\"'), version.end());
        version = version.substr(version.find("=") + 1);

        std::regex re("[.-]");
        std::sregex_token_iterator first{version.begin(), version.end(), re, -1}, last;
        std::vector<std::string> versionSub{first, last};

        formatVersion.push_back(std::stoul(versionSub[0].substr(1, versionSub[0].length() - 1), nullptr, 16));
        formatVersion.push_back(std::stoul(versionSub[1], nullptr, 16));
    } catch (std::exception &e) {
        std::cerr << "Get firmware version exception: " << e.what() << std::endl;
        return failedToReadFwVersion;
    }

    return formatVersion;
}

std::vector<uint8_t> readActiveBmcVersion()
{
    std::string version;
    char* versionFilePath = (char*)"/etc/os-release";
    char* versionFileSelect = (char*)"VERSION";

    std::ifstream versionFile("/etc/os-release");

    if (!versionFile.is_open()) {
        return failedToReadFwVersion;
    }

    while (std::getline(versionFile, version)){
        if(version.find(versionFileSelect) != std::string::npos)
        {
            break;
        }
    }

    return getFirmwareVersion(version);
}

std::vector<uint8_t> readBackupBmcVersion()
{
    std::string version;
    char* mtdDevice1 = (char*)"/dev/mtd/bak-u-boot";
    char* versionHead = (char*)"bmc_version=";
    char* versionEnd = (char*)"-s8056";

    version = readMtdDevice(mtdDevice1,0x0,0x0,versionHead,versionEnd);

    if(version.size() == 0)
    {
        return failedToReadFwVersion;
    }

    return getFirmwareVersion(version);
}

std::vector<uint8_t> readBiosVersion(uint8_t selectBios)
{
    char* mtdDevice = (char*)"/dev/mtd/bios";
    char* versionHead = (char*)"$TYN";
    char* versionEnd = (char*)".B10";
    uint64_t seekAddress = 0x00cc9890;

    std::vector<uint8_t> firmwareVersion;
    int currentUsingBios;

    common::setGpioOutput("FM_BMC_READY",1);
    sleep(1);
    common::setGpioOutput("FM_BMC_READY",0);
    sleep(1);
    common::setGpioOutput("SPI_MUX_BIOS",1);
    sleep(0.5);
    getCurrentUsingBios(currentUsingBios);
    selectUpdateBios(selectBios);
    setBiosMtdDevice(BIND);
    sleep(1);

    std::string version = readMtdDevice(mtdDevice,seekAddress,0x0,versionHead,versionEnd);
    if(version.size() != 0)
    {
        version = version.substr(version.find("V")) + std::string(versionEnd);
    }

    setBiosMtdDevice(UNBIND);
    sleep(1);
    selectUpdateBios(currentUsingBios);
    common::setGpioOutput("SPI_MUX_BIOS",0);

    if(version.size() == 0)
    {
        return failedToReadFwVersion;
    }

    for (int i = 0; i < version.length(); i++)
    {
        firmwareVersion.push_back((uint8_t)version[i]);
    }

    return firmwareVersion;
}

int getRegsInfoByte(uint8_t busId, uint8_t address, uint8_t regs, uint8_t* data)
{
    std::string i2cBus = "/dev/i2c-" + std::to_string(busId);
    int fd = open(i2cBus.c_str(), O_RDWR);

    if (fd < 0)
    {
        std::cerr << " unable to open i2c device" << i2cBus << "  err=" << fd
                  << "\n";
        return -1;
    }

    if (ioctl(fd, I2C_SLAVE_FORCE, address) < 0)
    {
        std::cerr << " unable to set device address\n";
        close(fd);
        return -1;
    }

    unsigned long funcs = 0;
    if (ioctl(fd, I2C_FUNCS, &funcs) < 0)
    {
        std::cerr << " not support I2C_FUNCS\n";
        close(fd);
        return -1;
    }

    if (!(funcs & I2C_FUNC_SMBUS_READ_BYTE_DATA))
    {
        std::cerr << " not support I2C_FUNC_SMBUS_READ_BYTE_DATA\n";
        close(fd);
        return -1;
    }

    *data = i2c_smbus_read_byte_data(fd, regs);
    close(fd);

    if (*data < 0)
    {
        return -1;
    }

    return 0;
}

std::vector<uint8_t> getCpldVersion(uint8_t selectFirmware)
{
    uint8_t responseRegMSB,responseRegLSB;
    uint8_t busId, slaveAddr;
    std::string device;
    std::vector<uint8_t> readBuf;

    switch(selectFirmware)
    {
        case version::MB_CPLD:
            device = "MB";
            busId = 0x04;
            slaveAddr = 0x59;
            break;
        case version::DC_SCM_CPLD:
            device = "DC-SCM";
            busId = 0x09;
            slaveAddr = 0x59;
            break;
        default:
            return failedToReadFwVersion;
    }

    if(getRegsInfoByte(busId,slaveAddr,0x01,&responseRegLSB) < 0)
    {
        return failedToReadFwVersion;
    }
    if(getRegsInfoByte(busId,slaveAddr,0x02,&responseRegMSB) < 0)
    {
        return failedToReadFwVersion;
    }

    readBuf.insert(readBuf.end(), responseRegMSB);
    readBuf.insert(readBuf.end(), responseRegLSB);

    if(DEBUG){
        char cpldVersion[100];
        snprintf(&cpldVersion[0], sizeof(cpldVersion), "0x%02x%02x", responseRegMSB, responseRegLSB);  
        std::cerr << device << " CPLD Version : " << cpldVersion << std::endl;
    }

    return readBuf;
}

std::vector<uint8_t> sendBlockWriteRead(uint8_t busId, uint8_t slaveAddr, uint8_t cmd)
{
    uint8_t readCount = 0x10;

    std::vector<uint8_t> readBuf(readCount);
    std::vector<uint8_t> writeDataWithOffset;
    writeDataWithOffset.insert(writeDataWithOffset.begin(), cmd);

    std::string i2cBus =
        "/dev/i2c-" + std::to_string(static_cast<uint8_t>(busId));

    ipmi::i2cWriteRead(i2cBus, static_cast<uint8_t>(slaveAddr),
                                      writeDataWithOffset, readBuf);

    return readBuf;
}

int8_t getMfrId(uint8_t bus, uint8_t addr, std::string &mfrId)
{
    uint8_t CMD_MFR_ID = 0x99;

    std::vector<uint8_t> rBuf = sendBlockWriteRead(bus, addr, CMD_MFR_ID);

    if (rBuf.size() > 1)
    {
        mfrId.assign(rBuf.begin() + 1, rBuf.end());
    }
    else
    {
        std::cerr << ": Get MFR ID size fail\n";
        return -1;
    }

    return 0;
}

int8_t getChiconyPsuVersion(uint8_t bus, uint8_t addr, std::vector<uint8_t> *version)
{
    const uint8_t CMD_VERSION = 0xD9;

    std::vector<uint8_t> rBuf = sendBlockWriteRead(bus, addr, CMD_VERSION);

    if (rBuf.size() <= 1)
    {
        return -1;
    }

    for(int i = 0; i < rBuf[0]; i++)
    {
        version->insert(version->end(),rBuf[i+1]);
    }

    return 0;
}

int8_t getDeltaPsuVersion(uint8_t bus, uint8_t addr, std::vector<uint8_t> *version)
{
    const uint8_t CMD_VERSION = 0x9B;
    const uint8_t BYTE_MAJOR = 0x02;
    const uint8_t BYTE_MINOR = 0x03;
    const uint8_t BYTE_MAINTAIN = 0x04;

    std::vector<uint8_t> rBuf = sendBlockWriteRead(bus, addr, CMD_VERSION);

    if (rBuf.size() <= 1)
    {
        return -1;
    }

    for(int i = 0; i < rBuf[0]; i++)
    {
        switch (i+1)
        {
            case BYTE_MAJOR:
            case BYTE_MINOR:
            case BYTE_MAINTAIN:
                version->insert(version->end(),rBuf[i+1]);
                break;
        }
    }

    return 0;
}

std::vector<uint8_t> getPsuVersion(uint8_t select)
{
    uint8_t psuI2cBus;
    uint8_t psuI2cAddr;
    std::string mfrId;
    std::vector<uint8_t> version;
    int ret;

    switch(select)
    {
        case 1:
            psuI2cBus = 0x03;
            psuI2cAddr = 0x59;
            break;
        case 2:
            psuI2cBus = 0x03;
            psuI2cAddr = 0x58;
            break;
        default:
            return failedToReadFwVersion;
    }

    if (getMfrId(psuI2cBus,psuI2cAddr,mfrId) < 0)
    {
        return failedToReadFwVersion;
    }

    if (mfrId.find(modeNameChicony) != std::string::npos)
    {
        ret = getChiconyPsuVersion(psuI2cBus,psuI2cAddr,&version);
    }
    else if (mfrId.find(modeNameDelta) != std::string::npos)
    {
        ret = getDeltaPsuVersion(psuI2cBus,psuI2cAddr,&version);
    }
    else
    {
        std::cerr << "PSU verdor is not in support list : " << mfrId << "\n";
        return failedToReadFwVersion;
    }

    if(ret < 0)
    {
        return failedToReadFwVersion;
    }

    return version;
}

} // namespace version
