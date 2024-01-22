LIB    = libobmcjtag.so
DEPPKGS = gio-unix-2.0 glib-2.0 libsystemd
CC ?= $(CROSS_COMPILE)g++
CFLAGS += $(shell pkg-config --cflags $(DEPPKGS))
CFLAGS += -std=c++1z
LIBS += $(shell pkg-config --libs $(DEPPKGS))
LIBS += -fPIC -lrt -lcrypt -lm

lib: $(LIB)

libobmcjtag.so: *.cpp
	$(CC) $(CFLAGS) $(LIBS) -shared  -o $@ $^

.PHONY: clean

clean:
	rm -f $(LIB) *.o *.d

distclean: clean
	rm -f *.cpp~ *.hpp~ *.sh~ Makefile~ config.mk~
