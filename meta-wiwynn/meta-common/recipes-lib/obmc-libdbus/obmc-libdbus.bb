SUMMARY = "Phosphor OpenBMC Project-specific D-Bus Library"
DESCRIPTION = "Phosphor OpenBMC D-Bus Library for Project-specific."
PR = "r1"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://COPYING.MIT;md5=3da9cfbcb788c80a0384361b4de20420"

inherit pkgconfig

DEPENDS += "systemd glib-2.0 sdbusplus phosphor-dbus-interfaces"
RDEPENDS_${PN} += "libsystemd glib-2.0 sdbusplus"
TARGET_CC_ARCH += "${LDFLAGS}"

S = "${WORKDIR}"

SRC_URI = "file://libobmcdbus.cpp \
           file://libobmcdbus.hpp \
           file://Makefile \
           file://COPYING.MIT \
          "

do_install() {
        install -d ${D}${libdir}
        install -Dm755 libobmcdbus.so ${D}${libdir}/libobmcdbus.so
        install -d ${D}${includedir}/openbmc
        install -m 0644 ${S}/libobmcdbus.hpp ${D}${includedir}/openbmc/libobmcdbus.hpp
}

FILES_${PN} = "${libdir}/libobmcdbus.so"
FILES_${PN}-dev = "${includedir}/openbmc/"


