FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://s7106.cfg \
            file://0001-S7106-Initial-dts.patch \
            file://0002-S7106-Common-sourec-files.patch \
            file://0003-Implement-Heart-Beat-LED.patch \
            file://0004-Set-RTD_1_2_3-to-thermistor-mode.patch \
            file://0005-Support-IPMB-in-OpenBMC.patch \
            file://0006-Modified-ibm-cffps-driver.patch \
            file://0007-Fix-fan-sensor-disappear-when-power-off-problem.patch \
            file://0008-s7106-increase-rofs-space.patch \
            "

