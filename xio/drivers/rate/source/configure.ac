AC_PREREQ([2.60])

AC_INIT([globus_xio_rate_driver],[1.8],[https://github.com/globus/globus-toolkit/issues])
AC_SUBST([MAJOR_VERSION], [${PACKAGE_VERSION%%.*}])
AC_SUBST([MINOR_VERSION], [${PACKAGE_VERSION##*.}])
AC_SUBST([AGE_VERSION], [1])
AC_SUBST([PACKAGE_DEPS], ["globus-common >= 0, globus-xio >= 0"])

AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([1.11 foreign parallel-tests tar-pax])
LT_INIT([dlopen win32-dll])

m4_include([dirt.sh])
AC_SUBST(DIRT_TIMESTAMP)
AC_SUBST(DIRT_BRANCH_ID)

PKG_CHECK_MODULES([PACKAGE_DEP], $PACKAGE_DEPS)

AC_CONFIG_FILES(
        globus-xio-rate-driver-uninstalled.pc
        globus-xio-rate-driver.pc
        Makefile
	version.h)
AC_OUTPUT
