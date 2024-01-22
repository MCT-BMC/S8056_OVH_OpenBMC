#!/bin/sh
 
echo "Get default PSU0 and PSU 1 voltage"

PSU0_HWMON_PATH=`ls /sys/bus/i2c/drivers/pmbus/3-0058/hwmon`
PSU1_HWMON_PATH=`ls /sys/bus/i2c/drivers/pmbus/3-0059/hwmon`

if [ ! -z "$PSU0_HWMON_PATH" ]; then
    PSU0_Voltage=$(cat /sys/class/hwmon/$PSU0_HWMON_PATH/in1_input)
    if [ ! -z "$PSU0_Voltage" ]; then
        if [ $PSU0_Voltage == 0 ]; then
            echo "Setting PSU0 config failed."
        elif [ $PSU0_Voltage -lt 180000 ]; then
            echo 1 > "/run/psu0-config"
        else
            echo 2 > "/run/psu0-config"
        fi
    fi
fi

if [ ! -z "$PSU1_HWMON_PATH" ]; then
    PSU1_Voltage=$(cat /sys/class/hwmon/$PSU1_HWMON_PATH/in1_input)
    if [ ! -z "$PSU1_Voltage" ]; then
        if [ $PSU1_Voltage == 0 ]; then
            echo "Setting PSU1 config failed."
        elif [ $PSU1_Voltage -lt 180000 ]; then
            echo 1 > "/run/psu1-config"
        else
            echo 2 > "/run/psu1-config"
        fi
    fi
fi

if [ -z "$PSU0_HWMON_PATH" ] &&  [ -z "$PSU1_HWMON_PATH" ]; then
    touch "/run/pdb-config"
fi