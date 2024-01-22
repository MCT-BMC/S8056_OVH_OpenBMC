inherit extrausers
EXTRA_USERS_PARAMS_pn-obmc-phosphor-image = " \
  usermod -p '\$1\$KdSxTq5i\$W7g5v8sWXDsF0zvh.8Rk61' root; \
"

OBMC_IMAGE_EXTRA_INSTALL_append_sv300g3e += " entity-manager \
                                              dbus-sensors \
                                              intel-dbus-interfaces \
                                              intel-ipmi-oem \
                                              phosphor-pid-control \
                                              sv300g3e-gpio-init \
                                              sv300g3e-powerctrl \
                                              ipmitool \
                                              phosphor-sel-logger \
                                              phosphor-ipmi-ipmb \
                                              sv300g3e-service-oem \
                                              pattern-matching \
                                              sv300g3e-ipmi-oem \
                                              cpu-state \
                                              leaky-bucket \
                                              wiwynn-interrupt-dbus-register \
                                              sv300g3e-gpio-monitor-register \
                                              phosphor-gpio-monitor \
                                              crashdump \
                                              bios-updater \
                                              cpu-prochot-event \
                                              phosphor-post-code-manager \
                                              phosphor-host-postd \
                                              guid-generator \
                                              sv300g3e-bmc-update-sel \
                                              intel-asd \
                                              cpld-updater \
                                              system-watchdog \
                                              button-handler \
                                            "
