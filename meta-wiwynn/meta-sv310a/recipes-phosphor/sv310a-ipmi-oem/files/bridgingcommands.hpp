#pragma once

#include "Utils.hpp"

#include <ipmid/api.hpp>

#include <vector>

using namespace phosphor::logging;

enum class GetMessageFlagsBit
{
    ReceiveMessage = 0,
    EventMessage = 1,
    WatchdogPreTimeOut = 3,
    OEM0 = 5,
    OEM1 = 6,
    OEM2 = 7
};

enum class ChannelType
{
    IPMB = 0x1,
    ICMB10 = 0x2,
    ICMB09 = 0x3,
    Lan = 0x4,
    SerialModem = 0x5,
    OtherLan = 0x6,
    PCISmbus = 0x7,
    Smbus10 = 0x8,
    Smbus20 = 0x9,
    SystemInterface = 0xC

};

enum class ChannelMode
{
    NoTracking = 0x0,
    TrackRequest = 0x1,
    SendRaw = 0x2,
    Reserved = 0x3
};

constexpr size_t ipmbMaxDataSize = 256;
constexpr size_t ipmbConnectionHeaderLength = 3;
constexpr size_t ipmbResponseDataHeaderLength = 4;
constexpr size_t ipmbRequestDataHeaderLength = 3;
constexpr size_t ipmbChecksumSize = 1;
constexpr size_t ipmbMinFrameLength = 7;
constexpr size_t ipmbMaxFrameLength = ipmbConnectionHeaderLength +
                                      ipmbResponseDataHeaderLength +
                                      ipmbChecksumSize + ipmbMaxDataSize;

struct IpmbRequest
{
    IpmbRequest(const std::vector<uint8_t>& data) :
        rsSA(data[0]), netFn(data[1] >> 2), rsLun(data[1] & 0x03),
        rqSA(data[3]), seq(data[4] >> 2), rqLun(data[4] & 0x03), cmd(data[5]),
        data(data.begin() +
                 (ipmbConnectionHeaderLength + ipmbRequestDataHeaderLength),
             data.end() - ipmbChecksumSize)
    {
        isValid = true;

        // Check IPMB connection header checksum.
        if (!validateZeroChecksum(
                {data.begin(), data.begin() + ipmbConnectionHeaderLength}))
        {
            log<level::ERR>(
                "IPMB Request - Invalid connection header checksum");
            isValid = false;
            return;
        }

        if (!validateZeroChecksum(
                {data.begin() + ipmbConnectionHeaderLength, data.end()}))
        {
            log<level::ERR>("IPMB Request - Invalid data checksum");
            isValid = false;
            return;
        }
    }

    bool isValid;
    uint8_t rsSA;
    uint8_t netFn;
    uint8_t rsLun;
    uint8_t checksum1;
    uint8_t rqSA;
    uint8_t seq;
    uint8_t rqLun;
    uint8_t cmd;
    std::vector<uint8_t> data;
};

struct IpmbResponse
{
    IpmbResponse(uint8_t rqSA, uint8_t netFn, uint8_t rqLun, uint8_t rsSA,
                 uint8_t seq, uint8_t rsLun, uint8_t cmd, uint8_t cc,
                 const std::vector<uint8_t>& data) :
        rqSA(rqSA),
        netFn(netFn), rqLun(rqLun), rsSA(rsSA), seq(seq), rsLun(rsLun),
        cmd(cmd), cc(cc), data(std::move(data))
    {}

    std::vector<uint8_t> convertToI2cResponse() const;

    uint8_t rqSA;
    uint8_t netFn;
    uint8_t rqLun;
    uint8_t rsSA;
    uint8_t seq;
    uint8_t rsLun;
    uint8_t cmd;
    uint8_t cc;
    std::vector<uint8_t> data;
};

struct SendMessageRequest
{
    uint8_t channelNumber;
    std::vector<uint8_t> data;

    constexpr uint8_t getChannelType()
    {
        return (channelNumber & 0x0F);
    }

    constexpr uint8_t getAuthentication()
    {
        return (channelNumber & 0x10) >> 4;
    }

    constexpr uint8_t getEncryption()
    {
        return (channelNumber & 0x20) >> 5;
    }

    constexpr uint8_t getMode()
    {
        return (channelNumber & 0xC0) >> 6;
    }
};

struct GetMessageResponse
{
    uint8_t channelNumber = 0;
    std::vector<uint8_t> data;

    constexpr void setChannelType(ChannelType type)
    {
        channelNumber |= (static_cast<uint8_t>(type) & 0x0F);
    }

    constexpr void setCommandPrivilege(CommandPrivilege privilege)
    {
        channelNumber |= (static_cast<uint8_t>(privilege) & 0xF0);
    }
};
