sbin_PROGRAMS = mct-psu-status-led-handler

mct_psu_status_led_handler_SOURCES = mct-psu-status-led-handler.cpp
mct_psu_status_led_handler_LDFLAGS = $(SYSTEMD_LIBS) \
                                     $(SDBUSPLUS_LIBS) \
                                     -pthread \
                                     -lgpiodcxx \
                                     $(PTHREAD_LIBS) \
                                     $(BOOST_CPPFLAGS)
mct_psu_status_led_handler_CFLAGS = $(SYSTEMD_CFLAGS) \
                                    $(SDBUSPLUS_CFLAGS) \
                                    -pthread \
                                    $(PTHREAD_CFLAGS) \
                                    $(BOOST_CPPFLAGS)