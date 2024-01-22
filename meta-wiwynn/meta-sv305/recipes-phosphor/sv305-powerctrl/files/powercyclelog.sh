#!/bin/bash

SERVICE="xyz.openbmc_project.Logging.IPMI"
OBJECT="/xyz/openbmc_project/Logging/IPMI"
INTERFACE="xyz.openbmc_project.Logging.IPMI"
METHOD="IpmiSelAdd"

busctl call $SERVICE $OBJECT $INTERFACE $METHOD ssaybq "FRU Status - FRU Activation Assert" "/xyz/openbmc_project/sensors/discrete/FRU_Status" 3 {0x02,0xff,0xff} 1 0x20
