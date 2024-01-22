LIB    = libobmcdbus.so
DEPPKGS = gio-unix-2.0 glib-2.0 libsystemd sdbusplus
CC ?= $(CROSS_COMPILE)g++
CFLAGS += $(shell pkg-config --cflags $(DEPPKGS))
CFLAGS += -std=c++1z
LIBPKGS = phosphor-dbus-interfaces
LIBS += $(shell pkg-config --libs $(LIBPKGS))
LIBS += -fPIC -lrt -lcrypt -lm

lib: $(LIB)

libobmcdbus.so: *.cpp
	$(CC) $(CFLAGS) -shared -o $@ $^ -lstdc++ $(LIBS)

.PHONY: clean

clean:
	rm -f $(LIB) *.o *.d

distclean: clean
	rm -f *.cpp~ *.hpp~ *.sh~ Makefile~ config.mk~
