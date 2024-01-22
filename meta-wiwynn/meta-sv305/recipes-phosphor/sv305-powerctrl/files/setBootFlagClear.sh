#!/bin/bash

echo "[SV305] set clear flag..."

clearingPolicy=$(busctl get-property xyz.openbmc_project.Settings /xyz/openbmc_project/control/host0/boot_option xyz.openbmc_project.Control.Boot.Option BootFlagValidClearing | cut -d' ' -f2)
restartCause=$(busctl get-property xyz.openbmc_project.State.Host /xyz/openbmc_project/state/host0 xyz.openbmc_project.State.Host RestartCause | cut -d' ' -f2 | tr -d "\"")
isNoise=$(busctl get-property xyz.openbmc_project.State.Host /xyz/openbmc_project/state/host0 xyz.openbmc_project.State.Host isNoise | cut -d' ' -f2 | tr -d "\"")

busctl set-property xyz.openbmc_project.State.Host /xyz/openbmc_project/state/host0 xyz.openbmc_project.State.Host RestartCause s xyz.openbmc_project.State.Host.RestartCause.Unknown
busctl set-property xyz.openbmc_project.State.Host /xyz/openbmc_project/state/host0 xyz.openbmc_project.State.Host isNoise b true

loggingService="xyz.openbmc_project.Logging.IPMI"
loggingObjPath="/xyz/openbmc_project/Logging/IPMI"
loggingInterFace="xyz.openbmc_project.Logging.IPMI"
loggingMethod="IpmiSelAdd"
loggingMsg="SystemRestart"
sensorPath="/xyz/openbmc_project/sensors/discrete/System_Restart"

powerBtnDontClear=$((clearingPolicy & 0x01))
if [ "$restartCause" = "xyz.openbmc_project.State.Host.RestartCause.PowerButton" ]; then
    busctl call $loggingService $loggingObjPath $loggingInterFace "IpmiSelAdd" ssaybq $loggingMsg $sensorPath 3 0x07 0x03 0xff yes 0x20
    if [$powerBtnDontClear -gt 0]; then
        exit 0
    fi
fi

resetBtnDontClear=$((clearingPolicy & 0x02))
if [ "$restartCause" = "xyz.openbmc_project.State.Host.RestartCause.ResetButton" ]; then
    busctl call $loggingService $loggingObjPath $loggingInterFace "IpmiSelAdd" ssaybq $loggingMsg $sensorPath 3 0x07 0x02 0xff yes 0x20
    if [$resetBtnDontClear -gt 0]; then
        exit 0
    fi
fi

wdResetDontClear=$((clearingPolicy & 0x04))
if [ "$restartCause" = "xyz.openbmc_project.State.Host.RestartCause.WatchdogTimer" ]; then
    busctl call $loggingService $loggingObjPath $loggingInterFace "IpmiSelAdd" ssaybq $loggingMsg $sensorPath 3 0x07 0x04 0xff yes 0x20
    if [$wdResetDontClear -gt 0]; then
        exit 0
    fi
fi

#boot flag will be checked in chassis control command
if [ "$restartCause" = "xyz.openbmc_project.State.Host.RestartCause.IpmiPowerOn" ]; then
    busctl call $loggingService $loggingObjPath $loggingInterFace "IpmiSelAdd" ssaybq $loggingMsg $sensorPath 3 0x07 0x01 0xff yes 0x20
    exit 0
fi

if [ "$restartCause" = "xyz.openbmc_project.State.Host.RestartCause.IpmiPowerCycle" ]; then
    busctl call $loggingService $loggingObjPath $loggingInterFace "IpmiSelAdd" ssaybq $loggingMsg $sensorPath 3 0x07 0x0C 0xff yes 0x20
    exit 0
fi

if [ "$restartCause" = "xyz.openbmc_project.State.Host.RestartCause.IpmiPowerReset" ]; then
    busctl call $loggingService $loggingObjPath $loggingInterFace "IpmiSelAdd" ssaybq $loggingMsg $sensorPath 3 0x07 0x0D 0xff yes 0x20
    exit 0
fi

if [ "$restartCause" = "xyz.openbmc_project.State.Host.RestartCause.PowerPolicyAlwaysOn" ]; then
    busctl call $loggingService $loggingObjPath $loggingInterFace "IpmiSelAdd" ssaybq $loggingMsg $sensorPath 3 0x07 0x06 0xff yes 0x20
    exit 0
fi

if [ "$restartCause" = "xyz.openbmc_project.State.Host.RestartCause.PowerPolicyPreviousState" ]; then
    busctl call $loggingService $loggingObjPath $loggingInterFace "IpmiSelAdd" ssaybq $loggingMsg $sensorPath 3 0x07 0x07 0xff yes 0x20
    exit 0
fi

if [ "$restartCause" = "xyz.openbmc_project.State.Host.RestartCause.Unknown" ]; then
    if [ "$isNoise" = "true" ]; then
        exit 0
    fi
    busctl call $loggingService $loggingObjPath $loggingInterFace "IpmiSelAdd" ssaybq $loggingMsg $sensorPath 3 0x07 0x0A 0xff yes 0x20
fi

busctl set-property xyz.openbmc_project.Settings /xyz/openbmc_project/control/host0/boot_option xyz.openbmc_project.Control.Boot.Option ClearBootFlagValid b true
