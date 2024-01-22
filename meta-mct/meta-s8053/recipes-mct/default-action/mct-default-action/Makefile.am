sbin_PROGRAMS = mct-default-setting

mct_default_setting_SOURCES = mct-default-setting.cpp
mct_default_setting_LDFLAGS = $(SYSTEMD_LIBS) \
                              $(SDBUSPLUS_LIBS) \
                              -pthread \
                              -lobmcmisc \
                              $(PTHREAD_LIBS) \
                              $(PHOSPHOR_DBUS_INTERFACES_LIBS) \
                              $(BOOST_CPPFLAGS)
mct_default_setting_CFLAGS = $(SYSTEMD_CFLAGS) \
                             $(SDBUSPLUS_CFLAGS) \
                             -pthread \
                             $(PTHREAD_CFLAGS) \
                             $(PHOSPHOR_DBUS_INTERFACES_CFLAGS) \
                             $(BOOST_CPPFLAGS)