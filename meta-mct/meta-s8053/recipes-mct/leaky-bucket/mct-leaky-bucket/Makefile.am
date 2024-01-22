sbin_PROGRAMS = mct-leaky-bucket

mct_leaky_bucket_SOURCES = mct-leaky-bucket.cpp
mct_leaky_bucket_LDFLAGS = $(SYSTEMD_LIBS) \
                           $(SDBUSPLUS_LIBS) \
                           -pthread \
                           $(PTHREAD_LIBS) \
                           $(PHOSPHOR_DBUS_INTERFACES_LIBS) \
                           $(BOOST_CPPFLAGS)
mct_leaky_bucket_CFLAGS = $(SYSTEMD_CFLAGS) \
                          $(SDBUSPLUS_CFLAGS) \
                          -pthread \
                          $(PTHREAD_CFLAGS) \
                          $(PHOSPHOR_DBUS_INTERFACES_CFLAGS) \
                          $(BOOST_CPPFLAGS)