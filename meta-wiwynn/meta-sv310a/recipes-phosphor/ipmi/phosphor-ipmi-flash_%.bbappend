FILESEXTRAPATHS_prepend_sv310a := "${THISDIR}/${PN}:"

inherit obmc-phosphor-systemd

RDEPENDS_${PN}_append = " openssl-bin bash"

PACKAGECONFIG_append = " reboot-update static-bmc net-bridge aspeed-p2a"

SRC_URI += " file://verify_image.sh \
             file://update_image.sh \
           "

SYSTEMD_SERVICE_${PN} += "verify-image.service \
                          update-image.service \
                         "

EXTRA_OECONF_append = " CPPFLAGS='-DENABLE_PCI_BRIDGE'"
EXTRA_OECONF_append += " enable_reboot_update=no "

do_install_append() {
    install -d ${D}${bindir}
    install -m 0755 ${WORKDIR}/verify_image.sh ${D}${bindir}
    install -m 0755 ${WORKDIR}/update_image.sh ${D}${bindir}
}


