#!/bin/bash

IPMB_CONFIG_PATH="/usr/share/ipmbbridge/ipmb-channels.json"
READONLY_PATH_ROOT="/run/initramfs/ro"
MB_CONFIG_PATH="/usr/share/entity-manager/configurations/sv310a-MB.json"
SYSTEM_CONFIG_PATH="/var/configuration/system.json"
FAN_TABLE_DIR="/usr/share/entity-manager/configurations/"
SLOT_ID_1_DIR="/run/slot_id_1/"

removeFanThreshold() {
    jq --arg index "$1" \
       '.Exposes |= map((select(.Name == ("SYS FAN F" + $index) or
                                .Name == ("SYS FAN R" + $index))
                 |= del(.Thresholds)))' "$MB_CONFIG_PATH" > tmp.json
    mv tmp.json "$MB_CONFIG_PATH"
}

reviseFanPowerState() {
    jq --arg index "$1" \
        '.Exposes |= map((select(.Name == ("SYS FAN F" + $index) or
                                 .Name == ("SYS FAN R" + $index))
                  |= (.PowerState = "On")))' "$MB_CONFIG_PATH" > tmp.json
    mv tmp.json "$MB_CONFIG_PATH"
}

probePCA954x() {
    echo "Start to probe PCA964x"
    echo 14-0075 > /sys/bus/i2c/drivers/pca954x/unbind
    sleep 0.1
    echo 14-0075 > /sys/bus/i2c/drivers/pca954x/bind
    if
    [ -d "/sys/bus/i2c/devices/25-004e/hwmon" ] &&
    [ -d "/sys/bus/i2c/devices/26-0042/hwmon" ] &&
    [ -d "/sys/bus/i2c/devices/27-0010/hwmon" ] &&
    [ -d "/sys/bus/i2c/devices/28-005a/hwmon" ]
    then
        i2ctransfer -f -y 28 w3@0x5a 0xc7 0x3f 0x00
        i2ctransfer -f -y 28 w1@0x5a 0xc5 r4 > /run/openbmc/VR_48V_PDB.tmp
        echo "Finish to probe PCA954x"
    else
        echo "Retry to probe PCA954x"
        echo 14-0075 > /sys/bus/i2c/drivers/pca954x/unbind
        sleep 0.1
        echo 14-0075 > /sys/bus/i2c/drivers/pca954x/bind
        i2ctransfer -f -y 28 w3@0x5a 0xc7 0x3f 0x00
        i2ctransfer -f -y 28 w1@0x5a 0xc5 r4 > /run/openbmc/VR_48V_PDB.tmp
        echo "Finish to probe PCA954x"
    fi
}

mkdir -p /run/openbmc/
SLOT_ID_1=$(gpioget gpiochip0 123)
mkdir -p "$SLOT_ID_1_DIR"
touch "$SLOT_ID_1_DIR$SLOT_ID_1"
if [ "$SLOT_ID_1" == "1" ] # Right Node
then
    bmc_addr=48
    remote_addr=50

    # If system.json exists means it is not the first boot up of this flash.
    if [ -f "$SYSTEM_CONFIG_PATH" ]; then
        # Check whether BMC changed sides.
        # FAN3 ~ FAN5 should have no threshold at right node.
        result=$(jq '."SV310A Main Board".Exposes[] |
                    select(.Name == "SYS FAN F3").Thresholds' "$SYSTEM_CONFIG_PATH")
        if [ "$result" != "null" ]; then
            # Replace original configuration with build-in configuration.
            rm "$SYSTEM_CONFIG_PATH"
            cp "$READONLY_PATH_ROOT$MB_CONFIG_PATH" "$MB_CONFIG_PATH"
            removeFanThreshold 3
            removeFanThreshold 4
            removeFanThreshold 5
            reviseFanPowerState 0
            reviseFanPowerState 1
            reviseFanPowerState 2
        fi
    else
        # Remove thresholds if it is the first boot up of this flash.
        removeFanThreshold 3
        removeFanThreshold 4
        removeFanThreshold 5
        reviseFanPowerState 0
        reviseFanPowerState 1
        reviseFanPowerState 2
    fi

    # Get PG_P12V_FAN1, 2, 3 to check whether fans have power.
    PG_P12V_FAN1=$(gpioget $(basename /sys/bus/i2c/devices/14-0026/gpiochip*) 15)
    PG_P12V_FAN2=$(gpioget $(basename /sys/bus/i2c/devices/14-0026/gpiochip*) 13)
    PG_P12V_FAN3=$(gpioget $(basename /sys/bus/i2c/devices/14-0026/gpiochip*) 11)
    if [ "$PG_P12V_FAN1" == "1" ] || [ "$PG_P12V_FAN2" == "1" ] || \
       [ "$PG_P12V_FAN3" == "1" ]
    then
        touch /run/fan-power-enable
    fi

    # Bind mux on 48V CPLD
    mux_status=0
    retry=0
    while [ "$retry" != "20" ]; do
        i2ctransfer -f -y 14 w2@0x5c 0x04 0xf2
        sleep 1
        mux_status=$(i2ctransfer -f -y 14 w1@0x5c 0x04 r1)
        mux_status=${mux_status:0-1:4}
        mux_status=$((0x0$mux_status))
        mux_status=$((mux_status / 4))
        if [ "$mux_status" == "2" ]; then
            probePCA954x
            break;
        else
            retry=$((retry+1))
        fi
    done
    if [ "$retry" == "20" ]; then
        echo "Release CPLD mux after 20 times retry"
        i2ctransfer -f -y 14 w2@0x5c 0x04 0xf0
        i2ctransfer -f -y 14 w2@0x5c 0x04 0xf2
        sleep 1
        probePCA954x
    fi

else # Left Node
    bmc_addr=50
    remote_addr=48

    # If system.json exists means it is not the first boot up of this flash.
    if [ -f "$SYSTEM_CONFIG_PATH" ]; then
        # Check whether BMC changed sides.
        # FAN0 ~ FAN2 should have no threshold at left node.
        result=$(jq '."SV310A Main Board".Exposes[] |
                    select(.Name == "SYS FAN F0").Thresholds' "$SYSTEM_CONFIG_PATH")
        if [ "$result" != "null" ]; then
            # Replace original configuration with build-in configuration.
            rm "$SYSTEM_CONFIG_PATH"
            cp "$READONLY_PATH_ROOT$MB_CONFIG_PATH" "$MB_CONFIG_PATH"
            removeFanThreshold 0
            removeFanThreshold 1
            removeFanThreshold 2
        fi
    else
        # Remove thresholds if it is the first boot up of this flash.
        removeFanThreshold 0
        removeFanThreshold 1
        removeFanThreshold 2
    fi

    # Get PG_P12V_FAN4, 5, 6 to check whether fans have power.
    PG_P12V_FAN4=$(gpioget $(basename /sys/bus/i2c/devices/14-0027/gpiochip*) 15)
    PG_P12V_FAN5=$(gpioget $(basename /sys/bus/i2c/devices/14-0027/gpiochip*) 13)
    PG_P12V_FAN6=$(gpioget $(basename /sys/bus/i2c/devices/14-0027/gpiochip*) 11)
    if [ "$PG_P12V_FAN4" == "1" ] || [ "$PG_P12V_FAN5" == "1" ] || \
       [ "$PG_P12V_FAN6" == "1" ]
    then
        touch /run/fan-power-enable
    fi

    # Bind mux on 48V CPLD
    mux_status=0
    retry=0
    while [ "$retry" != "20" ]; do
        i2ctransfer -f -y 14 w2@0x5c 0x04 0xf1
        sleep 1
        mux_status=$(i2ctransfer -f -y 14 w1@0x5c 0x04 r1)
        mux_status=${mux_status:0-1:4}
        mux_status=$((0x0$mux_status))
        mux_status=$((mux_status / 4))
        if [ "$mux_status" == "1" ]; then
            probePCA954x
            break;
        else
            retry=$((retry+1))
        fi
    done
    if [ "$retry" == "20" ]; then
        echo "Release CPLD mux after 20 times retry"
        i2ctransfer -f -y 14 w2@0x5c 0x04 0xf0
        i2ctransfer -f -y 14 w2@0x5c 0x04 0xf1
        sleep 1
        probePCA954x
    fi
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
            .channels[0]."slave-path"=="/dev/ipmb-11" and
            .channels[0]."bmc-addr"==$bmc_addr and
            .channels[0]."remote-addr"==$remote_addr' "$IPMB_CONFIG_PATH")
if [ "$valid" == "true" ]; then
    exit 0
else
    echo "Failed to setup ipmb slave address configuration"
    exit 1
fi

