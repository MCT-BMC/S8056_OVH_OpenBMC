SUMMARY = "Call logrotate per minute to rotate event logs in BMC"
DESCRIPTION = "Call logrotate per minute to rotate event logs in BMC"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE;md5=8cf727c7441179e7d76866522073754f"

inherit meson pkgconfig

DEPENDS += "boost"
DEPENDS += "phosphor-logging"

FILESEXTRAPATHS_append := "${THISDIR}"
S = "${WORKDIR}"

SRC_URI = "file://rotate-event-logs.cpp \
           file://meson.build \
           file://LICENSE \
          "

