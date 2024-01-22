FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://xyz.openbmc_project.led.controller@.service"

DBUS_SERVICE_${PN} += "xyz.openbmc_project.led.controller@.service"
