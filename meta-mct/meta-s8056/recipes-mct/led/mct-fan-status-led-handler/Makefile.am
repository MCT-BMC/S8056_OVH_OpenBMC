sbin_PROGRAMS = mct-fan-status-led-handler

mct_fan_status_led_handler_SOURCES = mct-fan-status-led-handler.cpp
mct_fan_status_led_handler_LDFLAGS = $(SYSTEMD_LIBS) \
                                     $(SDBUSPLUS_LIBS) \
                                     -pthread \
                                     -lgpiodcxx \
                                     $(PTHREAD_LIBS) \
                                     $(BOOST_CPPFLAGS)
mct_fan_status_led_handler_CFLAGS = $(SYSTEMD_CFLAGS) \
                                    $(SDBUSPLUS_CFLAGS) \
                                    -pthread \
                                    $(PTHREAD_CFLAGS) \
                                    $(BOOST_CPPFLAGS)
