FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += " \
    file://IPv6.conf \
"

do_install:append() {
    install -d ${D}/${libdir}/sysctl.d
    install -D -m 644 ${WORKDIR}/IPv6.conf ${D}/${libdir}/sysctl.d/IPv6.conf
}

