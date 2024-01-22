inherit extrausers
EXTRA_USERS_PARAMS_pn-obmc-phosphor-image = " \
  usermod -p '\$1\$KdSxTq5i\$W7g5v8sWXDsF0zvh.8Rk61' root; \
"

OBMC_IMAGE_EXTRA_INSTALL_append_sv310a += " ipmitool \
                                           entity-manager \
                                           dbus-sensors \
                                           wiwynn-interrupt-dbus-register \
                                           phosphor-gpio-monitor \
                                           sv310a-gpio-monitor-register \
                                           sv310a-gpio-init \
                                           sv310a-powerctrl \
                                           sv310a-ipmi-oem \
                                           phosphor-post-code-manager \
                                           phosphor-host-postd \
                                           sv310a-service-oem \
                                           phosphor-sel-logger \
                                           discrete-sensor \
                                           guid-generator \
                                           button-handler \
                                           leaky-bucket \
                                           bios-updater \
                                           srvcfg-manager \
                                           cpld-updater \
                                           amd-apml \
                                           twitter-ipmi-oem-bin \
                                           sv310a-bmc-update-sel \
                                           amd-cpu-status-control \
                                           system-watchdog \
                                           phosphor-ipmi-ipmb \
                                           boot-option \
                                           rotate-event-logs \
                                           phosphor-ipmi-flash \
                                           phosphor-ipmi-blobs \
                                           phosphor-image-signing \
                                           sensor-bridged \
                                         "
