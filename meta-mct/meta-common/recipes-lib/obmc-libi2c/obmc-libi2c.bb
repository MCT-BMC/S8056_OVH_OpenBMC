SUMMARY = "Phosphor OpenBMC Project-specific I2C Library"
DESCRIPTION = "Phosphor OpenBMC I2C Library for Project-specific."
PR = "r1"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://COPYING.MIT;md5=3da9cfbcb788c80a0384361b4de20420"

DEPENDS += "systemd glib-2.0"
RDEPENDS:${PN} += "libsystemd glib-2.0"
TARGET_CC_ARCH += "${LDFLAGS}"

S = "${WORKDIR}"

SRC_URI = "file://libobmci2c.c \
           file://libobmci2c.h \
           file://Makefile \
           file://COPYING.MIT \
          "

do_install() {
        install -d ${D}${libdir}
        install -Dm755 libobmci2c.so ${D}${libdir}/libobmci2c.so
        install -d ${D}${includedir}/openbmc
        install -m 0644 ${S}/libobmci2c.h ${D}${includedir}/openbmc/libobmci2c.h
}

FILES:${PN} = "${libdir}/libobmci2c.so"
FILES:${PN}-dev = "${includedir}/openbmc/"

