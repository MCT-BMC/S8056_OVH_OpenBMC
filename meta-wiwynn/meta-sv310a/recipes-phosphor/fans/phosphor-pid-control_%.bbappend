FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

DEPENDS += "libgpiod"

RDEPENDS_${PN} += "bash"

SRC_URI += " \
    file://writePwm.sh \
    file://0001-Do-not-build-unused-codes.patch \
    file://0002-Modify-pid-algorithm-and-add-debug-mode.patch \
    file://0003-Support-tjmax-adjust.patch \
    file://0004-Fan-and-sensor-fail-detection.patch \
    file://0005-Reinitialize-sensors.patch \
    file://0006-Keep-manual-mode-after-host-power-cycle.patch \
    file://0007-Remove-debug-messages-during-initilizing.patch \
    file://0008-Node-policy.patch \
    file://0009-Add-pid-debug-message.patch \
"

inherit obmc-phosphor-systemd
SYSTEMD_SERVICE_${PN} = "phosphor-pid-control.service"

# Do not use the upstream OEM IPMI handler for the fan control.
FILES_${PN}_append_remove = " ${libdir}/ipmid-providers/lib*${SOLIBS}"
FILES_${PN}_append_remove = " ${libdir}/host-ipmid/lib*${SOLIBS}"
FILES_${PN}_append_remove = " ${libdir}/net-ipmid/lib*${SOLIBS}"
FILES_${PN}-dev_append_remove = " ${libdir}/ipmid-providers/lib*${SOLIBSDEV} ${libdir}/ipmid-providers/*.la"

HOSTIPMI_PROVIDER_LIBRARY_remove = "libmanualcmds.so"

FILES_${PN} += "${bindir}/writePwm.sh"

do_install_append() {
    install -m 0755 ${WORKDIR}/writePwm.sh ${D}${bindir}/
}
