#!/bin/bash

while [ 1 ]
do
	echo swmod_on > /sys/class/mct/hbled
	sleep 0.5
	echo swmod_off > /sys/class/mct/hbled
	sleep 0.5
done
