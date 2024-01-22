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

#include <cstdint>

constexpr auto rngReg = 0x1e6e2078;
constexpr auto nodeSize = 6;
constexpr auto guidLength = 16;
constexpr auto mbFruIndex = 0;
constexpr auto macAddressOffset = 0x1900;
constexpr auto deviceGuidOffset = 0x1920;
constexpr auto systemGuidOffset = 0x1940;

constexpr uint16_t guidGeneration = 0x1;

constexpr auto guidDevice = 0x00;
constexpr auto guidSystem = 0x01;

struct Node_T
{
    uint8_t node[nodeSize];
};

struct IPMI_GUID_T
{
    Node_T nodeID;
    uint8_t clockSeqAndReserved[2];
    uint8_t timeHighAndVersion[2];
    uint8_t timeMid[2];
    uint8_t timeLow[4];
};
