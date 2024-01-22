FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://ast2600_openbmc_spl.cfg \
    file://0001-Add-S8056-dtb.patch \
    file://0002-Support-MCT-board-related-configrtion.patch \
    file://0003-Read-mac-from-eeprom-and-set-to-eth.patch \
    file://0004-Read-MAC3-address-from-EEPROM.patch \
    file://0005-U-Boot-Modify-Lan-LED-behavior.patch \
    file://0006-Support-port80h-snoop-to-sgpio.patch \
    file://0007-Uboot-Disable-RTL8211f-EEE-LED-feature.patch \
    file://0008-Modify-Lan-LED-blinking-period.patch \
    file://0009-Support-micron-mt25ql01g.patch \
    file://0011-Add-bmc-firmware-version-string-supported-in-u-boot-.patch \
"
SRC_URI:append = " \
    file://ast2600-s8056.dts;subdir=git/arch/arm/dts \
    file://mct_board_info.h;subdir=git/arch/arm/include/asm/arch-aspeed \
    file://mct_board_info.c;subdir=git/arch/arm/mach-aspeed/ast2600 \
"