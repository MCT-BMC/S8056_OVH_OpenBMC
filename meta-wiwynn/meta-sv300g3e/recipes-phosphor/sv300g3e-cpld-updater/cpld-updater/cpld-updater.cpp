#include <iostream>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <chrono>
#include <thread>

#include "cpld-updater.hpp"
#include "cpld-lattice.hpp"
#include "openbmc/libobmcjtag.hpp"

CpldUpdateManager::CpldUpdateManager()
{
    jtagFd = -1;
    cpldFp = NULL;
    cfSize = 0;
    ufmSize = 0;
    eraseType = 0;
    devInfo = {0};
    devInfo.CF = NULL;
    devInfo.UFM = NULL;
}

CpldUpdateManager::~CpldUpdateManager()
{
    cpldUpdateCloseAll();
}

int CpldUpdateManager::indexof(char *str, const char *ptn)
{
    char *ptr = std::strstr(str, ptn);
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

bool CpldUpdateManager::startWith(const char *str, const char *ptn)
{
    int len = std::strlen(ptn);
    
    for (int i=0; i<len; i++)
    {
        if ( str[i] != ptn[i] )
        {
            return false;
        }
    }

    return true;
}

int CpldUpdateManager::shiftData(char *data, unsigned int *result, int len)
{
    int resultIdx = 0;
    int dataIdx = 0;

    for (int i=0; i<len; i++)
    {
        data[i] = data[i] - 0x30;
        result[resultIdx] |= ((unsigned char)data[i] << dataIdx);
        dataIdx++;

        if (0 == ((i+1) % 32))
        {
            dataIdx = 0;
            resultIdx++;
        }
    }
    
    return 0;
}

int CpldUpdateManager::checkDeviceStatus(uint8_t mode, unsigned int &status)
{
    uint32_t retry = maxRetry;
    std::vector<unsigned int> temp(1,0);
    int ret = -1;
    status = 0;

    switch (mode)
    {
        case CHECK_BUSY:
            ret = jtag_interface_end_tap_state(jtagFd, 0, JTAG_STATE_IDLE, 2);
            if (ret < 0)
            {
                return -1;
            }

            do
            {
                ret = jtag_interface_sir_xfer(jtagFd, JTAG_STATE_IDLE,
                                              LatticeInsLength, Lcmxo2LscCheckBusy);
                if (ret >= 0)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    temp.at(0) = 0;
                    ret = jtag_interface_tdo_xfer(jtagFd, JTAG_STATE_IDLE, 8, temp.data());
                    status = temp.at(0);
                    temp.at(0) = temp.at(0) & 0x01;

                    if ((0x0 == temp.at(0)) && (ret >=0))
                    {
                        break;
                    }
                }
                retry--;
            } while (retry != 0);
        break;

        case CHECK_STATUS:
            ret = jtag_interface_end_tap_state(jtagFd, 0, JTAG_STATE_IDLE, 2);
            if (ret < 0)
            {
                return -1;
            }

            do
            {

                ret = jtag_interface_sir_xfer(jtagFd, JTAG_STATE_IDLE,
                                              LatticeInsLength, Lcmxo2LscReadStatus);

                if (ret >= 0)
                {
                    // jtag_interface_end_tap_state(jtagFd, 0, JTAG_STATE_IDLE, 2);
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    temp.at(0) = 0;
                    ret = jtag_interface_tdo_xfer(jtagFd, JTAG_STATE_IDLE, 32, temp.data());
                    status = temp.at(0);
                    temp.at(0) = (temp.at(0) >> 12) & 0x3;

                    if ((0x0 == temp.at(0)) && (ret >=0))
                    {
                        break;
                    }
                }
                retry--;
            } while (retry != 0);
        break;

        default:
            return -1;
    }

    if(retry == 0)
    {
        return -1;
    }

    return 0;
}

/* Write CF data */
int CpldUpdateManager::SendCFdata()
{
    int CurrentAddr = 0;
    unsigned int status = 0;
    int ret = -1;
    std::vector<unsigned int> CFdata(4,0);

    for (int i=0; i<devInfo.CF_Line; i++)
    {
        CurrentAddr = (i * LatticeColSize) / 32;

        ret = jtag_interface_end_tap_state(jtagFd, 0, JTAG_STATE_IDLE, 2);
        if (ret < 0)
        {
            return -1;
        }

        // set page to program page
        ret = jtag_interface_sir_xfer(jtagFd, JTAG_STATE_IDLE,
                                      LatticeInsLength, Lcmxo2LscProgIncrNv);
        if (ret < 0)
        {
            return -1;
        }

        std::memset(CFdata.data(), 0, CFdata.size());
        for (int j=0; j<CFdata.size(); j++)
        {
            CFdata.at(j) = devInfo.CF[CurrentAddr + j];
        }

        // send data
        ret = jtag_interface_tdi_xfer(jtagFd, JTAG_STATE_IDLE,
                                      LatticeColSize, CFdata.data());
        if (ret < 0)
        {
            return -1;
        }

        ret = jtag_interface_end_tap_state(jtagFd, 0, JTAG_STATE_IDLE, 2);
        if (ret < 0)
        {
            return -1;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        ret = checkDeviceStatus(CHECK_BUSY, status);
        if (ret < 0)
        {
            return -1;
        }
    }

    return 0;
}

/* Write UFM data */
int CpldUpdateManager::SendUFMdata()
{
    int CurrentAddr = 0;
    unsigned int status = 0;
    int ret = -1;

    for (int i=0; i<devInfo.UFM_Line; i++)
    {
        CurrentAddr = (i * LatticeColSize) / 32;

        ret = jtag_interface_end_tap_state(jtagFd, 0, JTAG_STATE_IDLE, 2);
        if (ret < 0)
        {
            return -1;
        }

        // set page to program page
        ret = jtag_interface_sir_xfer(jtagFd, JTAG_STATE_IDLE,
                                      LatticeInsLength, Lcmxo2LscProgIncrNv);
        if (ret < 0)
        {
            return -1;
        }

        // send data
        ret = jtag_interface_tdi_xfer(jtagFd, JTAG_STATE_IDLE,
                                      LatticeColSize, &devInfo.UFM[CurrentAddr]);
        if (ret < 0)
        {
            return -1;
        }

        ret = jtag_interface_end_tap_state(jtagFd, 0, JTAG_STATE_IDLE, 2);
        if (ret < 0)
        {
            return -1;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        ret = checkDeviceStatus(CHECK_BUSY, status);
        if (ret < 0)
        {
            return -1;
        }
    }

    return 0;
}

int CpldUpdateManager::cpldUpdateDevOpen()
{
    jtagFd = open_jtag_dev();
    if (jtagFd < 0)
    {
        std::cerr << "Error in " << __FUNCTION__ << " in " << __LINE__ << "\n";
        return -1;
    }

    // Reset to idle/run-test state and stay in 2 tcks
    int ret = jtag_interface_end_tap_state(jtagFd, 1, JTAG_STATE_IDLE, 2);
    if (ret < 0)
    {
        std::cerr << "Error in " << __FUNCTION__ << " in " << __LINE__ << "\n";
        return -1;
    }

    return 0;
}

void CpldUpdateManager::cpldUpdateDevClose()
{
    close_jtag_dev(jtagFd);
    jtagFd = -1;

    return;
}

int CpldUpdateManager::cpldUpdateFileOpen(const std::string& cpld_path)
{
    cpldFp = std::fopen(cpld_path.c_str(), "r");

    if (NULL == cpldFp) 
    {
        std::cerr << "Error in " << __FUNCTION__ << " in " << __LINE__ << "\n";
        return -1;
    }

    return 0;
}

void CpldUpdateManager::cpldUpdateFileClose()
{
    std::fclose(cpldFp);
    cpldFp = NULL;

    return;
}

void CpldUpdateManager::cpldUpdateCloseAll()
{
    if (jtagFd > 0)
    {
        // std::printf("Close jtag device\n");
        cpldUpdateDevClose();
    }

    if (cpldFp != NULL)
    {
        // std::printf("Close cpld fw file\n");
        cpldUpdateFileClose();
    }

    if (NULL != devInfo.CF)
    {
        // std::printf("Free CF memory\n");
        std::free(devInfo.CF);
        devInfo.CF = NULL;
    }

    if (NULL != devInfo.UFM)
    {
        // std::printf("Free UFM memory\n");
        std::free(devInfo.UFM);
        devInfo.UFM = NULL;
    }
}

int CpldUpdateManager::cpldUpdateCheckId()
{
    // Return to idle state
    int ret = -1;
    ret = jtag_interface_end_tap_state(jtagFd, 0, JTAG_STATE_IDLE, 2);
    if (ret < 0)
    {
        std::cerr << "Error in " << __FUNCTION__ << " in " << __LINE__ << "\n";
        return -1;
    }

    // Check the IDCODE_PUB
    ret = jtag_interface_sir_xfer(jtagFd, JTAG_STATE_IDLE,
                                  LatticeInsLength, Lcmxo2IdcodePub);
    if (ret < 0)
    {
        std::cerr << "Error in " << __FUNCTION__ << " in " << __LINE__ << "\n";
        return -1;
    }

    std::vector<unsigned int> dr_data(4,0);

    ret = jtag_interface_tdo_xfer(jtagFd, JTAG_STATE_IDLE, 32, dr_data.data());
    if (ret < 0)
    {
        std::cerr << "Error in " << __FUNCTION__ << " in " << __LINE__ << "\n";
        return -1;
    }

    std::printf("CPLD ID = 0x%x\n", dr_data.at(0));

    if (dr_data.at(0) != LCMXO2_2000HC_DevId)
    {
        std::cerr << "Invalid CPLD device ID!!!\n";
        return -1;
    }

    return 0;
}


int CpldUpdateManager::cpldUpdateGetUpdateDataSize()
{
    std::fseek(cpldFp, 0, SEEK_SET);
    
    const char TAG_CF_START[] = "L000";
    const char TAG_UFM[] = "NOTE TAG DATA";

    int readLineSize = LatticeColSize;
    char tmp_buf[readLineSize];

    bool CFStart = false;
    bool UFMStart = false;
    
    while( NULL != std::fgets(tmp_buf, readLineSize, cpldFp) )
    {
        if (startWith(tmp_buf, TAG_CF_START) == true)
        {
            CFStart = true;
        }
        else if (startWith(tmp_buf, TAG_UFM) == true)
        {
            UFMStart = true;
        }

        if (CFStart == true)
        {
            if ((startWith(tmp_buf, TAG_CF_START) == false) &&
                (std::strlen(tmp_buf) != 1))
            {
                if ((startWith(tmp_buf,"0") == true) ||
                    (startWith(tmp_buf,"1") == true))
                {
                    cfSize++;
                }
                else
                {
                    CFStart = false;
                }
            }
        }
        else if (UFMStart == true)
        {
            if ((startWith(tmp_buf, TAG_UFM) == false) &&
                (startWith(tmp_buf, "L") == false) &&
                (std::strlen(tmp_buf) != 1))
            {
                if ( startWith(tmp_buf,"0") || startWith(tmp_buf,"1") )
                {
                    ufmSize++;
                }
                else
                {
                    UFMStart = false;
                }
            }
        }
    }

    if (0 == cfSize)
    {
        return -1;
    }

    std::printf("JED CF Size = %d\n", cfSize);
    std::printf("JED UFM Size = %d\n", ufmSize);

    return 0;
}


int CpldUpdateManager::cpldUpdateJedFileParser()
{
    std::fseek(cpldFp, 0, SEEK_SET);

    const char TAG_QF[] = "QF";
    const char TAG_CF_START[] = "L000";
    const char TAG_UFM[] = "NOTE TAG DATA";
    const char TAG_ROW[] = "NOTE FEATURE";
    const char TAG_CHECKSUM[] = "C";
    const char TAG_USERCODE[] = "NOTE User Electronic";

    bool CFStart = false;
    bool UFMStart = false;
    bool ROWStart = false;
    bool VersionStart = false;
    bool ChecksumStart = false;

    /* data + \r\n */
    int readLineSize = LatticeColSize + 2;
    char tmp_buf[readLineSize];
    char data_buf[LatticeColSize];
    int copySize = 0;
    int current_addr = 0;
    unsigned int JedChecksum = 0;

    int i = 0;
    int cfSizeUsed = (cfSize * LatticeColSize) / 8; // unit: bytes
    int ufmSizeUsed = (ufmSize * LatticeColSize) / 8; // unit: bytes

    devInfo.CF = (unsigned int*)std::malloc(cfSizeUsed);
    if (NULL == devInfo.CF)
    {
        std::cerr << "Failed to malloc for CF\n";
        return -1;
    }
    std::memset(devInfo.CF, 0, cfSizeUsed);

    if (0 != ufmSizeUsed)
    {
        devInfo.UFM = (unsigned int*)std::malloc(ufmSizeUsed);
        if (NULL== devInfo.UFM)
        {
            std::cerr << "Failed to malloc for UFM\n";
            return -1;
        }
        std::memset(devInfo.UFM, 0, ufmSizeUsed);
    }

    while( NULL != std::fgets(tmp_buf, readLineSize, cpldFp) )
    {
        if (startWith(tmp_buf, TAG_QF) == true)
        {
            copySize = indexof(tmp_buf, "*") - indexof(tmp_buf, "F") - 1;
            if (copySize < 0)
            {
                std::cerr << "Error in " << __FUNCTION__ << " in " << __LINE__ << "\n";
                return -1;
            }
            
            std::memset(data_buf, 0, sizeof(data_buf));
            std::memcpy(data_buf, &tmp_buf[2], copySize);
            devInfo.QF = std::atol(data_buf);

            std::printf("QF Size = %ld\n", devInfo.QF);
        }
        else if (startWith(tmp_buf, TAG_CF_START) == true)
        {
            CFStart = true;
        }
        else if (startWith(tmp_buf, TAG_UFM) == true)
        {
            UFMStart = true;
        }
        else if (startWith(tmp_buf, TAG_ROW) == true)
        {
            ROWStart = true;
        }
        else if (startWith(tmp_buf, TAG_USERCODE) == true)
        {
            VersionStart = true;
        }
        else if (startWith(tmp_buf, TAG_CHECKSUM) == true)
        {
            ChecksumStart = true;
        }

        if (CFStart == true)
        {
            if ((startWith(tmp_buf, TAG_CF_START) == false) &&
                (std::strlen(tmp_buf) != 1))
            {
                if ((startWith(tmp_buf,"0") == true) ||
                    (startWith(tmp_buf,"1") == true))
                {
                    current_addr = (devInfo.CF_Line * LatticeColSize) / 32;

                    std::memset(data_buf, 0, sizeof(data_buf));
                    std::memcpy(data_buf, tmp_buf, LatticeColSize);

                    // Convert string to byte data
                    shiftData(data_buf, &devInfo.CF[current_addr], LatticeColSize);

                    for ( i = 0; i < sizeof(unsigned int); i++ )
                    {
                        JedChecksum += (devInfo.CF[current_addr+i]>>24) & 0xff;
                        JedChecksum += (devInfo.CF[current_addr+i]>>16) & 0xff;
                        JedChecksum += (devInfo.CF[current_addr+i]>>8)  & 0xff;
                        JedChecksum += (devInfo.CF[current_addr+i])     & 0xff;
                    }

                    devInfo.CF_Line++;
                }
                else
                {
                    std::printf("CF Line = %d\n", devInfo.CF_Line);
                    CFStart = false;
                }
            }
        }
        else if ((ChecksumStart == true) &&
                 (std::strlen(tmp_buf) != 1))
        {
            ChecksumStart = false;

            copySize = indexof(tmp_buf, "*") - indexof(tmp_buf, "C") - 1;
            if (copySize < 0)
            {
                std::cerr << "Error in " << __FUNCTION__ << " in " << __LINE__ << "\n";
                return -1;
            }
            
            std::memset(data_buf, 0, sizeof(data_buf));
            std::memcpy(data_buf, &tmp_buf[1], copySize);
            devInfo.CheckSum = std::strtoul(data_buf, NULL, 16);

            std::printf("Checksum = 0x%x\n", devInfo.CheckSum);
        }
        else if (ROWStart == true)
        {
            if ((startWith(tmp_buf, TAG_ROW) == false) &&
                (std::strlen(tmp_buf) != 1))
            {
                if (startWith(tmp_buf, "E" ) == true)
                {
                    copySize = std::strlen(tmp_buf) - indexof(tmp_buf, "E") - 2;
                    if (copySize < 0)
                    {
                        std::cerr << "Error in " << __FUNCTION__ << " in " << __LINE__ << "\n";
                        return -1;
                    }
                    
                    std::memset(data_buf, 0, sizeof(data_buf));
                    std::memcpy(data_buf, &tmp_buf[1], copySize);
                    devInfo.FeatureRow = strtoul(data_buf, NULL, 2);
                }
                else
                {
                    copySize = indexof(tmp_buf, "*") - 1;
                    if (copySize < 0)
                    {
                        std::cerr << "Error in " << __FUNCTION__ << " in " << __LINE__ << "\n";
                        return -1;
                    }

                    std::memset(data_buf, 0, sizeof(data_buf));
                    std::memcpy(data_buf, &tmp_buf[2], copySize);
                    devInfo.FEARBits = strtoul(data_buf, NULL, 2);

                    std::printf("FeatureRow = 0x%x\n", devInfo.FeatureRow);
                    std::printf("FEARBits = 0x%x\n", devInfo.FEARBits);
           
                    ROWStart = false;
                }
            }
        }
        else if (VersionStart == true)
        {
            if ((startWith(tmp_buf, TAG_USERCODE) == false) &&
                (std::strlen(tmp_buf) != 1))
            {
                VersionStart = false;
                
                if (startWith(tmp_buf, "UH") == true)
                {
                    copySize = indexof(tmp_buf, "*") - indexof(tmp_buf, "H") - 1;
                    if (copySize < 0)
                    {
                        std::cerr << "Error in " << __FUNCTION__ << " in " << __LINE__ << "\n";
                        return -1;
                    }
                    
                    std::memset(data_buf, 0, sizeof(data_buf));
                    std::memcpy(data_buf, &tmp_buf[2], copySize);
                    devInfo.Version = strtoul(data_buf, NULL, 16);
                    
                    std::printf("UserCode = 0x%x\n",devInfo.Version);
                }
            }
        }
        else if (UFMStart == true)
        {
            if ((startWith(tmp_buf, TAG_UFM) == false) &&
                (startWith(tmp_buf, "L") == false) && 
                (std::strlen(tmp_buf) != 1))
            {
                if ((startWith(tmp_buf,"0") == true) ||
                    (startWith(tmp_buf,"1") == true))
                {
                    current_addr = (devInfo.UFM_Line * LatticeColSize) / 32;

                    std::memset(data_buf, 0, sizeof(data_buf));
                    std::memcpy(data_buf, tmp_buf, LatticeColSize);

                    // Convert string to byte data
                    shiftData(data_buf, &devInfo.UFM[current_addr], LatticeColSize);

                    devInfo.UFM_Line++;
                }
                else
                {
                    std::printf("UFM Line = %d\n", devInfo.UFM_Line);
                    UFMStart = false;
                }
            }
        }
    }

    JedChecksum = JedChecksum & 0xffff;

    if ((devInfo.CheckSum != JedChecksum) ||
         (devInfo.CheckSum == 0))
    {
        std::printf("CPLD JED File CheckSum Error\n");
        return -1;
    }

    std::printf("JED File CheckSum = 0x%x\n", JedChecksum);

    return 0;
}

int CpldUpdateManager::cpldUpdateCpldStart()
{
    int ret = -1;

    ret = jtag_interface_end_tap_state(jtagFd, 0, JTAG_STATE_IDLE, 2);
    if (ret < 0)
    {
        std::cerr << "Error in " << __FUNCTION__ << " in " << __LINE__ << "\n";
        return -1;
    }

    ret = jtag_interface_sir_xfer(jtagFd, JTAG_STATE_IDLE,
                                  LatticeInsLength, Lcmxo2IscEnableX);
    if (ret < 0)
    {
        std::cerr << "Error in " << __FUNCTION__ << " in " << __LINE__ << "\n";
        return -1;
    }

    unsigned int dr_data = 0x08;
    ret = jtag_interface_tdi_xfer(jtagFd, JTAG_STATE_IDLE, 8, &dr_data);
    if (ret < 0)
    {
        std::cerr << "Error in " << __FUNCTION__ << " in " << __LINE__ << "\n";
        return -1;
    }

    ret = jtag_interface_end_tap_state(jtagFd, 0, JTAG_STATE_IDLE, 2);
    if (ret < 0)
    {
        std::cerr << "Error in " << __FUNCTION__ << " in " << __LINE__ << "\n";
        return -1;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    unsigned int status = 0;
    ret = checkDeviceStatus(CHECK_BUSY, status);
    if (ret < 0)
    {
        std::printf("[%s][0x%x] Device Busy!\n", __FUNCTION__, status);
        return -1;
    }

    ret = checkDeviceStatus(CHECK_STATUS, status);
    if (ret < 0)
    {
        std::printf("[%s][0x%x] Status Busy!\n", __FUNCTION__, status);
        return -1;
    }

    return 0;
}

int CpldUpdateManager::cpldUpdateCpldErase()
{
    int ret = -1;
    unsigned int status = 0;

    ret = jtag_interface_end_tap_state(jtagFd, 0, JTAG_STATE_IDLE, 2);
    if (ret < 0)
    {
        std::cerr << "Error in " << __FUNCTION__ << " in " << __LINE__ << "\n";
        return -1;
    }

    ret = jtag_interface_sir_xfer(jtagFd, JTAG_STATE_IDLE,
                                  LatticeInsLength, Lcmxo2IscErase);
    if (ret < 0)
    {
        std::cerr << "Error in " << __FUNCTION__ << " in " << __LINE__ << "\n";
        return -1;
    }

    std::vector<unsigned int> dr_data(4,0);


    if (0 == devInfo.UFM_Line)
    {
        dr_data.at(0) = 0x04;
    }
    else
    {
        dr_data.at(0) = 0x0C;
    }

    ret = jtag_interface_tdi_xfer(jtagFd, JTAG_STATE_IDLE, 8, dr_data.data());
    if (ret < 0)
    {
        std::cerr << "Error in " << __FUNCTION__ << " in " << __LINE__ << "\n";
        return -1;
    }
    jtag_interface_end_tap_state(jtagFd, 0, JTAG_STATE_IDLE, 2);
    std::this_thread::sleep_for(std::chrono::seconds(10));

    // unsigned int status = 0;
    ret = checkDeviceStatus(CHECK_BUSY, status);
    if (ret < 0)
    {
        std::printf("[%s][0x%x] Device Busy!\n", __FUNCTION__, status);
        return -1;
    }

    ret = checkDeviceStatus(CHECK_STATUS, status);
    if (ret < 0)
    {
        std::printf("[%s][0x%x] Status Busy!\n", __FUNCTION__, status);
        return -1;
    }

    return 0;
}

int CpldUpdateManager::cpldUpdateCpldProgram()
{
    int ret = -1;
    unsigned int status = 0;

    ret = jtag_interface_end_tap_state(jtagFd, 0, JTAG_STATE_IDLE, 2);
    if (ret < 0)
    {
        std::cerr << "Error in " << __FUNCTION__ << " in " << __LINE__ << "\n";
        return -1;
    }

    ret = jtag_interface_sir_xfer(jtagFd, JTAG_STATE_IDLE,
                                  LatticeInsLength, Lcmxo2LscInitAddr);
    if (ret < 0)
    {
        std::cerr << "Error in " << __FUNCTION__ << " in " << __LINE__ << "\n";
        return -1;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    ret = SendCFdata();
    if (ret < 0)
    {
        std::cerr << "Error in " << __FUNCTION__ << " in " << __LINE__ << "\n";
        return -1;
    }

    if (0 != devInfo.UFM_Line)
    {
        ret = jtag_interface_end_tap_state(jtagFd, 0, JTAG_STATE_IDLE, 2);
        if (ret < 0)
        {
            std::cerr << "Error in " << __FUNCTION__ << " in " << __LINE__ << "\n";
            return -1;
        }

        ret = jtag_interface_sir_xfer(jtagFd, JTAG_STATE_IDLE,
                                      LatticeInsLength, Lcmxo2LscInitAddrUfm);
        if (ret < 0)
        {
            std::cerr << "Error in " << __FUNCTION__ << " in " << __LINE__ << "\n";
            return -1;
        }

        ret = SendUFMdata();
        if (ret < 0)
        {
            std::cerr << "Error in " << __FUNCTION__ << " in " << __LINE__ << "\n";
            return -1;
        }
    }

    // unsigned int status = 0;
    ret = checkDeviceStatus(CHECK_STATUS, status);
    if (ret < 0)
    {
        std::printf("[%s][0x%x] Status Busy!\n", __FUNCTION__, status);
        return -1;
    }

    ret = jtag_interface_end_tap_state(jtagFd, 0, JTAG_STATE_IDLE, 2);
    if (ret < 0)
    {
        std::cerr << "Error in " << __FUNCTION__ << " in " << __LINE__ << "\n";
        return -1;
    }

    ret = jtag_interface_sir_xfer(jtagFd, JTAG_STATE_IDLE,
                                  LatticeInsLength, Lcmxo2Usercode);
    if (ret < 0)
    {
        std::cerr << "Error in " << __FUNCTION__ << " in " << __LINE__ << "\n";
        return -1;
    }

    // Write UserCode
    std::vector<unsigned int> dr_data(4,0);
    dr_data.at(0) = devInfo.Version;
    ret = jtag_interface_tdi_xfer(jtagFd, JTAG_STATE_IDLE, 32, dr_data.data());
    if (ret < 0)
    {
        std::cerr << "Failed to write CPLD Usercode!\n";
        return -1;
    }

    ret = jtag_interface_end_tap_state(jtagFd, 0, JTAG_STATE_IDLE, 2);
    if (ret < 0)
    {
        std::cerr << "Error in " << __FUNCTION__ << " in " << __LINE__ << "\n";
        return -1;
    }

    ret = jtag_interface_sir_xfer(jtagFd, JTAG_STATE_IDLE,
                                  LatticeInsLength, Lcmxo2IscProgUsercode);
    if (ret < 0)
    {
        std::cerr << "Error in " << __FUNCTION__ << " in " << __LINE__ << "\n";
        return -1;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    ret = checkDeviceStatus(CHECK_STATUS, status);
    if (ret < 0)
    {
        std::printf("[%s][0x%x] Status Busy!\n", __FUNCTION__, status);
        return -1;
    }

    return 0;
}

int CpldUpdateManager::cpldUpdateCpldVerify()
{
    int ret = -1;
    unsigned int status = 0;

    ret = jtag_interface_end_tap_state(jtagFd, 0, JTAG_STATE_IDLE, 2);
    if (ret < 0)
    {
        std::cerr << "Error in " << __FUNCTION__ << " in " << __LINE__ << "\n";
        // return -1;
    }

    ret = jtag_interface_sir_xfer(jtagFd, JTAG_STATE_IDLE,
                                  LatticeInsLength, Lcmxo2LscInitAddr);
    if (ret < 0)
    {
        std::cerr << "Error in " << __FUNCTION__ << " in " << __LINE__ << "\n";
        // return -1;
    }

    std::vector<unsigned int> dr_data(4,0);
    dr_data.at(0) = 0x04;
    ret = jtag_interface_tdi_xfer(jtagFd, JTAG_STATE_IDLE, 8, dr_data.data());
    if (ret < 0)
    {
        std::cerr << "Failed to write CPLD Usercode!\n";
        // return -1;
    }

    ret = jtag_interface_end_tap_state(jtagFd, 0, JTAG_STATE_IDLE, 2);
    if (ret < 0)
    {
        std::cerr << "Error in " << __FUNCTION__ << " in " << __LINE__ << "\n";
        // return -1;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(2));

    ret = jtag_interface_sir_xfer(jtagFd, JTAG_STATE_IDLE,
                                  LatticeInsLength, Lcmxo2LscReadIncrNv);
    if (ret < 0)
    {
        std::cerr << "Error in " << __FUNCTION__ << " in " << __LINE__ << "\n";
        // return -1;
    }

    ret = jtag_interface_end_tap_state(jtagFd, 0, JTAG_STATE_IDLE, 2);
    if (ret < 0)
    {
        std::cerr << "Error in " << __FUNCTION__ << " in " << __LINE__ << "\n";
        // return -1;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(2));

    int current_addr = 0;
    int result = 0;

    for(int i=0; i<devInfo.CF_Line; i++)
    {
        current_addr = (i * LatticeColSize) / 32;
        std::memset(dr_data.data(), 0, dr_data.size());

        ret = jtag_interface_tdo_xfer(jtagFd, JTAG_STATE_IDLE,
                                      LatticeColSize, dr_data.data());
        if (ret < 0)
        {
            std::cerr << "Error in " << __FUNCTION__ << " in " << __LINE__ << "\n";
            // return -1;
        }

        if (dr_data.at(0) != devInfo.CF[current_addr + 0] ||
            dr_data.at(1) != devInfo.CF[current_addr + 1] ||
            dr_data.at(2) != devInfo.CF[current_addr + 2] ||
            dr_data.at(3) != devInfo.CF[current_addr + 3])
        {
            std::printf("\nPage-%d (%x %x %x %x) did not match with CF (%x %x %x %x)\n",
                        i, dr_data.at(0), dr_data.at(1), dr_data.at(2), dr_data.at(3),
                        devInfo.CF[current_addr], devInfo.CF[current_addr+1],
                        devInfo.CF[current_addr+2], devInfo.CF[current_addr+3]);
            // return -1;
            break;
        }

        if (ret < 0)
        {
            std::cerr << "Error in " << __FUNCTION__ << " in " << __LINE__ << "\n";
            break;
        }

        ret = jtag_interface_end_tap_state(jtagFd, 0, JTAG_STATE_IDLE, 2);
        if (ret < 0)
        {
            std::cerr << "Error in " << __FUNCTION__ << " in " << __LINE__ << "\n";
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    ret = checkDeviceStatus(CHECK_STATUS, status);
    if (ret < 0)
    {
        std::printf("[%s][0x%x] Status Busy!\n", __FUNCTION__, status);
        return -1;
    }

    // UserCode (0xC0)
    ret = jtag_interface_sir_xfer(jtagFd, JTAG_STATE_IDLE,
                                  LatticeInsLength, Lcmxo2Usercode);
    if (ret < 0)
    {
        std::cerr << "Error in " << __FUNCTION__ << " in " << __LINE__ << "\n";
    }

    ret = jtag_interface_end_tap_state(jtagFd, 0, JTAG_STATE_IDLE, 2);
    if (ret < 0)
    {
        std::cerr << "Error in " << __FUNCTION__ << " in " << __LINE__ << "\n";
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(2));

    std::vector<unsigned int> Usercode(1,0);
    ret = jtag_interface_tdo_xfer(jtagFd, JTAG_STATE_IDLE, 16, Usercode.data());
    if (ret < 0)
    {
        std::cerr << "Error in " << __FUNCTION__ << " in " << __LINE__ << "\n";
    }
    std::printf("New Usercode = [0x%x]\n", Usercode.at(0));

    return 0;
}

int CpldUpdateManager::cpldUpdateCpldEnd()
{
    int ret = -1;
    unsigned int status = 0;

    ret = jtag_interface_end_tap_state(jtagFd, 0, JTAG_STATE_IDLE, 2);
    if (ret < 0)
    {
        std::cerr << "Error in " << __FUNCTION__ << " in " << __LINE__ << "\n";
        // return -1;
    }

    ret = jtag_interface_sir_xfer(jtagFd, JTAG_STATE_IDLE,
                                  LatticeInsLength, Lcmxo2IscProgDone);
    if (ret < 0)
    {
        std::cerr << "Error in " << __FUNCTION__ << " in " << __LINE__ << "\n";
        // return -1;
    }

    ret = jtag_interface_end_tap_state(jtagFd, 0, JTAG_STATE_IDLE, 2);
    if (ret < 0)
    {
        std::cerr << "Error in " << __FUNCTION__ << " in " << __LINE__ << "\n";
        // return -1;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    // Check busy
    ret = checkDeviceStatus(CHECK_BUSY, status);
    if (ret < 0)
    {
        std::printf("[%s][0x%x] Device Busy!\n", __FUNCTION__, status);
        return -1;
    }

    // Check status
    ret = checkDeviceStatus(CHECK_STATUS, status);
    if (ret < 0)
    {
        std::printf("[%s][0x%x] Status Busy!\n", __FUNCTION__, status);
        return -1;
    }

    // Send ByPass instruction
    ret = jtag_interface_sir_xfer(jtagFd, JTAG_STATE_IDLE,
                                 LatticeInsLength, Lcmxo2ByPass);
    if (ret < 0)
    {
        std::cerr << "Error in " << __FUNCTION__ << " in " << __LINE__ << "\n";
        // return -1;
    }

    ret = jtag_interface_end_tap_state(jtagFd, 0, JTAG_STATE_IDLE, 2);
    if (ret < 0)
    {
        std::cerr << "Error in " << __FUNCTION__ << " in " << __LINE__ << "\n";
        // return -1;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    // Exit the programming mode
    ret = jtag_interface_sir_xfer(jtagFd, JTAG_STATE_IDLE,
                                  LatticeInsLength, Lcmxo2IscDisable);
    if (ret < 0)
    {
        std::cerr << "Error in " << __FUNCTION__ << " in " << __LINE__ << "\n";
        // return -1;
    }

    ret = jtag_interface_end_tap_state(jtagFd, 0, JTAG_STATE_IDLE, 2);
    if (ret < 0)
    {
        std::cerr << "Error in " << __FUNCTION__ << " in " << __LINE__ << "\n";
        // return -1;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(60));

    // Check status
    status = 0;
    ret = checkDeviceStatus(CHECK_STATUS, status);
    if (ret < 0)
    {
        std::printf("[%s][0x%x] Status Busy!\n", __FUNCTION__, status);
        // return -1;
    }

    // Send ByPass instruction
    ret = jtag_interface_sir_xfer(jtagFd, JTAG_STATE_IDLE,
                                  LatticeInsLength, Lcmxo2ByPass);
    if (ret < 0)
    {
        std::cerr << "Error in " << __FUNCTION__ << " in " << __LINE__ << "\n";
        // return -1;
    }

    ret = jtag_interface_end_tap_state(jtagFd, 0, JTAG_STATE_IDLE, 2);
    if (ret < 0)
    {
        std::cerr << "Error in " << __FUNCTION__ << " in " << __LINE__ << "\n";
        // return -1;
    }
    // std::this_thread::sleep_for(std::chrono::milliseconds(2));

    return 0;
}
