SUMMARY = "MiTAC initializing default action"
DESCRIPTION = "Implement initializing default action"
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
           file://mct-default-setting.cpp \
           file://mct-default-setting.hpp \
           file://mct-default-setting.service \
           file://dhcp-check.service \
           file://dhcp-check.sh \
           file://bmc-reboot-sel.sh \
           file://bmc-reboot-sel.service \
           file://psu-status-sel.sh \
           file://psu0-status-sel.service \
           file://psu1-status-sel.service \
           file://pdb-status-sel.service \
           file://service-status-check.sh \
           file://set-default-pwm.sh \
           file://get-default-psu-voltage.sh \
           file://mct-bmc-booting-finish.service \
           file://virtual-sensor-check.sh \
           file://check-service-running.sh \
           file://setting-hsc-register.sh \
           file://setting-hsc-register@.service \
           file://setting-fru-table.sh \
           "

DEPENDS = "systemd"
DEPENDS += "autoconf-archive-native"
DEPENDS += "sdbusplus"
DEPENDS += "phosphor-dbus-interfaces"
DEPENDS += "boost"
DEPENDS += "obmc-libmisc"
DEPENDS += "nlohmann-json"

RDEPENDS:${PN} = "bash"
RDEPENDS:${PN} += "sdbusplus"
RDEPENDS:${PN} += "phosphor-logging"
RDEPENDS:${PN} += "phosphor-dbus-interfaces"
RDEPENDS:${PN} += "obmc-libmisc"

SYSTEMD_PACKAGES = "${PN}"
SYSTEMD_SERVICE:${PN} = " \
                         dhcp-check.service \
                         bmc-reboot-sel.service \
                         psu0-status-sel.service \
                         psu1-status-sel.service \
                         pdb-status-sel.service \
                         mct-default-setting.service \
                         mct-bmc-booting-finish.service \
                         setting-hsc-register@.service \
                         "

do_install() {
    install -d ${D}/usr/sbin
    install -m 0755 mct-default-setting ${D}/${sbindir}/
    install -m 0755 ${S}/dhcp-check.sh ${D}/${sbindir}/
    install -m 0755 ${S}/bmc-reboot-sel.sh ${D}/${sbindir}/
    install -m 0755 ${S}/psu-status-sel.sh ${D}/${sbindir}/
    install -m 0755 ${S}/service-status-check.sh ${D}/${sbindir}/
    install -m 0755 ${S}/set-default-pwm.sh ${D}/${sbindir}/
    install -m 0755 ${S}/get-default-psu-voltage.sh ${D}/${sbindir}/
    install -m 0755 ${S}/virtual-sensor-check.sh ${D}/${sbindir}/
    install -m 0755 ${S}/check-service-running.sh ${D}/${sbindir}/
    install -m 0755 ${S}/setting-hsc-register.sh ${D}/${sbindir}/
    install -m 0755 ${S}/setting-fru-table.sh ${D}/${sbindir}/
}
