# Initialization
AC_PREREQ([2.69])
AC_INIT([mct-dcmi-power], [1.0])
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

AC_ARG_ENABLE([dbus-mode],
    AC_HELP_STRING([--enable-dbus-mode], [Enable DBUS mode])
)
AS_IF([test "x$enable_dbus_mode" = "xyes"], \
    [AX_APPEND_COMPILE_FLAGS([-DDBUS_MODE], [CXXFLAGS])])
AM_CONDITIONAL([DBUS_MODE], [test "$enable_dbus_mode" = "xyes"])

AC_ARG_ENABLE([sysfs-mode],
    AC_HELP_STRING([--enable-sysfs-mode], [Enable SYSFS mode])
)
AS_IF([test "x$enable_sysfs_mode" = "xyes"], \
    [AX_APPEND_COMPILE_FLAGS([-DSYSFS_MODE], [CXXFLAGS])])
AM_CONDITIONAL([SYSFS_MODE], [test "$enable_sysfs_mode" = "xyes"])


# Create configured output
AC_CONFIG_FILES([Makefile])
AC_OUTPUT



