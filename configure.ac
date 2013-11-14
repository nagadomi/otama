
#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT(otama, 0.6.0, nagadomi@nurs.or.jp)
OTAMA_MAJOR=0
OTAMA_MINOR=6
AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_SRCDIR([src])
AC_CONFIG_HEADER([src/config.h:src/config.h.in])
AC_CONFIG_MACRO_DIR([m4])
AC_CANONICAL_HOST

# clear CFLAGS
AC_SUBST(CFLAGS)
AC_SUBST(CXXFLAGS)
AC_SUBST(CPPFLAGS)
if test "" = "$CFLAGS"; then
    CFLAGS=" "
fi
if test "" = "$CXXFLAGS"; then
    CXXFLAGS=" "
fi
if test "" = "$CPPFLAGS"; then
    CPPFLAGS=" "
fi
if test "" = "$LDFLAGS"; then
    LDFLAGS=" "
fi

CFLAGS=" $CFLAGS -D__STDC_LIMIT_MACROS -D__STDC_FORMAT_MACROS -Wall "
CFLAGS=" $CFLAGS -D_FORTIFY_SOURCE=2 -fstack-protector --param=ssp-buffer-size=4 -Wformat -Werror=format-security "

# Checks for programs.
AC_PROG_CC
AC_PROG_CPP
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_MKDIR_P
#AC_OPENMP

AC_PATH_PROG([DEBUILD], [debuild], [notfound])
AC_PATH_PROG([TEST], [test], [notfound])
AC_PATH_PROG([ECHO], [echo], [notfound])
AC_PATH_PROG([CP], [cp], [notfound])
AC_PATH_PROG([MV], [mv], [notfound])
AC_PATH_PROG([TAR], [tar], [notfound])

AC_CHECK_PROG(GZIP,gzip,gzip,no)
if test "$GZIP" = no ; then
   AC_MSG_ERROR(*** gzip command not found. please install gzip command.)]
fi

# Checks for libraries.
LT_INIT([win32-dll])

# Checks for typedefs, structures, and compiler characteristics.

AC_CHECK_LIB(m, main)
AC_CHECK_LIB(stdc++, main)

otama_deb=0
otama_debsign=0
DPKG_FLAGS=""
AX_ARG_ENABLE(deb,
        AC_HELP_STRING([--enable-deb], [build debian package. ]), [no],
	[otama_deb=1],
        [])
AX_ARG_ENABLE(debsrc,
        AC_HELP_STRING([--enable-debsrc], [build debian source package. ]), [no],
	[otama_deb=1;DPKG_FLAGS=" $DPKG_FLAGS -S -sa "],
        [])
AX_ARG_ENABLE(sign,
        AC_HELP_STRING([--enable-sign], [build signed debian package. ]), [no],
	[otama_deb=1],
        [DPKG_FLAGS=" $DPKG_FLAGS -uc -us "])

AX_ARG_ENABLE(debug,
        AS_HELP_STRING([--enable-debug], [enable assert and -g ]), [no],
#	[CFLAGS="$CFLAGS -g -D_GLIBCXX_DEBUG "], # bug
	[CFLAGS="$CFLAGS -g "],
        [CFLAGS="$CFLAGS -ffast-math -O3 -g0 -funroll-loops -DNDEBUG "])

AX_ARG_ENABLE(native,
        AS_HELP_STRING([--disable-native],[disable -march=native]), [yes],
        [CFLAGS=" $CFLAGS -march=native "],
        [])
        
otama_kc=0        
AC_LANG_PUSH([C++])
AX_ARG_ENABLE(kyotocabinet,
        AS_HELP_STRING([--enable-kyotocabinet], [enable kyotocabinet.]), [no],
	[AC_CHECK_HEADERS([kcpolydb.h],
		[otama_kc=1
		AC_CHECK_LIB(kyotocabinet, main)
		],
		[AC_MSG_ERROR(*** libkyotocabinet not found. please install kyotocabinet.)])],
        [])

otama_leveldb=0
AC_LANG_PUSH([C++])
AX_ARG_ENABLE(leveldb,
        AS_HELP_STRING([--enable-leveldb], [enable leveldb.]), [yes],
	[AC_CHECK_HEADERS([leveldb/db.h],
		[otama_leveldb=1
		AC_CHECK_LIB(snappy, main)
		AC_CHECK_LIB(leveldb, main)
		],
		[AC_MSG_ERROR(*** libleveldb not found. please install libleveldb-dev and libsnappy-dev.)])],
        [])

# unordered_map
# TODO: -std=c++0x
otama_unordered_map_tr1=0
otama_unordered_map=0
AC_CHECK_HEADER(tr1/unordered_map,[otama_unordered_map_tr1=1],[])
AC_CHECK_HEADER(unordered_map,[otama_unordered_map=1],[])

AC_LANG_POP([C++])

otama_lzo=0
AX_ARG_ENABLE(lzo,
        AS_HELP_STRING([--enable-lzo], [enable lzo compress.]), [no],
	[AC_CHECK_HEADERS([lzo/lzo1x.h],
		[otama_lzo=1
        AC_CHECK_LIB(lzo2, main)
		],
		[AC_MSG_ERROR(*** liblzo2 not found. please install liblzo2-dev.)])],
        [])

AX_ARG_ENABLE(popcnt,
        AS_HELP_STRING([--enable-popcnt], [enable popcnt]), [no],
        [CFLAGS="$CFLAGS -mpopcnt -D__POPCNT__=1 "],
        [])

AX_ARG_ENABLE(openmp,
        AS_HELP_STRING([--disable-openmp], [ disable OpenMP ]), [yes],
        [AC_CHECK_HEADERS(
		[omp.h],
		[CFLAGS="$CFLAGS -fopenmp -D_GLIBCXX_PARALLEL "
		AC_CHECK_LIB(gomp, omp_get_num_procs)
		AC_CHECK_LIB(pthread, pthread_create)],
		[AC_MSG_ERROR(*** omp.h not found. please either install gcc version 4.2 or any later version or use the --disable-openmp option..)])
	],
    []
)

otama_pgsql=0
otama_sqlite3=0
otama_mysql=0
AX_ARG_ENABLE(pgsql,
        AS_HELP_STRING([--enable-pgsql], [enable pgsql driver.]), [no],
        [AC_CHECK_HEADERS([postgresql/libpq-fe.h],
		[otama_pgsql=1
        AC_CHECK_LIB(pq, PQsetdbLogin)
		],
		[AC_MSG_ERROR(*** libpq not found. please install libpq-dev.)])],
        [])
AX_ARG_ENABLE(sqlite3,
        AS_HELP_STRING([--enable-sqlite3], [enable sqlite3 driver.]), [yes],
        [AC_CHECK_HEADERS([sqlite3.h],
		[otama_sqlite3=1
        AC_CHECK_LIB(sqlite3, sqlite3_open)
		],
		[AC_MSG_ERROR(*** libsqlite3 not found. please install libsqlite3-dev.)])],
        [])
AX_ARG_ENABLE(mysql,
        AS_HELP_STRING([--enable-mysql], [enable mysql driver.]), [no],
        [AC_CHECK_HEADERS([mysql/mysql.h],
		[otama_mysql=1
        AC_CHECK_LIB(mysqlclient, mysql_real_connect)
		],
		[AC_MSG_ERROR(*** libmysqlclient not found. please install libmysqlclient-dev.)])],
        [])
        
AC_CHECK_HEADERS([nv_core.h],
	[AC_CHECK_LIB(nv, nv_matrix_alloc)],
	[AC_MSG_ERROR(*** nv not found. please install libnv.)])

AC_CHECK_HEADERS(
	[yaml.h],
	[AC_CHECK_LIB(yaml, main)],
	[AC_MSG_ERROR(*** yaml.h not found. please install libyaml-dev.)])

AC_ARG_WITH(pkgconfigdir,
	AS_HELP_STRING([--with-pkgconfigdir],
	[use the specified pkgconfig dir (default is ${libdir}/pkgconfig)]),
	[pkgconfigdir=${withval}],
	[pkgconfigdir='${libdir}/pkgconfig'])
AC_MSG_NOTICE([pkgconfig directory is ${pkgconfigdir}])	

AC_C_CONST
AC_TYPE_INT64_T
AC_HEADER_STDBOOL
AC_TYPE_INT64_T
AC_TYPE_SIZE_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

AC_FUNC_MMAP
AC_FUNC_STRERROR_R
AC_FUNC_STRTOD

AC_HEADER_STDC
AC_CHECK_HEADERS([arpa/inet.h fcntl.h float.h inttypes.h limits.h stdlib.h string.h unistd.h])
AC_CHECK_FUNCS([ftruncate memset munmap realpath strcasecmp strerror strtol])

# host
AC_CANONICAL_HOST
case $host in
  *-*-mingw* | *-*-cygwin*)
    CFLAGS="$CFLAGS -mstackrealign -mthreads "
    LDFLAGS="$LDFLAGS -Wl,--stack,10485760 "
    ;;
  *-linux*)
    LDFLAGS="$LDFLAGS -Wl,-z,relro "
    CFLAGS="$CFLAGS -rdynamic "
    ;;
  *)
    CFLAGS="$CFLAGS -rdynamic "
    ;;
esac

CXXFLAGS=$CFLAGS

AM_CONDITIONAL([BUILDING_DEB], [$TEST $otama_deb = 1])
AC_SUBST(DPKG_FLAGS)
AC_SUBST(otama_kc)
AC_SUBST(otama_leveldb)
AC_SUBST(otama_lzo)
AC_SUBST(otama_sqlite3)
AC_SUBST(otama_pgsql)
AC_SUBST(otama_mysql)
AC_SUBST(otama_unordered_map_tr1)
AC_SUBST(otama_unordered_map)
AC_SUBST(OTAMA_MAJOR)
AC_SUBST(OTAMA_MINOR)
AC_SUBST(pkgconfigdir)

AC_CONFIG_FILES([
src/lib/otama_config_auto.h
src/nvcolorex/Makefile
src/nvbovw/Makefile
src/nvvlad/Makefile
src/nvlmcaex/Makefile
src/tests/Makefile
src/otama.pc
src/Makefile
vcproj/Makefile
image/Makefile
examples/Makefile
examples/example-webapp/Makefile
examples/rpc/Makefile
examples/ukbench/Makefile
Makefile
])

AC_OUTPUT
