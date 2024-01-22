FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://s8033-Baseboard.json \
            file://s8033-Baseboard-gpu.json \
            file://Chicony-R800-PSU.json \
            file://Delta-DPS-550AB-PSU.json \
            file://Delta-DPS-800AB-PSU.json \
            file://s8033-PDB.json \
            file://s8033-CRPS-fru.json \
            file://s8033-PDB-fru.json \
            file://fan-table.json \
            file://fan-table-gpu-sku.json \
            file://blacklist.json \
            file://0001-Support-Fru-related-feature.patch \
            file://0002-Revert-entity-manager-scan-after-NameOwnerChanged.patch \
            file://0003-Modify-configuration-file-missing-probe-log-to-debug.patch \
            "

do_install:append(){
	install -d ${D}/usr/share/entity-manager/configurations
	install -m 0444 ${WORKDIR}/*.json ${D}/usr/share/entity-manager/configurations
	install -m 0444 ${WORKDIR}/blacklist.json ${D}/usr/share/entity-manager
}
