AC_INIT(imgdecode, 1.0, mechalas@sourceforge.net)

AC_CONFIG_HEADER(config.h)
AC_PROG_CXX()
AC_C_BIGENDIAN()

AC_CHECK_HEADERS([inttypes.h])
AC_CHECK_TYPES([u_int16_t, uint16_t])

if test "$ac_cv_type_u_int16_t" = "no" -a "$ac_cv_type_uint16_t" = "no"
then
	AC_MSG_ERROR([Can't continue without a defined uint16_t or u_int16_t type])
fi

AC_OUTPUT(Makefile)
makedepend *.cc 2>/dev/null
