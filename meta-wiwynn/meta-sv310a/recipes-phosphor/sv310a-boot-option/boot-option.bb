SUMMARY = "Boot option"
DESCRIPTION = "Boot option service for valid bit"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE;md5=838c366f69b72c5df05c96dff79b35f2"

inherit meson pkgconfig
inherit obmc-phosphor-systemd

DEPENDS += "boost"
DEPENDS += "phosphor-logging"
DEPENDS += "sdbusplus"
DEPENDS += "systemd"

SYSTEMD_SERVICE_${PN} += "boot-option.service"

S = "${WORKDIR}"

SRC_URI = " \
    file://LICENSE \
    file://meson.build \
    file://boot-option.cpp \
"
