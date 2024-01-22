#!/bin/sh
 
SETTING_SERVICE=xyz.openbmc_project.Settings
SERVICE_STATUS_PATH=/xyz/openbmc_project/oem/ServiceStatus
SERVICE_STATUS_INTERFACE=xyz.openbmc_project.OEM.ServiceStatus

VM_STATUS=$(busctl get-property ${SETTING_SERVICE} \
                                ${SERVICE_STATUS_PATH} \
                                ${SERVICE_STATUS_INTERFACE} VmService | awk '{print $2}')

if [[ $VM_STATUS == "true" ]];then
    echo "Virtual Media service enable"
    touch /etc/vm_enable
else
    echo "Virtual Media service disable"
    rm -f /etc/vm_enable
fi

exit 0
