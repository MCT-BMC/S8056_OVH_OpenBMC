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

#include "xyz/openbmc_project/Common/error.hpp"
#include <systemd/sd-journal.h>
#include <array>
#include <filesystem>
#include <iostream>
#include <ipmid/api.hpp>
#include <ipmid/api.hpp>
#include <ipmid/utils.hpp>
#include <phosphor-logging/log.hpp>
#include <string>
#include <variant>
#include <vector>
#include <appcommands.hpp>
#include <openbmc/libobmci2c.h>


static constexpr auto i2cbus_eeprom = "6";
static constexpr auto i2caddr_eeprom = "54";
static constexpr auto offset_devguid = 0x1920;
static constexpr auto offset_sysguid = 0x1940;
static constexpr auto guid_length = 16;

static void register_app_functions() __attribute__((constructor));

/*
    Get Self Test Result
    NetFn: App (0x6) / CMD: 0x4
*/
ipmi_ret_t ipmiSv300g3eGetSelfTestResult(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                                  ipmi_request_t request, ipmi_response_t response,
                                  ipmi_data_len_t dataLen, ipmi_context_t context)
{
    int32_t reqDataLen = (int32_t)*dataLen;
    *dataLen = 0;

    /* Data Length - 0 */
    if(reqDataLen != 0)
    {
        sd_journal_print(LOG_ERR, "[%s] invalid cmd data length %d\n",
                         __FUNCTION__, reqDataLen);
        return IPMI_CC_REQ_DATA_LEN_INVALID;
    }

    GetSelfTestResultRes* resData = reinterpret_cast<GetSelfTestResultRes*>(response);

    *dataLen = sizeof(GetSelfTestResultRes);
    resData->data1 = 0x55; // No error
    resData->data2 = 0x00;

    return IPMI_CC_OK;
}

/*
    Get Device GUID
    NetFn: App (0x6) / CMD: 0x8
*/
ipmi_ret_t ipmiSv300g3eGetDevGUID(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                                  ipmi_request_t request, ipmi_response_t response,
                                  ipmi_data_len_t dataLen, ipmi_context_t context)
{
    OEMLibGetDevGUIDResponse* res = reinterpret_cast<OEMLibGetDevGUIDResponse*>(response);
    uint8_t buffer[16] = {0};
    uint8_t status;
    *dataLen = 0;

    status = i2cEEPROMGet(i2cbus_eeprom, i2caddr_eeprom,
                        offset_devguid, guid_length, buffer);

    if (0 != status)
    {
        sd_journal_print(LOG_CRIT, "failed to access EEPROM.\n", __FUNCTION__);
        return IPMI_CC_INVALID_FIELD_REQUEST;
    }

    memcpy(res->guid, buffer, guid_length);
    *dataLen = 16;

    return IPMI_CC_OK;
}

/*
    Get System GUID
    NetFn: App (0x6) / CMD: 0x37
*/
ipmi_ret_t ipmiSv300g3eGetSysGUID(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                                  ipmi_request_t request, ipmi_response_t response,
                                  ipmi_data_len_t dataLen, ipmi_context_t context)
{
    OEMLibGetSysGUIDResponse* res = reinterpret_cast<OEMLibGetSysGUIDResponse*>(response);
    uint8_t buffer[16] = {0};
    uint8_t status;
    *dataLen = 0;

    status = i2cEEPROMGet(i2cbus_eeprom, i2caddr_eeprom,
                        offset_sysguid, guid_length, buffer);

    if (0 != status)
    {
        sd_journal_print(LOG_CRIT, "failed to access EEPROM.\n", __FUNCTION__);
        return IPMI_CC_INVALID_FIELD_REQUEST;
    }

    memcpy(res->guid, buffer, guid_length);
    *dataLen = 16;

    return IPMI_CC_OK;
}

static void register_app_functions(void)
{
    // < Get Self Test Result >
    ipmi_register_callback(NETFUN_APP, CMD_GET_SELF_TEST_RESULT, NULL,
                        ipmiSv300g3eGetSelfTestResult, PRIVILEGE_USER);

    // < Get Device Guid >
    ipmi_register_callback(NETFUN_APP, CMD_GET_DEV_GUID, NULL,
                        ipmiSv300g3eGetDevGUID, PRIVILEGE_USER);

    // < Get System Guid >
    ipmi_register_callback(NETFUN_APP, CMD_GET_SYS_GUID, NULL,
                        ipmiSv300g3eGetSysGUID, PRIVILEGE_USER);
}
