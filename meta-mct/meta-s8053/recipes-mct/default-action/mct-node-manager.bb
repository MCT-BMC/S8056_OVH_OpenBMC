SUMMARY = "MiTAC node manager"
DESCRIPTION = "Implement the manager for current using node"
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
           file://mct-node-manager.cpp \
           file://mct-node-manager.hpp \
           file://LICENSE \
           file://Makefile.am \
           file://mct-node-manager.service \
           file://setting-led-table.sh \
           file://setting-led-table.service \
          "

DEPENDS += "autoconf-archive-native"
DEPENDS += "boost"
DEPENDS += "sdbusplus"
DEPENDS += "phosphor-dbus-interfaces"
DEPENDS += "libgpiod"
DEPENDS += "i2c-tools"
DEPENDS += "nlohmann-json"

RDEPENDS:${PN} += " \
        sdbusplus \
        phosphor-dbus-interfaces \
        libgpiod \
        i2c-tools \
        "

SYSTEMD_SERVICE:${PN} = "\
                         mct-node-manager.service \
                         setting-led-table.service \
                         "

do_install() {
    install -d ${D}/usr/sbin
    install -m 0755 mct-node-manager ${D}/${sbindir}/
    install -m 0755 ${S}/setting-led-table.sh ${D}/${sbindir}/
}