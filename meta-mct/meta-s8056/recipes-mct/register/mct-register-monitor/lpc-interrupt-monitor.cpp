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

#include "lpc-interrupt-monitor.hpp"

#include <iostream>
#include "utils.hpp"

static constexpr bool DEBUG = false;

void propertyInitialize()
{
    registerMatch = 
    {
        lpc_rst,
        interrupt
    };
}

std::vector<std::string> findRegisterPath()
{
    std::vector<std::string> storedPath;
    std::string parentPath;

    std::vector<fs::path> registerPaths;
    if (!findFiles(fs::path("/sys/bus/platform/drivers/aspeed-bmc-misc"), registerMatch[0], registerPaths, 1))
    {
        if constexpr (DEBUG)
        {
            std::cerr << "Could not monitor register in system\n";
        }
        return storedPath;
    }

    for (const auto& registerPath : registerPaths)
    {
        std::ifstream registerFile(registerPath);
        if (!registerFile.good())
        {
            if constexpr (DEBUG)
            {
                std::cerr << "Failure finding register path " << registerPath << "\n";
            }
            continue;
        }
        parentPath = registerPath.parent_path().string();
        break;
    }

    for (const auto& registerName : registerMatch)
    {
        storedPath.push_back(parentPath+"/"+registerName);
    }
    
    return storedPath;
}

void interruptHandler(std::string lpcPath,std::string interruptPath)
{
    struct inotify_event *event = NULL;
    struct pollfd fds[2];
    char buf[BUF_LEN];
    nfds_t nfds;
    int fd, ret, status;

    fd = inotify_init1(IN_NONBLOCK);
    if (fd == -1)
    {
        if constexpr (DEBUG)
        {
            std::cerr << "Could not access the inotify API\n";
        }
        return;
    }

    ret = inotify_add_watch(fd, interruptPath.c_str(), IN_MODIFY);
    if (ret == -1)
    {
        if constexpr (DEBUG)
        {
            std::cerr << "Cannot watch " << interruptPath <<" \n";
        }
        close(fd);
        return;
    }

    nfds = 2;
    fds[0].fd = STDIN_FILENO;
    fds[0].events = 0;
    fds[1].fd = fd;
    fds[1].events = POLLIN;

    while (1)
    {
        ret = poll(fds, nfds, -1);
        if (ret > 0)
        {
            if (fds[1].revents & POLLIN)
            {
                int len = read(fd, buf, BUF_LEN);
                char* p = buf;
                while (p < buf + len) {
                    event = (struct inotify_event*)p;
                    uint32_t mask = event->mask;
                    if (mask & IN_MODIFY) {
                        if constexpr (DEBUG)
                        {
                            std::cerr<<"LPC interrupt occurred\n";
                        }
                        status = readFileValue(lpcPath);
                        if(status)
                        {
                            interruptAction(lpcPath,status);
                        }
                    }
                    p += sizeof(struct inotify_event) + event->len;
                }
            }
        }
    }
    close(fd);
}

void registerHandler(boost::asio::io_context& io,double delay)
{
    static boost::asio::steady_timer timer(io);

    timer.expires_after(std::chrono::microseconds((long)delay));

    timer.async_wait([&io](const boost::system::error_code&) 
    {
        if(findPath.empty())
        {
            findPath = findRegisterPath();
            if(registerMatch.size()!=findPath.size())
            {
                findPath.clear();
                registerHandler(io, 1*MICRO_OFFSET);
                return; 
            }
        }

        std::vector<std::string>::iterator element;

        element = find(registerMatch.begin(), registerMatch.end(), lpc_rst);
        int lpcPosition = distance(registerMatch.begin(),element);

        element = find(registerMatch.begin(), registerMatch.end(), interrupt);
        int interruptPosition = distance(registerMatch.begin(),element);

        interruptHandler(findPath.at(lpcPosition),findPath.at(interruptPosition));

        registerHandler(io,0);
        return;
    });
}

void interruptAction(std::string lpcPath, int status)
{   
    constexpr auto objectPath = "/";
    constexpr auto interface = "org.freedesktop.DBus";
    constexpr auto signal = "LpcReset";
    uint8_t responseStatus = status & 0x01;

    auto msg = bus->new_signal(objectPath, interface, signal);
    uint32_t parameter = responseStatus;
    msg.append(parameter);
    msg.signal_send();
    setSensorStatus(bus,"DIMMSensorStatus",0x00);
    writeFileValue(lpcPath,0);
}

int main(int argc, char *argv[])
{
    boost::asio::io_context io;
    bus = std::make_shared<sdbusplus::asio::connection>(io);

    propertyInitialize();

    io.post(
        [&]() { registerHandler(io, 0); });
    
    io.run();
    return 0;
}