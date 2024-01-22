#!/bin/bash

SERVICE="xyz.openbmc_project.Logging.IPMI"
OBJECT="/xyz/openbmc_project/Logging/IPMI"
INTERFACE="xyz.openbmc_project.Logging.IPMI"
METHOD="IpmiSelAdd"

SCU3C=0x1e6e203c
VAL=$(devmem $SCU3C)
if [ $(($VAL & 0x01)) -eq 1 ]; then
    mkdir -p /run/openbmc/
    touch /run/openbmc/AC-lost@0
    busctl call $SERVICE $OBJECT $INTERFACE $METHOD ssaybq \
                "AC Lost" "/xyz/openbmc_project/sensors/event/Power_Unit" \
                3 {0x04,0xff,0xff} 1 0x20
    rm -f /var/lib/phosphor-led-manager/savedGroups
    VAL=$(( $VAL & ~0x01 ))
    devmem $SCU3C 32 $VAL
else
    rm -f /run/openbmc/AC-lost@0
fi
