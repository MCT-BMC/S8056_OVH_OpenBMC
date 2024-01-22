SUMMARY = "MiTAC BMC2 update handler"
DESCRIPTION = "Implement the bmc2 update progress"
PR = "r1"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${MCTBASE}/COPYING.apache-2.0;md5=34400b68072d710fecd0a2940a0d1658"

inherit autotools pkgconfig
inherit obmc-phosphor-ipmiprovider-symlink

S = "${WORKDIR}"

SRC_URI = "file://bootstrap.sh \
           file://configure.ac \
           file://bmc2-update.cpp \
           file://bmc2-update.hpp \
           file://bmc2-update-handler \
           file://LICENSE \
           file://Makefile.am \
          "

DEPENDS += "autoconf-archive-native"
DEPENDS += "sdbusplus"
DEPENDS += "phosphor-logging"
DEPENDS += "phosphor-dbus-interfaces"
DEPENDS += "boost"
DEPENDS += "libgpiod"


RDEPENDS:${PN} += " \
        sdbusplus \
        phosphor-logging \
        phosphor-dbus-interfaces \
        libgpiod \
        "

do_install() {
    install -d ${D}/usr/sbin
    install -m 0755 bmc2-update ${D}/${sbindir}/
    install -m 0755 ${S}/bmc2-update-handler ${D}/${sbindir}/
}