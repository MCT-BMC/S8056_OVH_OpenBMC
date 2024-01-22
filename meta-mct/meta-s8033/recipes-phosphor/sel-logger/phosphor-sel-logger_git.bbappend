FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

#Enable threshold monitoring
EXTRA_OECMAKE += "-DSEL_LOGGER_MONITOR_THRESHOLD_EVENTS=ON"
EXTRA_OECMAKE += "-DREDFISH_LOG_MONITOR_PULSE_EVENTS=ON"
EXTRA_OECMAKE += "-DSEL_LOGGER_MONITOR_THRESHOLD_ALARM_EVENTS=ON"

SRC_URI += "file://0001-Change-ipmi_sel-location-to-persistent-folder.patch \
            file://0002-Add-fault-led-alert-feature-supported.patch \
            file://0003-Fix-IPMI-SEL-entry-in-command-does-not-match-IPMI-SE.patch \
            file://0004-Add-filter-function-supported-for-specified-sensor.patch \
            file://0005-Add-specified-sensor-event-to-trigger-prochot-relate.patch \
            "

DEPENDS += "i2c-tools \
            phosphor-dbus-interfaces \
           "

RDEPENDS:${PN} += "i2c-tools \
                   phosphor-dbus-interfaces \
                  "
