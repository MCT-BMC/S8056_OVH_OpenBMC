SUMMARY = "CATEER Trigger application"
PR = "r1"

inherit obmc-phosphor-systemd
inherit autotools pkgconfig
inherit obmc-phosphor-ipmiprovider-symlink
inherit pythonnative
inherit systemd
inherit obmc-phosphor-systemd


LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${MCTBASE}/COPYING.apache-2.0;md5=34400b68072d710fecd0a2940a0d1658"

DEPENDS += "virtual/obmc-gpio-monitor"
RDEPENDS_${PN} += "virtual/obmc-gpio-monitor"

S = "${WORKDIR}"
SRC_URI += "file://bootstrap.sh \
            file://configure.ac \
            file://caterr_update.cpp \
            file://LICENSE \
            file://Makefile.am \
            file://caterr_update.sh \
            file://xyz.openbmc_project.caterr.service \
            file://xyz.openbmc_project.caterr_deassert.service "

DEPENDS += "autoconf-archive-native"
DEPENDS += "sdbusplus sdbusplus-native"
DEPENDS += "phosphor-logging"
DEPENDS += "phosphor-dbus-interfaces phosphor-dbus-interfaces-native"
DEPENDS += "sdbus++-native"

do_install() {
    install -d ${D}/usr/sbin
    install -m 0755 ${WORKDIR}/caterr_update.sh ${D}/${sbindir}/
}

SYSTEMD_ENVIRONMENT_FILE_${PN} +="obmc/gpio/caterr"
SYSTEMD_ENVIRONMENT_FILE_${PN} +="obmc/gpio/caterr_deassert"

CATERR_SERVICE = "caterr"
CATERR_DEASSERT_SERVICE = "caterr_deassert"

TMPL = "phosphor-gpio-monitor@.service"
INSTFMT = "phosphor-gpio-monitor@{0}.service"
TGT = "${SYSTEMD_DEFAULT_TARGET}"
FMT = "../${TMPL}:${TGT}.requires/${INSTFMT}"

SYSTEMD_SERVICE_${PN} += "xyz.openbmc_project.caterr.service"
SYSTEMD_SERVICE_${PN} += "xyz.openbmc_project.caterr_deassert.service"
SYSTEMD_LINK_${PN} += "${@compose_list(d, 'FMT', 'CATERR_SERVICE')}"
SYSTEMD_LINK_${PN} += "${@compose_list(d, 'FMT', 'CATERR_DEASSERT_SERVICE')}"



