#pragma once

#include <iostream>
#include <chrono>
#include <ctime>
#include <fstream>
#include <string>
#include <fcntl.h>
#include <openbmc/libmisc.h>
#include <chicony-psu-update-manager.hpp>
#include <delta-psu-update-manager.hpp>
#include <misc-utils.hpp>
#include <psu-update-utils.hpp>

typedef struct {
    int program;                    /* enable/disable program  */
    int debug;                      /* enable/disable debug flag */
    int progress;                   /* progress update, use the version id */
    int select;                     /* Select updae PSU flash */
    char version_id[256];         /* version id */
}psu_t;

std::map<std::pair<uint8_t,uint8_t>,std::string> PsuInfoMap = {
    {std::make_pair(0x03,0x59),"PSU1"},
    {std::make_pair(0x03,0x58),"PSU2"}
};

std::vector<std::string> psuSupportList = 
{
    modeNameDelta,
    modeNameChicony
};

std::string readEepromDevice(std::string device, uint64_t seekAddress, std::vector<std::string> psuSupportList)
{
    uint64_t maxSeekSize =  0x800;
    unsigned char* read_buf = NULL;

    int fd = open(device.c_str(), O_RDWR);

    read_buf = (unsigned char*)malloc(maxSeekSize * sizeof(unsigned char));

    lseek(fd, seekAddress, SEEK_SET);
    read(fd, read_buf, maxSeekSize);

    for(int i = 0; i < maxSeekSize; i++)
    {
        for(int j = 0; j < psuSupportList.size(); j++)
        {
            int matching = 0;
            for(int k = 0; k < psuSupportList[j].size(); k++) 
            {
                if(psuSupportList[j][k] == read_buf[i+k])
                {
                    matching++;
                }
                else
                {
                    matching = 0;
                }

                if(matching == psuSupportList[j].size())
                {
                    free(read_buf);
                    close(fd);
                    return psuSupportList[j];
                }
            }
        }
    }

    free(read_buf);
    close(fd);

    return "";
}