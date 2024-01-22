FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

inherit obmc-phosphor-systemd

SRC_URI += "file://0001-Supprot-debug-mode-and-derivative-control-for-PID-al.patch \
            file://0002-Modify-Fan-and-sensor-fail-detection.patch \
            file://controlFan.sh \
            file://phosphor-pid-control.sh \
            file://writePwm.sh \
            file://setting-fan-poweroff.service \
            file://setting-fan-poweron.service \
            file://setting-fan-init.service \
           "

RDEPENDS:${PN} += "bash"

FILES:${PN} += "${bindir}/controlFan.sh"
FILES:${PN} += "${bindir}/phosphor-pid-control.sh"
FILES:${PN} += "${bindir}/writePwm.sh"

SYSTEMD_SERVICE:${PN} = "phosphor-pid-control.service \
                         setting-fan-poweroff.service \
                         setting-fan-poweron.service \
                         setting-fan-init.service \
                        "

do_install:append() {
    install -m 0755 ${WORKDIR}/controlFan.sh ${D}${bindir}/
    install -m 0755 ${WORKDIR}/phosphor-pid-control.sh ${D}${bindir}/
    install -m 0755 ${WORKDIR}/writePwm.sh ${D}${bindir}/
}
