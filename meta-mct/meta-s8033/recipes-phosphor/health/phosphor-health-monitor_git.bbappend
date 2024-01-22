FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "    \
            file://0001-Moidfy-to-support-IPMI-and-SEL.patch \
            file://0002-take-zero-to-replace-first-reading-to-fill-buffer.patch \
            file://0003-Add-restart-property-for-phosphor-health-monitor-ser.patch \
            file://0004-Add-chassis-association-for-health-monitoring-sensor.patch \
            file://0005-logging-switch-to-lg2.hpp.patch \
            file://bmc_health_config.json;subdir=git \
            "

