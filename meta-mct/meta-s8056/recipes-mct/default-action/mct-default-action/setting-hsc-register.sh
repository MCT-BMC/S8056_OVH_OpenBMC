#!/bin/sh
 
echo "$1 hot swap controller via register"

IPMI_SERVICE="xyz.openbmc_project.Logging.IPMI"
IPMI_OBJECT="/xyz/openbmc_project/Logging/IPMI"
IPMI_INTERFACE="xyz.openbmc_project.Logging.IPMI"
IPMI_METHOD="IpmiSelAdd"

#TODO: Delete after env-manager finished
/usr/bin/env node-pmbus-device-setting.sh

HSC_BUS=$(/sbin/fw_printenv hsc_bus | cut -d'=' -f 2)
HSC_ADDRESS=$(/sbin/fw_printenv hsc_address | cut -d'=' -f 2)

if [ "$1" == "Disable" ] || [ "$1" == "DisableIPMI" ] ; then

    if [ "$1" == "Disable" ]; then
        busctl call $IPMI_SERVICE $IPMI_OBJECT $IPMI_INTERFACE $IPMI_METHOD ssaybq "HSC disable button SEL Entry" "/xyz/openbmc_project/sensors/buttons/HSC_DISABLE" 3 {0x00,0xFF,0xFF} yes 0x20
    elif [ "$1" == "DisableIPMI" ]; then
        busctl call $IPMI_SERVICE $IPMI_OBJECT $IPMI_INTERFACE $IPMI_METHOD ssaybq "HSC disable IPMI SEL Entry" "/xyz/openbmc_project/sensors/fru_state/IPMI_HSC_DISABLE" 3 {0x02,0x01,0xFF} yes 0x20
    fi
    sleep 3

    i2cset -f -y $HSC_BUS $HSC_ADDRESS 0x01 0x00
elif [[ $1 =~ SetDelay_* ]]; then

    DELAY=$(echo $1 | cut -d'_' -f 2)

    i2cset -f -y $HSC_BUS $HSC_ADDRESS 0xcc $DELAY
elif [ "$1" == "HSCACStart" ]; then
    busctl call $IPMI_SERVICE $IPMI_OBJECT $IPMI_INTERFACE $IPMI_METHOD ssaybq "HSC AC cycle IPMI SEL Entry" "/xyz/openbmc_project/sensors/fru_state/IPMI_HSC_ACCYCLE" 3 {0x02,0x01,0xFF} yes 0x20
    sleep 3

    i2cset -f -y $HSC_BUS $HSC_ADDRESS 0xd9
fi

exit 0
