#include "PsuStatusMonitor.hpp"

PsuStatus::PsuStatus(
    std::shared_ptr<boost::asio::io_context>& io, std::string object_path,
    std::string sensor_name,
    std::shared_ptr<sdbusplus::asio::dbus_interface> interface) :
    Sensor(object_path, interface, sensor_name),
    isFailureAssert(false), isPredictiveFailureAssert(false),
    isAcLostAssert(false), waitTimer(*io),
    delayPredictiveTimer(*io), delayAcLostTimer(*io), 
    delayFailureTimer(*io)
{
    int ret = 0;
    status_word_val.assign(2, 0);

    // clear fault to PSU
    sendPsuClearFault();
    if (ret < 0)
    {
        std::cerr << "Clear PSU fault failed, ret = %d" << ret << "\n";
    }

    // Set PSU page to 0
    ret = setPsuPage(0);
    if (ret < 0)
    {
        std::cerr << "Set PSU page failed, ret = %d" << ret << "\n";
    }

    std::cout << "Start to monitor PSU STATUS\n";
    setupRead();
}

void PsuStatus::setupRead(void)
{
    value = uint16_t(status_word_val.at(HIGH_BYTE) << 8) +
            uint16_t(status_word_val.at(LOW_BYTE));

    // Update sensor value if the value has been changed.
    if (value != pre_value)
    {
        interface->set_property("Value", value);
        pre_value = value;
    }

    // Get psu status including status word, feed and shutdown event.
    int ret = getPsuStatus();

    if (ret == 0)
    {
        // Check psu status and add SEL following the spec.
        handleResponse();
    }

    waitTimer.expires_from_now(boost::asio::chrono::milliseconds(sensorPollMs));
    waitTimer.async_wait([&](const boost::system::error_code& ec) {
        if (ec == boost::asio::error::operation_aborted)
        {
            return; // we're being canceled
        }
        setupRead();
    });
}

int8_t PsuStatus::getPsuStatus(void)
{
    int ret = 0;
    ret = getStatusWord();
    if (ret < 0)
    {
        std::cerr << sensor_name << " : get status word failed\n";
        return -1;
    }

    return 0;
}

void PsuStatus::handleResponse()
{
    int ret = 0;

    // Check Status Vout (0x7A)
    checkStatusVout();

    // Check Status Iout (0x7B)
    checkStatusIout();

    // Check Status Input (0x7C)
    checkStatusInput();

    // Check Status Yemp (0x7D)
    checkStatusTemp();

    // Check Status Fan (0x81)
    checkStatusFan();

    // Check Status DeAssert
    checkStatusWord();
}

void PsuStatus::checkStatusVout()
{
    // check STATUS_VOUT Error
    if ((status_word_val.at(HIGH_BYTE) & (1 << VOUT_FAULT_OR_WARNING)) ||
        (status_word_val.at(LOW_BYTE) & (1 << VOL_OUT_OV_FAULT)))
    {
        uint8_t status = 0;
        int ret = getPsuByteValue(CMD_STATUS_VOUT, &status);
        if (ret < 0)
        {
            std::cerr << sensor_name << " : get status vout failed\n";
        }

        if (((status & (1 << VOUT_UV_WARNING)) ||
             (status & (1 << VOUT_OV_WARNING))) &&
            (!isPredictiveFailureAssert))
        {
            isPredictiveFailureAssert = true;

            std::string message = "PSU Status Vout Predictive Failure Assert";
            std::vector<uint8_t> event_data = {PsuPredictiveFailure,
                                               status_word_val.at(LOW_BYTE),
                                               status_word_val.at(HIGH_BYTE)};

            addSel(message, object_path, event_data, true);
        }

        if (((status & (1 << VOUT_UV_FAULT)) ||
             (status & (1 << VOUT_OV_FAULT))) &&
            (!isFailureAssert))
        {
            isFailureAssert = true;

            std::string message = "PSU Status Vout Failure Assert";
            std::vector<uint8_t> event_data = {PsuFailure,
                                               status_word_val.at(LOW_BYTE),
                                               status_word_val.at(HIGH_BYTE)};

            addSel(message, object_path, event_data, true);
        }
    }
}

void PsuStatus::checkStatusIout()
{
    // check STATUS_IOUT Error
    if ((status_word_val.at(HIGH_BYTE) &
         (1 << IOUT_OR_POUT_FAULT_OR_WARNING)) ||
        (status_word_val.at(LOW_BYTE) & (1 << CUR_OUT_OC_FAULT)))
    {
        uint8_t status = 0;
        int ret = getPsuByteValue(CMD_STATUS_IOUT, &status);
        if (ret < 0)
        {
            std::cerr << sensor_name << " : get status iout failed\n";
        }

        if (((status & (1 << POUT_OP_WARNING)) ||
             (status & (1 << IOUT_OC_WARNING))) &&
            (!isPredictiveFailureAssert))
        {
            isPredictiveFailureAssert = true;

            std::string message = "PSU Status Iout Predictive Failure Assert";
            std::vector<uint8_t> event_data = {PsuPredictiveFailure,
                                               status_word_val.at(LOW_BYTE),
                                               status_word_val.at(HIGH_BYTE)};

            addSel(message, object_path, event_data, true);
        }

        if (((status & (1 << POUT_OP_FAULT)) ||
             (status & (1 << CURRENT_SHARE_FAULT)) ||
             (status & (1 << IOUT_UC_FAULT)) ||
             (status & (1 << IOUT_OC_LV_FAULT)) ||
             (status & (1 << IOUT_OC_FAULT))) &&
            (!isFailureAssert))
        {
            isFailureAssert = true;

            std::string message = "PSU Status Iout Failure Assert";
            std::vector<uint8_t> event_data = {PsuFailure,
                                               status_word_val.at(LOW_BYTE),
                                               status_word_val.at(HIGH_BYTE)};

            addSel(message, object_path, event_data, true);
        }
    }
}

void PsuStatus::checkStatusInput()
{
    // check STATUS_INPUT Error
    if ((status_word_val.at(HIGH_BYTE) & (1 << INPUT_FAULT_OR_WARNING)) ||
        (status_word_val.at(LOW_BYTE) & (1 << VOL_IN_UV_FAULT)))
    {
        uint8_t status = 0;
        int ret = getPsuByteValue(CMD_STATUS_INPUT, &status);
        if (ret < 0)
        {
            std::cerr << sensor_name << " : get status input failed\n";
        }

        if(!isPredictiveFailureAssert)
        {
            if ((status & (1 << PIN_OP_WARNING)) || (status & (1 << IIN_OC_WARNING)))
            {
                std::string message = "PSU Status Input Predictive Failure Assert";
                std::string path = object_path;
                std::vector<uint8_t> event_data = {PsuPredictiveFailure,
                                                   status_word_val.at(LOW_BYTE),
                                                   status_word_val.at(HIGH_BYTE)};
                isPredictiveFailureAssert = true;
                addSel(message, path, event_data, true);
            }
            else if ((status & (1 << VIN_UV_WARNING)))
            {
                delayPredictiveTimer.expires_from_now(boost::asio::chrono::milliseconds(delayAddSelMs));
                delayPredictiveTimer.async_wait([&](const boost::system::error_code & ec) {
                    if (ec == boost::asio::error::operation_aborted)
                    {
                        return; // we're being canceled
                    }
                    std::string message = "PSU Status Input Predictive Failure Assert";
                    std::string path = object_path;
                    std::vector<uint8_t> event_data = {PsuPredictiveFailure,
                                                       status_word_val.at(LOW_BYTE),
                                                       status_word_val.at(HIGH_BYTE)};
                    isPredictiveFailureAssert = true;
                    addSel(message, path, event_data, true);
                });
            }
        }

        if ((status & (1 << VIN_UV_FAULT)) && (!isFailureAssert))
        {

            delayFailureTimer.expires_from_now(boost::asio::chrono::milliseconds(delayAddSelMs));
            delayFailureTimer.async_wait([&](const boost::system::error_code & ec) {
                if (ec == boost::asio::error::operation_aborted)
                {
                    return; // we're being canceled
                }
                std::string message = "PSU Status Input Failure Assert";
                std::string path = object_path;
                std::vector<uint8_t> event_data = {PsuFailure,
                                                   status_word_val.at(LOW_BYTE),
                                                   status_word_val.at(HIGH_BYTE)};
                isFailureAssert = true;
                addSel(message, path, event_data, true);
            });
        }

        if (((status & (1 << UNIT_OFF_FOR_LOW_INPUT_VOLTAGE))) && (!isAcLostAssert))
        {
            delayAcLostTimer.expires_from_now(boost::asio::chrono::milliseconds(delayAddSelMs));
            delayAcLostTimer.async_wait([&](const boost::system::error_code & ec) {
                if (ec == boost::asio::error::operation_aborted)
                {
                    return; // we're being canceled
                }
                std::string message = "PSU Status Input AC loss Assert";
                std::string path = object_path;
                std::vector<uint8_t> event_data = {PsuAcLost,
                                                   status_word_val.at(LOW_BYTE),
                                                   status_word_val.at(HIGH_BYTE)};
                isAcLostAssert = true;
                addSel(message, path, event_data, true);
            });
        }
    }
}

void PsuStatus::checkStatusTemp()
{
    // check STATUS_TEMP Error
    if ((status_word_val.at(LOW_BYTE)) & (1 << TEMP_FAULT_OR_WARNING))
    {
        uint8_t status = 0;
        int ret = getPsuByteValue(CMD_STATUS_TEMP, &status);
        if (ret < 0)
        {
            std::cerr << sensor_name << " : get status temp failed\n";
        }

        if (((status & (1 << UT_WARNING)) || (status & (1 << OT_WARNING))) &&
            (!isPredictiveFailureAssert))
        {
            isPredictiveFailureAssert = true;

            std::string message = "PSU Status Temp Predictive Failure Assert";
            std::vector<uint8_t> event_data = {PsuPredictiveFailure,
                                               status_word_val.at(LOW_BYTE),
                                               status_word_val.at(HIGH_BYTE)};

            addSel(message, object_path, event_data, true);
        }

        if (((status & (1 << UT_FAULT)) || (status & (1 << OT_FAULT))) &&
            (!isFailureAssert))
        {
            isFailureAssert = true;

            std::string message = "PSU Status Temp Failure Assert";
            std::vector<uint8_t> event_data = {PsuFailure,
                                               status_word_val.at(LOW_BYTE),
                                               status_word_val.at(HIGH_BYTE)};

            addSel(message, object_path, event_data, true);
        }
    }
}

void PsuStatus::checkStatusFan()
{
    // check STATUS_FAN Error
    if ((status_word_val.at(HIGH_BYTE)) & (1 << FAN_FAULT_OR_WARNING))
    {
        uint8_t status = 0;
        int ret = getPsuByteValue(CMD_STATUS_FAN, &status);
        if (ret < 0)
        {
            std::cerr << sensor_name << " : get status temp failed\n";
        }

        if (((status & (1 << FAN2_WARNING)) ||
             (status & (1 << FAN1_WARNING))) &&
            (!isPredictiveFailureAssert))
        {
            isPredictiveFailureAssert = true;

            std::string message = "PSU Status Fan Predictive Failure Assert";
            std::vector<uint8_t> event_data = {PsuPredictiveFailure,
                                               status_word_val.at(LOW_BYTE),
                                               status_word_val.at(HIGH_BYTE)};

            addSel(message, object_path, event_data, true);
        }

        if (((status & (1 << FAN2_FAULT)) || (status & (1 << FAN1_FAULT))) &&
            (!isFailureAssert))
        {
            isFailureAssert = true;

            std::string message = "PSU Status Fan Failure Assert";
            std::vector<uint8_t> event_data = {PsuFailure,
                                               status_word_val.at(LOW_BYTE),
                                               status_word_val.at(HIGH_BYTE)};

            addSel(message, object_path, event_data, true);
        }
    }
}

void PsuStatus::checkStatusWord()
{
    // check status to de-assert
    if (((status_word_val.at(HIGH_BYTE) & 0xE4) == 0x0) &&
        ((status_word_val.at(LOW_BYTE) & 0x3C) == 0x0))
    {
        if (isFailureAssert)
        {
            isFailureAssert = false;

            std::string message = "PSU Status Failure Deassert";
            std::vector<uint8_t> event_data = {PsuFailure,
                                               status_word_val.at(LOW_BYTE),
                                               status_word_val.at(HIGH_BYTE)};

            addSel(message, object_path, event_data, false);
        }

        if (isPredictiveFailureAssert)
        {
            isPredictiveFailureAssert = false;
            std::string message = "PSU Status Predictive Deassert";
            std::vector<uint8_t> event_data = {PsuPredictiveFailure,
                                               status_word_val.at(LOW_BYTE),
                                               status_word_val.at(HIGH_BYTE)};
            addSel(message, object_path, event_data, false);
        }

        if (isAcLostAssert)
        {
            isAcLostAssert = false;
            std::string message = "PSU Status AC Lost Deassert";
            std::vector<uint8_t> event_data = {PsuAcLost,
                                               status_word_val.at(LOW_BYTE),
                                               status_word_val.at(HIGH_BYTE)};
            addSel(message, object_path, event_data, false);
        }
    }
    else
    {
        int ret = sendPsuClearFault();
        if (ret < 0)
        {
            std::cerr << __func__ << " : Clear PSU fault failed, ret = %d"
                      << ret << "\n";
        }
    }

    // check STATUS_WORD Error
    if ((status_word_val.at(LOW_BYTE)) & (1 << COMM_MEM_LOGIC_EVENT))
    {
        int ret = sendPsuClearFault();
        if (ret < 0)
        {
            std::cerr << __func__ << " : Clear PSU fault failed, ret = %d"
                      << ret << "\n";
        }
    }
}

int8_t PsuStatus::setPsuPage(uint8_t page)
{
    int res = -1;
    std::vector<char> filename;
    filename.assign(20, 0);

    // Open I2C Device
    int fd = open_i2c_dev(PSU_BUS, filename.data(), filename.size(), 0);
    if (fd < 0)
    {
        std::cerr << "Fail to open I2C device: " << PSU_BUS << "\n";
        return -1;
    }

    uint8_t busAddr = PSU_ADDR << 1;
    std::vector<uint8_t> crcData = {busAddr, CMD_SET_PAGE, page};
    uint8_t crcValue = i2c_smbus_pec(crcData.data(), crcData.size());

    // set PSU Page command
    std::vector<uint8_t> cmd_data = {CMD_SET_PAGE, page, crcValue};

    res = i2c_master_write(fd, PSU_ADDR, cmd_data.size(), cmd_data.data());
    if (res < 0)
    {
        std::cerr << "Fail to r/w I2C device: " << PSU_BUS
                  << ", Addr: " << PSU_ADDR << "\n";
        close_i2c_dev(fd);
        return -1;
    }

    close_i2c_dev(fd);
    return 0;
}

int8_t PsuStatus::sendPsuClearFault()
{
    int res = -1;
    std::vector<char> filename;
    filename.assign(20, 0);

    // Open I2C Device
    int fd = open_i2c_dev(PSU_BUS, filename.data(), filename.size(), 0);
    if (fd < 0)
    {
        std::cerr << "Fail to open I2C device: " << PSU_BUS << "\n";
        return -1;
    }

    uint8_t busAddr = PSU_ADDR << 1;
    std::vector<uint8_t> crcData = {busAddr, CMD_CLEAR_FAULT};
    uint8_t crcValue = i2c_smbus_pec(crcData.data(), crcData.size());

    // Send PSU clear fault command
    std::vector<uint8_t> cmd_data = {CMD_CLEAR_FAULT, crcValue};

    res = i2c_master_write(fd, PSU_ADDR, cmd_data.size(), cmd_data.data());
    if (res < 0)
    {
        std::cerr << "Fail to r/w I2C device: " << PSU_BUS
                  << ", Addr: " << PSU_ADDR << "\n";
        close_i2c_dev(fd);
        return -1;
    }

    close_i2c_dev(fd);
    return 0;
}

int8_t PsuStatus::getStatusWord()
{
    int res = -1;
    std::vector<char> filename;
    filename.assign(20, 0);

    int fd = open_i2c_dev(PSU_BUS, filename.data(), filename.size(), 0);
    if (fd < 0)
    {
        std::cerr << "Fail to open I2C device: " << PSU_BUS << "\n";
        return -1;
    }

    std::vector<uint8_t> cmd_data;
    cmd_data.assign(1, CMD_STATUS_WORD);

    std::vector<uint8_t> read_data;
    read_data.assign(2, 0x0);

    res = i2c_master_write_read(fd, PSU_ADDR, cmd_data.size(), cmd_data.data(),
                                read_data.size(), read_data.data());
    if (res < 0)
    {
        std::cerr << "Fail to r/w I2C device: " << PSU_BUS
                  << ", Addr: " << PSU_ADDR << "\n";
        close_i2c_dev(fd);
        return -1;
    }
    status_word_val = read_data;

    close_i2c_dev(fd);
    return 0;
}

int8_t PsuStatus::getPsuByteValue(uint8_t cmd, uint8_t* resValue)
{
    int res = -1;
    std::vector<char> filename;
    filename.assign(20, 0);

    int fd = open_i2c_dev(PSU_BUS, filename.data(), filename.size(), 0);
    if (fd < 0)
    {
        std::cerr << "Fail to open I2C device: " << PSU_BUS << "\n";
        return -1;
    }

    std::vector<uint8_t> cmd_data;
    cmd_data.assign(1, cmd);

    std::vector<uint8_t> read_data;
    read_data.assign(1, 0x0);

    res = i2c_master_write_read(fd, PSU_ADDR, cmd_data.size(), cmd_data.data(),
                                read_data.size(), read_data.data());
    if (res < 0)
    {
        std::cerr << "Fail to r/w I2C device: " << PSU_BUS
                  << ", Addr: " << PSU_ADDR << "\n";
        close_i2c_dev(fd);
        return -1;
    }
    *resValue = read_data.at(0);

    close_i2c_dev(fd);
    return 0;
}
