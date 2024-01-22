sbin_PROGRAMS = system-fmc-watchdog

system_fmc_watchdog_SOURCES = system-fmc-watchdog.cpp
system_fmc_watchdog_LDFLAGS = $(SYSTEMD_LIBS) \
                           -pthread \
                           -lobmcmisc \
                           $(PTHREAD_LIBS) \
                           $(BOOST_CPPFLAGS)
system_fmc_watchdog_CFLAGS = $(SYSTEMD_CFLAGS) \
                          -pthread \
                          -lobmcmisc \
                          $(PTHREAD_CFLAGS) \
                          $(BOOST_CPPFLAGS)
