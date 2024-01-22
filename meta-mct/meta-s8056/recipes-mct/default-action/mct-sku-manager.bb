SUMMARY = "MiTAC SKU manager"
DESCRIPTION = "Implement the manager for current using SKU"
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
           file://mct-sku-manager.cpp \
           file://mct-sku-manager.hpp \
           file://LICENSE \
           file://Makefile.am \
           file://mct-sku-manager.service \
          "

DEPENDS += "autoconf-archive-native"
DEPENDS += "sdbusplus"
DEPENDS += "phosphor-dbus-interfaces"
DEPENDS += "boost"
DEPENDS += "i2c-tools"


RDEPENDS:${PN} += " \
        sdbusplus \
        phosphor-dbus-interfaces \
        i2c-tools \
        "

SYSTEMD_SERVICE:${PN} = "mct-sku-manager.service"

do_install() {
    install -d ${D}/usr/sbin
    install -m 0755 mct-sku-manager ${D}/${sbindir}/
}