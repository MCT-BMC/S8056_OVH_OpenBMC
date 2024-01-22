FILESEXTRAPATHS_prepend := "${THISDIR}/linux-aspeed:"

SRC_URI += "file://sv305.cfg \
            file://aspeed-bmc-wiwynn-sv305.dts;subdir=git/arch/${ARCH}/boot/dts \
            file://0001-Add-a-flash-layout-dtsi-for-a-64MB-mtd-device.patch \
            file://0002-Read-MAC-address-from-eeprom.patch \
            file://0003-Change-default-fan-PWM-to-80.patch \
            file://0004-Limit-setting-page-in-pmbus.patch \
            file://0005-Supprot-read-the-divisor-of-ADC-clock-from-DTS-file.patch \
            file://0006-Add-AST2500-JTAG-driver-support.patch \
            file://0007-Remove-Unregister-SMC-Partitions.patch \
            file://0008-write-SRAM-panic-words-to-record-kernel-panic.patch \
            file://0009-Enable-WDT2-for-BMC-FW-failover-recovery-feature.patch \
            file://0010-Setting-RJ45-LED-status.patch \
            file://0011-Clear-the-bus-msg-buf-after-every-I2C-transaction.patch \
           "

PACKAGE_ARCH = "${MACHINE_ARCH}"
