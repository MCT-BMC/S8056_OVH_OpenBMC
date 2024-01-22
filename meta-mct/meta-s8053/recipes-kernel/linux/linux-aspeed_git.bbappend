FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += " \
    file://s8053.cfg \
    file://aspeed-bmc-mct-s8053.dts;subdir=git/arch/${ARCH}/boot/dts \
    file://0002-Set-RTD_1_2_3-to-thermistor-mode.patch \
    file://0003-Change-aspeed-rpm-driver-to-falling-edge.patch \
    file://0004-soc-aspeed-Miscellaneous-control-interfaces.patch \
    file://0005-drivers-jtag-Add-Aspeed-SoC-25xx-26xx-families.patch \
    file://0009-Support-logging-flag-to-ast2600-BSRAM-when-kernel-pa.patch \
    file://0010-Change-default-PWM-duty-to-50.patch \
    file://0011-Add-NVIAID-A10-GPU-hardware-monitor-driver-supported.patch \
    file://0012-PATCH-Add-I2C-IPMB-support.patch \
    file://0013-Add-CPU-DTS-margin-temperature-supported-in-AMD-SB-T.patch \
    file://0014-Fix-fan-sensor-disappear-when-power-off-problem.patch \
    file://0016-Fix-adm1272-using-incorrect-i2c-write-command-cause.patch \
    file://0017-Add-fixed-mac-address-for-usb-ethernet.patch \
    file://0018-Add-mct-pdb-driver-to-pmbus-driver-list.patch \
    file://0019-Support-larger-vmalloc-size.patch \
    file://0020-Sync-eSPI-driver-from-ASPEED-SDK-v8.01.patch \
    file://0021-Enable-devmem.patch \
    file://0024-Kernel-Support-windbond-25Q01JVFIQ.patch \
    file://0025-pinctrl-aspeed-Enable-pass-through-on-GPIOP1-and-GPI.patch \
    file://0026-Support-I2C-Mux-PCA9641-driver.patch \
    file://0027-Add-NCT7362-driver.patch \
    file://0028-Update-AMD-SB-RMI-ans-SB-TSI-driver.patch \
    file://0029-nct7904-Setting-multi-function-for-TR1-TR2-TR3.patch \
    file://0030-Add-missing-video-driver-setting-in-dtsi.patch \
    file://0031-nct7904-support-fan-vol-multi-func-options-for-DTS.patch \
    file://0032-Kernel-RTC-Support-NCT3015Y.patch \
    file://0034-Fix-host-MAC-at-gether_register_netdev.patch \
    file://0035-Support-two-ipmb-node.patch \
    file://0036-Replace-sbrmi-read-function-v10-to-v20.patch \
    file://0037-Optimaize-PCA9641-driver.patch \
    file://0038-spi-nor-micron-Support-chip-mt25ql01g.patch \
    file://0039-INA2XX-Modify-calibration-value-and-current-LSB.patch \
    file://0040-Set-EnLTD-to-thermistor-mode.patch \
    file://0041-Add-AST2600-PWM-Tacho-driver-v3.patch \
"
