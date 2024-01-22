#include "tyan-utils.hpp"

namespace tyan
{

ipmi::Cc setManufactureFieledProperty(uint8_t selctField, uint8_t size,
                                      uint8_t& readCount, uint8_t& offsetMSB, uint8_t& offsetLSB)
{
    std::vector<uint8_t> validSize = {2, 1, 16, 16, 16, 6, 6, 32};

    if(size != validSize[selctField] && readCount==0)
    {
        return ipmi::ccReqDataLenInvalid;
    }

    readCount = validSize[selctField];

    switch(selctField)
    {
        case 0:
            // 0h-Product ID, 2 Bytes, LSB
            offsetMSB = 0x00;
            offsetLSB = 0x0d;
            break;
        case 1:
            // 1h-SKU ID, 1 Byte
            offsetMSB = 0x00;
            offsetLSB = 0x0f;
            break;
        case 2:
            // 2h-Device GUID, 16 Bytes
            offsetMSB = 0x00;
            offsetLSB = 0x10;
            break;
        case 3:
            // 3h-System GUID, 16 Bytes
            offsetMSB = 0x00;
            offsetLSB = 0x20;
            break;
        case 4:
            // 4h-Motherboard Serial Number, 16 Bytes
            offsetMSB = 0x00;
            offsetLSB = 0x30;
            break;
        case 5:
            //  5h-MAC 1 Address, 6 Bytes
            offsetMSB = 0x00;
            offsetLSB = 0x40;
            break;
        case 6:
            //  6h-MAC 2 Address, 6 Bytes
            offsetMSB = 0x00;
            offsetLSB = 0x46;
            break;
        case 7:
            // 7h-System Name, 32 Bytes
            offsetMSB = 0x00;
            offsetLSB = 0x4c;
            break;
        default:
            return ipmi::ccUnspecifiedError;
            break;
    }

    return ipmi::ccSuccess;
}
}// namespace tyan