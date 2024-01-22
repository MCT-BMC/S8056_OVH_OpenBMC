#!/bin/sh
 
echo "$1 hot swap controller via register"

IPMI_SERVICE="xyz.openbmc_project.Logging.IPMI"
IPMI_OBJECT="/xyz/openbmc_project/Logging/IPMI"
IPMI_INTERFACE="xyz.openbmc_project.Logging.IPMI"
IPMI_METHOD="IpmiSelAdd"


if [ "$1" == "Disable" ] || [ "$1" == "DisableIPMI" ] ; then

    if [ "$1" == "Disable" ]; then
        busctl call $IPMI_SERVICE $IPMI_OBJECT $IPMI_INTERFACE $IPMI_METHOD ssaybq "HSC disable button SEL Entry" "/xyz/openbmc_project/sensors/pwr_button/HSC_DISABLE" 3 {0x00,0xFF,0xFF} yes 0x20
    elif [ "$1" == "DisableIPMI" ]; then
         busctl call $IPMI_SERVICE $IPMI_OBJECT $IPMI_INTERFACE $IPMI_METHOD ssaybq "HSC disable IPMI SEL Entry" "/xyz/openbmc_project/sensors/fru_state/IPMI_HSC_DISABLE" 3 {0x02,0x01,0xFF} yes 0x20
    fi

    i2cset -f -y 3 0x10 0x01 0x00
fi

exit 0
