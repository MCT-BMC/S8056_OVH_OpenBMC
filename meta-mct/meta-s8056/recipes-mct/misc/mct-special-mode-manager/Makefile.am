sbin_PROGRAMS = mct-special-mode-manager

mct_special_mode_manager_SOURCES = mct-special-mode-manager.cpp
mct_special_mode_manager_LDFLAGS = $(SYSTEMD_LIBS) \
                                   $(SDBUSPLUS_LIBS) \
                                   -pthread \
                                   $(PTHREAD_LIBS) \
                                   $(PHOSPHOR_DBUS_INTERFACES_LIBS) \
                                   $(BOOST_CPPFLAGS)
mct_special_mode_manager_CFLAGS = $(SYSTEMD_CFLAGS) \
                                  $(SDBUSPLUS_CFLAGS) \
                                  -pthread \
                                  $(PTHREAD_CFLAGS) \
                                  $(PHOSPHOR_DBUS_INTERFACES_CFLAGS) \
                                  $(BOOST_CPPFLAGS)