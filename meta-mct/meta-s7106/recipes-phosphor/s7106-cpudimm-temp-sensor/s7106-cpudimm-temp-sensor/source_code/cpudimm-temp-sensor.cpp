/* Copyright 2019 MCT
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

#include <fcntl.h>
#include <inttypes.h>
#include <poll.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/timerfd.h>
#include <sys/stat.h>
#include <dirent.h>

#include <algorithm>
#include <vector>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/message.hpp>
#include <sdbusplus/vtable.hpp>
#include <sdbusplus/server/interface.hpp>
#include <phosphor-logging/log.hpp>
#include <iostream>
#include <getopt.h>

#include <fstream>


#define BUS_FD 0
#define TIMER_FD 1
#define TOTAL_FDS 2

constexpr const char *DBUS_NAME = "xyz.openbmc_project.Ipmi.Ipmb.Cpudimm.temp";
constexpr const char *OBJ_NAME = "/xyz/openbmc_project/Ipmi/Ipmb/Cpudimm/temp";
constexpr const char *IPMB_DBUS_INTF = "org.openbmc.Ipmb";

constexpr const char *IPMBBridged_BUS = "xyz.openbmc_project.Ipmi.Channel.Ipmb";
constexpr const char *IPMBBridged_OBJ = "/xyz/openbmc_project/Ipmi/Channel/Ipmb";
constexpr size_t ipmbMaxDataSize = 256;
uint8_t cpu0_Tjmax = 0;
uint8_t cpu1_Tjmax = 0;

sdbusplus::bus::bus bus = sdbusplus::bus::new_system();

static int CreateFile(const char *path, const char *dataToWrite){
	int fd = open(path, O_CREAT | O_EXCL | O_RDWR | O_CLOEXEC);

	if(fd < 0) return -1;

	if(dataToWrite != NULL)
		write(fd, dataToWrite, strlen(dataToWrite));

	close(fd);
}

static int createHWmonFS(){

	uint8_t ToCreateFiles = 0;
	
	DIR *dir = opendir("/run/vsensor");
	if(dir == NULL){ 		
		if(mkdir("/run/vsensor", 0755) == -1) return -1;
	}
	else closedir(dir);

	dir = opendir("/run/vsensor/cpudimm-temp");
	if(dir == NULL){
		if(mkdir("/run/vsensor/cpudimm-temp", 0755) == -1) return -1;
		
		ToCreateFiles = 1;
	}
	else closedir(dir);

	if(ToCreateFiles){
		CreateFile("/run/vsensor/cpudimm-temp/device", NULL);
		CreateFile("/run/vsensor/cpudimm-temp/name", "cpudimm_temp\n");
		CreateFile("/run/vsensor/cpudimm-temp/of_node", NULL);
		CreateFile("/run/vsensor/cpudimm-temp/subsystem", NULL);
		CreateFile("/run/vsensor/cpudimm-temp/temp1_input", "40000\n");
		CreateFile("/run/vsensor/cpudimm-temp/uevent", "OF_NAME=cpudimm_temp\nOF_FULLNAME=/cpudimm-temp\nOF_COMPATIBLE_0=cpudimm-temp\nOF_COMPATIBLE_N=1\n");
	}		
	
	return 0;
}

int response_process_cpu0_tjmax (uint8_t seq, uint8_t netfn, uint8_t lun, 
                        uint8_t cmd, uint8_t cc, std::vector<uint8_t> rspData)
{
 
    if(netfn == 0x5 && cmd == 0x2d && seq == 0x01)
    {
        cpu0_Tjmax = rspData.at(0);
    }
    return 0;
}

int response_process_cpudimm_temp (uint8_t seq, uint8_t netfn, uint8_t lun, 
                        uint8_t cmd, uint8_t cc, std::vector<uint8_t> rspData)
{
    uint8_t cpu0_temp = 0;
    uint8_t dimm0_temp = 0;
    std::fstream file;

    if(netfn == 0x2f && cmd == 0x4b && seq == 0x02 && 
        rspData.at(0) == 0x57 && rspData.at(1) == 0x01 && rspData.at(2) == 0x00)
    {
        cpu0_temp = rspData.at(3);
        dimm0_temp = rspData.at(5);
     
        /*CPU0 Temp*/
        if((cpu0_temp != 0xff) && (cpu0_Tjmax != 0) && (cpu0_Tjmax != 0xff))
        {
            cpu0_temp = cpu0_Tjmax - cpu0_temp;
        }
        else
            cpu0_temp = 0;

        
        file.open("/run/vsensor/cpudimm-temp/temp1_input",std::ios::out);
        if(!file)
        {
            std::cout << "error, can't open temp1_input" << std::endl;
            return -1;
        }
    
        file << cpu0_temp*1000 << std::endl;

        file.close();
        
        /*DIMM 0 Temp*/
        if(dimm0_temp == 0xff)
        {
            if( 0 == access("/run/vsensor/cpudimm-temp/temp3_input",F_OK))
                remove("/run/vsensor/cpudimm-temp/temp3_input");
            
        }
        else
        {    
            file.open("/run/vsensor/cpudimm-temp/temp3_input",std::ios::out);
            if(!file)
            {
                std::cout << "error, can't open temp3_input" << std::endl;
                return -1;
            }
            
            file << dimm0_temp*1000 << std::endl;
            file.close();
        }

    }
    return 0;
}

static int method_process_Response(sd_bus_message *bus_msg, void *userdata,
							   sd_bus_error *ret_error)
{
    uint8_t netfn = 0, lun = 0, seq = 0, cmd = 0, cc = 0;
	static uint8_t firstCall = 1;
    std::vector<uint8_t> dataReceived;
	int input_fd = -1;
	char reading_str[7]={0};

    dataReceived.reserve(ipmbMaxDataSize);
    auto mesg = sdbusplus::message::message(bus_msg);

	mesg.read(seq, netfn, lun, cmd, cc, dataReceived);

#if 0	
	try{
		mesg.read(dataReceived);		
	}
	catch (const std::exception &e){
		sd_journal_print(LOG_INFO, "No any array data from DBUS");
		printf("No any array data from DBUS\n");
	}

    if (dataReceived.size() > ipmbMaxDataSize)
    {
        return 1;
    }
#endif 

#if 0
    {
        printf("recv: %02x %02x %02x %02x %02x : ",seq,netfn,lun,cmd,cc);
        std::vector<uint8_t>::iterator iter_u8;
        for(iter_u8=dataReceived.begin(); iter_u8 != dataReceived.end(); ++iter_u8)
            printf("%02x ",*iter_u8);
        printf("\n");

    }
#endif

	if(firstCall){
		firstCall = 0;
		createHWmonFS();
	}

	if(cc == 0x00)  //Completion code = 0x00
    {  
        response_process_cpu0_tjmax(seq, netfn, lun, cmd, cc, dataReceived);
        response_process_cpudimm_temp(seq, netfn, lun, cmd, cc, dataReceived);
       
	}
	
	return 0;
}
	
static const sdbusplus::vtable::vtable_t ipmbVtable[] = {
    sdbusplus::vtable::start(),
    sdbusplus::vtable::method("returnResponse", "yyyyyay", "", method_process_Response),
    sdbusplus::vtable::end()
};

struct MeCmd{
    uint8_t select;
    uint8_t seq;
    uint8_t netFun;
    uint8_t cmd;
    uint8_t len;
    uint8_t data[32];
};

struct MeCmd meCmd[] = 
{
    // Get CPU0 Tjmax sensor 
    {1, 0x01,   0x4,    0x2d,   1,  {0x30}},
    // Get All CPU and DIMM temperature    
    {2, 0x02,   0x2e,   0x4b,   12, {0x57,0x01,0x00,0x03,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},     
    {0, 0, 0, 0, 0, {0}}
};    

int select_command(uint8_t select, uint8_t *seq, uint8_t *netfun, uint8_t *cmd, std::vector<uint8_t>& data)
{
    int i = 0;

    while(meCmd[i].select != 0)
    {
        if(meCmd[i].select == select)
            break;
        i++;
    }

    if(meCmd[i].select == 0)
    {   
	    sd_journal_print(LOG_ERR,"Can't found ME command\n");
        printf("Can't found ME command\n");
        return -1;
    }

    *netfun = meCmd[i].netFun;
    *cmd = meCmd[i].cmd;
    *seq = meCmd[i].seq;
    data.clear();
    data.assign(meCmd[i].data,meCmd[i].data+meCmd[i].len);
#if 0    
    std::vector<uint8_t>::iterator it_u8;
    for(it_u8 = data.begin(); it_u8 != data.end(); ++it_u8)
        printf("%02x ",*it_u8);
    printf("\n");
#endif    
    
    return 0;
}


/**
 * @brief Main
 */
int main(int argc, char *argv[])
{	
	int r = -1, doonce = 1;
	uint8_t netFn = 0x0;
   	uint8_t rqLun = 0x00;
  	uint8_t seq = 0x00;
    uint8_t cmd = 0x0;	
	struct pollfd poll_fds[TOTAL_FDS];
	std::vector<uint8_t> data(32);
    uint8_t reqData[] = {48};
	struct itimerspec iTimerpec;
    uint16_t i;
    uint16_t reqLen = (uint16_t)sizeof(reqData);
	int opt;
	char ipmbDeviceName[20]={0};
    static const struct option long_options[] = 
    {
        {"d", required_argument, NULL, 'd'},
        {0, 0, 0, 0}
    };
	
	opt = getopt_long(argc, argv, "", long_options, NULL);
	snprintf(ipmbDeviceName, sizeof(ipmbDeviceName), "%s", optarg);
	sd_journal_print(LOG_ERR,"cpudimm temp sensor,ipmbDeviceName=%s!!!",ipmbDeviceName);
	
	sdbusplus::server::interface::interface ipmiInterface(
			bus, OBJ_NAME, IPMB_DBUS_INTF, ipmbVtable, nullptr);
	
	bus.request_name(DBUS_NAME);
	poll_fds[BUS_FD].fd = bus.get_fd();
	if (poll_fds[BUS_FD].fd < 0)
    {
        sd_journal_print(LOG_ERR,"Error getting fd from bus object");
        return -1;
    }

	poll_fds[TIMER_FD].fd = timerfd_create(CLOCK_MONOTONIC, 0);

	if (poll_fds[TIMER_FD].fd < 0)
    {
    	sd_journal_print(LOG_ERR,"Error creating Timer");
        return -1;
    }	

	iTimerpec.it_value.tv_sec = 2;
	iTimerpec.it_value.tv_nsec = 0;
	iTimerpec.it_interval.tv_sec = 0;
	iTimerpec.it_interval.tv_nsec = 0;
	
	r = timerfd_settime(poll_fds[TIMER_FD].fd, 0, &iTimerpec, NULL);

	if (r < 0) {
		sd_journal_print(LOG_ERR, "timerfd_settime error, Error:[%d:%s]", errno, strerror(errno));
		close(poll_fds[TIMER_FD].fd);
		return -1;
	}
	
	poll_fds[BUS_FD].events = POLLIN;
	poll_fds[TIMER_FD].events = POLLIN;	

	while(1){
		
		r = poll(poll_fds, TOTAL_FDS, -1);

        if (r < 0)
        {
            continue;
        }

        // received dbus event
        if (poll_fds[BUS_FD].revents & POLLIN)
        {        
            bus.process_discard();
        }

		if (poll_fds[TIMER_FD].revents & POLLIN)
        {
            /*Send CPU 0 Tjmax command to ME*/
            select_command(1,&seq,&netFn,&cmd,data);
            sdbusplus::message::message mesg = bus.new_signal(OBJ_NAME, IPMB_DBUS_INTF, "sendRequest");
            mesg.append(ipmbDeviceName, seq, netFn, rqLun, cmd, data); 
            mesg.signal_send();

            /*Send 0x2e 0x4b command to ME*/ 
            select_command(2,&seq,&netFn,&cmd,data);
            mesg = bus.new_signal(OBJ_NAME, IPMB_DBUS_INTF, "sendRequest");
            mesg.append(ipmbDeviceName, seq, netFn, rqLun, cmd, data); 
            mesg.signal_send();

            //send command to ME interval time
			iTimerpec.it_value.tv_sec = 5;
			iTimerpec.it_value.tv_nsec = 0;
			iTimerpec.it_interval.tv_sec = 0;
			iTimerpec.it_interval.tv_nsec = 0;
			r = timerfd_settime(poll_fds[TIMER_FD].fd, 0, &iTimerpec, NULL);
			if (r < 0) {
				sd_journal_print(LOG_ERR, "timerfd_settime error, Error:[%d:%s]", errno, strerror(errno));
				close(poll_fds[TIMER_FD].fd);
				return -1;
			}
		 }
	}
		
    return 0;
}
