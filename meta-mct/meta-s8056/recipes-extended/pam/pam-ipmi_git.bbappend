FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"


SRC_URI += "file://ipmi_pass"

do_install:append() { 
        rm -rf ${D}${sysconfdir}/ipmi_pass
        install -d ${D}/etc/
        install -m 0644 ${WORKDIR}/ipmi_pass ${D}${sysconfdir}/ipmi_pass
}
