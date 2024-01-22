#!/bin/bash
# GPIO Setup Script run once while BMC boot up

ElementSize=3 # One pin include 3 elements
OutPutSignal=1 # Output Signal

# GPIO Table Content Format: (GPIONumber Direction DefaultValue)
# LED GPIO Pins are controlled by LED driver so we don't need to set it here.
GPIOTable=(
     0   1 1 #GPIOA0 output high
     2   1 1 #GPIOA2 output high
     3   1 0 #GPIOA3 output low
     8   1 1 #GPIOB0 output high
     26  1 0 #GPIOD2 output low
     29  1 1 #GPIOD5 output high
     33  1 1 #GPIOE1 output high
     35  1 1 #GPIOE3 outout high
     40  1 1 #GPIOF0 outout high
     48  1 0 #GPIOG0 outout low
     64  1 1 #GPIOI0 output high
     92  1 0 #GPIOL4 output low
     99  1 0 #GPIOM3 output low
     100 1 0 #GPIOM4 output low
     103 1 1 #GPIOM7 output high
     104 1 1 #GPION0 output high
     105 1 0 #GPION1 output low
     115 1 1 #GPIOO3 output high
     135 1 1 #GPIOQ7 output high
     140 1 1 #GPIOR4 output high
     148 1 1 #GPIOS4 output high
     202 1 0 #GPIOZ2 output low
     203 1 1 #GPIOZ3 output high
     205 1 1 #GPIOZ5 output high
     206 1 1 #GPIOZ6 output high
     218 1 1 #GPIOAB2 output high
     219 1 1 #GPIOAB3 output high
)

# Setup GPIO
len=${#GPIOTable[@]}
for((i=0;i<len;i+="$ElementSize"))
do
    gpionum=${GPIOTable[i]}
    direction=${GPIOTable[i+1]}
    value=${GPIOTable[i+2]}

    if [ "${direction}" == "${OutPutSignal}" ]; then
        /usr/bin/gpioset gpiochip0 "$gpionum"="$value"
    fi
done
