SUMMARY = "MiTAC SOL Pattern handler"
DESCRIPTION = "Implement the handler for Twitter SOL pattern matching"
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
           file://mct-sol-handler.cpp \
           file://mct-sol-handler.hpp \
           file://LICENSE \
           file://Makefile.am \
           file://mct-sol-handler.service \
          "

DEPENDS += "autoconf-archive-native"
DEPENDS += "sdbusplus"
DEPENDS += "phosphor-logging"
DEPENDS += "phosphor-dbus-interfaces"
DEPENDS += "boost"
DEPENDS += "nlohmann-json"


RDEPENDS:${PN} += " \
        sdbusplus \
        phosphor-logging \
        phosphor-dbus-interfaces \
        "

SYSTEMD_SERVICE:${PN} = "mct-sol-handler.service"

do_install() {
    install -d ${D}/usr/sbin
    install -m 0755 mct-sol-handler ${D}/${sbindir}/
}