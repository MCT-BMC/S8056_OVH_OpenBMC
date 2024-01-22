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

#define PROPERTY_INTERFACE "org.freedesktop.DBus.Properties"

constexpr auto SETTIMG_SERVICE = "xyz.openbmc_project.Settings";
constexpr auto BMC_STATUS_PATH = "/xyz/openbmc_project/oem/BmcStatus";
constexpr auto BMC_STATUS_INTERFACE = "xyz.openbmc_project.OEM.BmcStatus";
constexpr auto BMC_PROP = "bootSourceStatus";

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
    if(read_register(0x1e6e2074,&registerBuffer) < 0)
    {
        std::cerr<<"Failed to read SCU3C register \n";
        return false;
    }

    if(registerBuffer & acLostFlag)
    {
        logSELEvent("AC Lost SEL Entry",
                    "/xyz/openbmc_project/sensors/power_unit/POWER_STATUS",0x04,0xFF,0xFF);
        write_register(0x1e6e2074, acLostFlag);
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

    uint32_t bSramControlAddress = 0x1e6ef000;
    uint32_t bSramKey = 0xdba078e2;
    uint32_t bSramStoreBSAddress = 0x1e6ef120;
    // ASCII IPMI
    uint32_t switchMask1 = 0x49504D49;
    // Checksum
    uint32_t switchMask2 = 0xD1000000;

    if (read_register(bSramStoreBSAddress, &registerBuffer1) < 0)
    {
        std::cerr<<"Failed to read boot source BSRAM register \n";
        return false;
    }
    if (read_register(bSramStoreBSAddress + 4, &registerBuffer2) < 0)
    {
        std::cerr<<"Failed to read boot source BSRAM with offset 4 register \n";
        return false;
    }

    if (registerBuffer1 == switchMask1 &&
        registerBuffer2 == switchMask2)
    {
        write_register(bSramControlAddress, bSramKey);
        write_register(bSramStoreBSAddress, 0x0);
        write_register(bSramStoreBSAddress + 4, 0x0);
        write_register(bSramControlAddress, 0x0);
        return true;
    }
    return false;
}

bool bmcWatchdogTimeoutSetting(bool& switchBmc)
{
    std::cerr << "Check BMC watchdog status..." << std::endl;

    uint32_t registerBufferFMC = 0;
    uint32_t FMCeventLogRegiter = 0x1E620064;
    uint32_t FMCwdt2Flag = 0x100;
    uint32_t FMCwdt2FlagClear = 0xEA000000;

    if (read_register(FMCeventLogRegiter, &registerBufferFMC) < 0)
    {
        std::cerr<<"Failed to read register \n";
        return false;
    }
    if((registerBufferFMC & FMCwdt2Flag) > 0)
    {
#ifdef FEATURE_BMC_FMC_ABR
        switchBmc = checkWatchdogExpected();
#endif
        if(!switchBmc){
            logSELEvent("BMC WDT Reboot SEL Entry",
                        "/xyz/openbmc_project/sensors/mgtsubsyshealth/WATCHDOG_REBOOT",0x02,0xFF,0xFF);
        }

        write_register(FMCeventLogRegiter, FMCwdt2FlagClear);
        return true;
    }

    return false;
}

bool kernelPanicSetting()
{
    std::cerr << "Check BMC kernel panic status..." << std::endl;

    uint32_t registerBuffer = 0;
    uint32_t bSramConorlAddress = 0x1e6ef000;
    uint32_t bSramKey = 0xdba078e2;
    uint32_t bSramStoteAddress = 0x1e6ef110;
    // ASCII K P & [checksum]
    uint32_t panicMask = 0x4B50263F;

    if (read_register(bSramStoteAddress, &registerBuffer) < 0)
    {
        std::cerr<<"Failed to read SRAM register \n";
        return false;
    }

    if (registerBuffer == panicMask)
    {
        logSELEvent("BMC Kernel Panic SEL Entry",
            "/xyz/openbmc_project/sensors/mgtsubsyshealth/BMC_PANIC",0x01,0xFF,0xFF);
        write_register(bSramConorlAddress, bSramKey);
        write_register(bSramStoteAddress, 0x0);
        write_register(bSramConorlAddress, 0x0);
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
    uint32_t bootSourceFlag = 0x10;

    static const std::string bootSourceFilePath = "/run/boot-source";

    if (read_register(0x1E620064, &registerBuffer) < 0)
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

    try
    {
        auto bus = sdbusplus::bus::new_default();
        auto method = bus.new_method_call(SETTIMG_SERVICE, BMC_STATUS_PATH, PROPERTY_INTERFACE, "Set");
        method.append(BMC_STATUS_INTERFACE, BMC_PROP, std::variant<int32_t>(bootSource));
        bus.call_noreply(method);
    }
    catch (std::exception& e)
    {
        std::cerr << "Failed to set boot source dbus property." << std::endl;
        return false;
    }

    return true;
}

#ifdef FEATURE_BMC_FMC_ABR
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
#endif

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

void fanDefaultSpeed()
{
    uint32_t reg_data = 0x1E780088;
    uint32_t reg_dir = 0x1E78008C;
    uint32_t reg_buff = 0;

    system("i2cset -y 3 0x70 0x01 0x05");

    //set gpio0,3,7 to output high
    system("i2cset -y 3 0x22 0x01 0xFF");
    system("i2cset -y 3 0x22 0x05 0xFF");
    system("i2cset -y 3 0x22 0x03 0x76");
    system("i2cset -y 3 0x23 0x01 0xFF");
    system("i2cset -y 3 0x23 0x05 0xFF");
    system("i2cset -y 3 0x23 0x03 0x76");

    system("i2cset -y 3 0x70 0x01 0x00");

    //set gpio v7 to output high
    read_register(reg_data, &reg_buff);
    reg_buff |= 0x8000;
    write_register(reg_data, reg_buff);

    read_register(reg_dir, &reg_buff);
    reg_buff |= 0x8000;
    write_register(reg_dir, reg_buff);

}

bool bmcP2aSetting()
{
    std::cerr << "Check BMC P2A feature setting..." << std::endl;

    constexpr auto service = "xyz.openbmc_project.Settings";
    constexpr auto path = "/xyz/openbmc_project/oem/ServiceStatus";
    constexpr auto interface = "xyz.openbmc_project.OEM.ServiceStatus";
    constexpr auto p2aService = "P2aService";
    constexpr auto propertyInterface = "org.freedesktop.DBus.Properties";

    auto bus = sdbusplus::bus::new_default();

    auto method = bus.new_method_call(service, path, propertyInterface, "Get");
    method.append(interface, p2aService);

    std::variant<bool> result;
    try
    {
        auto reply = bus.call(method);
        reply.read(result);
    }
    catch (std::exception& e)
    {
        std::cerr << "Failed to get BMC P2A status: " << e.what() << "\n";
        return false;
    }

    uint32_t registerBuffer = 0;
    uint32_t p2aRegiter = 0x1e6e20c8;

    if (read_register(p2aRegiter, &registerBuffer) < 0)
    {
        return false;
    }

    if(std::get<bool>(result))
    {
        registerBuffer = registerBuffer & (~0x00000001);
        std::cerr << "Enable BMC P2A feature..." << std::endl;
    }
    else
    {
        registerBuffer = registerBuffer | 0x00000001;
        std::cerr << "Disable BMC P2A feature..." << std::endl;
    }

    write_register(p2aRegiter, registerBuffer);

    return true;
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
#ifdef FEATURE_BMC_FMC_ABR
    bmcSwitchSetting(watchdogTimout,switchBmc,bootSource);
#endif
    ipv6Setting();
    bmcP2aSetting();
    //fanDefaultSpeed();
    return 0;
}
