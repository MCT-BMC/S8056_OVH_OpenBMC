#!/bin/bash

# Script for control fan when host state is changed

DbusSetPwm() {
    zonesString=$(busctl call "xyz.openbmc_project.ObjectMapper" \
                  "/xyz/openbmc_project/object_mapper" \
                  "xyz.openbmc_project.ObjectMapper" \
                  "GetSubTreePaths" sias \
                  "/xyz/openbmc_project/settings/fanctrl" 0 0 | \
                  cut -d " " -f 3- | sed -e 's/"//g')
    for path in $zonesString
    do
        busctl set-property xyz.openbmc_project.State.FanCtrl "$path" \
                            xyz.openbmc_project.Control.Mode \
                            SetPwm \(sd\) All "$1"
    done
}

KeepManualMode() {
    # Get zones that should keep manual mode.
    ManualFileDir="/var/fanManual/"
    if [ -d "$ManualFileDir" ]; then
        manualZonesString=$(ls $ManualFileDir)
        for zone in $manualZonesString
        do
            manualZones+=("$zone")
        done
    fi

    # Get all fan zones D-Bus object path
    zonesString=$(busctl call "xyz.openbmc_project.ObjectMapper" \
                  "/xyz/openbmc_project/object_mapper" \
                  "xyz.openbmc_project.ObjectMapper" \
                  "GetSubTreePaths" sias \
                  "/xyz/openbmc_project/settings/fanctrl" 0 0 | \
                  cut -d " " -f 3- | sed -e 's/"//g')
    for path in $zonesString
    do
        found=false
        # Get zone name from path for example: zone1
        zone="${path##*/}"

        # If this zone should keep manual mode,
        # restore pwm value writing in manual file.
        for manualZone in "${manualZones[@]}"
        do
            if [ "$manualZone" == "$zone" ]; then
                while read -r line ; do
                    found=true
                    sensor=$(echo "$line" | cut -d " " -f 2)
                    pwmValue=$(echo "$line" | cut -d " " -f 3)
                    busctl set-property xyz.openbmc_project.State.FanCtrl "$path" \
                                        xyz.openbmc_project.Control.Mode \
                                        SetPwm \(sd\) "$sensor" "$pwmValue"
                done < "$ManualFileDir$zone"
            fi
        done

        if [ "$found" == "false" ]; then
            busctl set-property xyz.openbmc_project.State.FanCtrl "$path" \
                                xyz.openbmc_project.Control.Mode Manual b false
        fi
    done
}

busctl call xyz.openbmc_project.State.FanCtrl \
            /xyz/openbmc_project/settings/fanctrl \
            org.freedesktop.DBus.Introspectable Introspect > /dev/null 2>&1
status=$?
if [ "$1" == "power-on" ] || [ "$1" == "post-not-complete" ]; then
    echo "Fan output 80% pwm for power host on/reset."
    if [ "$status" == "0" ]; then
        DbusSetPwm 80
    else
        writePwm.sh -p 80
    fi
elif [ "$1" == "power-off" ]; then
    echo "Fan output 0% pwm for host off."
    if [ "$status" == "0" ]; then
        DbusSetPwm 0
    else
        writePwm.sh -p 0
    fi
elif [ "$1" == "post-complete" ]; then
    if [ "$status" -eq "0" ]; then
        KeepManualMode
    else
        systemctl start phosphor-pid-control.service
    fi
fi
