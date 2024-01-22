#!/bin/bash

BUSCTL_CMD="/usr/bin/env busctl"


FaultSendLog=0
WarningLog=0
SuccessSendLog=0
Fault=0
Warning=0
FaultCount=0
VinFaultCount=0
NotPresentCount=0
WarningCount=0
AcLossTriggered=0

SERVICE="xyz.openbmc_project.Logging.IPMI"
OBJECT="/xyz/openbmc_project/Logging/IPMI"
INTERFACE="xyz.openbmc_project.Logging.IPMI"
METHOD="IpmiSelAdd"

DISCRETE_SERVICE="xyz.openbmc_project.EventSensor"
DISCRETE_OBJECT="/xyz/openbmc_project/sensors/power_supply/"
DISCRETE_INTERFACE="xyz.openbmc_project.Sensor.Value"
DISCRETE_PROPERTY="Value"

targetDevice=$1
sensorName=$2
clearFault=$3

psuBus=$(/sbin/fw_printenv "${targetDevice}_bus" | cut -d'=' -f 2)
psuaddress=$(/sbin/fw_printenv "${targetDevice}_address" | cut -d'=' -f 2)
echo "${targetDevice} bus: $psuBus, address: $psuaddress"

sleepTime=1

# Wait service reading
mapper wait $DISCRETE_OBJECT$sensorName

if [ $clearFault == 1 ]; then
    # Clear PSU fault
    i2cset -f -y $psuBus $psuaddress 0x03 cp 2> /dev/null
    sleep 1
fi

while true; do

    StatusWord=`i2cget -f -y $psuBus $psuaddress 0x79 w 2> /dev/null`

    if [ -z "$StatusWord" ]; then
        Fault=1
        (( FaultCount++ ))
        (( NotPresentCount++ ))
        if [ $FaultSendLog == 1 ];then
            sleepTime=10
        fi
    fi

    #check VOUT fault or warning
    if [ $(((StatusWord & 0x8000) >> 15)) -eq 1 ]; then
        # read STATUS_VOUT 0x7a , check fault or warning
        Status_Vout=`i2cget -f -y $psuBus $psuaddress 0x7a 2> /dev/null`
        if [ $((Status_Vout & 0x94)) -gt 0 ]; then
            Fault=1
            (( FaultCount++ ))
        else
            Warning=1
            (( WarningCount++ ))
        fi
    fi

    # check IOUT/POUT fault or warning
    if [ $(((StatusWord & 0x4000) >> 14)) -eq 1 ]; then
        # read STATUS_IOUT 0x7b , check fault or warning
        Status_Iout=`i2cget -f -y $psuBus $psuaddress 0x7b 2> /dev/null`
        if [ $((Status_Iout & 0xd2)) -gt 0 ]; then
            Fault=1
            (( FaultCount++ ))
        else
            Warning=1
            (( WarningCount++ ))
        fi
    fi
    
    # check VIN fault or warning
    if [ $(((StatusWord & 0x2000) >> 13)) -eq 1 ]; then
        # read STATUS_INPUT 0x7c , check fault or warning
        Status_Input=`i2cget -f -y $psuBus $psuaddress 0x7c 2> /dev/null`

        if [ $((Status_Input & 0x94)) -gt 0 ]; then
            Fault=1
            (( FaultCount++ ))
            if [ $((Status_Input & 0x10)) -gt 0 ]; then
                (( VinFaultCount++ ))
            fi
        else
            Warning=1
            (( WarningCount++ ))
        fi
    fi

    #check FAN fault or warning
    if [ $(((StatusWord & 0x400) >> 10)) -eq 1 ]; then
        # read STATUS_FANS_1_2 0x81 , check fault or warning
        Status_Fan12=`i2cget -f -y $psuBus $psuaddress 0x81 2> /dev/null`
        if [ $((Status_Fan12 & 0xc2)) -gt 0 ]; then
            Fault=1
            (( FaultCount++ ))
        else
            Warning=1
            (( WarningCount++ ))
        fi
    fi

    #check TEMPERATURE fault or warning
    if [ $(((StatusWord & 0x4) >> 2)) -eq 1 ]; then
        # read STATUS_TEMPERATURE 0x7d , check fault or warning
        Status_Temp=`i2cget -f -y $psuBus $psuaddress 0x7d 2> /dev/null`
        if [ $((Status_Temp & 0x90)) -gt 0 ]; then
            Fault=1
            (( FaultCount++ ))
        else
            Warning=1
            (( WarningCount++ ))
        fi
    fi

    if [ $Fault == 0 ]; then
        FaultCount=0
        VinFaultCount=0
    fi

    if [ $Warning == 0 ]; then
        WarningCount=0
    fi

    if [ $AcLossTriggered == 1 ] && [ $VinFaultCount == 0 ] ; then
        $BUSCTL_CMD call xyz.openbmc_project.mct.led /xyz/openbmc_project/mct/led xyz.openbmc_project.mct.led decreaseFaultLed
        AcLossTriggered=0
    fi

    # Send sel log , 01h : Power Supply Failure detected (Remove power module)
    # Send sel log , 03h : Power Supply AC lost (Remove AC power cord)
    if [ $FaultSendLog == 0 ]; then
        if [ $FaultCount -gt 10 ]; then
            if [ $FaultCount == $VinFaultCount ] && [ $VinFaultCount != 0 ] ; then
                $BUSCTL_CMD call $SERVICE $OBJECT $INTERFACE $METHOD ssaybq "$sensorName AC lost SEL Entry" \
                "${DISCRETE_OBJECT}${sensorName}" 3 {0x03,0xFF,0xFF} yes 0x20
                $BUSCTL_CMD set-property $DISCRETE_SERVICE $DISCRETE_OBJECT$sensorName $DISCRETE_INTERFACE $DISCRETE_PROPERTY d 8
                $BUSCTL_CMD call xyz.openbmc_project.mct.led /xyz/openbmc_project/mct/led xyz.openbmc_project.mct.led increaseFaultLed
                AcLossTriggered=1
            elif [ $NotPresentCount != 0 ]; then
                $BUSCTL_CMD call $SERVICE $OBJECT $INTERFACE $METHOD ssaybq "$sensorName Failure detected SEL Entry" \
                "${DISCRETE_OBJECT}${sensorName}" 3 {0x01,0xFF,0xFF} yes 0x20
                $BUSCTL_CMD set-property $DISCRETE_SERVICE $DISCRETE_OBJECT$sensorName $DISCRETE_INTERFACE $DISCRETE_PROPERTY d 2
                FaultSendLog=1
                FaultCount=0
                SuccessSendLog=1
                $BUSCTL_CMD call xyz.openbmc_project.mct.led /xyz/openbmc_project/mct/led xyz.openbmc_project.mct.led increaseFaultLed
            fi
            VinFaultCount=0
        fi
    fi

    # Send sel log , 02h : Power Supply Predictive Failure
    if [ $WarningLog == 0 ]; then
        if [ $WarningCount -gt 10 ]; then
            $BUSCTL_CMD call $SERVICE $OBJECT $INTERFACE $METHOD ssaybq "$sensorName Predictive Failure SEL Entry" \
            "${DISCRETE_OBJECT}${sensorName}" 3 {0x02,0xFF,0xFF} yes 0x20
            $BUSCTL_CMD set-property $DISCRETE_SERVICE $DISCRETE_OBJECT$sensorName $DISCRETE_INTERFACE $DISCRETE_PROPERTY d 4
            WarningLog=1
            WarningCount=0
        fi
    fi

    # Send sel log , 00h : Power Supply Presence detected
    if [ $((StatusWord & 0xe404)) == 0 ] && [ ! -z "$StatusWord" ]; then
        if [ $SuccessSendLog == 1 ]; then
            $BUSCTL_CMD call $SERVICE $OBJECT $INTERFACE $METHOD ssaybq "$sensorName Presence detected SEL Entry" \
            "${DISCRETE_OBJECT}${sensorName}" 3 {0x00,0xFF,0xFF} yes 0x20
            $BUSCTL_CMD set-property $DISCRETE_SERVICE $DISCRETE_OBJECT$sensorName $DISCRETE_INTERFACE $DISCRETE_PROPERTY d 1
            $BUSCTL_CMD call xyz.openbmc_project.mct.led /xyz/openbmc_project/mct/led xyz.openbmc_project.mct.led decreaseFaultLed
            SuccessSendLog=0
        fi
        FaultSendLog=0
        WarningLog=0
        sleepTime=1
        NotPresentCount=0
    fi

    if [ $clearFault == 1 ]; then
        # Clear PSU fault
        i2cset -f -y $psuBus $psuaddress 0x03 cp 2> /dev/null
    fi

    Fault=0
    Warning=0
    sleep $sleepTime
done
