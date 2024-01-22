#!/bin/bash

SERVICE="xyz.openbmc_project.State.BMC"
OBJECT="/xyz/openbmc_project/state/bmc0"
INTERFACE="xyz.openbmc_project.State.BMC"
PROPERTY="CurrentBMCState"

echo "Check BMC booting process"

mapper wait $OBJECT

while true
do
    bmc_state=$(busctl get-property $SERVICE $OBJECT $INTERFACE $PROPERTY \
              | awk '{print $NF;}')
    if [ "$bmc_state" = "\"xyz.openbmc_project.State.BMC.BMCState.Ready\"" ]; then
        break
    fi
    sleep 3
done

echo "BMC booting finish."
/usr/bin/env touch /run/booting-finish

exit 0;