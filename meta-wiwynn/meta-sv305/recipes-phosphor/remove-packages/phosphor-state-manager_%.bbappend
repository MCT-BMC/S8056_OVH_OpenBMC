# Remove phosphor-fan-control targets
SYSTEMD_SERVICE_${PN}-obmc-targets_remove += " \
    obmc-fans-ready.target \
    obmc-fan-control.target \
    obmc-fan-control-ready@.target \
    obmc-fan-watchdog-takeover.target \
    "
SYSTEMD_LINK_${PN}-obmc-targets_remove += "${@compose_list(d, 'FAN_LINK_FMT', 'OBMC_CHASSIS_INSTANCES')}"

SYSTEMD_SERVICE_${PN}-host-check_remove += " \
    phosphor-reset-host-check@.service \
    "

pkg_postinst_${PN}-obmc-targets_append() {
    LINK="$D$systemd_system_unitdir/obmc-host-reset@0.target.requires/phosphor-reset-host-check@0.service"
    rm $LINK
}
FILES_${PN}-host-check_remove = "${bindir}/phosphor-host-check"

FILES_${PN} += "/lib/systemd/system/*"

