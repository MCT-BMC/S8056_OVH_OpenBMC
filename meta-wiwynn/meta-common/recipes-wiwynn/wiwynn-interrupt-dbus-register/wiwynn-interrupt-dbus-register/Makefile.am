AM_DEFAULT_SOURCE_EXT = .cpp

bin_PROGRAMS = interruptdbusregister

interruptdbusregister_SOURCES = main.cpp

interruptdbusregister_LDADD = \
	$(SDBUSPLUS_LIBS) \
	-lgpiod

interruptdbusregister_CXXFLAGS = \
	$(SDBUSPLUS_CFLAGS)


