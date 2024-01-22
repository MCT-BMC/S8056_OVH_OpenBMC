sbin_PROGRAMS = mct-bios-version-handler mct-cpld-version-handler

mct_bios_version_handler_SOURCES = mct-bios-version-handler.cpp
mct_bios_version_handler_LDFLAGS = $(SYSTEMD_LIBS) \
                                   $(SDBUSPLUS_LIBS) \
                                   -pthread \
                                   $(PTHREAD_LIBS) \
                                   $(PHOSPHOR_DBUS_INTERFACES_LIBS)
mct_bios_version_handler_CFLAGS = $(SYSTEMD_CFLAGS) \
                                  $(SDBUSPLUS_CFLAGS) \
                                  -pthread \
                                  $(PTHREAD_CFLAGS) \
                                  $(PHOSPHOR_DBUS_INTERFACES_CFLAGS)

mct_cpld_version_handler_SOURCES = mct-cpld-version-handler.cpp
mct_cpld_version_handler_LDFLAGS = $(SYSTEMD_LIBS) \
                                   $(SDBUSPLUS_LIBS) \
                                   -pthread \
                                   $(PTHREAD_LIBS) \
                                   $(PHOSPHOR_DBUS_INTERFACES_LIBS) \
                                   -li2c
mct_cpld_version_handler_CFLAGS = $(SYSTEMD_CFLAGS) \
                                  $(SDBUSPLUS_CFLAGS) \
                                  -pthread \
                                  $(PTHREAD_CFLAGS) \
                                  $(PHOSPHOR_DBUS_INTERFACES_CFLAGS) \
                                  -li2c