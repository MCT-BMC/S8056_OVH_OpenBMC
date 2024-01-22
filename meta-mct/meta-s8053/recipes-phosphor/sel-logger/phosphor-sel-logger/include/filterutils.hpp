#include <iostream>

#include <sdbusplus/bus.hpp>
#include <openbmc/obmc-i2c.h>

extern "C" {
    #include <linux/i2c-dev.h>
    #include <i2c/smbus.h>
}

static constexpr bool DEBUG = false;

int getRegsInfoWord(uint8_t busId, uint8_t address, uint8_t regs, int16_t* pu16data)
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

    if (!(funcs & I2C_FUNC_SMBUS_READ_WORD_DATA))
    {
        std::cerr << " not support I2C_FUNC_SMBUS_READ_WORD_DATA\n";
        close(fd);
        return -1;
    }

    *pu16data = i2c_smbus_read_word_data(fd, regs);
    close(fd);

    if (*pu16data < 0)
    {
        return -1;
    }

    return 0;
}

int setBmcService(std::string service,std::string status, std::string setting)
{
    constexpr auto SYSTEMD_SERVICE = "org.freedesktop.systemd1";
    constexpr auto SYSTEMD_OBJ_PATH = "/org/freedesktop/systemd1";
    constexpr auto SYSTEMD_INTERFACE = "org.freedesktop.systemd1.Manager";

    auto bus = sdbusplus::bus::new_default();

    // start/stop servcie
    if(status != "Null"){
        auto method = bus.new_method_call(SYSTEMD_SERVICE, SYSTEMD_OBJ_PATH, SYSTEMD_INTERFACE, status.c_str());
        method.append(service, "replace");

        try
        {
            auto reply = bus.call(method);
        }
        catch (const sdbusplus::exception::SdBusError& e)
        {
            std::cerr << "Error in start/stop servcie. ERROR=" << e.what();
            return -1;
        }
    }

    // enable/disable service
    if(setting != "Null"){
        auto method = bus.new_method_call(SYSTEMD_SERVICE, SYSTEMD_OBJ_PATH, SYSTEMD_INTERFACE,setting.c_str());

        std::array<std::string, 1> appendMessage = {service};

        if(setting == "EnableUnitFiles")
        {
            method.append(appendMessage, false, true);
        }
        else
        {
            method.append(appendMessage, false);
        }

        try
        {
            auto reply = bus.call(method);
        }
        catch (const sdbusplus::exception::SdBusError& e)
        {
            std::cerr << "Error in enable/disable service. ERROR=" << e.what();
            return -1;
        }
    }

     return 0;
}

bool filterSensor(std::string sensorName, bool assert)
{
    if (DEBUG)
    {
        std::cerr << "Sensor name: " << sensorName << std::endl;
    }

    if(sensorName == "PDB_VOL_12VOUT")
    {
        uint8_t bus = 0x03;
        uint8_t address = 0x60;
        uint8_t requestReg = 0x79;
        int16_t responseReg;
        int32_t regMask = 0xF038; 

        int ret = getRegsInfoWord(bus,address,requestReg,&responseReg);

        if (DEBUG)
        {
            std::cerr << "Register["<< std::to_string(requestReg) <<"]: " << std::to_string(responseReg) << std::endl;
        }

        if (ret < 0)
        {
            return false;
        }

        if(!(responseReg & regMask))
        {
            return true;
        }
    }
    else if(sensorName == "CPU_Temp")
    {
        if(assert)
        {
            setBmcService("gpio-prochot-thermtrip-assert@prochot.service","RestartUnit","Null");
        }
        else
        {
            setBmcService("gpio-prochot-thermtrip-deassert@prochot.service","RestartUnit","Null");
        }
    }
    else if(sensorName == "CPU_CORE_MOSFET")
    {
        if(assert)
        {
            setBmcService("gpio-vr-hot-assert@CPU_VRHOT.service","RestartUnit","Null");
        }
        else
        {
            setBmcService("gpio-vr-hot-deassert@CPU_VRHOT.service","RestartUnit","Null");
        }
    }
    else if(sensorName == "CPU_SOC_MOSFET")
    {
        if(assert)
        {
            setBmcService("gpio-vr-hot-assert@SOC_VRHOT.service","RestartUnit","Null");
        }
        else
        {
            setBmcService("gpio-vr-hot-deassert@SOC_VRHOT.service","RestartUnit","Null");
        }
    }
    else if(sensorName == "DIMM_MOSFET_1")
    {
        if(assert)
        {
            setBmcService("gpio-vr-hot-assert@MEM_ABCD_VRHOT.service","RestartUnit","Null");
        }
        else
        {
            setBmcService("gpio-vr-hot-deassert@MEM_ABCD_VRHOT.service","RestartUnit","Null");
        }
    }
    else if(sensorName == "DIMM_MOSFET_2")
    {
        if(assert)
        {
            setBmcService("gpio-vr-hot-assert@MEM_EFGH_VRHOT.service","RestartUnit","Null");
        }
        else
        {
            setBmcService("gpio-vr-hot-deassert@MEM_EFGH_VRHOT.service","RestartUnit","Null");
        }
    }


    return false;
}