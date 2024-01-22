FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRCREV = "897ae72f1790817c67aa2e3a87cc0992eb997959"

SRC_URI += "file://0001-Support-IPMI-power-commands-and-SEL.patch \
            file://0002-Modified-IP-related-setting-and-fix-some-issue.patch \
            file://0003-Add-BMC-warm-reset-command-supported.patch \
            file://0004-Fix-expiration-flags-unexpected-status-in-get-watchd.patch \
            file://0005-Implement-the-DCMI-power-reading-and-power-limit-com.patch \
            file://0006-Fix-the-wrong-field-value-for-session-info-commnad.patch \
            file://0007-Update-firmware-revision-format-and-error-handling.patch \
            file://0008-Change-ipmi_sel-location-to-persistent-folder.patch \
            file://0009-Add-SDR-related-changing.patch \
            file://0010-Add-SEL-Entry-command-supported.patch \
            file://0011-Workaroud-for-ipmitool-fru-edit-issue.patch \
            file://0012-Implement-the-event-log-for-clear-SEL-command.patch \
            file://0013-Support-FRU-configuration-in-JSON-format.patch \
            file://0014-Add-leaky-bucket-supported.patch \
            file://0015-Customized-sensor-number-map-support.patch \
            file://0016-Using-dbus-sdr-function-to-replace-original-IPMI.patch \
            file://0017-Change-identify-command-controlling-LED-to-blink.patch \
            file://0018-Add-bmc-self-testing-command-supported.patch \
            file://0019-PATCH-Move-Set-SOL-config-parameter-to-host-ipmid.patch \
            file://0020-PATCH-Move-Get-SOL-config-parameter-to-host-ipmid.patch \
            file://0021-Clear-store-sensor-data-when-SDR-command-failed.patch \
            file://0022-fix-incorrect-null-value-check-issue.patch \
            file://0023-Support-setting-getting-system-information-via-dbus-.patch \
            file://0024-Update-ipmi-restart-cause.patch \
            file://0025-Fix-SDR-list-abnormal-issue.patch \
            file://0026-Move-hybrid-sensor-setting-function-before-event-onl.patch \
            file://0027-Using-dbus-sdr-based-set-sel-time-handler.patch \
            file://0028-Support-using-specified-sdr-type-for-sensors.patch \
            file://0029-Revert-openBMC-upstream-lastID-modification.patch \
            file://0030-Fix-get-LAN-configuration-parameters-command-respons.patch \
            file://master_write_read_white_list.json \
           "

FILES:${PN} += " ${datadir}/ipmi-providers/master_write_read_white_list.json \
               "
#EXTRA_OECONF += "--enable-dynamic_sensors=yes --disable-i2c-whitelist-check"
EXTRA_OECONF:append = " --enable-dynamic_sensors=yes" 
EXTRA_OECONF:append = " --disable-i2c-whitelist-check"

SYSTEMD_SERVICE:${PN} = "phosphor-ipmi-host.service"

EXTRA_OECONF:append = " --enable-hybrid-sensors=yes"
DEPENDS:append = " mct-yaml-config"

EXTRA_OECONF:append = " \
    SENSOR_YAML_GEN=${STAGING_DIR_HOST}${datadir}/mct-yaml-config/ipmi-sensors.yaml \
    "

do_install:append(){
  install -d ${D}${datadir}/ipmi-providers
  install -m 0644 -D ${WORKDIR}/master_write_read_white_list.json ${D}${datadir}/ipmi-providers/master_write_read_white_list.json
}
