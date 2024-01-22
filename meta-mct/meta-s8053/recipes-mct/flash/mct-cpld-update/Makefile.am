sbin_PROGRAMS = cpld-update

cpld_update_SOURCES = cpld-update.cpp cpld-update-utils.cpp cpld-i2c-update-utils.cpp
cpld_update_LDFLAGS = $(SYSTEMD_LIBS) \
                      $(SDBUSPLUS_LIBS) \
                      -pthread \
                      -lobmci2c \
                      -lobmcmisc \
                      $(PTHREAD_LIBS) \
                      $(BOOST_CPPFLAGS) \
                      -lgpiod 
cpld_update_CFLAGS = $(SYSTEMD_CFLAGS) \
                     $(SDBUSPLUS_CFLAGS) \
                     -pthread \
                     $(PTHREAD_CFLAGS) \
                     $(BOOST_CPPFLAGS)