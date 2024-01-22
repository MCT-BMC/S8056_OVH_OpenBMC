SUMMARY = "Dbus sensor bridged"
DESCRIPTION = "Bridge daemon for dbus-sensors"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE;md5=838c366f69b72c5df05c96dff79b35f2"

inherit meson pkgconfig
inherit obmc-phosphor-systemd

DEPENDS += "boost"
DEPENDS += "sdbusplus"
DEPENDS += "systemd"

SYSTEMD_SERVICE_${PN} += "sensor-bridged.service"

FILESEXTRAPATHS_append := "${THISDIR}/files:"
S = "${WORKDIR}"

SRC_URI = " \
    file://LICENSE \
    file://meson.build \
    file://main.cpp \
    file://utils.cpp \
    file://utils.hpp \
"
