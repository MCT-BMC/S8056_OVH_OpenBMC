SUMMARY = "cpld updater"
DESCRIPTION = "Command line tool to update cpld firmware"
PR = "r1"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://COPYING.MIT;md5=3da9cfbcb788c80a0384361b4de20420"

TARGET_CC_ARCH += "${LDFLAGS}"

S = "${WORKDIR}"

DEPENDS += "systemd sdbusplus obmc-libmisc obmc-libjtag libgpiod obmc-libdbus"
RDEPENDS_${PN} += "libsystemd obmc-libmisc obmc-libjtag obmc-libdbus"

SRC_URI = "file://Makefile \
           file://main.cpp \
           file://cpld-updater.hpp \
           file://cpld-updater.cpp \
           file://cpld-lattice.hpp \
           file://COPYING.MIT \
          "

# inherit obmc-phosphor-systemd
# SYSTEMD_PACKAGES = "${PN}"
# SYSTEMD_SERVICE_${PN} = "xyz.openbmc_project.cpld-updater@.service"

do_install() {
    install -d ${D}${sbindir}
    install -Dm755 ${S}/cpld-updater ${D}${sbindir}/cpld-updater
}
