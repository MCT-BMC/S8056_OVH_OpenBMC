FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://phosphor-multi-gpio-monitor.service"
SRC_URI += "file://phosphor-gpio-monitor@.service"
SRC_URI += "file://s8033_gpio.json"
SRC_URI += "file://0001-Support-GPIO-interrupt.patch \
            file://0002-Filter-the-debug-log-and-change-service-enable-metho.patch \
           "

FILES:${PN}-monitor += "${datadir}/phosphor-gpio-monitor/phosphor-multi-gpio-monitor.json"

SYSTEMD_SERVICE:${PN}-monitor += "phosphor-multi-gpio-monitor.service"

do_install:append(){
    install -d ${D}/usr/share/phosphor-gpio-monitor
    install -m 0444 ${WORKDIR}/*.json ${D}/usr/share/phosphor-gpio-monitor/phosphor-multi-gpio-monitor.json
}

