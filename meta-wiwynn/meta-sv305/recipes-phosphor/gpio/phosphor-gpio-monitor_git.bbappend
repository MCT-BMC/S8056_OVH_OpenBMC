FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SYSTEMD_SERVICE_${PN}-monitor += "phosphor-multi-gpio-monitor.service"
SYSTEMD_SERVICE_${PN}-monitor_remove += "phosphor-gpio-monitor@.service"
SYSTEMD_SERVICE_${PN}-presence_remove += "phosphor-gpio-presence@.service"

SRC_URI += "file://0001-Support-event-multi-targets.patch \
            file://0002-Ensure-GPIO-events-order.patch \
            file://0003-Set-BMC_SPD_SMBUS_SEL-to-high-when-post-not-complete.patch \
            file://0004-Set-GPIO-property-to-monitor-on-dbus.patch \
            file://phosphor-multi-gpio-monitor.service \
            file://phosphor-multi-gpio-monitor.json \
           "

DEPENDS += "obmc-libi2c \
           "

RDEPENDS_${PN}-monitor += "obmc-libi2c \
                  "

do_install_append(){
        install -d ${D}/usr/share/phosphor-gpio-monitor
        install -m 644 -D ${WORKDIR}/phosphor-multi-gpio-monitor.json ${D}/usr/share/phosphor-gpio-monitor/phosphor-multi-gpio-monitor.json
        rm ${D}/usr/bin/phosphor-gpio-monitor
        rm ${D}/usr/bin/phosphor-gpio-presence
        rm ${D}/usr/bin/phosphor-gpio-util
        rm ${D}/lib/systemd/system/phosphor-gpio-presence@.service
        rm ${D}/lib/systemd/system/phosphor-gpio-monitor@.service
}
