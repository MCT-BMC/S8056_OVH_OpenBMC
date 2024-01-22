inherit obmc-phosphor-dbus-service
inherit obmc-phosphor-systemd

SUMMARY = "CPU Crashdump"
DESCRIPTION = "CPU utilities for dumping CPU Crashdump and registers over PECI"

FILESEXTRAPATHS_append := "${THISDIR}/files:"
DEPENDS = "boost cjson sdbusplus safec gtest libpeci"

LICENSE = "Proprietary"
LIC_FILES_CHKSUM = "file://LICENSE;md5=26bb6d0733830e7bab774914a8f8f20a"
SRCREV = "0.5"

S = "${WORKDIR}"
SRC_URI = "file://crashdump.tgz \
           file://LICENSE  \
          "

INSANE_SKIP_${PN} += "already-stripped"
INSANE_SKIP_${PN} += "ldflags"


SYSTEMD_SERVICE_${PN} += "com.intel.crashdump.service"
DBUS_SERVICE_${PN} += "com.intel.crashdump.service"

do_install() {
    install -d ${D}/usr/bin
    install -m 0755 ${S}crashdump ${D}/usr/bin/
}
