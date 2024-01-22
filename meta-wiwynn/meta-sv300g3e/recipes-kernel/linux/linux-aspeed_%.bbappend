FILESEXTRAPATHS_prepend := "${THISDIR}/linux-aspeed:"

SRC_URI += "file://sv300g3e.cfg \
            file://aspeed-bmc-wiwynn-sv300g3e.dts;subdir=git/arch/${ARCH}/boot/dts \
            file://0001-at24.c-Disable-root-only.patch \
            file://0003-ftgmac100.c-Read-MAC-address-from-eeprom-sysfs.patch \
            file://0005-Add-a-flash-layout-dtsi-for-a-64MB-mtd-device.patch \
            file://0006-Change-default-fan-PWM-to-80.patch \
            file://0007-Modify-WDT2-timeout-from-30s-to-65s.patch \
            file://0008-Export-all-DIMMs-during-initialization.patch \
            file://0009-Support-CPU-margin-reading.patch \
            file://0010-Add-AST2500-JTAG-driver-support.patch \
            file://0011-Modify-PTCR-register-for-setting-desired-minimal-RPM.patch \
           "

PACKAGE_ARCH = "${MACHINE_ARCH}"
