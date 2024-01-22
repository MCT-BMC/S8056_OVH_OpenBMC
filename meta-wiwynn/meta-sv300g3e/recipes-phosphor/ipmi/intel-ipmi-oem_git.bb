SUMMARY = "Intel OEM IPMI commands"
DESCRIPTION = "Intel OEM IPMI commands"

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=a6a4edad4aed50f39a66d098d74b265b"

SRC_URI = "git://github.com/openbmc/intel-ipmi-oem \
           file://0003-Support-FRU-device-id-and-size-configuration-in-JSON.patch \
           file://0004-Support-event-only-type-SDR-format.patch \
           file://0005-Add-SDR-unit-hit-for-SOL-pattern-sensor.patch \
           file://0006-Support-non-volatile-IPMI-SEL-record.patch \
           file://0007-Support-the-reading-unavailable-bit.patch \
           file://0008-Add-sensor-type-system-event.patch \
           file://0009-Support-IPMI-Get-and-Add-SEL-Entry-commands.patch \
           file://0010-FRU-state-sensor-for-power-cycle-SEL.patch \
           file://0011-Handle-no-cacheTimer-case-in-replaceCacheFru-func.patch \
           file://0012-Support-BIOS-SDR-configuration-in-JSON.patch \
           file://0013-Modify-the-chassis-identify-behavior-to-solid-on.patch \
           file://0014-Add-sensor-type-power-supply-and-power-unit.patch \
           file://0015-Support-a-Memory-Correctable-ECC-event-log-filter-for-Leaky-Bucket.patch \
           file://0016-Correct-the-response-of-in-band-bridge-IPMB-command.patch \
           file://0017-Enhance-the-precision-of-sensor-reading-conversion.patch \
           file://0018-Support-Set-SEL-Time-command.patch \
           file://0019-Support-SEL-entry-for-IPMI-SEL-clear-event.patch \
           file://0020-Implement-BMC-Update-SEL.patch \
          "
# SRCREV = "ba9c1765f5edc2f0891e012f33b3059a0dbd7ff1"
SRCREV = "147daec5fcfcdacd8813eab6a7735d0f1b615c8a"

S = "${WORKDIR}/git"
PV = "0.1+git${SRCPV}"

DEPENDS = "boost phosphor-ipmi-host phosphor-logging systemd intel-dbus-interfaces"

inherit cmake obmc-phosphor-ipmiprovider-symlink

EXTRA_OECMAKE="-DENABLE_TEST=0 -DYOCTO=1"

LIBRARY_NAMES = "libzinteloemcmds.so"

HOSTIPMI_PROVIDER_LIBRARY += "${LIBRARY_NAMES}"
NETIPMI_PROVIDER_LIBRARY += "${LIBRARY_NAMES}"

FILES_${PN}_append = " ${libdir}/ipmid-providers/lib*${SOLIBS}"
FILES_${PN}_append = " ${libdir}/host-ipmid/lib*${SOLIBS}"
FILES_${PN}_append = " ${libdir}/net-ipmid/lib*${SOLIBS}"
FILES_${PN}-dev_append = " ${libdir}/ipmid-providers/lib*${SOLIBSDEV}"

do_install_append(){
   install -d ${D}${includedir}/intel-ipmi-oem
   install -m 0644 -D ${S}/include/*.hpp ${D}${includedir}/intel-ipmi-oem
}
