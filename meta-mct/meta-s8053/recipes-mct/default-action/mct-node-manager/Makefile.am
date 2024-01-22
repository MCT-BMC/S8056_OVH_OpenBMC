sbin_PROGRAMS = mct-node-manager

mct_node_manager_SOURCES = mct-node-manager.cpp
mct_node_manager_LDFLAGS = $(SYSTEMD_LIBS) \
                           $(SDBUSPLUS_LIBS) \
                           -pthread \
                           -lgpiodcxx \
                           -li2c \
                           $(PTHREAD_LIBS) \
                           $(BOOST_CPPFLAGS)
mct_node_manager_CFLAGS = $(SYSTEMD_CFLAGS) \
                          $(SDBUSPLUS_CFLAGS) \
                          -pthread \
                          -li2c \
                          $(PTHREAD_CFLAGS) \
                          $(BOOST_CPPFLAGS)
