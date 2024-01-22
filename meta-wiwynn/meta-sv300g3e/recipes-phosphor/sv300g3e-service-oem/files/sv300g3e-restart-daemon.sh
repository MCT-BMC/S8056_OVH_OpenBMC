#!/bin/bash
# SV300G3-E restart specific daemons/services after warm reset
tmpFile=/run/initramfs/WarmReset
ledDir=/sys/class/leds

if [ ! -f "${tmpFile}" ];then
    exit 0;
fi
rm -f ${tmpFile}

# Restart the LED services
# The udev rules of LED: 70-leds.rules
echo "Restart the LED services"

if [ ! -d "${ledDir}" ];then
    echo "${ledDir} not exist"
    exit 1;
fi

for led in $(ls /sys/class/leds)
do
    systemctl restart xyz.openbmc_project.led.controller@sys-class-leds-${led}.service
done

exit 0
