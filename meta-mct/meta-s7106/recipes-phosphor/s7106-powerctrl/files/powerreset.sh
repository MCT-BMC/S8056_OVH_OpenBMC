/bin/bash

echo "Enter Power Reset System Action"

SERVICE="xyz.openbmc_project.Logging.IPMI"
OBJECT="/xyz/openbmc_project/Logging/IPMI"
INTERFACE="xyz.openbmc_project.Logging.IPMI"
METHOD="IpmiSelAdd"

pwrstatus=$(/usr/bin/gpioget gpiochip0 26)
if [ $pwrstatus -eq 1 ]; then

    # *** Reset ***
    /usr/bin/gpioset gpiochip0 35=0
    sleep 1
    /usr/bin/gpioset gpiochip0 35=1

    busctl call $SERVICE $OBJECT $INTERFACE $METHOD ssaybq "SEL Entry" "/xyz/openbmc_project/sensors/restart/SYSTEM_RESTART" 3 {0x07,0x01,0x00} yes 0x20

else
    echo "System is off state."

fi

echo "Exit Power reset System action"
exit 0;
