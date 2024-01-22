SUMMARY = "MiTAC OEM IPMI commands"
DESCRIPTION = "MiTAC OEM Commands"
PR = "r1"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${MCTBASE}/COPYING.apache-2.0;md5=34400b68072d710fecd0a2940a0d1658"

inherit cmake obmc-phosphor-ipmiprovider-symlink pkgconfig

S = "${WORKDIR}"

SRC_URI = "file://CMakeLists.txt\
           file://LICENSE \
           file://oemcmd.cpp \
	       file://oemcmd.hpp \
           file://version-utils.hpp \
          "

DEPENDS = "boost \
           phosphor-ipmi-host \
           phosphor-logging \
           systemd \
           libgpiod\
           amd-libapmlpower \
           obmc-libmisc \
           "

RDEPENDS:${PN} += "amd-libapmlpower"
RDEPENDS:${PN} += "obmc-libmisc"

EXTRA_OECMAKE= "-DENABLE_TEST=0 -DYOCTO=1"

LIBRARY_NAMES = "libmctoemcmds.so"

FILES:${PN}:append = " ${libdir}/ipmid-providers/lib*${SOLIBS}"
FILES:${PN}:append = " ${libdir}/host-ipmid/lib*${SOLIBS}"
FILES:${PN}:append = " ${libdir}/net-ipmid/lib*${SOLIBS}"
FILES:${PN}-dev:append = " ${libdir}/ipmid-providers/lib*${SOLIBSDEV}"
