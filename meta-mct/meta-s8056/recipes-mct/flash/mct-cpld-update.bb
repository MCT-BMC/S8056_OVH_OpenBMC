SUMMARY = "MiTAC CPLD update handler"
DESCRIPTION = "Implement the cpld update progress"
PR = "r0"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${MCTBASE}/COPYING.apache-2.0;md5=34400b68072d710fecd0a2940a0d1658"

inherit autotools pkgconfig
inherit obmc-phosphor-ipmiprovider-symlink

S = "${WORKDIR}/"

SRC_URI = "file://bootstrap.sh \
           file://configure.ac \
           file://cpld-update.cpp \
           file://cpld-update.hpp \
           file://cpld-update-utils.cpp \
           file://cpld-update-utils.hpp \
           file://cpld-i2c-update-utils.cpp \
           file://cpld-i2c-update-utils.hpp \
           file://cpld-lattice.hpp \
           file://cpld-update-handler \
           file://LICENSE \
           file://Makefile.am \
          "

DEPENDS += "autoconf-archive-native"
DEPENDS += "sdbusplus"
DEPENDS += "phosphor-dbus-interfaces"
DEPENDS += "boost"
DEPENDS += "libgpiod"
DEPENDS += "obmc-libmisc"
DEPENDS += "obmc-libi2c"

RDEPENDS:${PN} += " \
        sdbusplus \
        phosphor-dbus-interfaces \
        libgpiod \
        obmc-libmisc \
        obmc-libi2c \
        "

EXTRA_OECONF:append = " --enable-host-reboot-mode=yes"
EXTRA_OECONF:append = " --enable-gpio-mode=yes"

do_install() {
    install -d ${D}/usr/sbin
    install -m 0755 cpld-update ${D}/${sbindir}/
    install -m 0755 ${S}/cpld-update-handler ${D}/${sbindir}/
}