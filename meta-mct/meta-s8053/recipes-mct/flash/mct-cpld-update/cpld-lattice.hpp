#pragma once

#include <stdint.h>

//I2C info
static uint8_t cpldI2cBusId;
static uint8_t cpldI2cSlaveAddr;

//CPLD address
constexpr uint32_t CFM0_START_ADDR = 0x00064000;
constexpr uint32_t CFM0_END_ADDR = 0x000BFFFF;
constexpr uint32_t ON_CHIP_FLASH_IP_CSR_STATUS_REG = 0x00200020;
constexpr uint32_t ON_CHIP_FLASH_IP_CSR_CTRL_REG = 0x00200024;

// CPLD control registers
constexpr uint32_t PROTECT_SEC_ID_5 = 0xF7FFFFFF;
constexpr uint32_t PROTECT_SECS = 0x000003E0;
constexpr uint32_t SELECT_SECTOR_ID_5 = 0x00500000;
constexpr uint32_t CLEAN_SECTOR_ID_BITS = 0xFF8FFFFF;
constexpr uint32_t SET_ERASE_NONE = 0x00700000;

//retry times
constexpr uint8_t MAX10_RETRY_TIMEOUT = 10;

//CPLD status register
constexpr unsigned int BUSY_IDLE = 0x00;
constexpr unsigned int BUSY_ERASE = 0x01;
constexpr unsigned int BUSY_WRITE = 0x02;
constexpr unsigned int BUSY_READ = 0x03;
constexpr unsigned int READ_SUCCESS = 0x04;
constexpr unsigned int WRITE_SUCCESS = 0x08;
constexpr unsigned int ERASE_SUCCESS = 0x10;
constexpr unsigned int STATUS_BIT_MASK = 0x1F;
