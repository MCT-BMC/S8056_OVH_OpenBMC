/*
// Copyright (c) 2018 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

#include <Utils.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/asio.hpp>
#include <chrono>
#include <ctime>
#include <iostream>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/asio/sd_event.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/exception.hpp>
#include <sdbusplus/server.hpp>
#include <sdbusplus/timer.hpp>
#include <sensor.hpp>
#include <systemd/sd-journal.h>

static constexpr bool DEBUG = false;

//{sensor name, sensor type}
const static boost::container::flat_map<std::string, std::string>eventsensors
    {
        {"IPMI_Power_Diag","fru_state"},
        {"SYSTEM_RESTART", "restart"}, //system restart sensor
        {"BUTTON","buttons"},
        {"BMC1_FW_UPDATE","versionchange"},
        {"BMC_REBOOT","mgtsubsyshealth"},
        {"WATCHDOG_REBOOT","mgtsubsyshealth"},
        {"PSU1_STATUS","power_supply"},
        {"PSU2_STATUS","power_supply"},
        {"DCMI_POWER_LIMIT","power_supply"},
        {"IPMI_POWER_CYCLE","fru_state"},
        {"IPMI_POWER_ON","fru_state"},
        {"IPMI_POWER_OFF","fru_state"},
        {"IPMI_POWER_SOFT","fru_state"},
        {"IPMI_POWER_RESET","fru_state"},
        {"WATCHDOG2","watchdog"},
        {"EVENT_CLEARED","event_disabled"},
        {"PROCESSOR_FAIL","processor"},
        {"CPU_State","processor"},
        {"MBIST_START","system_event"},
        {"MBIST_COMPLETE","system_event"},
        {"ACPI_POWER_STATE","acpi"},
        {"POWER_STATUS","power_unit"},
        {"HSC_DISABLE","buttons"},
        {"IPMI_HSC_DISABLE","fru_state"},
        {"BIOS1_BOOT_FAIL","system_firmware"},
        {"BIOS2_BOOT_FAIL","system_firmware"},
        {"SELECT_BIOS1","system_firmware"},
        {"SELECT_BIOS2","system_firmware"},
        {"NMI_BUTTON","buttons"},
        {"CLEAR_CMOS","fru_state"},
        {"BIOS1_FW_UPDATE","versionchange"},
        {"BIOS2_FW_UPDATE","versionchange"},
        {"BMC2_FW_UPDATE","versionchange"},
        {"CPLD_FW_UPDATE","versionchange"},
        {"SWITCH_BMC1","fru_state"},
        {"SWITCH_BMC2","fru_state"},
        {"PDB_STATUS","power_supply"},
        {"SYS_PWR_CAPPING","chassis"},
        {"END_OF_POST","system_event"},
        {"CPU_VRHOT","processor"},
        {"SOC_VRHOT","processor"},
        {"MEM_ABCD_VRHOT","processor"},
        {"MEM_EFGH_VRHOT","processor"},
        {"PSU1_FW_UPDATE","versionchange"},
        {"PSU2_FW_UPDATE","versionchange"},
        {"VR_FW_UPDATE","versionchange"},
    };

struct eventSensor 
{
    eventSensor(
                sdbusplus::asio::object_server& objectServer,
                std::shared_ptr<sdbusplus::asio::connection>& conn,
                boost::asio::io_service& io, const std::string& sensorName, const std::string& sensorType):
                objectServer(objectServer), dbusConnection(conn)
    {
        std::string sensorPath = "/xyz/openbmc_project/sensors/"+sensorType+"/"+sensorName;
        if (DEBUG)
        {
            std::cerr << sensorPath << "\n";
        }
        if (!conn)
        {
            std::cerr << "Connection not created\n";
            return;
        }
        
        intf = objectServer.add_interface(sensorPath,sensorValueInterface);
        intf->register_property("Value", value, sdbusplus::asio::PropertyPermission::readWrite);
        if (!intf->initialize())    
        {
            std::cerr << "error initializing value interface\n";    
        }
    }
    ~eventSensor()
    {
         objectServer.remove_interface(intf);
    }
    void init(void);

  private:
    sdbusplus::asio::object_server& objectServer;
    std::shared_ptr<sdbusplus::asio::connection> dbusConnection;
    std::shared_ptr<sdbusplus::asio::dbus_interface> intf;
    double value = 0;
};

void eventSensor::init(void)
{
   //do nothing for now;
}

void createSensors(
    boost::asio::io_service& io, sdbusplus::asio::object_server& objectServer,
    std::shared_ptr<sdbusplus::asio::connection>& dbusConnection, 
    boost::container::flat_map<std::string, std::unique_ptr<eventSensor>>& sensors)
{
    for(auto const& it:eventsensors)
    {
        std::cerr << it.first << ":" << it.second << '\n';
        sensors[it.first] = std::make_unique<eventSensor>(
            objectServer, dbusConnection, io, it.first, it.second );

        sensors[it.first]->init();
    }        
}

int main()
{
    // setup connection to dbus
    boost::asio::io_service io;
    auto systemBus = std::make_shared<sdbusplus::asio::connection>(io);
    auto objServer = sdbusplus::asio::object_server(systemBus);
    boost::container::flat_map<std::string, std::unique_ptr<eventSensor>> sensors;

    // setup object server, define interface
    systemBus->request_name("xyz.openbmc_project.eventSensor");

    if (DEBUG)
    {
        for(auto const& pair:eventsensors)
            std::cerr << pair.first << ":" << pair.second << '\n';
    }

    io.post([&]() {
        createSensors(io, objServer, systemBus,sensors);
    });

    //auto work = std::make_shared<boost::asio::io_service::work>(io);
    io.run();
    return 0;
}
