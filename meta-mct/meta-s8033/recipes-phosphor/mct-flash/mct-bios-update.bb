SUMMARY = "MiTAC BIOS update handler"
DESCRIPTION = "Implement the bios update progress"
PR = "r1"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${MCTBASE}/COPYING.apache-2.0;md5=34400b68072d710fecd0a2940a0d1658"

inherit autotools pkgconfig
inherit obmc-phosphor-ipmiprovider-symlink

S = "${WORKDIR}"

SRC_URI = "file://bootstrap.sh \
           file://configure.ac \
           file://bios-update.cpp \
           file://bios-update.hpp \
           file://bios-update-handler \
           file://LICENSE \
           file://Makefile.am \
          "

DEPENDS += "autoconf-archive-native"
DEPENDS += "sdbusplus"
DEPENDS += "phosphor-logging"
DEPENDS += "phosphor-dbus-interfaces"
DEPENDS += "boost"
DEPENDS += "libgpiod"
DEPENDS += "obmc-libmisc"


RDEPENDS:${PN} += " \
        sdbusplus \
        phosphor-logging \
        phosphor-dbus-interfaces \
        libgpiod \
        obmc-libmisc \
        "

PACKAGECONFIG[spi-mode] = "--enable-spi-mode"
PACKAGECONFIG:append = "spi-mode"

do_install() {
    install -d ${D}/usr/sbin
    install -m 0755 bios-update ${D}/${sbindir}/
    install -m 0755 ${S}/bios-update-handler ${D}/${sbindir}/
}