#!/bin/sh
 
echo "Setting dual BIOS function : $1"

DUAL_BIOS_SERVICE="xyz.openbmc_project.mct.DualBios"
DUAL_BIOS_OBJECT="/xyz/openbmc_project/mct/DualBios"
DUAL_BIOS_INTERFACE="xyz.openbmc_project.mct.DualBios"


if [ "$1" == "enableBiosStartTimer" ]; then
    busctl call $DUAL_BIOS_SERVICE $DUAL_BIOS_OBJECT $DUAL_BIOS_INTERFACE enableBiosStartTimer bb 1 1
fi
exit 0
