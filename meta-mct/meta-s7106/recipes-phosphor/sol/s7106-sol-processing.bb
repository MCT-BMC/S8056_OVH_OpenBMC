SUMMARY = "SOL Processing"
DESCRIPTION = "SOL Processing from Twitter"
PR = "r4"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${MCTBASE}/COPYING.apache-2.0;md5=34400b68072d710fecd0a2940a0d1658"

inherit autotools pkgconfig
inherit obmc-phosphor-ipmiprovider-symlink
inherit pythonnative
inherit systemd
inherit obmc-phosphor-systemd

S = "${WORKDIR}/"

SRC_URI = "file://bootstrap.sh \
           file://configure.ac \
           file://sol_processing.cpp \
           file://LICENSE \
           file://Makefile.am \
           file://xyz.openbmc_project.oem.sol-processing.service \
           file://sol_pattern \
           "

DEPENDS += "autoconf-archive-native"
DEPENDS += "sdbusplus sdbusplus-native"
DEPENDS += "phosphor-logging"
DEPENDS += "phosphor-dbus-interfaces phosphor-dbus-interfaces-native"
DEPENDS += "sdbus++-native"

RDEPENDS_${PN} += " \
        sdbusplus \
        phosphor-logging \
        phosphor-dbus-interfaces \
        "

SYSTEMD_SERVICE_${PN} = "xyz.openbmc_project.oem.sol-processing.service"

do_install() {
    install -d ${D}/etc
    install -m 0644 ${S}/sol_pattern ${D}/etc/sol_pattern
    install -d ${D}/usr/sbin
    install -m 0755 ${S}/build/sol_processing ${D}/${sbindir}/
}
