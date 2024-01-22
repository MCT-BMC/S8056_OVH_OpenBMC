SUMMARY = "CPU Prochot Event Handler"
DESCRIPTION = "CPU Prochot Event Service for Throttle SEL Record"

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=86d3f3a95c324c9479bd8986968f4327"

SRC_URI = "file://cmake \
           file://include \
           file://service \
           file://src \
           file://cpu-prochot-gpio.json \
           file://cmake-format.json \
           file://CMakeLists.txt \
           file://LICENSE \
          "

SYSTEMD_SERVICE_${PN} = "xyz.openbmc_project.cpu-prochot-event.service"

inherit cmake systemd
inherit obmc-phosphor-systemd

DEPENDS += "boost nlohmann-json sdbusplus libgpiod"
RDEPENDS_${PN} += "libsystemd sdbusplus libgpiod"


S = "${WORKDIR}"

EXTRA_OECMAKE = "-DYOCTO=1"

do_install_append() {
    install -d ${D}/etc
    install -m 0644 ${S}/cpu-prochot-gpio.json ${D}/etc/cpu-prochot-gpio.json
}
