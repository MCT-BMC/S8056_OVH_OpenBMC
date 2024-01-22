/* Copyright 2018 Intel
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include <endian.h>
#include <err.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <sys/mman.h>

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

static void setProperty(sdbusplus::bus::bus& bus, const std::string& path,
                 const std::string& property, const int64_t value)
{
    auto method = bus.new_method_call(SOL_PATTERN_SERVICE, path.c_str(),
                                      PROPERTY_INTERFACE, "Set");

    method.append(SOL_PATTERN_INTERFACE, property, sdbusplus::message::variant<int64_t>(value));
    bus.call_noreply(method);

    return;
}

static int setPropertyString(sdbusplus::bus::bus& bus, const std::string& path,
                 const std::string& property, const std::string& value, const std::string service, const std::string interface)
{
    auto method = bus.new_method_call(service.c_str(), path.c_str(),
                                      PROPERTY_INTERFACE, "Set");

    method.append(interface.c_str(), property, sdbusplus::message::variant<std::string>(value));
    bus.call_noreply(method);

    return 0;
}


static int getPatternString(sdbusplus::bus::bus& bus, const std::string& path,
                 std::string& pattern)
{

    auto method = bus.new_method_call(SOL_PATTERN_SERVICE, path.c_str(),
                                      PROPERTY_INTERFACE, "Get");
    method.append(SOL_PATTERN_INTERFACE,"PatternString");
    auto reply=bus.call(method);

    if (reply.is_method_error())
    {
        // Its okay if we do not see a corresponding physical LED.
        std::printf("Error looking up sol services, PATH=%s",SOL_PATTERN_INTERFACE);
        return -1;
    }

    sdbusplus::message::variant<std::string> patterntmp;

    try
    {
        reply.read(patterntmp);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        std::printf("Failed to get pattern string for match process");
        return -1;
    }
   
    pattern= std::get<std::string>(patterntmp);
    //std::printf("pattern=%s\n",pattern.c_str());

    return 0;
}

int64_t sol_pattern_match_count(std::string pattern)
{
    char solFilename[30]="/var/log/obmc-console.log";
    char cmd[400]={0};
    char response[50]="";
    int rc=0;
   
    if (access(solFilename,F_OK) !=0)
    {
        printf("sol log not exist!]n");
        return -1;
    }
    
    if (pattern.empty())
    {
        return 0;
    }

    memset(cmd,0,sizeof(cmd));
    snprintf(cmd,sizeof(cmd),"egrep -a -m 256 -o \"%s\" %s | wc -l ",pattern.c_str(),solFilename);
    //std::printf("%s\n",cmd);

    //system(cmd);

    memset(response,0,sizeof(response));
    rc=execmd(cmd,response);

    if(rc==0)
    {
        std::printf("number=%x!\n",strtol(response,NULL,16));
        return strtol(response,NULL,16);
    }else
    {
        //std::printf("FIO not access!\n");
        return -1;
    }    

}


/**
 * @brief Main
 */
int main(int argc, char *argv[])
{	
   
    char solpatternpath[100]={0};
    char solpatternflag[100]={0};
    size_t dataLen = 0;
    size_t bytesRead = 0;
    int64_t count[4]={0};
    std::string pattern[4];

    /* check pattern string first time*/
    int index=0;
 
    FILE *fp;
    size_t len;
    char *temp;
    char temp2[20]={0};
    char buf[300]={0};
    int i=0;

    /* use default conf to initialize resat api solpattern value first */

    fp=fopen("/etc/sol_pattern","r");
    if(fp!=NULL)
    {
        while(!feof(fp))
        {
            fscanf(fp,"%s",&buf);
            temp=strtok(buf,"=");
            if(temp==NULL) break;
            sscanf(temp,"solpattern%s",temp2);
            temp=strtok(NULL,"=");
            if(temp==NULL)
            {
               strcpy(temp2,"");
               break;
            }

            pattern[atoi(temp2)-1]=temp;
            i++;
        }

        fclose(fp);
    }
 
    auto bus = sdbusplus::bus::new_default();

    for(index=0;index<4;index++)
    {
       memset(solpatternpath,0,sizeof(solpatternpath));
       snprintf(solpatternpath,sizeof(solpatternpath),"%s%d",SOL_PATTEN_OBJECTPATH_BASE,index+1);
       //std::printf("pattern[%d]=%s\n",index,pattern[index].c_str());
       setPropertyString(bus,solpatternpath,"PatternString",pattern[index],SOL_PATTERN_SERVICE,SOL_PATTERN_INTERFACE);
    }

    /* Start sol monitor funcition*/
    while(true)
    {

        index=0;
        for(index=0;index<4;index++)
        {
            /*check if pattern change by command*/
            memset(solpatternflag,0,sizeof(solpatternflag));
            snprintf(solpatternflag,sizeof(solpatternflag),"/var/tmp/solpattern_modflag%d",index+1);
  
            memset(solpatternpath,0,sizeof(solpatternpath));
            snprintf(solpatternpath,sizeof(solpatternpath),"%s%d",SOL_PATTEN_OBJECTPATH_BASE,index+1);

            if (access(solpatternflag,F_OK) ==0)
            {
                std::string patternpath=solpatternpath;
                std::printf("original pattern[%d]:%s  ",index, pattern[index].c_str());
                getPatternString(bus, patternpath, pattern[index]);
                std::printf("modified pattern[%d]:%s\n",index, pattern[index].c_str());
                //std::printf("pattern[%d]=%s\n",index,pattern[index].c_str());
                unlink(solpatternflag);
            }

            count[index]=sol_pattern_match_count(pattern[index]);

            /*Update match count number*/

            if(count[index]>=0)
                setProperty(bus,solpatternpath,"MatchCount",count[index]);
        }
        sleep(5);
    }	

    return 0;
}
