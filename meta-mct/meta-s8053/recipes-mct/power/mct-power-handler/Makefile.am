sbin_PROGRAMS = mct-power-handler

mct_power_handler_SOURCES = mct-power-handler.cpp
mct_power_handler_LDFLAGS = $(SYSTEMD_LIBS) \
                           $(SDBUSPLUS_LIBS) \
                           -pthread \
                           $(PTHREAD_LIBS) \
                           $(PHOSPHOR_DBUS_INTERFACES_LIBS) \
                           $(BOOST_CPPFLAGS)
mct_power_handler_CFLAGS = $(SYSTEMD_CFLAGS) \
                          $(SDBUSPLUS_CFLAGS) \
                          -pthread \
                          $(PTHREAD_CFLAGS) \
                          $(PHOSPHOR_DBUS_INTERFACES_CFLAGS) \
                          $(BOOST_CPPFLAGS)