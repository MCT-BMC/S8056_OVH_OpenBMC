SUMMARY = "MiTAC environment manager"
DESCRIPTION = "Implement the manager for environment"
PR = "r1"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${MCTBASE}/COPYING.apache-2.0;md5=34400b68072d710fecd0a2940a0d1658"

inherit autotools pkgconfig
inherit systemd
inherit obmc-phosphor-systemd

S = "${WORKDIR}"

SRC_URI = "file://bootstrap.sh \
           file://configure.ac \
           file://Makefile.am \
           file://mct-env-manager.cpp \
           file://mct-env-manager.hpp \
           file://env-config.json \
           file://mct-env-manager.service \
           "

DEPENDS = "systemd"
DEPENDS += "autoconf-archive-native"
DEPENDS += "nlohmann-json"

SYSTEMD_SERVICE:${PN} = "mct-env-manager.service"


do_install() {
    install -d ${D}/usr/sbin
    install -m 0755 mct-env-manager ${D}/${sbindir}/
    install -d ${D}/usr/share/mct-env-manager
    install -m 0755 ${S}/env-config.json ${D}/${datadir}/mct-env-manager/
}
