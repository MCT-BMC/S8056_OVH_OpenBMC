#!/bin/bash

echo "[SV310A] Starting System Power off..."

Transition=$(busctl get-property "xyz.openbmc_project.State.Host" \
                                 "/xyz/openbmc_project/state/host0" \
                                 "xyz.openbmc_project.State.Host" \
                                 "RequestedHostTransition" | cut -d' ' -f2)

PowerGood=$(busctl get-property "xyz.openbmc_project.GpioMonitor" \
                                "/xyz/openbmc_project/gpio/status" \
                                "xyz.openbmc_project.GpioStatus" \
                                "PowerGood" | cut -d ' ' -f 2)

# Set GPIO to power on only if power good gpio state deasserted and host state
# is changed by BMC (Transition Unknown means it is not changed by BMC).
if [ "$PowerGood" == "true" ] && \
   [ "$Transition" != "\"xyz.openbmc_project.State.Host.Transition.Unknown\"" ]
then
    /usr/bin/gpioset gpiochip0 35=0
    sleep 6
    /usr/bin/gpioset gpiochip0 35=1
fi

echo "[SV310A] Finished System Power off."
exit 0
