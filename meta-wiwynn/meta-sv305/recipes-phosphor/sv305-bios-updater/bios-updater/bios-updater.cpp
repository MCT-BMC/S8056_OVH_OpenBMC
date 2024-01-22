#include "bios-updater.hpp"
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <gpiod.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/exception.hpp>
#include <sdbusplus/message/types.hpp>

static constexpr uint8_t meAddress = 1;
static constexpr uint8_t lun = 0;
static constexpr unsigned int fmBiosSpiBmcCtrl = 26;
static constexpr const char* strGpiochip = "gpiochip0";

int8_t BiosUpdateManager::systemPowerStateCheck()
{
    std::variant<int> currState;

    auto bus = sdbusplus::bus::new_default_system();
    auto method = bus.new_method_call(POWER_STATE_OBJ, POWER_STATE_PATH,
                                      PROPERTY_INTERFACE, "Get");

    method.append(POWER_STATE_INTERFACE, "pgood");
    try
    {
        auto reply = bus.call(method);
        reply.read(currState);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        std::cerr << "Error in " << __func__ << "\n";
        return -1;
    }

    return std::get<int>(currState);
}

int8_t BiosUpdateManager::performStateTransition(
                            State::Host::Transition transition)
{
    std::variant<std::string> property =
                            State::convertForMessage(transition);

    auto bus = sdbusplus::bus::new_default_system();
    auto method = bus.new_method_call(HOST_STATE_OBJ, HOST_STATE_PATH,
                                      PROPERTY_INTERFACE, "Set");

    method.append(HOST_STATE_INTERFACE, "RequestedHostTransition", property);
    try
    {
        bus.call_noreply(method);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        std::cerr << "Error in " << __func__ << "\n";
        return -1;
    }

    return 0;
}

int8_t BiosUpdateManager::setBiosMtdDevice(uint8_t state)
{
    int ret = 0;
    std::string spi = BIOS_SPI;
    std::string path = BIOS_DRIVER_PATH;
    int fd;

    switch (state)
    {
    case unbind:
        path = path + "unbind";
        break;
    case bind:
        path = path + "bind";
        break;
    default:
        std::cerr << "Fail to get state failed\n";
        return -1;
    }

    fd = open(path.c_str(), O_WRONLY);
    if (fd < 0)
    {
        std::cerr << "Fail in " << __func__ << "\n";
        return -1;
    }

    write(fd, spi.c_str(), spi.size());
    close(fd);

    return 0;
}

int8_t BiosUpdateManager::biosUpdatePwrStateCheck(uint8_t& state)
{
    int ret = -1;

    ret = systemPowerStateCheck();
    if (ret < 0)
    {
        std::cerr << "Fail to get system power state\n";
        return -1;
    }
    state = static_cast<uint8_t>(ret);

    return 0;
}


int8_t BiosUpdateManager::biosUpdatePwrCtl(uint8_t state)
{
    int ret = -1;

    ret = systemPowerStateCheck();
    if (ret < 0)
    {
        std::cerr << "Fail to get system power state\n";
        return -1;
    }

    uint8_t currState = static_cast<uint8_t>(ret);
    if(state == currState)
    {
        std::cout << "No need to perform power transition\n";
        return 0;
    }

    int interval;   // unit: second
    State::Host::Transition request;
    
    switch (state)
    {
        case off:
            interval = 16;
            request = State::Host::Transition::Off;
            break;
        case on:
            interval = 5;
            request = State::Host::Transition::On;
            break;
        default:
            std::cerr << "Invalid power state\n";
            return -1;
    }

    ret = -1;
    ret = performStateTransition(request);
    if (ret < 0)
    {
        std::cerr << "Failed to perform power transition\n";
        return -1;
    }

    std::cout << "Wait for "<< interval << " seconds...\n";
    sleep(interval);

    ret = -1;
    ret = systemPowerStateCheck();
    if (ret < 0)
    {
        std::cerr << "Fail to get system power state\n";
        return -1;
    }

    currState = static_cast<uint8_t>(ret);
    if(state != currState)
    {
        std::cerr << "Final state is not correct\n";
        return -1;
    }

    return 0;
}

int8_t BiosUpdateManager::biosUpdatePrepare()
{
    int ret = 0;

    std::cout << "Wait for 15 seconds...\n";
    sleep(15);

    // Set BIOS SPI MUX path to BMC (H) 
    gpiod_ctxless_set_value(strGpiochip,          // Label of the gpiochip.
                            fmBiosSpiBmcCtrl,     // Number of GPIO pin.
                            1,                    // GPIO set value.
                            false,                // The active state of this line - true if low.
                            "bios-updater",       // Name of comsumer.
                            NULL,                 // Callback function.
                            NULL);                // value passed to callback function.

    ret = setBiosMtdDevice(bind);
    if (ret < 0)
    {
        std::cerr << "Failed in bind mtd partition\n";
        return -1;
    }
    sleep(1);

    return 0;
}

int8_t BiosUpdateManager::biosUpdate(char* image_str)
{
    std::string flashcp_str = "/usr/sbin/flashcp -v ";
    flashcp_str = flashcp_str + image_str + " /dev/mtd/pnor";

    std::cout << "\nUpdating bios...\n";
    system(flashcp_str.c_str());

    return 0;
}

int8_t BiosUpdateManager::biosUpdateFinished(char* image_str)
{
    int ret = 0;

    sleep(1);

    ret = setBiosMtdDevice(unbind);
    if (ret < 0)
    {
        std::cerr << "Failed in unbind mtd partition\n";
        return -1;
    }
    sleep(10);

    // Set BIOS SPI MUX path to Host (L)
    gpiod_ctxless_set_value(strGpiochip,          // Label of the gpiochip.
                            fmBiosSpiBmcCtrl,     // Number of GPIO pin.
                            0,                    // GPIO set value.
                            false,                // The active state of this line - true if low.
                            "bios-updater",       // Name of comsumer.
                            NULL,                 // Callback function.
                            NULL);                // value passed to callback function.
    sleep(5);

    remove(image_str);

    return 0;
}
