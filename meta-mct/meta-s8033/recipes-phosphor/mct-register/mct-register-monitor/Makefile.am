sbin_PROGRAMS = lpc-interrupt-monitor

lpc_interrupt_monitor_SOURCES = lpc-interrupt-monitor.cpp
lpc_interrupt_monitor_LDFLAGS = $(SYSTEMD_LIBS) \
                         $(SDBUSPLUS_LIBS) \
                         -pthread \
                         $(PTHREAD_LIBS) \
                         $(PHOSPHOR_DBUS_INTERFACES_LIBS) \
                         $(BOOST_CPPFLAGS)
lpc_interrupt_monitor_CFLAGS = $(SYSTEMD_CFLAGS) \
                        $(SDBUSPLUS_CFLAGS) \
                        -pthread \
                        $(PTHREAD_CFLAGS) \
                        $(PHOSPHOR_DBUS_INTERFACES_CFLAGS) \
                        $(BOOST_CPPFLAGS)