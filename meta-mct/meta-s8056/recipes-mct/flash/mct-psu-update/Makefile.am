sbin_PROGRAMS = psu-update

psu_update_SOURCES = psu-update.cpp chicony-psu-update-manager.cpp delta-psu-update-manager.cpp psu-update-utils.cpp
psu_update_LDFLAGS = $(SYSTEMD_LIBS) \
                      $(SDBUSPLUS_LIBS) \
                      -pthread \
                      -lobmcmisc \
                      -lobmci2c \
                      $(PTHREAD_LIBS) \
                      $(BOOST_CPPFLAGS)
psu_update_CFLAGS = $(SYSTEMD_CFLAGS) \
                     $(SDBUSPLUS_CFLAGS) \
                     -pthread \
                     $(PTHREAD_CFLAGS) \
                     $(BOOST_CPPFLAGS)