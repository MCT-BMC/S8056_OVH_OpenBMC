#pragma once

#include <openbmc/libobmci2c.h>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/process.hpp>
#include <openbmc/libobmcdbus.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

constexpr unsigned int sensorPollMs = 1000;

class Sensor
{
  public:
    Sensor(std::string object_path,
           std::shared_ptr<sdbusplus::asio::dbus_interface> interface,
           std::string sensor_name);
    ~Sensor(){};
    std::string object_path;
    std::shared_ptr<sdbusplus::asio::dbus_interface> interface;
    std::string sensor_name;
    uint32_t pre_value;
    uint32_t value;
    void addSel(std::string message, std::string object_path,
                std::vector<uint8_t> event_data, bool assert);
};
