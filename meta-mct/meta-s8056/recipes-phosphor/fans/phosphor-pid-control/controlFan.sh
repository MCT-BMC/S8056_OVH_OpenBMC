#!/bin/bash

# Script for control fan when host state is changed

WaitDbusService()
{
    mapper_service="/usr/bin/env mapper"
    check_service=$1
    while : ; do
        $mapper_service get-service $check_service
        status=$($mapper_service get-service $check_service)
        if [ -n "$status" ]; then
            break
        fi
        sleep 3
    done 
}

EnablePIDManualMode() {
    change_mode="$1"
    if [ "$change_mode" == "true" ]; then
        echo 0 > "/run/fan-config"
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
        busctl set-property xyz.openbmc_project.State.FanCtrl "$path" \
                                xyz.openbmc_project.Control.Mode Manual b true
    done
}

EnablePIDAutoMode() {
    change_mode=$1
    if [ "$change_mode" == "true" ]; then
        echo 1 > "/run/fan-config"
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
        busctl set-property xyz.openbmc_project.State.FanCtrl "$path" \
                                xyz.openbmc_project.Control.Mode Manual b false
    done
}

CheckFanServiceInit() {
    WaitDbusService /xyz/openbmc_project/settings/fanctrl
    WaitDbusService /xyz/openbmc_project/state/host0

    currentHostState=$(busctl get-property xyz.openbmc_project.State.Host \
         /xyz/openbmc_project/state/host0 xyz.openbmc_project.State.Host \
         CurrentHostState | cut -d"." -f6)

    if [ "$currentHostState" == "Off\"" ]; then
        echo "Set default fan control status : power off"
        EnableManualMode false
        writePwm.sh -p 50
    fi
}

busctl call xyz.openbmc_project.State.FanCtrl \
            /xyz/openbmc_project/settings/fanctrl \
            org.freedesktop.DBus.Introspectable Introspect > /dev/null 2>&1

if [ "$1" == "power-on" ]; then
    echo "Enable auto fan control when user does not set manufacture mode."
    if [ ! -f /run/fan-config ]; then
        echo 1 > "/run/fan-config"
    fi
    currentMode=$(cat /run/fan-config)
    if [ "$currentMode" == "1" ]; then
        EnablePIDAutoMode false
    fi
elif [ "$1" == "power-off" ]; then
    echo "Fan output 50% pwm for host off."
    currentMode=$(cat /run/fan-config)
    if [ "$currentMode" != "0" ]; then
        EnablePIDManualMode false
        writePwm.sh -p 50
    fi
elif [ "$1" == "manual-mode" ]; then
    echo "Enable fan mode: Manual"
    EnablePIDManualMode true

elif [ "$1" == "auto-mode" ] || [ "$1" == "autoMode" ]; then
    echo "Enable fan mode: Auto"
    EnablePIDAutoMode true

elif [ "$1" == "service-init" ]; then
    CheckFanServiceInit

elif [[ $1 =~ oemMode_* ]]; then
    duty=$(echo $1 | cut -d'_' -f 2)

    EnablePIDManualMode false
    writePwm.sh -p $duty

elif [[ $1 =~ AlterNode_* ]]; then
    duty=$(echo $1 | cut -d'_' -f 2)

    writePwm.sh -p $duty -a

else
    echo "Invalid fan mode: $1"
fi
