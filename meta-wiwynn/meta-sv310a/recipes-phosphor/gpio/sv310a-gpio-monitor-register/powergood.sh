#!/bin/bash

# When host power state is changed not by BMC, we have to maintain targets.
busctl set-property "xyz.openbmc_project.State.Host" \
                    "/xyz/openbmc_project/state/host0" \
                    "xyz.openbmc_project.State.Host" \
                    "RequestedHostTransition" s \
                    "xyz.openbmc_project.State.Host.Transition.Unknown"

if [ "$1" == "on" ]; then
    # Set pgood property to "1"
    busctl set-property "org.openbmc.control.Power" \
                        "/org/openbmc/control/power0" \
                        "org.openbmc.control.Power" \
                        "pgood" i 1

    # Sync target when host state is changed not by BMC.
    job=$(systemctl show obmc-host-start@0.target -p Job | cut -d "=" -f 2)
    if [ -z "$job" ]; then
        status=$(systemctl is-active obmc-host-start@0.target)
        if [ "$status" == "inactive" ]; then
            systemctl start obmc-host-start@0.target
        fi

    fi

    # Set chassis power status to on after power good assert
    systemctl start obmc-chassis-poweron@0.target

    busctl call "xyz.openbmc_project.Logging.IPMI" \
                "/xyz/openbmc_project/Logging/IPMI" \
                "xyz.openbmc_project.Logging.IPMI" \
                "IpmiSelAdd" ssaybq "ACPI_Power_State" \
                "/xyz/openbmc_project/sensors/discrete/ACPI_Power_State" \
                0x03 0x00 0xff 0xff yes 0x20

    touch /run/fan-power-enable
else
    # Set pgood property to "0"
    busctl set-property "org.openbmc.control.Power" \
                        "/org/openbmc/control/power0" \
                        "org.openbmc.control.Power" \
                        "pgood" i 0

    # Sync target when host state is changed not by BMC.
    job=$(systemctl show obmc-host-stop@0.target -p Job | cut -d "=" -f 2)
    if [ -z "$job" ]; then
        status=$(systemctl is-active obmc-host-stop@0.target)
        if [ "$status" == "inactive" ]; then
            systemctl start obmc-host-stop@0.target
        fi

    fi

    # Set chassis power status to off after power good deassert
    systemctl start obmc-chassis-poweroff@0.target

    busctl set-property "xyz.openbmc_project.Watchdog" \
                        "/xyz/openbmc_project/watchdog/host0" \
                        "xyz.openbmc_project.State.Watchdog" \
                        "Enabled" b false

    busctl call "xyz.openbmc_project.Logging.IPMI" \
                "/xyz/openbmc_project/Logging/IPMI" \
                "xyz.openbmc_project.Logging.IPMI" \
                "IpmiSelAdd" ssaybq "ACPI_Power_State" \
                "/xyz/openbmc_project/sensors/discrete/ACPI_Power_State" \
                0x03 0x05 0xff 0xff yes 0x20
fi
