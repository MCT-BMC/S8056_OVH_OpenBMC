SUMMARY = "MiTAC dual BIOS handler"
DESCRIPTION = "Implement the handler for dual bios setting"
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
           file://mct-dual-bios-handler.cpp \
           file://mct-dual-bios-handler.hpp \
           file://mct-dual-bios-handler.service \
           file://setting-dual-bios.sh \
           file://setting-dual-bios.service \
           "

DEPENDS += "autoconf-archive-native"
DEPENDS += "sdbusplus"
DEPENDS += "phosphor-dbus-interfaces"
DEPENDS += "boost"
DEPENDS += "libgpiod"
DEPENDS += "obmc-libmisc"


RDEPENDS:${PN} += " \
                   sdbusplus \
                   phosphor-dbus-interfaces \
                   libgpiod \
                   obmc-libmisc \
                  "

SYSTEMD_SERVICE:${PN} = "mct-dual-bios-handler.service \
                         setting-dual-bios.service \
                        "

do_install() {
    install -d ${D}/usr/sbin
    install -m 0755 mct-dual-bios-handler ${D}/${sbindir}/
    install -m 0755 ${S}/setting-dual-bios.sh ${D}/${sbindir}/
}