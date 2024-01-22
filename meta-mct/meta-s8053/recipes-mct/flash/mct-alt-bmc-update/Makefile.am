sbin_PROGRAMS = alt-bmc-update

alt_bmc_update_SOURCES = alt-bmc-update.cpp
alt_bmc_update_LDFLAGS = $(SYSTEMD_LIBS) \
                      $(SDBUSPLUS_LIBS) \
                      -pthread \
                      $(PTHREAD_LIBS) \
                      $(BOOST_CPPFLAGS) \
                      -lgpiod
alt_bmc_update_CFLAGS = $(SYSTEMD_CFLAGS) \
                     $(SDBUSPLUS_CFLAGS) \
                     -pthread \
                     $(PTHREAD_CFLAGS) \
                     $(BOOST_CPPFLAGS)