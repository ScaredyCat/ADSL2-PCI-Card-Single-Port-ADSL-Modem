dnl
dnl Check for postgres
dnl

AC_DEFUN(AC_CHECK_POSTGRES, [
	POSTGRESDIR=""
	pqdir="no"
	tst_postgresdir="$CONFIG_ISDNLOG_POSTGRESDIR"

	AC_ARG_WITH(postgres,
		[  --with-postgres=DIR     Set postgres directory []],
		tst_postgresdir="${withval}")

	if test "$tst_postgresdir" != "" || test "$CONFIG_ISDNLOG_POSTGRES" = "y" ; then
		AC_MSG_CHECKING([for postgres in ${tst_postgresdir}])
		if test "${tst_postgresdir}" != "" ; then
			AC_EGREP_HEADER(PGconn,${tst_postgresdir}/include/libpq-fe.h,
			pqdir=${tst_postgresdir})
		fi
		if test "$pqdir" = "no" ; then
			AC_MSG_RESULT("$pqdir")
			AC_MSG_CHECKING([for postgres in /lib/postgres95])
			AC_EGREP_HEADER(PGconn,/lib/postgre95/include/libpq-fe.h,
			pqdir=/lib/postgres95)
		fi
		if test "$pqdir" = "no" ; then
			AC_MSG_RESULT("$pqdir")
			AC_MSG_CHECKING([for postgres in /usr/lib/postgres95])
			AC_EGREP_HEADER(PGconn,/usr/lib/postgre95/include/libpq-fe.h,
			pqdir=/usr/lib/postgres95)
		fi
		if test "$pqdir" = "no" ; then
			AC_MSG_RESULT("$pqdir")
			AC_MSG_CHECKING([for postgres in /usr/local/postgres95])
			AC_EGREP_HEADER(PGconn,/usr/lib/local/postgre95/include/libpq-fe.h,
			pqdir=/usr/local/postgres95)
		fi
		if test "$pqdir" = "no" ; then
			AC_MSG_RESULT("$pqdir")
			AC_MSG_CHECKING([for postgres in /usr/local/lib/postgres95])
			AC_EGREP_HEADER(PGconn,/usr/lib/local/lib/postgre95/include/libpq-fe.h,
			pqdir=/usr/local/lib/postgres95)
		fi
	fi
	if test "$pqdir" != "no" ; then
		AC_MSG_RESULT("yes")
		POSTGRES=1
		AC_DEFINE_UNQUOTED(POSTGRES,1)
	else
		AC_MSG_RESULT("no POSTGRES DISABLED")
		pqdir=""
	fi
	POSTGRESDIR="$pqdir"
	AC_DEFINE_UNQUOTED(POSTGRESDIR,"$pqdir")
	AC_SUBST(POSTGRES)
	AC_SUBST(POSTGRESDIR)
])
dnl
dnl Check for mysql
dnl

AC_DEFUN(AC_CHECK_MYSQLDB, [
	MYSQLDIR=""
	mydir="no"
	tst_mysqldir="$CONFIG_ISDNLOG_MYSQLDIR"

	AC_ARG_WITH(mysql,
		[  --with-mysql=DIR     Set mysql directory []],
		tst_mysqldir="${withval}")

	if test "$tst_mysqldir" != "" || test "$CONFIG_ISDNLOG_MYSQLDB" = "y" ; then
		AC_MSG_CHECKING([for mysql in ${tst_mysqldir}])
		if test "${tst_mysqldir}" != "" ; then
			AC_EGREP_HEADER(MYSQL,${tst_mysqldir}/include/mysql.h,
			mydir=${tst_mysqldir})
		fi
		if test "$mydir" = "no" ; then
			AC_MSG_RESULT("$mydir")
			AC_MSG_CHECKING([for mysql in /usr])
			AC_EGREP_HEADER(MYSQL,/usr/include/mysql/mysql.h,
			mydir=/usr)
		fi
		if test "$mydir" = "no" ; then
			AC_MSG_RESULT("$mydir")
			AC_MSG_CHECKING([for mysql in /lib/mysql])
			AC_EGREP_HEADER(MYSQL,/lib/mysql/include/mysql.h,
			mydir=/lib/mysql)
		fi
		if test "$mydir" = "no" ; then
			AC_MSG_RESULT("$mydir")
			AC_MSG_CHECKING([for mysql in /usr/lib/mysql])
			AC_EGREP_HEADER(MYSQL,/usr/lib/mysql/include/mysql.h,
			mydir=/usr/lib/mysql)
		fi
		if test "$mydir" = "no" ; then
			AC_MSG_RESULT("$mydir")
			AC_MSG_CHECKING([for mysql in /usr/local/mysql])
			AC_EGREP_HEADER(MYSQL,/usr/local/mysql/include/mysql.h,
			mydir=/usr/local/mysql)
		fi
		if test "$mydir" = "no" ; then
			AC_MSG_RESULT("$mydir")
			AC_MSG_CHECKING([for mysql in /usr/local/lib/mysql])
			AC_EGREP_HEADER(MYSQL,/usr/local/lib/mysql/include/mysql.h,
			mydir=/usr/local/lib/mysql)
		fi
	fi
	if test "$mydir" != "no" ; then
		AC_MSG_RESULT("yes")
		MYSQLDB=1
		AC_DEFINE_UNQUOTED(MYSQLDB,1)
	else
		AC_MSG_RESULT("no MYSQL DISABLED")
		mydir=""
	fi
	MYSQLDIR="$mydir"
	AC_DEFINE_UNQUOTED(MYSQLDIR,"$mydir")
	AC_SUBST(MYSQLDB)
	AC_SUBST(MYSQLDIR)
])

dnl
dnl Check for Oracle
dnl

AC_DEFUN(AC_CHECK_ORACLE, [
	oradir="no"
	if test "$ORACLE_HOME" != "" && test "$CONFIG_ISDNLOG_ORACLE" = "y" ; then
		AC_MSG_CHECKING([for Oracle in ${ORACLE_HOME}])
		if test -x ${ORACLE_HOME}/bin/proc ; then
			oradir="yes"
		fi
	fi
	if test "$oradir" != "no" ; then
		AC_MSG_RESULT("yes")
		ORACLE=1
		AC_DEFINE_UNQUOTED(ORACLE,1)
	else
		AC_MSG_RESULT("no ORACLE DISABLED")
	fi
	AC_SUBST(ORACLE)
])

