FILESEXTRAPATHS_append := "${THISDIR}/files:"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${WIWYNNBASE}/COPYING.apache-2.0;md5=34400b68072d710fecd0a2940a0d1658"

SUMMARY = "SV300G3-E IPMI OEM Command Set"
DESCRIPTION = "SV300G3-E ipmi oem command implementation"

DEPENDS += "boost"
DEPENDS += "phosphor-ipmi-host"
DEPENDS += "phosphor-logging"
DEPENDS += "systemd"
DEPENDS += "libgpiod"
DEPENDS += "obmc-libi2c"
DEPENDS += "obmc-libjtag"

RDEPENDS_${PN} += "obmc-libi2c"
RDEPENDS_${PN} += "obmc-libjtag"

inherit cmake obmc-phosphor-ipmiprovider-symlink
EXTRA_OECMAKE= "-DENABLE_TEST=0 -DYOCTO=1"

S = "${WORKDIR}"
SRC_URI = "file://CMakeLists.txt \
           file://LICENSE \
           file://oemcommands.cpp \
           file://oemcommands.hpp \
           file://appcommands.cpp \
           file://appcommands.hpp \
           file://Utils.cpp \
           file://Utils.hpp \
          "

LIBRARY_NAMES = "libzsv300g3eoemcmds.so"

HOSTIPMI_PROVIDER_LIBRARY += "${LIBRARY_NAMES}"
NETIPMI_PROVIDER_LIBRARY += "${LIBRARY_NAMES}"

FILES_${PN}_append = " ${libdir}/ipmid-providers/lib*${SOLIBS}"
FILES_${PN}_append = " ${libdir}/host-ipmid/lib*${SOLIBS}"
FILES_${PN}_append = " ${libdir}/net-ipmid/lib*${SOLIBS}"
FILES_${PN}-dev_append = " ${libdir}/ipmid-providers/lib*${SOLIBSDEV} ${libdir}/ipmid-providers/*.la"
