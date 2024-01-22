SUMMARY = "Register D-bus object for GPIO interrupt"
DESCRIPTION = "Register D-bus object for GPIO interrupt"
PR = "r0"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE;md5=8cf727c7441179e7d76866522073754f"

inherit systemd
inherit pkgconfig autotools
inherit obmc-phosphor-systemd

DEPENDS += "autoconf-archive-native"
DEPENDS += "libevdev"
DEPENDS += "systemd"
DEPENDS += "glib-2.0"
DEPENDS += "sdbusplus"
DEPENDS += "nlohmann-json"
DEPENDS += "boost"
DEPENDS += "libgpiod"

RDEPEND_${PN} += "bash"

SYSTEMD_PACKAGES = "${PN}"

FILESEXTRAPATHS_append := "${THISDIR}/files:"
S = "${WORKDIR}"

SRC_URI = "file://main.cpp \
           file://Makefile.am \
           file://configure.ac \
           file://LICENSE \
          "
