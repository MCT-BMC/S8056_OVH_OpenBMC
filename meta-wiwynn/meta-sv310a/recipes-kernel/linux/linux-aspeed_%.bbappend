FILESEXTRAPATHS_prepend := "${THISDIR}/linux-aspeed:"

SRC_URI += "file://sv310a.cfg \
            file://aspeed-bmc-wiwynn-sv310a.dts;subdir=git/arch/${ARCH}/boot/dts \
            file://0001-Add-a-flash-layout-dtsi-for-a-64MB-mtd-device.patch \
            file://0002-Read-MAC-address-from-eeprom.patch \
            file://0003-Change-default-fan-PWM-to-80.patch \
            file://0004-Limit-setting-page-in-pmbus.patch \
            file://0005-Supprot-read-the-divisor-of-ADC-clock-from-DTS-file.patch \
            file://0006-Add-AST2500-JTAG-driver-support.patch \
            file://0007-Remove-Unregister-SMC-Partitions.patch \
            file://0008-write-SRAM-panic-words-to-record-kernel-panic.patch \
            file://0009-Setting-RJ45-LED-status.patch \
            file://0010-Clear-the-bus-msg-buf-after-every-I2C-transaction.patch \
            file://0011-Support-I2C-Mux-PCA9641-driver.patch \
            file://0012-Initialize-fan-configuration-and-dynamics-registers-for-MAX31790-device.patch \
            file://0013-Support-TLC59208-driver.patch \
            file://0014-Setup-bmc-ipmb-slave-address.patch \
            file://0015-Enable-WDT2-for-BMC-FW-failover-recovery-feature.patch \
            file://0016-Enable-TEMP1-configuration-in-ADM1272.patch \
            file://0017-Add-mux-lock-for-48V-PDB.patch \ 
            file://0018-Soft-reset-for-i2c-arbiter-if-reading-timeout.patch \
            file://0019-revise-behavior-for-max-31790.patch \
            file://0020-Revise-i2c-arbiter-reset-time-to-500ms.patch \
	    file://0021-Fix-the-HSC-driver-use-incorrect-I2C-command.patch \
            file://0022-Support-the-interrupt-pin-debounce-time-of-IO-expander.patch \
           "

PACKAGE_ARCH = "${MACHINE_ARCH}"
