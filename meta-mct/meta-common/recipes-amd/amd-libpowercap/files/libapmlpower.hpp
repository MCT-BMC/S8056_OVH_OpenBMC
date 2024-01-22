#pragma once
#include <stdio.h>
#include <stdlib.h>

#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>

#include <iostream>
#include <memory>
#include <string>
#include <unistd.h>

extern "C"
{
#include "apml.h"
#include "esmi_cpuid_msr.h"
#include "esmi_mailbox.h"
#include "esmi_rmi.h"
#include "esmi_tsi.h"
}

std::timed_mutex esmiMutex;
int esmiMutexTimout = 3000; // 3 sec

uint32_t getCpuPowerLimit(int sockId);
uint32_t getCpuPowerConsumption(int sockId);
int setCpuPowerLimit(int sockId, uint32_t power);
int setDefaultCpuPowerLimit(int sockId);
uint64_t getMsrValue(uint32_t thread, uint32_t msrRegister);