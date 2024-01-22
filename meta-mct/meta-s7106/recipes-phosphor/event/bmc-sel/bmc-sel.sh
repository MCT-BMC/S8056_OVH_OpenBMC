#!/bin/bash

echo "Enter Check and generate SEL"

SERVICE="xyz.openbmc_project.Logging.IPMI"
OBJECT="/xyz/openbmc_project/Logging/IPMI"
INTERFACE="xyz.openbmc_project.Logging.IPMI"
METHOD="IpmiSelAdd"

/sbin/fw_printenv | grep 'bmc_update'
res=$?

if [ $res -eq 0 ]; then
    echo "Generate a SEL to record the occurrance of BMC FW update"
    busctl call $SERVICE $OBJECT $INTERFACE $METHOD ssaybq "SEL Entry" "/xyz/openbmc_project/sensors/versionchange/BMC_FW_UPDATE" 3 {0x07,0x00,0x00} yes 0x20
    /sbin/fw_setenv bmc_update
fi

echo "Exit Check and generate SEL"
exit 0;
