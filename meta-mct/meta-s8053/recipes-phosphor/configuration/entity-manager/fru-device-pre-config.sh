#!/bin/sh

DRIVER_PATH="/sys/bus/i2c/drivers"

function Check_and_Bind()
{
    if [ -d "$DRIVER_PATH/$1/$2" ]; then 
        echo "Device $2 found...";
    else 
        echo "Device $2 not found... Bind the driver.";
        echo $2 > $DRIVER_PATH/$1/bind;
    fi
}

# Device behind PCA9641
Check_and_Bind "adm1275" "16-0010";
Check_and_Bind "adm1275" "16-0012";
Check_and_Bind "nct7362" "16-0022";
Check_and_Bind "nct7362" "16-0023";
Check_and_Bind "pca953x" "16-0038";
Check_and_Bind "pca953x" "16-0039";
Check_and_Bind "ina2xx" "16-0044";
Check_and_Bind "ina2xx" "16-0045";
Check_and_Bind "at24" "16-0057";
Check_and_Bind "pmbus" "16-0060";
Check_and_Bind "pmbus" "16-0061";
Check_and_Bind "pca953x" "16-0071";
Check_and_Bind "pca953x" "16-0072";