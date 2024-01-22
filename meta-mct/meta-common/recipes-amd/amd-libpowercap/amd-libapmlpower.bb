SUMMARY = "AMD APML Power Library"
DESCRIPTION = "AMD APML Power Library for Project-specific."
PR = "r1"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://COPYING.MIT;md5=3da9cfbcb788c80a0384361b4de20420"

inherit pkgconfig

DEPENDS += "systemd glib-2.0 amd-apml boost"
RDEPENDS:${PN} += "libsystemd glib-2.0 amd-apml"
TARGET_CC_ARCH += "${LDFLAGS}"

S = "${WORKDIR}"

SRC_URI = "file://libapmlpower.cpp \
           file://libapmlpower.hpp \
           file://Makefile \
           file://COPYING.MIT \
          "

do_install() {
        install -d ${D}${libdir}
        install -Dm755 libapmlpower.so ${D}${libdir}/libapmlpower.so
        install -d ${D}${includedir}/amd
        install -m 0644 ${S}/libapmlpower.hpp ${D}${includedir}/amd/libapmlpower.hpp
}

FILES:${PN} = "${libdir}/libapmlpower.so"
FILES:${PN}-dev = "${includedir}/amd/"


