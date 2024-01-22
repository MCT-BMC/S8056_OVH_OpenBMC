/* Copyright 2022-present MCT
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

#include "alt-bmc-update.hpp"
#include <getopt.h>

typedef struct {
    int program;                    /* enable/disable program  */
    int debug;                      /* enable/disable debug flag */
    int progress;                   /* progress update, use the version id */
    char version_id[256];         /* version id */
}bmc_t;

static void usage(FILE *fp, int argc, char **argv)
{
    fprintf(fp,
            "Usage: %s [options]\n\n"
            "Options:\n"
            " -h | --help                   Print this message\n"
            " -p | --program                Program alternate bmc and verify\n"
            " -d | --debug                  debug mode\n"
            " -P | --Progress               Progress update with version id\n"
            "",
            argv[0]);
}

static const char short_options [] = "dhp:P:";

static const struct option
    long_options [] = {
    { "help",       no_argument,        NULL,    'h' },
    { "program",    required_argument,  NULL,    'p' },
    { "debug",      no_argument,        NULL,    'd' },
    { "Progress",   required_argument,  NULL,    'P' },
    { 0, 0, 0, 0 }
};

static void update_flash_progress(int pecent, const char *version_id, int progress)
{
    if(progress){
        FILE * fp;
        char buf[100];
        sprintf(buf, "flash-progress-update %s %d", version_id, pecent);
        if ((fp = popen(buf, "r")) == NULL){
            return ;
        }
        pclose(fp);
    }
    return;
}

void setUpdateProgress(bmc_t* bmc, char* input, uint8_t base, uint8_t divisor)
{
    std::string buffer(input);
    int start = buffer.find("(", 0);
    int end = buffer.find("%", 0);
    int progress = base + (atoi(buffer.substr(start+1,end-start-1).c_str())/ divisor);
    update_flash_progress(progress,bmc->version_id,bmc->progress);
}

int bmcUpdate(bmc_t* bmc, char* image_str)
{
    FILE *fp;
    char buffer[100];

    std::string flashcp_str = "/usr/sbin/flashcp -v ";
    std::string erase = "Erasing";
    std::string write = "Writing";
    std::string verify = "Verifying";

    uint8_t base = 30;
    uint8_t divisor = 5;
    uint8_t delay = 16;
    uint8_t counter = 0;

    flashcp_str = flashcp_str + image_str + " /dev/mtd/alt-bmc";

    fp = popen(flashcp_str.c_str(), "r");

    if (fp == NULL) {
        return -1;
    }

    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        if(counter==0){
            counter = delay;
            printf("%s\n",buffer);
            if(bmc->progress){
                if(strstr(buffer, erase.c_str()) != NULL)
                {
                    setUpdateProgress(bmc,buffer,base,divisor);
                }
                else if(strstr(buffer, write.c_str()) != NULL)
                {
                    setUpdateProgress(bmc,buffer,base+(100/divisor),divisor);
                }
                else if(strstr(buffer, verify.c_str()) != NULL)
                {
                    setUpdateProgress(bmc,buffer,base+((100/divisor)*2),divisor);
                }
            }
            printf("\033[1A");
            printf("\033[K");
        }
        counter--;
    }

    pclose(fp);

    return 0;
}

void logBmcUpdateSel()
{
    static const std::string bootSourceFilePath = "/run/boot-source";
    uint8_t bootSource = 0;
     std::string selectSensor;

    std::ifstream ifs(bootSourceFilePath);
    std::string line;
    getline(ifs, line);
    bootSource = std::stoi(line);

    switch(bootSource)
    {
        case 0:
            //Current using BMC flash device: BMC1
            selectSensor = "BMC2_FW_UPDATE";
            break;
        case 1:
             //Current using BMC flash device: BMC2
            selectSensor = "BMC1_FW_UPDATE";
            break;
        default:
            selectSensor = "BMC1_FW_UPDATE";
            break;
    }

    logSelEvent("Switch BMC SEL Entry",
        "/xyz/openbmc_project/sensors/versionchange/" + selectSensor,
        0x07,0x00,0xFF);
}


int main(int argc, char *argv[])
{
    char option;
    bmc_t bmc;
    char in_name[100] = "";

    if(argc <= 2) {
        usage(stdout, argc, argv);
        exit(EXIT_SUCCESS);
    }

    memset(&bmc, 0, sizeof(bmc));
    bmc.program = 0; /* Setting default program progress*/
    bmc.progress = 0; /* Setting default progress disable*/

    while ((option = getopt_long(argc, argv, short_options, long_options, NULL)) != (char) -1) {
        switch (option) {
        case 'h':
            usage(stdout, argc, argv);
            exit(EXIT_SUCCESS);
            break;
        case 'p':
            bmc.program = 1;
            strcpy(in_name, optarg);
            if (!strcmp(in_name, "")) {
                printf("No input file name!\n");
                usage(stdout, argc, argv);
                exit(EXIT_FAILURE);
            }
            break;

        case 'd':
            bmc.debug = 1;
            break;

        case 'P':
            bmc.progress = 1;
            strcpy(bmc.version_id, optarg);
            if (!strcmp(bmc.version_id, "")) {
                printf("No input version id!\n");
                usage(stdout, argc, argv);
                exit(EXIT_FAILURE);
            }
            break;
        default:
            usage(stdout, argc, argv);
            exit(EXIT_FAILURE);
        }
    }

    if(!bmc.program)
    {
        printf("No updating progress requested\n");
        exit(EXIT_SUCCESS);
    }

    auto current = std::chrono::system_clock::now();
    std::time_t currentTime = std::chrono::system_clock::to_time_t(current);

    std::cerr << "Bmc upgrade started at " << std::ctime(&currentTime);

    if (access(in_name, F_OK) < 0)
    {
        std::cerr << "Bmc image " << in_name << " doesn't exist" << std::endl;
        return -1;
    }
    else
    {
        std::cerr << "Bmc image is " << in_name << std::endl;
    }
    
    std::cerr << "Flashing bmc image..." << std::endl;
    update_flash_progress(30,bmc.version_id,bmc.progress);
    bmcUpdate(&bmc,in_name);
    sleep(1);

    logBmcUpdateSel();

    update_flash_progress(100,bmc.version_id,bmc.progress);

    std::cerr << "Bmc updated successfully..." << std::endl;

    return 0;
}