FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

EXTRA_OECMAKE += "-DSEL_LOGGER_MONITOR_THRESHOLD_EVENTS=ON"

# Max size 256k
EXTRA_OECMAKE += "-DMAX_SEL_SIZE=262144"

EXTRA_OECMAKE += "-DALMOST_FULL_PERCENTAGE=75"

SRC_URI += "file://0001-Add-clear-sel-method-and-adjust-service-order.patch \
            file://0002-Set-BMC-status-LED-while-critical-events-occur.patch \
            file://0003-Support-full-and-almost-full-sel.patch \
            file://0004-Use-sensor-yaml-MBR-in-threshold-event-sel.patch \
            file://0005-Revise-the-start-up-time-of-sel-logger-service.patch \
            file://0006-Control-the-behavior-about-sensor-fail.patch \
           "
