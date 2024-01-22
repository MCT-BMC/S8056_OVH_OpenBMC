sbin_PROGRAMS = mct-post-code-manager

mct_post_code_manager_SOURCES = mct-post-code-manager.cpp
mct_post_code_manager_LDFLAGS = $(SYSTEMD_LIBS) \
                                $(SDBUSPLUS_LIBS) \
                                -pthread \
                                $(PTHREAD_LIBS) \
                                $(PHOSPHOR_DBUS_INTERFACES_LIBS) \
                                $(BOOST_CPPFLAGS)
mct_post_code_manager_CFLAGS = $(SYSTEMD_CFLAGS) \
                               $(SDBUSPLUS_CFLAGS) \
                               -pthread \
                               $(PTHREAD_CFLAGS) \
                               $(PHOSPHOR_DBUS_INTERFACES_CFLAGS) \
                               $(BOOST_CPPFLAGS)