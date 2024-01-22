#pragma once

#include <stdint.h>

constexpr unsigned int LatticeInsLength = 0x08;
constexpr int LatticeColSize = 128; // Bits
constexpr uint32_t LCMXO2_4000HC_DevId = 0x12bc043;

// Lattice Command List

// LCMXO2 Programming Command
constexpr unsigned int Lcmxo2IdcodePub = 0xE0;
constexpr unsigned int Lcmxo2IscEnableX = 0x74;
constexpr unsigned int Lcmxo2LscCheckBusy = 0xF0;
constexpr unsigned int Lcmxo2LscReadStatus = 0x3C;
constexpr unsigned int Lcmxo2IscErase = 0x0E;
constexpr unsigned int Lcmxo2LscInitAddr = 0x46;
constexpr unsigned int Lcmxo2LscInitAddrUfm = 0x47;
constexpr unsigned int Lcmxo2LscProgIncrNv = 0x70;
constexpr unsigned int Lcmxo2LscReadIncrNv = 0x73;
constexpr unsigned int Lcmxo2Usercode = 0xC0;
constexpr unsigned int Lcmxo2IscProgUsercode = 0xC2;
constexpr unsigned int Lcmxo2IscProgDone = 0x5E;
constexpr unsigned int Lcmxo2IscDisable = 0x26;
constexpr unsigned int Lcmxo2ByPass = 0xFF;
