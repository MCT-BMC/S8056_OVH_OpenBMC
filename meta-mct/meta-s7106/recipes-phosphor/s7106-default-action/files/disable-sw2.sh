#!/bin/bash

service="xyz.openbmc_project.State.BMC"
interface="xyz.openbmc_project.State.BMC"
object="/xyz/openbmc_project/state/bmc0"
property="CurrentBMCState"
BMC_state="NotReady"

while [ $BMC_state != "Ready" ]; do
    BMC_state=$(busctl get-property $service $object $interface $property | awk -F'"' '{print $2}' | awk -F'.' '{print $6}')
    sleep 5
done

service="xyz.openbmc_project.HwmonTempSensor"
object="/xyz/openbmc_project/sensors/temperature/OCP_NIC_TEMP"
interface="xyz.openbmc_project.Sensor.Value"
property="Value"
sensor_value=$(busctl get-property $service $object $interface $property | awk -F' ' '{print $2}')

if [ -z "$sensor_value" ]; then
    /usr/bin/gpioset gpiochip0 49=1
    echo 0x1f > /sys/bus/i2c/devices/i2c-6/delete_device
    echo " Didn't have OCP card , disable SW2"
fi
