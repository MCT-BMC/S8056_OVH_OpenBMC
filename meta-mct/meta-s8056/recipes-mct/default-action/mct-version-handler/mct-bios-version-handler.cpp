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

#include <iostream>
#include <string>
#include <nlohmann/json.hpp>
#include <sdbusplus/asio/object_server.hpp>

static constexpr bool DEBUG = false;

static constexpr const char* PROPERTY_INTERFACE = "org.freedesktop.DBus.Properties";

namespace bios
{
    static constexpr const char* busName = "xyz.openbmc_project.Software.BMC.Updater";
    static constexpr const char* path = "/xyz/openbmc_project/software/bios_active";
    static constexpr const char* interface = "xyz.openbmc_project.Software.Version";
    static constexpr const char* prop = "Version";
} // namespace bios

namespace systemInfo
{
    static constexpr const char* busName = "xyz.openbmc_project.Settings";
    static constexpr const char* path = "/xyz/openbmc_project/oem/SystmeInfo";
    static constexpr const char* interface = "xyz.openbmc_project.OEM.SystmeInfo";
    static constexpr const char* prop = "SystemInformation";
} // namespace bios


enum SystemInfoParameter
{
    SYSINFO_SET_STATE = 0x00,
    SYSINFO_SYSTEM_FW_VERSION = 0x01,
    SYSINFO_HOSTNAME = 0x02,
    SYSINFO_PRIMARY_OS_NAME = 0x03,
    SYSINFO_OS_NAME = 0x04,
};

std::string getSystemInfo()
{

    auto bus = sdbusplus::bus::new_default();

    //Get web service status
    auto method = bus.new_method_call(systemInfo::busName, systemInfo::path, PROPERTY_INTERFACE, "Get");
    method.append(systemInfo::interface, systemInfo::prop);

    std::variant<std::string> result;
    try
    {
        auto reply = bus.call(method);
        reply.read(result);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        std::cerr << "Error in system info get. ERROR= : " << e.what() << std::endl;
    }

    return std::get<std::string>(result);
}

int updateBiosVersion(std::string version)
{
    auto bus = sdbusplus::bus::new_default();

    auto method = bus.new_method_call(bios::busName, bios::path, PROPERTY_INTERFACE, "Set");

    try
    {
        method.append(bios::interface, bios::prop, std::variant<std::string>(version));
        bus.call_noreply(method);
    }
    catch (std::exception& e)
    {
        std::cerr << "Failed to set property:" << e.what() << "\n";
        sleep(5);
        updateBiosVersion(version);
    }

    return 0;
}

int main(void)
{
    nlohmann::json sysInfo;

    try
    {
        std::string systmeInfo = getSystemInfo();

        if(systmeInfo.length() == 0)
        {
            return 0;
        }

        sysInfo = nlohmann::json::parse(systmeInfo, nullptr, false);

        for (auto it = sysInfo.begin(); it != sysInfo.end(); ++it)
        {
            if(it.key() == std::to_string(SYSINFO_SYSTEM_FW_VERSION))
            {
                if(DEBUG){
                    std::cerr << "BIOS Version : " << it.value() << std::endl;
                }

                updateBiosVersion(it.value());
                break;
            }
        }
    } catch (std::exception &e) {
        std::cout << "Read bios version failed. exception: " << e.what() << std::endl;
    }

    return 0;
}