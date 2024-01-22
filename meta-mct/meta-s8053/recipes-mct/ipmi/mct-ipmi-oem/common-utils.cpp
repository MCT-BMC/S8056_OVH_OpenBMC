#include "common-utils.hpp"

namespace common
{
static int getProperty(sdbusplus::bus::bus& bus, const std::string& path,
                       const std::string& property, double& value,
                       const std::string service,const std::string interface)
{
    std::variant<double> valuetmp;
    auto method = bus.new_method_call(service.c_str(), path.c_str(), PROPERTY_INTERFACE, "Get");
    method.append(interface.c_str(),property);
    try
    {
        auto reply=bus.call(method);
        if (reply.is_method_error())
        {
            std::printf("Error looking up services, PATH=%s",interface.c_str());
            return -1;
        }
        reply.read(valuetmp);
    }
    catch (const sdbusplus::exception::SdBusError& e){
        return -1;
    }

    value = std::get<double>(valuetmp);
    return 0;
}

static void setProperty(sdbusplus::bus::bus& bus, const std::string& path,
                 const std::string& property, const double value)
{
    auto method = bus.new_method_call(fsc::busName, path.c_str(),
                                      PROPERTY_INTERFACE, "Set");
    method.append(fsc::interface, property, std::variant<double>(value));

    bus.call_noreply(method);

    return;
}

int logSelEvent(std::string enrty, std::string path ,
                     uint8_t eventData0, uint8_t eventData1, uint8_t eventData2)
{
    std::vector<uint8_t> eventData(3, 0xFF);
    eventData[0] = eventData0;
    eventData[1] = eventData1;
    eventData[2] = eventData2;
    uint16_t genid = 0x20;

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
        std::cerr << "Failed to log the button event:" << e.what() << "\n";
        return -1;
    }
    return 0 ;
}

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

std::string intToHexString(int data)
{
    char buffer[10];
    snprintf(buffer, sizeof(buffer), "0x%X", data);
    return buffer;
}

int setBmcService(std::string service,std::string status, std::string setting)
{
    constexpr auto SYSTEMD_SERVICE = "org.freedesktop.systemd1";
    constexpr auto SYSTEMD_OBJ_PATH = "/org/freedesktop/systemd1";
    constexpr auto SYSTEMD_INTERFACE = "org.freedesktop.systemd1.Manager";

    auto bus = sdbusplus::bus::new_default();

    // start/stop servcie
    if(status != "Null"){
        auto method = bus.new_method_call(SYSTEMD_SERVICE, SYSTEMD_OBJ_PATH, SYSTEMD_INTERFACE, status.c_str());
        method.append(service, "replace");

        try
        {
            auto reply = bus.call(method);
        }
        catch (const sdbusplus::exception::SdBusError& e)
        {
            log<level::ERR>("Error in start/stop servcie",entry("ERROR=%s", e.what()));
            return -1;
        }
    }

    // enable/disable service
    if(setting != "Null"){
        auto method = bus.new_method_call(SYSTEMD_SERVICE, SYSTEMD_OBJ_PATH, SYSTEMD_INTERFACE,setting.c_str());

        std::array<std::string, 1> appendMessage = {service};

        if(setting == "EnableUnitFiles")
        {
            method.append(appendMessage, false, true);
        }
        else
        {
            method.append(appendMessage, false);
        }

        try
        {
            auto reply = bus.call(method);
        }
        catch (const sdbusplus::exception::SdBusError& e)
        {
            log<level::ERR>("Error in enable/disable service",entry("ERROR=%s", e.what()));
            return -1;
        }
    }

     return 0;
}

bool isPowerOn()
{
    auto bus = sdbusplus::bus::new_default();
    auto method = bus.new_method_call(chassis::busName, chassis::path, PROPERTY_INTERFACE,"Get");
    method.append(chassis::interface, chassis::request);
    std::variant<std::string> result;
    try
    {
        auto reply = bus.call(method);
        reply.read(result);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        return false;
    }

    bool on = boost::ends_with(std::get<std::string>(result), "On");

    if(on)
    {
        return true;
    }

    return false;
}

bool hasBiosPost()
{
    auto bus = sdbusplus::bus::new_default();
    auto method = bus.new_method_call(post::busName, post::path, PROPERTY_INTERFACE, "Get");
    method.append(post::interface, post::request);
    std::variant<std::string> result;
    try
    {
        auto reply = bus.call(method);
        reply.read(result);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        return false;
    }

    bool biosHasPost = boost::ends_with(std::get<std::string>(result), "Standby");

    if(biosHasPost)
    {
        return true;
    }

    return false;
}

bool setAst2500Register(uint32_t address,uint32_t setRegister)
{
    uint32_t registerBuffer = 0;

    if(read_register(address,&registerBuffer) < 0)
    {
        std::cerr << "Failed to read ast2500 register \n";
        return false;
    }

    registerBuffer = registerBuffer | setRegister;

    write_register(address, registerBuffer);

    return true;
}

void setGpioOutput(std::string lineName, int setValue)
{
    int value;
    struct gpiod_line *line = nullptr;;

    line = gpiod_line_find(lineName.c_str());
    if (gpiod_line_request_output(line,"set-output",setValue) < 0)
    {
        std::cerr << "Error request line" << std::endl;;
    }
    gpiod_line_release(line);
}

int getBaseboardFruAddress(uint8_t& bus, uint8_t& addr)
{
    int indexSelect= 0;
    std::ifstream FruFile(fruConfig);

    if(!FruFile)
    {
        std::cerr << "Failed to open FRU config file: " << fruConfig << std::endl;
        return -1;
    }

    auto data = nlohmann::json::parse(FruFile, nullptr, false);
    if (data.is_discarded())
    {
        std::cerr << "syntax error in " << fruConfig << std::endl;
        return -1;
    }
    if (data["Fru"][indexSelect]["Index"].is_null() ||
        data["Fru"][indexSelect]["Bus"].is_null() ||
        data["Fru"][indexSelect]["Address"].is_null())
    {
        std::cerr << "FRU index-" << std::to_string(indexSelect) <<" data invalied " << std::endl;
        return -1;
    }

    bus = data["Fru"][indexSelect]["Bus"];
    std::string addrStr = data["Fru"][indexSelect]["Address"];
    addr = std::stoul(addrStr, nullptr, 16);
    return 0;
}

uint8_t calculateZeroChecksum(const std::vector<uint8_t>& data)
{
    uint8_t checksum = 0;
    for (const auto& value : data)
    {
        checksum += value;
    }

    return (~checksum) + 1;
}

bool validateZeroChecksum(const std::vector<uint8_t>& data)
{
    if (data.empty())
    {
        return false;
    }

    uint8_t checkSum = 0;
    for (const auto& value : data)
    {
        checkSum += value;
    }

    if (checkSum == 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

int getCurrentNode(uint8_t& nodeId)
{
    std::shared_ptr<sdbusplus::asio::connection> busp = getSdBus();
    ipmi::Value result;

    try
    {
        result = ipmi::getDbusProperty(
                    *busp, node::busName, node::path, node::interface, node::request_nodeID);
        nodeId = std::get<uint8_t>(result);
    }
    catch (const std::exception& e)
    {
        return -1;
    }

    return 0;
}

int getCurrentSpecialMode(secCtrl::SpecialMode::Modes& mode)
{
    std::shared_ptr<sdbusplus::asio::connection> busp = getSdBus();
    ipmi::Value result;

    try
    {
        result = ipmi::getDbusProperty(
                    *busp, obmcSpMode::busName, obmcSpMode::path, obmcSpMode::interface, obmcSpMode::prop);
        mode = secCtrl::SpecialMode::convertModesFromString(std::get<std::string>(result));
    }
    catch (const std::exception& e)
    {
        return -1;
    }

    return 0;
}

int setCurrentSpecialMode(secCtrl::SpecialMode::Modes mode)
{
    std::shared_ptr<sdbusplus::asio::connection> busp = getSdBus();
    ipmi::Value result;

    try
    {
         ipmi::setDbusProperty(
                    *busp, obmcSpMode::busName, obmcSpMode::path, obmcSpMode::interface, obmcSpMode::prop,
                    secCtrl::SpecialMode::convertModesToString(mode));
    }
    catch (const std::exception& e)
    {
        return -1;
    }

    return 0;
}

} // namespace common