FILESEXTRAPATHS_prepend_sv305 := "${THISDIR}/${PN}:"
OBMC_CONSOLE_HOST_TTY = "ttyS0"

SRC_URI += "file://server.ttyS0.conf \
            file://0001-Add-timestamps-to-SOL-buffer.patch \
           "
SRC_URI_remove = "file://${BPN}.conf"

do_install_append() {
        # Install the server configuration
        install -m 0755 -d ${D}${sysconfdir}/${BPN}
        install -m 0644 ${WORKDIR}/*.conf ${D}${sysconfdir}/${BPN}/
        # Remove upstream-provided server configuration
        rm -f ${D}${sysconfdir}/${BPN}/server.ttyVUART0.conf
}
