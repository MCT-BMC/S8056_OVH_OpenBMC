SUMMARY = "Phosphor OpenBMC Project-specific GPIO Library"
DESCRIPTION = "Phosphor OpenBMC GPIO Library for Project-specific."
PR = "r1"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://COPYING.MIT;md5=3da9cfbcb788c80a0384361b4de20420"

DEPENDS += "systemd glib-2.0"
RDEPENDS_${PN} += "libsystemd glib-2.0"
TARGET_CC_ARCH += "${LDFLAGS}"

S = "${WORKDIR}"

SRC_URI = "file://libobmcgpio.c \
           file://libobmcgpio.h \
           file://Makefile \
           file://COPYING.MIT \
          "

do_install() {
        install -d ${D}${libdir}
        install -Dm755 libobmcgpio.so ${D}${libdir}/libobmcgpio.so
        install -d ${D}${includedir}/openbmc
        install -m 0644 ${S}/libobmcgpio.h ${D}${includedir}/openbmc/libobmcgpio.h
}

FILES_${PN} = "${libdir}/libobmcgpio.so"
FILES_${PN}-dev = "${includedir}/openbmc/"

