#include <string>
#include <vector>

/**
 *  @brief Get eeprom path from motherboard config json.
 *  @param[in] fru index - defined in motherboard config json.
 *
 *  @return - Eeprom path.
 **/
std::string getEepromPath(uint32_t index);

/**
 *  @brief Read eeprom data function.
 *  @param[in] index   - eeprom to read (defined in motherboard config json).
 *  @param[in] offset  - position starts to read.
 *  @param[in] readLen - size to read.
 *
 *  @return - vector of request data.
 **/
std::vector<uint8_t> readEeprom(uint32_t index, int offset, size_t readLen);

/**
 *  @brief Write eeprom data function.
 *  @param[in] index     - eeprom to write (defined in motherboard config json).
 *  @param[in] offset    - position starts to write.
 *  @param[in] writeData - data to write.
 *  @param[in] writeLen  - size to write.
 *
 **/
void writeEeprom(uint32_t index, int offset, uint8_t* writeData,
                 size_t writeLen);
