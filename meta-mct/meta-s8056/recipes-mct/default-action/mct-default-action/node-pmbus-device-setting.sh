#!/bin/sh

SERVICE="xyz.openbmc_project.mct.node"
OBJECT="/xyz/openbmc_project/mct/node"
INTERFACE="xyz.openbmc_project.mct.node"
PROPERTY="CurrentNodeID"

CURRENT_NODE_ID=$(busctl get-property $SERVICE $OBJECT $INTERFACE $PROPERTY | awk -F' ' '{print $2}')

if [[ "$CURRENT_NODE_ID" == "1" ]]; then
    echo "Target NODE ID: 1"
    /sbin/fw_setenv hsc_bus 16
    /sbin/fw_setenv hsc_address 16
    /sbin/fw_setenv pdb_bus 16
    /sbin/fw_setenv pdb_address 96
elif [[ "$CURRENT_NODE_ID" == "2" ]]; then
    echo "Target NODE ID: 2"
    /sbin/fw_setenv hsc_bus 16
    /sbin/fw_setenv hsc_address 18
    /sbin/fw_setenv pdb_bus 16
    /sbin/fw_setenv pdb_address 97
else
    echo "Could not find any supported node ID: $CURRENT_NODE_ID"
    exit -1
fi