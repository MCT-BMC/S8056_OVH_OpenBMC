#!/bin/bash

/usr/bin/env get-default-psu-voltage.sh

if [ -f "/run/pdb-config" ]; then
    mapper wait /xyz/openbmc_project/sensors/voltage/PSU1_VOL_12VOUT
    mapper wait /xyz/openbmc_project/sensors/current/PSU1_I_12VOUT
    mapper wait /xyz/openbmc_project/sensors/power/PSU1_PWR_48VIN
else
    mapper wait /xyz/openbmc_project/sensors/power/PSU1_Input_Power
    mapper wait /xyz/openbmc_project/sensors/power/PSU2_Input_Power
fi