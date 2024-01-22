FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SYSTEMD_ENVIRONMENT_FILE_${PN}-host += "obmc/phosphor-reboot-host/reboot.conf"

SRC_URI += " \
    file://0001-Support-random-power-on-and-cycle-interval.patch \
    file://0002-Add-restart-cause-for-power-policy.patch \
    "

