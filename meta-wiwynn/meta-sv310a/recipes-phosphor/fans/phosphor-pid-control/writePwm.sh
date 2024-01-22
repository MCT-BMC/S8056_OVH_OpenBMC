#!/bin/bash

pwmMax=255
pwmValue=$((pwmMax * "$1" / 100))

SLOT_ID_1=$(gpioget gpiochip0 123)
if [ "$SLOT_ID_1" == "1" ] # Right Node
then
    echo "$pwmValue" > /sys/bus/i2c/devices/14-0020/hwmon/**/pwm1
    echo "$pwmValue" > /sys/bus/i2c/devices/14-0020/hwmon/**/pwm2
    echo "$pwmValue" > /sys/bus/i2c/devices/14-0020/hwmon/**/pwm3
else # Left Node
    echo "$pwmValue" > /sys/bus/i2c/devices/14-0023/hwmon/**/pwm1
    echo "$pwmValue" > /sys/bus/i2c/devices/14-0023/hwmon/**/pwm2
    echo "$pwmValue" > /sys/bus/i2c/devices/14-0023/hwmon/**/pwm3
fi
