/* Copyright 2020 MCT
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include <psu-update.hpp>
#include <getopt.h>

static void usage(FILE *fp, int argc, char **argv)
{
    fprintf(fp,
            "Usage: %s [options]\n\n"
            "Options:\n"
            " -h | --help                   Print this message\n"
            " -p | --program                Program PSU and verify\n"
            " -d | --debug                  debug mode\n"
            " -P | --Progress               Progress update with version id\n"
            " -s | --select                 Select updae PSU flash \n"
            "",
            argv[0]);
}

static const char short_options [] = "dhp:P:s:";

static const struct option
    long_options [] = {
    { "help",       no_argument,        NULL,    'h' },
    { "program",    required_argument,  NULL,    'p' },
    { "debug",      no_argument,        NULL,    'd' },
    { "Progress",   required_argument,  NULL,    'P' },
    { "Select",     no_argument,        NULL,    's' },
    { 0, 0, 0, 0 }
};

int main(int argc, char *argv[])
{
    char option;
    psu_t psu;
    char in_name[100] = "";
    uint8_t psuI2cBus = 0x03;
    uint8_t psuI2cAddr = 0x59;
    std::string mfrId;
    std::string mfrModel;
    std::string psuName;
    std::string psuMonitoringService;
    int ret;

    if(argc <= 2) {
        usage(stdout, argc, argv);
        exit(EXIT_SUCCESS);
    }

    memset(&psu, 0, sizeof(psu));
    psu.program = 0; /* Setting default program progress*/
    psu.progress = 0; /* Setting default progress disable*/
    psu.select = 1; /* Setting default flashing PSU 1*/

    while ((option = getopt_long(argc, argv, short_options, long_options, NULL)) != (char) -1) {
        switch (option) {
        case 'h':
            usage(stdout, argc, argv);
            exit(EXIT_SUCCESS);
            break;
        case 'p':
            psu.program = 1;
            strcpy(in_name, optarg);
            if (!strcmp(in_name, "")) {
                printf("No input file name!\n");
                usage(stdout, argc, argv);
                exit(EXIT_FAILURE);
            }
            break;

        case 'd':
            psu.debug = 1;
            break;

        case 'P':
            psu.progress = 1;
            strcpy(psu.version_id, optarg);
            if (!strcmp(psu.version_id, "")) {
                printf("No input version id!\n");
                usage(stdout, argc, argv);
                exit(EXIT_FAILURE);
            }
            break;
        case 's':
            psu.select = atoi(optarg);
            break;
        default:
            usage(stdout, argc, argv);
            exit(EXIT_FAILURE);
        }
    }

    if(!psu.program)
    {
        printf("No updating progress requested\n");
        exit(EXIT_SUCCESS);
    }

    switch(psu.select)
    {
        case 1:
            psuI2cBus = 0x03;
            psuI2cAddr = 0x59;
            psuMonitoringService = "psu1-status-sel.service";
            break;
        case 2:
            psuI2cBus = 0x03;
            psuI2cAddr = 0x58;
            psuMonitoringService = "psu0-status-sel.service";
            break;
        default:
            printf("Please select exist PSU flash\n");
            exit(EXIT_SUCCESS);
            break;
    }

    auto current = std::chrono::system_clock::now();
    std::time_t currentTime = std::chrono::system_clock::to_time_t(current);

    std::cerr << "PSU upgrade started at " << std::ctime(&currentTime);

    if (access(in_name, F_OK) < 0)
    {
        std::cerr << "PSU image " << in_name << " doesn't exist" << std::endl;
        return -1;
    }
    else
    {
        std::cerr << "PSU image is " << in_name << std::endl;
    }


    std::pair<uint8_t,uint8_t> psuInfo(psuI2cBus,psuI2cAddr);
    auto iterPSU = PsuInfoMap.find(psuInfo);
    if(iterPSU != PsuInfoMap.end())
    {
        psuName = iterPSU->second;
    }
    else
    {
        std::cerr << "PSU busID and Slave address do not match.\n";
        return -1;
    }


    std::cerr << "Start to update "<< psuName <<".\n";
    PsuUpdateUtil::update_flash_progress(5,psu.version_id,psu.progress);
    initiateStateTransition("Off");

    PsuUpdateUtil::update_flash_progress(10,psu.version_id,psu.progress);
    setBmcService(psuMonitoringService,"StopUnit","Null");

    PsuUpdateUtil::update_flash_progress(15,psu.version_id,psu.progress);


    if (PsuUpdateUtil::getMfrId(psuI2cBus,psuI2cAddr,mfrId) < 0 || 
        PsuUpdateUtil::getMfrModel(psuI2cBus,psuI2cAddr,mfrModel) < 0)
    {
        std::cerr << "Fail to get PSU MFR ID/Model.\n";
        std::stringstream eepromPath;
        eepromPath << "/sys/bus/i2c/devices/" 
                << static_cast<int>(psuI2cBus)
                << "-00"
                << std::hex << std::setfill('0') << std::setw(2) 
                << static_cast<int>(psuI2cAddr-8)
                << std::dec
                << "/eeprom";

        mfrId = readEepromDevice(eepromPath.str(),0x00,psuSupportList);

        if(mfrId.size() == 0)
        {
            std::cerr << "Fail to get matching PSU MFR ID from EEPROM.\n";
            return -1;
        }
        std::cerr << "Get matching PSU MFR ID " << mfrId << " from EEPROM.\n";
    }

    PsuUpdateUtil::update_flash_progress(20,psu.version_id,psu.progress);

    if (mfrId.find(modeNameChicony) != std::string::npos)
    {
        ChiconyPsuUpdateManager chiconypsuUpdateManager(psuI2cBus, psuI2cAddr, in_name,
                                                        psu.progress, psu.version_id);
        ret = chiconypsuUpdateManager.fwUpdate();
    }
    else if (mfrId.find(modeNameDelta) != std::string::npos)
    {
        DeltaPsuUpdateManager deltaPsuUpdateManager(psuI2cBus, psuI2cAddr, in_name,
                                                    psu.progress, psu.version_id);
        ret = deltaPsuUpdateManager.fwUpdate();
    }
    else
    {
        std::cerr << "PSU verdor is not in support list : " << mfrId << "\n";
        return -1;
    }

    if(ret >= 0)
    {
        PsuUpdateUtil::update_flash_progress(90,psu.version_id,psu.progress);
        sleep(2);
    }

    setBmcService(psuMonitoringService,"StartUnit","Null");
    sleep(2);

    std::string selectSensor;
    switch(psu.select)
    {
        case 1:
            selectSensor = "PSU1_FW_UPDATE";
            break;
        case 2:
            selectSensor = "PSU2_FW_UPDATE";
            break;
        default:
            selectSensor = "PSU1_FW_UPDATE";
            break;
    }

    logSelEvent("Switch PSU SEL Entry",
                "/xyz/openbmc_project/sensors/versionchange/" + selectSensor,
                0x07,0x00,0xFF);

    if(ret < 0)
    {
        std::cerr << "PSU update failed..." << std::endl;
    }
    else
    {
        PsuUpdateUtil::update_flash_progress(100,psu.version_id,psu.progress);
        sleep(2);
        std::cerr << "PSU update successful..." << std::endl;
    }
    return 0;
}