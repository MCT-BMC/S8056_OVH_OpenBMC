DESCRIPTION = "BMC RTC time sync"
PR = "r1"

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${MCTBASE}/COPYING.apache-2.0;md5=34400b68072d710fecd0a2940a0d1658"

FILESEXTRAPATHS:append := "${THISDIR}/${PN}:"

inherit systemd
inherit obmc-phosphor-systemd
inherit cmake

S = "${WORKDIR}"

SRC_URI = "file://bmc-rtc-time-sync.cpp \
           file://CMakeLists.txt \
           file://bmc-rtc-time-sync.service \
           file://bmc-rtc-time-sync.timer \
           "

DEPENDS = "systemd"
RDEPENDS:${PN} = "bash"

SYSTEMD_PACKAGES = "${PN}"
SYSTEMD_SERVICE:${PN} = "bmc-rtc-time-sync.service \
                         bmc-rtc-time-sync.timer \
                         "

do_install() {
    install -d ${D}/usr/sbin
    install -m 0755 bmc-rtc-time-sync ${D}/${sbindir}/
}
