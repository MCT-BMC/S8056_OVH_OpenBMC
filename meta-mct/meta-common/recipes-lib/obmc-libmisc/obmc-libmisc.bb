SUMMARY = "Phosphor OpenBMC Project-specific Misc. Library"
DESCRIPTION = "Phosphor OpenBMC Misc. Library for Project-specific."
PR = "r1"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://COPYING.MIT;md5=3da9cfbcb788c80a0384361b4de20420"

DEPENDS += "systemd glib-2.0 jansson"
RDEPENDS:${PN} += "libsystemd glib-2.0 jansson"
TARGET_CC_ARCH += "${LDFLAGS}"

S = "${WORKDIR}"

SRC_URI = "file://libmath.c \
           file://libdbus.c \
           file://libregister.c \
           file://libmisc.h \
           file://Makefile \
           file://COPYING.MIT \
          "

do_install() {
        install -d ${D}${libdir}
        install -Dm755 libobmcmisc.so ${D}${libdir}/libobmcmisc.so
        install -d ${D}${includedir}/openbmc
        install -m 0644 ${S}/libmisc.h ${D}${includedir}/openbmc/libmisc.h
}

FILES:${PN} = "${libdir}/libobmcmisc.so"
FILES:${PN}-dev = "${includedir}/openbmc/"

