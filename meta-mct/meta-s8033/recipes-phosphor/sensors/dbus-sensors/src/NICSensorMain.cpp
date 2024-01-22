#include <Utils.hpp>
#include <NICSensor.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/container/flat_set.hpp>
#include <filesystem>
#include <fstream>
#include <regex>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>

static constexpr bool DEBUG = false;

namespace fs = std::filesystem;
static constexpr std::array<const char*, 1> sensorTypes =
{
    "xyz.openbmc_project.Configuration.NIC"
};

static boost::container::flat_map<std::string, std::string> sensorTable;
static boost::container::flat_map<std::string, ReadMode> modeTable;

void createSensors(
    boost::asio::io_service& io, sdbusplus::asio::object_server& objectServer,
    boost::container::flat_map<std::string, std::unique_ptr<NICSensor>>& sensors,
    std::shared_ptr<sdbusplus::asio::connection>& dbusConnection,
    const std::unique_ptr<boost::container::flat_set<std::string>>&
    sensorsChanged)
{
    bool firstScan = sensorsChanged == nullptr;
    // use new data the first time, then refresh
    ManagedObjectType sensorConfigurations;
    bool useCache = false;
    for (const char* type : sensorTypes)
    {
        if (!getSensorConfiguration(type, dbusConnection, sensorConfigurations,
                                    useCache))
        {
            std::cerr << "error communicating to entity manager\n";
            return;
        }
        useCache = true;
    }

    for (const std::pair<sdbusplus::message::object_path, SensorData>& sensor :
            sensorConfigurations)
    {
        const SensorData* sensorData = nullptr;
        const std::string* interfacePath = nullptr;
        const char* sensorType = nullptr;
        const std::pair<std::string, boost::container::flat_map<
        std::string, BasicVariantType>>*
                                     baseConfiguration = nullptr;

        sensorData = &(sensor.second);
        for (const char* type : sensorTypes)
        {
            auto sensorBase = sensorData->find(type);
            if (sensorBase != sensorData->end())
            {
                baseConfiguration = &(*sensorBase);
                sensorType = type;
                break;
            }
        }
        if (baseConfiguration == nullptr)
        {
            std::cerr << "error finding base configuration for NIC \n";
            continue;
        }

        auto configurationBus = baseConfiguration->second.find("Bus");
        auto configurationAddress = baseConfiguration->second.find("Address");
        auto configurationCmd = baseConfiguration->second.find("Command");

        if (configurationBus == baseConfiguration->second.end() ||
                configurationAddress == baseConfiguration->second.end() ||
                configurationCmd == baseConfiguration->second.end())
        {
            std::cerr << "fail to find the bus, addr, cmd, or page in JSON\n";
            continue;
        }

        uint8_t busId =
            static_cast<uint8_t>(std::get<uint64_t>(configurationBus->second));
        std::string i2cBus = "/sys/class/i2c-dev/i2c-" + std::to_string(busId)
                             + "/device";
        uint8_t slaveAddr = static_cast<uint8_t>(
                                std::get<uint64_t>(configurationAddress->second));
        uint8_t cmdCode =
            static_cast<uint8_t>(std::get<uint64_t>(configurationCmd->second));

        if (0x80 <= slaveAddr)
        {
            std::cerr
                    << "error i2c slave addr is out of the range (7-bit addr)\n";
            continue;
        }

        interfacePath = &(sensor.first.str);
        if (interfacePath == nullptr)
        {
            std::cerr << " invalid sensor interface path\n";
            continue;
        }

        auto findSensorName = baseConfiguration->second.find("Name");
        if (findSensorName == baseConfiguration->second.end())
        {
            std::cerr << "fail to find sensor name in JSON\n";
            continue;
        }
        std::string sensorName = std::get<std::string>(findSensorName->second);

        // Sensor Type: power, curr, temp, volt
        auto findSensorTypeJson = baseConfiguration->second.find("SensorType");
        if (findSensorTypeJson == baseConfiguration->second.end())
        {
            std::cerr << "fail to find sensor type in JSON\n";
            continue;
        }
        std::string SensorTypeJson =
            std::get<std::string>(findSensorTypeJson->second);

        auto findSensorType = sensorTable.find(SensorTypeJson);
        if (findSensorType == sensorTable.end())
        {
            std::cerr << "fail to find match for NIC sensorType: "
                      << SensorTypeJson << "\n";
            continue;
        }

        /* Convert Method: Byte, Word*/
        auto findConvertModeJson = baseConfiguration->second.find("Mode");
        if (findConvertModeJson == baseConfiguration->second.end())
        {
            std::cerr << "fail to find NIC ConvertMode in JSON\n";
            continue;
        }
        std::string ConvertModeJson =
            std::get<std::string>(findConvertModeJson->second);

        auto findConvertMode = modeTable.find(ConvertModeJson);
        if (findConvertMode == modeTable.end())
        {
            std::cerr << "fail to find match for NIC ConvertMode: "
                      << ConvertModeJson << "\n";
            continue;
        }
        uint32_t readMode = static_cast<int>(findConvertMode->second);

        /* Initialize scale and offset value */
        double scaleVal = 1;
        double offsetVal = 0;

        auto findScaleVal = baseConfiguration->second.find("ScaleValue");
        if (findScaleVal != baseConfiguration->second.end())
        {
            scaleVal = std::visit(VariantToDoubleVisitor(),
                                  findScaleVal->second);
        }

        auto findOffsetVal = baseConfiguration->second.find("OffsetValue");
        if (findOffsetVal != baseConfiguration->second.end())
        {
            offsetVal = std::visit(VariantToDoubleVisitor(),
                                   findOffsetVal->second);
        }

        int tctlMax;
        auto findTctlMax =
            baseConfiguration->second.find("TctlMax");
        if (findTctlMax != baseConfiguration->second.end())
        {

            tctlMax = std::visit(VariantToIntVisitor(),
                                  findTctlMax->second);
        }

        // on rescans, only update sensors we were signaled by
        auto findSensor = sensors.find(sensorName);

        std::vector<thresholds::Threshold> storesensorThresholds;
        double initvalue = std::numeric_limits<double>::quiet_NaN();
        bool found = false;

        if (!firstScan && findSensor != sensors.end())
        {
            storesensorThresholds = findSensor->second->thresholds;
            initvalue = findSensor->second->value;
            for (auto it = sensorsChanged->begin(); it != sensorsChanged->end();
                    it++)
            {
                if (boost::ends_with(*it, findSensor->second->name))
                {
                    sensorsChanged->erase(it);
                    findSensor->second = nullptr;
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                continue;
            }
        }

        std::vector<thresholds::Threshold> sensorThresholds;
        if (!parseThresholdsFromConfig(*sensorData, sensorThresholds))
        {
            std::cerr << "error populating thresholds for " << sensorName
                      << "\n";
        }

        if(found)
        {
            sensorThresholds = storesensorThresholds;
        }

        constexpr double defaultMaxReading = 255;
        constexpr double defaultMinReading = 0;
        auto limits = std::make_pair(defaultMinReading, defaultMaxReading);

        findLimits(limits, baseConfiguration);

        sensors[sensorName] = std::make_unique<NICSensor>(
                    i2cBus, sensorType, objectServer, dbusConnection,
                    io, sensorName, std::move(sensorThresholds),
                    *interfacePath, findSensorType->second, defaultMaxReading,
                    defaultMinReading, busId, slaveAddr, cmdCode,
                    readMode, scaleVal, offsetVal, tctlMax);

        sensors[sensorName]->value = initvalue;
    }
}

void propertyInitialize(void)
{
    sensorTable = {{"power", "power/"},
        {"curr", "current/"},
        {"temp", "temperature/"},
        {"volt", "voltage/"}
    };
    modeTable = {{"Byte", ReadMode::READ_BYTE}};
}

int main()
{
    boost::asio::io_service io;
    auto systemBus = std::make_shared<sdbusplus::asio::connection>(io);
    systemBus->request_name("xyz.openbmc_project.NICSensor");
    sdbusplus::asio::object_server objectServer(systemBus);
    boost::container::flat_map<std::string, std::unique_ptr<NICSensor>> sensors;
    std::vector<std::unique_ptr<sdbusplus::bus::match::match>> matches;
    std::unique_ptr<boost::container::flat_set<std::string>> sensorsChanged =
                std::make_unique<boost::container::flat_set<std::string>>();

    propertyInitialize();

    io.post([&]() {
        createSensors(io, objectServer, sensors, systemBus, nullptr);
    });

    boost::asio::steady_timer filterTimer(io);
    std::function<void(sdbusplus::message::message&)> eventHandler =
    [&](sdbusplus::message::message & message) {
        if (message.is_method_error())
        {
            std::cerr << "callback method error\n";
            return;
        }
        sensorsChanged->insert(message.get_path());
        // this implicitly cancels the timer
        filterTimer.expires_from_now(boost::asio::chrono::seconds(1));

        filterTimer.async_wait([&](const boost::system::error_code & ec) {
            if (ec == boost::asio::error::operation_aborted)
            {
                /* we were canceled*/
                return;
            }
            else if (ec)
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

    io.run();
}
