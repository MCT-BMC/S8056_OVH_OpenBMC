/* Copyright 2021 MCT
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

#include "mct-default-setting.hpp"

static constexpr bool DEBUG = false;

static bool logSELEvent(std::string enrty, std::string path ,
                     uint8_t eventData0, uint8_t eventData1, uint8_t eventData2)
{
    std::vector<uint8_t> eventData(3, 0xFF);
    eventData[0] = eventData0;
    eventData[1] = eventData1;
    eventData[2] = eventData2;
    uint16_t genid = 0x20;
    bool assert = 1;

    auto bus = sdbusplus::bus::new_default();

    sdbusplus::message::message writeSEL = bus.new_method_call(
        "xyz.openbmc_project.Logging.IPMI", "/xyz/openbmc_project/Logging/IPMI",
        "xyz.openbmc_project.Logging.IPMI", "IpmiSelAdd");
    writeSEL.append(enrty, path, eventData, true, (uint16_t)genid);
    try
    {
        bus.call(writeSEL);
    }
    catch (std::exception& e)
    {
        std::cerr << "Failed to log the event:" << e.what() << "\n";
        return false;
    }

    return true;
}

static bool setACBoot(std::string acBoot)
{
    auto bus = sdbusplus::bus::new_default();

    sdbusplus::message::message method = bus.new_method_call(
        "xyz.openbmc_project.Settings", "/xyz/openbmc_project/control/host0/ac_boot",
        "org.freedesktop.DBus.Properties", "Set");
    method.append("xyz.openbmc_project.Common.ACBoot","ACBoot", std::variant<std::string>(acBoot));
    try
    {
        bus.call(method);
    }
    catch (std::exception& e)
    {
        std::cerr << "Failed to set the ACBOOT:" << e.what() << "\n";
        return false;
    }

    return true;
}

bool acLostSetting()
{
    std::cerr << "Check BMC AC lost status..." << std::endl;

    uint32_t registerBuffer = 0;
    uint32_t acLostFlag = 0x01;
    if(read_register(0x1E6E203C,&registerBuffer) < 0)
    {
        std::cerr<<"Failed to read SCU3C register \n";
        return false;
    }

    if(registerBuffer & acLostFlag)
    {
        logSELEvent("AC Lost SEL Entry",
                    "/xyz/openbmc_project/sensors/power_unit/POWER_STATUS",0x04,0xFF,0xFF);

        registerBuffer = registerBuffer & 0xFFFFFFFE;
        write_register(0x1E6E203C, registerBuffer);
        setACBoot("True");
        return true;
    }
    else
    {
        std::ofstream bmcReboot("/run/bmcRebootFlag");
        std::cerr << "Create bmcRebootFlag flag" << '\n';
        bmcReboot.close();
        setACBoot("False");
        return false;
    }
    return false;
}

bool checkWatchdogExpected()
{
    uint32_t registerBuffer1 = 0;
    uint32_t registerBuffer2 = 0;
    uint32_t sramRegiter = 0x1e723010;
    // ASCII IPMI
    uint32_t switchMask1 = 0x49504D49;
    // Checksum
    uint32_t switchMask2 = 0xD1000000;

    if (read_register(sramRegiter, &registerBuffer1) < 0)
    {
        std::cerr<<"Failed to read SRAM register \n";
        return false;
    }
    if (read_register(sramRegiter + 4, &registerBuffer2) < 0)
    {
        std::cerr<<"Failed to read SRAM with offset 4 register \n";
        return false;
    }

    if (registerBuffer1 == switchMask1 &&
        registerBuffer2 == switchMask2)
    {
        write_register(sramRegiter, 0x0);
        write_register(sramRegiter + 4, 0x0);
        return true;
    }
    return false;
}

bool bmcWatchdogTimeoutSetting(bool& switchBmc)
{
    std::cerr << "Check BMC watchdog status..." << std::endl;

    uint32_t registerBuffer = 0;
    uint32_t wdt2Flag = 0x08;

    if (read_register(0x1E6E203C, &registerBuffer) < 0)
    {
        std::cerr<<"Failed to read SCU3C register \n";
        return false;
    }

    if(registerBuffer & wdt2Flag)
    {
        switchBmc = checkWatchdogExpected();
        if(!switchBmc){
            logSELEvent("BMC WDT Reboot SEL Entry",
                        "/xyz/openbmc_project/sensors/mgtsubsyshealth/WATCHDOG_REBOOT",0x02,0xFF,0xFF);
        }

        registerBuffer = registerBuffer & 0xFFFFFFF7;
        write_register(0x1E6E203C, registerBuffer);
        return true;
    }

    return false;
}

bool kernelPanicSetting()
{
    std::cerr << "Check BMC kernel panic status..." << std::endl;

    uint32_t registerBuffer1 = 0;
    uint32_t registerBuffer2 = 0;
    uint32_t sramRegiter = 0x1e723000;
    // ASCII pani
    uint32_t panicMask1 = 0x50414E49;
    // ASCII c & checksum
    uint32_t panicMask2 = 0x43950000;

    if (read_register(sramRegiter, &registerBuffer1) < 0)
    {
        std::cerr<<"Failed to read SRAM register \n";
        return false;
    }
    if (read_register(sramRegiter + 4, &registerBuffer2) < 0)
    {
        std::cerr<<"Failed to read SRAM with offset 4 register \n";
        return false;
    }

    if (registerBuffer1 == panicMask1 &&
        registerBuffer2 == panicMask2)
    {
        logSELEvent("BMC Kernel Panic SEL Entry",
            "/xyz/openbmc_project/sensors/mgtsubsyshealth/BMC_REBOOT",0x01,0xFF,0xFF);
        write_register(sramRegiter, 0x0);
        write_register(sramRegiter + 4, 0x0);
        return true;
    }

    return false;
}

void bmcRebootSetting(bool acLost,bool kernelPanic)
{
    std::cerr << "Check BMC reboot status..." << std::endl;

    if(!acLost && !kernelPanic)
    {
        logSELEvent("BMC Reboot SEL Entry",
                    "/xyz/openbmc_project/sensors/mgtsubsyshealth/BMC_REBOOT",0x02,0xFF,0xFF);
    }
}

bool bmcBootSourceSetting(uint8_t& bootSource)
{
    std::cerr << "Check BMC flash booting source..." << std::endl;

    uint32_t registerBuffer = 0;
    uint32_t bootSourceFlag = 0x02;

    static const std::string bootSourceFilePath = "/run/boot-source";

    if (read_register(0x1e785030, &registerBuffer) < 0)
    {
        return false;
    }

    if(registerBuffer & bootSourceFlag)
    {
        bootSource = 1;
    }

    std::ofstream bootSourceFileOutput(bootSourceFilePath.c_str());
    if(bootSourceFileOutput)
    {
        bootSourceFileOutput << +bootSource;
        bootSourceFileOutput.close();
    }

    return true;
}

void bmcSwitchSetting(bool watchdogTimout, bool switchBmc, uint8_t bootSource)
{
    if(!watchdogTimout && !switchBmc)
    {
        return;
    }

    if(bootSource)
    {
        logSELEvent("BMC2 Switching SEL Entry",
                    "/xyz/openbmc_project/sensors/fru_state/SWITCH_BMC2",0x02,0x01,0xFF);
    }
    else
    {
        logSELEvent("BMC1 Switching SEL Entry",
                    "/xyz/openbmc_project/sensors/fru_state/SWITCH_BMC1",0x02,0x01,0xFF);
    }
}

void ipv6Setting()
{
    const static std::string dhcpConfigPath = "/etc/dhcp-config.json";

    std::ifstream dhcpConfig(dhcpConfigPath);
    nlohmann::json dhcpConfigInfo;

    if(!dhcpConfig)
    {
        std::cerr << "Using default setting for IP family." << std::endl;
        return;
    }

    dhcpConfigInfo = nlohmann::json::parse(dhcpConfig, nullptr, false);

    for (auto it = dhcpConfigInfo.begin(); it != dhcpConfigInfo.end(); ++it)
    {
        if(it.value() == "DualStack")
        {
            std::cerr << it.key() << ": Using dual stack for IP family." << std::endl;
        }
        else if(it.value() == "IPv4Only")
        {
            std::cerr << it.key() << ": Using IPv4 only for IP family." << std::endl;
            std::string cmd = "sysctl -w net.ipv6.conf." + it.key() + ".disable_ipv6=1";
            system(cmd.c_str());
        }
        else if(it.value() == "IPv6Only")
        {
            std::cerr << it.key() << ": Using IPv6 only for IP family." << std::endl;
        }
    }
}

void biosVersionSetting()
{
    system("/usr/bin/env mct-bios-version-handler");
}

int main(int argc, char *argv[])
{
    bool switchBmc = false;
    uint8_t bootSource = 0;

    bool acLost = acLostSetting();
    bool watchdogTimout = bmcWatchdogTimeoutSetting(switchBmc);
    bool kernelPanic = kernelPanicSetting();
    bmcRebootSetting(acLost,kernelPanic);
    bmcBootSourceSetting(bootSource);
    bmcSwitchSetting(watchdogTimout,switchBmc,bootSource);
    ipv6Setting();
    biosVersionSetting();

    return 0;
}