SUMMARY = "Heart beat LED control"
DESCRIPTION = "Heart beat LED blinking control"
PR = "r1"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${MCTBASE}/COPYING.apache-2.0;md5=34400b68072d710fecd0a2940a0d1658"

FILESEXTRAPATHS:append := "${THISDIR}/${PN}:"

inherit systemd
inherit obmc-phosphor-systemd

S = "${WORKDIR}"

SRC_URI = "file://hbled-blink.sh \
           file://hbled-blink.service"

DEPENDS = "systemd"
RDEPENDS:${PN} = "bash"

SYSTEMD_PACKAGES = "${PN}"
SYSTEMD_SERVICE:${PN} = "hbled-blink.service"

do_install() {
    install -d ${D}/usr/sbin
    install -m 0755 ${S}/hbled-blink.sh ${D}/${sbindir}/
}
