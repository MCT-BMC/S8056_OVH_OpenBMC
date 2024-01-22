#pragma once

#include <iostream>
#include <vector>
#include <host-ipmid/ipmid-api.h>
#include <ipmid/api.hpp>
#include <ipmid/utils.hpp>

namespace tyan
{

ipmi::Cc setManufactureFieledProperty(uint8_t selctField, uint8_t size,
                                      uint8_t& readCount, uint8_t& offsetMSB, uint8_t& offsetLSB);
}// namespace tyan