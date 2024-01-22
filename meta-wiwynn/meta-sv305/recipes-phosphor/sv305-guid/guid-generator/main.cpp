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
#include "guid.hpp"

#include <openbmc/libmisc.h>
#include <uuid/uuid.h>

#include <openbmc/libsysfs.hpp>

#include <algorithm>
#include <array>
#include <iostream>
#include <random>
#include <string>
#include <vector>

static int createGUID(int offset)
{
    // Generate time-based uuid with current time.
    uuid_t uu;
    uuid_generate_time(uu);

    auto macAddress = readEeprom(mbFruIndex, macAddressOffset, nodeSize + 1);
    uint8_t checksum = zeroChecksum(macAddress.data(), nodeSize);
    if (checksum != macAddress.back())
    {
        // Mac address doesn't exist, use random number for node.
        uint8_t invalidRn = 0;
        std::array<uint32_t, 2> random;
        for (int i = 0; i < random.size(); ++i)
        {
            if (read_register(rngReg, &random[i]) < 0)
            {
                std::cerr << "Failed to generate random number\n";
                invalidRn = 1;
                break;
            }
            else if (0 == random[i])
            {
                std::cerr << "Random number is equal to 0\n";
                invalidRn = 1;
                break;
            }
        }

        // Check whether random number data equal to zero
        if (1 == invalidRn)
        {
            std::random_device rd;
            std::mt19937 generator(rd());
            for (int i = 0; i < random.size(); ++i)
            {
                random[i] = static_cast<uint32_t>(generator());
            }
        }

        auto randomBytes = reinterpret_cast<uint8_t*>(&random);
        // Node part starts from 11th byte to 16th byte.
        std::copy(randomBytes, randomBytes + nodeSize, std::begin(uu) + 10);
    }
    else
    {
        // Mac address exists, use mac address for node.
        std::copy(macAddress.begin(), macAddress.begin() + nodeSize,
                  std::begin(uu) + 10);
    }

    // Convert RFC4122 uuid to IPMI guid format.
    std::reverse(std::begin(uu), std::end(uu));

    std::array<uint8_t, guidLength + 1> writeBuffer;
    std::copy(std::begin(uu), std::end(uu), writeBuffer.begin());

    // The 17th byte of guid is checksum.
    writeBuffer.back() = zeroChecksum(writeBuffer.data(), guidLength);

    try
    {
        writeEeprom(mbFruIndex, offset, writeBuffer.data(), writeBuffer.size());
    }
    catch (const std::exception& e)
    {
        std::cerr << "Failed to write guid " << e.what() << "("
                  << strerror(errno) << ")\n";
        return -1;
    }

    return 0;
}

static int checkGUID(uint8_t guidType)
{
    uint32_t offset;
    if (guidType == guidDevice)
    {
        offset = deviceGuidOffset;
    }
    else if (guidType == guidSystem)
    {
        offset = systemGuidOffset;
    }
    else
    {
        std::cerr << "Invalid guid type entered\n";
        return -1;
    }

    try
    {
        // Guid length + 1 byte is checksum.
        auto buffer = readEeprom(mbFruIndex, offset, guidLength + 1);
        uint8_t checksum = zeroChecksum(buffer.data(), guidLength);
        if (checksum != buffer.back())
        {
            std::cerr << "Creating guid\n";
            if (createGUID(offset) < 0)
            {
                if (guidType == guidDevice)
                {
                    std::cerr << "Failed to create device guid\n";
                }
                else
                {
                    std::cerr << "Failed to create system guid\n";
                }

                return -1;
            }
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Failed to read guid " << e.what() << "("
                  << strerror(errno) << ")\n";
        return -1;
    }

    return 0;
}

int main()
{
    // Check Device GUID
    if (checkGUID(guidDevice) < 0)
    {
        return -1;
    }

    // Check System GUID
    if (checkGUID(guidSystem) < 0)
    {
        return -1;
    }

    return 0;
}
