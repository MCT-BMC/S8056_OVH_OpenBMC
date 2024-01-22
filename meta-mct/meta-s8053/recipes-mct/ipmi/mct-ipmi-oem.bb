SUMMARY = "MiTAC OEM IPMI commands"
DESCRIPTION = "MiTAC OEM Commands"
PR = "r1"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${MCTBASE}/COPYING.apache-2.0;md5=34400b68072d710fecd0a2940a0d1658"

inherit cmake obmc-phosphor-ipmiprovider-symlink pkgconfig

S = "${WORKDIR}"

SRC_URI = "file://CMakeLists.txt\
           file://LICENSE \
           file://oem-cmd.cpp \
           file://oem-cmd.hpp \
           file://tyan-oem-cmd.cpp \
           file://tyan-oem-cmd.hpp \
           file://general-cmd-overlay.cpp \
           file://general-cmd-overlay.hpp \
           file://common-utils.cpp \
           file://common-utils.hpp \
           file://version-utils.cpp \
           file://version-utils.hpp \
           file://tyan-utils.cpp \
           file://tyan-utils.hpp \
           file://bridgingcommands.cpp \
           file://bridgingcommands.hpp \
          "

DEPENDS = "boost \
           phosphor-ipmi-host \
           phosphor-logging \
           systemd \
           libgpiod\
           amd-libapmlpower \
           obmc-libmisc \
           i2c-tools \
           libobmc-i2c \
           "
RDEPENDS:${PN} += "amd-libapmlpower"
RDEPENDS:${PN} += "i2c-tools"
RDEPENDS:${PN} += "obmc-libmisc"
RDEPENDS:${PN} += "libobmc-i2c"

EXTRA_OECMAKE= "-DENABLE_TEST=0 -DYOCTO=1"

LIBRARY_NAMES = "libmctoemcmds.so"

FILES:${PN}:append = " ${libdir}/ipmid-providers/lib*${SOLIBS}"
FILES:${PN}:append = " ${libdir}/host-ipmid/lib*${SOLIBS}"
FILES:${PN}:append = " ${libdir}/net-ipmid/lib*${SOLIBS}"
FILES:${PN}-dev:append = " ${libdir}/ipmid-providers/lib*${SOLIBSDEV}"

do_install() {
    install -d ${D}${libdir}/ipmid-providers/
    install -m 0755 lib*${SOLIBS} ${D}/${libdir}/ipmid-providers/
}