SUMMARY = "Register D-bus object for discrete sensors"
DESCRIPTION = "Register D-bus object for discrete sensors"
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
DEPENDS += "nlohmann-json"
DEPENDS += "boost"
DEPENDS += "libgpiod"
DEPENDS += "obmc-libmisc"
DEPENDS += "obmc-libi2c"
DEPENDS += "obmc-libdbus"

RDEPEND_${PN} += "bash"
RDEPENDS_${PN} += "obmc-libmisc"
RDEPENDS_${PN} += "obmc-libi2c"
RDEPENDS_${PN} += "obmc-libdbus"

SYSTEMD_PACKAGES = "${PN}"

FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"
SYSTEMD_SERVICE_${PN} = "discretesensor.service"

S = "${WORKDIR}"

SRC_URI = "file://main.cpp \
           file://Sensor.cpp \
           file://Sensor.hpp \
           file://sdbus_asio.hpp \
           file://PsuStatusMonitor.cpp \
           file://PsuStatusMonitor.hpp \
           file://DiscreteSensorInterrupt.hpp \
           file://MatchMonitor.hpp \
           file://CMakeLists.txt \
           file://LICENSE \
          "

FILES_${PN} += "${bindir}/discretesensor"

EXTRA_OECMAKE += "-DDISCRETE_SENSOR_DBUS_PATH=/usr/share/entity-manager/configurations/sv310a-DiscreteSensor.json"
