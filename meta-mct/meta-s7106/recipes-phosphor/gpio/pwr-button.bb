SUMMARY = "Power Button pressed application"
PR = "r1"

inherit obmc-phosphor-systemd
#inherit autotools pkgconfig
#inherit obmc-phosphor-ipmiprovider-symlink
#inherit pythonnative
#inherit systemd
#inherit obmc-phosphor-systemd


LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${MCTBASE}/COPYING.apache-2.0;md5=34400b68072d710fecd0a2940a0d1658"

DEPENDS += "virtual/obmc-gpio-monitor"
RDEPENDS_${PN} += "virtual/obmc-gpio-monitor"

S = "${WORKDIR}"
SRC_URI += "file://pwrbutton_update.sh \
            file://xyz.openbmc_project.pwrbutton.service "

#DEPENDS += "autoconf-archive-native"
#DEPENDS += "sdbusplus sdbusplus-native"
#DEPENDS += "phosphor-logging"
#DEPENDS += "phosphor-dbus-interfaces phosphor-dbus-interfaces-native"
#DEPENDS += "sdbus++-native"

do_install() {
    install -d ${D}/usr/sbin
    install -m 0755 ${WORKDIR}/pwrbutton_update.sh ${D}/${sbindir}/
}

SYSTEMD_ENVIRONMENT_FILE_${PN} +="obmc/gpio/pwrbutton"

PWRBUTTON_SERVICE = "pwrbutton"

TMPL = "phosphor-gpio-monitor@.service"
INSTFMT = "phosphor-gpio-monitor@{0}.service"
TGT = "${SYSTEMD_DEFAULT_TARGET}"
FMT = "../${TMPL}:${TGT}.requires/${INSTFMT}"

SYSTEMD_SERVICE_${PN} += "xyz.openbmc_project.pwrbutton.service"
SYSTEMD_LINK_${PN} += "${@compose_list(d, 'FMT', 'PWRBUTTON_SERVICE')}"



