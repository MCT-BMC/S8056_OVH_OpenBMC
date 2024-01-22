#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#define BIT(bit)    (1 << bit)

struct deviceInfo
{
    size_t size;
    uint8_t bus;
    uint8_t address;
    bool readOnly;
    std::vector<uint8_t> data;
};

enum class GetFRUAreaAccessType : uint8_t
{
    byte = 0x0,
    words = 0x1
};
