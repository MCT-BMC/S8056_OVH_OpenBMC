#!/bin/sh

pwmMax=255

if [ -z "$1" ]; then
    pwmPercent=60
else
    pwmPercent=$1
fi

if [ $pwmPercent -gt 100 ]; then
    pwmPercent=100
fi

pwmValue=$(($pwmMax * $pwmPercent / 100))

IFS=$'\n' read -rd '' -a pwmPath <<< "$(ls /sys/devices/platform/ahb/ahb:apb/1e786000.pwm-tacho-controller/hwmon/**/pwm*)"
for i in "${pwmPath[@]}";
do
    echo "$pwmValue" > "$i"
    returnValue="$?"
    if [ "${returnValue}" == 0 ]; then
        echo "Success write $pwmPercent% pwm to $i."
    else
        echo "Failed write $pwmPercent% pwm to $i, error code: $returnValue."
    fi
done
