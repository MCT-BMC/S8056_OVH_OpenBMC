#!/bin/sh
 
echo "Setting current using power SKU"

FRU_TABEL_PATH="/usr/share/entity-manager/configurations/"

#PDB_HWMON_PATH=`ls /sys/bus/i2c/drivers/pmbus/3-0060/hwmon`

rm -rf "${FRU_TABEL_PATH}fru.json"

if false; then
    if [ -z "$PDB_HWMON_PATH" ]; then
        echo "Using CRPS SKU"
        cp "${FRU_TABEL_PATH}s8033-CRPS-fru.json" "${FRU_TABEL_PATH}fru.json"
    else
        echo "Using PDB SKU"
        cp "${FRU_TABEL_PATH}s8033-PDB-fru.json" "${FRU_TABEL_PATH}fru.json"
    fi
fi

echo "Using PDB SKU"
cp "${FRU_TABEL_PATH}PDB-fru.json" "${FRU_TABEL_PATH}fru.json"