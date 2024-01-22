SUMMARY = "MiTAC VR update handler"
DESCRIPTION = "Implement the VR update progress"
PR = "r1"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${MCTBASE}/COPYING.apache-2.0;md5=34400b68072d710fecd0a2940a0d1658"

inherit autotools pkgconfig
inherit obmc-phosphor-ipmiprovider-symlink

S = "${WORKDIR}"

SRC_URI = "file://bootstrap.sh \
           file://configure.ac \
           file://vr-update.cpp \
           file://vr-update.hpp \
           file://misc-utils.hpp \
           file://vr-update-manager.cpp \
           file://vr-update-manager.hpp \
           file://vr-update-utils.cpp \
           file://vr-update-utils.hpp \
           file://vr-update-handler \
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
    install -m 0755 vr-update ${D}/${sbindir}/
    install -m 0755 ${S}/vr-update-handler ${D}/${sbindir}/
}