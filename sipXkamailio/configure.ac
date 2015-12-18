AC_PREREQ(2.57)
AC_INIT(sipXportLib, 15.06, sipx-dev@list.sipfoundry.org)
AC_CONFIG_AUX_DIR(config)
m4_include([config/general.m4])
m4_include([config/ax_boost_base.m4])
m4_include([config/oss_lib.m4])
AM_INIT_AUTOMAKE(foreign tar-ustar)
SFAC_AUTOMAKE_VERSION([1.6])
AM_CONFIG_HEADER([src/config.h])
AC_PROG_LIBTOOL
AC_LTDL_SHLIBEXT
AC_PATH_PROG([BASH], [bash])
AC_PROG_CXX
AX_COMPILER_VENDOR
AC_PROG_CC
CHECK_XARGS_REPLACE
CHECK_MSG_NOSIGNAL
CHECK_LIBRT
AX_BOOST_BASE([1.39],,
  SF_MISSING_DEP([no compatible boost library found]))
AC_SEARCH_LIBS([dlopen], [dl])
CHECK_PCRE
CHECK_SSL
CHECK_POCO
SFAC_INIT_FLAGS
SFAC_LIB_CORE
AC_CHECK_FUNCS(setenv)
AC_CONFIG_FILES([
  Makefile
  include/Makefile
  src/Makefile
])

AC_OUTPUT
