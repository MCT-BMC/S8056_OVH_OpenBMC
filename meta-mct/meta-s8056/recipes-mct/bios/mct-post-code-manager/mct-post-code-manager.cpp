#include "mct-post-code-manager.hpp"

static constexpr bool DEBUG = false;

std::vector<uint64_t> getPostCodes()
{
    return collectedPostCode;
}

inline static sdbusplus::bus::match::match
    setPostCodeHandler(std::shared_ptr<sdbusplus::asio::connection>& systemBus)
{
    auto postCodeMatcherCallback = [&](sdbusplus::message::message &msg)
    {
        uint64_t value;
        msg.read(value);

        if constexpr (DEBUG)
        {
            char buffer[10];
            sprintf(buffer, "0x%X", value);
            std::string val = buffer;
            std::cerr << "Current post code: " << val << std::endl;
        }

        if(collectedPostCode.size() >= MAX_COLLECTION_POST_CODE_SIZE){
            collectedPostCode.erase(collectedPostCode.begin());
            collectedPostCode.push_back(value);
        }
        else{
            collectedPostCode.push_back(value);
        }

    };
    sdbusplus::bus::match::match postCodeMatcher(
        static_cast<sdbusplus::bus::bus &>(*systemBus),
        sdbusplus::bus::match::rules::type::signal() +
        sdbusplus::bus::match::rules::member(std::string("getPostCode")),
        std::move(postCodeMatcherCallback));
    return postCodeMatcher;
}

inline static sdbusplus::bus::match::match
    setLpcResetHandler(std::shared_ptr<sdbusplus::asio::connection>& systemBus)
{
    auto lpcResetMatcherCallback = [&](sdbusplus::message::message &msg)
    {
        uint32_t value;
        msg.read(value);

        if(value == 0x01)
        {
            collectedPostCode.clear();
        }
    };
    sdbusplus::bus::match::match lpcResetMatcher(
        static_cast<sdbusplus::bus::bus &>(*systemBus),
        sdbusplus::bus::match::rules::type::signal() +
        sdbusplus::bus::match::rules::member(std::string("LpcReset")),
        std::move(lpcResetMatcherCallback));
    return lpcResetMatcher;
}

void dbusServiceInitialize()
{
    systemBus->request_name(postCode::busName);
    sdbusplus::asio::object_server objServer(systemBus);
    iface=objServer.add_interface(postCode::path,postCode::interface);
    iface->register_method(postCode::getPostCodeMethod, getPostCodes);
    iface->initialize();
}

int main()
{
    systemBus = std::make_shared<sdbusplus::asio::connection>(io);

    dbusServiceInitialize();

    sdbusplus::bus::match::match postCodeHandler = setPostCodeHandler(systemBus);
    sdbusplus::bus::match::match lpcResetHandler = setLpcResetHandler(systemBus);

    io.run();

    return 0;
}