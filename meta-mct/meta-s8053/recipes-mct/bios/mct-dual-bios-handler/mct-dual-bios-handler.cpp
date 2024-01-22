#include "mct-dual-bios-handler.hpp"

static constexpr bool DEBUG = false;

template <class T>
int setDbusProperty(std::shared_ptr<sdbusplus::asio::connection>& systemBus,
                    std::string busName, std::string path,std::string interface,
                    std::string request, T property)
{
    try
    {
        auto method = systemBus->new_method_call(busName.c_str(), path.c_str(),
                                                 PROPERTIES_INTERFACE,
                                                "Set");
        method.append(interface.c_str(), request,  std::variant<T>(property));
        systemBus->call_noreply(method);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Failed to set DBUS interface:" << interface << std::endl;
        return -1;
    }

    return 0;
}

template <class T>
int getDbusProperty(std::shared_ptr<sdbusplus::asio::connection>& systemBus,
                    std::string busName, std::string path,std::string interface,
                    std::string request,T& property)
{
    std::variant<T> result;
    try
    {
        auto method = systemBus->new_method_call(busName.c_str(), path.c_str(),
                                                 PROPERTIES_INTERFACE,
                                                "Get");
        method.append(interface.c_str(), request);
        auto reply = systemBus->call(method);
        reply.read(result);
        property = std::get<T>(result);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Failed to get DBUS interface:" << interface << std::endl;
        return -1;
    }

    return 0;
}

int sethostPowerState(std::shared_ptr<sdbusplus::asio::connection>& systemBus, std::string target)
{
    try
    {
        auto method = systemBus->new_method_call(host::busName, host::path,
                                                 PROPERTIES_INTERFACE,
                                                "Set");
        method.append(host::interface, host::request,  std::variant<std::string>(target.c_str()));
        systemBus->call_noreply(method);
    }
    catch (const std::exception& e)
    {
        return -1;
    }

    return 0;
}

static void logSELEvent(std::string enrty, std::string path ,
                        uint8_t eventData0, uint8_t eventData1, uint8_t eventData2)
{
    std::vector<uint8_t> eventData(3, 0xFF);
    eventData[0] = eventData0;
    eventData[1] = eventData1;
    eventData[2] = eventData2;
    uint16_t genid = 0x20;

    auto bus = sdbusplus::bus::new_default();
    sdbusplus::message::message writeSEL = bus.new_method_call(
        ipmi::busName, ipmi::path,ipmi::interface, ipmi::request);
    writeSEL.append(enrty, path, eventData, true, (uint16_t)genid);
    try
    {
        bus.call(writeSEL);
    }
    catch (std::exception& e)
    {
        std::cerr << "Failed to log the button event:" << e.what() << "\n";
    }
}

bool isPowerOn()
{
    auto bus = sdbusplus::bus::new_default();
    auto method = bus.new_method_call(chassis::busName, chassis::path, PROPERTIES_INTERFACE, "Get");
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

bool isPostEnd()
{
    auto bus = sdbusplus::bus::new_default();
    auto method = bus.new_method_call(postComplete::busName, postComplete::path, PROPERTIES_INTERFACE, "Get");
    method.append(postComplete::interface, postComplete::request);
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

    bool enable = boost::ends_with(std::get<std::string>(result), "Standby");

    if(enable)
    {
        return true;
    }

    return false;
}

bool isCurrentPostCodeStarted()
{
    static const size_t MAX_POST_SIZE = 1;
    auto bus = sdbusplus::bus::new_default();

    try
    {
        /* Get CurrentBootCycleIndex property */
        auto cycleMethod = bus.new_method_call(post::busName, post::path, PROPERTIES_INTERFACE, "Get");
        cycleMethod.append(post::interface, post::requestCurrentCycle);
        std::variant<uint16_t> cycleResult;

        auto cycleReply = bus.call(cycleMethod);
        cycleReply.read(cycleResult);
        uint16_t currentBootCycle = std::get<uint16_t>(cycleResult);


        /* Set the argument for method call */
        auto postMethod = bus.new_method_call(post::busName, post::path, post::interface, post::requestPostCode);
        postMethod.append(currentBootCycle);
        std::vector<std::tuple<uint64_t, std::vector<uint8_t>>> postCodeBuffer;

        /* Get the post code of CurrentBootCycleIndex */
        auto postReply = bus.call(postMethod);
        postReply.read(postCodeBuffer);

        int postCodeBufferSize = postCodeBuffer.size();
        int postCodeIndex = 0;

        if(postCodeBufferSize == 0)
        {
            std::cerr << "Checking current post code size is zero " << std::endl;
            return false;
        }

        // Set command return length to return the last 20 post code.
        if (postCodeBufferSize > MAX_POST_SIZE)
        {
            postCodeIndex = postCodeBufferSize - MAX_POST_SIZE;
        }
        else
        {
            postCodeIndex = 0;
        }

        /* Get post code data */
        std::cerr << "Checking current post code : " <<  std::to_string(std::get<0>(postCodeBuffer[postCodeIndex])) << std::endl;

        if(std::get<0>(postCodeBuffer[postCodeIndex]) != 0xff)
        {
            return true;
        }
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        std::cerr << "isCurrentPostCodeStarted catch error." << std::endl;
        return false;
    }
    return false;
}

void setbiosSelect(int select, bool log)
{
    biosSelectValue = select;
    biosSelectLine.set_value(select);
    iface->set_property(dualBios::biosSelectProp, biosSelectValue);

    std::string selectSensor = "SELECT_BIOS1";
    if(biosSelectValue){
         selectSensor = "SELECT_BIOS2";
    }

    if(log){
        logSELEvent("Switch BIOS SEL Entry",
                    "/xyz/openbmc_project/sensors/system_firmware/" + selectSensor,
                    0x02,0x14,0xFF);
    }
}

void primaryBiosAction(bool enablePowerControl)
{
    std::cerr << "BMC default watchdog timeout. Switch to backup BIOS" << std::endl;
    if(enablePowerControl){
        sethostPowerState(systemBus, "xyz.openbmc_project.State.Host.Transition.Off");
    }

    setbiosSelect(1,true);

    if(enablePowerControl){
        sleep(10);
        sethostPowerState(systemBus, "xyz.openbmc_project.State.Host.Transition.On");
    }

    std::cerr << "Finish switching to backup BIOS" << std::endl;
}

void backupBiosAction()
{
    std::cerr << "Already in backup BIOS. Waiting for FA." << std::endl;
}

void biosAction(uint8_t rootCause, bool enablePowerControl, bool disableEndTimer)
{
    if(disableEndTimer)
    {
        biosEndTimer.cancel(cancelErrorCode);
    }

    if(!isPowerOn())
    {
        if constexpr (DEBUG)
        {
            std::cerr << "BIOS action function only enable when host pwoer on." << std::endl;
        }
        return;
    }

    std::string selectSensor = "BIOS1_BOOT_FAIL";
    if(biosSelectValue){
         selectSensor = "BIOS2_BOOT_FAIL";
    }

    if(rootCause != 0)
    {
        logSELEvent("Switch BIOS SEL Entry",
            "/xyz/openbmc_project/sensors/system_firmware/" + selectSensor,
            0x01,0x14,rootCause);
    }

    if(!biosSelectValue)
    {
        primaryBiosAction(enablePowerControl);
    }
    else
    {
        backupBiosAction();
    }
}

static void postStartHandler()
{
    gpiod::line_event gpioLineEvent = postStartLine.event_read();

    if (gpioLineEvent.event_type == gpiod::line_event::FALLING_EDGE)
    {
        postStartValue = 0;
    }
    else if (gpioLineEvent.event_type == gpiod::line_event::RISING_EDGE)
    {
        postStartValue = 1;
    }

    postStartEvent.async_wait(
        boost::asio::posix::stream_descriptor::wait_read,
        [&](const boost::system::error_code ec) {
            if (ec)
            {
                 std::cerr << "post start handler error: " << ec.message();
                return;
            }
            postStartHandler();
        });
}

bool biosStartGpioInit(const std::string& name)
{
    postStartLine = gpiod::find_line(name);
    if (!postStartLine)
    {
        std::cerr << "Failed to find the " << name << " line" << std::endl;
        return false;
    }
    try
    {
        postStartLine.request({"dual-bios-handler", gpiod::line_request::EVENT_BOTH_EDGES});
    }
    catch (std::exception&)
    {
        std::cerr << "Failed to request events for " << name << std::endl;
        return false;
    }

    int gpioLineFd = postStartLine.event_get_fd();
    if (gpioLineFd < 0)
    {
        std::cerr << "Failed to name " << name << " fd" << std::endl;
        return false;
    }

    postStartEvent.assign(gpioLineFd);

    postStartEvent.async_wait(boost::asio::posix::stream_descriptor::wait_read,
        [&](const boost::system::error_code ec) {
            if (ec)
            {
                std::cerr << name << " fd handler error: " << ec.message();
                return;
            }
            postStartHandler();
    });

    return true;
}

bool biosSelectInit(const std::string& name)
{
    biosSelectLine = gpiod::find_line(name);
    if (!biosSelectLine)
    {
        std::cerr << "Failed to find the " << name << " line" << std::endl;
        return false;
    }

    uint32_t registerBuffer = 0;
    uint32_t GpioI7Flag = 0x80;
    if (read_register(0x1E780070, &registerBuffer) < 0)
    {
        std::cerr<<"Failed to read GPIO I/J/K/L register \n";
        return false;
    }

    if(registerBuffer & GpioI7Flag)
    {
        biosSelectValue = 1;
    }
    else
    {
        biosSelectValue = 0;
    }

    iface->set_property(dualBios::biosSelectProp, biosSelectValue);

    try
    {
        biosSelectLine.request({"dual-bios-handler", gpiod::line_request::DIRECTION_OUTPUT, 0});
    }
    catch (std::exception&)
    {
        std::cerr << "Failed to request output for " << name << std::endl;
        return false;
    }

    biosSelectLine.set_value(biosSelectValue);

    return true;
}

int setCurrentBootBios(int selectBios, bool log)
{
    std::cerr << "Switch to BIOS : " << std::to_string(selectBios) << std::endl;
    setbiosSelect(selectBios,log);
    return selectBios;
}

int enableBiosStartTimer(bool disablePreTimer, bool disableEndTimer)
{
    if constexpr (DEBUG)
    {
        std::cerr << "Enable bios related timer" << std::endl;
        biosStartTime = std::chrono::high_resolution_clock::now();
    }

    if(disablePreTimer)
    {
        biosStartTimer.cancel(cancelErrorCode);
    }

    if(disableEndTimer)
    {
        biosEndTimer.cancel(cancelErrorCode);
    }

    if(disablePreTimer)
    {
        biosStartTimer.expires_after(std::chrono::milliseconds(biosStartTimeMs));
        biosStartTimer.async_wait([&](const boost::system::error_code ec) {
            if constexpr (DEBUG)
            {
                std::cerr << "Enable bios started timer" << std::endl;
            }
            if (ec)
            {
                if (ec != boost::asio::error::operation_aborted)
                {
                    std::cerr << "BIOS started async_wait failed: " << ec.message();
                }
                return;
            }

            if(isPowerOn()){
                std::cerr << "Current using BIOS booting flash : " << std::to_string(biosSelectValue) << std::endl;
                biosAction(POST_START,true,true);
            }
        });
    }

    if(disableEndTimer)
    {
        biosEndTimer.expires_after(std::chrono::milliseconds(biosEndTimesMs));
        biosEndTimer.async_wait([&](const boost::system::error_code ec) {
            if constexpr (DEBUG)
            {
                std::cerr << "Enable bios end timer" << std::endl;
            }
            if (ec)
            {
                if (ec != boost::asio::error::operation_aborted)
                {
                    std::cerr << "BIOS end async_wait failed: " << ec.message();
                }
                return;
            }

            if constexpr (DEBUG)
            {
                std::cerr << "Post Durnint time : " 
                        << std::chrono::duration<double, std::milli>(
                            std::chrono::high_resolution_clock::now()-biosStartTime).count()
                        << std::endl;
            }

            if(!isPostEnd() && isPowerOn()){
                std::cerr << "Current using BIOS booting flash : " << std::to_string(biosSelectValue) << std::endl;
                biosAction(POST_END,true,false);
            }
        });
    }

    return 0;
}

void dbusServiceInitialize()
{
    systemBus->request_name(dualBios::busName);
    sdbusplus::asio::object_server objServer(systemBus);
    iface=objServer.add_interface(dualBios::path,dualBios::interface);
    iface->register_method(dualBios::setCurrentBootBiosMethod, setCurrentBootBios);
    iface->register_method(dualBios::enableBiosStartTimerMethod, enableBiosStartTimer);
    iface->register_method(dualBios::biosActionMethod, biosAction);
    iface->register_property(dualBios::biosSelectProp, biosSelectValue,
                             sdbusplus::asio::PropertyPermission::readWrite);
    iface->initialize();
}

inline static sdbusplus::bus::match::match
    setWatchdogHandler(std::shared_ptr<sdbusplus::asio::connection>& systemBus)
{
    auto watchdogMatcherCallback = [&](sdbusplus::message::message &msg)
    {
            if constexpr (DEBUG)
            {
                std::cerr << "Enable setting watchdog handler" << std::endl;
            }
            uint32_t value;
            msg.read(value);

            std::cerr << "Current using BIOS booting flash : " << std::to_string(biosSelectValue) << std::endl;

            if(value == 0x01)
            {
                biosAction(FRB2_TIMEOUT,false,true);
            }
    };
    sdbusplus::bus::match::match watchdogMatcher(
        static_cast<sdbusplus::bus::bus &>(*systemBus),
        sdbusplus::bus::match::rules::type::signal() +
        sdbusplus::bus::match::rules::member(std::string("IpmiWatchdogTimeout")),
        std::move(watchdogMatcherCallback));
    return watchdogMatcher;
}

inline static sdbusplus::bus::match::match
    setLpcResetHandler(std::shared_ptr<sdbusplus::asio::connection>& systemBus)
{
    auto lpcResetMatcherCallback = [&](sdbusplus::message::message &msg)
    {
        if constexpr (DEBUG)
        {
            std::cerr << "Enable lpc reset handler" << std::endl;
        }
        uint32_t value;
        msg.read(value);

        if(value == 0x01)
        {
            enableBiosStartTimer(true,true);
            amdBistState = false;
        }
    };
    sdbusplus::bus::match::match lpcResetMatcher(
        static_cast<sdbusplus::bus::bus &>(*systemBus),
        sdbusplus::bus::match::rules::type::signal() +
        sdbusplus::bus::match::rules::member(std::string("LpcReset")),
        std::move(lpcResetMatcherCallback));
    return lpcResetMatcher;
}

inline static sdbusplus::bus::match::match
    setPostCodeHandler(std::shared_ptr<sdbusplus::asio::connection>& systemBus)
{
    auto postCodeMatcherCallback = [&](sdbusplus::message::message &msg)
    {
        if constexpr (DEBUG)
        {
            std::cerr << "Enable post code handler" << std::endl;
        }
        uint64_t value;
        msg.read(value);

        if(value != 0xFF)
        {
            biosStartTimer.cancel(cancelErrorCode);
        }

        if constexpr (DEBUG)
        {
            char buffer[10];
            sprintf(buffer, "0x%X", value);
            std::string val = buffer;
            std::cerr << "Current post code: " << val << std::endl;
        }

        if(value == 0xE336)
        {
            uint32_t amdMbistResponse = 0x01;
            getDbusProperty<uint32_t>(systemBus,amdMbistStatus::busName,
                                      amdMbistStatus::path, amdMbistStatus::interface,
                                      amdMbistStatus::requestMbistStatus, amdMbistResponse);
            if(amdMbistResponse == 0x00)
            {
                std::cerr << "AMD MBIST starting" << std::endl;
                logSELEvent("AMD MBIST starting SEL Entry",
                    "/xyz/openbmc_project/sensors/system_event/MBIST_START",
                    0x00,0x00,0xFF);
                amdBistState = true;
                biosEndTimer.cancel(cancelErrorCode);
            }
        }

        if(amdBistState && value == 0xE337)
        {
            std::cerr << "AMD MBIST completed" << std::endl;
            logSELEvent("AMD MBIST completed SEL Entry",
                "/xyz/openbmc_project/sensors/system_event/MBIST_COMPLETE",
                0x00,0x00,0xFF);
            amdBistState = false;
            enableBiosStartTimer(false,true);
            setDbusProperty<uint32_t>(systemBus,amdMbistStatus::busName,
                            amdMbistStatus::path, amdMbistStatus::interface,
                            amdMbistStatus::requestMbistStatus, 1);
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
    setPostEndHandler(std::shared_ptr<sdbusplus::asio::connection>& systemBus)
{
    auto postEndMatcherCallback = [&](sdbusplus::message::message &msg)
    {
        if constexpr (DEBUG)
        {
            std::cerr << "Enable post end handler" << std::endl;
        }
        bool value;
        msg.read(value);

        if(value)
        {
            if constexpr (DEBUG)
            {
                std::cerr << "Disable post end timer" << std::endl;
            }
            biosEndTimer.cancel(cancelErrorCode);
        }
    };
    sdbusplus::bus::match::match postEndMatcher(
        static_cast<sdbusplus::bus::bus &>(*systemBus),
        sdbusplus::bus::match::rules::type::signal() +
        sdbusplus::bus::match::rules::member(std::string("EndOfPost")),
        std::move(postEndMatcherCallback));
    return postEndMatcher;
}

int main()
{
    std::cerr << "MCT dual BIOS handler start to monitor." << std::endl;

    systemBus = std::make_shared<sdbusplus::asio::connection>(io);

    dbusServiceInitialize();

    biosSelectInit(biosSelectName);
    biosStartGpioInit(postStartName);

    sdbusplus::bus::match::match watchdogHandler = setWatchdogHandler(systemBus);
    sdbusplus::bus::match::match lpcResetHandler = setLpcResetHandler(systemBus);
    sdbusplus::bus::match::match postCodeHandler = setPostCodeHandler(systemBus);
    sdbusplus::bus::match::match postEndHandler = setPostEndHandler(systemBus);

    io.run();

    return 0;
}