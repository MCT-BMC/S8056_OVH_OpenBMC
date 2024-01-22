#!/bin/bash

echo "[SV300G3-E][S] System Power soft-off"

pwrstatus=$(busctl get-property org.openbmc.control.Power /org/openbmc/control/power0 org.openbmc.control.Power pgood | cut -d' ' -f2)
if [ ${pwrstatus} -eq 1 ]; then   
    /usr/bin/gpioset gpiochip0 27=0
    sleep 1
    /usr/bin/gpioset gpiochip0 27=1
fi

echo "[SV300G3-E][P] System Power soft-off"
exit 0;
