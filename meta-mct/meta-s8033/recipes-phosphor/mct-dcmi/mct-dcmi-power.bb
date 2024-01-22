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
           file://mct-dcmi-power.cpp \
           file://mct-dcmi-power.hpp \
           file://utils.hpp \
           file://LICENSE \
           file://Makefile.am \
           file://mct-dcmi-power.service \
          "

DEPENDS += "autoconf-archive-native"
DEPENDS += "sdbusplus"
DEPENDS += "phosphor-logging"
DEPENDS += "phosphor-dbus-interfaces"
DEPENDS += "boost"
DEPENDS += "amd-libapmlpower"

RDEPENDS:${PN} += " \
        sdbusplus \
        phosphor-logging \
        phosphor-dbus-interfaces \
        amd-libapmlpower \
        "

SYSTEMD_SERVICE:${PN} = "mct-dcmi-power.service"

PACKAGECONFIG[dbus-mode] = "--enable-dbus-mode"
PACKAGECONFIG:append = "dbus-mode"

do_install() {
    install -d ${D}/usr/sbin
    install -m 0755 mct-dcmi-power ${D}/${sbindir}/
}
