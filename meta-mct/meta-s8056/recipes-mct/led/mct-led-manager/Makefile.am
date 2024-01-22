sbin_PROGRAMS = mct-led-manager

mct_led_manager_SOURCES = mct-led-manager.cpp
mct_led_manager_LDFLAGS = $(SYSTEMD_LIBS) \
                          $(SDBUSPLUS_LIBS) \
                          -pthread \
                          $(PTHREAD_LIBS) \
                          $(BOOST_CPPFLAGS)
mct_led_manager_CFLAGS = $(SYSTEMD_CFLAGS) \
                         $(SDBUSPLUS_CFLAGS) \
                         -pthread \
                         $(PTHREAD_CFLAGS) \
                         $(BOOST_CPPFLAGS)