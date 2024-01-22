#!/bin/sh
# Update caterr status

type=$1


SERVICE="xyz.openbmc_project.Settings"
OBJECT="/xyz/openbmc_project/control/processor"
INTERFACE="org.freedesktop.DBus.Properties"

PROCESSOR_INTF="xyz.openbmc_project.Control.Processor"
PROPERTY="ProcessorStatus"
STATE="xyz.openbmc_project.Control.Processor.State.CATERR"
STATE_NORMAL="xyz.openbmc_project.Control.Processor.State.NORMAL"

if [ "$type" == "assert" ]; then
    busctl call $SERVICE $OBJECT $INTERFACE "Set" "ssv" $PROCESSOR_INTF $PROPERTY "s" $STATE
else
    busctl call $SERVICE $OBJECT $INTERFACE "Set" "ssv" $PROCESSOR_INTF $PROPERTY "s" $STATE_NORMAL

fi
