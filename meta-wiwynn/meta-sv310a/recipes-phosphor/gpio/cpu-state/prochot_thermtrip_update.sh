#!/bin/sh
# Update prochot and thermtrip status

type=$1
event=$3

if [ "$event" == "prochot" ]; then
   offset=0x0A
elif [ "$event" == "thermtrip" ]; then
   offset=0x01
fi

SERVICE="xyz.openbmc_project.Logging.IPMI"
OBJECT="/xyz/openbmc_project/Logging/IPMI"
INTERFACE="xyz.openbmc_project.Logging.IPMI"
METHOD="IpmiSelAdd"

if [ "$type" == "assert" ]; then
   busctl call $SERVICE $OBJECT $INTERFACE $METHOD ssaybq "SEL Entry" "/xyz/openbmc_project/sensors/processor/$2" 3 {$offset,0x00,0xff} yes 0x20  
else
   busctl call $SERVICE $OBJECT $INTERFACE $METHOD ssaybq "SEL Entry" "/xyz/openbmc_project/sensors/processor/$2" 3 {$offset,0x00,0xff} no 0x20
fi

