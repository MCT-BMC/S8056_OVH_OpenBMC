#pragma once
#include "VariantVisitors.hpp"

#include <iostream>
#include <map>
#include <string>

enum MuxList {
    Invalid,
    PCA9544
};

static const std::map<std::string, MuxList> MuxTable {
        { "PCA9544", PCA9544 }
};

MuxList resolveMuxList(std::string input);
int switchPCA9544(uint8_t busId, uint8_t address, uint8_t regs);