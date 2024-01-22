SUMMARY = "MiTAC FAN status LED handelr"
DESCRIPTION = "Implement handler for FAN status LED"
PR = "r1"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${MCTBASE}/COPYING.apache-2.0;md5=34400b68072d710fecd0a2940a0d1658"

inherit autotools pkgconfig
inherit obmc-phosphor-ipmiprovider-symlink
inherit systemd
inherit obmc-phosphor-systemd


S = "${WORKDIR}"

SRC_URI = "file://bootstrap.sh \
           file://configure.ac \
           file://mct-fan-status-led-handler.cpp \
           file://mct-fan-status-led-handler.hpp \
           file://LICENSE \
           file://Makefile.am \
           file://mct-fan-status-led-handler.service \
          "

DEPENDS += "autoconf-archive-native"
DEPENDS += "sdbusplus"
DEPENDS += "phosphor-dbus-interfaces"
DEPENDS += "boost"
DEPENDS += "libgpiod"

RDEPENDS:${PN} += " \
        sdbusplus \
        phosphor-dbus-interfaces \
        "

SYSTEMD_SERVICE:${PN} = "mct-fan-status-led-handler.service"

do_install() {
    install -d ${D}/usr/sbin
    install -m 0755 mct-fan-status-led-handler ${D}/${sbindir}/
}
