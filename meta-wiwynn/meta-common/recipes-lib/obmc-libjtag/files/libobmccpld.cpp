
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/jtag.h>
#include <cstring>
#include <vector>

#include "libobmcjtag.hpp"
#include "libobmccpld.hpp"

constexpr const uint32_t latticeIrWtLen = 8;
constexpr const uint32_t latticeDrRdLen = 32;

int getCpldUserCode(uint8_t part, uint32_t* buffer)
{
    uint32_t jtagIrWtLen = 0;
    uint32_t jtagDrRdLen = 0;
    uint32_t jtagCmd = 0x0;

    switch (part)
    {
        case LATTICE:
        {
            jtagIrWtLen = latticeIrWtLen;
            jtagDrRdLen = latticeDrRdLen;
            jtagCmd = lattice_read_usercode;
            break;
        }

        default:
            std::cerr << "Invalid CPLD part providor. " << __FUNCTION__ << "\n";
            return -1;
    }

    int jtagFd = open_jtag_dev();
    if (jtagFd < 0)
    {
        std::cerr << "Error in " << __FUNCTION__ << " in " << __LINE__ << "\n";
        return -1;
    }

    // Reset to idle/run-test state and stay in 2 tcks
    jtag_interface_end_tap_state(jtagFd, 1, JTAG_STATE_IDLE, 2);

    int ret = -1;
    ret = jtag_interface_xfer(jtagFd, JTAG_SIR_XFER, JTAG_WRITE_XFER,
                              JTAG_STATE_IDLE, jtagIrWtLen, &jtagCmd);
    if (ret < 0)
    {
         std::cerr << "Error in " << __FUNCTION__ << " in " << __LINE__ << "\n";
         close_jtag_dev(jtagFd);
         return -1;
    }

    // Go to idle/run-test state and stay in 2 tcks
    jtag_interface_end_tap_state(jtagFd, 0, JTAG_STATE_IDLE, 2);

    uint32_t result = 0;
    ret = jtag_interface_xfer(jtagFd, JTAG_SDR_XFER, JTAG_READ_XFER,
                              JTAG_STATE_IDLE, jtagDrRdLen, &result);
    if (ret < 0)
    {
         std::cerr << "Error in " << __FUNCTION__ << " in " << __LINE__ << "\n";
         close_jtag_dev(jtagFd);
         return -1;
    }

    close_jtag_dev(jtagFd);

    std::memcpy(buffer, &result, sizeof(uint32_t));

    return 0;
}

int getCpldIdCode(uint8_t part, uint32_t* buffer)
{
    uint32_t jtagIrWtLen = 0;
    uint32_t jtagDrRdLen = 0;
    uint32_t jtagCmd = 0x0;

    switch (part)
    {
        case LATTICE:
        {
            jtagIrWtLen = latticeIrWtLen;
            jtagDrRdLen = latticeDrRdLen;
            jtagCmd = lattice_read_idcodepub;
            break;
        }

        default:
            std::cerr << "Invalid CPLD part providor. " << __FUNCTION__ << "\n";
            return -1;
    }

    int jtagFd = open_jtag_dev();
    if (jtagFd < 0)
    {
        std::cerr << "Error in " << __FUNCTION__ << " in " << __LINE__ << "\n";
        return -1;
    }

    // Reset to idle/run-test state and stay in 2 tcks
    int ret = jtag_interface_end_tap_state(jtagFd, 1, JTAG_STATE_IDLE, 2);
    if (ret < 0)
    {
        std::cerr << "Error in " << __FUNCTION__ << " in " << __LINE__ << "\n";
        return -1;
    }

    // Return to idle state
    ret = jtag_interface_end_tap_state(jtagFd, 0, JTAG_STATE_IDLE, 2);
    if (ret < 0)
    {
        std::cerr << "Error in " << __FUNCTION__ << " in " << __LINE__ << "\n";
        return -1;
    }

    // Check the IDCODE_PUB
    ret = jtag_interface_sir_xfer(jtagFd, JTAG_STATE_IDLE,
                                  jtagIrWtLen, jtagCmd);
    if (ret < 0)
    {
        std::cerr << "Error in " << __FUNCTION__ << " in " << __LINE__ << "\n";
        return -1;
    }

    std::vector<unsigned int> dr_data(4,0);

    ret = jtag_interface_tdo_xfer(jtagFd, JTAG_STATE_IDLE, jtagDrRdLen, dr_data.data());
    if (ret < 0)
    {
        std::cerr << "Error in " << __FUNCTION__ << " in " << __LINE__ << "\n";
        return -1;
    }

    std::cerr<<"CPLD ID = 0x"<<std::hex<<dr_data.at(0)<<"\n";

    close_jtag_dev(jtagFd);

    std::memcpy(buffer, &dr_data.at(0), sizeof(uint32_t));

    return 0;
}