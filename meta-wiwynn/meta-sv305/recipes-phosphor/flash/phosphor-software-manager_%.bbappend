FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://0001-sv305-Prevent-OOB-BMC-method-from-checking-firmware-.patch \
            file://0002-Add-CPLD-update-flow.patch \
            file://0003-Add-BIOS-update-flow.patch \
           "
PACKAGECONFIG[cpld_update] = "-Dcpld-update=enabled"

SYSTEMD_SERVICE_${PN}-updater += "${@bb.utils.contains('PACKAGECONFIG', 'flash_bios', 'obmc-flash-cpld@.service', '', d)}"

PACKAGECONFIG_append = " flash_bios cpld_update"
