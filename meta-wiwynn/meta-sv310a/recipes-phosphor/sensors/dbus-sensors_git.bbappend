FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

PACKAGECONFIG = " \
    adcsensor \
    amdcpusensor \
    dimmsensor \
    fansensor \
    hwmontempsensor \
    i2csensor \
    nvmesensor \
    patternsensor \
    psusensor \
    "

PACKAGECONFIG[amdcpusensor] = "-Damdcpu=enabled"
PACKAGECONFIG[dimmsensor] = "-Ddimm=enabled"
PACKAGECONFIG[i2csensor] = "-Di2c=enabled"
PACKAGECONFIG[nvmesensor] = "-Dnvme-i2c=enabled"
PACKAGECONFIG[patternsensor] = "-Dpattern=enabled"

SYSTEMD_SERVICE_${PN} += " xyz.openbmc_project.cpusensor.service"
SYSTEMD_SERVICE_${PN} += " xyz.openbmc_project.dimmsensor.service"
SYSTEMD_SERVICE_${PN} += " xyz.openbmc_project.i2csensor.service"
SYSTEMD_SERVICE_${PN} += " xyz.openbmc_project.nvmesensor.service"
SYSTEMD_SERVICE_${PN} += " xyz.openbmc_project.patternsensor.service"

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
    file://0011-Fan-fail-host-off-and-log-sel.patch \
    file://0012-Add-sensor-fail-sel.patch \
    file://0013-Add-sel-when-sensors-driver-failed.patch \
    file://0014-Check-threshold-immediately-when-setting-threshold.patch \
    file://0015-Check-power-on-per-second-in-threshold-delay.patch \
    file://0016-Add-pattern-match-sensor.patch \
    file://0017-Remove-negative-sensor-reading-value.patch \
    file://0018-Add-retries-for-ADC-sensor-boundry-condition.patch \
    file://0019-Set-fan-sensor-value-as-0-when-disconnected.patch \
    file://0020-Add-VR-type-in-PSU-sensor.patch \
    file://0021-NVME-sensor-switch-mux.patch \
    file://0022-Support-fan-presence-sel-and-pwm-readstate.patch \
    file://0023-Ignore-fan-fail-in-the-other-nodes.patch \
    file://0024-Add-option-for-mux-lock-algorithm.patch \
    file://0025-Add-switch-mux-parameter-in-DIMM-sensor.patch \
    file://0026-Fan-sensor-and-ipmb-sensor-chaneged.patch \
    file://0027-Disable-fan-fail-power-off-host.patch \
    file://0028-Set-available-when-sensor-fail.patch \
    file://0029-Support-fan-power-sensor.patch \
    file://0030-Check-fan-power-when-AC-on.patch \
    file://0031-Revise-the-source-of-GPIO-bridge-enabled-time.patch \
    file://0032-Update-value-to-0-when-fan-is-absent.patch \
    file://0033-Fix-DIMM-temperature-upper-critical-issue-during-host-powers-off.patch \
    file://0034-Support-fan-sensor-fail.patch \
    file://0035-Fix-core-dump-during-restoring-to-default.patch \
"

DEPENDS += " obmc-libdbus"
DEPENDS += " obmc-libi2c"
DEPENDS += " obmc-libmisc"
DEPENDS += " phosphor-ipmi-host"
DEPENDS += " phosphor-dbus-interfaces"

RDEPENDS_${PN} += " obmc-libdbus"
RDEPENDS_${PN} += " obmc-libi2c"
RDEPENDS_${PN} += " obmc-libmisc"

EXTRA_OEMESON += "-Dinsecure-sensor-override=enabled"
