FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"
OBMC_CONSOLE_HOST_TTY = "ttyS2"

SRC_URI += "file://obmc-console-ssh@.service \
            file://server.ttyS2.conf \
            file://0001-Support-start-preparing-and-stop-commnad-configurati.patch \
            file://0002-Add-timestamps-to-SOL-buffer.patch \
            file://0003-Set-the-SOL-buffer-size-from-16m-to-4m.patch \
            file://0005-Modify-file-path-and-max-size-of-console-log.patch \
            "
SRC_URI:remove = "file://${BPN}.conf"

do_install:append() {
        # Install the server configuration
        install -m 0755 -d ${D}${sysconfdir}/${BPN}
        install -m 0644 ${WORKDIR}/*.conf ${D}${sysconfdir}/${BPN}/
        # Remove upstream-provided server configuration
        rm -f ${D}${sysconfdir}/${BPN}/server.ttyVUART0.conf
}