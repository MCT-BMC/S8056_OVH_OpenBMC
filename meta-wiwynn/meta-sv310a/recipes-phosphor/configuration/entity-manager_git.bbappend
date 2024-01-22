FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

EXTRA_OEMESON += "-Dmotherboard-config-path=/usr/share/entity-manager/configurations/sv310a-MB.json"

SRC_URI += "file://sv310a-MB.json \
            file://sv310a-BIOS.json \
            file://sv310a-DiscreteSensor.json \
            file://Delta-DPS-500AB-PSU.json \
            file://Delta-DPS-550AB-PSU.json \
            file://Chicony-R550-PSU.json \
            file://Broadcom-NicCard.json \
            file://Mellanox-NicCard.json \
            file://fan-table.json \
            file://0001-Get-fru-device-from-motherboard-config.patch \
            file://0002-Set-the-boot-order-of-EntityManager-after-FruDevice.patch \
            file://0003-Register-dbus-done-object-path.patch \
            file://0004-Get-fru-with-fixed-size-in-config.patch \
            file://0005-Add-match-property-in-setting-table.patch \
            file://0006-Add-name-property-in-FRU-device.patch \
            file://0007-Keep-system-config-in-Json.patch \
            "

do_install_append(){
        install -d ${D}/usr/share/entity-manager/configurations
        install -m 0444 ${WORKDIR}/sv310a-MB.json ${D}/usr/share/entity-manager/configurations
        install -m 0444 ${WORKDIR}/sv310a-BIOS.json ${D}/usr/share/entity-manager/configurations
        install -m 0444 ${WORKDIR}/sv310a-DiscreteSensor.json ${D}/usr/share/entity-manager/configurations
        install -m 0444 ${WORKDIR}/Delta-DPS-500AB-PSU.json ${D}/usr/share/entity-manager/configurations
        install -m 0444 ${WORKDIR}/Delta-DPS-550AB-PSU.json ${D}/usr/share/entity-manager/configurations
        install -m 0444 ${WORKDIR}/Chicony-R550-PSU.json ${D}/usr/share/entity-manager/configurations
        install -m 0444 ${WORKDIR}/Broadcom-NicCard.json ${D}/usr/share/entity-manager/configurations
        install -m 0444 ${WORKDIR}/Mellanox-NicCard.json ${D}/usr/share/entity-manager/configurations
        install -m 0444 ${WORKDIR}/fan-table.json ${D}/usr/share/entity-manager/configurations
}
