#!/bin/sh

fanTablePath_1="/usr/share/entity-manager/configurations/fan-table.json"
GPU_fanTablePath_1="/usr/share/entity-manager/configurations/fan-table-gpu-sku.json"

fanTablePath_2="/usr/share/entity-manager/configurations/fan-table-node2.json"
GPU_fanTablePath_2="/usr/share/entity-manager/configurations/fan-table-gpu-sku-node2.json"

NODE_ID=$(busctl get-property "xyz.openbmc_project.mct.node" \
                             "/xyz/openbmc_project/mct/node" \
                             "xyz.openbmc_project.mct.node" \
                             CurrentNodeID | cut -d' ' -f2)

REARRISER_FruPath=`ls /sys/bus/i2c/devices/34-0056/eeprom`

setFullSpeed()
{
    writePwm.sh -p 100
    systemctl stop phosphor-pid-control.service
}

if [ "$NODE_ID" == "1" ]; then
    if [ ! -z "$REARRISER_FruPath" ]; then
        if [ -f "$GPU_fanTablePath_1" ]; then
            echo "Node 1 GPU SKU"
            /usr/bin/swampd -c $GPU_fanTablePath_1
        else
            echo $GPU_fanTablePath_1" does not exist, fan output 100% pwm"
            setFullSpeed
        fi
    else
        if [ -f "$fanTablePath_1" ]; then
            echo "Node 1 none GPU SKU"
            /usr/bin/swampd -c $fanTablePath_1
        else
            echo $fanTablePath_1" does not exist, fan output 100% pwm"
            setFullSpeed
        fi
    fi
    exit
fi


if [ "$NODE_ID" == "2" ]; then
    if [ ! -z "$REARRISER_FruPath" ]; then
        if [ -f "$GPU_fanTablePath_2" ]; then
            echo "Node 2 GPU SKU"
            /usr/bin/swampd -c $GPU_fanTablePath_2
        else
            echo $GPU_fanTablePath_2" does not exist, fan output 100% pwm"
            setFullSpeed
        fi
    else
        if [ -f "$fanTablePath_2" ]; then
            echo "Node 2 none GPU SKU"
            /usr/bin/swampd -c $fanTablePath_2
        else
            echo $fanTablePath_2" does not exist, fan output 100% pwm"
            setFullSpeed
        fi
    fi
    exit
fi
