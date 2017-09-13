AC_DEFUN(AC_CHECK_TIMRU, [
	OLD_CPPFLAGS="$CPPFLAGS"
	have_timru="no"
	AC_MSG_CHECKING([for TIMRU in linux/isdn.h])
	AC_TRY_COMPILE([#include <linux/isdn.h>],int x = IIOCNETARU;,have_timru="yes",)
	AC_MSG_RESULT("${have_timru}")
	CPPFLAGS="$OLD_CPPFLAGS"
	if test "$have_timru" != "no" ; then
		AC_DEFINE(HAVE_TIMRU)
	fi
	AC_SUBST(HAVE_TIMRU)
])
