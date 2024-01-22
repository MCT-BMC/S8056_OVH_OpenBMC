LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${MCTBASE}/COPYING.apache-2.0;md5=34400b68072d710fecd0a2940a0d1658"
FILESEXTRAPATHS:append := "${THISDIR}/files:"

inherit systemd
inherit obmc-phosphor-systemd

S = "${WORKDIR}"

SRC_URI = " \
           file://stateRecheck.sh \
           file://state-recheck.service \
          "

DEPENDS = "systemd"
RDEPENDS:${PN} = "bash"

SYSTEMD_PACKAGES = "${PN}"
SYSTEMD_SERVICE:${PN} = "state-recheck.service"

do_install() {
    install -d ${D}/usr/sbin
    install -m 0755 ${S}/stateRecheck.sh ${D}/${sbindir}/
}
