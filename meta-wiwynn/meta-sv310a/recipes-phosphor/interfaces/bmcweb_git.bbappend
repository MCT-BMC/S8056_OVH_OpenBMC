FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

DEPENDS += "phosphor-ipmi-host"

SRC_URI += "file://0001-Add-http-end-point-for-SOL-buffer.patch \
            file://0002-Disable-version-compare-judgement-in-image-upload-AP.patch \
            file://0003-Add-SEL-for-BMC-reset.patch \
            file://0004-Support-chassis-action.patch \
            file://0005-Chassis-sensors-collect-all-sensors.patch \
            file://0006-Support-PATCH-command-to-set-sensor-thresholds.patch \
            file://0007-Support-AMD-power-capping-in-chassis-power.patch \
            file://0008-Support-SEL-APIs.patch \
            file://0009-Integrate-redfish-nic-sensor-into-general-sensor.patch \
            file://0010-Support-Update-Interface.patch \
           "

# Enable Redfish BMC Journal support
EXTRA_OEMESON += "-Dredfish-bmc-journal=enabled"
EXTRA_OEMESON += "-Dhttp-body-limit=512"
EXTRA_OEMESON += "-Dinsecure-tftp-update=disabled"
