# Initialization
AC_PREREQ([2.69])
AC_INIT([interrupt-dbus-register], [1.0], [http://10.248.43.110:8000/kid95123/SV300G3-E/issues])
AC_LANG([C++])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([subdir-objects -Wall -Wno-portability -Werror foreign dist-xz])
AM_SILENT_RULES([yes])

# Make sure the default CFLAGS of `-O2 -g` don't override CODE_COVERAGE_CFLAGS
# It is important that this comes before AC_PROG_C{C,XX}, as we are attempting
# to stop them from populating default CFLAGS and CXXFLAGS.
AS_IF([test "x$enable_tests" = "xno"], [enable_code_coverage=no])
AS_IF([test "x$enable_code_coverage" != "xno"], [
    AS_IF([test "x${CXXFLAGS+set}" != "xset"], [
        AC_SUBST(CXXFLAGS, [""])
    ])
    AS_IF([test "x${CFLAGS+set}" != "xset"], [
        AC_SUBST(CFLAGS, [""])
    ])
])

# Checks for programs.
AC_PROG_CXX
AM_PROG_AR
AC_PROG_INSTALL
AC_PROG_MAKE_SET

# Checks for typedefs, structures, and compiler characteristics.
AX_CXX_COMPILE_STDCXX_17([noext])
AX_APPEND_COMPILE_FLAGS([-fpic -Wall -Werror], [CXXFLAGS])

# Checks for libraries.
PKG_CHECK_MODULES(
    [SYSTEMD],
    [libsystemd >= 221],
    [],
    [AC_MSG_ERROR(["systemd required and not found"])]
)
PKG_CHECK_MODULES(
    [SDBUSPLUS],
    [sdbusplus],
    [],
    [AC_MSG_ERROR(["sdbusplus required and not found"])]
)

AC_CHECK_HEADER(
    nlohmann/json.hpp,
    [],
    [AC_MSG_ERROR([Could not find nlohmann/json.hpp])]
)

# check for boost headers
AC_CHECK_HEADER(
    boost/asio/io_context.hpp,
    [],
    [AC_MSG_ERROR([Could not find boost/asio/io_context.hpp])]
)

AX_APPEND_COMPILE_FLAGS(["-DBOOST_ASIO_DISABLE_THREADS"], [CPPFLAGS])
AX_APPEND_COMPILE_FLAGS(["-DBOOST_ERROR_CODE_HEADER_ONLY"], [CPPFLAGS])
AX_APPEND_COMPILE_FLAGS(["-DBOOST_SYSTEM_NO_DEPRECATED"], [CPPFLAGS])
AX_APPEND_COMPILE_FLAGS(["-DBOOST_ASIO_NO_DEPRECATED"], [CPPFLAGS])

AC_ARG_VAR(GPIO_INTERRUPT_DBUS_PATH,
           [Absolutely path of PlatformInventory json file.])
AS_IF([test "x$GPIO_INTERRUPT_DBUS_PATH" == "x"], [GPIO_INTERRUPT_DBUS_PATH=""])
AC_DEFINE_UNQUOTED([GPIO_INTERRUPT_DBUS_PATH], ["$GPIO_INTERRUPT_DBUS_PATH"],
                   [Absolutely path of InterruptDbusRegister json file.])

# Checks for library functions.
LT_INIT # Required for systemd linking

# Create configured output
AC_CONFIG_FILES([Makefile])
AC_OUTPUT


