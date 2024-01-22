sbin_PROGRAMS = mct-cpld-manager

mct_cpld_manager_SOURCES = mct-cpld-manager.cpp
mct_cpld_manager_LDFLAGS = $(SYSTEMD_LIBS) \
                           $(SDBUSPLUS_LIBS) \
                           -pthread \
                           -lobmci2c \
                           $(PTHREAD_LIBS) \
                           $(PHOSPHOR_DBUS_INTERFACES_LIBS) \
                           $(BOOST_CPPFLAGS)
mct_cpld_manager_CFLAGS = $(SYSTEMD_CFLAGS) \
                          $(SDBUSPLUS_CFLAGS) \
                          -pthread \
                          $(PTHREAD_CFLAGS) \
                          $(PHOSPHOR_DBUS_INTERFACES_CFLAGS) \
                          $(BOOST_CPPFLAGS)