SUMMARY = "MCT System watchdog for FMC WDT2"
DESCRIPTION = "BMC hardware watchdog service for FMC WDT2 that is used to reset BMC \
               when unrecoverable events occurs"
PR = "r1"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${MCTBASE}/COPYING.apache-2.0;md5=34400b68072d710fecd0a2940a0d1658"

inherit autotools pkgconfig
inherit obmc-phosphor-ipmiprovider-symlink
inherit systemd
inherit obmc-phosphor-systemd

S = "${WORKDIR}"

SRC_URI = "file://bootstrap.sh \
           file://configure.ac \
           file://system-fmc-watchdog.cpp \
           file://LICENSE \
           file://Makefile.am \
           file://system-fmc-watchdog.service \
          "

DEPENDS += "autoconf-archive-native"
DEPENDS += "boost"
DEPENDS += "obmc-libmisc"
RDEPENDS:${PN} += "obmc-libmisc"

SYSTEMD_SERVICE:${PN} = "system-fmc-watchdog.service"
SYSTEMD_ENVIRONMENT_FILE:${PN} += "conf/system-fmc-watchdog.conf"

do_install() {
    install -d ${D}/usr/sbin
    install -m 0755 system-fmc-watchdog ${D}/${sbindir}/
}