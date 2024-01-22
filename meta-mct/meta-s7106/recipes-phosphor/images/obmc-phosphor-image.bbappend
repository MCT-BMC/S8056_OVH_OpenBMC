FLASH_RWFS_OFFSET = "30720"

OBMC_IMAGE_EXTRA_INSTALL_append_s7106 = " libgpiod \
                                          gpio-initial \
                                          hbled-ctrl \
                                          s7106-powerctrl \
                                          s7106-default-action \
                                          caterr-update \ 
                                          phosphor-ipmi-ipmb \
                                          entity-manager \
                                          dbus-sensors \
                                          obmc-ikvm \ 
                                          intel-ipmi-oem \
                                          phosphor-sel-logger \
                                          s7106-sol-processing \
                                          s7106-ipmi-oem \
                                          phosphor-pid-control \
                                          id-button \
                                          pwr-button \
                                          bios-update \
                                          s7106-config \
                                          system-watchdog \
                                          phosphor-u-boot-mgr \
                                          bmc-sel \
                                        "
