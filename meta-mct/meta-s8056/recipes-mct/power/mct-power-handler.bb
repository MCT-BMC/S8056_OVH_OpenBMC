SUMMARY = "MiTAC Power state handler"
DESCRIPTION = "Implement the handler for Power state"
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
           file://mct-power-handler.cpp \
           file://mct-power-handler.hpp \
           file://mct-power-handler.service \
           file://obmc-chassis-poweroff.target \
           file://obmc-chassis-poweron.target \
           file://obmc-host-poweroff.target \
           file://obmc-host-poweron.target \
           "

DEPENDS += "autoconf-archive-native"
DEPENDS += "sdbusplus"
DEPENDS += "phosphor-dbus-interfaces"
DEPENDS += "boost"


RDEPENDS:${PN} += " \
        sdbusplus \
        phosphor-dbus-interfaces \
        "

SYSTEMD_SERVICE:${PN} = "mct-power-handler.service \
                         obmc-chassis-poweroff.target \
                         obmc-chassis-poweron.target \
                         obmc-host-poweroff.target \
                         obmc-host-poweron.target \
                        "

do_install() {
    install -d ${D}/usr/sbin
    install -m 0755 mct-power-handler ${D}/${sbindir}/
}