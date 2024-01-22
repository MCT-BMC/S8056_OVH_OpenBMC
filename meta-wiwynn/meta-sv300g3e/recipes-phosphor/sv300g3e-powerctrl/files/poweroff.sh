#!/bin/bash

echo "[SV300G3-E][S] System Power off"

pwrstatus=$(busctl get-property org.openbmc.control.Power /org/openbmc/control/power0 org.openbmc.control.Power pgood | cut -d' ' -f2)
if [ ${pwrstatus} -eq 1 ]; then   
    /usr/bin/gpioset gpiochip0 27=0
    sleep 6
    /usr/bin/gpioset gpiochip0 27=1
    sleep 1
    obmcutil chassisoff
fi

echo "[SV300G3-E][P] System Power off"
exit 0;
