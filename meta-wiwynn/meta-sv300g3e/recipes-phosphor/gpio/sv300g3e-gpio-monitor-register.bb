SUMMARY = "SV300G3-E event register application for gpio monitor"
PR = "r1"

inherit obmc-phosphor-systemd

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${WIWYNNBASE}/COPYING.apache-2.0;md5=34400b68072d710fecd0a2940a0d1658"

DEPENDS += "virtual/obmc-gpio-monitor"
RDEPENDS_${PN} += "virtual/obmc-gpio-monitor"

S = "${WORKDIR}"
SRC_URI += "file://toggle_identify_led.sh \
            file://id-button-pressed.service \
            file://end-of-post-event.sh \
            file://end-of-post-event.service \
            file://SetPowerGoodPropertyOff.service \
            file://SetPowerGoodPropertyOn.service \
            file://caterr-event.sh \
            file://caterr-event.service \
           "

do_install() {
        install -d ${D}${bindir}
        install -d ${D}${sbindir}
        install -m 0755 ${WORKDIR}/toggle_identify_led.sh ${D}${bindir}/toggle_identify_led.sh
        install -m 0755 ${WORKDIR}/end-of-post-event.sh ${D}${sbindir}/end-of-post-event.sh
        install -m 0755 ${WORKDIR}/caterr-event.sh ${D}${sbindir}/caterr-event.sh
}

SYSTEMD_SERVICE_${PN} += "id-button-pressed.service"
SYSTEMD_SERVICE_${PN} += "end-of-post-event.service"
SYSTEMD_SERVICE_${PN} += "SetPowerGoodPropertyOff.service"
SYSTEMD_SERVICE_${PN} += "SetPowerGoodPropertyOn.service"
SYSTEMD_SERVICE_${PN} += "caterr-event.service"
