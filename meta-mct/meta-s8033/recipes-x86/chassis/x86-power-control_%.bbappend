FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

DEPENDS += "nlohmann-json"
DEPENDS += "phosphor-dbus-interfaces"

SRC_URI += "file://power-config-host0.json;subdir=git/config \
            file://0001-Implement-some-power-feature.patch \
            file://0002-Add-power-button-delay-supported.patch \
            file://0003-Add-power-good-GPIO-trigger-supported.patch \
            file://0004-Add-hot-swap-controler-disable-feature-supported.patch \
            file://0005-Update-NMI-output-default-voltage-level-and-support-.patch \
            file://0006-Add-post-complete-sel-supported.patch \
            file://0007-Improve-power-button-press-action-and-button-lock-de.patch \
            file://0008-Avoiding-DIMM-sensor-status-triggered-for-incorrect-.patch \
            file://0009-Add-post-complete-dbus-signal-supported.patch \
            file://0010-Add-restart-cause-SEL-on-s8033-platform.patch \
            "

SYSTEMD_SERVICE:${PN} += "chassis-system-reset.service \
                          chassis-system-reset.target \
                         "
