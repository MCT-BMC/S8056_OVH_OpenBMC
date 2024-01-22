#include "storagecommands.hpp"

#include "Utils.hpp"

#include <boost/container/flat_map.hpp>
#include <boost/process.hpp>
#include <ipmid/api.hpp>
#include <ipmid/message.hpp>
#include <phosphor-ipmi-host/selutility.hpp>
#include <sdbusplus/message/types.hpp>
#include <sdbusplus/timer.hpp>

#include <filesystem>
#include <openbmc/sensor-gen-extra.cpp>

namespace ipmi
{

namespace storage
{

using DbusVariant = std::variant<std::string, bool, uint8_t, uint16_t, int16_t,
                                 uint32_t, int32_t, uint64_t, int64_t, double>;
using ObjectType = boost::container::flat_map<
    std::string, boost::container::flat_map<std::string, DbusVariant>>;
using ManagedObjectType =
    boost::container::flat_map<sdbusplus::message::object_path, ObjectType>;

constexpr static const char* fruDeviceServiceName =
    "xyz.openbmc_project.FruDevice";
constexpr static const size_t writeTimeoutSeconds = 10;

static std::vector<uint8_t> writeFruCache;
static int lastWriteAddr = 0;
static uint8_t writeBus = 0xFF;
static uint8_t writeAddr = 0XFF;
static bool isWriting = false;
static bool dbusInterfacesDone = true;
std::unique_ptr<phosphor::Timer> writeTimer = nullptr;

static std::vector<sdbusplus::bus::match::match> fruMatches;

ManagedObjectType frus;

constexpr static const size_t clearFlagTimeoutSeconds = 2;
static bool hadCorrectableEcc = false;
std::unique_ptr<phosphor::Timer> cancelOemMcaSelTimer = nullptr;

boost::container::flat_map<uint8_t, struct deviceInfo> deviceMap;

/*
 * While FruDevice is rescaning after writing fru,
 * fru read responses last cached data.
 */
boost::container::flat_map<uint8_t, struct deviceInfo> deviceMapCache;

void registerStorageFunctions() __attribute__((constructor));

bool writeFru()
{
    if (writeBus == 0xFF && writeAddr == 0xFF)
    {
        return true;
    }
    std::shared_ptr<sdbusplus::asio::connection> dbus = getSdBus();
    sdbusplus::message::message writeFru = dbus->new_method_call(
        fruDeviceServiceName, "/xyz/openbmc_project/FruDevice",
        "xyz.openbmc_project.FruDeviceManager", "WriteFru");
    writeFru.append(writeBus, writeAddr, writeFruCache);
    try
    {
        isWriting = false;
        sdbusplus::message::message writeFruResp = dbus->call(writeFru);
        dbusInterfacesDone = false;
    }
    catch (sdbusplus::exception_t&)
    {
        std::cerr << "Error writing fru\n";
        return false;
    }
    writeBus = 0xFF;
    writeAddr = 0xFF;
    return true;
}

bool clearCorrEccFlag()
{
    if (hadCorrectableEcc)
    {
        hadCorrectableEcc = false;
    }
    return true;
}

void createTimers()
{
    writeTimer = std::make_unique<phosphor::Timer>(writeFru);
    cancelOemMcaSelTimer = std::make_unique<phosphor::Timer>(clearCorrEccFlag);
}

void createDeviceMap()
{
    deviceMap.clear();
    for (const auto& fru : frus)
    {
        auto fruIface = fru.second.find("xyz.openbmc_project.FruDevice");
        if (fruIface == fru.second.end())
        {
            continue;
        }

        auto indexFind = fruIface->second.find("INDEX");
        auto sizeFind = fruIface->second.find("SIZE");
        auto busFind = fruIface->second.find("BUS");
        auto addrFind = fruIface->second.find("ADDRESS");
        if (busFind == fruIface->second.end() ||
            sizeFind == fruIface->second.end() ||
            addrFind == fruIface->second.end() ||
            indexFind == fruIface->second.end())
        {
            std::cerr << "Fru device missing Bus or Size or Address or Index. "
                      << "Fru = " << fru.first.str.c_str() << "\n";
            continue;
        }

        uint8_t fruIndex = std::get<uint32_t>(indexFind->second);
        size_t fruSize = std::get<size_t>(sizeFind->second);
        uint8_t fruBus = std::get<uint32_t>(busFind->second);
        uint8_t fruAddr = std::get<uint32_t>(addrFind->second);

        bool readOnly = false;
        auto readOnlyFind = fruIface->second.find("READONLY");
        if (readOnlyFind != fruIface->second.end())
        {
            readOnly = std::get<bool>(readOnlyFind->second);
        }

        struct deviceInfo newDev = {fruSize, fruBus, fruAddr, readOnly};

        auto deviceFind = deviceMap.find(fruIndex);
        if (deviceFind == deviceMap.end())
        {
            deviceMap.emplace(fruIndex, newDev);
        }
        else
        {
            std::cerr << "Fru devices have the same index. Index: "
                      << static_cast<uint32_t>(fruIndex)
                      << ", Bus: " << static_cast<uint32_t>(fruBus)
                      << ", Address: " << static_cast<uint32_t>(fruAddr)
                      << "\n";
        }
    }
}

void replaceCacheFru(const std::shared_ptr<sdbusplus::asio::connection>& bus,
                     boost::asio::yield_context& yield,
                     const std::optional<std::string>& path = std::nullopt)
{
    boost::system::error_code ec;

    frus = bus->yield_method_call<ManagedObjectType>(
        yield, ec, fruDeviceServiceName, "/",
        "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
    if (ec)
    {
        std::cerr << "GetMangagedObjects for fru device failed."
                  << "ERROR = " << ec.message() << "\n";
        return;
    }
    createDeviceMap();
}

int retryCacheFru(const std::shared_ptr<sdbusplus::asio::connection>& bus)
{
    try
    {
        auto message =
            bus->new_method_call(fruDeviceServiceName, "/",
                                 "org.freedesktop.DBus.ObjectManager",
                                 "GetManagedObjects");
        auto reply = bus->call(message);
        reply.read(frus);
        createDeviceMap();
    }
    catch (const sdbusplus::exception_t& e)
    {
        std::cerr << "GetMangagedObjects for fru device retry failed."
                  << "ERROR = " << e.what() << "\n";
        return -1;
    }

    return 0;
}

ipmi::Cc getFru(ipmi::Context::ptr ctx, struct deviceInfo& device)
{
    if (device.data.empty())
    {
        auto getFru = ctx->bus->new_method_call(
            fruDeviceServiceName, "/xyz/openbmc_project/FruDevice",
            "xyz.openbmc_project.FruDeviceManager", "GetRawFru");
        getFru.append(device.bus, device.address);
        try
        {
            auto reply = ctx->bus->call(getFru);
            reply.read(device.data);
            if (device.data.empty())
            {
                std::cerr << "Fru data is empty\n";
                return ipmi::ccUnspecifiedError;
            }
        }
        catch (sdbusplus::exception_t& e)
        {
            std::cerr << "Couldn't get raw fru. ERROR = " << e.what() << "\n";
            return ipmi::ccResponseError;
        }
    }

    return ipmi::ccSuccess;
}

void startMatch(void)
{
    if (fruMatches.size())
    {
        return;
    }

    fruMatches.reserve(2);

    auto bus = getSdBus();
    fruMatches.emplace_back(*bus,
                            "type='signal',arg0path='/xyz/openbmc_project/"
                            "FruDevice/',member='InterfacesAdded'",
                            [](sdbusplus::message::message& message) {
                                sdbusplus::message::object_path path;
                                ObjectType object;
                                try
                                {
                                    message.read(path, object);
                                }
                                catch (sdbusplus::exception_t&)
                                {
                                    return;
                                }
                                auto findType = object.find(
                                    "xyz.openbmc_project.FruDevice");
                                if (findType == object.end())
                                {
                                    return;
                                }
                                dbusInterfacesDone = true;
                                deviceMapCache.clear();
                                frus[path] = object;
                                createDeviceMap();
                            });

    fruMatches.emplace_back(
        *bus,
        "type='signal',arg0path='/xyz/openbmc_project/"
        "FruDevice/',member='InterfacesRemoved'",
        [](sdbusplus::message::message& message) {
            sdbusplus::message::object_path path;
            std::set<std::string> interfaces;
            try
            {
                message.read(path, interfaces);
            }
            catch (sdbusplus::exception_t&)
            {
                return;
            }
            auto findType = interfaces.find("xyz.openbmc_project.FruDevice");
            if (findType == interfaces.end())
            {
                return;
            }

            // Copy the current fru data at first time.
            if (dbusInterfacesDone == false && deviceMapCache.empty())
            {
                deviceMapCache = deviceMap;
            }
            frus.erase(path);
            createDeviceMap();
        });

    // call once to populate
    boost::asio::spawn(*getIoContext(), [](boost::asio::yield_context yield) {
        replaceCacheFru(getSdBus(), yield);
    });
}

/** @brief implements the read FRU data command
 *  @param fruDeviceId        - FRU Device ID
 *  @param fruInventoryOffset - FRU Inventory Offset to write
 *  @param countToRead        - Count to read
 *
 *  @returns ipmi completion code plus response data
 *   - countWritten  - Count written
 */
ipmi::RspType<uint8_t,             // Count
              std::vector<uint8_t> // Requested data
              >
    ipmiStorageReadFruData(ipmi::Context::ptr ctx, uint8_t fruDeviceId,
                           uint16_t fruInventoryOffset, uint8_t countToRead)
{
    if (frus.empty() && retryCacheFru(getSdBus()))
    {
        return ipmi::responseUnspecifiedError();
    }

    if (fruDeviceId == 0xFF)
    {
        return ipmi::responseInvalidFieldRequest();
    }

    boost::container::flat_map<uint8_t, struct deviceInfo>* devices;
    if (dbusInterfacesDone == false && deviceMapCache.empty() == false)
    {
        devices = &deviceMapCache;
    }
    else
    {
        devices = &deviceMap;
    }

    auto deviceFind = devices->find(fruDeviceId);
    if (deviceFind == devices->end())
    {
        return ipmi::responseSensorInvalid();
    }
    auto& device = deviceFind->second;

    if (device.data.empty())
    {
        ipmi::Cc status = getFru(ctx, device);
        if (status != ipmi::ccSuccess)
        {
            return ipmi::response(status);
        }
    }

    size_t fromFruByteLen = 0;
    if (countToRead + fruInventoryOffset < device.data.size())
    {
        fromFruByteLen = countToRead;
    }
    else if (device.data.size() > fruInventoryOffset)
    {
        fromFruByteLen = device.data.size() - fruInventoryOffset;
    }
    else
    {
        return ipmi::responseReqDataLenExceeded();
    }

    std::vector<uint8_t> requestedData;

    requestedData.insert(
        requestedData.begin(), device.data.begin() + fruInventoryOffset,
        device.data.begin() + fruInventoryOffset + fromFruByteLen);

    return ipmi::responseSuccess(static_cast<uint8_t>(requestedData.size()),
                                 requestedData);
}

/** @brief implements the write FRU data command
 *  @param fruDeviceId        - FRU Device ID
 *  @param fruInventoryOffset - FRU Inventory Offset to write
 *  @param dataToWrite        - Data to write
 *
 *  @returns ipmi completion code plus response data
 *   - countWritten  - Count written
 */
ipmi::RspType<uint8_t>
    ipmiStorageWriteFruData(ipmi::Context::ptr ctx, uint8_t fruDeviceId,
                            uint16_t fruInventoryOffset,
                            std::vector<uint8_t>& dataToWrite)
{
    if (frus.empty() && retryCacheFru(getSdBus()))
    {
        return ipmi::responseUnspecifiedError();
    }

    if (fruDeviceId == 0xFF)
    {
        return ipmi::responseInvalidFieldRequest();
    }

    if (dbusInterfacesDone == false)
    {
        std::cerr << "FruDevice is registering interfaces\n";
        return ipmi::responseBusy();
    }

    auto deviceFind = deviceMap.find(fruDeviceId);
    if (deviceFind == deviceMap.end())
    {
        return ipmi::responseSensorInvalid();
    }
    auto& device = deviceFind->second;

    if (device.readOnly == true)
    {
        std::cerr << "Fru " << static_cast<int>(fruDeviceId)
                  << " is read only device\n";
        return ipmi::responseInvalidFieldRequest();
    }

    size_t writeLen = dataToWrite.size();
    size_t writeAddrEnd = fruInventoryOffset + writeLen;
    if (writeAddrEnd > device.size)
    {
        std::cerr << "Write size is too large: Fru "
                  << static_cast<int>(fruDeviceId) << " size is " << device.size
                  << "\n";
        return ipmi::responseInvalidFieldRequest();
    }

    if (isWriting == true)
    {
        if (writeBus != device.bus || lastWriteAddr != fruInventoryOffset)
        {
            std::cerr << "Fru is writting now\n";
            return ipmi::responseBusy();
        }
    }
    else
    {
        ipmi::Cc status = getFru(ctx, device);
        if (status != ipmi::ccSuccess)
        {
            return ipmi::response(status);
        }
        writeFruCache = device.data;
        writeBus = device.bus;
        writeAddr = device.address;
        isWriting = true;
    }

    lastWriteAddr = writeAddrEnd;
    std::copy(dataToWrite.begin(), dataToWrite.begin() + writeLen,
              writeFruCache.begin() + fruInventoryOffset);

    uint8_t countWritten = 0;
    if (lastWriteAddr >= deviceFind->second.size)
    {
        // Cancel timer, The end of write fru, send it out.
        writeTimer->stop();
        if (!writeFru())
        {
            return ipmi::responseInvalidFieldRequest();
        }
        countWritten =
            std::min(writeFruCache.size(), static_cast<size_t>(0xFF));
    }
    else
    {
        // Start a timer, if no further data is sent, send it out.
        writeTimer->start(std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::seconds(writeTimeoutSeconds)));
    }

    return ipmi::responseSuccess(countWritten);
}

/** @brief implements the get FRU inventory area info command
 *  @param fruDeviceId  - FRU Device ID
 *
 *  @returns IPMI completion code plus response data
 *   - inventorySize - Number of possible allocation units
 *   - accessType    - Allocation unit size in bytes.
 */
ipmi::RspType<uint16_t, // inventorySize
              uint8_t>  // accessType
    ipmiStorageGetFruInvAreaInfo(ipmi::Context::ptr ctx, uint8_t fruDeviceId)
{
    if (frus.empty() && retryCacheFru(getSdBus()))
    {
        return ipmi::responseUnspecifiedError();
    }

    if (fruDeviceId == 0xFF)
    {
        return ipmi::responseInvalidFieldRequest();
    }

    boost::container::flat_map<uint8_t, struct deviceInfo>* devices;
    if (dbusInterfacesDone == false && deviceMapCache.empty() == false)
    {
        devices = &deviceMapCache;
    }
    else
    {
        devices = &deviceMap;
    }

    auto deviceFind = devices->find(fruDeviceId);
    if (deviceFind == devices->end())
    {
        return ipmi::responseSensorInvalid();
    }

    constexpr uint8_t accessType =
        static_cast<uint8_t>(GetFRUAreaAccessType::byte);

    return ipmi::responseSuccess(deviceFind->second.size, accessType);
}

static constexpr auto ipmiSelService = "xyz.openbmc_project.Logging.IPMI";
static constexpr auto ipmiSelPath = "/xyz/openbmc_project/Logging/IPMI";
static constexpr auto ipmiSelInterface = "xyz.openbmc_project.Logging.IPMI";

static constexpr auto timeService = "xyz.openbmc_project.Time.Manager";
static constexpr auto timePath = "/xyz/openbmc_project/time/bmc";
static constexpr auto dbusProperties = "org.freedesktop.DBus.Properties";
static constexpr auto timeInterface = "xyz.openbmc_project.Time.EpochTime";
static constexpr auto timeElapsedProperty = "Elapsed";

static const std::filesystem::path selLogDir = "/var/log";
static const std::filesystem::path selLogTargetDir = "/usr/share/sel";
static constexpr auto selClearTimestamp = "/var/lib/ipmi/sel_clear_time";
static const std::string selLogFilename = "ipmi_sel";

// Max SEL size is 256k.
static constexpr size_t maxSelSize = 262144;
static constexpr uint8_t selOperationSupport = 0x02;
static constexpr uint8_t systemEvent = 0x02;
static constexpr size_t systemEventSize = 3;
static constexpr uint8_t oemTsEventFirst = 0xC0;
static constexpr uint8_t oemTsEventLast = 0xDF;
static constexpr size_t oemTsEventSize = 9;
static constexpr uint8_t oemEventFirst = 0xE0;
static constexpr uint8_t oemEventLast = 0xFF;
static constexpr size_t oemEventSize = 13;
static constexpr uint8_t eventMsgRev = 0x04;

// For leacky bucket SEL.
static constexpr uint16_t genIdBios = 0x0021;
static constexpr uint8_t sensorTypeMemory = 0x0C;
static constexpr uint8_t eventTypeSpecific = 0x6F;
static constexpr uint8_t eventDataMemoryCorrEcc = 0x0;
static constexpr uint8_t oemMcaStatusRecordType = 0xE0;
static constexpr uint8_t unifiedMemoryController1 = 0x11;
static constexpr uint8_t unifiedMemoryController2 = 0x12;
static constexpr uint8_t validAndCorrStatus = 0x80;

static int getFileTimestamp(const std::filesystem::path& file)
{
    struct stat st;

    if (stat(file.c_str(), &st) >= 0)
    {
        return st.st_mtime;
    }
    return ipmi::sel::invalidTimeStamp;
}

void updateSelClearTimestamp()
{
    // open the file, creating it if necessary
    int fd = open(selClearTimestamp, O_WRONLY | O_CREAT | O_CLOEXEC, 0644);
    if (fd < 0)
    {
        std::cerr << "Failed to open file\n";
        return;
    }

    // update the file timestamp to the current time
    if (futimens(fd, NULL) < 0)
    {
        std::cerr << "Failed to update timestamp: " << strerror(errno) << "\n";
    }
    close(fd);
}

static bool getSELLogFiles(std::vector<std::filesystem::path>& selLogFiles)
{
    // Loop through the directory looking for ipmi_sel log files
    for (const std::filesystem::directory_entry& dirEnt :
         std::filesystem::directory_iterator(selLogTargetDir))
    {
        std::string filename = dirEnt.path().filename();
        if (boost::starts_with(filename, selLogFilename))
        {
            // If we find an ipmi_sel log file, save the path
            selLogFiles.emplace_back(selLogTargetDir / filename);
        }
    }
    // As the log files rotate, they are appended with a ".#" that is higher for
    // the older logs. Since we don't expect more than 10 log files, we
    // can just sort the list to get them in order from newest to oldest
    std::sort(selLogFiles.begin(), selLogFiles.end());

    return !selLogFiles.empty();
}

static int countSELEntries(std::vector<std::filesystem::path>& selLogFiles)
{
    int numSELEntries = 0;
    // Loop through each log file and count the number of logs
    for (const std::filesystem::path& file : selLogFiles)
    {
        std::ifstream logFile(file);
        if (!logFile.is_open())
        {
            continue;
        }

        std::string line;
        while (std::getline(logFile, line))
        {
            numSELEntries++;
        }
    }
    return numSELEntries;
}

static bool findSELEntry(const int recordID,
                         const std::vector<std::filesystem::path>& selLogFiles,
                         std::string& entry)
{
    // Record ID is the first entry field following the timestamp. It is
    // preceded by a space and followed by a comma
    std::string search = " " + std::to_string(recordID) + ",";

    // Loop through the ipmi_sel log entries
    for (const std::filesystem::path& file : selLogFiles)
    {
        std::ifstream logFile(file);
        if (!logFile.is_open())
        {
            continue;
        }

        while (std::getline(logFile, entry))
        {
            // Check if the record ID matches
            if (entry.find(search) != std::string::npos)
            {
                return true;
            }
        }
    }
    return false;
}

static uint16_t
    getNextRecordID(const uint16_t recordID,
                    const std::vector<std::filesystem::path>& selLogFiles)
{
    uint16_t nextRecordID = recordID + 1;
    std::string entry;
    if (findSELEntry(nextRecordID, selLogFiles, entry))
    {
        return nextRecordID;
    }
    else
    {
        return ipmi::sel::lastEntry;
    }
}

static int fromHexStr(const std::string& hexStr, std::vector<uint8_t>& data)
{
    for (unsigned int i = 0; i < hexStr.size(); i += 2)
    {
        try
        {
            data.push_back(static_cast<uint8_t>(
                std::stoul(hexStr.substr(i, 2), nullptr, 16)));
        }
        catch (std::exception& e)
        {
            std::cerr << "Failed to parsing event data " << e.what() << "\n";
            return -1;
        }
    }
    return 0;
}

ipmi::RspType<uint8_t,  // SEL version
              uint16_t, // SEL entry count
              uint16_t, // free space
              uint32_t, // last add timestamp
              uint32_t, // last erase timestamp
              uint8_t>  // operationSupportion support
    ipmiStorageGetSELInfo()
{
    // Get the list of ipmi_sel log files
    std::vector<std::filesystem::path> selLogFiles;
    getSELLogFiles(selLogFiles);

    constexpr uint8_t selVersion = ipmi::sel::selVersion;
    uint16_t entries = countSELEntries(selLogFiles);

    // Calculate free space.
    size_t selSize = 0;
    for (const auto& path : selLogFiles)
    {
        selSize += std::filesystem::file_size(path);
    }
    size_t freeSpaceAll = maxSelSize - selSize;
    uint16_t freeSpace;
    if (freeSpaceAll >= 0xffff)
    {
        freeSpace = 0xffff;
    }
    else
    {
        freeSpace = freeSpaceAll;
    }

    uint32_t addTimeStamp = getFileTimestamp(selLogDir / selLogFilename);
    uint32_t eraseTimeStamp = getFileTimestamp(selClearTimestamp);
    constexpr uint8_t operationSupport = selOperationSupport;

    return ipmi::responseSuccess(selVersion, entries, freeSpace, addTimeStamp,
                                 eraseTimeStamp, operationSupport);
}

using systemEventType =
    std::tuple<uint32_t,                              // Timestamp
               uint16_t,                              // Generator ID
               uint8_t,                               // EvM Rev
               uint8_t,                               // Sensor Type
               uint8_t,                               // Sensor Number
               uint7_t,                               // Event Type
               bool,                                  // Event Direction
               std::array<uint8_t, systemEventSize>>; // Event Data

using oemTsEventType =
    std::tuple<uint32_t,                             // Timestamp
               std::array<uint8_t, oemTsEventSize>>; // Event Data

using oemEventType = std::array<uint8_t, oemEventSize>; // Event Data

ipmi::RspType<uint16_t, // Next Record ID
              uint16_t, // Record ID
              uint8_t,  // Record Type
              std::variant<systemEventType, oemTsEventType,
                           oemEventType>> // Record Content
    ipmiStorageGetSELEntry(uint16_t reservationID, uint16_t targetID,
                           uint8_t offset, uint8_t size)
{
    // Only support getting the entire SEL record. If a partial size or non-zero
    // offset is requested, return an error
    if (offset != 0 || size != ipmi::sel::entireRecord)
    {
        return ipmi::responseRetBytesUnavailable();
    }

    // Check the reservation ID if one is provided or required (only if the
    // offset is non-zero)
    if (reservationID != 0 || offset != 0)
    {
        if (!checkSELReservation(reservationID))
        {
            return ipmi::responseInvalidReservationId();
        }
    }

    // Get the ipmi_sel log files
    std::vector<std::filesystem::path> selLogFiles;
    if (!getSELLogFiles(selLogFiles))
    {
        return ipmi::responseSensorInvalid();
    }

    std::string targetEntry;
    if (targetID == ipmi::sel::firstEntry)
    {
        // The first entry will be at the top of the oldest log file
        std::ifstream logFile(selLogFiles.back());
        if (logFile.is_open() == false)
        {
            return ipmi::responseUnspecifiedError();
        }

        if (!std::getline(logFile, targetEntry))
        {
            return ipmi::responseUnspecifiedError();
        }
    }
    else if (targetID == ipmi::sel::lastEntry)
    {
        // The last entry will be at the bottom of the newest log file
        std::ifstream logFile(selLogFiles.front());
        if (logFile.is_open() == false)
        {
            return ipmi::responseUnspecifiedError();
        }

        std::string line;
        while (std::getline(logFile, line))
        {
            targetEntry = line;
        }
    }
    else
    {
        if (findSELEntry(targetID, selLogFiles, targetEntry) == false)
        {
            return ipmi::responseSensorInvalid();
        }
    }

    /**
     * Parsing SEL message, the format of the ipmi_sel message is below:
     * <Timestamp> <ID>,<Type>,<EventData>,[<Generator ID>,<Path>,<Direction>].
     **/

    // Get the Timestamp
    size_t space = targetEntry.find_first_of(" ");
    if (space == std::string::npos)
    {
        return ipmi::responseUnspecifiedError();
    }
    std::string entryTimestamp = targetEntry.substr(0, space);

    // Get the remaining log contents
    size_t entryStart = targetEntry.find_first_not_of(" ", space);
    if (entryStart == std::string::npos)
    {
        return ipmi::responseUnspecifiedError();
    }
    std::string_view entry(targetEntry);
    entry.remove_prefix(entryStart);

    // Use split to separate the entry into its fields
    std::vector<std::string> targetEntryFields;
    boost::split(targetEntryFields, entry, boost::is_any_of(","),
                 boost::token_compress_on);
    if (targetEntryFields.size() < 3)
    {
        return ipmi::responseUnspecifiedError();
    }
    std::string& recordIDStr = targetEntryFields[0];
    std::string& recordTypeStr = targetEntryFields[1];
    std::string& eventDataStr = targetEntryFields[2];

    uint16_t recordID;
    uint8_t recordType;
    try
    {
        recordID = std::stoul(recordIDStr);
        recordType = std::stoul(recordTypeStr, nullptr, 16);
    }
    catch (const std::invalid_argument&)
    {
        return ipmi::responseUnspecifiedError();
    }

    uint16_t nextRecordID = getNextRecordID(recordID, selLogFiles);
    std::vector<uint8_t> eventDataBytes;
    if (fromHexStr(eventDataStr, eventDataBytes) < 0)
    {
        return ipmi::responseUnspecifiedError();
    }

    if (recordType == systemEvent && targetEntryFields.size() > 4)
    {
        // Get the timestamp
        std::tm timeStruct = {};
        std::istringstream entryStream(entryTimestamp);

        uint32_t timestamp = ipmi::sel::invalidTimeStamp;
        if (entryStream >> std::get_time(&timeStruct, "%Y-%m-%dT%H:%M:%S"))
        {
            timestamp = std::mktime(&timeStruct);
        }

        // Set the event message revision
        uint8_t evmRev = eventMsgRev;

        uint16_t generatorID = 0;
        uint8_t sensorType = 0;
        uint8_t sensorNum = 0xFF;
        uint7_t eventType = 0;
        bool eventDir = 0;
        // System type events should have six fields
        if (targetEntryFields.size() >= 6)
        {
            std::string& generatorIDStr = targetEntryFields[3];
            std::string& sensorPath = targetEntryFields[4];
            std::string& eventDirStr = targetEntryFields[5];

            // Get the generator ID
            try
            {
                generatorID = std::stoul(generatorIDStr, nullptr, 16);
            }
            catch (const std::invalid_argument&)
            {
                std::cerr << "Invalid Generator ID\n";
            }

            // Get sensor type, sensor number, and event type for the sensor.
            auto sensor = ipmi::sensor::sensorsInfo.find(sensorPath);
            if (sensor != ipmi::sensor::sensorsInfo.end())
            {
                sensorNum = sensor->second.sensorNumber;
                sensorType = sensor->second.sensorType;
                eventType = sensor->second.sensorReadingType;
            }
            else
            {
                std::cerr << "Unknown sensor path: " << sensorPath << "\n";
            }

            // Get the event direction
            try
            {
                eventDir = std::stoul(eventDirStr) ? 0 : 1;
            }
            catch (const std::invalid_argument&)
            {
                std::cerr << "Invalid Event Direction\n";
            }
        }

        // Only keep the eventData bytes that fit in the record
        std::array<uint8_t, systemEventSize> eventData{};
        std::copy_n(eventDataBytes.begin(),
                    std::min(eventDataBytes.size(), eventData.size()),
                    eventData.begin());

        return ipmi::responseSuccess(
            nextRecordID, recordID, recordType,
            systemEventType{timestamp, generatorID, evmRev, sensorType,
                            sensorNum, eventType, eventDir, eventData});
    }
    else if (((recordType >= oemTsEventFirst) &&
              (recordType <= oemTsEventLast)) ||
             (recordType == systemEvent) && (targetEntryFields.size() == 4))
    {
        // Get the timestamp
        std::tm timeStruct = {};
        std::istringstream entryStream(entryTimestamp);

        uint32_t timestamp = ipmi::sel::invalidTimeStamp;
        if (entryStream >> std::get_time(&timeStruct, "%Y-%m-%dT%H:%M:%S"))
        {
            timestamp = std::mktime(&timeStruct);
        }

        // Only keep the bytes that fit in the record
        std::array<uint8_t, oemTsEventSize> eventData{};
        std::copy_n(eventDataBytes.begin(),
                    std::min(eventDataBytes.size(), eventData.size()),
                    eventData.begin());

        return ipmi::responseSuccess(nextRecordID, recordID, recordType,
                                     oemTsEventType{timestamp, eventData});
    }
    else if ((recordType >= oemEventFirst) && (recordType <= oemEventLast))
    {
        // Only keep the bytes that fit in the record
        std::array<uint8_t, oemEventSize> eventData{};
        std::copy_n(eventDataBytes.begin(),
                    std::min(eventDataBytes.size(), eventData.size()),
                    eventData.begin());

        return ipmi::responseSuccess(nextRecordID, recordID, recordType,
                                     eventData);
    }

    return ipmi::responseUnspecifiedError();
}

ipmi::RspType<uint16_t> ipmiStorageAddSELEntry(
    uint16_t recordID, uint8_t recordType, uint32_t timestamp,
    uint16_t generatorID, uint8_t evmRev, uint8_t sensorType, uint8_t sensorNum,
    uint8_t eventType, uint8_t eventData1, uint8_t eventData2,
    uint8_t eventData3)
{

    uint16_t resID = 0xFFFF;

    // Filter Memory Correctable Ecc Event
    if ((generatorID == genIdBios) && (sensorType == sensorTypeMemory) &&
        (eventType == eventTypeSpecific) &&
        ((eventData1 & 0x0f) == eventDataMemoryCorrEcc))
    {
        tempDimmSensorNum = sensorNum;
        auto dimmPath = leakyBucketDimmPathRoot + std::to_string(sensorNum);

        std::shared_ptr<sdbusplus::asio::connection> bus = getSdBus();
        auto dimmEccMsg =
            bus->new_method_call(leakyBucketService, dimmPath.c_str(),
                                 leakyBucketEccErrorIntf, "DimmEccAssert");
        try
        {
            bus->call_noreply(dimmEccMsg);
        }
        catch (sdbusplus::exception_t& e)
        {
            std::cerr << "Failed to add correctable memory ecc: " << e.what()
                      << "\n";
            return ipmi::responseResponseError();
        }

        hadCorrectableEcc = true;
        if (cancelOemMcaSelTimer->isRunning())
        {
            cancelOemMcaSelTimer->stop();
        }
        cancelOemMcaSelTimer->start(
            std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::seconds(clearFlagTimeoutSeconds)));

        return ipmi::responseSuccess(resID);
    }

    // Per the IPMI spec, need to cancel the reservation when a SEL entry is
    // added
    cancelSELReservation();

    std::vector<uint8_t> eventData;
    /* Follow the section-32 of ipmi spec,
     * There are three kinds of Record Types
     * 1. 02h - System Event Record
     * 2. C0h-DFh - OEM timestamped, byte 8-16 OEM defined
     * 3. E0h-FFh - OEM non-timestamped, byte 4-16 OEM defined
     */
    if ((recordType == systemEvent) ||
        ((recordType >= oemTsEventFirst) && (recordType <= oemTsEventLast)))
    {
        eventData.resize(9);
        eventData[0] = generatorID;
        eventData[1] = generatorID >> 8;
        eventData[2] = evmRev;
        eventData[3] = sensorType;
        eventData[4] = sensorNum;
        eventData[5] = eventType;
        eventData[6] = eventData1;
        eventData[7] = eventData2;
        eventData[8] = eventData3;
    }
    else if ((recordType >= oemEventFirst) && (recordType <= oemEventLast))
    {
        eventData.resize(13);
        eventData[0] = timestamp;
        eventData[1] = timestamp >> 8;
        eventData[2] = timestamp >> 16;
        eventData[3] = timestamp >> 24;
        eventData[4] = generatorID;
        eventData[5] = generatorID >> 8;
        eventData[6] = evmRev;
        eventData[7] = sensorType;
        eventData[8] = sensorNum;
        eventData[9] = eventType;
        eventData[10] = eventData1;
        eventData[11] = eventData2;
        eventData[12] = eventData3;

        /* Check the "OEM MCA Status SEL" that is generated by memory
         * correctable ECC error
         * The bit-7 of event data 5 is Val.
         * The bit-5 of event data 5 is UC.
         */
        if ((recordType == oemMcaStatusRecordType) &&
            ((eventData[0] == unifiedMemoryController1) ||
             (eventData[0] == unifiedMemoryController2)) &&
            ((eventData[5] & (BIT(7) | BIT(5))) == validAndCorrStatus))
        {
            if (!hadCorrectableEcc)
            {
                std::cerr << "Drop this event log: ";
                std::copy(eventData.begin(), eventData.end(),
                          std::ostream_iterator<int>(std::cerr, " "));
                std::cerr << "\n";
                return ipmi::responseSuccess(resID);
            }

            hadCorrectableEcc = false;

            cancelOemMcaSelTimer->stop();

            auto dimmPath =
                leakyBucketDimmPathRoot + std::to_string(tempDimmSensorNum);

            std::shared_ptr<sdbusplus::asio::connection> bus = getSdBus();
            auto eventDataMsg = bus->new_method_call(
                leakyBucketService, dimmPath.c_str(), dbusProperties, "Set");
            eventDataMsg.append(leakyBucketEccErrorIntf,
                                "OemMcaStatusEventData",
                                std::variant<std::vector<uint8_t>>(eventData));
            try
            {
                bus->call(eventDataMsg);
            }
            catch (sdbusplus::exception::SdBusError& e)
            {
                std::cerr << "Failed to set OemMcaStatusEventData property,"
                          << e.what() << "\n";
                return ipmi::responseResponseError();
            }

            return ipmi::responseSuccess(resID);
        }
    }
    else
    {
        std::cerr << "Undefind the Record Type[" << std::hex << (int)recordType
                  << "]\n";
        return ipmi::responseUnspecifiedError();
    }

    std::shared_ptr<sdbusplus::asio::connection> bus = getSdBus();
    auto writeSEL = bus->new_method_call(ipmiSelService, ipmiSelPath,
                                         ipmiSelInterface, "IpmiSelAddOem");
    writeSEL.append("IPMI SEL Add Oem", eventData, recordType);

    uint16_t responseID = 0xFFFF;
    try
    {
        auto ret = bus->call(writeSEL);
        ret.read(responseID);
    }
    catch (sdbusplus::exception_t& e)
    {
        std::cerr << "Failed to call IpmiSelAddOem failed\n";
        return ipmi::responseResponseError();
    }

    return ipmi::responseSuccess(responseID);
}

ipmi::RspType<uint8_t> ipmiStorageClearSEL(uint16_t reservationID,
                                           const std::array<uint8_t, 3>& clr,
                                           uint8_t eraseOperation)
{
    static constexpr std::array<uint8_t, 3> clrExpected = {'C', 'L', 'R'};
    if (clr != clrExpected)
    {
        return ipmi::responseInvalidFieldRequest();
    }

    /*
     * Erasure status cannot be fetched from DBUS, so always return erasure
     * status as `erase completed`.
     */
    if (eraseOperation == ipmi::sel::getEraseStatus)
    {
        return ipmi::responseSuccess(
            static_cast<uint8_t>(ipmi::sel::eraseComplete));
    }
    else if (eraseOperation != ipmi::sel::initiateErase)
    {
        return ipmi::responseInvalidFieldRequest();
    }

    // Per the IPMI spec, need to cancel any reservation when the SEL is cleared
    cancelSELReservation();

    // Save the clear time
    updateSelClearTimestamp();

    std::shared_ptr<sdbusplus::asio::connection> bus = getSdBus();
    auto selClearMsg = bus->new_method_call(ipmiSelService, ipmiSelPath,
                                            ipmiSelInterface, "IpmiSelClear");

    int rc;
    try
    {
        auto reply = bus->call(selClearMsg);
        reply.read(rc);
    }
    catch (sdbusplus::exception_t& e)
    {
        std::cerr << "Error to call sel clear method: " << e.what() << "\n";
        return ipmi::responseResponseError();
    }

    if (rc < 0)
    {
        std::cerr << "Error while clearing sel: " << strerror(errno) << "\n";

        switch (rc)
        {
            case -EIO:
                return ipmi::responseResponseError();
            case -ETIME:
                return ipmi::responseTimeout();
            case -EBADFD:
            case -EINVAL:
                return ipmi::responseUnspecifiedError();
            default:
                return ipmi::responseUnspecifiedError();
        }
    }

    return ipmi::responseSuccess(ipmi::sel::eraseComplete);
}

ipmi::RspType<uint32_t> ipmiStorageGetSELTime()
{
    using namespace std::chrono;

    sdbusplus::bus::bus bus(ipmid_get_sd_bus_connection());
    auto method =
        bus.new_method_call(timeService, timePath, dbusProperties, "Get");
    method.append(timeInterface, timeElapsedProperty);

    uint64_t timeUsec = 0;
    try
    {
        auto reply = bus.call(method);

        std::variant<uint64_t> value;
        reply.read(value);
        timeUsec = std::get<uint64_t>(value);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        std::cerr << "Failed to get sel time: " << e.what() << "\n";
        return ipmi::responseResponseError();
    }

    // Time is really long int but IPMI wants just uint32. This works okay until
    // the number of seconds since 1970 overflows uint32 size.. Still a whole
    // lot of time here to even think about that.
    return ipmi::responseSuccess(
        duration_cast<seconds>(microseconds(timeUsec)).count());
}

ipmi::RspType<> ipmiStorageSetSELTime(uint32_t selTime)
{
    using namespace std::chrono;

    sdbusplus::bus::bus bus(ipmid_get_sd_bus_connection());

    microseconds timeUsec = seconds(selTime);
    std::variant<uint64_t> value(static_cast<uint64_t>(timeUsec.count()));

    auto method =
        bus.new_method_call(timeService, timePath, dbusProperties, "Set");
    method.append(timeInterface, timeElapsedProperty, value);
    try
    {
        bus.call_noreply(method);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        std::cerr << "Failed to set sel time: " << e.what() << "\n";
        return ipmi::responseResponseError();
    }

    // Sync RTC clock.
    auto fp = popen("/sbin/hwclock -w", "r");
    if (fp == NULL)
    {
        std::cerr << "Failed to open popen when sync RTC clock\n";
        return ipmi::responseUnspecifiedError();
    }
    char buffer[128] = {0};
    while (fgets(buffer, 128, fp) != NULL)
    {
        std::cerr << "Failed to sync RTC clock: " << buffer << "\n";
        pclose(fp);
        return ipmi::responseUnspecifiedError();
    }
    pclose(fp);

    return ipmi::responseSuccess();
}

void registerStorageFunctions()
{
    createTimers();
    startMatch();

    // <Get FRU Inventory Area Info>
    ipmi::registerHandler(ipmi::prioOemBase, ipmi::netFnStorage,
                          ipmi::storage::cmdGetFruInventoryAreaInfo,
                          ipmi::Privilege::User, ipmiStorageGetFruInvAreaInfo);
    // <READ FRU Data>
    ipmi::registerHandler(ipmi::prioOpenBmcBase, ipmi::netFnStorage,
                          ipmi::storage::cmdReadFruData, ipmi::Privilege::User,
                          ipmiStorageReadFruData);

    // <WRITE FRU Data>
    ipmi::registerHandler(ipmi::prioOpenBmcBase, ipmi::netFnStorage,
                          ipmi::storage::cmdWriteFruData,
                          ipmi::Privilege::Operator, ipmiStorageWriteFruData);

    // <Get SEL Info>
    ipmi::registerHandler(ipmi::prioOpenBmcBase, ipmi::netFnStorage,
                          ipmi::storage::cmdGetSelInfo, ipmi::Privilege::User,
                          ipmiStorageGetSELInfo);

    // <Get SEL Entry>
    ipmi::registerHandler(ipmi::prioOpenBmcBase, ipmi::netFnStorage,
                          ipmi::storage::cmdGetSelEntry, ipmi::Privilege::User,
                          ipmiStorageGetSELEntry);

    // <Add SEL Entry>
    ipmi::registerHandler(ipmi::prioOpenBmcBase, ipmi::netFnStorage,
                          ipmi::storage::cmdAddSelEntry,
                          ipmi::Privilege::Operator, ipmiStorageAddSELEntry);

    // <Clear SEL>
    ipmi::registerHandler(ipmi::prioOpenBmcBase, ipmi::netFnStorage,
                          ipmi::storage::cmdClearSel, ipmi::Privilege::Operator,
                          ipmiStorageClearSEL);

    // <Get SEL Time>
    ipmi::registerHandler(ipmi::prioOpenBmcBase, ipmi::netFnStorage,
                          ipmi::storage::cmdGetSelTime,
                          ipmi::Privilege::Operator, ipmiStorageGetSELTime);

    // <Set SEL Time>
    ipmi::registerHandler(ipmi::prioOpenBmcBase, ipmi::netFnStorage,
                          ipmi::storage::cmdSetSelTime,
                          ipmi::Privilege::Operator, ipmiStorageSetSELTime);
}
} // namespace storage
} // namespace ipmi
