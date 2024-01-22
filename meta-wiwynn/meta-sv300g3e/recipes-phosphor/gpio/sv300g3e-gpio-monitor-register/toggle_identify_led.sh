#!/bin/sh
# Toggle the state of identify LED Group

SERVICE="xyz.openbmc_project.LED.Controller.identify"
INTERFACE="xyz.openbmc_project.Led.Physical"
object="/xyz/openbmc_project/led/physical/identify"
PROPERTY="State"

# Get current state
object=$(busctl tree $SERVICE --list | grep identify)
state=`busctl get-property $SERVICE $object $INTERFACE $PROPERTY | awk '{print $2}'`

if [ "$state" == "\"xyz.openbmc_project.Led.Physical.Action.Off\"" ]; then
    target='xyz.openbmc_project.Led.Physical.Action.On'
else
    target='xyz.openbmc_project.Led.Physical.Action.Off'
fi

# Set target state
busctl set-property $SERVICE $object $INTERFACE $PROPERTY s $target
