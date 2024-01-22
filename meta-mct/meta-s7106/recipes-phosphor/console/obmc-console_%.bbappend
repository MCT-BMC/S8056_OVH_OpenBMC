FILESEXTRAPATHS_prepend_s7106 := "${THISDIR}/${PN}:"
OBMC_CONSOLE_HOST_TTY = "ttyS2"

SRC_URI += "file://obmc-console-ssh@.service"
