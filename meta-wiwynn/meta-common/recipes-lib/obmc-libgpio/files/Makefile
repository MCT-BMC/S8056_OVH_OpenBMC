LIB    = libobmcgpio.so
DEPPKGS = gio-unix-2.0 glib-2.0 libsystemd
CC ?= $(CROSS_COMPILE)gcc
CFLAGS += $(shell pkg-config --cflags $(DEPPKGS))
LIBS += $(shell pkg-config --libs $(DEPPKGS))
LIBS += -fPIC -lrt -lcrypt -lm

lib: $(LIB)

libobmcgpio.so: *.c
	$(CC) $(CFLAGS) $(LIBS) -shared  -o $@ $^

.PHONY: clean

clean:
	rm -f $(LIB) *.o *.d

distclean: clean
	rm -f *.c~ *.h~ *.sh~ Makefile~ config.mk~
