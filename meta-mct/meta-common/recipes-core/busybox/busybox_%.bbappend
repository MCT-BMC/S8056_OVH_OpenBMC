FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"
SRC_URI += "${@bb.utils.contains('DISTRO_FEATURES', 'obmc-ubi-fs', '', 'file://reboot.cfg', d)}"
