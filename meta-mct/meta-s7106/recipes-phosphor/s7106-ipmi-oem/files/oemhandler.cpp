#include "oemhandler.hpp"
#include <host-ipmid/ipmid-api.h>
#include <fstream>
#include <stdio.h>
#include <string.h>
#include <systemd/sd-bus.h>
#include <endian.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <iostream>

#include <linux/types.h>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/message.hpp>
#include <sdbusplus/vtable.hpp>
#include <sdbusplus/server/interface.hpp>


#define SOL_PATTEN_OBJECTPATH_BASE "/xyz/openbmc_project/oem/solpattern"
#define SOL_PATTERN_SERVICE "xyz.openbmc_project.Settings"
#define SOL_PATTERN_INTERFACE "xyz.openbmc_project.OEM.SOLPattern"
#define PROPERTY_INTERFACE "org.freedesktop.DBus.Properties"


#define SOL_PATTERN_MAXLEN 256

void register_netfn_twitter_oem() __attribute__((constructor));

int execmd(char* cmd,char* result) {
        char buffer[128];
        FILE* pipe = popen(cmd, "r");
        if (!pipe)
                return -1;

        while(!feof(pipe)) {
                if(fgets(buffer, 128, pipe)){
                        strcat(result,buffer);
                }
        }
        pclose(pipe);
        return 0;
}

#if 1
static int setPropertyString(sdbusplus::bus::bus& bus, const std::string& path,
                 const std::string& property, const std::string& value, const std::string service, const std::string interface)
{
    auto method = bus.new_method_call(service.c_str(), path.c_str(),
                                      PROPERTY_INTERFACE, "Set");

    method.append(interface.c_str(), property, sdbusplus::message::variant<std::string>(value));
    bus.call_noreply(method);

    return 0;
}
#endif
static int getPatternString(sdbusplus::bus::bus& bus, const std::string& path,
                 const std::string& property, std::string& value, const std::string service, const std::string interface)
           //const std::string& property, const char* value, const std::string service, const std::string interface)
{
    auto method = bus.new_method_call(service.c_str(), path.c_str(),
                                      PROPERTY_INTERFACE, "Get");
    method.append(interface.c_str(),property);
    auto reply=bus.call(method);

    if (reply.is_method_error())
    {
        // Its okay if we do not see a corresponding physical LED.
        std::printf("Error looking up services, PATH=%s",interface.c_str());
        return -1;
    }

    sdbusplus::message::variant<std::string> valuetmp;

    try
    {
        reply.read(valuetmp);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        std::printf("Failed to get pattern string for match process");
        return -1;
    }

    value=std::get<std::string>(valuetmp);
   // std::printf("value=%s\n",value.c_str());

    return 0;
}


/*
    NetFun: 0x30
    Cmd : 0xB2
    Request:
            Byte 1: pattern num (1~4)
            Byte 2-257: pattern data
*/
ipmi_ret_t ipmi_twitter_SetPatternBuffers(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                              ipmi_request_t request, ipmi_response_t response,
                              ipmi_data_len_t data_len, ipmi_context_t context)
{
    ipmi_ret_t ipmi_rc = IPMI_CC_OK;
    int ret=0,i=0;
    char objpath[100]={0};
    char cmdtmp[400]={0};
    int8_t patternNum=0;
    char patternString[256]={0};
    char rebuf[50]={0};
    int8_t matchline=0;
    char patternFile[100]="/etc/sol_pattern";
     
    //std::printf("data len %d",*data_len);
    
    if((int)*data_len>=MAXBUFFERLEN+1 || (int)*data_len < 2) 
    {
        return IPMI_CC_REQ_DATA_LEN_INVALID;
    }

    SetPatternBuffersReq* requestData= reinterpret_cast<SetPatternBuffersReq*>(request);

    patternNum= requestData->patternNum;
    if(patternNum<1 || patternNum>4)
    {
        return IPMI_CC_INVALID_RESERVATION_ID;
    }

    for(i=0;i<(int)((*data_len)-1);i++)
    {
        patternString[i]=requestData->patternString[i];
    
    }
        patternString[i]='\0';

        std::printf("%s\n",patternString);

    auto bus = sdbusplus::bus::new_default();
    memset(objpath,0,sizeof(objpath));
    snprintf(objpath,sizeof(objpath),"%s%d",SOL_PATTEN_OBJECTPATH_BASE,patternNum);

    ret=setPropertyString(bus,objpath,"PatternString",patternString,SOL_PATTERN_SERVICE,SOL_PATTERN_INTERFACE);
    if(ret<0)
    {
        return IPMI_CC_UNSPECIFIED_ERROR;
    }

    /*add sol_pattern update flag for monitor function*/
    memset(cmdtmp,0,sizeof(cmdtmp));
    snprintf(cmdtmp,sizeof(cmdtmp),"touch /var/tmp/solpattern_modflag%d",patternNum);
    system(cmdtmp);

    /*sync to config fire*/
    memset(cmdtmp,0,sizeof(cmdtmp));
    snprintf(cmdtmp,sizeof(cmdtmp),"grep -n solpattern%d %s | awk 'BEGIN {FS=\":\"} {print$1}'",patternNum,patternFile);
    memset(rebuf,0,sizeof(rebuf));
    ret=execmd(cmdtmp,rebuf);
    if(!ret) matchline=(int8_t)strtol(rebuf,NULL,10);
   
    memset(cmdtmp,0,sizeof(cmdtmp));
    snprintf(cmdtmp,sizeof(cmdtmp),"sed -i '%ds/.*/solpattern%d=%s/' %s",matchline,patternNum, patternString, patternFile);
    system(cmdtmp);


    *data_len =0;

    return ipmi_rc;
}

ipmi_ret_t ipmi_twitter_GetPatternBuffers(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                              ipmi_request_t request, ipmi_response_t response,
                              ipmi_data_len_t data_len, ipmi_context_t context)
{

    char objpath[100]={0};
    //char responseData[256]={0};
    char* responseData=(char*) malloc(sizeof(char)*256);
   
    std::string patternString;
    ipmi_ret_t ipmi_rc = IPMI_CC_OK;
    GetPatternBuffersReq* requestData = static_cast<GetPatternBuffersReq*>(request);
    //GetPatternBuffersRes* responseData = static_cast<GetPatternBuffersRes*>(response);

    if(requestData->patternNum<1 || requestData->patternNum>4)
    {
        return IPMI_CC_INVALID_RESERVATION_ID;
    }

    memset(objpath,0,sizeof(objpath));
    snprintf(objpath,sizeof(objpath),"%s%d",SOL_PATTEN_OBJECTPATH_BASE,requestData->patternNum);
    auto bus = sdbusplus::bus::new_default();
    getPatternString(bus,objpath,"PatternString",patternString,SOL_PATTERN_SERVICE,SOL_PATTERN_INTERFACE);
    
    strcpy(responseData,patternString.c_str());
    
    int i=0,j=0;
    for (i=0;i<(int8_t)patternString.length();i++)
    {
        if(j==16)
        {
           j=0;
        }
        j++;
    }
    
    memcpy(response, responseData, strlen(responseData));

    *data_len = strlen(responseData);
    
    return ipmi_rc;

}

void register_netfn_twitter_oem()
{

        ipmi_register_callback(NETFUN_TWITTER_OEM, IPMI_CMD_SetPatternBuffers, NULL, ipmi_twitter_SetPatternBuffers,
                           SYSTEM_INTERFACE);
        ipmi_register_callback(NETFUN_TWITTER_OEM, IPMI_CMD_GetPatternBuffers, NULL, ipmi_twitter_GetPatternBuffers,
                           SYSTEM_INTERFACE);
}
