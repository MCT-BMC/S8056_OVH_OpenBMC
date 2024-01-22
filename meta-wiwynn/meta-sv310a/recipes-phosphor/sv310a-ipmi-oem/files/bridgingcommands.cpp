#include "bridgingcommands.hpp"

#include <queue>

void registerBridgingFunctions() __attribute__((constructor));

static constexpr auto IpmbService = "xyz.openbmc_project.Ipmi.Channel.Ipmb";
static constexpr auto IpmbPath = "/xyz/openbmc_project/Ipmi/Channel/Ipmb";
static constexpr auto IpmbInterface = "org.openbmc.Ipmb";

static constexpr size_t ResponseQueueSizeMax = 20;
static std::queue<IpmbResponse> responseQueue;

// Mapping device slave address and channel number.
static std::unordered_map<uint8_t, uint8_t> channelMap = {{0x30, 0x00},
                                                          {0x32, 0x00}};

std::vector<uint8_t> IpmbResponse::convertToI2cResponse() const
{
    std::vector<uint8_t> response;

    response.emplace_back(rqSA);

    uint8_t netFnRqLun = (netFn << 2) | rqLun;
    response.emplace_back(netFnRqLun);

    uint8_t checksum1 = calculateZeroChecksum({rqSA, netFnRqLun});
    response.emplace_back(checksum1);

    response.emplace_back(rsSA);

    uint8_t seqRsLun = (seq << 2) | rsLun;
    response.emplace_back(seqRsLun);

    response.emplace_back(cmd);
    response.emplace_back(cc);

    response.insert(response.end(), data.begin(), data.end());

    std::vector<uint8_t> tmp{rsSA, seq, rsLun, cmd, cc};
    tmp.insert(tmp.end(), data.begin(), data.end());
    uint8_t checksum2 = calculateZeroChecksum(tmp);
    response.emplace_back(checksum2);

    return response;
}

static ipmi::RspType<std::vector<uint8_t>>
    sendIpmbMessage(SendMessageRequest request)
{
    if ((request.data.size() < ipmbMinFrameLength) ||
        (request.data.size() > ipmbMaxFrameLength))
    {
        return ipmi::responseReqDataLenInvalid();
    }

    IpmbRequest ipmbRequest(request.data);
    if (!ipmbRequest.isValid)
    {
        return ipmi::responseInvalidFieldRequest();
    }

    auto findChannel = channelMap.find(ipmbRequest.rsSA);
    if (findChannel == channelMap.end())
    {
        log<level::ERR>("SendMessage, Invalid IPMB slave address",
                        entry("SLAVE_ADDRESS=%#lx", ipmbRequest.rsSA));
        return ipmi::responseInvalidFieldRequest();
    }

    std::tuple<int, uint8_t, uint8_t, uint8_t, uint8_t, std::vector<uint8_t>>
        response;
    try
    {
        auto bus = getSdBus();
        auto msg = bus->new_method_call(IpmbService, IpmbPath, IpmbInterface,
                                        "sendRequest");
        msg.append(findChannel->second, ipmbRequest.netFn, ipmbRequest.rqLun,
                   ipmbRequest.cmd, ipmbRequest.data);

        auto reply = bus->call(msg);
        reply.read(response);
    }
    catch (const sdbusplus::exception_t& e)
    {
        log<level::ERR>("SendMessage, send D-Bus IPMB request failed",
                        entry("ERROR=%s", e.what()));
        return ipmi::responseResponseError();
    }

    const auto& [status, netFn, rqLun, cmd, cc, data] = response;

    if (status)
    {
        log<level::ERR>("SendMessage, IPMB response non zero status");
        return ipmi::responseResponseError();
    }

    IpmbResponse ipmbResponse(ipmbRequest.rqSA, netFn, rqLun, ipmbRequest.rsSA,
                              ipmbRequest.seq, ipmbRequest.rsLun, cmd, cc,
                              data);

    switch (static_cast<ChannelMode>(request.getMode()))
    {
        case ChannelMode::NoTracking:
            if (responseQueue.size() == ResponseQueueSizeMax)
            {
                log<level::ERR>("SendMessage - Resonse queue is full");
                return ipmi::responseBusy();
            }
            else
            {
                responseQueue.push(ipmbResponse);
                return ipmi::responseSuccess(std::vector<uint8_t>{});
            }
            break;
        case ChannelMode::TrackRequest:
            return ipmi::responseSuccess(ipmbResponse.convertToI2cResponse());
            break;
        default:
            log<level::ERR>("SendMessage - Mode not supported",
                            entry("MODE=%u", request.getMode()));
            return ipmi::responseInvalidFieldRequest();
    }

    return ipmi::responseUnspecifiedError();
}

/** @brief: Implement the clear message flags command
 *  @param flags - 8 bits flags that need to be cleared.
 *                     [7] - 1b = Clear OEM 2
 *                     [6] - 1b = Clear OEM 1
 *                     [5] - 1b = Clear OEM 0
 *                     [4] - reserved
 *                     [3] - 1b = Clear watchdog pre-timeout interrupt flag
 *                     [2] - reserved
 *                     [1] - 1b = Clear Event Message Buffer.
 *                     [0] - 1b = Clear Receive Message Queue.
 *
 *  @returns IPMI completion code
 */
ipmi::RspType<> ipmiAppClearMessageFlags(std::bitset<8> flags)
{
    if (flags.test(static_cast<size_t>(GetMessageFlagsBit::ReceiveMessage)))
    {
        std::queue<IpmbResponse> empty;
        responseQueue.swap(empty);
    }

    // Watchdog pre-timeout interrupt not support now.

    return ipmi::responseSuccess();
}

/** @brief: Implement the get message flags command
 *
 *  @returns IPMI completion code plus response data
 *   bitset<8> - 8 bits data.
 *                   [7] - 1b = OEM 2 data available.
 *                   [6] - 1b = OEM 1 data available.
 *                   [5] - 1b = OEM 0 data available.
 *                   [4] - reserved
 *                   [3] - 1b = Watchdog pre-timeout interrupt occurred
 *                   [2] - reserved
 *                   [1] - 1b = Event Message Buffer Full.
 *                   [0] - 1b = Receive Message Available.
 */
ipmi::RspType<std::bitset<8>> ipmiAppGetMessageFlags()
{
    std::bitset<8> flags;
    if (responseQueue.size() > 0)
    {
        flags.set(static_cast<size_t>(GetMessageFlagsBit::ReceiveMessage));
    }

    flags.set(static_cast<size_t>(GetMessageFlagsBit::EventMessage));

    // Watchdog pre-timeout interrupt not support now.

    return ipmi::responseSuccess(flags);
}

/** @brief: Implement the get message command
 *
 *  @returns IPMI completion code plus response data
 *   uint8_t channelNumber - Channel Number.
 *                               [7:4] Inferred privilege level for message.
 *                               [3:0] channel number.
 *   vector<uint8_t> data - Message Data.
 */
ipmi::RspType<uint8_t, std::vector<uint8_t>> ipmiAppGetMessage()
{
    if (responseQueue.empty())
    {
        log<level::INFO>("GetMessage - No data available in message queue");
        return ipmi::response(static_cast<uint8_t>(IPMI_CC_DATA_NOT_AVAILABLE));
    }

    GetMessageResponse response;

    response.setChannelType(ChannelType::SystemInterface);
    response.setCommandPrivilege(CommandPrivilege::SYSTEM_INTERFACE);

    response.data = responseQueue.front().convertToI2cResponse();
    responseQueue.pop();

    return ipmi::responseSuccess(response.channelNumber, response.data);
}

/** @brief: Implement the send message command
 *  @param channelNumber - [7:6] 00b = No tracking.
 *                               01b = Track Request.
 *                               10b = Send Raw.
 *                               11b = reserved.
 *                         [5] 1b = Send message with encryption.
 *                             0b = Encryption not required.
 *                         [4] 1b = Send message with authentication.
 *                             0b = Authentication not required.
 *                         [3:0] channel number to send message to.
 *  @param data - Message Data.
 *  @returns IPMI completion code plus response data
 */
ipmi::RspType<std::vector<uint8_t>>
    ipmiAppSendMessage(uint8_t channelNumber, std::vector<uint8_t> data)
{
    SendMessageRequest request{channelNumber, data};
    if (request.getAuthentication())
    {
        log<level::ERR>("SendMessage - Authentication not supported");
        return ipmi::responseInvalidFieldRequest();
    }

    if (request.getEncryption())
    {
        log<level::ERR>("SendMessage - Encryption not supported");
        return ipmi::responseInvalidFieldRequest();
    }

    if ((request.getMode() != static_cast<uint8_t>(ChannelMode::NoTracking)) &&
        (request.getMode() != static_cast<uint8_t>(ChannelMode::TrackRequest)))
    {
        log<level::ERR>("SendMessage - Mode not supported",
                        entry("MODE=%u", request.getMode()));
        return ipmi::responseInvalidFieldRequest();
    }

    switch (static_cast<ChannelType>(request.getChannelType()))
    {
        case ChannelType::IPMB:
        case ChannelType::OtherLan:
            return sendIpmbMessage(request);
        default:
            log<level::ERR>("SendMessage - Channel type not supported",
                            entry("TYPE=%u", request.getChannelType()));
            return ipmi::responseInvalidFieldRequest();
    }

    return ipmi::responseUnspecifiedError();
}

void registerBridgingFunctions()
{
    // <Clear Message Flags>
    ipmi::registerHandler(ipmi::prioOemBase, ipmi::netFnApp,
                          ipmi::app::cmdClearMessageFlags,
                          ipmi::Privilege::User, ipmiAppClearMessageFlags);

    // <Get Message Flags>
    ipmi::registerHandler(ipmi::prioOemBase, ipmi::netFnApp,
                          ipmi::app::cmdGetMessageFlags, ipmi::Privilege::User,
                          ipmiAppGetMessageFlags);

    // <Get Message>
    ipmi::registerHandler(ipmi::prioOemBase, ipmi::netFnApp,
                          ipmi::app::cmdGetMessage, ipmi::Privilege::User,
                          ipmiAppGetMessage);

    // <Send Message>
    ipmi::registerHandler(ipmi::prioOemBase, ipmi::netFnApp,
                          ipmi::app::cmdSendMessage, ipmi::Privilege::User,
                          ipmiAppSendMessage);
}
