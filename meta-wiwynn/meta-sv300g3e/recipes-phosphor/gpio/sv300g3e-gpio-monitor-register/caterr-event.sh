#!/bin/sh
# CATERR (IERR/MCERR) Error Event Log

SERVICE="xyz.openbmc_project.EventOnlySensor"
OBJECT="/xyz/openbmc_project/sensors/processor/Processor_fail"
INTERFACE="xyz.openbmc_project.CATERR.Log"
METHOD="CaterrEvent"

# Call CATERR Event Log method
busctl call $SERVICE $OBJECT $INTERFACE $METHOD

exit 0
