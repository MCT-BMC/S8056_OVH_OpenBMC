/* Copyright 2021 MCT
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

#include <iostream>
#include <string>
#include <sdbusplus/asio/object_server.hpp>

extern "C" {
    #include <linux/i2c-dev.h>
    #include <i2c/smbus.h>
}

static constexpr bool DEBUG = false;

static constexpr const uint8_t CPLD_BUS = 0x02;
static constexpr const uint8_t CPLD_ADDRESS = 0x44;

static constexpr const char* PROPERTY_INTERFACE = "org.freedesktop.DBus.Properties";

namespace cpld
{
    static constexpr const char* busName = "xyz.openbmc_project.Software.BMC.Updater";
    static constexpr const char* path = "/xyz/openbmc_project/software/cpld_active";
    static constexpr const char* interface = "xyz.openbmc_project.Software.Version";
    static constexpr const char* prop = "Version";
} // namespace bios

int updateCpldVersion(std::string version)
{
    auto bus = sdbusplus::bus::new_default();

    auto method = bus.new_method_call(cpld::busName, cpld::path, PROPERTY_INTERFACE, "Set");

    try
    {
        method.append(cpld::interface, cpld::prop, std::variant<std::string>(version));
        bus.call_noreply(method);
    }
    catch (std::exception& e)
    {
        std::cerr << "Failed to set property:" << e.what() << "\n";
        sleep(5);
        updateCpldVersion(version);
    }

    return 0;
}

int getRegsInfoByte(uint8_t busId, uint8_t address, uint8_t regs, uint8_t* data)
{
    std::string i2cBus = "/dev/i2c-" + std::to_string(busId);
    int fd = open(i2cBus.c_str(), O_RDWR);

    if (fd < 0)
    {
        std::cerr << " unable to open i2c device" << i2cBus << "  err=" << fd
                  << "\n";
        return -1;
    }

    if (ioctl(fd, I2C_SLAVE_FORCE, address) < 0)
    {
        std::cerr << " unable to set device address\n";
        close(fd);
        return -1;
    }

    unsigned long funcs = 0;
    if (ioctl(fd, I2C_FUNCS, &funcs) < 0)
    {
        std::cerr << " not support I2C_FUNCS\n";
        close(fd);
        return -1;
    }

    if (!(funcs & I2C_FUNC_SMBUS_READ_BYTE_DATA))
    {
        std::cerr << " not support I2C_FUNC_SMBUS_READ_BYTE_DATA\n";
        close(fd);
        return -1;
    }

    *data = i2c_smbus_read_byte_data(fd, regs);
    close(fd);

    if (*data < 0)
    {
        return -1;
    }

    return 0;
}

int getCpldVersion(uint8_t bus, uint8_t address, uint8_t* responseRegMSB, uint8_t* responseRegLSB)
{
    if(getRegsInfoByte(bus,address,0x01,responseRegLSB) < 0)
    {
        return -1;
    }
    if(getRegsInfoByte(bus,address,0x02,responseRegMSB) < 0)
    {
        return -1;
    }

    return 0;
}

int main(void)
{
    uint8_t responseRegMSB,responseRegLSB;

    while(1)
    {
        if(getCpldVersion(CPLD_BUS,CPLD_ADDRESS,&responseRegMSB,&responseRegLSB) == 0)
        {
            break;
        }
        sleep(10);
    }

    std::string cpldVersion = "V" + std::to_string(responseRegMSB & 0x0f) + // CPLD Major Version
                              "." + std::to_string((responseRegLSB & 0xf0) >> 4) + // CPLD Minor Version
                              "." + std::to_string(responseRegLSB & 0x0f); // CPLD Maintain Version

    if(DEBUG){
        std::cerr << "CPLD Version : " << cpldVersion << std::endl;
    }

    updateCpldVersion(cpldVersion);

    return 0;
}