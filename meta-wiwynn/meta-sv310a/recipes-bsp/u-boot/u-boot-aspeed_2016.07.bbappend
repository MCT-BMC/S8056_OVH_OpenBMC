FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://0001-Set-the-default-UART-route-setting.patch \
            file://0003-Configure-HICR5-register-value-to-default.patch \
            file://0004-Support-Winbond-w25q512jv-flash.patch \
            file://0005-Initialize-SCU-setting-for-setting-multi-function-and-GPIO-pins.patch \
            file://0006-Initialize-default-LED-status.patch \
            file://0007-Enable-WDT2-for-BMC-FW-failover-recovery-feature.patch \
            file://0008-Support-bmc-boot-from-command.patch \
            file://0009-Enable-GPIOE0-and-GPIOE2-pass-through-mode.patch \
            file://0010-Aspeed-i2c-driver.patch \
            file://0011-Set-fan-controller-I2C-watchdog-and-power-enable.patch \
            file://0012-Release-the-lock-of-CPLD-Mux-on-48V-PDB.patch \
           "
