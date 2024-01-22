sbin_PROGRAMS = gpio-initial

gpio_initial_SOURCES = gpio-initial.cpp
gpio_initial_LDFLAGS = $(SYSTEMD_LIBS) \
                         -lgpiod
gpio_initial_CFLAGS = $(SYSTEMD_CFLAGS)