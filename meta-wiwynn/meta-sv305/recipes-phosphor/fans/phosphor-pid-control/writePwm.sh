#!/bin/bash
Usage()
{
    echo "Usage: PWM: [-p] percentage (0 - 100) or [-v] value (0 - 255)"
    echo "       FAN: [-z] zone id (0: for all zones, 1, 2) or [-f] fan id (1 ~ 6)"
    exit 255
}

writePWM()
{
    path=$1
    echo "$pwmValue" > "$path"
    returnValue="$?"
    if [ "${returnValue}" == 0 ]; then
        if [ -n "$pwmPercent" ]; then
            echo "Success write $pwmPercent% pwm to $path."
        else
            echo "Success write $pwmValue pwm to $path."
        fi
    else
        if [ -n "$pwmPercent" ]; then
            echo "Failed write $pwmPercent% pwm to $path, error code: $returnValue."
        else
            echo "Failed write $pwmValue pwm to $path, error code: $returnValue."
        fi
    fi
}
zoneId=0
pwmMax=255
while getopts "p:v:z:f:" opt; do
    case $opt in
        p)
            if ! [[ "$OPTARG" =~ ^-?[0-9]+$ ]]; then
                echo "Unknown argument. Require a number of pwm percentage after -p"
                Usage
            elif [ "$OPTARG" -gt 100 ] || [ "$OPTARG" -lt 0 ]; then
                echo "Error: pwm percentage has to be between 0 - 100"
                Usage
            fi
            pwmPercent=$OPTARG
            pwmValue=$((pwmMax * pwmPercent / 100))
            ;;
        v)
            if ! [[ "$OPTARG" =~ ^-?[0-9]+$ ]]; then
                echo "Unknown argument. Require a number of pwm after -v"
                Usage
            elif [ "$OPTARG" -gt 255 ] || [ "$OPTARG" -lt 0 ]; then
                echo "Error: pwm has to be between 0 - 255"
                Usage
            fi
            pwmValue=$OPTARG
            ;;
        z)
            if ! [[ "$OPTARG" =~ ^-?[0-9]+$ ]]; then
                echo "Unknown argument. Require a zone id  after -z"
                Usage
            elif [ "$OPTARG" -ne 0 ] && [ "$OPTARG" -ne 1 ] && [ "$OPTARG" -ne 2 ]; then
                echo "Unsupported fan id"
                Usage
            fi
            zoneId=$OPTARG
            ;;
        f)
            if ! [[ "$OPTARG" =~ ^-?[0-9]+$ ]]; then
                echo "Unknown argument. Require a fan id  after -f"
                Usage
            elif [ "$OPTARG" -lt 1 ] || [ "$OPTARG" -gt 6 ]; then
                echo "Unsupported zone id"
                Usage
            fi
            fanIds+=("$OPTARG")
            ;;
        *)
            Usage
            ;;
    esac
done

if [ -z "$pwmValue" ]; then
    Usage
fi

IFS=$'\n' read -rd '' -a pwmPath <<< "$(ls /sys/devices/platform/ahb/ahb:apb/1e786000.pwm-tacho-controller/hwmon/**/pwm*)"
for path in "${pwmPath[@]}";
do
    pwmNum=$(echo "$path" | grep -Eo '[0-9]+$')
    if [ ${#fanIds[@]} -eq 0 ]; then
        for fid in "${fanIds[@]}";
        do
            if [ "$fid" == "$pwmNum" ]; then
                writePWM "$path"
                break
            fi
        done
    else
        if [ "$zoneId" -eq "1" ]; then
            if [ "$pwmNum" -lt 3 ]; then
                    writePWM "$path"
            fi
        elif [ "$zoneId" -eq 2 ]; then
            if [ "$pwmNum" -gt 2 ]; then
                    writePWM "$path"
            fi
        else
            writePWM "$path"
        fi
    fi
done
