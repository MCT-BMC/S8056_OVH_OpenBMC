FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRCREV = "124e63beaf84a848d51daaf73065860ec6a0482d"

RDEPENDS:${PN} += " bash"

PACKAGECONFIG[static-dual-image] = "-Dbmc-static-dual-image=enabled, -Dbmc-static-dual-image=disabled"
SYSTEMD_SERVICE:${PN}-updater += "${@bb.utils.contains('PACKAGECONFIG', 'static-dual-image', 'obmc-flash-bmc-alt@.service', '', d)}"
SYSTEMD_SERVICE:${PN}-updater += "${@bb.utils.contains('PACKAGECONFIG', 'static-dual-image', 'obmc-flash-bmc-static-mount-alt.service', '', d)}"
SYSTEMD_SERVICE:${PN}-updater += "${@bb.utils.contains('PACKAGECONFIG', 'static-dual-image', 'obmc-flash-bmc-prepare-for-sync.service', '', d)}"

PACKAGECONFIG[side_switch_on_boot] = "-Dside-switch-on-boot=enabled, -Dside-switch-on-boot=disabled, cli11"
FILES:${PN}-side-switch += "\
    ${bindir}/phosphor-bmc-side-switch \
    "
SYSTEMD_SERVICE:${PN}-side-switch += "${@bb.utils.contains('PACKAGECONFIG', 'side_switch_on_boot', 'phosphor-bmc-side-switch.service', '', d)}"

SRC_URI += "file://0001-Add-additional-information-for-updating-progress.patch \
            file://0002-Add-bios-update-feature-supported.patch \
            file://0003-Add-cpld-update-supported.patch \
            file://0004-Add-VR-update-supported.patch \
            file://0005-Add-alternate-BMC-update-supported.patch \
            file://0005-Add-bios2-update-supported.patch \
            "

PACKAGECONFIG[flash_cpld] = "-Dcpld-upgrade=enabled, -Dcpld-upgrade=disabled"
SYSTEMD_SERVICE:${PN}-updater += "${@bb.utils.contains('PACKAGECONFIG', 'flash_cpld', 'obmc-flash-cpld@.service', '', d)}"

PACKAGECONFIG[flash_vr] = "-Dvr-upgrade=enabled, -Dvr-upgrade=disabled"
SYSTEMD_SERVICE:${PN}-updater += "${@bb.utils.contains('PACKAGECONFIG', 'flash_vr', 'obmc-flash-vr-p0-vdd11@.service', '', d)}"
SYSTEMD_SERVICE:${PN}-updater += "${@bb.utils.contains('PACKAGECONFIG', 'flash_vr', 'obmc-flash-vr-p0-vddcr-soc@.service', '', d)}"
SYSTEMD_SERVICE:${PN}-updater += "${@bb.utils.contains('PACKAGECONFIG', 'flash_vr', 'obmc-flash-vr-p0-vddio@.service', '', d)}"

PACKAGECONFIG[flash_alt_bmc] = "-Dalt-bmc-upgrade=enabled, -Dalt-bmc-upgrade=disabled"
SYSTEMD_SERVICE:${PN}-updater += "${@bb.utils.contains('PACKAGECONFIG', 'flash_cpld', 'obmc-flash-alt-bmc@.service', '', d)}"

SYSTEMD_SERVICE:${PN}-updater += "${@bb.utils.contains('PACKAGECONFIG', 'flash_bios', 'obmc-flash-host-bios2@.service', '', d)}"

PACKAGECONFIG:append = " flash_bios"
PACKAGECONFIG:append = " flash_cpld"
PACKAGECONFIG:append = " flash_vr"
PACKAGECONFIG:append = " flash_alt_bmc"
