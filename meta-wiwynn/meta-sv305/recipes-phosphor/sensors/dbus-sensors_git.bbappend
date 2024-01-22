FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

PACKAGECONFIG = " \
    adcsensor \
    amdcpusensor \
    dimmsensor \
    fansensor \
    hwmontempsensor \
    nicsensor \
    nvmesensor \
    patternsensor \
    psusensor \
    vrsensor \
    "

PACKAGECONFIG[amdcpusensor] = "-Damdcpu=enabled"
PACKAGECONFIG[dimmsensor] = "-Ddimm=enabled"
PACKAGECONFIG[nicsensor] = "-Dnic=enabled"
PACKAGECONFIG[nvmesensor] = "-Dnvme-i2c=enabled"
PACKAGECONFIG[patternsensor] = "-Dpattern=enabled"
PACKAGECONFIG[vrsensor] = "-Dvr=enabled"


SYSTEMD_SERVICE_${PN} += " xyz.openbmc_project.cpusensor.service"
SYSTEMD_SERVICE_${PN} += " xyz.openbmc_project.dimmsensor.service"
SYSTEMD_SERVICE_${PN} += " xyz.openbmc_project.nicsensor.service"
SYSTEMD_SERVICE_${PN} += " xyz.openbmc_project.nvmesensor.service"
SYSTEMD_SERVICE_${PN} += " xyz.openbmc_project.patternsensor.service"
SYSTEMD_SERVICE_${PN} += " xyz.openbmc_project.vrsensor.service"

SRC_URI += " \
    file://0001-Add-to-configure-MaxValue-and-MinValue-in-ADC-sensor.patch \
    file://0002-Revise-host-power-on-and-post-complete-judgements.patch \
    file://0003-Replacing-deadline-timer-with-steady-timer.patch \
    file://0004-Checking-reading-state-in-ADCSensor.patch \
    file://0005-Sensor-services-wait-for-entity-manager-dbus-done.patch \
    file://0006-Add-to-support-NVME-sensor.patch \
    file://0007-Add-to-support-PSU-sensor.patch \
    file://0008-Add-to-support-NIC-sensor.patch \
    file://0009-Add-to-support-DIMM-sensor.patch \
    file://0010-Add-to-support-AMD-CPU-sensor.patch \
    file://0011-Add-to-support-VR-sensor.patch \
    file://0012-Fan-fail-host-off-and-log-sel.patch \
    file://0013-Add-sensor-fail-sel.patch \
    file://0014-Add-sel-when-sensors-driver-failed.patch \
    file://0015-Check-threshold-immediately-when-setting-threshold.patch \
    file://0016-Check-power-on-per-second-in-threshold-delay.patch \
    file://0017-Add-pattern-match-sensor.patch \
    file://0018-Remove-negative-sensor-reading-value.patch \
    file://0019-Add-retries-for-ADC-sensor-boundry-condition.patch \
    file://0020-Set-fan-sensor-value-as-0-when-disconnected.patch \
"

DEPENDS += " obmc-libdbus"
DEPENDS += " obmc-libi2c"
DEPENDS += " obmc-libmisc"
DEPENDS += " phosphor-ipmi-host"

RDEPENDS_${PN} += " obmc-libdbus"
RDEPENDS_${PN} += " obmc-libi2c"
RDEPENDS_${PN} += " obmc-libmisc"

