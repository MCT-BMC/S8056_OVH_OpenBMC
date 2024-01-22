FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://0001-Set-watchdog-default-action.patch \
            file://0002-Add-interface-for-setting-specified-service-status.patch \
            file://0003-Add-interface-for-DCMI-power.patch \
            file://0004-Add-spcified-firmware-property-for-version-purpose.patch \
            file://0005-Add-x86-power-control-relatived-dbus-interface.patch \
            file://0006-Add-sensor-status-interface-supported.patch \
            file://0008-Add-nmi-property-supported-for-x86-power-control.patch \
            file://0011-Add-system-information-interface-supported.patch \
	        file://0013-Update-restart-cause-interface-and-its-properties.patch \
            file://0014-Add-host-status-interface-supported.patch \
            file://0016-Add-additional-information-for-updating-progress.patch \
            file://0017-PATCH-Add-Error-to-Software-Version-Interface.patch \
            file://0018-Add-bmc-status-interface-supported.patch \
            file://0019-Add-BMC-P2A-property-for-setting-specified-service-s.patch \
            "

