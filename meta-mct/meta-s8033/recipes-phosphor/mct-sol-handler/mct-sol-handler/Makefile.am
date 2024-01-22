sbin_PROGRAMS = mct-sol-handler

mct_sol_handler_SOURCES = mct-sol-handler.cpp
mct_sol_handler_LDFLAGS = $(SYSTEMD_LIBS) \
                          $(SDBUSPLUS_LIBS) \
                          -pthread \
                          $(PTHREAD_LIBS) \
                          $(BOOST_CPPFLAGS)
mct_sol_handler_CFLAGS = $(SYSTEMD_CFLAGS) \
                         $(SDBUSPLUS_CFLAGS) \
                         -pthread \
                         $(PTHREAD_CFLAGS) \
                         $(BOOST_CPPFLAGS)