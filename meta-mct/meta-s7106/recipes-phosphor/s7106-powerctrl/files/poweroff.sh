#!/bin/bash

echo "Enter Power off System action"

pwrstatus=$(/usr/bin/gpioget gpiochip0 26)
if [ $pwrstatus -eq 1 ]; then   
    # *** Push power button ***
    # GPIO E1
    /usr/bin/gpioset gpiochip0 33=0
    for (( i=0; i<=6; i=i+1 ))
    do
        sleep 1
        pwrstatus=$(/usr/bin/gpioget gpiochip0 26)
        if [ $pwrstatus -eq 0 ]; then
             break;    
        fi
    done    
    /usr/bin/gpioset gpiochip0 33=1
    sleep 1
fi

obmcutil chassisoff

echo "Exit Power off System action"
exit 0;
