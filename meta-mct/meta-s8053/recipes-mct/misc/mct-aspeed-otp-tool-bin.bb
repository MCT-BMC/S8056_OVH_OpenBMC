SUMMARY = "MiTAC aspeed otp tool"
DESCRIPTION = "Implement aspeed otp tool"
PR = "r1"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${MCTBASE}/COPYING.apache-2.0;md5=34400b68072d710fecd0a2940a0d1658"

inherit systemd

S = "${WORKDIR}"

SRC_URI = "file://mct-aspeed-otp-tool \
          "

SRCREV = "${AUTOREV}"

RDEPENDS:${PN} += " \
        libsystemd \
        sdbusplus \
        phosphor-dbus-interfaces \
        obmc-libmisc \
        "

INSANE_SKIP:${PN} += "already-stripped"

do_install() {
    install -d ${D}/usr/sbin
    install -m 0755 mct-aspeed-otp-tool ${D}/${sbindir}/
}