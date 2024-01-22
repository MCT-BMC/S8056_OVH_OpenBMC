#!/bin/bash

SERVICE="xyz.openbmc_project.Logging.IPMI"
OBJECT="/xyz/openbmc_project/Logging/IPMI"
INTERFACE="xyz.openbmc_project.Logging.IPMI"
METHOD="IpmiSelAdd"

recordId=$(busctl call $SERVICE $OBJECT $INTERFACE $METHOD ssaybq "SEL Entry" "/xyz/openbmc_project/sensors/discrete/IB_BMC_FW_UPDATE" 3 {0xE1,0x02,0x01} yes 0x20 | cut -f2 -d' ')

ibBmcFwUpdateSel=""

for (( retry=0; retry<=3; retry++ )); do
    sleep 1
    while IFS= read -r line; do
        OIFS="$IFS"
        IFS=' '
        read -a spcaeSplit <<< "${line}"
        IFS="$OIFS"

        OIFS="$IFS"
        IFS=','
        read -a commaSplit <<< "${spcaeSplit[1]}"
        IFS="$OIFS"
        if [ "${commaSplit[0]}" == "$recordId" ]; then
            # The "IB BMC FW UPDATE" SEL format: <timestamp> <ID>,<Type>,<EventData>,<Generator ID>,<Path>,<Direction>
            ibBmcFwUpdateSel=$(printf '%s %d,%s,%s,%s,%s,%s' "${spcaeSplit[0]}" 1 "${commaSplit[1]}" "${commaSplit[2]}" "${commaSplit[3]}" "${commaSplit[4]}" "${commaSplit[5]}")
        fi
	done < /usr/share/sel/ipmi_sel
	if [ "$ibBmcFwUpdateSel" != "" ]; then
		break
	fi
done

if [[ ! -d "/var/lib/bmc-update" ]]
then
    /bin/mkdir /var/lib/bmc-update
fi

if [ -f "/var/lib/bmc-update/ib_update" ]; then
	/bin/rm /var/lib/bmc-update/ib_update
fi

if [ "$ibBmcFwUpdateSel" != "" ]; then
	echo "$ibBmcFwUpdateSel" >> /var/lib/bmc-update/ib_update
else
	/bin/touch /var/lib/bmc-update/ib_update
fi

/sbin/reboot
