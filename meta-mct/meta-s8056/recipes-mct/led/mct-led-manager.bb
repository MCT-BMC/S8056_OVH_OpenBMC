SUMMARY = "MiTAC DCMI Power"
DESCRIPTION = "Implement the power for IPMI DCMI"
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
           file://mct-led-manager.cpp \
           file://mct-led-manager.hpp \
           file://LICENSE \
           file://Makefile.am \
           file://mct-led-manager.service \
          "

DEPENDS += "autoconf-archive-native"
DEPENDS += "sdbusplus"
DEPENDS += "phosphor-dbus-interfaces"
DEPENDS += "boost"


RDEPENDS:${PN} += " \
        sdbusplus \
        phosphor-dbus-interfaces \
        "

SYSTEMD_SERVICE:${PN} = "mct-led-manager.service"

do_install() {
    install -d ${D}/usr/sbin
    install -m 0755 mct-led-manager ${D}/${sbindir}/
}