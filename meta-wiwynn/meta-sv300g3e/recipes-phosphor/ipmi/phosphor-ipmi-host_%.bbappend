FILESEXTRAPATHS_prepend_sv300g3e := "${THISDIR}/${PN}:"

SRC_URI += "file://0001-Add-to-support-IPMI-Set-Power-Cycle-Interval-command.patch \
            file://0002-Add-to-support-IPMI-mc-warm-reset-command.patch \
            file://0003-Add-IPMI-SDR-Type-03h-Event-Only-sensor-data-record.patch \
            file://0004-Configure-IPMI-unspecified-IP-source-to-DHCP.patch \
            file://0005-Support-to-set-system-boot-option-UEFI-boot-type.patch \
            file://0006-Remove-the-IPMI-OEM-command-handler-used-by-NetFn-0x2E.patch \
            file://0007-Support-IPMI-power-reset-command.patch \
            file://0008-Support-IPMI-power-soft-command.patch \
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
}
