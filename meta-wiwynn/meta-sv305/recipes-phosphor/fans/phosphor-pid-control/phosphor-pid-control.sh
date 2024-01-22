#!/bin/bash

fanTablePath="/usr/share/entity-manager/configurations/fan-table.json"

pgood=$(busctl get-property xyz.openbmc_project.GpioMonitor \
                            /xyz/openbmc_project/gpio/status \
                            xyz.openbmc_project.GpioStatus \
                            PowerGood | cut -d" " -f 2)

postcomplete=$(busctl get-property xyz.openbmc_project.GpioMonitor \
                                   /xyz/openbmc_project/gpio/status \
                                   xyz.openbmc_project.GpioStatus \
                                   PostComplete | cut -d" " -f 2)

if [ "$pgood" == "true" ]; then
    if [ "$postcomplete" == "false" ]; then
        if [ -f "$fanTablePath" ]; then
            /usr/bin/swampd -c $fanTablePath
        else
            echo "Fan table does not exist, fan output 100% pwm."
            writePwm.sh -p 100
            systemctl stop phosphor-pid-control.service
        fi
    else
        controlFan.sh power-on
        systemctl stop phosphor-pid-control.service
    fi
else
    controlFan.sh power-off
    systemctl stop phosphor-pid-control.service
fi
