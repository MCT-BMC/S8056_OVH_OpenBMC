# Initialization
AC_PREREQ([2.69])
AC_INIT([mct-cpld-update], [1.0])
AC_LANG([C++])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([subdir-objects -Wall -Werror foreign dist-xz])
AM_SILENT_RULES([yes])

# Checks for programs.
AC_PROG_CC
AM_PROG_AR
AC_PROG_INSTALL
AC_PROG_MAKE_SET

# Checks for libraries.
PKG_CHECK_MODULES([SYSTEMD], [libsystemd >= 221])
PKG_CHECK_MODULES([SDBUSPLUS], [sdbusplus],,\
                  AC_MSG_ERROR(["Requires sdbusplus package."]))
AX_CXX_COMPILE_STDCXX_17([noext])

# Checks for header files.
AC_CHECK_HEADER(systemd/sd-bus.h, ,[AC_MSG_ERROR([Could not find systemd/sd-bus.h...systemd developement package required])])

# Checks for typedefs, structures, and compiler characteristics.
AX_APPEND_COMPILE_FLAGS([-Wall -Werror], [CFLAGS])

# Checks for library functions.
LT_INIT # Removes 'unrecognized options: --with-libtool-sysroot'

AC_ARG_ENABLE([debug],
    AC_HELP_STRING([--enable-debug], [Enable debug mode])
)
AS_IF([test "x$enable_debug" = "xyes"], \
    [AX_APPEND_COMPILE_FLAGS([-DDEBUG], [CXXFLAGS])])
AM_CONDITIONAL([DEBUG], [test "$enable_debug" = "xyes"])

AC_ARG_ENABLE([host-reboot-mode],
    AC_HELP_STRING([--enable-host-reboot-mode], [Enable debug mode])
)
AS_IF([test "x$enable_host_reboot_mode" = "xyes"], \
    [AX_APPEND_COMPILE_FLAGS([-DHOST_REBOOT_MODE], [CXXFLAGS])])
AM_CONDITIONAL([HOST_REBOOT_MODE], [test "$enable_host_reboot_mode" = "xyes"])

AC_ARG_ENABLE([gpio-mode],
    AC_HELP_STRING([--enable-gpio-mode], [Enable debug mode])
)
AS_IF([test "x$enable_gpio_mode" = "xyes"], \
    [AX_APPEND_COMPILE_FLAGS([-DGPIO_MODE], [CXXFLAGS])])
AM_CONDITIONAL([GPIO_MODE], [test "$enable_gpio_mode" = "xyes"])

# Create configured output
AC_CONFIG_FILES([Makefile])
AC_OUTPUT



