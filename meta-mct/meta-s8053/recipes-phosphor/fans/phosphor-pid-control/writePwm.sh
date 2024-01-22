#!/bin/sh
Usage()
{
    echo "Usage: PWM: [-p] percentage (0 - 100), [-v] value (0 - 255) or [-a] for setting another node's fan."
    #echo "       FAN: [-z] zone id (0: for all zones, 1, 2) or [-f] fan id (1 ~ 6)"
    exit -1
}

writePWM()
{
    path=$1
    echo "$pwmValue" > "$path"
    returnValue="$?"

    if [ "${returnValue}" == 0 ]; then
        if ! [ -z "$pwmPercent" ]; then
            echo "Success write $pwmPercent% pwm to $path."
        else
            echo "Success write $pwmValue pwm to $path."
        fi
    else
        if ! [ -z "$pwmPercent" ]; then
            echo "Failed write $pwmPercent% pwm to $path, error code: $returnValue."
        else
            echo "Failed write $pwmValue pwm to $path, error code: $returnValue."
        fi
    fi
}
isAnothernode=0
pwmMax=255
while getopts "p:a" opt; do
    case $opt in
        p)
            if ! [[ "$OPTARG" =~ ^-?[0-9]+$ ]]; then
                echo "Unknown argument. Require a number of pwm percentage after -p"
                Usage
            elif [ $OPTARG -gt 100 ] || [ $OPTARG -lt 0 ]; then
                echo "Error: pwm percentage has to be between 0 - 100"
                Usage
            fi
            pwmPercent=$OPTARG
            pwmValue=$(($pwmMax * $pwmPercent / 100))
            ;;
        a)
            echo "Control fans of another node."
            isAnothernode=1
            ;;
        *)
            Usage
            ;;
    esac
done

if [ -z "$pwmValue" ]; then
    Usage
fi

NODE_ID=$(busctl get-property "xyz.openbmc_project.mct.node" \
                             "/xyz/openbmc_project/mct/node" \
                             "xyz.openbmc_project.mct.node" \
                             CurrentNodeID | cut -d' ' -f2)

echo "Current NODE_ID: $NODE_ID"
if [ "$isAnothernode" == 1 ]; then
    if [ "$NODE_ID" == "1" ]; then
        NODE_ID=2
    else
        NODE_ID=1
    fi
fi
echo "NODE_ID of Controled fans: $NODE_ID"

if [ "$NODE_ID" == "1" ] ; then
    IFS=$'\n' read -rd '' -a pwmPath <<< "$(ls /sys/devices/platform/ahb/ahb:apb/1e610000.pwm-tacho-controller/hwmon/**/pwm*)"
else
    IFS=$'\n' read -rd '' -a pwmPath <<< "$(ls /sys/devices/platform/ahb/ahb:apb/1e610000.pwm-tacho-controller/hwmon/**/pwm*)"
fi

for path in "${pwmPath[@]}";
do
    pwmNum=$(echo $path | grep -Eo '[1,4,8]+$')
    if ! [ -z "$pwmNum" ]; then
        writePWM $path
    fi
done
