sbin_PROGRAMS = vr-update

vr_update_SOURCES = vr-update.cpp vr-update-manager.cpp vr-update-utils.cpp
vr_update_LDFLAGS = $(SYSTEMD_LIBS) \
                    $(SDBUSPLUS_LIBS) \
                    -pthread \
                    -lobmcmisc \
                    -lobmci2c \
                    $(PTHREAD_LIBS) \
                    $(BOOST_CPPFLAGS)
vr_update_CFLAGS = $(SYSTEMD_CFLAGS) \
                   $(SDBUSPLUS_CFLAGS) \
                   -pthread \
                   $(PTHREAD_CFLAGS) \
                   $(BOOST_CPPFLAGS)