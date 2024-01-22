FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://led-group-node1-config.json"
SRC_URI += "file://led-group-node2-config.json"

PACKAGECONFIG:append = "use-json"

do_install:append() {
        install -m 0644 ${WORKDIR}/led-group-node1-config.json ${D}${datadir}/phosphor-led-manager/
        install -m 0644 ${WORKDIR}/led-group-node2-config.json ${D}${datadir}/phosphor-led-manager/
}