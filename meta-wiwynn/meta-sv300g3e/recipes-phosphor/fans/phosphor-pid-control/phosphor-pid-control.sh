#!/bin/sh

fanTablePath="/run/initramfs/ro/usr/share/entity-manager/configurations/fan-table.json"

if [ -f "$fanTablePath" ]; then
    # Check the post complete property status
    pcstatus=$(busctl get-property org.openbmc.control.Power /org/openbmc/control/power0 org.openbmc.control.PostComplete postcomplete | cut -d ' ' -f 2)
    while [ ${pcstatus} -ne 0 ]
    do
        /usr/bin/writePwm.sh 80
        sleep 5
        pcstatus=$(busctl get-property org.openbmc.control.Power /org/openbmc/control/power0 org.openbmc.control.PostComplete postcomplete | cut -d ' ' -f 2)
    done
    sleep 1
    /usr/bin/swampd -c $fanTablePath
else
    # If fan table do not exist, write 100% pwm and stop phosphor-pid-control.service
    echo "Fan table does not exist, write 100% pwm and stop fan control service" | systemd-cat -t phosphor-pid-control
    /usr/bin/writePwm.sh 100
    systemctl stop phosphor-pid-control.service
fi
