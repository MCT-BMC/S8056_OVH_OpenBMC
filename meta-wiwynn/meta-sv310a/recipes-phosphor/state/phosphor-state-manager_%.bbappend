FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SYSTEMD_ENVIRONMENT_FILE_${PN}-host += "obmc/phosphor-reboot-host/reboot.conf"

SRC_URI += " \
    file://0001-Support-random-power-on-and-cycle-interval.patch \
    file://0002-Add-restart-cause-for-power-policy.patch \
    file://0003-Correct-power-control-targets.patch \
    file://0004-Check-last-host-state-when-power-policy-previous.patch \
    file://0005-Power-on-host-after-fan-control-is-ready.patch \
    "

do_install_append() {
    rm ${D}${systemd_system_unitdir}/phosphor-reset-host-check@.service
    rm ${D}${systemd_system_unitdir}/phosphor-reboot-host@.service
    rm ${D}${systemd_system_unitdir}/phosphor-reset-host-reboot-attempts@.service
}
