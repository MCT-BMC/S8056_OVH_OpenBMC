PACKAGECONFIG_append = "use-json"

FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SRC_URI += " \
    file://led-group-config.json \
    file://0001-Support-persist-option-for-group-in-json.patch \
"

do_install_append() {
    install -m 0644 ${WORKDIR}/led-group-config.json ${D}${datadir}/phosphor-led-manager/
}

SYSTEMD_LINK_${PN}_remove += "${@compose_list(d, 'FMT', 'STATES')}"
SYSTEMD_LINK_${PN}_remove += "../obmc-led-group-start@.service:multi-user.target.wants/obmc-led-group-start@bmc_booted.service"
SYSTEMD_OVERRIDE_${PN}_remove += "bmc_booted.conf:obmc-led-group-start@bmc_booted.service.d/bmc_booted.conf"
