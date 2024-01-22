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

#include "mct-sku-manager.hpp"

static constexpr bool DEBUG = false;


int i2cWriteRead(std::string i2cBus, const uint8_t slaveAddr,
                      std::vector<uint8_t> writeData,
                      std::vector<uint8_t>& readBuf)
{
    // Open the i2c device, for low-level combined data write/read
    int i2cDev = ::open(i2cBus.c_str(), O_RDWR | O_CLOEXEC);
    if (i2cDev < 0)
    {
        std::cerr << "Failed to open i2c bus. BUS=" << i2cBus << std::endl;
        return -1;
    }

    const size_t writeCount = writeData.size();
    const size_t readCount = readBuf.size();
    int msgCount = 0;
    i2c_msg i2cmsg[2] = {0};
    if (writeCount)
    {
        // Data will be writtern to the slave address
        i2cmsg[msgCount].addr = slaveAddr;
        i2cmsg[msgCount].flags = 0x00;
        i2cmsg[msgCount].len = writeCount;
        i2cmsg[msgCount].buf = writeData.data();
        msgCount++;
    }
    if (readCount)
    {
        // Data will be read into the buffer from the slave address
        i2cmsg[msgCount].addr = slaveAddr;
        i2cmsg[msgCount].flags = I2C_M_RD;
        i2cmsg[msgCount].len = readCount;
        i2cmsg[msgCount].buf = readBuf.data();
        msgCount++;
    }

    i2c_rdwr_ioctl_data msgReadWrite = {0};
    msgReadWrite.msgs = i2cmsg;
    msgReadWrite.nmsgs = msgCount;

    // Perform the combined write/read
    int ret = ::ioctl(i2cDev, I2C_RDWR, &msgReadWrite);
    ::close(i2cDev);

    if (ret < 0)
    {
        std::cerr << "I2C WR Failed! RET=" << std::to_string(ret) << std::endl;
        return -1;
    }
    if (readCount)
    {
        readBuf.resize(msgReadWrite.msgs[msgCount - 1].len);
    }

    return 0;
}

void currentSkuInit()
{
    uint8_t readCount = 0x01;

    std::string i2cBus =
        "/dev/i2c-" + std::to_string(static_cast<uint8_t>(BUS_ID));

    std::vector<uint8_t> readBuf(readCount);
    std::vector<uint8_t> writeDataWithOffset;
    writeDataWithOffset.insert(writeDataWithOffset.begin(), OFFSET_LSB);
    writeDataWithOffset.insert(writeDataWithOffset.begin(), OFFSET_MSB);

    int ret = i2cWriteRead(i2cBus, static_cast<uint8_t>(SLAVE_ADDR),
                                      writeDataWithOffset, readBuf);

    if(ret < 0)
    {
        return;
    }

    switch(readBuf[0])
    {
        case 1 :
        case 2 :
            currentSku = readBuf[0];
            break;
        default:
            break;
    }

    std::cerr << "Current SKU ID : " << std::to_string(currentSku) << std::endl;
}

void dbusServiceInitialize()
{
    bus->request_name(sku::busName);
    sdbusplus::asio::object_server objServer(bus);
    iface=objServer.add_interface(sku::path,sku::interface);
    iface->register_property(sku::currentSkuProp, currentSku,
                             sdbusplus::asio::PropertyPermission::readOnly);
    iface->initialize();
}

int main(int argc, char *argv[])
{
    boost::asio::io_context io;
    bus = std::make_shared<sdbusplus::asio::connection>(io);

    currentSkuInit();
    dbusServiceInitialize();
    
    io.run();
    return 0;
}