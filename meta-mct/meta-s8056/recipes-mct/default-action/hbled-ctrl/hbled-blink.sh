#!/bin/bash

# Disable HW blinking mode
/usr/bin/env devmem 0x1e6e269C 32 0x0

# Using SW driven mode
# Set GPIOP7 data as high or low
while [ 1 ]
do
    /usr/bin/env gpioset `gpiofind HEARTBEAT_LED`=0
    sleep 0.5
    /usr/bin/env gpioset `gpiofind HEARTBEAT_LED`=1
    sleep 0.5
done