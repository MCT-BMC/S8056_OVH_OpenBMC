#!/bin/bash

echo "Check and generate SEL for BMC reboot"

SERVICE="xyz.openbmc_project.Logging.IPMI"
OBJECT="/xyz/openbmc_project/Logging/IPMI"
INTERFACE="xyz.openbmc_project.Logging.IPMI"
METHOD="IpmiSelAdd"

/sbin/fw_printenv | grep 'bmc_update'
res=$?

while true; do
    BMC_BOOT_SOURCE_PATH=`ls /run/boot-source`
    if [ ! -z "$BMC_BOOT_SOURCE_PATH" ]; then
        break
    fi
    sleep 2
done

BMC_BOOT_SOURCE=$( cat $BMC_BOOT_SOURCE_PATH )

if [ $res -eq 0 ]; then
    echo "Generate a SEL to record the occurrance of BMC FW update"
 
    if [ $BMC_BOOT_SOURCE == 0 ]; then
        busctl call $SERVICE $OBJECT $INTERFACE $METHOD ssaybq \
        "BMC1 FW Update SEL Entry" "/xyz/openbmc_project/sensors/versionchange/BMC1_FW_UPDATE" \
        3 {0x07,0x00,0x00} yes 0x20
    else
        busctl call $SERVICE $OBJECT $INTERFACE $METHOD ssaybq \
        "BMC2 FW Update SEL Entry" "/xyz/openbmc_project/sensors/versionchange/BMC2_FW_UPDATE" \
        3 {0x07,0x00,0x00} yes 0x20
    fi

    /sbin/fw_setenv bmc_update
    exit 0;
fi

echo "Exit Check and generate SEL"
exit 0;