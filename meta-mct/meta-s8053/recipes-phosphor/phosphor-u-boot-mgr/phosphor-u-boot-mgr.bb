SUMMARY = "Phosphor U-Boot environment manager"
DESCRIPTION = "Daemon to read or write U-Boot environment variables"
PR = "r1"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${MCTBASE}/COPYING.apache-2.0;md5=34400b68072d710fecd0a2940a0d1658"

inherit pkgconfig cmake systemd
inherit obmc-phosphor-systemd

S = "${WORKDIR}"

SRC_URI = "file://include/u-boot-env-mgr.hpp \
           file://src/u-boot-env-mgr.cpp \
           file://src/mainapp.cpp \
           file://.clang-format \
           file://CMakeLists.txt \
           file://LICENSE \
           file://xyz.openbmc_project.U_Boot.Environment.Manager.service \
          "

SYSTEMD_SERVICE:${PN} = "xyz.openbmc_project.U_Boot.Environment.Manager.service"

DEPENDS = "boost sdbusplus phosphor-logging"

do_install() {
    install -d ${D}/usr/bin
    install -m 0755 phosphor-u-boot-env-mgr ${D}/${bindir}/
}