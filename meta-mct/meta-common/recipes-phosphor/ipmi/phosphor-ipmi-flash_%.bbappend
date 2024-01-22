FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

inherit obmc-phosphor-systemd

SRCREV = "0df4085097d8454a918de8fed76b0574bb2da9db"

EXTRA_OECONF:append = " --enable-net-bridge"
EXTRA_OECONF:append = " --enable-static-layout"
EXTRA_OECONF:append = " --enable-reboot-update"
EXTRA_OECONF:append = " --enable-aspeed-p2a"
EXTRA_OECONF:append = " --enable-aspeed-lpc"

SRC_URI += "file://reboot-bmc-with-delay.service \
            file://bmc-reboot.service \
            file://0001-Support-latest-host-tool-and-fix-tool-interrupt-issu.patch \
            file://0002-Modify-phosphor-ipmi-flash-update-target-to-service.patch \
            file://0003-Support-fixed-bmc-device-when-updating-BMC-flash-dev.patch \
           "

SYSTEMD_SERVICE:${PN} += "reboot-bmc-with-delay.service \
                          bmc-reboot.service \
                         "