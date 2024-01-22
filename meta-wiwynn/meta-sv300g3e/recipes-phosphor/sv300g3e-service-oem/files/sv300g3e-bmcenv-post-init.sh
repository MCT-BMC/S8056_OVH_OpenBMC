#!/bin/bash
# SV300G3-E BMC environment post initialization

flagFilePath="/etc/initScriptFlag"
ipmbServ="xyz.openbmc_project.Ipmi.Channel.Ipmb"
ipmbPath="/xyz/openbmc_project/Ipmi/Channel/Ipmb"
ipmbIntf="org.openbmc.Ipmb"

# Config PSU register
configPSU()
{
    local meCommand="busctl call $ipmbServ $ipmbPath $ipmbIntf sendRequest yyyyay 0x01 0x2e 0x0 0xd9 19 0x57 0x1 0x0 0x1c 2 0xb0 0x0 0x0 0x0 0x20 7 0 0x5 0x4 0x1 0x1b 0x7c 0xff 0xf"
    local pmbusCommand="i2ctransfer -f -y 7 w7@0x58 0x5 0x4 0x1 0x1b 0x7c 0xff 0xf"
    local validMeResponse="(iyyyyay) 0 47 0 217 0 3 87 1 0"
    local retryCount=0
    local retryMax=3

    # PSU SMBALERT_MASK for Input UV event
    # Set PSU register via ME
    while [ ${retryCount} -lt ${retryMax} ]
    do
        res=$(${meCommand})

        if [ "${res}" = "${validMeResponse}" ]; then
            echo "[${retryCount}] Set PSU SMBALERT_MASK via ME successfully"
            break
        fi

        retryCount=$(($retryCount+1))
        sleep 1
    done

    if [ ${retryCount} -eq ${retryMax} ]; then
        echo "[${retryCount}] Failed to set PSU SMBALERT_MASK via ME"

        # Set PSU register via PMBus
        retryCount=0
        while [ ${retryCount} -lt ${retryMax} ]
        do
            res=$(${pmbusCommand})

        if [ $? == 0 ]; then
            echo "[${retryCount}] Set PSU SMBALERT_MASK via PMBus successfully"
            break
        fi

        retryCount=$(($retryCount+1))
        sleep 1
        done
    fi

    if [ ${retryCount} -eq ${retryMax} ]; then
        echo "[${retryCount}] Failed to set PSU SMBALERT_MASK via PMBUs"
    fi
}


# Register functions that post-init script is going to do.
initialize()
{
    configPSU
}

# If flag file does not exist, create it.
if [ ! -f "$flagFilePath" ]; then
    touch $flagFilePath
fi

initialize

exit 0
