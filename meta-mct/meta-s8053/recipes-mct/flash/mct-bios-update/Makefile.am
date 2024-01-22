sbin_PROGRAMS = bios-update

bios_update_SOURCES = bios-update.cpp
bios_update_LDFLAGS = $(SYSTEMD_LIBS) \
                      $(SDBUSPLUS_LIBS) \
                      -pthread \
                      -lobmcmisc \
                      $(PTHREAD_LIBS) \
                      $(BOOST_CPPFLAGS) \
                      -lgpiod
bios_update_CFLAGS = $(SYSTEMD_CFLAGS) \
                     $(SDBUSPLUS_CFLAGS) \
                     -pthread \
                     $(PTHREAD_CFLAGS) \
                     $(BOOST_CPPFLAGS)