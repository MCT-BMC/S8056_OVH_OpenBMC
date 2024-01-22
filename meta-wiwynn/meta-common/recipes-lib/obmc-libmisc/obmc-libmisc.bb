SUMMARY = "Phosphor OpenBMC Project-specific Misc. Library"
DESCRIPTION = "Phosphor OpenBMC Misc. Library for Project-specific."
PR = "r1"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://COPYING.MIT;md5=3da9cfbcb788c80a0384361b4de20420"

DEPENDS += "systemd glib-2.0 jansson"
RDEPENDS_${PN} += "libsystemd glib-2.0 jansson"
TARGET_CC_ARCH += "${LDFLAGS}"

inherit cmake

S = "${WORKDIR}"

SRC_URI = "file://libmath.c \
           file://libdbus.c \
           file://libregister.c \
           file://libsemaphore.c \
           file://libmisc.h \
           file://CMakeLists.txt \
           file://COPYING.MIT \
          "
