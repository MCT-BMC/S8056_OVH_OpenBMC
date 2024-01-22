#!/bin/sh

fanTablePath="/usr/share/entity-manager/configurations/fan-table.json"
PDB_fanTablePath="/usr/share/entity-manager/configurations/fan-table.json"
GPU_fanTablePath="/usr/share/entity-manager/configurations/fan-table-gpu-sku.json"

PDB_HWMON_PATH=`ls /sys/bus/i2c/drivers/pmbus/3-0060/hwmon`

SKU_ID=$(busctl get-property "xyz.openbmc_project.mct.sku" \
                             "/xyz/openbmc_project/mct/sku" \
                             "xyz.openbmc_project.mct.sku" \
                             CurrentSku | cut -d' ' -f2)

if [ "$SKU_ID" == "2" ] && [ -f "$GPU_fanTablePath" ]; then
    if [ ! -z "$PDB_HWMON_PATH" ]; then
        /usr/bin/swampd -c $GPU_fanTablePath
    else
        /usr/bin/swampd -c $GPU_fanTablePath
    fi
    exit
fi

if [ ! -z "$PDB_HWMON_PATH" ] && [ -f "$PDBfanTablePath" ]; then
    /usr/bin/swampd -c $PDB_fanTablePath
elif [ -f "$fanTablePath" ]; then
    /usr/bin/swampd -c $fanTablePath
else
    echo "Fan table does not exist, fan output 100% pwm"
    writePwm.sh -p 100
    systemctl stop phosphor-pid-control.service
fi
