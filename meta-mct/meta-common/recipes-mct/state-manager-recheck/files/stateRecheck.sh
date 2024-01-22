#!/bin/sh

bmcState=$(busctl get-property xyz.openbmc_project.State.BMC /xyz/openbmc_project/state/bmc0 xyz.openbmc_project.State.BMC CurrentBMCState|cut -d ' ' -f 2|cut -d . -f 6|cut -c -5)

if [ $bmcState == "Ready" ]; then
    echo "BMC is Ready"
    exit 0;
else 
    echo "BMC is NotReady - Restart service"
    systemctl restart xyz.openbmc_project.State.BMC.service
    systemctl restart xyz.openbmc_project.State.Host.service
    systemctl restart xyz.openbmc_project.State.Chassis.service
fi
exit -1;
