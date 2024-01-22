/* 
// Copyright (c) 2020 Wiwynn Corporation
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

#pragma once

static constexpr const char* i2cbus_eeprom = "6";
static constexpr const char* i2caddr_eeprom = "54";
static constexpr auto offset_devguid = 0x1920;
static constexpr auto offset_sysguid = 0x1940;
static constexpr auto device_guid = 0;
static constexpr auto system_guid = 1;
static constexpr auto guid_generation = 0x1;

typedef struct
{
    uint8_t node[6]; 
}__attribute__((packed)) Node_T;

typedef struct
{
    Node_T nodeID;
    uint16_t clock_seq_resved;
    uint16_t time_high_ver;
    uint16_t time_mid;
    uint32_t time_low;
}__attribute__((packed)) IPMI_GUID_T;

