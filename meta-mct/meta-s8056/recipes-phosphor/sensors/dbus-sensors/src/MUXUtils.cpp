
#include "MUXUtils.hpp"

#include <iostream>

#include <fcntl.h>
#include <unistd.h>

extern "C" {
#include <sys/ioctl.h>
#include <i2c/smbus.h>
#include <linux/i2c-dev.h>
}

constexpr const bool debug = false;

MuxList resolveMuxList(std::string input)
{
    auto it = MuxTable.find(input);
    if( it != MuxTable.end()) {
        return it->second;
    }
    return Invalid;
}

int switchPCA9544(uint8_t busId, uint8_t address, uint8_t regs)
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

    if (!(funcs & I2C_FUNC_SMBUS_WRITE_BYTE))
    {
        std::cerr << " not support I2C_FUNC_SMBUS_WRITE_BYTE\n";
        close(fd);
        return -1;
    }

    int ret = i2c_smbus_write_byte(fd, regs);
    close(fd);

    if (ret < 0)
    {
        if constexpr (debug)
        {
            std::cerr << " write byte data failed at " << static_cast<int>(regs) << "\n";
        }
        return -1;
    }

    return 0;
}