#include <stdint.h>
#include <host-ipmid/ipmid-api.h>

#define MAXBUFFERLEN 256


struct SetPatternBuffersReq
{
    uint8_t patternNum;
    uint8_t patternString[MAXBUFFERLEN];
};

struct GetPatternBuffersReq
{
    uint8_t patternNum;
};

struct GetPatternBuffersRes
{
    char patternString[MAXBUFFERLEN];
};


enum ipmi_net_fns_oem
{
    NETFUN_TWITTER_OEM = 0x30,
};

enum ipmi_net_fns_oem_cmds
{
    IPMI_CMD_SetPatternBuffers = 0xB2,
    IPMI_CMD_GetPatternBuffers = 0xB3,
};

enum ipmi_netfn_none_cmds
{

};


ipmi_ret_t ipmi_twitter_SetPatternBuffers(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                              ipmi_request_t request, ipmi_response_t response,
                              ipmi_data_len_t data_len, ipmi_context_t context);
ipmi_ret_t ipmi_twitter_GetPatternBuffers(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                              ipmi_request_t request, ipmi_response_t response,
                              ipmi_data_len_t data_len, ipmi_context_t context);
