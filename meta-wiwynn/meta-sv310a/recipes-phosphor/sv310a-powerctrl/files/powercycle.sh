#!/bin/bash

# We need to wait power-off script finished first and then do power-on.
systemctl start obmc-host-stop@0.target

sleep "$(cut -d'=' -f2 "/etc/default/obmc/phosphor-reboot-host/reboot.conf")"

# Set property to power-on host.
# After power-off the RequestedHostTransition property will be Unknown and GPIO
# will not be set in power-on script.
busctl set-property "xyz.openbmc_project.State.Host" \
                    "/xyz/openbmc_project/state/host0" \
                    "xyz.openbmc_project.State.Host" \
                    "RequestedHostTransition" s \
                    "xyz.openbmc_project.State.Host.Transition.On"

SERVICE="xyz.openbmc_project.Logging.IPMI"
OBJECT="/xyz/openbmc_project/Logging/IPMI"
INTERFACE="xyz.openbmc_project.Logging.IPMI"
METHOD="IpmiSelAdd"

busctl call $SERVICE $OBJECT $INTERFACE $METHOD ssaybq "FRU Status - FRU Activation Assert" "/xyz/openbmc_project/sensors/discrete/FRU_Status" 3 {0x02,0xff,0xff} 1 0x20
