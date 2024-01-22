FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://0001-Modify-the-tap-state-and-xfer-structures-to-match-with-jtag-driver.patch \
            file://0002-Set-PLTRST_N-pin-type-to-PIN_NONE-to-avoid-changing-the-LPC-multi-func.patch \
            file://intel.asd.service \
           "

SYSTEMD_SERVICE_${PN} += "intel.asd.service"

do_install_append() {
    install -d ${D}/lib/systemd/system/
    install -m 0755 ${WORKDIR}/intel.asd.service ${D}/lib/systemd/system/
}
