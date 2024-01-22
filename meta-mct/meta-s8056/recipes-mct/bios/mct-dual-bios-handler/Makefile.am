sbin_PROGRAMS = mct-dual-bios-handler

mct_dual_bios_handler_SOURCES = mct-dual-bios-handler.cpp
mct_dual_bios_handler_LDFLAGS = $(SYSTEMD_LIBS) \
                                $(SDBUSPLUS_LIBS) \
                                -pthread \
                                -lobmcmisc \
                                $(PTHREAD_LIBS) \
                                $(PHOSPHOR_DBUS_INTERFACES_LIBS) \
                                $(BOOST_CPPFLAGS) \
                                -lgpiodcxx
mct_dual_bios_handler_CFLAGS = $(SYSTEMD_CFLAGS) \
                               $(SDBUSPLUS_CFLAGS) \
                               -pthread \
                               $(PTHREAD_CFLAGS) \
                               $(PHOSPHOR_DBUS_INTERFACES_CFLAGS) \
                               $(BOOST_CPPFLAGS)