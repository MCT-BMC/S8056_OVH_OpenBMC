sbin_PROGRAMS = bmc2-update

bmc2_update_SOURCES = bmc2-update.cpp
bmc2_update_LDFLAGS = $(SYSTEMD_LIBS) \
                      $(SDBUSPLUS_LIBS) \
                      -pthread \
                      $(PTHREAD_LIBS) \
                      $(BOOST_CPPFLAGS) \
                      -lgpiod
bmc2_update_CFLAGS = $(SYSTEMD_CFLAGS) \
                     $(SDBUSPLUS_CFLAGS) \
                     -pthread \
                     $(PTHREAD_CFLAGS) \
                     $(BOOST_CPPFLAGS)