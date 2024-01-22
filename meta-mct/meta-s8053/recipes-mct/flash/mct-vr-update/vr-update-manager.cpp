#include <vr-update-manager.hpp>

static constexpr bool DEBUG = false;

VrUpdateManager::VrUpdateManager(uint8_t bus, uint8_t addr, std::string fwPath,
                                 uint32_t deviceId, int progress, std::string versionId) :
    bus(bus), addr(addr), fwPath(fwPath), deviceId(deviceId), progress(progress), versionId(versionId)
{
    std::string line;

    if constexpr (DEBUG)
    {
        std::cerr << "Bus: " << std::to_string(bus) << std::endl;
        std::cerr << "Address: " << std::to_string(addr) << std::endl;
    }

    std::ifstream inputFile(fwPath.c_str());
    if (inputFile.is_open()) {
        while (getline(inputFile, line))
        {
            std::vector<uint8_t> hexData = VrUpdateUtil::hexToNumByteArray(line);
            vr_hex_t tempHex;

            tempHex.header =  hexData[0];
            tempHex.byteCount =  hexData[1];
            tempHex.pmbusAddress =  hexData[2];
            tempHex.pmbusCommand =  hexData[3];
            tempHex.checksum =  hexData[hexData.size()-1];

            for(int i = 4; i < hexData.size()-1; i++)
            {
                tempHex.data.insert(tempHex.data.end(),hexData[i]);
            }
            
            switch(tempHex.header)
            {
                case DATA_RECORD:
                    hexStore.insert(hexStore.end(),tempHex);
                    break;
                case HEADER_RECORD :
                    if(tempHex.pmbusCommand == IcReg::CMD_DEVICE_ID)
                    {
                        hexDeviceId.assign(tempHex.data.begin(), tempHex.data.end());
                    }
                    else if(tempHex.pmbusCommand == IcReg::CMD_DEVICE_REV)
                    {
                        hexDeviceRev.assign(tempHex.data.begin(), tempHex.data.end());
                    }
                    break;
            }
        }
    }
}

VrUpdateManager::~VrUpdateManager()
{
}

int8_t VrUpdateManager::writeReadPmbusCmd(std::vector<uint8_t> writeData,
                                           uint8_t readDateSize,
                                           std::vector<uint8_t> *resData)
{

    return VrUpdateUtil::writeReadPmbusCmd(bus,addr,writeData,readDateSize,resData);
}

int8_t VrUpdateManager::writePmbusCmd(std::vector<uint8_t> writeData)
{
    
    return VrUpdateUtil::writePmbusCmd(bus,addr,writeData);
}

int8_t VrUpdateManager::sendBlockWriteRead(uint8_t cmd, std::vector<uint8_t> *rBuf)
{
    

    return VrUpdateUtil::sendBlockWriteRead(bus,addr,cmd,rBuf);;
}

int8_t VrUpdateManager::getDeviceId(std::vector<uint8_t>* deviceId)
{
    std::vector<uint8_t> cmdData;
    cmdData.assign(1, IcReg::CMD_DEVICE_ID);
    std::vector<uint8_t> rBuf;

    int ret = writeReadPmbusCmd(cmdData, 5, &rBuf);
    if (ret < 0)
    {
        std::cerr << __func__ << ": Get device ID fail" << std::endl;
        return -1;
    }

    deviceId->assign(rBuf.begin()+1, rBuf.end());
    std::reverse(deviceId->begin(), deviceId->end());

    return 0;
}

int8_t VrUpdateManager::getDeviceRev(std::vector<uint8_t>* deviceRev)
{
    std::vector<uint8_t> cmdData;
    cmdData.assign(1, IcReg::CMD_DEVICE_REV);
    std::vector<uint8_t> rBuf;

    int ret = writeReadPmbusCmd(cmdData, 5, &rBuf);
    if (ret < 0)
    {
        std::cerr << __func__ << ": Get MSR device Rev fail" << std::endl;
        return -1;
    }

    deviceRev->assign(rBuf.begin()+1, rBuf.end());
    std::reverse(deviceRev->begin(), deviceRev->end());

    return 0;
}

int8_t VrUpdateManager::disablePacketCapture()
{
    std::vector<uint8_t> writeCmdData = {0xc7, 0xA2, 0x00};
    std::vector<uint8_t> readCmdData {0xc5};
    std::vector<uint8_t> rBuf;
    int ret;

    ret = writePmbusCmd(writeCmdData);
    if (ret < 0)
    {
        std::cerr << __func__ << ": Get devcie data write fail" << std::endl;
        return -1;
    }

    ret = writeReadPmbusCmd(readCmdData, 4, &rBuf);
    if (ret < 0)
    {
        std::cerr << __func__ << ": Get devcie data read fail" << std::endl;
        return -1;
    }

    std::vector<uint8_t> writeDisablePacketCaptureCmdData = {0xc5, rBuf[0], rBuf[1], rBuf[2], rBuf[3]};

    ret = writePmbusCmd(writeCmdData);
    if (ret < 0)
    {
        std::cerr << __func__ << ": Get disable packet capture write fail" << std::endl;
        return -1;
    }

    return 0;
}

int8_t VrUpdateManager::applyPacketCaptureChanges()
{
    std::vector<uint8_t> writeCmdData = {0xE7, 0x02, 0x00};
    int ret;

    ret = writePmbusCmd(writeCmdData);
    if (ret < 0)
    {
        std::cerr << __func__ << ": Get apply packet capture changes write fail" << std::endl;
        return -1;
    }

     return 0;
}

int8_t VrUpdateManager::getAvailableNvmSlot(uint32_t& availableNvmSlot)
{
    std::vector<uint8_t> writeCmdData = {0xc7, 0x35, 0x00};
    std::vector<uint8_t> readCmdData {0xc5};
    std::vector<uint8_t> rBuf;
    int ret;


    ret = writePmbusCmd(writeCmdData);
    if (ret < 0)
    {
        std::cerr << __func__ << ": Get available NVM Slot write fail" << std::endl;
        return -1;
    }

    ret = writeReadPmbusCmd(readCmdData, 4, &rBuf);
    if (ret < 0)
    {
        std::cerr << __func__ << ": Get available NVM Slot read fail" << std::endl;
        return -1;
    }

    if constexpr (DEBUG)
    {
        std::cerr << "getAvailableNvmSlot: "
                << VrUpdateUtil::byteArrayToString(rBuf)
                << std::endl;
    }

    for(int i=0; i < rBuf.size(); i++)
    {
        availableNvmSlot += rBuf.at(i);
    }


    return 0;
}

int8_t VrUpdateManager::getPollProgreammerStatus()
{
    std::vector<uint8_t> writeCmdData = {0xc7, 0x7E, 0x00};
    std::vector<uint8_t> readCmdData {0xc5};
    std::vector<uint8_t> rBuf;
    int ret;

    ret = writePmbusCmd(writeCmdData);
    if (ret < 0)
    {
        std::cerr << __func__ << ": Get poll progreammer status write fail" << std::endl;
        return -1;
    }

    ret = writeReadPmbusCmd(readCmdData, 4, &rBuf);
    if (ret < 0)
    {
        std::cerr << __func__ << ": Get poll progreammer status read fail" << std::endl;
        return -1;
    }

    if constexpr (DEBUG)
    {
        std::cerr << "getPollProgreammerStatus: "
                  << VrUpdateUtil::byteArrayToString(rBuf)
                  << std::endl;
    }

    if(rBuf.at(0) & 0x1)
    {
        return 0;
    }

    return -1;
}

int8_t VrUpdateManager::getBankStatus()
{
    std::vector<uint8_t> writeCmdData = {0xc7, 0x7F, 0x00};
    std::vector<uint8_t> readCmdData {0xc5};
    std::vector<uint8_t> rBuf;
    int ret;

    ret = writePmbusCmd(writeCmdData);
    if (ret < 0)
    {
        std::cerr << __func__ << ": Get bank status write fail" << std::endl;
        return -1;
    }

    ret= writeReadPmbusCmd(readCmdData, 4, &rBuf);
    if (ret < 0)
    {
        std::cerr << __func__ << ": Get bank status read fail" << std::endl;
        return -1;
    }

    if constexpr (DEBUG)
    {
        std::cerr << "getBankStatus: " 
                  << VrUpdateUtil::byteArrayToString(rBuf)
                  << std::endl;
    }

    return 0;
}


int8_t VrUpdateManager::getCurretVrFwCrc()
{
    std::vector<uint8_t> writeCmdData = {0xc7, 0x94, 0x00};
    std::vector<uint8_t> readCmdData {0xc5};
    std::vector<uint8_t> rBuf;
    int ret;

    ret = writePmbusCmd(writeCmdData);
    if (ret < 0)
    {
        std::cerr << __func__ << ": Get current VR firmware CRC write fail" << std::endl;
        return -1;
    }

    ret = writeReadPmbusCmd(readCmdData, 4, &rBuf);
    if (ret < 0)
    {
        std::cerr << __func__ << ": Get current VR firmware CRC read fail" << std::endl;
        return -1;
    }

    std::cerr << "getCurretVrFwCrc: "
              << VrUpdateUtil::byteArrayToString(rBuf)
              << std::endl;

    return 0;
}

int8_t VrUpdateManager::resetVrDevcie()
{
    std::vector<uint8_t> writeCmdData = {0xf2, 0x00};

    int ret= writePmbusCmd(writeCmdData);
    if (ret < 0)
    {
        std::cerr << __func__ << ": Get reset VR device fail" << std::endl;
        return -1;
    }

    return 0;
}

int8_t VrUpdateManager::algorithmCompletion()
{
    std::vector<uint8_t> writeCmdData = {0xE7, 0x01, 0x00};

    int ret= writePmbusCmd(writeCmdData);
    if (ret < 0)
    {
        std::cerr << __func__ << ": Get algorithm completion write fail" << std::endl;
        return -1;
    }

    return 0;
}

int8_t VrUpdateManager::fwUpdate()
{
    int ret;

    // (Optional) Disable packet capture.
    // This step should be completed for parts with a configuration
    // For production programming, Step 1 can be ignored.
    // ret = disablePacketCapture();
    // if(ret < 0)
    // {
    //     return -1;
    // }

    // ret = applyPacketCaptureChanges();
    // if(ret < 0)
    // {
    //     return -1;
    // }

    uint32_t availableNvmSlot = 0;
    ret = getAvailableNvmSlot(availableNvmSlot);
    if(ret < 0)
    {
        return -1;
    }

    std::string availableNvmSlotInfo =  "VR device available NVM Slot : " + std::to_string(availableNvmSlot);

    std::cerr << availableNvmSlotInfo<< std::endl;

    VrUpdateUtil::update_flash_progress(vrUpdateProcressBase-5,versionId.c_str(),progress,availableNvmSlotInfo);
    sleep(2);

    if(availableNvmSlot <= 1)
    {
        std::string info = "Your VR device available NVM Slot almost hit the limitation. "
                            "Please using VR device cable to update VR firmware.";
        std::cerr << info << std::endl;
        VrUpdateUtil::update_flash_progress(vrUpdateProcressBase-5,versionId.c_str(),progress,info);
        sleep(2);
        return -1;
    }

    usleep(100000);

    std::vector<uint8_t> chipDeviceId;
    ret = getDeviceId(&chipDeviceId);
    if(ret < 0)
    {
        return -1;
    }

    std::cerr << "getDeviceId: "
              << VrUpdateUtil::byteArrayToString(chipDeviceId)
              << std::endl;
    std::cerr << "hexDeviceId: "
              << VrUpdateUtil::byteArrayToString(hexDeviceId)
              << std::endl;
    
    if(!std::equal(hexDeviceId.begin(), hexDeviceId.end(), chipDeviceId.begin()))
    {
        std::cerr << "VR firmware Device ID is mismatching!" << std::endl;
        return -1;
    }
    std::cerr << "VR firmware Device ID is matching." << std::endl;
    VrUpdateUtil::update_flash_progress(vrUpdateProcressBase-5,versionId.c_str(),progress);

    std::vector<uint8_t> deviceRev;
    ret = getDeviceRev(&deviceRev);
    if(ret < 0)
    {
        return -1;
    }
    std::cerr << "getDeviceRev: "
              << VrUpdateUtil::byteArrayToString(deviceRev)
              << std::endl;
    std::cerr << "hexDeviceRev: "
              << VrUpdateUtil::byteArrayToString(hexDeviceRev)
              << std::endl;
    VrUpdateUtil::update_flash_progress(vrUpdateProcressBase,versionId.c_str(),progress);

    // VR IC only have limitation for 28 times programmable NVM.
    for(int i = 0; i < hexStore.size(); i++)
    {
        std::vector<uint8_t> cmdData = {hexStore[i].pmbusCommand};

        for(int j = 0; j < hexStore[i].data.size(); j++)
        {
            cmdData.push_back(hexStore[i].data[j]);
        }

        if constexpr (DEBUG)
        {
            std::cerr << "Write hex file data: "
                << VrUpdateUtil::byteArrayToString(cmdData)
                << std::endl;
        }

        writePmbusCmd(cmdData);

        int progressRate = (i * 100) / hexStore.size();
        std::cerr << "Update :" << progressRate << "% \r";
        VrUpdateUtil::update_flash_progress(vrUpdateProcressBase + progressRate/2,
                                            versionId.c_str(),progress);
        usleep(100000);
    }
    
    sleep(1);
    int counter = 0;
    while(getPollProgreammerStatus() !=0)
    {
        sleep(1);
        if(counter >= 5)
        {
            std::cerr << "Failed to get PROGRAMMER_STATUS during 5 seconds." << std::endl;
            break;
        }
        counter++;
    }

    VrUpdateUtil::update_flash_progress(vrUpdateProcressBase+50,versionId.c_str(),progress);

    sleep(1);
    ret = getBankStatus();
    if(ret < 0)
    {
        return -1;
    }

    VrUpdateUtil::update_flash_progress(vrUpdateProcressBase+55,versionId.c_str(),progress);

    // It would cause host power off
    // ret = resetVrDevcie();
    // if(ret < 0)
    // {
    //     return -1;
    // }
    // sleep(2);

    getCurretVrFwCrc();

    algorithmCompletion();

    VrUpdateUtil::update_flash_progress(vrUpdateProcressBase+60,versionId.c_str(),progress);

    if(counter >= 5)
    {
        return -1;
    }

    return 0;
}
