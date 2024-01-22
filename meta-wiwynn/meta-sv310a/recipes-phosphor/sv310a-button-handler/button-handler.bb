SUMMARY = "Button Event Handler"
DESCRIPTION = "Button Event Handler"

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=86d3f3a95c324c9479bd8986968f4327"

SRC_URI = "file://cmake \
           file://include \
           file://service \
           file://src \
           file://button-gpio.json \
           file://cmake-format.json \
           file://CMakeLists.txt \
           file://LICENSE \
          "

SYSTEMD_SERVICE_${PN} = "xyz.openbmc_project.button-handler.service"

inherit cmake systemd
inherit obmc-phosphor-systemd

DEPENDS += "boost nlohmann-json sdbusplus libgpiod phosphor-dbus-interfaces obmc-libi2c phosphor-logging"
RDEPENDS_${PN} += "libsystemd sdbusplus libgpiod phosphor-dbus-interfaces obmc-libi2c"

S = "${WORKDIR}"

EXTRA_OECMAKE = "-DYOCTO=1"
EXTRA_OECMAKE += "-DPWR_SOFTOFF_TIME_MS=2000"
EXTRA_OECMAKE += "-DPWR_HARDOFF_TIME_MS=6000"
EXTRA_OECMAKE += "-DPWR_ACOFF_TIME_MS=10000"

do_install_append() {
    install -d ${D}/etc
    install -m 0644 ${S}/button-gpio.json ${D}/etc/button-gpio.json
}
