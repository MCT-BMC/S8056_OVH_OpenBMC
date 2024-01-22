PR = "r1"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${MCTBASE}/COPYING.apache-2.0;md5=34400b68072d710fecd0a2940a0d1658"
inherit systemd
inherit obmc-phosphor-systemd


SRC_URI = " file://phosphor-ipmi-flash-bmc-verify.service \
            file://bmc-verify.sh \
            file://config-alt-bmc.json \
            file://image-verify.sh \
            file://phosphor-ipmi-flash-alt-bmc-verify.service \
            file://phosphor-ipmi-flash-alt-bmc-update.service \
          "

DEPENDS += "systemd"
DEPENDS += "phosphor-ipmi-flash"
RDEPENDS:${PN} = "bash"

FILES:${PN} += "${datadir}/phosphor-ipmi-flash/config-alt-bmc.json"

SYSTEMD_PACKAGES = "${PN}"
SYSTEMD_SERVICE:${PN}:append = " phosphor-ipmi-flash-bmc-verify.service"
SYSTEMD_SERVICE:${PN}:append = " phosphor-ipmi-flash-alt-bmc-verify.service"
SYSTEMD_SERVICE:${PN}:append = " phosphor-ipmi-flash-alt-bmc-update.service"

do_install() {
    install -d ${D}${bindir}
    install -m 0755 ${WORKDIR}/bmc-verify.sh ${D}${bindir}/
    install -m 0755 ${WORKDIR}/image-verify.sh ${D}${bindir}/

    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/phosphor-ipmi-flash-bmc-verify.service ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/phosphor-ipmi-flash-alt-bmc-verify.service ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/phosphor-ipmi-flash-alt-bmc-update.service ${D}${systemd_system_unitdir}

    install -d ${D}${datadir}/phosphor-ipmi-flash
    install -m 0644 ${WORKDIR}/config-alt-bmc.json ${D}${datadir}/phosphor-ipmi-flash
}

