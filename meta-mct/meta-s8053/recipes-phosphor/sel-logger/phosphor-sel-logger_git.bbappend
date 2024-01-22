FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

#Enable threshold monitoring
EXTRA_OECMAKE += "-DSEL_LOGGER_MONITOR_THRESHOLD_EVENTS=ON"
EXTRA_OECMAKE += "-DREDFISH_LOG_MONITOR_PULSE_EVENTS=ON"
EXTRA_OECMAKE += "-DSEL_LOGGER_MONITOR_THRESHOLD_ALARM_EVENTS=ON"

PACKAGECONFIG:append = " log-threshold"

SRC_URI += "file://0001-Change-ipmi_sel-location-to-persistent-folder.patch \
            file://0002-Add-fault-led-alert-feature-supported.patch \
            file://0003-Fix-IPMI-SEL-entry-in-command-does-not-match-IPMI-SE.patch \
            file://0004-Add-filter-function-supported-for-specified-sensor.patch \
            file://0005-Modify-SEL-file-checking-algorithm.patch \
            "

SRC_URI += "file://include/filterutils.hpp;subdir=git/ \
            "

DEPENDS += "i2c-tools \
            phosphor-dbus-interfaces \
            libobmc-i2c \
           "

RDEPENDS:${PN} += "i2c-tools \
                   phosphor-dbus-interfaces \
                   libobmc-i2c \
                  "
