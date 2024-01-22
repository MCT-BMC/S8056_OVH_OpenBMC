/* Copyright 2022 MCT
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

#include "mct-node-manager.hpp"

static constexpr bool DEBUG = false;

bool isFileExisted(std::string name) {
    return (access(name.c_str(), F_OK ) != -1 );
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
            std::cerr << "Error in start/stop servcie. Error" << e.what() << std::endl;
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
            std::cerr << "Error in enable/disable service. Error" << e.what() << std::endl;
            return -1;
        }
    }

     return 0;
}

int i2cWriteRead(std::string i2cBus, const uint8_t slaveAddr,
                      std::vector<uint8_t> writeData,
                      std::vector<uint8_t>& readBuf)
{
    // Open the i2c device, for low-level combined data write/read
    int i2cDev = ::open(i2cBus.c_str(), O_RDWR | O_CLOEXEC);
    if (i2cDev < 0)
    {
        std::cerr << "Failed to open i2c bus. BUS=" << i2cBus << std::endl;
        return -1;
    }

    const size_t writeCount = writeData.size();
    const size_t readCount = readBuf.size();
    int msgCount = 0;
    i2c_msg i2cmsg[2] = {0};
    if (writeCount)
    {
        // Data will be writtern to the slave address
        i2cmsg[msgCount].addr = slaveAddr;
        i2cmsg[msgCount].flags = 0x00;
        i2cmsg[msgCount].len = writeCount;
        i2cmsg[msgCount].buf = writeData.data();
        msgCount++;
    }
    if (readCount)
    {
        // Data will be read into the buffer from the slave address
        i2cmsg[msgCount].addr = slaveAddr;
        i2cmsg[msgCount].flags = I2C_M_RD;
        i2cmsg[msgCount].len = readCount;
        i2cmsg[msgCount].buf = readBuf.data();
        msgCount++;
    }

    i2c_rdwr_ioctl_data msgReadWrite = {0};
    msgReadWrite.msgs = i2cmsg;
    msgReadWrite.nmsgs = msgCount;

    // Perform the combined write/read
    int ret = ::ioctl(i2cDev, I2C_RDWR, &msgReadWrite);
    ::close(i2cDev);

    if (ret < 0)
    {
        //std::cerr << "I2C WR Failed! RET=" << std::to_string(ret) << std::endl;
        return -1;
    }
    if (readCount)
    {
        readBuf.resize(msgReadWrite.msgs[msgCount - 1].len);
    }

    return 0;
}

bool requestNodePresent()
{
    gpioLineNode = gpiod::find_line(gpioNameNodePresent);

    if (!gpioLineNode)
    {
        std::cerr << "Failed to find the " << gpioNameNodePresent << " line" << std::endl;
        return 0;
    }

    try
    {
        gpioLineNode.request(
            {"mct-node-manager", gpiod::line_request::DIRECTION_INPUT, 0});
    }
    catch (std::exception&)
    {
        std::cerr << "Failed to request events for " << gpioNameNodePresent << std::endl;
        return 0;
    }

    bool gpioValue = gpioLineNode.get_value();

    return !gpioValue;
}

int8_t requestNodeStatus()
{
    int8_t ipmbNodeAddr;
    bool nodePresent = requestNodePresent();
    if (nodePresent)
    {
        if (currentNodeId == 1)
        {
            ipmbNodeAddr = RIGHT_NODE_ADDR;
        }
        else
        {
            ipmbNodeAddr = LEFT_NODE_ADDR;
        }
        std::string i2cBus = "/dev/i2c-" + std::to_string(static_cast<uint8_t>(IPMB_NODE_BUS));
        std::vector<uint8_t> readBuf(1);
        std::vector<uint8_t> writeData{0x0};

        int ret = i2cWriteRead(i2cBus, static_cast<uint8_t>(ipmbNodeAddr), writeData, readBuf);
        auto returnAddress = (uint8_t)readBuf[0];

        return (returnAddress == (ipmbNodeAddr << 1) + 1)? NODE_PRESENT : NODE_HANGED;
    }
    else
    {
        return NODE_NOT_PRESENT;
    }
}

void updateNodeState(int8_t nodeDetected)
{
    switch(nodeDetected)
    {
        case NODE_PRESENT:
            anotherNodeState = "nodePresent";
            break;
        case NODE_HANGED:
            anotherNodeState = "nodeHanged";
            break;
        case NODE_NOT_PRESENT:
            anotherNodeState = "nodeNotPresent";
            break;
        default:
            std::cerr << "Error node state:" << std::to_string(nodeDetected) << std::endl;
            return;
    }
    iface->set_property(node::anotherNodeStateProp, anotherNodeState);
}

/* nodePresentHandler rule:
1. nodeStatus diagram:
            Node both present?
            |   1   |   0
        =====================
            |Node   |
          1 |Present|
  Node  =====================
  State     |Node   |Node not
          0 |hanged |Present

2. nodeStatus:
    - Node Present: 1
    - Node hanged: 0
    - Node not Present: -1

3. SEL rule:
    1. 1 -> 0 : Absent assert, eventdata[0] = 0 / eventdata[1] = 0
    2. 1 -> -1: Absent assert, eventdata[0] = 0 / eventdata[1] = 1
    3. 0 -> 1 : Present assert, eventdata[0] = 1
    4. 0 -> -1: X
    5. -1 -> 1: Present assert, eventdata[0] = 1
    6. -1 -> 0: X
*/
void nodePresentHandler(boost::asio::io_context& io, unsigned int delay)
{
    static boost::asio::steady_timer timer(io);
    timer.expires_after(std::chrono::seconds(delay));
    timer.async_wait([&io](const boost::system::error_code&)
    {
        // Ensure sel-logger is running.
        if (handlerReady == false)
        {
            if(!isFileExisted(selloggerReadyFile))
            {
                nodePresentHandler(io, 1);
                return;
            }
            else
            {
                handlerReady = true;
                sleep(5);
            }
        }

        int8_t nodeStatus = requestNodeStatus();
        if (nodeStatus == NODE_HANGED && isNodeHanged == false)
        {
            if (NodeHangedRetry > NODE_RESPONSE_TIMEOUT)
            {
                NodeHangedRetry = 0;
                isNodeHanged = true;
            }
            NodeHangedRetry++;
        }
        else if (previousNodeState != nodeStatus)
        {
            // Fan control 
            switch(nodeStatus)
            {
                case NODE_PRESENT:
                    break;
                case NODE_HANGED:
                    if (currentSku == S8053_GPU_SKU)
                    {
                        setBmcService("setting-fan-oem@AlterNode_85.service","StartUnit","Null");
                    }
                    else
                    {
                        setBmcService("setting-fan-oem@AlterNode_60.service","StartUnit","Null");
                    }
                    break;
                case NODE_NOT_PRESENT:
                    setBmcService("setting-fan-oem@AlterNode_20.service","StartUnit","Null");
                    break;
                default:
                    std::cerr << "Error node state:" << std::to_string(nodeStatus) << std::endl;
                    return;
            }
            if ((previousNodeState + nodeStatus) >= 0)
            {
                auto bus = sdbusplus::bus::new_default();
                uint16_t genId = 0x20;
                bool assert = true;
                bool presence = (nodeStatus == NODE_PRESENT)? true : false;
                std::vector<uint8_t> eventData(selEvtDataMaxSize, 0xFF);
                std::string sensor = (currentNodeId == 1)? "NODE2_DETECT" : "NODE1_DETECT";
                std::string senorPath = std::string(generalSensorPath) + sensor;
                std::string ipmiSELAddMessage = sensor + " SEL Entry";

                if (presence == false)
                {
                    eventData[0] = 0x00;
                    if (nodeStatus == NODE_HANGED)
                    {
                        eventData[1] = 0x00;
                    }
                    else if (nodeStatus == NODE_NOT_PRESENT)
                    {
                        eventData[1] = 0x01;
                    }
                    else
                    {
                        std::cerr << "Unexpected nodeStatus parameters" << std::endl;
                        return;
                    }
                }
                else if (presence == true)
                {
                    isNodeHanged = false;
                    NodeHangedRetry = 0;
                    eventData[0] = 0x01;
                }
                else
                {
                    std::cerr << "Unexpected assert parameters" << std::endl;
                    return;
                }

                sdbusplus::message::message addSEL = bus.new_method_call(
                                sel::busName, sel::path, sel::interface, "IpmiSelAdd");
                addSEL.append(ipmiSELAddMessage, senorPath, eventData, assert, genId);

                try
                {
                    bus.call(addSEL);
                }
                catch (const std::exception& e)
                {
                    std::cerr << "ERROR=" << e.what() << std::endl;
                    return;
                }
            }
            previousNodeState = nodeStatus;
            updateNodeState(nodeStatus);
        }
        else if (nodeStatus != 0 && NodeHangedRetry != 0)
        {
            NodeHangedRetry = 0;
        }

        nodePresentHandler(io, 1);
    });
}

int settingNodeFwEnv(uint8_t currentNodeId)
{
    const static std::string envConfigPath = "/usr/share/mct-env-manager/env-config.json";
    const static std::string specifiedService = "mct-node-manager.service";

    constexpr auto KEY = "Key";
    constexpr auto VALUE = "Value";
    constexpr auto SERVICE = "Service";

    std::ifstream envConfigfile(envConfigPath);

    if(!envConfigfile)
    {
        std::cerr << "Env config file not found. PATH:" << envConfigPath << std::endl;
        return -1;
    }

    auto envConfig = nlohmann::json::parse(envConfigfile, nullptr, false);

    if(envConfig.is_discarded())
    {
        std::cerr << "Syntax error in " << envConfigPath << std::endl;
        return -1;
    }

    for (auto& object : envConfig)
    {
        if(
           object[SERVICE].get<std::string>() != specifiedService)
        {
            continue;
        }

        int idx = 0;
        std::string selectValue = "";
        std::string nodeKey = object[KEY].get<std::string>() + "_node_" + std::to_string(currentNodeId);

        for (auto& selectedObject : envConfig)
        {
            if(selectedObject[KEY].get<std::string>() == nodeKey)
            {
                selectValue = selectedObject[VALUE].get<std::string>();
                break;
            }
        }

        if(selectValue.empty())
        {
            std::cerr << "Failed to get key " << object[KEY]
                      << "for node " << std::to_string(currentNodeId) << std::endl;
            return -1;
        }

        std::string cmd = "/sbin/fw_setenv " + object[KEY].get<std::string>() + " " + selectValue;

        FILE* fp = popen(cmd.c_str(), "r");

        if (!fp)
        {
            std::cerr << "Syntax error in export command" << std::endl;
            return -1;
        }
        pclose(fp);
    }
    return 0;
}

void currentNodeInit()
{
    gpioLineNode = gpiod::find_line(gpioNameNode);

    if (!gpioLineNode)
    {
        std::cerr << "Failed to find the " << gpioNameNode << " line" << std::endl;
        return;
    }

    try
    {
        gpioLineNode.request(
            {"mct-node-manager", gpiod::line_request::DIRECTION_INPUT, 0});
    }
    catch (std::exception&)
    {
        std::cerr << "Failed to request events for " << gpioNameNode << std::endl;
        return;
    }

    bool gpioValue = gpioLineNode.get_value();

    /* Node GPIO define: 
            voltage level high - node 1 (left)
            voltage level low - node 2 (right)
        
        ===================
        |       PDB       |
        |=================|
        |        |        |
        | NODE 1 | NODE 2 |
        |        |        |
        ===================
    */
    if(gpioValue)
    {
        currentNode = "left";
        currentNodeId = 1;
    }
    else
    {
        currentNode = "right";
        currentNodeId = 2;
    }

    int ret = settingNodeFwEnv(currentNodeId);
    if( ret != 0)
    {
        std::cerr << "Failed to set node related firmware environment." << std::endl;
    }

    std::cerr << "Current Node : " << currentNode
              << ", Current Node ID : " << std::to_string(currentNodeId)
              << std::endl;
}

int8_t getSkuID()
{
    int8_t skuType;

    if (isFileExisted(frontRiserEeporm) || isFileExisted(rearRiserEeporm))
    {
        std::cerr << "currentSku: S8053_GPU_SKU." << std::endl;
        skuType = S8053_GPU_SKU;
    }
    else
    {
        std::cerr << "currentSku: S8053_NON_GPU_SKU." << std::endl;
        skuType = S8053_NON_GPU_SKU;
    }

    return skuType;
}

void dbusServiceInitialize()
{
    bus->request_name(node::busName);
    sdbusplus::asio::object_server objServer(bus);
    iface=objServer.add_interface(node::path,node::interface);
    iface->register_property(node::currentNodeProp, currentNode,
                             sdbusplus::asio::PropertyPermission::readOnly);
    iface->register_property(node::currentNodeIdProp, currentNodeId,
                             sdbusplus::asio::PropertyPermission::readOnly);
    iface->register_property(node::anotherNodeStateProp, anotherNodeState,
                            sdbusplus::asio::PropertyPermission::readOnly);
    iface->initialize();
}

int main(int argc, char *argv[])
{
    boost::asio::io_context io;
    bus = std::make_shared<sdbusplus::asio::connection>(io);

    currentNodeInit();
    dbusServiceInitialize();
    currentSku = getSkuID();

    io.post(
        [&]() { nodePresentHandler(io, 1); });

    io.run();
    return 0;
}