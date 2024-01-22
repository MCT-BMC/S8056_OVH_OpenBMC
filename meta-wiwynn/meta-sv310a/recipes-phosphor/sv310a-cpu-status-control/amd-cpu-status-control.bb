SUMMARY = "Register D-bus object for AMD CPU status control"
DESCRIPTION = "Register D-bus object for AMD CPU status control"
PR = "r0"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE;md5=8cf727c7441179e7d76866522073754f"

inherit cmake systemd
inherit obmc-phosphor-systemd

DEPENDS += "autoconf-archive-native"
DEPENDS += "libevdev"
DEPENDS += "systemd"
DEPENDS += "glib-2.0"
DEPENDS += "sdbusplus"
DEPENDS += "boost"
DEPENDS += "obmc-libdbus"
DEPENDS += "amd-apml"
DEPENDS += "nlohmann-json"

RDEPEND_${PN} += "bash"
RDEPENDS_${PN} += "obmc-libdbus"
RDEPENDS_${PN} += "amd-apml"

SYSTEMD_PACKAGES = "${PN}"

FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"
SYSTEMD_SERVICE_${PN} = "amd-cpu-status-control.service"

S = "${WORKDIR}"

SRC_URI = "file://main.cpp \
           file://PowerCapping.cpp \
           file://PowerCapping.hpp \
           file://CMakeLists.txt \
           file://LICENSE \
          "

FILES_${PN} += "${bindir}/amd-cpu-status-control"

