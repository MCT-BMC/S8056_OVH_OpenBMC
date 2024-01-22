FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://0001-Set-watchdog-default-action.patch \
            file://0002-Add-interface-for-setting-specified-service-status.patch \
            file://0003-Add-interface-for-DCMI-power.patch \
            file://0004-Add-the-version-ID-parameter-to-software-version-int.patch \
            file://0005-Add-x86-power-control-relatived-dbus-interface.patch \
            file://0006-Add-sensor-status-interface-supported.patch \
            file://0007-Add-bmc2-property-for-version-purpose.patch \
            file://0008-Add-nmi-property-supported-for-x86-power-control.patch \
            file://0009-Add-cpld-property-for-version-purpose.patch \
            file://0010-Add-bios2-property-for-version-purpose.patch \
            file://0011-Add-system-information-interface-supported.patch \
            file://0012-Add-psu2-property-for-version-purpose.patch \
	        file://0013-Update-restart-cause-interface-and-its-properties.patch \
            file://0014-Add-host-status-interface-supported.patch \
            file://0015-Add-vr-property-for-version-purpose.patch \
            file://0016-Add-additional-information-for-updating-progress.patch \
            "

