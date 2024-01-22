FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

PACKAGECONFIG:append = " flash_bmc2"
PACKAGECONFIG:append = " flash_cpld"
PACKAGECONFIG:append = " flash_psu"
PACKAGECONFIG:append = " flash_vr"