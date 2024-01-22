FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://0001-Set-the-default-UART-route-setting.patch \
            file://0002-Set-default-fan-PWM-value.patch \
            file://0003-Configure-HICR5-register-value-to-default.patch \
            file://0004-Initialize-the-BMC-status-LED-to-solid-Amber.patch \
            file://0005-Initialize-the-CATERR-LED-to-Off.patch \
            file://0006-Set-the-power-button-GPIOs-to-pass-through-mode.patch \
           "
