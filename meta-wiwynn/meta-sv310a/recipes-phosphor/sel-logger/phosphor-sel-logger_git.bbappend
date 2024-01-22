FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

EXTRA_OECMAKE += "-DSEL_LOGGER_MONITOR_THRESHOLD_EVENTS=ON"

# Max size 256k
EXTRA_OECMAKE += "-DMAX_SEL_SIZE=262144"

EXTRA_OECMAKE += "-DALMOST_FULL_PERCENTAGE=75"

SRC_URI += " \
    file://0001-Add-clear-sel-method-and-almost-full-and-full-sel.patch \
    file://0002-Use-sensor-yaml-MBR-in-threshold-event-sel.patch \
    file://0003-Monitor-sensor-fail-signal.patch \
    file://0004-Set-led-when-sensor-fail-events-occur.patch \
    file://0005-Fix-record-ID-issue-after-clearing-SEL.patch \
"
