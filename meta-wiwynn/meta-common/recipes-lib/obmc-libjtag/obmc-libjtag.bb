SUMMARY = "Phosphor OpenBMC JTAG Library"
DESCRIPTION = "Phosphor OpenBMC JTAG Common Library"
PR = "r1"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://COPYING.MIT;md5=3da9cfbcb788c80a0384361b4de20420"

DEPENDS += "systemd glib-2.0"
RDEPENDS_${PN} += "libsystemd glib-2.0"
TARGET_CC_ARCH += "${LDFLAGS}"

S = "${WORKDIR}"

SRC_URI = "file://libobmcjtag.cpp \
           file://libobmcjtag.hpp \
           file://libobmccpld.cpp \
           file://libobmccpld.hpp \
           file://Makefile \
           file://COPYING.MIT \
          "

do_install() {
        install -d ${D}${libdir}
        install -Dm755 libobmcjtag.so ${D}${libdir}/libobmcjtag.so
        install -d ${D}${includedir}/openbmc
        install -m 0644 ${S}/libobmcjtag.hpp ${D}${includedir}/openbmc/libobmcjtag.hpp
        install -m 0644 ${S}/libobmccpld.hpp ${D}${includedir}/openbmc/libobmccpld.hpp
}

FILES_${PN} = "${libdir}/libobmcjtag.so"
FILES_${PN}-dev = "${includedir}/openbmc/"

