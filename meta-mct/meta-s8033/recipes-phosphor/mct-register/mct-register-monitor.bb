SUMMARY = "MiTAC Regiser Monitor"
DESCRIPTION = "Implement the regiser monitor"
PR = "r1"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${MCTBASE}/COPYING.apache-2.0;md5=34400b68072d710fecd0a2940a0d1658"

inherit autotools pkgconfig
inherit systemd
inherit obmc-phosphor-systemd
inherit obmc-phosphor-dbus-service

S = "${WORKDIR}"

SRC_URI = "file://bootstrap.sh \
           file://configure.ac \
           file://LICENSE \
           file://Makefile.am \
           file://utils.hpp \
           file://lpc-interrupt-monitor.cpp \
           file://lpc-interrupt-monitor.hpp \
           file://lpc-interrupt-monitor.service \
          "

DEPENDS += "autoconf-archive-native"
DEPENDS += "sdbusplus"
DEPENDS += "phosphor-logging"
DEPENDS += "phosphor-dbus-interfaces"
DEPENDS += "boost"


RDEPENDS:${PN} += " \
        sdbusplus \
        phosphor-logging \
        phosphor-dbus-interfaces \
        "

SYSTEMD_SERVICE:${PN} = "lpc-interrupt-monitor.service"

do_install() {
    install -d ${D}/usr/sbin
    install -m 0755 lpc-interrupt-monitor ${D}/${sbindir}/
}