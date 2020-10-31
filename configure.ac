AC_INIT(atque, 1.1.2, [http://sourceforge.net/tracker/?func=add&group_id=235817&atid=1097912] )

AC_PREREQ([2.59])
AC_CONFIG_AUX_DIR(config)
AM_INIT_AUTOMAKE([foreign])

AC_CONFIG_HEADER(config.h)

AC_LANG(C++)

AC_PROG_CC
AC_PROG_CPP
AC_PROG_CXX
AC_PROG_CXXCPP
AC_PROG_INSTALL
AC_PROG_LIBTOOL

case "$host" in
*-mingw32*)
	AC_CHECK_TOOL(WINDRES, windres, :)
	make_windows=true
	LDFLAGS="$LDFLAGS -Wl,-S"
	;;
*)
	make_windows=false
	;;
esac

AC_CHECK_HEADERS([sndfile.h], , AC_ERROR([Atque requires libsndfile]))
AC_CHECK_LIB(sndfile, sf_open, LIBS="-lsndfile $LIBS", AC_ERROR([Atque requires libsndfile]))

AX_BOOST_BASE([1.33.1])
AX_BOOST_LOCALE
LIBS="$LIBS $BOOST_LOCALE_LIB"
AM_OPTIONS_WXCONFIG  
reqwx=3.0.0
AM_PATH_WXCONFIG($reqwx)

CPPFLAGS="$CPPFLAGS $WX_CPPFLAGS"
CXXFLAGS="$CXXFLAGS $WX_CXXFLAGS_ONLY"
CFLAGS="$CFLAGS $WX_CFLAGS_ONLY"
LIBS="$LIBS $WX_LIBS `wx-config --optional-libs richtext `"

AM_CONDITIONAL(BUILD_ATQUEGUI, [test "$wxWin" = 1 ])

AM_CONDITIONAL(MAKE_WINDOWS, test x$make_windows = xtrue)

AC_CONFIG_FILES([Makefile ferro/Makefile])
AC_OUTPUT

echo "Configuration done. Now type \"make\"."

