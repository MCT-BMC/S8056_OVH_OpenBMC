FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://0001-Customize-phosphor-watchdog-for-Intel-platforms.patch \
            file://0002-Implement-add-SEL-feature-for-watchdog-timeout.patch \
            file://0003-Add-post-code-SEL-for-FRB2-event.patch \
            file://0004-Send-dbus-signal-with-watchdog-user-when-watchdog-ti.patch \
            file://0005-Fix-building-error-with-obmc-2.11.patch \
           "

# Remove the override to keep service running after DC cycle
SYSTEMD_OVERRIDE:${PN}:remove = "poweron.conf:phosphor-watchdog@poweron.service.d/poweron.conf"
SYSTEMD_SERVICE:${PN} = "phosphor-watchdog.service"
