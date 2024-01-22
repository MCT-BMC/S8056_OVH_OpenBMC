PR = "r1"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${MCTBASE}/COPYING.apache-2.0;md5=34400b68072d710fecd0a2940a0d1658"

inherit systemd
inherit obmc-phosphor-systemd

SRC_URI = " \
            file://config-bios.json \
            file://phosphor-ipmi-flash-bios-update.service \
            file://config-bios2.json \
            file://phosphor-ipmi-flash-bios2-update.service \
          "

DEPENDS += "systemd"
DEPENDS += "phosphor-ipmi-flash"
RDEPENDS:${PN} = "bash"

FILES:${PN} += "${datadir}/phosphor-ipmi-flash/config-bios.json"
FILES:${PN} += "${datadir}/phosphor-ipmi-flash/config-bios2.json"

SYSTEMD_PACKAGES = "${PN}"
SYSTEMD_SERVICE:${PN}:append = " phosphor-ipmi-flash-bios-update.service"
SYSTEMD_SERVICE:${PN}:append = " phosphor-ipmi-flash-bios2-update.service"

do_install() {
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/phosphor-ipmi-flash-bios-update.service ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/phosphor-ipmi-flash-bios2-update.service ${D}${systemd_system_unitdir}

    install -d ${D}${datadir}/phosphor-ipmi-flash
    install -m 0644 ${WORKDIR}/config-bios.json ${D}${datadir}/phosphor-ipmi-flash
    install -m 0644 ${WORKDIR}/config-bios2.json ${D}${datadir}/phosphor-ipmi-flash
}

