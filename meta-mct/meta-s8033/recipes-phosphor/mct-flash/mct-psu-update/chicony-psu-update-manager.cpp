#include <chicony-psu-update-manager.hpp>

ChiconyPsuUpdateManager::ChiconyPsuUpdateManager(uint8_t bus, uint8_t addr, std::string fwPath,
                                                 int progress,std::string versionId) :
    bus(bus), addr(addr), fwPath(fwPath), progress(progress), versionId(versionId),
    delayUpdateProcress(chiconyDelayUpdateProcressTimes)
{
    std::ifstream fs(fwPath, std::ios::binary);

    fs.unsetf(std::ios::skipws);
    std::streampos fileSize;

    // Compute data size
    fs.seekg(0, std::ios::end);
    fileSize = fs.tellg();
    fs.seekg(0, std::ios::beg);

    fwData.reserve(fileSize);
    // read the data:
    fwData.insert(fwData.begin(),
               std::istream_iterator<uint8_t>(fs),
               std::istream_iterator<uint8_t>());
    fs.close();
}

ChiconyPsuUpdateManager::~ChiconyPsuUpdateManager()
{
}

int8_t ChiconyPsuUpdateManager::writeReadPmbusCmd(std::vector<uint8_t> writeData,
                                           uint8_t readDateSize,
                                           std::vector<uint8_t> *resData)
{

    return PsuUpdateUtil::writeReadPmbusCmd(bus,addr,writeData,readDateSize,resData);
}

int8_t ChiconyPsuUpdateManager::writePmbusCmd(std::vector<uint8_t> writeData)
{
    
    return PsuUpdateUtil::writePmbusCmd(bus,addr,writeData);
}

int8_t ChiconyPsuUpdateManager::sendBlockWriteRead(uint8_t cmd, std::vector<uint8_t> *rBuf)
{
    

    return PsuUpdateUtil::sendBlockWriteRead(bus,addr,cmd,rBuf);;
}

int8_t ChiconyPsuUpdateManager::setFwUploadMode(uint8_t mode)
{
    uint8_t busAddr = addr << 1;
    std::vector<uint8_t> crcData = {busAddr, CMD_FWUPLOAD_MODE, mode};
    uint8_t crcValue = i2c_smbus_pec(0, crcData.data(), crcData.size());

    std::vector<uint8_t> cmdData = {CMD_FWUPLOAD_MODE, mode, crcValue};

    int ret = writePmbusCmd(cmdData);
    if (ret < 0)
    {
        std::cerr << __func__ << ": Write pmbus command fail" << std::endl;;
    }

    return 0;
}

int8_t ChiconyPsuUpdateManager::getFwUploadMode(int8_t *mode)
{
    std::vector<uint8_t> cmdData;
    cmdData.assign(1, CMD_FWUPLOAD_MODE);
    std::vector<uint8_t> rBuf;

    int ret = writeReadPmbusCmd(cmdData, 1, &rBuf);
    if (ret < 0)
    {
        std::cerr << __func__ << ": Get firmware upload mode fail" << std::endl;;
        return -1;
    }

    *mode = rBuf[0];

    return 0;
}

int8_t ChiconyPsuUpdateManager::getFwUploadStatus(uint16_t *status)
{
    std::vector<uint8_t> cmdData;
    cmdData.assign(1, CMD_FWUPLOAD_STATUS);
    std::vector<uint8_t> rBuf;

    int ret = writeReadPmbusCmd(cmdData, 2, &rBuf);
    if (ret < 0)
    {
        std::cerr << __func__ << ": Get firmware upload mode fail" << std::endl;;
        return -1;
    }

    *status = rBuf[0] | (rBuf[1] << 8);

    return 0;
}

int8_t ChiconyPsuUpdateManager::enterUploadMode()
{
    int ret = 0;
    unsigned int retry = 0;

    do
    {
        int8_t mode = -1;
        if ((ret = setFwUploadMode(enterFwUpdateMode)) < 0)
        {
            std::cerr << __func__ << ": Set firmware upload mode fail" << std::endl;
        }
        else
        {
            sleep(delayTimeToSetUploadMode);

            if ((ret = getFwUploadMode(&mode)) != 0)
            {
                std::cerr << __func__ << ": Get firmware upload mode fail" << std::endl;
            }
            else
            {
                if (mode != enterFwUpdateMode)
                {
                    std::cerr << __func__ << ": Wrong PSU FW upload mode : " << static_cast<int16_t>(mode) << std::endl;
                    ret = -1;
                }
                else
                {
                    ret = 0;
                }
            }
        }
    }
    while (ret != 0 && (retry++ < maxRetry));

    return ret;
}

int8_t ChiconyPsuUpdateManager::exitUploadMode()
{
    int ret = 0;
    unsigned int setModeRetry = 0, getModeRetry = 0;
    unsigned int delayExitUpload = 0, maxRetryVerifyMode = 0, delayGetMode = 0;

    delayExitUpload = chiconyDelayAfterUpdate;
    maxRetryVerifyMode = chiconyMaxRetry;
    delayGetMode = chiconyRetryDelay;

    do
    {
        int8_t mode = -1;
        if ((ret = setFwUploadMode(exitFwUpdateMode)) != 0)
        {
            std::cerr << __func__ << ": Set firmware upload mode fail" << std::endl;
        }
        else
        {
            sleep(delayExitUpload);
            do
            {
                if ((ret = getFwUploadMode(&mode)) != 0)
                {
                    std::cerr << __func__ << ": Get firmware upload mode fail" << std::endl;
                    sleep(delayGetMode);
                }
                else
                {
                    if (mode != exitFwUpdateMode)
                    {
                        std::cerr << __func__ << ": Wrong PSU FW upload mode : " << mode << std::endl;
                        ret = -1;
                    }
                }
            } while (ret && (getModeRetry++ < maxRetryVerifyMode));
        }
    }
    while (ret != 0 && (setModeRetry++ < maxRetry));

    return ret;
}

int8_t ChiconyPsuUpdateManager::writeDataToPsu(int start, uint8_t size)
{
    uint8_t busAddr = addr << 1;
    std::vector<uint8_t> crcData = {busAddr, CMD_FWUPLOAD, size};

    crcData.insert(crcData.end(), fwData.begin() + start, fwData.begin() + start + size);

    uint8_t crcValue = i2c_smbus_pec(0, crcData.data(), crcData.size());
    std::vector<uint8_t> cmdData = crcData;

    // Erase bus data
    cmdData.erase(cmdData.begin());

    // Add pec
    cmdData.emplace_back(crcValue);

    int ret = writePmbusCmd(cmdData);
    if (ret < 0)
    {
        std::cerr << __func__ << ": Write pmbus command fail" << std::endl;
    }

    return 0;
}

int8_t ChiconyPsuUpdateManager::checkFwUploadStatus(int currentIndex)
{
    int retry = 0;
    uint16_t uploadSts = 0;
    // std::cout<<currentIndex<<" "<<fwData.size() << std::endl;;
    int progressRate = (currentIndex * 100) / fwData.size();
    std::cout << "Update :" << progressRate << "% \r";

    if(delayUpdateProcress > 0 ){
            delayUpdateProcress --;
    }
    else
    {
        PsuUpdateUtil::update_flash_progress(chiconyUpdateProcressBase + progressRate/2,
                                             versionId.c_str(),progress);
        delayUpdateProcress = chiconyDelayUpdateProcressTimes;
    }


    do
    {
        if (getFwUploadStatus(&uploadSts))
        {
            std::cerr << __func__ << " : Get PSU FW upload status fail" << std::endl;;
        }
        else
        {
            if (currentIndex == fwData.size())
            {
                if (uploadSts & fullImageReceive)
                {
                    return 0;
                }
                std::cerr << __func__ << ": Full image receive is not set" << std::endl;;
            }
            else
            {
                if (!(uploadSts & BadImageSafeMode) &&
                        !(uploadSts & BadImage) &&
                        !(uploadSts & ImageNotSupport))
                {
                    return 0;
                }
            }
        }

        sleep(retryDelay);
        std::cout << __func__ << " : uploadSts " << uploadSts << ",retry " << retry << std::endl;;
    } while ((retry++) < maxRetry);

    return -1;
}

int8_t ChiconyPsuUpdateManager::sendImageToPsu()
{
    int currentIndex = 0, blockNum = 0;
    uint8_t writeSize = 0;
    int psuFwUploadBlockSize = fwData[psuFwUploadBlockSizeOffset];

    bool oneBlockHeader = true;

    std::cout << __func__ << ": The size of PSU firmware header : " << psuFwHeaderSize << std::endl;;
    std::cout << __func__ << ": The block size of PSU firmware upload : " << psuFwUploadBlockSize << std::endl;;

    while (currentIndex < fwData.size())
    {
        writeSize = (blockNum == 0 && oneBlockHeader) ? psuFwHeaderSize : psuFwUploadBlockSize;
        // std::cout << "Upgrade image block " << blockNum << ", size " << unsigned(writeSize) << std::endl;;

        if (writeDataToPsu(currentIndex, writeSize))
        {
            std::cerr << __func__ << ": upgrade image block " << blockNum << " fail" << std::endl;;
            return -1;
        }

        if (currentIndex < psuFwHeaderSize)
        {
            sleep(flashHeaderDelayTime);
        }
        else
        {
            usleep(flashBlockDelayTimeUs);
        }

        currentIndex += writeSize;
        blockNum++;

        if (checkFwUploadStatus(currentIndex))
        {
            std::cerr << __func__ << ": Upload Status Error " << std::endl;;
            return -1;
        }
    }
    std::cout << "update finish" << std::endl;
    return 0;
}

int8_t ChiconyPsuUpdateManager::fwUpdate()
{
    int ret = -1;

    PsuUpdateUtil::update_flash_progress(25,versionId.c_str(),progress);

    ret = enterUploadMode();
    if (ret < 0)
    {
        std::cerr << __func__ << ": Enter boot load mode fail" << std::endl;;
        return -1;
    }

    PsuUpdateUtil::update_flash_progress(chiconyUpdateProcressBase,versionId.c_str(),progress);

    ret = sendImageToPsu();
    if (ret < 0)
    {
        std::cerr << __func__ << ": Send image to PSU fail" << std::endl;;
        return -1;
    }

    sleep(delayFinishUpload);

    ret = exitUploadMode();
    if (ret < 0)
    {
        std::cerr << __func__ << ": Exit boot load mode fail" << std::endl;;
        return -1;
    }

    sleep(delayFinishUpdate);

    PsuUpdateUtil::update_flash_progress(85,versionId.c_str(),progress);
    
    return 0;
}
