SUMMARY = "MiTAC leaky bucket"
DESCRIPTION = "Implement the mechanism for leaky bucket"
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
           file://mct-leaky-bucket.cpp \
           file://mct-leaky-bucket.hpp \
           file://lcp-utils.hpp \
           file://mct-leaky-bucket.service \
           "

DEPENDS += "autoconf-archive-native"
DEPENDS += "sdbusplus"
DEPENDS += "phosphor-dbus-interfaces"
DEPENDS += "boost"


RDEPENDS:${PN} += " \
        sdbusplus \
        phosphor-dbus-interfaces \
        "

SYSTEMD_SERVICE:${PN} = "mct-leaky-bucket.service"

do_install() {
    install -d ${D}/usr/sbin
    install -m 0755 mct-leaky-bucket ${D}/${sbindir}/
}