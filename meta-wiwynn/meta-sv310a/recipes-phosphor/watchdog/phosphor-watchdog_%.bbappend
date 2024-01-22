FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SRC_URI += " file://obmc \
             file://phosphor-watchdog@.service \
             file://obmc-enable-host-watchdog@.service \
             file://0001-Add-sel-when-whatchdog-timeout.patch \
             file://0002-Set-system-restart-cause-when-timeout.patch \
             file://0003-Clear-boot-flags-valid-when-timeout.patch \
             file://0004-sv310a-Record-OEM-SEL-F1-for-the-FRB2-postcodes.patch \
           "

SYSTEMD_PACKAGES = "${PN}"
SYSTEMD_SERVICE_${PN} += "obmc-enable-host-watchdog@0.service \
                          "

SYSTEMD_LINK_${PN}_remove += "${@compose_list(d, 'ENABLE_WATCHDOG_FMT', 'OBMC_HOST_INSTANCES')}"
SYSTEMD_OVERRIDE_${PN}_remove += "poweron.conf:phosphor-watchdog@poweron.service.d/poweron.conf"

WATCHDOG_FMT = "../${WATCHDOG_TMPL}:multi-user.target.wants/${WATCHDOG_TGTFMT}"
