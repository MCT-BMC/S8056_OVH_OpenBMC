#!/bin/sh
# echo "End of Post SEL Add"

SERVICE="xyz.openbmc_project.Logging.IPMI"
OBJECT="/xyz/openbmc_project/Logging/IPMI"
INTERFACE="xyz.openbmc_project.Logging.IPMI"
METHOD="IpmiSelAdd"

busctl call $SERVICE $OBJECT $INTERFACE $METHOD ssaybq "SEL Entry" "/xyz/openbmc_project/sensors/systemevent/End_of_Post" 3 {0x1,0xff,0xff} yes 0x20

# Set the "postcomplete" property to "0"
SERVICE_PC="org.openbmc.control.Power"
OBJECT_PC="/org/openbmc/control/power0"
INTERFACE_PC="org.openbmc.control.PostComplete"
PROPERTY="postcomplete"

busctl set-property $SERVICE_PC $OBJECT_PC $INTERFACE_PC $PROPERTY i 0

exit 0
