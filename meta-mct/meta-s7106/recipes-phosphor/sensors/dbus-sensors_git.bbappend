FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://0001-Support-temp2-and-share-the-same-threshold.patch \
            file://0002-s7106-add-supported-pmbus-name-cffps1.patch \
            file://0003-s7106-add-cpu-and-dimm-sesnor-via-ME.patch \
            file://0004-Add-gpio-dbus-sensor-for-caterr-feature.patch \
            file://0005-s7106-ipmbsensor-add-DIMM-temperature-channl-4-7-sup.patch \
            file://0006-Filter-out-FAN-threshold-event-during-power-transiti.patch \
            file://0007-s7106-new-processor-sensor-to-replace-gpio-sensor.patch \
            file://0008-s7106-add-ipmi-ACPI-sensor.patch \
            file://0009-Support-system-restart-sensor-for-IPMI.patch \
            file://0010-pwr-button-sel-log.patch \
            file://0011-s7106-add-OCP-NIC-temperature-sensor.patch \
            file://0012-version-change-sel-log.patch \
            file://0013-add-ipmi-power-status-sensor.patch \
            file://0014-FRU-state-sensor-for-power-cycle-log.patch \
            "

#SYSTEMD_SERVICE_${PN} += " xyz.openbmc_project.gpiosensor.service"
SYSTEMD_SERVICE_${PN} += " xyz.openbmc_project.processorsensor.service"
SYSTEMD_SERVICE_${PN} += " xyz.openbmc_project.acpisensor.service"
SYSTEMD_SERVICE_${PN} += " xyz.openbmc_project.hostresetsensor.service"
SYSTEMD_SERVICE_${PN} += " xyz.openbmc_project.pwrbuttonsensor.service"
SYSTEMD_SERVICE_${PN} += " xyz.openbmc_project.versionchangesensor.service"
SYSTEMD_SERVICE_${PN} += " xyz.openbmc_project.aclostsensor.service"
SYSTEMD_SERVICE_${PN} += " xyz.openbmc_project.frustatesensor.service"
