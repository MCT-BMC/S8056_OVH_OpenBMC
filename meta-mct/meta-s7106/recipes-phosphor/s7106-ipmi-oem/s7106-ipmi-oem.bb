SUMMARY = "Phosphor IPMI plugin for MingDer OEM Commands"
DESCRIPTION = "Phosphor IPMI plugin for MingDer OEM Commands"
PR = "r1"

inherit autotools pkgconfig
inherit obmc-phosphor-ipmiprovider-symlink
inherit pythonnative

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${MCTBASE}/COPYING.apache-2.0;md5=34400b68072d710fecd0a2940a0d1658"

S = "${WORKDIR}/"

SRC_URI = "file://bootstrap.sh \
           file://LICENSE \
           file://oemhandler.cpp \
           file://README.md \
           file://configure.ac \
           file://Makefile.am \
	   file://oemhandler.hpp "

DEPENDS += "phosphor-ipmi-host"
DEPENDS += "autoconf-archive-native"
DEPENDS += "sdbusplus sdbusplus-native"
DEPENDS += "phosphor-logging"
DEPENDS += "phosphor-dbus-interfaces phosphor-dbus-interfaces-native"
DEPENDS += "sdbus++-native"

RDEPENDS_${PN} += " \
        sdbusplus \
        phosphor-logging \
        phosphor-dbus-interfaces \
        "

TARGET_CFLAGS += "-fpic"

HOSTIPMI_PROVIDER_LIBRARY += "libmingderoemhandler.so"
NETIPMI_PROVIDER_LIBRARY += "libmingderoemhandler.so"

FILES_${PN}_append = " ${libdir}/ipmid-providers/lib*${SOLIBS}"
FILES_${PN}_append = " ${libdir}/host-ipmid/lib*${SOLIBS}"
FILES_${PN}_append = " ${libdir}/net-ipmid/lib*${SOLIBS}"
FILES_${PN}-dev_append = " ${libdir}/ipmid-providers/lib*${SOLIBSDEV} ${libdir}/ipmid-providers/*.la"
