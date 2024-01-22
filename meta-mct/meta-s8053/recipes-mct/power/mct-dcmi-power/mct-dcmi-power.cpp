/* Copyright 2020 MCT
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

#include "mct-dcmi-power.hpp"
#include "utils.hpp"
#include <numeric>


static constexpr bool DEBUG = false;

//===========================================================================================
int PowerHandler::updateMovingAverage(void)
{
    double max=0;
    double min=0;
	double average=0;
    Power currentPower;
    
    if(powerStore.powerPath == "")
    {
#ifdef SYSFS_MODE
        powerStore.powerPath = findPowerPath();
#endif
    
#ifdef DBUS_MODE
        powerStore.powerPath = POWER_PATH;
#endif
        if(powerStore.powerPath == "")
        {           
            return -1; 
        }
    }
#ifdef SYSFS_MODE
    currentPower.value = readPower(powerStore.powerPath);
#endif

#ifdef DBUS_MODE
    currentPower.value = readDbusPower(powerStore.powerPath);
#endif
    
    if(currentPower.value > 0 )
    {
        //auto cb = powerStore.cb;
        psuPIN = currentPower.value * mW;
        powerStore.cb.push_back(currentPower.value);
        auto cb = powerStore.cb;
        if (debugModeEnabled){ 
            std::cerr << "[Debug Mode] update moving average\n";        
            for (int i : cb)
                std::cerr<<i<<"\t";
            std::cerr<<std::endl<<std::endl;
        }

        average = std::accumulate(cb.begin(), cb.end(), 0.0)/cb.size();
		psuPINMovingAvg = average * mW;
        max = *std::max_element(cb.begin(),cb.end());
        min = *std::min_element(cb.begin(),cb.end());

        if (debugModeEnabled)
        {
            std::cerr << "average_: " << powerStore.average << "\n";
            std::cerr << "max_: " << powerStore.max << "\n";
            std::cerr << "min_: " << powerStore.min << "\n";
        }
           
        updatePowerValue(PERIOD_AVERAGE_PROP,average,powerStore.average);
        updatePowerValue(PERIOD_MAX_PROP,max,powerStore.max);
        updatePowerValue(PERIOD_MIN_PROP,min,powerStore.min);
   }
   return 0;
}
//===========================================================================================

/* This function is essentially where the powercapping algorithm is implemented
 */
void PowerHandler::checkAndCapPower()
{
    static int time = 0, capTimeIndex = 0;

    static int counter = 0;
    static int fastReleaseTimeIndex = 0, slowReleaseTimeIndex = 0;
    std::vector<uint8_t> powerlimitEventData{3, 0xff};


    if (debugModeEnabled)
        {
            std::cerr << "psuPINMovingAvg: " << psuPINMovingAvg << "\n";
            std::cerr << "powerStore.powerCap: " << powerStore.powerCap << "\n";
            std::cerr << "powerStore.cb.size(): " << powerStore.cb.size() << "\n";
        }
           

    if ((psuPINMovingAvg > powerStore.powerCap) && powerStore.cb.size() >= 5)
    { // Apply Power Capping
        fastReleaseTimeIndex = 0;
        slowReleaseTimeIndex = 0;

        if (cpuPowerLimit <= MIN_POWER_LIMIT)
        {
            return;
        }
        if (((counter == 0) || (counter == 2) || (counter == 4) ||
             (counter == 6)) &&
            (psuPIN > powerStore.powerCap))
        {
            if (psuPINMovingAvg > (powerStore.powerCap + FAST_APPLY_POWER_CAP))
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
            if(!cappingAssert){
                cappingAssert = true;        
                generateSELEvent(bus,powerlimitSensorPath,powerlimitEventData, true);
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
            if(cappingAssert){
                cappingAssert = false;        
                generateSELEvent(bus,powerlimitSensorPath,powerlimitEventData, false);

            }
            return;
        }
        if (psuPIN < (powerStore.powerCap - SLOW_RELEASE_POWER_CAP))
        {
            slowReleaseTimeIndex++;

            if (psuPIN < (powerStore.powerCap - FAST_RELEASE_POWER_CAP))
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
                  << powerStore.powerCap << "]\n";
        std::cerr << "[Debug Mode]"
                  << "Cap Power Time[" << time << "]\n";
        std::cerr << "[Debug Mode]"
                  << "Apply Capping Index[" << capTimeIndex << "]\n";
        std::cerr << "[Debug Mode]"
                  << "Fast Release Index[" << fastReleaseTimeIndex << "] "
                  << "Slow Release Index[" << slowReleaseTimeIndex << "]\n";
    }
}
//===========================================================================================

void PowerHandler::cappingHandle(void)
{
    int ret;
    
    //std::cerr << "============== capping loop =============== \n";
	if(powerStore.powerCapEnable && powerStore.powerCap)
	{
    	updateMovingAverage();
		
		std::tie(ret, cpuPowerLimit) = getCpuPowerLimit(cpuSocketId);
		if(ret){
			std::cerr << "Can't get CPU power limit \n";
		}else{
			checkAndCapPower();
		}
	}
    waitTimer.expires_from_now(boost::asio::chrono::seconds(polling));
    waitTimer.async_wait([this](const boost::system::error_code& ec) {
        if (ec == boost::asio::error::operation_aborted)
        {
            return; // we're being canceled
        }
        else if (ec)
        {
            std::cerr << "timer error\n";
            return;
        }
        cappingHandle();
    });

}



PowerHandler::PowerHandler(boost::asio::io_service& io,PowerStore& powerStore,
        std::shared_ptr<sdbusplus::asio::connection>& connection, int polling):
    waitTimer(io),polling(polling),conn(connection), powerStore(powerStore)
{
    std::cerr << "============== PowerHandler =============== \n";

    while(true){
        waitTimer.expires_from_now(boost::asio::chrono::seconds(5));
        waitTimer.wait();
        if(hasBiosPost(conn))
        {
            break;
        }
    }

    std::cerr << "============== POST-END =============== \n";

    auto powerCapMatcherCallback = [&](sdbusplus::message::message &msg)
    {
        std::string interface;
        boost::container::flat_map<std::string, std::variant<std::string, uint16_t, bool, uint32_t>> propertiesChanged;
        msg.read(interface, propertiesChanged);
        std::string event = propertiesChanged.begin()->first;
        
        if (propertiesChanged.empty() || event.empty())
        {
            return;
        }
        
        if(event == "SamplingPeriod")
        {
            auto value = std::get_if<uint16_t>(&propertiesChanged.begin()->second);
            powerStore.samplingPeriod = *value;
        }
        if(event == "CorrectionTime")
        {
            auto value = std::get_if<uint32_t>(&propertiesChanged.begin()->second);
            powerStore.correctionTime = *value;
        }
        if(event == "ExceptionAction")
        {
            auto value = std::get_if<std::string>(&propertiesChanged.begin()->second);
            powerStore.exceptionAction = *value;
        }
        if(event == "PowerCap")
        {
            auto value = std::get_if<uint32_t>(&propertiesChanged.begin()->second);
            powerStore.powerCap = (*value) * mW;
            std::cerr << "New Sytem power capping value detected: " << powerStore.powerCap << "\n";
        }
        
        if(event == "PowerCapEnable")
        {
            auto value = std::get_if<bool>(&propertiesChanged.begin()->second);
                powerStore.powerCapEnable = *value;
                powerStore.actionEnable = *value;
            if(powerStore.powerCapEnable)
            {
                //set capping on handler
                std::cerr << "========@@ Power capping enabled @@ ======== \n";
                if(!running){
                    running = true;
                    cappingHandle();
                }
            }else
            {
                std::cerr << "========@@ Power capping disabled @@ ======== \n";
                //set capping off handler
                waitTimer.cancel();
                running = false;   
                setDefaultCpuPowerLimit(cpuSocketId);
              
            }
        }
        
        if constexpr (DEBUG)
        {
            std::cerr << "Properties changed event: " << event <<"\n";
            std::cerr << "PowerStore.samplingPeriod: " <<  powerStore.samplingPeriod <<"\n";
            std::cerr << "PowerStore.powerCapEnable: " <<  powerStore.powerCapEnable <<"\n";
            std::cerr << "PowerStore.correctionTime: " <<  powerStore.correctionTime <<"\n";
            std::cerr << "PowerStore.exceptionAction: " <<  powerStore.exceptionAction <<"\n";
            std::cerr << "PowerStore.powerCap: " <<  powerStore.powerCap <<"\n";
        }
        
    };
       
    static sdbusplus::bus::match::match powerCapMatcher(
         static_cast<sdbusplus::bus::bus &>(*conn),
         "type='signal',interface='org.freedesktop.DBus.Properties',member='"
         "PropertiesChanged',arg0namespace='xyz.openbmc_project.Control.Power.Cap'",
         std::move(powerCapMatcherCallback));
         
        
    std::cerr <<"cap: " << powerStore.powerCapEnable << " value: " << powerStore.powerCap << "\n";
    if(powerStore.powerCapEnable && !running){
            
        //set capping on handler
        std::cerr << "==============@@ started by init =============== \n";
        running = true;
        cappingHandle();
    }

#if 0 // leave power check to service file
    auto powerMatcherCallback = [&](sdbusplus::message::message &msg)
    {
        msg.read(pgood);
        std::cerr << "==============@@ powerMatcherCallback =============== \n";
            if(pgood)
            {
                //set power on handler
                std::cerr << "==============@@ powered on!! =============== \n";
                handle();
            }
            else
            {
                std::cerr << "==============@@ powered off!! =============== \n";
                //set power off handler
                       
            }
       };
       static sdbusplus::bus::match::match powerMatcher(
           static_cast<sdbusplus::bus::bus &>(*conn),
           sdbusplus::bus::match::rules::type::signal() +
           sdbusplus::bus::match::rules::member(std::string("PowerTrigger")),
           std::move(powerMatcherCallback));
#endif

}


PowerHandler::~PowerHandler()
{
    waitTimer.cancel();
}


#ifdef SYSFS_MODE
int readPower(std::string path)
{
    std::fstream powerValueFile(path, std::ios_base::in);
    int value;
    int factor = 1000000;
    powerValueFile >> value;
    return value / factor;
}
#endif

#ifdef DBUS_MODE

double readDbusPower(std::string path)
{
    
    //std::cerr << "readDbusPower\n";
#if 0  //todo, improve the sensor reading 
    std::promise<double> p;
    std::future<double> fu = p.get_future(); 
    sdbusplus::asio::getProperty<double>(*bus, POWER_SERVICE, POWER_PATH, POWER_INTERFACE, "Value",
            [&](boost::system::error_code ec, double newValue){
                if(ec){
                    std::cerr << "is_method_error\n"; 
                    return -1;    
                }  
                std::cerr << "newvalue: " << newValue <<"\n"; 
                p.set_value(10);
                //std::cerr << "valueBuf: " << valueBuf <<"\n"; 
            });

    auto value = fu.get();
#endif     
#if 1
    std::variant<double> valueBuf;
    auto method = bus->new_method_call(POWER_SERVICE, path.c_str(), "org.freedesktop.DBus.Properties", "Get");
    method.append(POWER_INTERFACE,"Value");
    try
    {
        auto reply=bus->call(method);
        if (reply.is_method_error())
        {
            std::cerr << "is_method_error\n"; 
            return -1;
        }
        reply.read(valueBuf);
    }
    catch (const sdbusplus::exception::SdBusError& e){
        std::cerr << "exception\n"; 
        return -1;
    }
    //std::cerr << "readDbusPower___success!!\n"; 
#endif 

    return std::get<double>(valueBuf);

}
#endif


void updatePowerValue(std::string property,const double& newValue,double& oldValue)
{
    if(newValue != oldValue)
    {
        oldValue = newValue;
        powerInterface->set_property(property, newValue);
    }
}

void dbusServiceInitialize(PowerStore& powerStore)
{
    bus->request_name(DCMI_SERVICE);
    sdbusplus::asio::object_server objServer(bus);
    powerInterface=objServer.add_interface(DCMI_POWER_PATH,DCMI_POWER_INTERFACE);
    powerInterface->register_property(
        PERIOD_MAX_PROP, powerStore.max, sdbusplus::asio::PropertyPermission::readWrite);
    powerInterface->register_property(
        PERIOD_MIN_PROP, powerStore.min, sdbusplus::asio::PropertyPermission::readWrite);
    powerInterface->register_property(
        PERIOD_AVERAGE_PROP, powerStore.average, sdbusplus::asio::PropertyPermission::readWrite);
    powerInterface->initialize();
}

void propertyInitialize(PowerStore& powerStore)
{
    labelMatch = 
    {
        {"pout1", PSUProperty("Output Power", 3000, 0, 6)},
        {"pout2", PSUProperty("Output Power", 3000, 0, 6)},
        {"pout3", PSUProperty("Output Power", 3000, 0, 6)},
        {"power1", PSUProperty("Output Power", 3000, 0, 6)}
    };

    try
    {
        auto service = getService(bus, PCAP_INTERFACE, PCAP_PATH);
        auto properties = getAllDbusProperties(bus, service, PCAP_PATH,
                                                     PCAP_INTERFACE,DBUS_TIMEOUT);
		powerStore.powerCap = std::get<uint32_t>(properties[POWER_CAP_PROP]);
        
        //all calculations use mW
        powerStore.powerCap *= mW;
        powerStore.powerCapEnable = std::get<bool>(properties[POWER_CAP_ENABLE_PROP]);
        powerStore.exceptionAction = std::get<std::string>(properties[EXCEPTION_ACTION_PROP]);
        powerStore.correctionTime = std::get<uint32_t>(properties[CORRECTION_TIME_PROP]);
        powerStore.samplingPeriod = std::get<uint16_t>(properties[SAMPLING_PERIOD_PROP]);

    }
    catch (SdBusError& e)
    {
        log<level::ERR>("Error in initialize power cap parameter set",entry("ERROR=%s", e.what()));
        sleep(3);
        propertyInitialize(powerStore);
    }

}


int main()
{
    boost::asio::io_service io;
    bus = std::make_shared<sdbusplus::asio::connection>(io);
    sdbusplus::asio::object_server objServer = sdbusplus::asio::object_server(bus);
	PowerStore powerStore;

	dbusServiceInitialize(powerStore);
	propertyInitialize(powerStore);
	powerStore.powerPath = POWER_PATH;

#if 0  //for debug only
    boost::asio::steady_timer t(io);
    while(1){ 
         t.expires_from_now(boost::asio::chrono::seconds(1));
        t.wait();
        auto value = readDbusPower(POWER_PATH);
        std::cerr << "value: " << value << "\n";
    }
#endif
	auto powerHandler = std::make_unique<PowerHandler>(io, powerStore, bus, 1);

    io.run();

    return 0;
}


