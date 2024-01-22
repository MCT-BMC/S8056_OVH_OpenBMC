#!/bin/bash

echo "[SV300G3-E][S] System Power on"

pwrstatus=$(busctl get-property org.openbmc.control.Power /org/openbmc/control/power0 org.openbmc.control.Power pgood | cut -d' ' -f2)
if [ ${pwrstatus} -eq 0 ]; then
    /usr/bin/gpioset gpiochip0 27=0
    sleep 1
    /usr/bin/gpioset gpiochip0 27=1
    sleep 1
    # Monitor the PGood Status
    CHECK=0
    while [ ${CHECK} -lt 10 ]
    do
        sleep 1
        pwrstatus=$(busctl get-property org.openbmc.control.Power /org/openbmc/control/power0 org.openbmc.control.Power pgood | cut -d' ' -f2)
        if [ ${pwrstatus} -eq 1 ]; then
            busctl set-property xyz.openbmc_project.Watchdog /xyz/openbmc_project/watchdog/host0 xyz.openbmc_project.State.Watchdog Enabled b false
            if [ $? == 0 ]; then
                break;
            fi
        fi
        echo "${CHECK} - Failed to disable WDT."
        (( CHECK=CHECK+1 ))
    done
    echo "[SV300G3-E][P] System Power on"
else
    echo "[SV300G3-E][P] System Power already on"
fi

exit 0;
