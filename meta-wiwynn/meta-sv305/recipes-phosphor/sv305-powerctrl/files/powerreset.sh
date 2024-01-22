#!/bin/bash

echo "[SV305] Starting System Power Reset..."

pwrstatus=$(busctl get-property org.openbmc.control.Power /org/openbmc/control/power0 org.openbmc.control.Power pgood | cut -d' ' -f2)
if [ ${pwrstatus} -eq 1 ]; then
    /usr/bin/gpioset gpiochip0 33=0
    sleep 0.001
    /usr/bin/gpioset gpiochip0 33=1
#    busctl set-property "org.openbmc.control.Power" "/org/openbmc/control/power0" "org.openbmc.control.Power" "powerrst" b true
fi

echo "[SV305] Finished System Power Reset."
exit 0;
