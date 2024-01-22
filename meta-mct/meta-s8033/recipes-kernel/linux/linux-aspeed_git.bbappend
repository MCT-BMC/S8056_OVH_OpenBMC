FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += " \
    file://s8033.cfg \
    file://aspeed-bmc-mct-s8033.dts;subdir=git/arch/${ARCH}/boot/dts \
    file://0001-Implement-Heart-Beat-LED.patch \
    file://0002-Set-RTD_1_2_3-to-thermistor-mode.patch \
    file://0003-Change-aspeed-rpm-driver-to-falling-edge.patch \
    file://0004-soc-aspeed-Miscellaneous-control-interfaces.patch \
    file://0005-drivers-jtag-Add-Aspeed-SoC-24xx-25xx-26xx-families.patch \
    file://0006-Enable-pass-through-on-GPIOE1-and-GPIOE3-free.patch \
    file://0007-Enable-GPIOE0-and-GPIOE2-pass-through-by-default.patch \
    file://0008-Allow-monitoring-of-power-control-input-GPIOs.patch \
    file://0009-write-SRAM-panic-words-to-record-kernel-panic.patch \
    file://0010-Change-default-PWM-duty-to-50.patch \
    file://0011-Add-NVIAID-A10-GPU-hardware-monitor-driver-supported.patch \
    file://0012-PATCH-Add-I2C-IPMB-support.patch \
    file://0013-Add-CPU-DTS-margin-temperature-supported-in-AMD-SB-T.patch \
    file://0014-Fix-fan-sensor-disappear-when-power-off-problem.patch \
    file://0015-Change-ADM1272-R-sense-to-0.5.patch \
    file://0016-Fix-adm1272-using-incorrect-i2c-write-command-cause.patch \
    file://0017-Add-fixed-mac-address-for-usb-ethernet.patch \
    file://0018-Add-mct-pdb-driver-to-pmbus-driver-list.patch \
"
