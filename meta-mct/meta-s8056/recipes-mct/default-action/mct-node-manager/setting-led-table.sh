#!/bin/sh
 
echo "Setting current using led table"

SERVICE="xyz.openbmc_project.mct.node"
OBJECT="/xyz/openbmc_project/mct/node"
INTERFACE="xyz.openbmc_project.mct.node"
PROPERTY="CurrentNodeID"

LED_TABEL_PATH="/usr/share/phosphor-led-manager/"

echo "set led-group-config.json table"

rm -rf "${LED_TABEL_PATH}led-group-config.json"

mapper wait $OBJECT

while true; do
    CURRENT_NODE_ID=$(busctl get-property $SERVICE $OBJECT $INTERFACE $PROPERTY | awk -F' ' '{print $2}')

    if [[ "$CURRENT_NODE_ID" == "1" ]]; then
        cp "${LED_TABEL_PATH}led-group-node1-config.json" "${LED_TABEL_PATH}led-group-config.json"
        break
    elif [[ "$CURRENT_NODE_ID" == "2" ]]; then
        cp "${LED_TABEL_PATH}led-group-node2-config.json" "${LED_TABEL_PATH}led-group-config.json"
        break
    else
        echo "Could not find any supported node ID: $CURRENT_NODE_ID"
    fi
    sleep 3
done