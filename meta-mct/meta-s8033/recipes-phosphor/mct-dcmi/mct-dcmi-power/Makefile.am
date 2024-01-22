sbin_PROGRAMS = mct-dcmi-power

mct_dcmi_power_SOURCES = mct-dcmi-power.cpp
mct_dcmi_power_LDFLAGS = $(SYSTEMD_LIBS) \
                         $(SDBUSPLUS_LIBS) \
                         -pthread \
                         -lesmi_oob64 \
                         $(PTHREAD_LIBS) \
                         $(BOOST_CPPFLAGS)
mct_dcmi_power_CFLAGS = $(SYSTEMD_CFLAGS) \
                        $(SDBUSPLUS_CFLAGS) \
                        -pthread \
                        -lesmi_oob64 \
                        $(PTHREAD_CFLAGS) \
                        $(BOOST_CPPFLAGS)
