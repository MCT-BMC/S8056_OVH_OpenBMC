FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://PDB-fru.json \
            file://s8056-Baseboard.json \
            file://s8056-DCSCM.json \
            file://s8056-Event-Sensor.json \
            file://s8056-PDB-node-1.json \
            file://s8056-PDB-node-2.json \
            file://s8056-Paddle-Card.json \
            file://s8056-Rear-Riser.json \
            file://s8056-Front-Riser.json \
            file://fan-table.json \
            file://fan-table-node2.json \
            file://fan-table-gpu-sku.json \
            file://fan-table-gpu-sku-node2.json \
            file://blacklist.json \
            file://0001-Support-Fru-related-feature.patch \
            file://0002-Revert-entity-manager-scan-after-NameOwnerChanged.patch \
            file://0004-Modify-configuration-file-missing-probe-log-to-debug.patch \
            file://0005-Add-retry-on-opening-i2c-device-and-remove-rescan-on.patch \
            file://0006-Add-retry-mechanism-at-getting-invalid-fru.json.patch \
            file://fru-device-pre-config.sh \
            file://service_files/xyz.openbmc_project.FruDevice.service;subdir=git/ \
            "

do_install:append(){
	install -d ${D}/usr/share/entity-manager/configurations
	install -m 0444 ${WORKDIR}/*.json ${D}/usr/share/entity-manager/configurations
	install -m 0444 ${WORKDIR}/blacklist.json ${D}/usr/share/entity-manager
}
