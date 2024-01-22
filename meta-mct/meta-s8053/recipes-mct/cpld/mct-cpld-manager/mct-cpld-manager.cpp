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

#include "mct-cpld-manager.hpp"

static constexpr bool DEBUG = false;

int writeReaI2cCmd(uint8_t bus, uint8_t addr,
                    std::vector<uint8_t> writeData, uint8_t readDateSize,
                    std::vector<uint8_t> *readData)
{
    std::vector<char> filename;
    filename.assign(20, 0);

    int fd = open_i2c_dev(bus, filename.data(), filename.size(), 0);

    if (fd < 0)
    {
        std::cerr << "Fail to open I2C device: " << bus << "\n";
        return -1;
    }

    int res = i2c_master_write_read(fd, addr, writeData.size(), writeData.data(),
                                    readData->size(), readData->data());
    if (res < 0)
    {
        std::cerr << "Fail to r/w I2C device: " << +bus
                  << ", Addr: " << +addr << "\n";
        close_i2c_dev(fd);
        return -1;
    }

    close_i2c_dev(fd);
    return 0;
}

int buttonLockControl(uint8_t butonCtl)
{
    std::vector<uint8_t> cmd = {cpldCommand::CMD_BUTTON_CTL, butonCtl};
    uint8_t readSize = 1;

    std::vector<uint8_t> readBuf;
    readBuf.assign(readSize, 0);

    writeReaI2cCmd(dcScmCpld::bus, dcScmCpld::address, cmd, readBuf.size(), &readBuf);

    return readBuf.at(0);
}

int buttonLockMethod(void)
{
    return buttonLockControl(button::REG_BTN_LOCK);
}

int buttonUnlockMethod(void)
{
    return buttonLockControl(button::REG_BTN_UNLOCK);
}


void dbusServiceInitialize()
{
    bus->request_name(cpld::busName);
    sdbusplus::asio::object_server objServer(bus);
    iface=objServer.add_interface(cpld::path,cpld::interface);

    iface->register_method(cpld::buttonLockMethod, buttonLockMethod);
    iface->register_method(cpld::buttonUnlockMethod, buttonUnlockMethod);

    iface->initialize();
}

void cpldInit()
{
    std::cerr << "Setting button control: " << cpld::buttonLockMethod << std::endl;
    buttonLockMethod();
}

int main(int argc, char *argv[])
{
    boost::asio::io_context io;
    bus = std::make_shared<sdbusplus::asio::connection>(io);

    dbusServiceInitialize();
    cpldInit();

    io.run();
    return 0;
}