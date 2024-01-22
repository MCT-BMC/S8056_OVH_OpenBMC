FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://0001-Light-BMC-Heartbeat-LED.patch \
            file://0002-Set-the-default-UART-route-setting.patch \
            file://0003-Get-mac-from-eeprom.patch \
            file://0004-Set-BMC-RTL8211E-PHY-LED.patch \
            file://0005-Enable-WDT-in-uboot.patch \
            file://0006-Set-the-default-GPIO-pass-through.patch \
            file://0007-Add-default-aspeed-pwm-setting-supported.patch \
            file://0008-Chanage-kernl-offset-to-0x20100000-for-fixing-boot-f.patch \
            file://0009-Add-default-WDT-mask-setting-supported.patch \
            file://0010-Add-default-bmc-firmware-version-setting-supported.patch \
            "

