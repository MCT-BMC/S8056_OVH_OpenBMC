SUMMARY = "MiTAC PSU update handler"
DESCRIPTION = "Implement the psu update progress"
PR = "r1"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${MCTBASE}/COPYING.apache-2.0;md5=34400b68072d710fecd0a2940a0d1658"

inherit autotools pkgconfig
inherit obmc-phosphor-ipmiprovider-symlink

S = "${WORKDIR}"

SRC_URI = "file://bootstrap.sh \
           file://configure.ac \
           file://psu-update.cpp \
           file://psu-update.hpp \
           file://chicony-psu-update-manager.cpp \
           file://chicony-psu-update-manager.hpp \
           file://delta-psu-update-manager.cpp \
           file://delta-psu-update-manager.hpp \
           file://misc-utils.hpp \
           file://psu-update-utils.cpp \
           file://psu-update-utils.hpp \
           file://psu-update-handler \
           file://LICENSE \
           file://Makefile.am \
          "

DEPENDS += "autoconf-archive-native"
DEPENDS += "sdbusplus"
DEPENDS += "phosphor-dbus-interfaces"
DEPENDS += "boost"
DEPENDS += "obmc-libmisc"
DEPENDS += "obmc-libi2c"


RDEPENDS:${PN} += " \
        sdbusplus \
        phosphor-dbus-interfaces \
        obmc-libmisc \
        obmc-libi2c \
        "

do_install() {
    install -d ${D}/usr/sbin
    install -m 0755 psu-update ${D}/${sbindir}/
    install -m 0755 ${S}/psu-update-handler ${D}/${sbindir}/
}
