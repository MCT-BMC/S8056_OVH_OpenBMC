#include "libapmlpower.hpp"

static constexpr bool DEBUG = true;

uint32_t getCpuPowerLimit(int sockId)
{
    oob_status_t ret;
    uint32_t power = 0;
    if (!esmiMutex.try_lock_for(boost::asio::chrono::milliseconds(esmiMutexTimout)))
    {
        if constexpr (DEBUG)
        {
            std::cerr << "[Debug Mode]"
                      << "esmiMutex is still lock\n";
        }
        esmiMutex.unlock();
        esmiMutex.lock();
    }

    /* Get the PowerLimit for a given socket index */
    ret = read_socket_power_limit(sockId, &power);
    if (ret != OOB_SUCCESS)
    {
        fprintf(stderr, "Failed: to get socket[%d] powerlimit, Err[%d]: %s\n",
                sockId, ret, esmi_get_err_msg(ret));
    }
    esmiMutex.unlock();

    if constexpr (DEBUG)
    {
        std::cerr << "[Debug Mode]"
                  << "Get SockID[" << sockId << "] CPU Power Limit[" << power
                  << "]\n";
    }
    return power;
}

uint32_t getCpuPowerConsumption(int sockId)
{
    oob_status_t ret;
    uint32_t power = 0;

    if (!esmiMutex.try_lock_for(boost::asio::chrono::milliseconds(esmiMutexTimout)))
    {
        if constexpr (DEBUG)
        {
            std::cerr << "[Debug Mode]"
                      << "esmiMutex is still lock\n";
        }
        esmiMutex.unlock();
        esmiMutex.lock();
    }

    ret = read_socket_power(sockId, &power);
    if (ret != OOB_SUCCESS)
    {
        fprintf(stderr, "Failed: to get socket[%d] power, Err[%d]: %s\n",
                sockId, ret, esmi_get_err_msg(ret));
    }
    esmiMutex.unlock();

    if constexpr (DEBUG)
    {
        std::cerr << "[Debug Mode]"
                  << "Get SockID[" << sockId << "] CPU Power Consumption["
                  << power << "]\n";
    }
    return power;
}

int setCpuPowerLimit(int sockId, uint32_t power)
{
    oob_status_t ret;
    uint32_t max_power = 0;

    if (!esmiMutex.try_lock_for(boost::asio::chrono::milliseconds(esmiMutexTimout)))
    {
        if constexpr (DEBUG)
        {
            std::cerr << "[Debug Mode]"
                      << "esmiMutex is still lock\n";
        }
        esmiMutex.unlock();
        esmiMutex.lock();
    }

    ret = read_max_socket_power_limit(sockId, &max_power);
    if ((ret == OOB_SUCCESS) && (power > max_power))
    {
        fprintf(stderr,
                "Input power is more than max limit,"
                " So It set's to default max %.3f Watts\n",
                (double)max_power / 1000);
        power = max_power;
    }

    ret = write_socket_power_limit(sockId, power);
    if (ret != OOB_SUCCESS)
    {
        fprintf(stderr, "Failed: to set socket[%d] power_limit, Err[%d]: %s\n",
                sockId, ret, esmi_get_err_msg(ret));
    }

    esmiMutex.unlock();

    if constexpr (DEBUG)
    {
        std::cerr << "[Debug Mode]"
                  << "Set SockID[" << sockId << "] CPU Power Limit[" << power
                  << "]\n";
    }
    return ret;
}

int setDefaultCpuPowerLimit(int sockId)
{
    oob_status_t ret;
    uint32_t max_power;

    if (!esmiMutex.try_lock_for(boost::asio::chrono::milliseconds(esmiMutexTimout)))
    {
        if constexpr (DEBUG)
        {
            std::cerr << "[Debug Mode]"
                      << "esmiMutex is still lock\n";
        }
        esmiMutex.unlock();
        esmiMutex.lock();
    }

    ret = read_max_socket_power_limit(sockId, &max_power);
    if (ret != OOB_SUCCESS)
    {
        fprintf(
            stderr,
            "Failed: to get socket[%d] default max power limit, Err[%d]: %s\n",
            sockId, ret, esmi_get_err_msg(ret));
        goto error_handler;
    }

    ret = write_socket_power_limit(sockId, max_power);
    if (ret != OOB_SUCCESS)
    {
        fprintf(stderr,
                "Failed: to set socket[%d] default power_limit, Err[%d]: %s\n",
                sockId, ret, esmi_get_err_msg(ret));
        goto error_handler;
    }
    esmiMutex.unlock();

    if constexpr (DEBUG)
    {
        std::cerr << "[Debug Mode]"
                  << "Set SockID[" << sockId << "] Default CPU Power Limit["
                  << max_power << "]\n";
    }
    return OOB_SUCCESS;

error_handler:
    esmiMutex.unlock();
    return ret;
}

uint64_t getMsrValue(uint32_t thread, uint32_t msrRegister)
{
    oob_status_t ret;
    uint64_t msrValue = 0x00;

    if (!esmiMutex.try_lock_for(boost::asio::chrono::milliseconds(esmiMutexTimout)))
    {
        if constexpr (DEBUG)
        {
            std::cerr << "[Debug Mode]"
                      << "esmiMutex is still lock\n";
        }
        esmiMutex.unlock();
        esmiMutex.lock();
    }

    ret = esmi_oob_read_msr(0, thread, msrRegister, &msrValue);
    if (ret != OOB_SUCCESS) {
        fprintf(stderr,"Failed to access MCA MSR, Err[%d]: %s\n", ret, esmi_get_err_msg(ret));
    }
    esmiMutex.unlock();

    return msrValue;
}
