/* Copyright 2022 MCT
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

#include "vr-update.hpp"
#include <getopt.h>

static void usage(FILE *fp, int argc, char **argv)
{
    fprintf(fp,
            "Usage: %s [options]\n\n"
            "Options:\n"
            " -h | --help                   Print this message\n"
            " -p | --program                Program vr and verify\n"
            " -d | --debug                  debug mode\n"
            " -P | --Progress               Progress update with version id\n"
            " -s | --select                 Select updae VR devcie \n"
            " -c | --crc                    Get current VR firmware CRC value \n"
            "",
            argv[0]);
}

static const char short_options [] = "dhp:P:s:c";

static const struct option
    long_options [] = {
    { "help",       no_argument,        NULL,    'h' },
    { "program",    required_argument,  NULL,    'p' },
    { "debug",      no_argument,        NULL,    'd' },
    { "Progress",   required_argument,  NULL,    'P' },
    { "Select",     required_argument,  NULL,    's' },
    { "crc",        no_argument,        NULL,    'c' },
    { 0, 0, 0, 0 }
};

int main(int argc, char *argv[])
{
    char option;
    vr_t vr;
    char in_name[100] = "";
    int ret;

    if(argc <= 2) {
        usage(stdout, argc, argv);
        exit(EXIT_SUCCESS);
    }

    memset(&vr, 0, sizeof(vr));
    vr.program = 0; /* Setting default program progress*/
    vr.progress = 0; /* Setting default progress disable*/
    vr.select = 0; /* Setting default select device disable*/
    vr.crc = 0; /* Setting default getting current VR firmware CRC value disable*/
    strcpy(vr.selectDevice, ""); /* Setting default select device disable*/

    while ((option = getopt_long(argc, argv, short_options, long_options, NULL)) != (char) -1) {
        switch (option) {
        case 'h':
            usage(stdout, argc, argv);
            exit(EXIT_SUCCESS);
            break;
        case 'p':
            vr.program = 1;
            strcpy(in_name, optarg);
            if (!strcmp(in_name, "")) {
                printf("No input file name!\n");
                usage(stdout, argc, argv);
                exit(EXIT_FAILURE);
            }
            break;

        case 'd':
            vr.debug = 1;
            break;

        case 'P':
            vr.progress = 1;
            strcpy(vr.version_id, optarg);
            if (!strcmp(vr.version_id, "")) {
                printf("No input version id!\n");
                usage(stdout, argc, argv);
                exit(EXIT_FAILURE);
            }
            break;
        case 's':
            vr.select = atoi(optarg);
            switch(vr.select)
            {
                case 1:
                    strcpy(vr.selectDevice, "P0_VDDCR_SOC");
                    break;
                case 2:
                    strcpy(vr.selectDevice, "P0_VDD11");
                    break;
                case 3:
                    strcpy(vr.selectDevice, "P0_VDDIO");
                    break;
            }
            break;
        case 'c':
            vr.crc = 1;
            break;
        default:
            usage(stdout, argc, argv);
            exit(EXIT_FAILURE);
        }
    }

    if(!vr.program)
    {
        printf("No updating progress requested\n");
        exit(EXIT_SUCCESS);
    }


    auto devicesIt = supportedDevice.find(vr.selectDevice);
    if (devicesIt == supportedDevice.end())
    {
        std::cerr << "Device not supported!\n";
        std::cout << "  Supported VR device:\n";
        std::cout << "    01h - P0_VDDCR_SOC\n";
        std::cout << "    02h - P0_VDD11\n";
        std::cout << "    03h - P0_VDDIO\n";
        return -1;
    }

    std::cerr << "Select VR device: " << vr.selectDevice << std::endl;

    auto current = std::chrono::system_clock::now();
    std::time_t currentTime = std::chrono::system_clock::to_time_t(current);

    VrUpdateManager vrUpdateManager(devicesIt->second.bus, devicesIt->second.pmbusAddr, in_name,
                        devicesIt->second.devcieId, vr.progress, vr.version_id);
    if(vr.crc)
    {
        ret = vrUpdateManager.getCurretVrFwCrc();

        if(ret < 0)
        {
            std::cerr << "Get current VR firmware CRC value failed..." << std::endl;
            return -1;
        }
        return 0;
    }

    std::cerr << "VR upgrade started at " << std::ctime(&currentTime);

    if (access(in_name, F_OK) < 0)
    {
        std::cerr << "VR image " << in_name << " doesn't exist" << std::endl;
        return -1;
    }
    else
    {
        std::cerr << "VR image is " << in_name << std::endl;
    }

    std::cerr << "Check host power is enabled..." << std::endl;
    if(!isPowerOn())
    {
        std::cerr << "Waiting for host power on..." << std::endl;
        initiateStateTransition("On");
        sleep(2);
    }

    std::cerr << "Flashing VR image..." << std::endl;
    VrUpdateUtil::update_flash_progress(20,vr.version_id,vr.progress);

    ret = vrUpdateManager.fwUpdate();

    if(ret < 0)
    {
        std::cerr << "VR firmware update failed..." << std::endl;
        return -1;
    }

    std::string finishInfo =  "Please do the AC cycle for finishing VR update.";
    VrUpdateUtil::update_flash_progress(95,vr.version_id,vr.progress,finishInfo);
    sleep(2);

    logVrUpdateSel(devicesIt->second.selDeviceType);

    VrUpdateUtil::update_flash_progress(100,vr.version_id,vr.progress);
    std::cerr << "VR updated successfully..." << std::endl;
    std::cerr << finishInfo << std::endl;

    return 0;
}