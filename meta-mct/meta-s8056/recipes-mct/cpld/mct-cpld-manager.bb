SUMMARY = "MiTAC CPLD manager"
DESCRIPTION = "Implement CPLD manager via I2C interface"
PR = "r1"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${MCTBASE}/COPYING.apache-2.0;md5=34400b68072d710fecd0a2940a0d1658"

inherit autotools pkgconfig
inherit systemd
inherit obmc-phosphor-systemd

S = "${WORKDIR}"

SRC_URI = "file://bootstrap.sh \
           file://configure.ac \
           file://mct-cpld-manager.cpp \
           file://mct-cpld-manager.hpp \
           file://LICENSE \
           file://Makefile.am \
          "

DEPENDS += "autoconf-archive-native"
DEPENDS += "systemd"
DEPENDS += "sdbusplus"
DEPENDS += "phosphor-dbus-interfaces"
DEPENDS += "boost"
DEPENDS += "obmc-libi2c"

RDEPENDS:${PN} += " \
        libsystemd \
        sdbusplus \
        phosphor-dbus-interfaces \
        obmc-libi2c \
        "

SYSTEMD_SERVICE:${PN} = "mct-cpld-manager.service"

do_install() {
    install -d ${D}/usr/sbin
    install -m 0755 mct-cpld-manager ${D}/${sbindir}/
}