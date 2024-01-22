sbin_PROGRAMS = mct-sku-manager

mct_sku_manager_SOURCES = mct-sku-manager.cpp
mct_sku_manager_LDFLAGS = $(SYSTEMD_LIBS) \
                          $(SDBUSPLUS_LIBS) \
                          -pthread \
                          -li2c \
                          $(PTHREAD_LIBS) \
                          $(BOOST_CPPFLAGS)
mct_sku_manager_CFLAGS = $(SYSTEMD_CFLAGS) \
                         $(SDBUSPLUS_CFLAGS) \
                         -pthread \
                         -li2c \
                         $(PTHREAD_CFLAGS) \
                         $(BOOST_CPPFLAGS)