# Initialization
AC_PREREQ([2.69])
AC_INIT([mct-default-setting], [1.0])
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
PKG_CHECK_MODULES([PHOSPHOR_DBUS_INTERFACES], [phosphor-dbus-interfaces])
AX_CXX_COMPILE_STDCXX_17([noext])

# Checks for header files.
AC_CHECK_HEADER(systemd/sd-bus.h, ,[AC_MSG_ERROR([Could not find systemd/sd-bus.h...systemd developement package required])])

# Checks for typedefs, structures, and compiler characteristics.
AX_APPEND_COMPILE_FLAGS([-Wall -Werror], [CFLAGS])

# Checks for library functions.
LT_INIT # Removes 'unrecognized options: --with-libtool-sysroot'


# BMC FMC ABR stack is disabled by default; offer a way to enable it
AC_ARG_ENABLE([bmc-fmc-abr],
    [ --enable-bmc-fmc-abr   Enable/disable BMC FMC ABR],
    [case "${enableval}" in
      yes) bmc_fmc_abr=true ;;
      no) bmc_fmc_abr=false ;;
      *) AC_MSG_ERROR([bad value ${enableval} for --enable-bmc-fmc-abr]) ;;
      esac],[bmc-fmc-abr=false]
      )
AM_CONDITIONAL([FEATURE_BMC_FMC_ABR], [test x$bmc_fmc_abr = xtrue])

AS_IF([test x$bmc_fmc_abr = xtrue],
    AC_MSG_NOTICE([Enabling BMC FMC ABR])
    [cpp_flags="$cpp_flags -DFEATURE_BMC_FMC_ABR"]
    AC_SUBST([CPPFLAGS], [$cpp_flags]),
    AC_MSG_WARN([Disabling BMC FMC ABR feature])
)

# Create configured output
AC_CONFIG_FILES([Makefile])
AC_OUTPUT



