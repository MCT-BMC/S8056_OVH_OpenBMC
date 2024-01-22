inherit extrausers
EXTRA_USERS_PARAMS_pn-obmc-phosphor-image = " \
  usermod -p '\$1\$KdSxTq5i\$W7g5v8sWXDsF0zvh.8Rk61' root; \
"

OBMC_IMAGE_EXTRA_INSTALL_append_sv305 += " ipmitool \
                                           entity-manager \
                                           dbus-sensors \
                                           wiwynn-interrupt-dbus-register \
                                           phosphor-gpio-monitor \
                                           sv305-gpio-monitor-register \
                                           sv305-gpio-init \
                                           sv305-powerctrl \
                                           sv305-ipmi-oem \
                                           phosphor-post-code-manager \
                                           phosphor-host-postd \
                                           sv305-service-oem \
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
                                           sv305-bmc-update-sel \
                                           amd-cpu-status-control \
                                           system-watchdog \
                                         "
