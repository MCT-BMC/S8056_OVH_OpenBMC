#pragma once

#include "Sensor.hpp"

inline constexpr uint8_t PSU_BUS = 7;
inline constexpr uint8_t PSU_ADDR = 0x58;

inline constexpr uint8_t LOW_BYTE = 0;
inline constexpr uint8_t HIGH_BYTE = 1;

inline constexpr int MaxRetry = 3;

constexpr unsigned int delayAddSelMs = 2000;

enum Pmbus_Cmd_E
{
    CMD_SET_PAGE = 0x00,
    CMD_CLEAR_FAULT = 0x03,
    CMD_STATUS_WORD = 0x79,
    CMD_STATUS_VOUT = 0x7A,
    CMD_STATUS_IOUT = 0x7B,
    CMD_STATUS_INPUT = 0x7C,
    CMD_STATUS_TEMP = 0x7D,
    CMD_STATUS_FAN = 0x81,
};

enum Pmbus_Status_Word_High_Byte_E
{
    UNKNOWNFAULTORWARNING = 0,
    OTHER,
    FAN_FAULT_OR_WARNING,
    POWER_GOOD_NEGATED,
    MFR_SPECIFIC,
    INPUT_FAULT_OR_WARNING,
    IOUT_OR_POUT_FAULT_OR_WARNING,
    VOUT_FAULT_OR_WARNING
};

enum Pmbus_Status_Word_Low_Byte_E
{
    NONE_OF_THE_ABOVE = 0,
    COMM_MEM_LOGIC_EVENT,
    TEMP_FAULT_OR_WARNING,
    VOL_IN_UV_FAULT,
    CUR_OUT_OC_FAULT,
    VOL_OUT_OV_FAULT,
    UNIT_IS_OFF,
    UNIT_WAS_BUSY
};

enum Pmbus_Status_Vout_E
{
    VOUT_TRACKING_ERROR = 0,
    TOFF_MAX_WARNING,
    TOFF_MAX_FAULT,
    VOUT_MAX_WARNING,
    VOUT_UV_FAULT,
    VOUT_UV_WARNING,
    VOUT_OV_WARNING,
    VOUT_OV_FAULT
};

enum Pmbus_Status_Iout_E
{
    POUT_OP_WARNING = 0,
    POUT_OP_FAULT,
    IN_POWER_LIMITING_MODE,
    CURRENT_SHARE_FAULT,
    IOUT_UC_FAULT,
    IOUT_OC_WARNING,
    IOUT_OC_LV_FAULT,
    IOUT_OC_FAULT
};

enum Pmbus_Status_Input_E
{
    PIN_OP_WARNING = 0,
    IIN_OC_WARNING,
    IIN_OC_FAULT,
    UNIT_OFF_FOR_LOW_INPUT_VOLTAGE,
    VIN_UV_FAULT,
    VIN_UV_WARNING,
    VIN_OV_WARNING,
    VIN_OV_FAULT
};

enum Pmbus_Status_Temp_E
{
    UT_FAULT = 4,
    UT_WARNING,
    OT_WARNING,
    OT_FAULT,
};

enum Pmbus_Status_Fan_E
{
    FAN2_WARNING = 4,
    FAN1_WARNING,
    FAN2_FAULT,
    FAN1_FAULT,
};

enum Failure_State_E
{
    PsuFailure = 0x1,
    PsuPredictiveFailure = 0x2,
    PsuAcLost = 0x3
};

class PsuStatus : public Sensor
{
  public:
    PsuStatus(std::shared_ptr<boost::asio::io_context>& io,
              std::string object_path, std::string sensor_name,
              std::shared_ptr<sdbusplus::asio::dbus_interface> interface);
    ~PsuStatus(){};

  private:
    std::vector<uint8_t> status_word_val;

    bool isFailureAssert;
    bool isPredictiveFailureAssert;
    bool isAcLostAssert;

    void setupRead(void);

    int8_t getPsuStatus(void);

    void handleResponse(void);

    void checkStatusVout();
    void checkStatusIout();
    void checkStatusInput();
    void checkStatusTemp();
    void checkStatusFan();
    void checkStatusWord();

    int8_t getStatusWord();
    int8_t getPsuByteValue(uint8_t cmd, uint8_t* value);
    int8_t sendPsuClearFault();
    int8_t setPsuPage(uint8_t page);

    boost::asio::steady_timer waitTimer;
    boost::asio::steady_timer delayPredictiveTimer;
    boost::asio::steady_timer delayAcLostTimer;
    boost::asio::steady_timer delayFailureTimer;
};
