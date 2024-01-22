#include "PowerCapping.hpp"

PowerCapping::PowerCapping(
    boost::asio::io_service& io, int pollingMs,
    sdbusplus::asio::object_server& objectServer,
    std::shared_ptr<sdbusplus::asio::connection>& connection) :
    waitTimer(io),
    pollingMs(pollingMs), objServer(objectServer), conn(connection)
{
    // Initialize psuQueue
    psuQueueInit(DEFAULT_FIFO_BUFFER_SIZE);

    std::string strAmdCpuPowerCapPath = amdCpuPowerCapPath;
    std::string strPowerCapName = powerCapName;
    fs::path namePath = strAmdCpuPowerCapPath + strPowerCapName;

    std::ifstream pwrCapStatusFile(namePath);
    if (pwrCapStatusFile.is_open())
    {
        auto data = nlohmann::json::parse(pwrCapStatusFile, nullptr, false);
        if (!data.is_discarded())
        {
            isPowerCapEnabled = data.value("isPowerCapEnabled", "OFF");
            systemPowerLimit = data.value("systemPowerLimit", 0);
        }
        else
        {
            fprintf(stderr, "[%s] Power Cap Status JSON parser failure.\n",
                    __func__);
        }
        pwrCapStatusFile.close();
    }
    else
    {
        fprintf(stderr, "[%s] Power Cap Status File isn't exist.\n", __func__);

        auto dir = fs::path(amdCpuPowerCapPath);
        fs::create_directories(dir);

        std::ofstream file(namePath);
        nlohmann::json outJson;
        outJson["isPowerCapEnabled"] = "OFF";
        outJson["systemPowerLimit"] = 0;
        file << outJson.dump(4);
        if (file.fail())
        {
            fprintf(stderr,
                    "[%s] Failed to write the setting to the conf file\n",
                    __func__);
        }
        file.close();

        isPowerCapEnabled = "OFF";
        systemPowerLimit = 0;
    }

    auto methodInterface =
        objServer.add_interface(objPathName, powerControlIntfName);

    methodInterface->register_method("GetCPUPowerLimit", [this](int sockId) {
        return getCpuPowerLimit(sockId);
    });
    methodInterface->register_method(
        "GetCPUPowerConsumption",
        [this](int sockId) { return getCpuPowerConsumption(sockId); });
    methodInterface->register_method("SetSystemPowerLimit",
                                     [this](int sockId, uint32_t power) {
                                         return setParameters(sockId, power);
                                     });
    methodInterface->register_method(
        "SetDefaultCPUPowerLimit",
        [this](int sockId) { return stopSystemPowerCapping(sockId); });

    if (!methodInterface->initialize())
    {
        fprintf(stderr, "[%s] Error initializing method interface\n", __func__);
    }

    if (!operationalInterface)
    {
        operationalInterface =
            std::make_shared<sdbusplus::asio::dbus_interface>(
                conn, objPathName, operationalIntfName);
        operationalInterface->register_property("isPowerCapEnabled",
                                                isPowerCapEnabled);

        if (!operationalInterface->initialize())
        {
            fprintf(stderr,
                    "[%s] Error initializing operational status interface\n",
                    __func__);
        }
    }

    if (!parameterInterface)
    {
        parameterInterface = std::make_shared<sdbusplus::asio::dbus_interface>(
            conn, objPathName, parameterIntfName);

        parameterInterface->register_property(
            "debugModeEnabled", debugModeEnabled,
            [this](const bool propIn, bool& old) {
                if (propIn == old)
                {
                    return 1;
                }
                old = propIn;
                debugModeEnabled = propIn;
                return 1;
            });

        parameterInterface->register_property(
            "FastReleaseTimeout", fastReleaseTimeout,
            [this](const uint8_t propIn, uint8_t& old) {
                if (propIn == old)
                {
                    return 1;
                }
                old = propIn;
                fastReleaseTimeout = propIn;
                return 1;
            });

        parameterInterface->register_property(
            "SlowReleaseTimeout", slowReleaseTimeout,
            [this](const uint8_t propIn, uint8_t& old) {
                if (propIn == old)
                {
                    return 1;
                }
                old = propIn;
                slowReleaseTimeout = propIn;
                return 1;
            });

        parameterInterface->register_property("systemPowerLimit",
                                              systemPowerLimit);

        if (!parameterInterface->initialize())
        {
            fprintf(stderr, "[%s] Error initializing parameter interface\n",
                    __func__);
        }
    }

    auto powerStatusMatcherCallback = [this](sdbusplus::message::message& msg) {
        boost::container::flat_map<std::string, std::variant<bool, int>>
            propertiesChanged;
        std::string objName;

        if (msg.is_method_error())
        {
            return;
        }

        if (isPowerCapEnabled == "OFF")
        {
            return;
        }

        msg.read(objName, propertiesChanged);
        auto findPlatformRstStatus = propertiesChanged.find("platformreset");
        auto findPostCompleteStatus = propertiesChanged.find("postcomplete");
        if (findPlatformRstStatus != propertiesChanged.end())
        {
            bool* platfromRstStatus =
                std::get_if<bool>(&findPlatformRstStatus->second);
            if ((*platfromRstStatus == true) && (isPowerCapEnabled == "ON"))
            {
                waitTimer.cancel();
                if (operationalInterface)
                {
                    isPowerCapEnabled = "PAUSE";
                    operationalInterface->set_property("isPowerCapEnabled",
                                                       isPowerCapEnabled);
                }
                if (debugModeEnabled)
                {
                    std::cerr << "[Debug Mode]"
                              << "Pause to cap CPU power\n";
                }
            }
        }
        else if (findPostCompleteStatus != propertiesChanged.end())
        {
            int* postCompleteStatus =
                std::get_if<int>(&findPostCompleteStatus->second);
            if ((*postCompleteStatus == 0) && (!isCrashdump()))
            {
                startSystemPowerCapping();
                if (operationalInterface)
                {
                    isPowerCapEnabled = "ON";
                    operationalInterface->set_property("isPowerCapEnabled",
                                                       isPowerCapEnabled);
                }
                if (debugModeEnabled)
                {
                    std::cerr << "[Debug Mode]"
                              << "Restart to cap CPU power\n";
                }
            }
        }
        return;
    };

    static sdbusplus::bus::match::match powerStatusMatcher(
        static_cast<sdbusplus::bus::bus&>(*conn),
        sdbusplus::bus::match::rules::propertiesChanged(
            "/org/openbmc/control/power0", "org.openbmc.control.Power"),
        std::move(powerStatusMatcherCallback));

    static sdbusplus::bus::match::match postCompleteMatcher(
        static_cast<sdbusplus::bus::bus&>(*conn),
        sdbusplus::bus::match::rules::propertiesChanged(
            "/org/openbmc/control/power0", "org.openbmc.control.PostComplete"),
        std::move(powerStatusMatcherCallback));

    /* TODO: Register a matcher to listen the crashdump state.
       The action of callback function is
       Discard this event if the isPowerCapEnabled is "OFF".
       When crashdump state is true and the isPowerCapEnabled is "ON"
         (1) Stop timer to stop capping power.
         (2) Change parameter "isPowerCapEnabled" to "PAUSE" and set property "isPowerCapEnabled" on dbus.
         (3) Print debug message if the parameter "debugModeEnabled" is true.
       When crashdump state is false, host is power-on and BIOS post complete
         (1) Start system power capping.
         (2) Change parameter "isPowerCapEnabled" to "ON" and set property "isPowerCapEnabled" on dbus.
         (3) Print debug message if the parameter "debugModeEnabled" is true.
     */

    int ret = 0;
    int retryStartPwrCapTime = 0, retrySetDefaultPwrLimitTime = 0;
    int maxStartPwrCapRetryTime = 3, maxSetDefaultPwrLimitTime = 5;
    if (isPowerCapEnabled == "ON")
    {
        ret = setParameters(cpuSocketId, systemPowerLimit);
        while (retryStartPwrCapTime <= maxStartPwrCapRetryTime)
        {
            ret = setParameters(cpuSocketId, systemPowerLimit);
            if (ret == 0)
            {
                    retryStartPwrCapTime = 0;
                    break;
            }
            if (retryStartPwrCapTime == maxStartPwrCapRetryTime)
            {
                    fprintf(stderr, "[%s] Failed to start to cap power\n",
                            __func__);
                    retryStartPwrCapTime = 0;
                    break;
            }
            ++retryStartPwrCapTime;
            usleep(200000); //sleep 200ms
        }
    }
    else
    {
        if (isPgoodOn() && isPostComplete())
        {
            while (retrySetDefaultPwrLimitTime <= maxSetDefaultPwrLimitTime)
            {
                ret = stopSystemPowerCapping(cpuSocketId);
                if (ret == 0)
                {
                        retrySetDefaultPwrLimitTime = 0;
                        break;
                }
                if (retrySetDefaultPwrLimitTime == maxSetDefaultPwrLimitTime)
                {
                        fprintf(stderr, "[%s] Failed to set default power limit\n",
                                __func__);
                        retrySetDefaultPwrLimitTime = 0;
                        break;
                }
                ++retrySetDefaultPwrLimitTime;
                usleep(200000); //sleep 200ms
            }
        }
    }
}

PowerCapping::~PowerCapping()
{
    waitTimer.cancel();
    free(psuQueue);
    if (operationalInterface)
    {
        objServer.remove_interface(operationalInterface);
    }
    if (parameterInterface)
    {
        objServer.remove_interface(parameterInterface);
    }
}

bool PowerCapping::isCrashdump()
{
    /* TODO: Return Crashdump State
       The retrun value is as true if Crashdump is enable.
       Or the return value is false.
    */
    return false;
}

void PowerCapping::psuQueueInit(int size)
{
    psuQueue = (QUEUE*)malloc(sizeof(QUEUE));
    psuQueue->front = NULL;
    psuQueue->rear = NULL;
    psuQueue->size = size;
    psuQueue->entries = 0;
}

int PowerCapping::psuQueueDelNode()
{
    int ret;
    if ((psuQueue->front == NULL) || (psuQueue->rear == NULL))
    {
        return 0;
    }

    ret = psuQueue->front->data;
    psuQueue->front = psuQueue->front->prev;

    if (psuQueue->front == NULL)
    {
        psuQueue->rear = NULL;
    }
    else
    {
        psuQueue->front->next = NULL;
    }

    psuQueue->entries--;
    return ret;
}

void PowerCapping::psuQueueAddNode(double data)
{
    if (psuQueue->entries >= psuQueue->size)
    {
        psuQueueDelNode();
    }

    QNODE* newnode = (QNODE*)malloc(sizeof(QNODE));
    newnode->data = data;
    newnode->next = psuQueue->rear;
    newnode->prev = NULL;

    if (newnode->next != NULL)
    {
        newnode->next->prev = newnode;
    }

    psuQueue->rear = newnode;

    if (psuQueue->front == NULL)
    {
        psuQueue->front = newnode;
    }

    psuQueue->entries++;
}

int PowerCapping::readPsuPin()
{
    // Get PSU PIN from dbus
    std::variant<double> value;

    auto bus = sdbusplus::bus::new_system();
    auto method =
        bus.new_method_call("xyz.openbmc_project.PSUSensor",
                            "/xyz/openbmc_project/sensors/power/PSU_PIN",
                            "org.freedesktop.DBus.Properties", "Get");

    method.append("xyz.openbmc_project.Sensor.Value", "Value");
    try
    {
        auto reply = bus.call(method);
        reply.read(value);
        // Convert to mWatts from Watts
        psuPIN = std::get<double>(value) * 1000;
    }
    catch (sdbusplus::exception_t& e)
    {
        std::cerr << "Error to get PSU PIN sensor value, " << e.what() << "\n";
        return -1;
    }
    return 0;
}

/* Calculate the moving average of the last 5 readings */
void PowerCapping::calcMovingAvg()
{
    psuQueueAddNode(psuPIN);
    double sum = 0;
    int counter = 0;
    struct qnode* current;

    // No need to calculate the moving average
    if (psuQueue->entries < DEFAULT_FIFO_BUFFER_SIZE)
    {
        psuPINMovingAvg = 0;
    }

    current = psuQueue->rear;
    while (current != NULL)
    {
        sum += current->data;
        current = current->next;
        counter++;
    }

    if (counter != psuQueue->entries)
    {
        std::cerr << "Fail to calculate the moving average of the last 5 PSU "
                     "PIN readings\n";
        psuPINMovingAvg = 0;
    }
    else
    {
        psuPINMovingAvg = sum / counter;
    }
}

/* This function is essentially where the powercapping algorithm is implemented
 */
void PowerCapping::checkAndCapPower()
{
    static int time = 0, capTimeIndex = 0;

    static int counter = 0;
    static int fastReleaseTimeIndex = 0, slowReleaseTimeIndex = 0;

    if (psuPINMovingAvg > systemPowerLimit)
    { // Apply Power Capping
        fastReleaseTimeIndex = 0;
        slowReleaseTimeIndex = 0;

        if (cpuPowerLimit <= MIN_POWER_LIMIT)
        {
            return;
        }
        if (((counter == 0) || (counter == 2) || (counter == 4) ||
             (counter == 6)) &&
            (psuPIN > systemPowerLimit))
        {
            if (psuPINMovingAvg > (systemPowerLimit + FAST_APPLY_POWER_CAP))
            { // Fast Apply Power Capping
                if (debugModeEnabled)
                {
                    std::cerr << "[Debug Mode]"
                              << "Fast Apply Power Capping - 20W\n";
                }
                counter += (7 - counter);
                cpuPowerLimit -= 4 * RATE_OF_DECLINE_IN_MWATTS;
                capTimeIndex += 4;
                if (cpuPowerLimit < MIN_POWER_LIMIT)
                {
                    cpuPowerLimit = MIN_POWER_LIMIT;
                }
                setCpuPowerLimit(cpuSocketId, cpuPowerLimit);
            }
            else
            { // Slow Apply Power Capping
                if (debugModeEnabled)
                {
                    std::cerr << "[Debug Mode]"
                              << "Slow Apply Power Capping - 5W\n";
                }
                cpuPowerLimit -= RATE_OF_DECLINE_IN_MWATTS;
                setCpuPowerLimit(cpuSocketId, cpuPowerLimit);
            }
        }
        counter++;
        capTimeIndex++;
        if (capTimeIndex > 16)
        {
            capTimeIndex = 0;
            counter = 0;
        }
    }
    else
    { // Release Power Capping
        counter = 0;
        capTimeIndex = 0;

        if (cpuPowerLimit >= MAX_POWER_LIMIT)
        {
            return;
        }
        if (psuPIN < (systemPowerLimit - SLOW_RELEASE_POWER_CAP))
        {
            slowReleaseTimeIndex++;

            if (psuPIN < (systemPowerLimit - FAST_RELEASE_POWER_CAP))
            {
                fastReleaseTimeIndex++;
            }
            else
            {
                fastReleaseTimeIndex = 0;
            }

            if (fastReleaseTimeIndex > fastReleaseTimeout)
            { // Fast Release Power Capping
                if (debugModeEnabled)
                {
                    std::cerr << "[Debug Mode]"
                              << "Fast Release Power Capping - 20W\n";
                }
                if (cpuPowerLimit <
                    (MAX_POWER_LIMIT - (4 * RATE_OF_DECLINE_IN_MWATTS)))
                {
                    cpuPowerLimit += 4 * RATE_OF_DECLINE_IN_MWATTS;
                }
                else
                {
                    cpuPowerLimit = MAX_POWER_LIMIT;
                }

                setCpuPowerLimit(cpuSocketId, cpuPowerLimit);
                fastReleaseTimeIndex = 0;
                slowReleaseTimeIndex = 0;
            }
            else if (slowReleaseTimeIndex > slowReleaseTimeout)
            { // Slow Release Power Capping
                if (debugModeEnabled)
                {
                    std::cerr << "[Debug Mode]"
                              << "Slow Release Power Capping - 5W\n";
                }
                if (cpuPowerLimit != MAX_POWER_LIMIT)
                {
                    cpuPowerLimit += RATE_OF_DECLINE_IN_MWATTS;
                }
                setCpuPowerLimit(cpuSocketId, cpuPowerLimit);
                fastReleaseTimeIndex = 0;
                slowReleaseTimeIndex = 0;
            }
        }
        else
        {
            fastReleaseTimeIndex = 0;
            slowReleaseTimeIndex = 0;
        }
    }
    time++;
    if (time < 0)
    {
        time = 0;
    }
    if (debugModeEnabled)
    {
        std::cerr << "[Debug Mode]"
                  << "PSU PIN[" << psuPINMovingAvg << "] System Power Limit["
                  << systemPowerLimit << "]\n";
        std::cerr << "[Debug Mode]"
                  << "Cap Power Time[" << time << "]\n";
        std::cerr << "[Debug Mode]"
                  << "Apply Capping Index[" << capTimeIndex << "]\n";
        std::cerr << "[Debug Mode]"
                  << "Fast Release Index[" << fastReleaseTimeIndex << "] "
                  << "Slow Release Index[" << slowReleaseTimeIndex << "]\n";
    }
}

int PowerCapping::setParameters(int sockId, uint32_t power)
{

    auto [ret, cpuPowerLimit] = getCpuPowerLimit(sockId);
    if (ret != 0)
    {
        return ret;
    }

    cpuSocketId = sockId;
    systemPowerLimit = power;

    if (parameterInterface)
    {
        parameterInterface->set_property("systemPowerLimit", systemPowerLimit);
    }

    if (operationalInterface)
    {
        isPowerCapEnabled = "ON";
        operationalInterface->set_property("isPowerCapEnabled",
                                           isPowerCapEnabled);
    }
    if (debugModeEnabled)
    {
        std::cerr << "[Debug Mode]"
                  << "Start to cap CPU power\n";
    }
    return startSystemPowerCapping();
}

int PowerCapping::startSystemPowerCapping(void)
{
    int ret;
    std::string message;
    std::vector<uint8_t> eventData(3, 0xFF);

    ret = readPsuPin();
    if (ret != 0)
    {
        return ret;
    }
    calcMovingAvg();

    std::tie(ret, cpuPowerLimit) = getCpuPowerLimit(cpuSocketId);
    if (ret != 0)
    {
        return ret;
    }

    checkAndCapPower();

    if (isAsserted && (cpuPowerLimit > POWER_LIMIT_THRESHOLD))
    {
        isAsserted = false;
        message = "Server Power Cap - Status Dessert";
        eventData[0] = static_cast<uint8_t>(0x0);
        if (ipmiSelAdd(message,
                       "/xyz/openbmc_project/sensors/discrete/Server_Power_Cap",
                       eventData, true) != 0)
        {
            std::cerr << "Fail to add SEL:" << message << "\n";
        }
    }
    else if ((!isAsserted) && (cpuPowerLimit <= POWER_LIMIT_THRESHOLD))
    {
        isAsserted = true;
        message = "Server Power Cap - Status Assert";
        eventData[0] = static_cast<uint8_t>(0x01);
        if (ipmiSelAdd(message,
                       "/xyz/openbmc_project/sensors/discrete/Server_Power_Cap",
                       eventData, true) != 0)
        {
            std::cerr << "Fail to add SEL:" << message << "\n";
        }
    }

    waitTimer.expires_from_now(boost::asio::chrono::milliseconds(pollingMs));
    waitTimer.async_wait([this](const boost::system::error_code& ec) {
        if (ec == boost::asio::error::operation_aborted)
        {
            return 0; // we're being canceled
        }
        this->startSystemPowerCapping();
    });
    return 0;
}

int PowerCapping::stopSystemPowerCapping(int sockId)
{
    /* Set Default Power Limit into AMD CPU
       Cancel timer to stop power capping
    */
    std::string message;
    std::vector<uint8_t> eventData(3, 0xFF);
    int ret = setDefaultCpuPowerLimit(sockId);
    if (ret != 0)
    {
        return ret;
    }

    if (isAsserted)
    {
        isAsserted = false;
        message = "Server Power Cap - Status Deassert";
        eventData[0] = static_cast<uint8_t>(0x00);
        if (ipmiSelAdd(message,
                       "/xyz/openbmc_project/sensors/discrete/Server_Power_Cap",
                       eventData, true) != 0)
        {
            std::cerr << "Fail to add SEL:" << message << "\n";
        }
    }
    waitTimer.cancel();
    if (operationalInterface)
    {
        isPowerCapEnabled = "OFF";
        operationalInterface->set_property("isPowerCapEnabled",
                                           isPowerCapEnabled);
    }

    systemPowerLimit = 0;
    if (parameterInterface)
    {
        parameterInterface->set_property("systemPowerLimit", systemPowerLimit);
    }

    if (debugModeEnabled)
    {
        std::cerr << "[Debug Mode]"
                  << "Set Default Power Limit into AMD CPU and stop to do "
                     "power capping\n";
    }
    return ret;
}

std::tuple<int, uint32_t> PowerCapping::getCpuPowerLimit(int sockId)
{
    oob_status_t ret;
    uint32_t power = 0;
    if (!esmiMutex.try_lock_for(boost::asio::chrono::milliseconds(esmiMutexTimout)))
    {
        if (debugModeEnabled)
        {
            std::cerr << "[Debug Mode]"
                      << "esmiMutex is still lock\n";
        }
        esmiMutex.unlock();
        esmiMutex.lock();
    }
    esmi_oob_init(sockId);

    /* Get the PowerLimit for a given socket index */
    ret = read_socket_power_limit(sockId, &power);
    if (ret != OOB_SUCCESS)
    {
        fprintf(stderr, "Failed: to get socket[%d] powerlimit, Err[%d]: %s\n",
                sockId, ret, esmi_get_err_msg(ret));
    }
    esmi_oob_exit();
    esmiMutex.unlock();

    if (debugModeEnabled)
    {
        std::cerr << "[Debug Mode]"
                  << "Get SockID[" << sockId << "] CPU Power Limit[" << power
                  << "]\n";
    }
    return std::make_tuple(ret, power);
}

std::tuple<int, uint32_t> PowerCapping::getCpuPowerConsumption(int sockId)
{
    oob_status_t ret;
    uint32_t power = 0;

    if (!esmiMutex.try_lock_for(boost::asio::chrono::milliseconds(esmiMutexTimout)))
    {
        if (debugModeEnabled)
        {
            std::cerr << "[Debug Mode]"
                      << "esmiMutex is still lock\n";
        }
        esmiMutex.unlock();
        esmiMutex.lock();
    }
    esmi_oob_init(sockId);

    ret = read_socket_power(sockId, &power);
    if (ret != OOB_SUCCESS)
    {
        fprintf(stderr, "Failed: to get socket[%d] power, Err[%d]: %s\n",
                sockId, ret, esmi_get_err_msg(ret));
    }
    esmi_oob_exit();
    esmiMutex.unlock();

    if (debugModeEnabled)
    {
        std::cerr << "[Debug Mode]"
                  << "Get SockID[" << sockId << "] CPU Power Consumption["
                  << power << "]\n";
    }
    return std::make_tuple(ret, power);
}

int PowerCapping::setCpuPowerLimit(int sockId, uint32_t power)
{
    oob_status_t ret;
    uint32_t max_power = 0;

    if (!esmiMutex.try_lock_for(boost::asio::chrono::milliseconds(esmiMutexTimout)))
    {
        if (debugModeEnabled)
        {
            std::cerr << "[Debug Mode]"
                      << "esmiMutex is still lock\n";
        }
        esmiMutex.unlock();
        esmiMutex.lock();
    }
    esmi_oob_init(sockId);

    ret = read_max_socket_power_limit(sockId, &max_power);
    if ((ret == OOB_SUCCESS) && (power > max_power))
    {
        fprintf(stderr,
                "Input power is more than max limit,"
                " So It set's to default max %.3f Watts\n",
                (double)max_power / 1000);
        power = max_power;
    }

    ret = write_socket_power_limit(sockId, power);
    if (ret != OOB_SUCCESS)
    {
        fprintf(stderr, "Failed: to set socket[%d] power_limit, Err[%d]: %s\n",
                sockId, ret, esmi_get_err_msg(ret));
    }
    esmi_oob_exit();
    esmiMutex.unlock();

    if (debugModeEnabled)
    {
        std::cerr << "[Debug Mode]"
                  << "Set SockID[" << sockId << "] CPU Power Limit[" << power
                  << "]\n";
    }
    return ret;
}

int PowerCapping::setDefaultCpuPowerLimit(int sockId)
{
    oob_status_t ret;
    uint32_t max_power;

    if (!esmiMutex.try_lock_for(boost::asio::chrono::milliseconds(esmiMutexTimout)))
    {
        if (debugModeEnabled)
        {
            std::cerr << "[Debug Mode]"
                      << "esmiMutex is still lock\n";
        }
        esmiMutex.unlock();
        esmiMutex.lock();
    }
    esmi_oob_init(sockId);

    ret = read_max_socket_power_limit(sockId, &max_power);
    if (ret != OOB_SUCCESS)
    {
        fprintf(
            stderr,
            "Failed: to get socket[%d] default max power limit, Err[%d]: %s\n",
            sockId, ret, esmi_get_err_msg(ret));
        goto error_handler;
    }

    ret = write_socket_power_limit(sockId, max_power);
    if (ret != OOB_SUCCESS)
    {
        fprintf(stderr,
                "Failed: to set socket[%d] default power_limit, Err[%d]: %s\n",
                sockId, ret, esmi_get_err_msg(ret));
        goto error_handler;
    }
    esmi_oob_exit();
    esmiMutex.unlock();

    if (debugModeEnabled)
    {
        std::cerr << "[Debug Mode]"
                  << "Set SockID[" << sockId << "] Default CPU Power Limit["
                  << max_power << "]\n";
    }
    return OOB_SUCCESS;

error_handler:
    esmi_oob_exit();
    esmiMutex.unlock();
    return ret;
}
