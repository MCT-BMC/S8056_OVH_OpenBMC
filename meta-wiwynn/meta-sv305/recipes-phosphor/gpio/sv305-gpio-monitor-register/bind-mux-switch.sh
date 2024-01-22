#!/bin/bash

/usr/bin/gpioset gpiochip0 151=0

for retry in {1..4};
do
	if [ -d "/sys/bus/i2c/drivers/pca954x/9-0070" ]; then
	    break
	else
		/bin/sh -c '/bin/echo "9-0070" > /sys/bus/i2c/drivers/pca954x/bind'
	fi
done