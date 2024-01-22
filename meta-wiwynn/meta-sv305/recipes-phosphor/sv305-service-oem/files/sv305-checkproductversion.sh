#!/bin/bash

SERVICE="xyz.openbmc_project.FruDevice"
OBJECT="/xyz/openbmc_project/FruDevice/F5AWW_MAIN_BOARD"
INTERFACE="xyz.openbmc_project.FruDevice"
PROPERTY="PRODUCT_VERSION"

VERSION=$(busctl get-property $SERVICE $OBJECT $INTERFACE $PROPERTY)
if [ $? -eq 0 ]; then
	if [[ $VERSION == *"DVT"* ]]; then
		GPIOIJKL=$(devmem 0x1e780070)
		GPIOIJKL=$(( $GPIOIJKL | $((0x01 << 11)) ))
		devmem 0x1e780070 32 $(printf "0x%x" $GPIOIJKL)
	fi
else
	echo "Cannot get the product version on dbus"
	exit
fi


