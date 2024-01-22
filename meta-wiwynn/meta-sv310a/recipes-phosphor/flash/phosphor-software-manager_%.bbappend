FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

DEPENDS += "obmc-libi2c obmc-libjtag libgpiod"
RDEPENDS_${PN}-updater += "obmc-libi2c obmc-libjtag"

SRC_URI += "file://0001-sv310a-Prevent-OOB-BMC-method-from-checking-firmware-.patch \
            file://0002-Add-CPLD-update-flow.patch \
            file://0003-Add-BIOS-update-flow.patch \
            file://0004-Add-flow-for-CPLD-update-through-i2c.patch \
            file://0005-Add-Version-Change-SEL-before-rebooting-BMC.patch \
            file://0006-Correct-the-BMC-firmware-update-behavior.patch \
            file://0007-Support-different-types-of-CPLD.patch \
            file://0008-Revise-BIOS-version-behavior.patch \
           "

PACKAGECONFIG[cpld_update] = "-Dcpld-update=enabled"

SYSTEMD_SERVICE_${PN}-updater += "${@bb.utils.contains('PACKAGECONFIG', 'flash_bios', 'obmc-flash-cpld@.service', '', d)}"

PACKAGECONFIG_append = " flash_bios cpld_update"
