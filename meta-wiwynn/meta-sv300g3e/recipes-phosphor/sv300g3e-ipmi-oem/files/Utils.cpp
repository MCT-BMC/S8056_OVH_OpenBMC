/*
// Copyright (c) 2019 Wiwynn Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

#include "Utils.hpp"
#include <nlohmann/json.hpp>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stddef.h>

vector<string> getDirFiles(string dirPath, string regexStr)
{
    vector<string> result;

    for (const auto& entry : filesystem::directory_iterator(dirPath))
    {
        // If filename matched the regular expression put it in result.
        if (regex_match(entry.path().filename().string(), regex(regexStr)))
        {
            result.emplace_back(move(entry.path().string()));
        }
    }

    return result;
}

DbusSubTree getSubTree(sdbusplus::bus::bus& bus, const std::string& pathRoot,
                       int depth, const std::string& intf)
{
    DbusSubTree subTree;
    auto subTreeMsg = bus.new_method_call(
        "xyz.openbmc_project.ObjectMapper", "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree");

    const std::vector<std::string> interfaces = {intf};

    subTreeMsg.append(pathRoot, depth, interfaces);

    sdbusplus::message::message reply;
    try
    {
        reply = bus.call(subTreeMsg);
    }
    catch (sdbusplus::exception::SdBusError& e)
    {
        sd_journal_print(LOG_ERR,
                         "GetSubTree Failed in bus call, %s\n",
                         e.what());
    }

    try
    {
        reply.read(subTree);
    }
    catch (sdbusplus::exception::SdBusError& e)
    {
        sd_journal_print(LOG_ERR,
                         "GetSubTree Failed in read reply, %s\n",
                         e.what());
    }

    return subTree;
}

bool getDimmConfig(std::vector<uint8_t>& dimmConfig, const std::string& path)
{
    static bool initialized = false;

    if(initialized)
    {
        return true;
    }

    std::ifstream dimmConfigFile(path);
    if(!dimmConfigFile)
    {
        sd_journal_print(LOG_ERR, "[%s] Failed to open DIMM config file: %s\n",
                                    __FUNCTION__, path.c_str());
        return false;
    }
    else
    {
        auto data = nlohmann::json::parse(dimmConfigFile, nullptr, false);
        if(data.is_discarded())
        {
            sd_journal_print(LOG_ERR, "[%s] Syntax error in %s\n",
                                    __FUNCTION__, path.c_str());
            return false;
        }
        else
        {
            int idx = 0;
            dimmConfig.clear();

            while(!data[idx].is_null())
            {
                if(!data[idx]["Number"].is_null() &&
                   !data[idx]["Name"].is_null())
                {
                    uint8_t dimmSensorNum = static_cast<uint8_t>(data[idx]["Number"]);
                    dimmConfig.push_back(dimmSensorNum);
                }
                else
                {
                    sd_journal_print(LOG_ERR, "[%s] Invalid DIMM configuration Data\n", __FUNCTION__);
                    return false;
                }
                idx++;
            }
        }
    }

    initialized = true;

    return true;
}

#define PATH_GPIO_ROOT "/sys/class/gpio/gpio"
#define PATH_GPIO_VAL PATH_GPIO_ROOT"%d/value"
#define PATH_GPIO_DIR PATH_GPIO_ROOT"%d/direction"
#define PATH_GPIO_EXP "/sys/class/gpio/export"
#define PATH_GPIO_UNEXP "/sys/class/gpio/unexport"

#define GPIO_DIR_OUT_STR "out"
#define GPIO_DIR_IN_STR "in"

static constexpr uint8_t MAX_RETRY = 5;
static constexpr uint32_t MAX_STRING_SIZE = 256;

//Add non-busy-wait msleep() function
void msleep(int32_t msec)
{
    struct timespec req;

    req.tv_sec = 0;
    req.tv_nsec = msec * 1000 * 1000;

    while (nanosleep(&req, &req) == -1 && errno == EINTR)
    {
        continue;
    }
}

int32_t Export_GPIO(int gpio_num)
{
    int fd, i = 0;
    char path[MAX_STRING_SIZE];

    memset(path, 0, sizeof(path));

    snprintf(path, sizeof(path),"%s%d", PATH_GPIO_ROOT, gpio_num);
    if (access(path, F_OK) != 0) // if file does not exist then export its GPIO node
    {
        for (i = 0; i < MAX_RETRY; i++)
        {
            fd = open(PATH_GPIO_EXP, O_WRONLY);
            if (fd < 0)
            {
                if (i == (MAX_RETRY - 1))
                {
                    sd_journal_print(LOG_ERR, "[%s] failed to open device %s (%s)\n",
                                     __FUNCTION__, path, strerror(errno));
                    close(fd);
                    return -1;
                }
            }
            else
            {
                break;
            }
            msleep(10);
        }
        sprintf(path, "%d", gpio_num);
        write(fd, path, strlen(path));
        close(fd);
    }
    else
    {
        return 0;
    }

    return 0;
}

int32_t Unexport_GPIO(int gpio_num)
{
    int fd, i = 0;
    char path[MAX_STRING_SIZE];

    memset(path, 0, sizeof(path));

    snprintf(path, sizeof(path),"%s%d", PATH_GPIO_ROOT, gpio_num);
    if (access(path, F_OK) == 0) // if file does exist then unexport its GPIO node
    {
        for (i = 0; i < MAX_RETRY; i++)
        {
            fd = open(PATH_GPIO_UNEXP, O_WRONLY);
            if (fd < 0)
            {
                if (i == (MAX_RETRY - 1))
                {
                    sd_journal_print(LOG_ERR, "[%s] failed to open device %s (%s)\n",
                                     __FUNCTION__, path, strerror(errno));
                    close(fd);
                    return -1;
                }
            }
            else
            {
                break;
            }
            msleep(10);
        }
        sprintf(path, "%d", gpio_num);
        write(fd, path, strlen(path));
        close(fd);
    }
    else
    {
        return 0;
    }

    return 0;
}

int32_t Get_GPIO_Value(uint16_t gpio_num, uint8_t *value)
{
    FILE *fp = NULL;
    char path[MAX_STRING_SIZE] = {0};
    int32_t tmp_value = -1;
    int32_t rc = 0;
    int32_t ret = 0;
    uint8_t i = 0;

    sprintf(path, PATH_GPIO_VAL, gpio_num);
    for (i = 0; i < MAX_RETRY; i++)
    {
        fp = fopen(path, "r");
        if (fp == NULL)
        {
            if (i == (MAX_RETRY - 1))
            {
                sd_journal_print(LOG_ERR, "[%s] failed to open device %s (%s)\n",
                                     __FUNCTION__, path, strerror(errno));
                fclose(fp);
                return -1;
            }
        }
        else
        {
            break;
        }
        msleep(10);
    }

    for (i = 0; i < MAX_RETRY; i++)
    {
        ret = 0;
        rc = fscanf(fp, "%d", &tmp_value);
        if (rc != 1)
        {
            if (i == (MAX_RETRY - 1))
            {
                sd_journal_print(LOG_ERR, "[%s] failed to read device %s (%s)\n",
                                     __FUNCTION__, path, strerror(errno));
                fclose(fp);
                return -1;
            }
        }
        else
        {
            break;
        }
        msleep(10);
    }

    fclose(fp);
    *value = (uint8_t) tmp_value;

    return ret;
}

int32_t Get_GPIO_Direction(uint16_t gpio_num, uint8_t *direction)
{
    FILE *fp = NULL;
    char path[MAX_STRING_SIZE] = {0};
    char tmp_direction[MAX_STRING_SIZE] = {0};
    int32_t rc = 0;
    int32_t ret = 0;
    uint8_t i = 0;

    sprintf(path, PATH_GPIO_DIR, gpio_num);
    for (i = 0; i < MAX_RETRY; i++)
    {
        fp = fopen(path, "r");
        if (fp == NULL)
        {
            if (i == (MAX_RETRY - 1))
            {
                sd_journal_print(LOG_ERR, "[%s] failed to close device %s (%s)\n",
                                     __FUNCTION__, path, strerror(errno));
                fclose(fp);
                return -1;
            }
        }
        else
        {
            break;
        }
        msleep(10);
    }

    for (i = 0; i < MAX_RETRY; i++)
    {
        ret = 0;
        rc = fscanf(fp, "%s", &tmp_direction);
        if (rc != 1)
        {
            if (i == (MAX_RETRY - 1))
            {
                sd_journal_print(LOG_ERR, "[%s] failed to read device %s (%s)\n",
                                     __FUNCTION__, path, strerror(errno));
                fclose(fp);
                return -1;
            }
        }
        else
        {
            break;
        }
        msleep(10);
    }

    fclose(fp);

    if (tmp_direction)
    {
        if (strncmp(tmp_direction, GPIO_DIR_OUT_STR, strlen(GPIO_DIR_OUT_STR)) == 0)
        {
            *direction = out_direction;
        }
        else if (strncmp(tmp_direction, GPIO_DIR_IN_STR, strlen(GPIO_DIR_IN_STR)) == 0)
        {
            *direction = in_direction;
        }
        else
        {
            return -1;
        }
    }

    return ret;
}

int32_t Set_GPIO_Value(uint16_t gpio_num, uint8_t value)
{
    char path[MAX_STRING_SIZE] = {0};
    FILE *fp = NULL;
    int32_t rc;
    uint8_t write_retry = 0;
    char value_str[MAX_STRING_SIZE] = {0};

    memset(path, 0, sizeof(path));

    if (value != low_value && value != high_value)
    {
        return -1;
    }

    //Set GPIO Value
    snprintf(path, sizeof(path), PATH_GPIO_VAL, gpio_num);
    while (write_retry < MAX_RETRY)
    {
        fp = fopen(path, "w");
        if (fp != NULL)
        {
            snprintf(value_str, sizeof(value_str), "%d", value);
            rc = fputs(value_str, fp);
            fclose(fp);

            if (rc < 0)
            {
                sd_journal_print(LOG_ERR, "[%s] failed to set gpio %s (%d)\n",
                                     __FUNCTION__, path, write_retry);
                msleep(10);
                write_retry++;
            }
            else
            {
                break;
            }
        }
        else
        {
            sd_journal_print(LOG_ERR, "[%s] failed to open device %s (%d)\n",
                                     __FUNCTION__, path, write_retry);
            msleep(10);
            write_retry++;
        }
    }

    if (write_retry == MAX_RETRY)
    {
        return -1;
    }

    return 0;

}

int32_t Set_GPIO_Direction(uint16_t gpio_num, uint8_t direction)
{
    char path[MAX_STRING_SIZE] = {0};
    FILE *fp = NULL;
    int32_t rc;
    uint8_t write_retry = 0;
    char value_str[MAX_STRING_SIZE] = {0};

    if (direction != in_direction && direction != out_direction)
    {
        return -1;
    }

    //Set GPIO Direction
    memset(path, 0, sizeof(path));
    snprintf(path, sizeof(path), PATH_GPIO_DIR, gpio_num);
    while (write_retry < MAX_RETRY)
    {
        fp = fopen(path, "w");
        if (fp != NULL)
        {
            if (direction == in_direction)
            {
                rc = fputs(GPIO_DIR_IN_STR, fp);
            }
            else if (direction == out_direction)
            {
                rc = fputs(GPIO_DIR_OUT_STR, fp);
            }
            else
            {
                fclose(fp);
                return -1;
            }
            fclose(fp);

            if (rc < 0)
            {
                sd_journal_print(LOG_ERR, "[%s] failed to set gpio %s (%d)\n",
                                     __FUNCTION__, path, write_retry);
                msleep(10);
                write_retry++;
            }
            else
            {
                break;
            }
        }
        else
        {
            sd_journal_print(LOG_ERR, "[%s] failed to open device %s (%d)\n",
                                     __FUNCTION__, path, write_retry);
            msleep(10);
            write_retry++;
        }
    }

    if (write_retry == MAX_RETRY)
    {
        return -1;
    }

    return 0;
}
