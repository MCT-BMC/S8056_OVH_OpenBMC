SUMMARY = "MiTAC firmware update handler"
DESCRIPTION = "Implement the firmware update progress"
PR = "r1"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${MCTBASE}/COPYING.apache-2.0;md5=34400b68072d710fecd0a2940a0d1658"

S = "${WORKDIR}"

SRC_URI = "file://image-active \
           file://flash-progress-update \
          "

DEPENDS = "systemd"
RDEPENDS:${PN} = "bash"

do_install() {
    install -d ${D}/usr/sbin
    install -m 0755 ${S}/image-active ${D}/${sbindir}/
    install -m 0755 ${S}/flash-progress-update ${D}/${sbindir}/
}