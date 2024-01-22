FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SRC_URI += " \
    file://op-power-start@.service \
    file://op-power-stop@.service \
    "

DBUS_SERVICE_${PN}_remove += "${@compose_list(d, 'FMT', 'OBMC_POWER_INSTANCES')}"

