#!/bin/sh

IPMB_CONFIG_PATH="/usr/share/ipmbbridge/ipmb-channels.json"

SERVICE="xyz.openbmc_project.mct.node"
OBJECT="/xyz/openbmc_project/mct/node"
INTERFACE="xyz.openbmc_project.mct.node"
PROPERTY="CurrentNodeID"

mapper wait $OBJECT
sleep 1

CURRENT_NODE_ID=$(busctl get-property $SERVICE $OBJECT $INTERFACE $PROPERTY | awk -F' ' '{print $2}')

if [[ "$CURRENT_NODE_ID" == "1" ]]; then
    bmc_addr=48
    remote_addr=50
elif [[ "$CURRENT_NODE_ID" == "2" ]]; then
    bmc_addr=50
    remote_addr=48
else
    echo "node-ipmb-device-setting: Could not find any supported node ID: $CURRENT_NODE_ID"
    exit -1
fi

jq --argjson bmc_addr "$bmc_addr" --argjson remote_addr "$remote_addr" \
   '.channels[0]."bmc-addr"=$bmc_addr | .channels[0]."remote-addr"=$remote_addr' \
   "$IPMB_CONFIG_PATH" > tmp.json
ret=$?
if [ "$ret" != "0" ]; then
    exit $ret
fi

mv tmp.json "$IPMB_CONFIG_PATH"

# Validate setup slave address.
valid=$(jq --argjson bmc_addr "$bmc_addr" --argjson remote_addr "$remote_addr" \
           '.channels[0].type=="ipmb" and
            .channels[0]."slave-path"=="/dev/ipmb-10" and
            .channels[0]."bmc-addr"==$bmc_addr and
            .channels[0]."remote-addr"==$remote_addr' "$IPMB_CONFIG_PATH")
if [ "$valid" == "true" ]; then
    exit 0
else
    echo "Failed to setup ipmb slave address configuration"
    exit 1
fi
