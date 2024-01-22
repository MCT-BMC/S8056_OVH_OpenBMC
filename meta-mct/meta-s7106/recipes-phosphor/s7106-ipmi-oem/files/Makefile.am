libmingderoemhandlerdir = ${libdir}/ipmid-providers
libmingderoemhandler_LTLIBRARIES = libmingderoemhandler.la
libmingderoemhandler_la_SOURCES = oemhandler.cpp
libmingderoemhandler_la_LDFLAGS = $(SYSTEMD_LIBS) -version-info 0:0:0 -shared \
                                  $(SDBUSPLUS_LIBS)\
                                  $(PHOSPHOR_DBUS_INTERFACES_LIBS)\
                                  $(PHOSPHOR_LOGGING_CFLAGS)\
                                  $(libmapper_LIBS) 
libmingderoemhandler_la_CFLAGS = $(SYSTEMD_CFLAGS) \
                                 $(SDBUSPLUS_CFLAGS) \
                                 $(PHOSPHOR_DBUS_INTERFACES_LIBS)\
                                 $(PHOSPHOR_LOGGING_CFLAGS)\
                                 $(libmapper_LIBS)
libmingderoemhandler_la_CXXFLAGS = $(SYSTEMD_CFLAGS) $(AM_CXXFLAGS)
