#include "libsysfs.hpp"

#include <nlohmann/json.hpp>

#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

// Mapping fru index to eeprom path.
std::map<uint32_t, std::string> eepromPaths;

static std::string findEEPROMPath(const nlohmann::json& data, uint32_t index)
{
    auto fruFind = data.find("Fru");
    if (fruFind != data.end())
    {
        auto frus = data["Fru"];
        for (auto& fru : frus)
        {
            auto indexFind = fru.find("Index");
            auto busFind = fru.find("Bus");
            auto addressFind = fru.find("Address");
            if (indexFind == fru.end() || *indexFind != index ||
                busFind == fru.end() || addressFind == fru.end())
            {
                continue;
            }

            int bus;
            std::string address;
            busFind->get_to(bus);
            addressFind->get_to(address);
            int addressInt = std::stoi(address, nullptr, 16);

            std::stringstream ss;
            ss << bus << "-" << std::right << std::setfill('0') << std::setw(4)
               << std::hex << addressInt;

            return "/sys/bus/i2c/devices/" + ss.str() + "/eeprom";
        }
    }

    throw std::runtime_error("Fru is not found.");
}

std::string getEepromPath(uint32_t index)
{
    auto pathFind = eepromPaths.find(index);
    if (pathFind == eepromPaths.end())
    {
        try
        {
            std::ifstream jsonFile(MOTHERBOARD_CONFIG_PATH);
            if (jsonFile.is_open() == false)
            {
                throw std::runtime_error("Failed to open json file");
            }

            auto data = nlohmann::json::parse(jsonFile, nullptr, false);
            if (data.is_discarded())
            {
                throw std::runtime_error("Invalid json format");
            }

            eepromPaths[index] = findEEPROMPath(data, index);
        }
        catch (const std::exception& e)
        {
            std::cerr << "Failed to obtain eeprom path from motherboard json: "
                      << e.what() << "\n";
        }
    }

    return eepromPaths[index];
}

std::vector<uint8_t> readEeprom(uint32_t index, int offset, size_t readLen)
{
    std::string EepromPath = getEepromPath(index);

    if (EepromPath.empty() == true)
    {
        throw std::runtime_error("Failed to get eeprom path");
    }

    std::ifstream eepromFin(EepromPath);
    if (eepromFin.fail())
    {
        throw std::runtime_error("Failed to open eeprom file");
    }

    eepromFin.seekg(offset, eepromFin.beg);
    if (eepromFin.fail())
    {
        eepromFin.close();
        throw std::runtime_error("Failed to seek eeprom file");
    }

    std::vector<uint8_t> data(readLen);
    eepromFin.read(reinterpret_cast<char*>(data.data()), readLen);
    if (eepromFin.fail())
    {
        eepromFin.close();
        throw std::runtime_error("Failed to read eeprom file");
    }

    eepromFin.close();
    return data;
}

void writeEeprom(uint32_t index, int offset, uint8_t* writeData,
                 size_t writeLen)
{
    std::string EepromPath = getEepromPath(index);

    if (EepromPath.empty() == true)
    {
        throw std::runtime_error("Failed to get eeprom path");
    }

    std::ofstream eepromFout(EepromPath, std::ofstream::app);
    if (eepromFout.fail())
    {
        throw std::runtime_error("Failed to open eeprom file");
    }

    eepromFout.seekp(offset, eepromFout.beg);
    if (eepromFout.fail())
    {
        eepromFout.close();
        throw std::runtime_error("Failed to seek eeprom file");
    }

    eepromFout.write(reinterpret_cast<char*>(writeData), writeLen);
    if (eepromFout.fail())
    {
        eepromFout.close();
        throw std::runtime_error("Failed to write eeprom file");
    }

    eepromFout.close();
    return;
}
