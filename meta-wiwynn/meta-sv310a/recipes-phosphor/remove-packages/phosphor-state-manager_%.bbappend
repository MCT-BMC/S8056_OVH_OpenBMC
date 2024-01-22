# Remove phosphor-fan-control targets
SYSTEMD_SERVICE_${PN}-obmc-targets_remove += " \
    obmc-fans-ready.target \
    obmc-fan-control.target \
    obmc-fan-control-ready@.target \
    obmc-fan-watchdog-takeover.target \
    "
SYSTEMD_LINK_${PN}-obmc-targets_remove += "${@compose_list(d, 'FAN_LINK_FMT', 'OBMC_CHASSIS_INSTANCES')}"

SYSTEMD_SERVICE_${PN}-host_remove += "phosphor-reset-host-reboot-attempts@.service"

SYSTEMD_SERVICE_${PN}-host-check_remove += " \
    phosphor-reset-host-check@.service \
    "

DBUS_SERVICE_${PN}-host_remove += "phosphor-reboot-host@.service"

pkg_postinst_${PN}-obmc-targets_append() {
    LINK="$D$systemd_system_unitdir/obmc-host-reset@0.target.requires/phosphor-reset-host-check@0.service"
    rm $LINK
    LINK="$D$systemd_system_unitdir/multi-user.target.requires/obmc-host-reset@0.target"
    rm $LINK
    LINK="$D$systemd_system_unitdir/obmc-host-shutdown@0.target.requires/obmc-chassis-poweroff@0.target"
    rm $LINK
    LINK="$D$systemd_system_unitdir/obmc-host-start@0.target.requires/phosphor-reset-host-reboot-attempts@0.service"
    rm $LINK
    LINK="$D$systemd_system_unitdir/obmc-host-reboot@0.target.requires/obmc-host-shutdown@0.target"
    rm $LINK
    LINK="$D$systemd_system_unitdir/obmc-host-reboot@0.target.requires/phosphor-reboot-host@0.service"
    rm $LINK
    LINK="$D$systemd_system_unitdir/obmc-host-force-warm-reboot@0.target.requires/obmc-host-stop@0.target"
    rm $LINK
    LINK="$D$systemd_system_unitdir/obmc-host-force-warm-reboot@0.target.requires/phosphor-reboot-host@0.service"
    rm $LINK
}
FILES_${PN}-host-check_remove = "${bindir}/phosphor-host-check"

FILES_${PN} += "/lib/systemd/system/*"

