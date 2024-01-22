FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://0001-Add-property-for-power-on-dealy-policy.patch \
            file://0003-Add-interrupt-type-in-watchdog.patch \
            file://0004-Add-interface-for-setting-specified-service-status.patch \
            file://0005-Add-CPLD-property-in-Software-Version.patch \
            file://0006-Add-Hit-proeprty-in-Value-interface.patch \
            file://0008-Add-system-restart-cause-property.patch \
            file://0009-Add-fan-control-debug-mode-and-set-pwm.patch \
            file://0010-Add-host-soft-off-and-unknown-transition.patch \
            file://0011-Add-host-last-state-property.patch \
           "
