EXE     = bios-updater
CFILE  = *.cpp
DEPPKGS = gio-unix-2.0 glib-2.0 
CC      ?= $(CROSS_COMPILE)g++
CFLAGS += $(shell pkg-config --cflags $(DEPPKGS))
CFLAGS += -std=c++1z
LIBS += $(shell pkg-config --libs $(LIBPKGS))
LIBS += -fPIC -lrt -lm -lsystemd -lsdbusplus -lobmci2c -lgpiod -lphosphor_dbus -lobmcdbus -lobmcmisc 

all: $(EXE)
$(EXE): $(CFILE)
	$(CC) -g  $(CFLAGS) $(INCLUDES) -o $@ $^ -lstdc++ $(LIBS) 

clean:
	rm -f $(EXE) *.o *.d

distclean: clean
	rm -f *.cpp~ *.hpp~ *.sh~ Makefile~ config.mk~
