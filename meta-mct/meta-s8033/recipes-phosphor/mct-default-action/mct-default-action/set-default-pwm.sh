#!/bin/sh
 
echo "Set all of the fan PWM to default"

for i in {1..6};
do
    echo 127 > /sys/class/hwmon/hwmon0/pwm$i
done

exit 0
