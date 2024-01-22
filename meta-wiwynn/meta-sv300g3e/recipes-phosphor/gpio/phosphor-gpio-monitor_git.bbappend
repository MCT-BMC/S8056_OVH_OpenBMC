FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://0001-Support-GPIO-interrupt.patch \
            file://phosphor-multi-gpio-monitor.service \
            file://phosphor-multi-gpio-monitor.json \
           "

do_install_append(){
        install -d ${D}/usr/share/phosphor-gpio-monitor
        install -m 644 -D ${WORKDIR}/phosphor-multi-gpio-monitor.json ${D}/usr/share/phosphor-gpio-monitor/phosphor-multi-gpio-monitor.json
}