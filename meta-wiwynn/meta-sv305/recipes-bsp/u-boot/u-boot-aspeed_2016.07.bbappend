FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://0001-Set-the-default-UART-route-setting.patch \
            file://0002-Set-default-fan-PWM-value.patch \
            file://0003-Configure-HICR5-register-value-to-default.patch \
            file://0004-Support-Winbond-w25q512jv-flash.patch \
            file://0005-Initialize-SCU-setting-for-setting-multi-function-and-GPIO-pins.patch \
            file://0006-Initialize-the-BMC-status-LED-to-solid-Amber.patch \
            file://0007-Initialize-the-Power-LED-control-pin-to-logic-low.patch \
            file://0008-Enable-WDT2-for-BMC-FW-failover-recovery-feature.patch \
           "
