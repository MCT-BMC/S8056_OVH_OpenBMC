LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${WIWYNNBASE}/COPYING.apache-2.0;md5=34400b68072d710fecd0a2940a0d1658"

FILESEXTRAPATHS_append := "${THISDIR}/files:"

inherit systemd
inherit obmc-phosphor-systemd

DEPENDS += "systemd"
RDEPENDS_${PN} += "bash"

SYSTEMD_PACKAGES = "${PN}"
SYSTEMD_SERVICE_${PN} = "sv305-gpio-init.service"

S = "${WORKDIR}"
SRC_URI = "file://sv305-gpio-init.sh \
           file://sv305-gpio-init.service \
          "

do_install() {
    install -d ${D}${sbindir}
    install -m 0755 ${S}/sv305-gpio-init.sh ${D}${sbindir}/
}
