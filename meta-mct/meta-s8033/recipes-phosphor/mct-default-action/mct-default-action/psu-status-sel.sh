#!/bin/bash

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

psuBus=$1
psuaddress=$2
psuNumber=$((90-$2))

sleepTime=1

sensorName="";

PSU_CLEAR_FAULT_PEC=70

if [ -f "/run/pdb-config" ] && [ $psuaddress != 96 ]; then
    exit 0
elif [ ! -f "/run/pdb-config" ] && [ $psuaddress == 96 ]; then
    exit 0
fi

if [ $psuaddress == 89 ]; then
    PSU_CLEAR_FAULT_PEC=108
fi

if [ $psuaddress != 96 ]; then
    sensorName="PSU${psuNumber}_STATUS"
    #Clear PSU fault
    i2cset -f -y $psuBus $psuaddress 0x03 $PSU_CLEAR_FAULT_PEC 2> /dev/null
    sleep 1
else
    sensorName="PDB_STATUS"
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
        busctl call xyz.openbmc_project.mct.led /xyz/openbmc_project/mct/led xyz.openbmc_project.mct.led decreaseFaultLed
        AcLossTriggered=0
    fi

    if [ $FaultSendLog == 0 ]; then
        if [ $FaultCount -gt 10 ]; then
            # send sel log , 01h : Power Supply Failure detected
            if [ $FaultCount == $VinFaultCount ] && [ $VinFaultCount != 0 ] ; then
                busctl call $SERVICE $OBJECT $INTERFACE $METHOD ssaybq "$sensorName Failure detected SEL Entry" "/xyz/openbmc_project/sensors/power_supply/${sensorName}" 3 {0x03,0x00,0x00} yes 0x20
                busctl call xyz.openbmc_project.mct.led /xyz/openbmc_project/mct/led xyz.openbmc_project.mct.led increaseFaultLed
                AcLossTriggered=1
            elif [ $NotPresentCount != 0 ]; then
                busctl call $SERVICE $OBJECT $INTERFACE $METHOD ssaybq "$sensorName Failure detected SEL Entry" "/xyz/openbmc_project/sensors/power_supply/${sensorName}" 3 {0x01,0x00,0x00} yes 0x20
                FaultSendLog=1
                FaultCount=0
                SuccessSendLog=1
                busctl call xyz.openbmc_project.mct.led /xyz/openbmc_project/mct/led xyz.openbmc_project.mct.led increaseFaultLed
            fi
            VinFaultCount=0
        fi
    fi

    if [ $WarningLog == 0 ]; then
        if [ $WarningCount -gt 10 ]; then
            # send sel log , 02h : Predictive Failure
            busctl call $SERVICE $OBJECT $INTERFACE $METHOD ssaybq "$sensorName Predictive FailureSEL Entry" "/xyz/openbmc_project/sensors/power_supply/${sensorName}" 3 {0x02,0x00,0x00} yes 0x20
            WarningLog=1
            WarningCount=0
        fi
    fi

    if [ $((StatusWord & 0xe404)) == 0 ] && [ ! -z "$StatusWord" ]; then
        if [ $SuccessSendLog == 1 ]; then
            busctl call $SERVICE $OBJECT $INTERFACE $METHOD ssaybq "$sensorName Failure detected SEL Entry" "/xyz/openbmc_project/sensors/power_supply/${sensorName}" 3 {0x00,0x00,0x00} yes 0x20
            busctl call xyz.openbmc_project.mct.led /xyz/openbmc_project/mct/led xyz.openbmc_project.mct.led decreaseFaultLed
            SuccessSendLog=0
        fi
        FaultSendLog=0
        WarningLog=0
        sleepTime=1
        NotPresentCount=0
    fi

    if [ $psuaddress != 96 ]; then
        #Clear PSU fault
        i2cset -f -y $psuBus $psuaddress 0x03 $PSU_CLEAR_FAULT_PEC 2> /dev/null
    fi

    Fault=0
    Warning=0
    sleep $sleepTime
done
