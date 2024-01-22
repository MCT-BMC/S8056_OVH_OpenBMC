#!/bin/bash

echo "[SV310A] Starting System Power Reset..."

PowerGood=$(busctl get-property "xyz.openbmc_project.GpioMonitor" \
                                "/xyz/openbmc_project/gpio/status" \
                                "xyz.openbmc_project.GpioStatus" \
                                "PowerGood" | cut -d ' ' -f 2)

if [ "$PowerGood" == "true" ]
then
    /usr/bin/gpioset gpiochip0 33=0
    sleep 0.001
    /usr/bin/gpioset gpiochip0 33=1
fi

echo "[SV310A] Finished System Power Reset."
exit 0;
