#include "mct-leaky-bucket.hpp"

static constexpr bool DEBUG = false;

boost::container::flat_map<std::string, std::unique_ptr<leakyBucketSensor>> lbSensors;

std::shared_ptr<sdbusplus::asio::connection> systemBus;
std::shared_ptr<sdbusplus::asio::dbus_interface> iface;

uint8_t t1 = 1;
uint8_t t2 = 0;
uint8_t t3 = 10;
static size_t pollTime = 24;

static void logOemSelEvent(std::string enrty, uint8_t sensorNumber, uint8_t eventData1)
{
    uint8_t recordType = 0x2; // Record Type

    std::vector<uint8_t> selData(9, 0xFF);
    selData[0] = 0x3f;         // Generator ID
    selData[1] = 0x00;         // Generator ID
    selData[2] = 0x04;         // EvM Rev
    selData[3] = 0x0C;         // Sensor Type - Memory
    selData[4] = sensorNumber; // Sensor Number
    selData[5] = 0x6f;         // Event Dir | Event Type
    selData[6] = eventData1;   // Event Data 1
    selData[7] = 0xFF;         // Event Data 2
    selData[8] = 0xFF;         // Event Data 3

    auto bus = sdbusplus::bus::new_default();
    sdbusplus::message::message writeSEL = bus.new_method_call(
        ipmi::busName, ipmi::path,ipmi::interface, ipmi::request);
    writeSEL.append(enrty, selData, recordType);
    try
    {
        bus.call(writeSEL);
    }
    catch (std::exception& e)
    {
        std::cerr << "Failed to add Memory correctable ECC event:" << e.what() << std::endl;
    }
}

leakyBucketSensor::leakyBucketSensor(const std::string& name, const uint8_t& sensorNum,
                                     std::shared_ptr<sdbusplus::asio::connection>& conn,
                                     boost::asio::io_service& io, sdbusplus::asio::object_server& objectServer):
                                     name(name), sensorNum(sensorNum), objectServer(objectServer), dbusConnection(conn), waitTimer(io)
{
    std::string bucketPath = HOST_DIMM_ECC_Path + "/" + name;
    intf = objectServer.add_interface(bucketPath,VALUE_INTERFACE);
    intf->register_property("count", count);
    intf->register_property("limitCount", limitCount);
    intf->initialize();

    lpcResetMatch = std::make_unique<sdbusplus::bus::match::match>(
        static_cast<sdbusplus::bus::bus&>(*conn),
        sdbusplus::bus::match::rules::type::signal() +
        sdbusplus::bus::match::rules::member(std::string("LpcReset")),
        [&](sdbusplus::message::message& message) {
            uint32_t value;
            message.read(value);
            if(value == 0x01){
                if(count != 0 || limitCount !=0)
                {
                    std::cerr << this->name << " clear leaky bucket internal conuter." << std::endl;
                }
                this->count = 0;
                this->limitCount = 0;
                this->intf->set_property("count", this->count);
                this->intf->set_property("limitCount", this->limitCount);
            }
        });
    }

leakyBucketSensor::~leakyBucketSensor()
{
    waitTimer.cancel();
    objectServer.remove_interface(intf);
}

void createBucket(const std::string &bucketName, const uint8_t &sensorNum,
                  boost::asio::io_service& io, sdbusplus::asio::object_server& objectServer,
                  std::shared_ptr<sdbusplus::asio::connection>& dbusConnection)
{
    if (!dbusConnection)
    {
        std::cerr << "Connection not created\n";
        return;
    }
    
    auto& bucket = lbSensors[bucketName];

    bucket = std::make_unique<leakyBucketSensor>(
            bucketName, sensorNum, dbusConnection, io, objectServer);

    bucket->init();
}

void leakyBucketSensor::init(void)
{
    read();
}

void leakyBucketSensor::read(void)
{

    waitTimer.expires_from_now(boost::asio::chrono::hours(pollTime));
    waitTimer.async_wait([this](const boost::system::error_code& ec) {
        if (ec == boost::asio::error::operation_aborted)
        {
            return; // we're being cancelled
        }
        // read timer error
        else if (ec)
        {
            std::cerr << "timer error\n";
            return;
        }
        count = (count > t2) ? (count-t2):0;
        intf->set_property("count", count);
        read();
    });
}

bool leakyBucketSensor::addEcc(void)
{
    bool overflow = false;

    if(limitCount >= t3)
    {
        if(enableLimitSel)
        {
            logOemSelEvent("Memory ECC limit SEL Entry", sensorNum , 0x05);
            enableLimitSel = false;
        }

        return overflow;
    }

    count++;
    enableLimitSel = true;
    
    //restart timer 
#if 0    
    if(count == 1)
    {
        waitTimer.cancel();
        waitTimer.expires_from_now(boost::asio::chrono::hours(pollTime));
    }
#endif     
    //NOTE!!: according to F4 implementation, the count should be cleared when overflow
    if(count >= t1){
        count = 0; 
        overflow = true;
        limitCount++;
    }

    std::cerr << "limitCount" << std::to_string(limitCount) << std::endl;
    std::cerr << "overflow" << std::to_string(overflow) << std::endl;

    intf->set_property("count", count);
    intf->set_property("limitCount", limitCount);
    if constexpr (1){
        typedef std::chrono::duration<int, std::ratio<86400>> days;

        std::chrono::duration<int64_t, std::nano> td = waitTimer.expires_from_now();
        std::chrono::nanoseconds ns = std::chrono::nanoseconds(td.count());
        auto d = std::chrono::duration_cast<days>(ns);
        ns -= d;
        auto h = std::chrono::duration_cast<std::chrono::hours>(ns);
        ns -= h;
        auto m = std::chrono::duration_cast<std::chrono::minutes>(ns);
        ns -= m;
        auto s = std::chrono::duration_cast<std::chrono::seconds>(ns);

        std::cerr
            << std::setw(2) << d.count() << "d:"
            << std::setw(2) << h.count() << "h:"
            << std::setw(2) << m.count() << "m:"
            << std::setw(2) << s.count() << 's'
            << "\n";
    }
    
    return overflow;
}

static bool increaseEccToBucket(const std::string &bucketName, const uint8_t &sensorNum,
                                boost::asio::io_service& io, sdbusplus::asio::object_server& objectServer,
                                std::shared_ptr<sdbusplus::asio::connection>& dbusConnection)
{
    auto bucket = lbSensors.find(bucketName);
    if (bucket != lbSensors.end())
    {
        std::cerr << "bucket existing:" << bucketName << std::endl;
    }else{
       createBucket(bucketName, sensorNum, io, objectServer, dbusConnection);
    }

    auto& sensor = lbSensors[bucketName];
    return sensor->addEcc();
    
}

void updateConfig()
{
    static bool init = false;

    if(!init)
    {
        init = true;
        std::ifstream lbConfig(LEAKLY_BUCKET_CONFIG_PATH,std::ios::binary);
        if(!lbConfig.good())
        {
            std::cerr << "file not exist, take default\n";
            return;
        }
        //load from file
 
        t1 = lbConfig.get();
        t2 = lbConfig.get();
        t3 = lbConfig.get();
        pollTime = (size_t)lbConfig.get();
  
        return;
    }

    std::ofstream output(LEAKLY_BUCKET_CONFIG_PATH,std::ios::binary);
    if (!output.good())
    {
        std::cerr << "can't create config\n";
    }
    else
    {
        output << t1;
        output << t2;
        output << t3;
        output << (uint8_t)pollTime;
        output.close();
    }
    return;
}

void dbusServiceInitialize(boost::asio::io_service& io,sdbusplus::asio::object_server& objServer)
{
    updateConfig();

    systemBus->request_name(LEAKLY_BUCKET_SERVICE.c_str());
    iface = objServer.add_interface(HOST_DIMM_ECC_Path,VALUE_INTERFACE);
    iface->register_property("T1", t1, sdbusplus::asio::PropertyPermission::readWrite);
    iface->register_property("T2", t2, sdbusplus::asio::PropertyPermission::readWrite);
    iface->register_property("T3", t3, sdbusplus::asio::PropertyPermission::readWrite);
    iface->register_property("polltime", (uint8_t)pollTime, sdbusplus::asio::PropertyPermission::readWrite);
    iface->register_method(
        "increaseEccToBucket", [&](const std::string &bucketName, const uint8_t &sensorNum) {
            return increaseEccToBucket(bucketName, sensorNum, io, objServer, systemBus);
        });

    iface->initialize();
}

inline static sdbusplus::bus::match::match 
    setEccHandler(std::shared_ptr<sdbusplus::asio::connection>& systemBus)
{
    auto eventHandler =
        [&](sdbusplus::message::message& message) {
            boost::container::flat_map<std::string, std::variant<uint8_t>> values;
            std::string objectName;
            if (DEBUG)
            {
                std::cerr << message.get_path() << " is changed\n";
            }
 
            message.read(objectName, values);
          
            auto tValue = values.find("T1");
            if (tValue != values.end())
            {
                 std::cerr << "T1 change to :" << unsigned(std::get<std::uint8_t>(tValue->second)) << "\n";
                 t1 = std::get<std::uint8_t>(tValue->second);
            }
            tValue = values.find("T2");
            if (tValue != values.end())
            {
                 std::cerr << "T2 change to :" << unsigned(std::get<std::uint8_t>(tValue->second)) << "\n";
                 t2 = std::get<std::uint8_t>(tValue->second);
            }
            tValue = values.find("T3");
            if (tValue != values.end())
            {
                 std::cerr << "T3 change to :" << unsigned(std::get<std::uint8_t>(tValue->second)) << "\n";
                 t3 = std::get<std::uint8_t>(tValue->second);
            }
            tValue = values.find("polltime");
            if (tValue != values.end())
            {
                 std::cerr << "polltime change to :" << unsigned(std::get<std::uint8_t>(tValue->second)) << "\n";
                 pollTime = (size_t)(std::get<std::uint8_t>(tValue->second));
            }

            updateConfig();
        };

    sdbusplus::bus::match::match configMatch(
        static_cast<sdbusplus::bus::bus&>(*systemBus),
        sdbusplus::bus::match::rules::type::signal() +
        sdbusplus::bus::match::rules::member("PropertiesChanged") +
        sdbusplus::bus::match::rules::path_namespace(std::string(HOST_DIMM_ECC_Path)) +
        sdbusplus::bus::match::rules::arg0namespace(std::string(VALUE_INTERFACE)),
        eventHandler);
    
    return configMatch;
}

int main()
{
    boost::asio::io_service io;
    systemBus = std::make_shared<sdbusplus::asio::connection>(io);
    sdbusplus::asio::object_server objServer = sdbusplus::asio::object_server(systemBus);
    
    dbusServiceInitialize(io,objServer);
    sdbusplus::bus::match::match eccHandler = setEccHandler(systemBus);

    io.run();

    return 0;
}