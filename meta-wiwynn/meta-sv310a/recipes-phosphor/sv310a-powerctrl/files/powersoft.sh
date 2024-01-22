#!/bin/bash

echo "[SV310A] Starting System Power soft-off..."

PowerGood=$(busctl get-property "xyz.openbmc_project.GpioMonitor" \
                                "/xyz/openbmc_project/gpio/status" \
                                "xyz.openbmc_project.GpioStatus" \
                                "PowerGood" | cut -d' ' -f2)

# Set GPIO to power soft off only if power good gpio state asserted.
if [ "$PowerGood" == "true" ]
then
    /usr/bin/gpioset gpiochip0 35=0
    sleep 1
    /usr/bin/gpioset gpiochip0 35=1
fi

echo "[SV310A] Finished System Power soft-off."
exit 0