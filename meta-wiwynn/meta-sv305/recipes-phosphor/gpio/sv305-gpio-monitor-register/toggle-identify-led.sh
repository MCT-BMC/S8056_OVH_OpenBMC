#!/bin/sh
# Toggle the state of identify LED Group

SERVICE="xyz.openbmc_project.LED.GroupManager"
OBJECT="/xyz/openbmc_project/led/groups/enclosure_identify"
INTERFACE="xyz.openbmc_project.Led.Group"
PROPERTY="Asserted"

# Get current state
state=`busctl get-property $SERVICE $OBJECT $INTERFACE $PROPERTY | awk '{print $2}'`

if [ "$state" == "false" ]; then
    TARGET="true"
else
    TARGET="false"
fi

# Set target state
busctl set-property $SERVICE $OBJECT $INTERFACE $PROPERTY b $TARGET

