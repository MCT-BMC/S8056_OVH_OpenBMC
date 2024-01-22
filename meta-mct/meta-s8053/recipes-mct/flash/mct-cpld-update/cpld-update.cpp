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

#include "cpld-update.hpp"
#include <getopt.h>

static void usage(FILE *fp, int argc, char **argv)
{
    fprintf(fp,
            "Usage: %s [options]\n\n"
            "Options:\n"
            " -h | --help                   Print this message\n"
            " -p | --program                Program cpld and verify\n"
            " -d | --debug                  debug mode\n"
            " -P | --Progress               Progress update with version id\n"
            " -b | --bus                    Select cpld device i2c bus\n"
            " -a | --address                Select cpld device i2c address\n"
            " -i | --information            Return JED file information (User code)\n"
            "",
            argv[0]);
}

static const char short_options [] = "dhp:P:b:a:i";

static const struct option
    long_options [] = {
    { "help",       no_argument,        NULL,    'h' },
    { "program",    required_argument,  NULL,    'p' },
    { "debug",      no_argument,        NULL,    'd' },
    { "Progress",   required_argument,  NULL,    'P' },
    { "Bus",        required_argument,  NULL,    'b' },
    { "Address",    required_argument,  NULL,    'a' },
    { "Information",no_argument,        NULL,    'i' },
    { 0, 0, 0, 0 }
};

#ifdef GPIO_MODE
void enableGpioToBmc()
{
    setOutput("FM_BMC_UPDATE_CPLD_SMB_OPTION",1);
}

void disableGpioToBMC()
{
    setOutput("FM_BMC_UPDATE_CPLD_SMB_OPTION",1);
}
#endif

int main(int argc, char *argv[])
{
    char option;
    cpld_t cpld;
    char in_name[100] = "";

    if(argc <= 2) {
        usage(stdout, argc, argv);
        exit(EXIT_SUCCESS);
    }

    memset(&cpld, 0, sizeof(cpld));
    cpld.program = 0; /* Setting default program progress*/
    cpld.progress = 0; /* Setting default progress disable*/
    cpld.jed_info = 0; /* Setting default JED information flag to disable*/

    while ((option = getopt_long(argc, argv, short_options, long_options, NULL)) != (char) -1) {
        switch (option) {
        case 'h':
            usage(stdout, argc, argv);
            exit(EXIT_SUCCESS);
            break;
        case 'i':
            cpld.jed_info = 1;
            break;
        case 'p':
            cpld.program = 1;
            strcpy(in_name, optarg);
            if (!strcmp(in_name, "")) {
                printf("No input file name!\n");
                usage(stdout, argc, argv);
                exit(EXIT_FAILURE);
            }
            break;

        case 'd':
            cpld.debug = 1;
            break;

        case 'P':
            cpld.progress = 1;
            strcpy(cpld.version_id, optarg);
            if (!strcmp(cpld.version_id, "")) {
                printf("No input version id!\n");
                usage(stdout, argc, argv);
                exit(EXIT_FAILURE);
            }
            break;
        case 'b':
            cpld.bus = atoi(optarg);
            break;
        case 'a':
            cpld.address = atoi(optarg);
            break;
        default:
            usage(stdout, argc, argv);
            exit(EXIT_FAILURE);
        }
    }

    if(CpldUpdateUtil::i2cLatticeCpldJedFileUserCode(in_name, cpld.version) < 0){
        printf("JED file %s is invalid file\n", in_name);
        return -1;
    }
    printf("JED file user code: 0x%08x\n", cpld.version);

    if (cpld.jed_info) {
        exit(EXIT_SUCCESS);
    }

    if(!cpld.program)
    {
        printf("No updating progress requested\n");
        exit(EXIT_SUCCESS);
    }

    auto current = std::chrono::system_clock::now();
    std::time_t currentTime = std::chrono::system_clock::to_time_t(current);

    std::cerr << "CPLD upgrade started at " << std::ctime(&currentTime);

    if (access(in_name, F_OK) < 0)
    {
        std::cerr << "CPLD image " << in_name << " doesn't exist" << std::endl;
        return -1;
    }
    else
    {
        std::cerr << "CPLD image is " << in_name << std::endl;
    }

#ifdef HOST_REBOOT_MODE
    std::cerr << "Power off host server" << std::endl;
    if(checkHostPower())
    {
        initiateStateTransition("Off");
        sleep(15);
    }

    if(checkHostPower())
    {
        std::cerr << "Host server didn't power off" << std::endl;
        std::cerr << "CPLD upgrade failed" << std::endl;
        return 0;
    }

    CpldUpdateUtil::update_flash_progress(10,cpld.version_id,cpld.progress);
    std::cerr << "Host server powered off" << std::endl;
#endif

#ifdef GPIO_MODE
    std::cerr << "Enable GPIO to access cpld device for BMC used" << std::endl;
    enableGpioToBmc();
    sleep(2);
#endif

    std::cerr << "Flashing cpld image..." << std::endl;
    int result = CpldUpdateUtil::i2cLatticeCpldFwUpdate(cpld, in_name);
    sleep(1);

#ifdef GPIO_MODE
    std::cerr << "Disable GPIO to access cpld device for BMC used" << std::endl;
    disableGpioToBMC();
#endif

    if(result != 0)
    {
        std::cerr << "CPLD updating failed..." << std::endl;
        return -1;
    }

    CpldUpdateUtil::update_flash_progress(95,cpld.version_id,cpld.progress);
    logUpdateSel(cpld.version);
    sleep(3);

    CpldUpdateUtil::update_flash_progress(100,cpld.version_id,cpld.progress);

    std::cerr << "CPLD updated successfully..." << std::endl;
    std::cerr << "Please do the AC cycle for finishing cpld update" << std::endl;

    return 0;
}