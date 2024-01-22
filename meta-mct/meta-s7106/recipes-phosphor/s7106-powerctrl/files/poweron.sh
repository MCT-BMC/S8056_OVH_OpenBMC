#!/bin/bash

echo "Enter Power on System action"

#USE GPIOD2 to check power status
pwrstatus=$(/usr/bin/gpioget gpiochip0 26)
if [ $pwrstatus -eq 0 ]; then
    # *** Push power button ***
    # GPIO E1 for power on
    /usr/bin/gpioset gpiochip0 33=0
    sleep 2
    /usr/bin/gpioset gpiochip0 33=1 

fi

echo "Exit Power on System action"
exit 0;
