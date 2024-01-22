FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SRC_URI += " file://0001-Add-fan-control-linear.patch \
             file://0002-Modify-the-fan-control-pid-process.patch \
             file://0003-Add-type-linear-and-debug-manual-mode.patch \
             file://0004-Detecting-fan-failures-and-support-timeout-option.patch \
             file://0005-Add-DIMM-CPU-sensor-and-JSON-defined-cycle-time.patch \
             file://0006-Check-sensor-failures.patch \
             file://0007-Modify-debug-manual-mode-implementation.patch \
             file://0008-Refer-to-linear-result-for-PID-PWM-calculation.patch \
             file://0009-Add-NVMe-NIC-and-DIMM-fan-tables-to-FSC-algorithm.patch \
             file://0010-Fix-CPU-temp-sensor-slow-ready-issue.patch \
             file://0011-Add-sensor-failed-function-for-CPU-NIC-and-FP-inlet.patch \
             file://0012-Fix-for-PWM-accumulation.patch \
             file://0013-Fix-fan-mode-switch-issue-and-keep-fan-mode-after-dc.patch \
             file://writePwm.sh \
             file://phosphor-pid-control.sh \
           "

inherit obmc-phosphor-systemd
SYSTEMD_SERVICE_${PN} = "phosphor-pid-control.service"
EXTRA_OECONF = "--enable-configure-dbus=no"
EXTRA_OECONF += "MAX_FAN_REDUNDANCY=2"

FILES_${PN} += "${bindir}/writePwm.sh"
FILES_${PN} += "${bindir}/phosphor-pid-control.sh"

do_install_append() {
    install -m 0755 ${WORKDIR}/writePwm.sh ${D}${bindir}/
    install -m 0755 ${WORKDIR}/phosphor-pid-control.sh ${D}${bindir}/
}
