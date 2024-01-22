#!/bin/bash

echo "[SV305] Starting System Power on..."

pwrstatus=$(busctl get-property org.openbmc.control.Power /org/openbmc/control/power0 org.openbmc.control.Power pgood | cut -d' ' -f2)
if [ ${pwrstatus} -eq 0 ]; then
    /usr/bin/gpioset gpiochip0 35=0
    sleep 1
    /usr/bin/gpioset gpiochip0 35=1
    sleep 1
    systemctl start obmc-enable-host-watchdog@0.service
    echo "[SV305] Finished System Power on."
else
    echo "[SV305] System Power already on."
fi

exit 0;
