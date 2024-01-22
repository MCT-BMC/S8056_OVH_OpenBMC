SUMMARY = "MiTAC firmware version handler"
DESCRIPTION = "Implement firmware version handler"
PR = "r1"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${MCTBASE}/COPYING.apache-2.0;md5=34400b68072d710fecd0a2940a0d1658"

FILESEXTRAPATHS:append := "${THISDIR}/${PN}:"

inherit autotools pkgconfig
inherit systemd
inherit obmc-phosphor-systemd

S = "${WORKDIR}"

SRC_URI = "file://bootstrap.sh \
           file://configure.ac \
           file://Makefile.am \
           file://mct-bios-version-handler.cpp \
           file://mct-cpld-version-handler.cpp \
           "

DEPENDS = "systemd"
DEPENDS += "autoconf-archive-native"
DEPENDS += "sdbusplus"
DEPENDS += "phosphor-dbus-interfaces"
DEPENDS += "i2c-tools"
DEPENDS += "nlohmann-json"

RDEPENDS:${PN} = "bash"
RDEPENDS:${PN} += "sdbusplus"
RDEPENDS:${PN} += "phosphor-logging"
RDEPENDS:${PN} += "phosphor-dbus-interfaces"
RDEPENDS:${PN} += "i2c-tools"

SYSTEMD_PACKAGES = "${PN}"
SYSTEMD_SERVICE:${PN} = " \
                         mct-cpld-version-handler.service \
                         "

do_install() {
    install -d ${D}/usr/sbin
    install -m 0755 mct-bios-version-handler ${D}/${sbindir}/
    install -m 0755 mct-cpld-version-handler ${D}/${sbindir}/
}
