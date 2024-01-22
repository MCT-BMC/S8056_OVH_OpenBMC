#include <delta-psu-update-manager.hpp>

DeltaPsuUpdateManager::DeltaPsuUpdateManager(uint8_t bus, uint8_t addr, std::string fwPath,
                                             int progress, std::string versionId) :
    bus(bus), addr(addr), fwPath(fwPath), progress(progress), versionId(versionId),
    delayUpdateProcress(deltaDelayUpdateProcressTimes)
{
    std::string line;

    std::ifstream inputFile(fwPath.c_str());
    if (inputFile.is_open()) {
        while (getline(inputFile, line))
        {
            std::string origStr = line.substr(1, line.size());
            std::vector<uint8_t> hexData = PsuUpdateUtil::hexToNumByteArray(origStr);
            intel_hex_t intelHex;

            intelHex.byteCount =  hexData[0];
            intelHex.address[0] =  hexData[1];
            intelHex.address[1] =  hexData[2];
            intelHex.recordType =  hexData[3];
            intelHex.checksum =  hexData[hexData.size()-1];

            for(int i = 4; i < hexData.size()-1; i++)
            {
                intelHex.data.insert(intelHex.data.end(),hexData[i]);
            }

            switch(intelHex.recordType)
            {
                case DATA_RECORD:
                    intelHexStore.insert(intelHexStore.end(),intelHex);
                    break;
                case END_OF_FILE_RECORD :
                case EXTENDED_LINEAR_ADDRESS :
                case START_SEGMENT_ADDRESS :
                case EXTENDED_SEGMENT_ADDRESS :
                case START_LINEAR_ADDRESS :
                    break;
            }
        }
    }
}

DeltaPsuUpdateManager::~DeltaPsuUpdateManager()
{
}

int8_t DeltaPsuUpdateManager::writeReadPmbusCmd(std::vector<uint8_t> writeData,
                                           uint8_t readDateSize,
                                           std::vector<uint8_t> *resData)
{

    return PsuUpdateUtil::writeReadPmbusCmd(bus,addr,writeData,readDateSize,resData);
}

int8_t DeltaPsuUpdateManager::writePmbusCmd(std::vector<uint8_t> writeData)
{
    
    return PsuUpdateUtil::writePmbusCmd(bus,addr,writeData);
}

int8_t DeltaPsuUpdateManager::sendBlockWriteRead(uint8_t cmd, std::vector<uint8_t> *rBuf)
{
    

    return PsuUpdateUtil::sendBlockWriteRead(bus,addr,cmd,rBuf);;
}

int8_t DeltaPsuUpdateManager::getDeviceCode(std::vector<uint8_t> *deviceCode)
{
    std::vector<uint8_t> cmdData;
    cmdData.assign(1, CMD_MFR_DEVICE_CODE);
    std::vector<uint8_t> rBuf;

    int ret = writeReadPmbusCmd(cmdData, 4, deviceCode);
    if (ret < 0)
    {
        std::cerr << __func__ << ": Get MSR device code fail" << std::endl;
        return -1;
    }

    return 0;
}

int8_t DeltaPsuUpdateManager::setIspKey(uint32_t ispKey)
{
    std::vector<uint8_t> cmdData;
    cmdData.push_back(CMD_ISP_KEY);

    uint32_t ispKeyBuf = ispKey;

    for (size_t i = 0; i < sizeof(ispKeyBuf); ++i) {
        cmdData.push_back((ispKeyBuf & 0xFF000000) >> 24);
        ispKeyBuf <<= 8;
    }

    int ret = writePmbusCmd(cmdData);
    if (ret < 0)
    {
        std::cerr << __func__ << ": Set ISP key fail" << std::endl;
        return -1;
    }

    return 0;
}

int8_t DeltaPsuUpdateManager::setIspStatus(uint8_t mode)
{
    std::vector<uint8_t> cmdData;
    cmdData.push_back(CMD_ISP_STATUS);
    cmdData.push_back(mode);

    int ret = writePmbusCmd(cmdData);
    if (ret < 0)
    {
        std::cerr << __func__ << ": Set ISP status fail" << std::endl;
        return -1;
    }

    return 0;
}

int8_t DeltaPsuUpdateManager::getIspStatus(int8_t *ispStatus)
{
    std::vector<uint8_t> cmdData;
    cmdData.assign(1, CMD_ISP_STATUS);
    std::vector<uint8_t> rBuf;

    int ret = writeReadPmbusCmd(cmdData, 1, &rBuf);
    if (ret < 0)
    {
        std::cerr << __func__ << ": Get MSR device code fail" << std::endl;
        return -1;
    }

    *ispStatus = rBuf[0];

    return 0;
}

int8_t DeltaPsuUpdateManager::getFwUploadCapability(std::vector<uint8_t> *fwCapability)
{
    std::vector<uint8_t> cmdData;
    cmdData.assign(1, CMD_FWUPLOAD_CAPABILITY);
    // std::vector<uint8_t> rBuf;

    int ret = writeReadPmbusCmd(cmdData, 5, fwCapability);
    if (ret < 0)
    {
        std::cerr << __func__ << ": Get MSR device code fail" << std::endl;
        return -1;
    }

    return 0;
}

int8_t DeltaPsuUpdateManager::getFwUpdataStatus(std::vector<uint8_t> *fwUpdatStatus)
{
    std::vector<uint8_t> cmdData;
    cmdData.assign(1, CMD_FWUPDATE_STATUS);
    std::vector<uint8_t> rBuf;

    int ret = writeReadPmbusCmd(cmdData, 4, fwUpdatStatus);
    if (ret < 0)
    {
        std::cerr << __func__ << ": Get MSR device code fail" << std::endl;
        return -1;
    }

    return 0;
}

int8_t DeltaPsuUpdateManager::writeDataToPsu(std::vector<uint8_t> hexData)
{
    std::vector<uint8_t> cmdData = {CMD_ISP_MEMORY};

    cmdData.insert(cmdData.end(), hexData.begin(), hexData.end());

    int ret = writePmbusCmd(cmdData);
    if (ret < 0)
    {
        std::cerr << __func__ << ": Write pmbus command fail" << std::endl;
    }

    return 0;
}

int8_t DeltaPsuUpdateManager::sendImageToPsu()
{
    int currentIndex = 0, blockNum = 0, blockNumCounter = 1;
    uint8_t writeSize = 0;
    bool oneBlockHeader = false;
    std::vector<uint8_t> fwCapability;

    int psuFwblockSize = 16;
    int eraseFwDelayTimeMs = 1500;
    int writeFw16DelayTimeMs = 20;
    int writeFw32DelayTimeMs = 40;


    if ((getFwUploadCapability(&fwCapability)) != 0)
    {
        std::cerr << __func__ << ": Get firmware upload capability fail" << std::endl;
    }
    std::cerr << __func__ << ": firmware capability : "
              << PsuUpdateUtil::byteArrayToString(fwCapability) << std::endl;


    if(fwCapability[1] != 0)
    {
        if(fwCapability[1] & 0x02)
        {
            psuFwblockSize = 32;
        }
    }

    if(fwCapability[2] != 0)
    {
        eraseFwDelayTimeMs = fwCapability[2] * 100;
    }

    if(fwCapability[3] != 0)
    {
        writeFw16DelayTimeMs = fwCapability[3];
    }

    if(fwCapability[4] != 0)
    {
        writeFw32DelayTimeMs = fwCapability[4];
    }

    std::cout << __func__ << ": The size of PSU firmware header : " << deltaPsuFwHeaderSize << std::endl;
    std::cout << __func__ << ": The block size of PSU firmware upload : " << psuFwblockSize << std::endl;

    setIspStatus(CMD_BOOT_ISP);

    sleep(delayTimeToSetUpMode);

    setIspStatus(CMD_RESET_SEQ);

    sleep(delayTimeToSetUpMode);

    int headerDelay = (eraseFwDelayTimeMs)*1000;
    int blockDelay = (writeFw32DelayTimeMs)*1000;

    if(psuFwblockSize == 32)
    {
        blockNumCounter = 2;
        blockDelay = (writeFw32DelayTimeMs)*1000;
    }
    else
    {
        blockNumCounter = 1;
        blockDelay = (writeFw16DelayTimeMs)*1000;
    }

    while (blockNum < intelHexStore.size())
    {
        // std::cerr << "Upgrade image block " << blockNum << ", size " << unsigned(psuFwblockSize) << std::endl;

        std::vector<uint8_t> hexData;

        hexData.insert(hexData.end(),
               intelHexStore[blockNum].data.begin(),
               intelHexStore[blockNum].data.end());

        if(psuFwblockSize == 32)
        {
            hexData.insert(hexData.end(),
                           intelHexStore[blockNum+1].data.begin(),
                           intelHexStore[blockNum+1].data.end());
        }

        if (writeDataToPsu(hexData))
        {
            std::cerr << __func__ << ": upgrade image block " << blockNum << " fail" << std::endl;
            return -1;
        }

        int progressRate = (blockNum * 100) / intelHexStore.size();
        std::cerr << "Update :" << progressRate << "% \r";
        if(delayUpdateProcress > 0 ){
            delayUpdateProcress --;
        }
        else
        {
            PsuUpdateUtil::update_flash_progress(deltaUpdateProcressBase + progressRate/2,
                                                 versionId.c_str(),progress);
            delayUpdateProcress = deltaDelayUpdateProcressTimes;
        }

        if(currentIndex <= deltaPsuFwHeaderSize)
        {
            usleep(headerDelay);
        }
        else
        {
            usleep(blockDelay);
        }

        currentIndex += psuFwblockSize;
        blockNum+=blockNumCounter;
    }

    std::cerr << __func__ << ": Update finish." << std::endl;
    return 0;
}

int8_t DeltaPsuUpdateManager::fwUpdate()
{
    int ret = -1;
    int8_t ispStatus = -1;
    std::vector<uint8_t> deviceCode;
    std::vector<uint8_t> fwUpdatStatus;

    PsuUpdateUtil::update_flash_progress(25,versionId.c_str(),progress);

    if (getDeviceCode(&deviceCode) != 0)
    {
        std::cerr << __func__ << ": Get MSR device code fail" << std::endl;
        return -1;
    }
    std::cerr << __func__ << ": MSR device code : " 
              << PsuUpdateUtil::byteArrayToString(deviceCode) << std::endl;
    

    setIspKey(ispKey);

    PsuUpdateUtil::update_flash_progress(deltaUpdateProcressBase,versionId.c_str(),progress);

    ret = sendImageToPsu();

    sleep(delayTimeToSetUpMode);

    if (getIspStatus(&ispStatus) != 0)
    {
        std::cerr << __func__ << ": Get ISP status fail" << std::endl;
    }
    std::cerr << __func__  << std::hex << ": ISP status : 0x" 
              << static_cast<int>(ispStatus) << std::dec << std::endl;

    if ((getFwUpdataStatus(&fwUpdatStatus)) != 0)
    {
        std::cerr << __func__ << ": Get firmware update status fail" << std::endl;
    }
    std::cerr << __func__ << ": Firmware update status : "
              << PsuUpdateUtil::byteArrayToString(fwUpdatStatus) << std::endl;

    setIspStatus(CMD_BOOT_PM);

    sleep(delayTimeToSetUpMode);

    if (getIspStatus(&ispStatus) != 0)
    {
        std::cerr << __func__ << ": Get ISP status fail" << std::endl;
    }
    std::cerr << __func__  << std::hex << ": ISP status : 0x" 
              << static_cast<int>(ispStatus) << std::dec << std::endl;

    PsuUpdateUtil::update_flash_progress(85,versionId.c_str(),progress);

    return 0;
}
