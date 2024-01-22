/*
// Copyright (c) 2017 Intel Corporation
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

#include <HwmonPowerSensor.hpp>
#include <Utils.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/bus/match.hpp>

#include <array>
#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>
#include <regex>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

static constexpr float pollRateDefault = 0.5;

static constexpr double maxValuePower = 3000;  // Watt
static constexpr double minValuePower = 0; // Watt

namespace fs = std::filesystem;
static auto sensorTypes{
    std::to_array<const char*>({"xyz.openbmc_project.Configuration.SBRMI",
                                "xyz.openbmc_project.Configuration.HwmonPower"})};

static struct SensorParams getSensorParameters()
{
    // offset is to default to 0 and scale to 1, see lore
    // https://lore.kernel.org/linux-iio/5c79425f-6e88-36b6-cdfe-4080738d039f@metafoo.de/
    struct SensorParams tmpSensorParameters = {.minValue = minValuePower,
                                               .maxValue = maxValuePower,
                                               .offsetValue = 0.0,
                                               .scaleValue = 1000000,
                                               .units =
                                                   sensor_paths::unitWatts,
                                               .typeName = "power"};

    return tmpSensorParameters;
}

struct SensorConfigKey
{
    uint64_t bus;
    uint64_t addr;
    bool operator<(const SensorConfigKey& other) const
    {
        if (bus != other.bus)
        {
            return bus < other.bus;
        }
        return addr < other.addr;
    }
};

struct SensorConfig
{
    std::string sensorPath;
    SensorData sensorData;
    std::string interface;
    SensorBaseConfigMap config;
    std::vector<std::string> name;
};

using SensorConfigMap =
    boost::container::flat_map<SensorConfigKey, SensorConfig>;

static SensorConfigMap
    buildSensorConfigMap(const ManagedObjectType& sensorConfigs)
{
    SensorConfigMap configMap;
    for (const std::pair<sdbusplus::message::object_path, SensorData>& sensor :
         sensorConfigs)
    {
        for (const std::pair<std::string, SensorBaseConfigMap>& cfgmap :
             sensor.second)
        {
            const SensorBaseConfigMap& cfg = cfgmap.second;
            auto busCfg = cfg.find("Bus");
            auto addrCfg = cfg.find("Address");
            if ((busCfg == cfg.end()) || (addrCfg == cfg.end()))
            {
                continue;
            }

            if ((!std::get_if<uint64_t>(&busCfg->second)) ||
                (!std::get_if<uint64_t>(&addrCfg->second)))
            {
                std::cerr << sensor.first.str << " Bus or Address invalid\n";
                continue;
            }

            std::vector<std::string> hwmonNames;
            auto nameCfg = cfg.find("Name");
            if (nameCfg != cfg.end())
            {
                hwmonNames.push_back(std::get<std::string>(nameCfg->second));
                size_t i = 1;
                while (true)
                {
                    auto sensorNameCfg = cfg.find("Name" + std::to_string(i));
                    if (sensorNameCfg == cfg.end())
                    {
                        break;
                    }
                    hwmonNames.push_back(
                        std::get<std::string>(sensorNameCfg->second));
                    i++;
                }
            }

            SensorConfigKey key = {std::get<uint64_t>(busCfg->second),
                                   std::get<uint64_t>(addrCfg->second)};
            SensorConfig val = {sensor.first.str, sensor.second, cfgmap.first,
                                cfg, hwmonNames};

            auto [it, inserted] = configMap.emplace(key, std::move(val));
            if (!inserted)
            {
                std::cerr << sensor.first.str
                          << ": ignoring duplicate entry for {" << key.bus
                          << ", 0x" << std::hex << key.addr << std::dec
                          << "}\n";
            }
        }
    }
    return configMap;
}

void createSensors(
    boost::asio::io_service& io, sdbusplus::asio::object_server& objectServer,
    boost::container::flat_map<std::string, std::shared_ptr<HwmonPowerSensor>>&
        sensors,
    std::shared_ptr<sdbusplus::asio::connection>& dbusConnection,
    const std::shared_ptr<boost::container::flat_set<std::string>>&
        sensorsChanged)
{
    auto getter = std::make_shared<GetSensorConfiguration>(
        dbusConnection,
        [&io, &objectServer, &sensors, &dbusConnection,
         sensorsChanged](const ManagedObjectType& sensorConfigurations) {
            bool firstScan = sensorsChanged == nullptr;

            SensorConfigMap configMap =
                buildSensorConfigMap(sensorConfigurations);

            // IIO _raw devices look like this on sysfs:
            //     /sys/bus/iio/devices/iio:device0/in_power_raw
            //     /sys/bus/iio/devices/iio:device0/in_power_offset
            //     /sys/bus/iio/devices/iio:device0/in_power_scale
            //
            // Other IIO devices look like this on sysfs:
            //     /sys/bus/iio/devices/iio:device1/in_power_input
            std::vector<fs::path> paths;
            fs::path root("/sys/bus/iio/devices");
            findFiles(root, R"(in_power\d*_(input|raw))", paths);
            findFiles(fs::path("/sys/class/hwmon"), R"(power\d+_input)", paths);

            if (paths.empty())
            {
                return;
            }

            // iterate through all found power sensors,
            // and try to match them with configuration
            for (auto& path : paths)
            {
                std::smatch match;
                const std::string pathStr = path.string();
                auto directory = path.parent_path();
                fs::path device;

                std::string deviceName;
                if (pathStr.starts_with("/sys/bus/iio/devices"))
                {
                    device = fs::canonical(directory);
                    deviceName = device.parent_path().stem();
                }
                else
                {
                    device = directory / "device";
                    deviceName = fs::canonical(device).stem();
                }
                auto findHyphen = deviceName.find('-');
                if (findHyphen == std::string::npos)
                {
                    std::cerr << "found bad device " << deviceName << "\n";
                    continue;
                }
                std::string busStr = deviceName.substr(0, findHyphen);
                std::string addrStr = deviceName.substr(findHyphen + 1);

                size_t bus = 0;
                size_t addr = 0;
                try
                {
                    bus = std::stoi(busStr);
                    addr = std::stoi(addrStr, nullptr, 16);
                }
                catch (const std::invalid_argument&)
                {
                    continue;
                }

                auto thisSensorParameters = getSensorParameters();
                auto findSensorCfg = configMap.find({bus, addr});
                if (findSensorCfg == configMap.end())
                {
                    continue;
                }

                const std::string& interfacePath =
                    findSensorCfg->second.sensorPath;
                const SensorData& sensorData = findSensorCfg->second.sensorData;
                const std::string& sensorType = findSensorCfg->second.interface;
                const SensorBaseConfigMap& baseConfigMap =
                    findSensorCfg->second.config;
                std::vector<std::string>& hwmonName =
                    findSensorCfg->second.name;

                // Power has "Name"
                auto findSensorName = baseConfigMap.find("Name");

                if (findSensorName == baseConfigMap.end())
                {
                    std::cerr << "could not determine configuration name for "
                              << deviceName << "\n";
                    continue;
                }
                std::string sensorName =
                    std::get<std::string>(findSensorName->second);
                // on rescans, only update sensors we were signaled by
                auto findSensor = sensors.find(sensorName);
                if (!firstScan && findSensor != sensors.end())
                {
                    bool found = false;
                    auto it = sensorsChanged->begin();
                    while (it != sensorsChanged->end())
                    {
                        if (boost::ends_with(*it, findSensor->second->name))
                        {
                            it = sensorsChanged->erase(it);
                            findSensor->second = nullptr;
                            found = true;
                            break;
                        }
                        ++it;
                    }
                    if (!found)
                    {
                        continue;
                    }
                }

                std::vector<thresholds::Threshold> sensorThresholds;
                int index = 1;

                if (!parseThresholdsFromConfig(sensorData, sensorThresholds,
                                               &sensorName, &index))
                {
                    std::cerr << "error populating thresholds for "
                              << sensorName << " index 1\n";
                }

                auto findPollRate = baseConfigMap.find("PollRate");
                float pollRate = pollRateDefault;
                if (findPollRate != baseConfigMap.end())
                {
                    pollRate = std::visit(VariantToFloatVisitor(),
                                          findPollRate->second);
                    if (pollRate <= 0.0f)
                    {
                        pollRate = pollRateDefault; // polling time too short
                    }
                }

                auto findPowerOn = baseConfigMap.find("PowerState");
                PowerState readState = PowerState::always;
                if (findPowerOn != baseConfigMap.end())
                {
                    std::string powerState = std::visit(
                        VariantToStringVisitor(), findPowerOn->second);
                    setReadState(powerState, readState);
                }

                auto findScaleFactor = baseConfigMap.find("ScaleFactor");
                if (findScaleFactor != baseConfigMap.end())
                {
                    thisSensorParameters.scaleValue = std::visit(VariantToFloatVisitor(),
                                             findScaleFactor->second);
                }

                auto findMaxValue = baseConfigMap.find("MaxValue");
                if (findMaxValue != baseConfigMap.end())
                {
                    thisSensorParameters.maxValue = std::visit(VariantToFloatVisitor(),
                                             findMaxValue->second);
                }

                auto findMinValue = baseConfigMap.find("MinValue");
                if (findMinValue != baseConfigMap.end())
                {
                    thisSensorParameters.minValue = std::visit(VariantToFloatVisitor(),
                                             findMinValue->second);
                }

                auto permitSet = getPermitSet(baseConfigMap);
                auto& sensor = sensors[sensorName];
                sensor = nullptr;
                auto hwmonFile = getFullHwmonFilePath(directory.string(),
                                                      "power1", permitSet);
                if (pathStr.starts_with("/sys/bus/iio/devices"))
                {
                    hwmonFile = pathStr;
                }
                if (hwmonFile)
                {
                    if (sensorName != "IGNORE")
                    {
                        sensor = std::make_shared<HwmonPowerSensor>(
                            *hwmonFile, sensorType, objectServer, dbusConnection,
                            io, sensorName, std::move(sensorThresholds),
                            thisSensorParameters, pollRate, interfacePath,
                            readState);
                        sensor->setupRead();
                        hwmonName.erase(
                            remove(hwmonName.begin(), hwmonName.end(), sensorName),
                            hwmonName.end());
                    }
                }
                // Looking for keys like "Name1" for power2_input,
                // "Name2" for power3_input, etc.
                int i = 0;
                while (true)
                {
                    ++i;
                    auto findKey =
                        baseConfigMap.find("Name" + std::to_string(i));
                    if (findKey == baseConfigMap.end())
                    {
                        break;
                    }
                    std::string sensorName =
                        std::get<std::string>(findKey->second);
                    
                    std::vector<thresholds::Threshold> sensorThresholds;
                    if (!parseThresholdsFromConfig(sensorData, sensorThresholds, &sensorName))
                    {
                        std::cerr << "error populating thresholds for "
                                << sensorName << "\n";
                    }

                    hwmonFile = getFullHwmonFilePath(
                        directory.string(), "power" + std::to_string(i + 1),
                        permitSet);
                    if (pathStr.starts_with("/sys/bus/iio/devices"))
                    {
                        continue;
                    }
                    if (hwmonFile)
                    {
                        // To look up thresholds for these additional sensors,
                        // match on the Index property in the threshold data
                        // where the index comes from the sysfs file we're on,
                        // i.e. index = 2 for power2_input.
                        int index = i + 1;
                        std::vector<thresholds::Threshold> thresholds;

                        if (!parseThresholdsFromConfig(sensorData, thresholds,
                                                       nullptr, &index))
                        {
                            std::cerr << "error populating thresholds for "
                                      << sensorName << " index " << index
                                      << "\n";
                        }

                        auto& sensor = sensors[sensorName];
                        sensor = nullptr;
                        if (sensorName != "IGNORE")
                        {
                            sensor = std::make_shared<HwmonPowerSensor>(
                                *hwmonFile, sensorType, objectServer,
                                dbusConnection, io, sensorName,
                                std::move(thresholds), thisSensorParameters,
                                pollRate, interfacePath, readState);
                            sensor->setupRead();
                            hwmonName.erase(remove(hwmonName.begin(),
                                                hwmonName.end(), sensorName),
                                            hwmonName.end());
                        }
                    }
                }
                if (hwmonName.empty())
                {
                    configMap.erase(findSensorCfg);
                }
            }
        });
    getter->getConfiguration(
        std::vector<std::string>(sensorTypes.begin(), sensorTypes.end()));
}

void interfaceRemoved(
    sdbusplus::message::message& message,
    boost::container::flat_map<std::string, std::shared_ptr<HwmonPowerSensor>>&
        sensors)
{
    if (message.is_method_error())
    {
        std::cerr << "interfacesRemoved callback method error\n";
        return;
    }

    sdbusplus::message::object_path path;
    std::vector<std::string> interfaces;

    message.read(path, interfaces);

    // If the xyz.openbmc_project.Confguration.X interface was removed
    // for one or more sensors, delete those sensor objects.
    auto sensorIt = sensors.begin();
    while (sensorIt != sensors.end())
    {
        if ((sensorIt->second->configurationPath == path) &&
            (std::find(interfaces.begin(), interfaces.end(),
                       sensorIt->second->objectType) != interfaces.end()))
        {
            sensorIt = sensors.erase(sensorIt);
        }
        else
        {
            sensorIt++;
        }
    }
}

int main()
{
    boost::asio::io_service io;
    auto systemBus = std::make_shared<sdbusplus::asio::connection>(io);
    systemBus->request_name("xyz.openbmc_project.HwmonPowerSensor");
    sdbusplus::asio::object_server objectServer(systemBus);
    boost::container::flat_map<std::string, std::shared_ptr<HwmonPowerSensor>>
        sensors;
    std::vector<std::unique_ptr<sdbusplus::bus::match::match>> matches;
    auto sensorsChanged =
        std::make_shared<boost::container::flat_set<std::string>>();

    io.post([&]() {
        createSensors(io, objectServer, sensors, systemBus, nullptr);
    });

    boost::asio::steady_timer filterTimer(io);
    std::function<void(sdbusplus::message::message&)> eventHandler =
        [&](sdbusplus::message::message& message) {
            if (message.is_method_error())
            {
                std::cerr << "callback method error\n";
                return;
            }
            sensorsChanged->insert(message.get_path());
            // this implicitly cancels the timer
            filterTimer.expires_from_now(boost::asio::chrono::seconds(1));

            filterTimer.async_wait([&](const boost::system::error_code& ec) {
                if (ec == boost::asio::error::operation_aborted)
                {
                    /* we were canceled*/
                    return;
                }
                if (ec)
                {
                    std::cerr << "timer error\n";
                    return;
                }
                createSensors(io, objectServer, sensors, systemBus,
                              sensorsChanged);
            });
        };

    for (const char* type : sensorTypes)
    {
        auto match = std::make_unique<sdbusplus::bus::match::match>(
            static_cast<sdbusplus::bus::bus&>(*systemBus),
            "type='signal',member='PropertiesChanged',path_namespace='" +
                std::string(inventoryPath) + "',arg0namespace='" + type + "'",
            eventHandler);
        matches.emplace_back(std::move(match));
    }

    setupManufacturingModeMatch(*systemBus);

    // Watch for entity-manager to remove configuration interfaces
    // so the corresponding sensors can be removed.
    auto ifaceRemovedMatch = std::make_unique<sdbusplus::bus::match::match>(
        static_cast<sdbusplus::bus::bus&>(*systemBus),
        "type='signal',member='InterfacesRemoved',arg0path='" +
            std::string(inventoryPath) + "/'",
        [&sensors](sdbusplus::message::message& msg) {
            interfaceRemoved(msg, sensors);
        });

    matches.emplace_back(std::move(ifaceRemovedMatch));

    io.run();
}
