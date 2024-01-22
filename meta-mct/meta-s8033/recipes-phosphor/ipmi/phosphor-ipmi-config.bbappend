FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://sdr_general.json \
            file://sdr_event_only.json \
           "
FILES:${PN} += "${datadir}/ipmi-providers/sdr_general.json"
FILES:${PN} += "${datadir}/ipmi-providers/sdr_event_only.json"

do_install:append() {
    install -m 0644 -D ${WORKDIR}/sdr_general.json \
    ${D}${datadir}/ipmi-providers/sdr_general.json

    install -m 0644 -D ${WORKDIR}/sdr_event_only.json \
    ${D}${datadir}/ipmi-providers/sdr_event_only.json
}