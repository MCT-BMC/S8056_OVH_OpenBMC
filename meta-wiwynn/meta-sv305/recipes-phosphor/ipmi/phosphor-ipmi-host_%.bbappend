FILESEXTRAPATHS_prepend_sv305 := "${THISDIR}/${PN}:"

SRC_URI += "file://0001-Add-to-support-IPMI-mc-warm-reset-command.patch \
            file://0002-Create-sensor-gen-extra-from-sensor-yaml.patch \
            file://0003-Support-sdr-related-command-handlers.patch \
            file://0004-Get-fru-sdr-from-fru-dbus.patch \
            file://0005-Support-IPMI-power-reset-command.patch \
            file://0006-Support-host-power-soft-off.patch \
            file://0007-Add-watchdog-timeout-interrupt.patch \
            file://0008-Support-owner-id-for-sdr-sensors.patch \
            file://0009-Disable-OEM-command-special-handling.patch \
            file://0010-Add-sensor-MRB-value-in-sensor-gen-extra.patch \
            file://0011-Limit-out-of-range-sensor-value-to-max-or-min-value.patch \
            file://0012-Add-hit-Sensor-Unit.patch \
            file://0013-Get-default-gateway-mac-directly-from-kernel-route.patch \
            file://0014-Get-fru-name-from-json-setting.patch \
            file://0015-Add-system-restart-cause-for-chassis-power-control.patch \
            file://master_write_read_white_list.json \
            "

FILES_${PN} += " ${datadir}/ipmi-providers/master_write_read_white_list.json \
               "

do_install_append(){
  install -d ${D}${includedir}/phosphor-ipmi-host
  install -m 0644 -D ${S}/include/ipmid/utils.hpp ${D}${includedir}/phosphor-ipmi-host
  install -m 0644 -D ${S}/include/ipmid/types.hpp ${D}${includedir}/phosphor-ipmi-host
  install -m 0644 -D ${S}/sensorhandler.hpp ${D}${includedir}/phosphor-ipmi-host
  install -m 0644 -D ${S}/selutility.hpp ${D}${includedir}/phosphor-ipmi-host
  install -m 0644 -D ${S}/storageaddsel.hpp ${D}${includedir}/phosphor-ipmi-host
  install -d ${D}${datadir}/ipmi-providers
  install -m 0644 -D ${WORKDIR}/master_write_read_white_list.json ${D}${datadir}/ipmi-providers/master_write_read_white_list.json
  install -d ${D}${includedir}/openbmc
  install -m 0644 -D ${B}/sensor-gen-extra.cpp ${D}${includedir}/openbmc/sensor-gen-extra.cpp
}
FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SYSTEMD_LINK_${PN}_remove += "${@compose_list_zip(d, 'SOFT_FMT', 'OBMC_HOST_INSTANCES')}"
