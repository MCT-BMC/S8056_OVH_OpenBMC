LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${WIWYNNBASE}/COPYING.apache-2.0;md5=34400b68072d710fecd0a2940a0d1658"


SUMMARY = "CPU State Event Log Application"
PR = "r1"

inherit obmc-phosphor-systemd

DEPENDS += "virtual/obmc-gpio-monitor"
RDEPENDS_${PN} += "virtual/obmc-gpio-monitor"

S = "${WORKDIR}"
SRC_URI += "file://prochot_thermtrip_update.sh"

do_install() {
        install -d ${D}${sbindir}
        install -m 0755 ${WORKDIR}/prochot_thermtrip_update.sh \
            ${D}${sbindir}/prochot_thermtrip_update.sh
}

# SYSTEMD_ENVIRONMENT_FILE_${PN} +="obmc/gpio/prochot0"
# SYSTEMD_ENVIRONMENT_FILE_${PN} +="obmc/gpio/prochot0_deassert"
# SYSTEMD_ENVIRONMENT_FILE_${PN} +="obmc/gpio/prochot1"
# SYSTEMD_ENVIRONMENT_FILE_${PN} +="obmc/gpio/prochot1_deassert"
SYSTEMD_ENVIRONMENT_FILE_${PN} +="obmc/gpio/thermtrip0"
SYSTEMD_ENVIRONMENT_FILE_${PN} +="obmc/gpio/thermtrip0_deassert"
SYSTEMD_ENVIRONMENT_FILE_${PN} +="obmc/gpio/thermtrip1"
SYSTEMD_ENVIRONMENT_FILE_${PN} +="obmc/gpio/thermtrip1_deassert"

# PROCHOT0_SERVICE = "prochot0"
# PROCHOT0_DEASSERT_SERVICE = "prochot0_deassert"
# PROCHOT1_SERVICE = "prochot1"
# PROCHOT1_DEASSERT_SERVICE = "prochot1_deassert"
THERMTRIP0_SERVICE = "thermtrip0"
THERMTRIP0_DEASSERT_SERVICE = "thermtrip0_deassert"
THERMTRIP1_SERVICE = "thermtrip1"
THERMTRIP1_DEASSERT_SERVICE = "thermtrip1_deassert"

TMPL = "phosphor-gpio-monitor@.service"
INSTFMT = "phosphor-gpio-monitor@{0}.service"
TGT = "${SYSTEMD_DEFAULT_TARGET}"
FMT = "../${TMPL}:${TGT}.requires/${INSTFMT}"

# SYSTEMD_SERVICE_${PN} += "xyz.openbmc_project.prochot0.service"
# SYSTEMD_SERVICE_${PN} += "xyz.openbmc_project.prochot0_deassert.service"
# SYSTEMD_SERVICE_${PN} += "xyz.openbmc_project.prochot1.service"
# SYSTEMD_SERVICE_${PN} += "xyz.openbmc_project.prochot1_deassert.service"
SYSTEMD_SERVICE_${PN} += "xyz.openbmc_project.thermtrip0.service"
SYSTEMD_SERVICE_${PN} += "xyz.openbmc_project.thermtrip0_deassert.service"
SYSTEMD_SERVICE_${PN} += "xyz.openbmc_project.thermtrip1.service"
SYSTEMD_SERVICE_${PN} += "xyz.openbmc_project.thermtrip1_deassert.service"
# SYSTEMD_LINK_${PN} += "${@compose_list(d, 'FMT', 'PROCHOT0_SERVICE')}"
# SYSTEMD_LINK_${PN} += "${@compose_list(d, 'FMT', 'PROCHOT0_DEASSERT_SERVICE')}"
# SYSTEMD_LINK_${PN} += "${@compose_list(d, 'FMT', 'PROCHOT1_SERVICE')}"
# SYSTEMD_LINK_${PN} += "${@compose_list(d, 'FMT', 'PROCHOT1_DEASSERT_SERVICE')}"
SYSTEMD_LINK_${PN} += "${@compose_list(d, 'FMT', 'THERMTRIP0_SERVICE')}"
SYSTEMD_LINK_${PN} += "${@compose_list(d, 'FMT', 'THERMTRIP0_DEASSERT_SERVICE')}"
SYSTEMD_LINK_${PN} += "${@compose_list(d, 'FMT', 'THERMTRIP1_SERVICE')}"
SYSTEMD_LINK_${PN} += "${@compose_list(d, 'FMT', 'THERMTRIP1_DEASSERT_SERVICE')}"

