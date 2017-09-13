dnl aclocal.m4 generated automatically by aclocal 1.3

dnl Copyright (C) 1994, 1995, 1996, 1997, 1998 Free Software Foundation, Inc.
dnl This Makefile.in is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY, to the extent permitted by law; without
dnl even the implied warranty of MERCHANTABILITY or FITNESS FOR A
dnl PARTICULAR PURPOSE.

dnl #------------------------------------------------------------------------#
dnl # Test if all needed libraries for vboxgetty are installed. We will end  #
dnl # the configure script if one is missing!                                #
dnl #------------------------------------------------------------------------#

AC_DEFUN(GND_PACKAGE_TCL,
   [
      HAVE_TCL_LIBS="n"
      HAVE_TCL_INCL="n"
      HAVE_TCL_PACK="n"
      LINK_TCL_LIBS=""
      LINK_TCL_INCL=""

      gnd_use_tcl_lib=""
      gnd_use_tcl_dir=""

      AC_ARG_WITH(tcllib,
         [  --with-tcllib=LIB       use tcl library LIB to link [tcl]],
         gnd_use_tcl_lib="${withval}",
         gnd_use_tcl_lib="`eval echo ${VBOX_TCL:-""}`"
      )

      AC_ARG_WITH(tcldir,
         [  --with-tcldir=DIR       tcl base directory []],
         gnd_use_tcl_dir="${withval}"
      )

      gnd_tcl_inc_dir=""
      gnd_tcl_lib_dir=""

      if (test "${gnd_use_tcl_dir}" != "")
      then
         gnd_tcl_inc_dir="-I${gnd_use_tcl_dir}/include"
         gnd_tcl_lib_dir="-L${gnd_use_tcl_dir}/lib"
      fi

      if (test "${gnd_use_tcl_lib}" = "")
      then
         gnd_1st_tcl_lib_test="tcl8.4"
         gnd_2nd_tcl_lib_test="tcl8.3"
         gnd_3rd_tcl_lib_test="tcl8.0"
      else
         gnd_1st_tcl_lib_test="${gnd_use_tcl_lib}"
         gnd_2nd_tcl_lib_test="tcl8.4"
         gnd_3rd_tcl_lib_test="tcl8.3"
      fi

      AC_CHECK_LIB(m,
         cos,
         [AC_CHECK_LIB(dl,
            dlerror,
            [AC_CHECK_LIB(${gnd_1st_tcl_lib_test},
               Tcl_CreateInterp,
               LINK_TCL_LIBS="${gnd_tcl_lib_dir} -l${gnd_1st_tcl_lib_test} -lm -ldl",
               [AC_CHECK_LIB(${gnd_2nd_tcl_lib_test},
                  Tcl_CreateInterp,
                  LINK_TCL_LIBS="${gnd_tcl_lib_dir} -l${gnd_2nd_tcl_lib_test} -lm -ldl",
                  [AC_CHECK_LIB(${gnd_3rd_tcl_lib_test},
                     Tcl_CreateInterp,
                     LINK_TCL_LIBS="${gnd_tcl_lib_dir} -l${gnd_3rd_tcl_lib_test} -lm -ldl",
                     ,
                     ${gnd_tcl_lib_dir} -lm -ldl
                  )],
                  ${gnd_tcl_lib_dir} -lm -ldl
               )],
               ${gnd_tcl_lib_dir} -lm -ldl
            )],
         )],
      )

      if (test "${LINK_TCL_LIBS}" != "")
      then
         HAVE_TCL_LIBS="y"
      fi

         dnl #-------------------------------------------#
         dnl # Check if the compiler find the tcl header #
         dnl #-------------------------------------------#

      AC_CHECK_HEADER(tcl.h, HAVE_TCL_INCL="y")

      if (test "${HAVE_TCL_INCL}" = "n")
      then
         if (test "${gnd_use_tcl_dir}" != "")
         then
            AC_MSG_CHECKING("for tcl header in ${gnd_use_tcl_dir}/include")

            if (test -e "${gnd_use_tcl_dir}/include/tcl.h")
            then
               AC_MSG_RESULT("yes")

               HAVE_TCL_INCL="y"
               LINK_TCL_INCL="${gnd_tcl_inc_dir}"
            fi
         else
            AC_MSG_CHECKING("for tcl header in /usr/include/tcl8.3/tcl.h")
            if (test -e "/usr/include/tcl8.3/tcl.h")
            then
               AC_MSG_RESULT("yes")
               HAVE_TCL_INCL="y"
               LINK_TCL_INCL="-I/usr/include/tcl8.3"
            else
               AC_MSG_RESULT("no")
            fi
         fi
      fi

      if (test "${HAVE_TCL_LIBS}" = "y")
      then
         if (test "${HAVE_TCL_INCL}" = "y")
         then
            HAVE_TCL_PACK="y"
         fi
      fi

      if (test "${HAVE_TCL_PACK}" = "n")
      then
         AC_MSG_WARN("**")
         AC_MSG_WARN("** Unable to find a installed tcl package!")
         AC_MSG_WARN("**")

         AC_MSG_ERROR("stop")
      fi

      AC_SUBST(HAVE_TCL_PACK)
      AC_SUBST(LINK_TCL_LIBS)
      AC_SUBST(LINK_TCL_INCL)
   ]
)

dnl #------------------------------------------------------------------------#
dnl # Checks if the ncurses package is installed:                            #
dnl #------------------------------------------------------------------------#

AC_DEFUN(GND_PACKAGE_NCURSES,
   [
      LINK_NCURSES_LIBS=""
      LINK_NCURSES_INCL=""
      HAVE_NCURSES_LIBS="n"
      HAVE_NCURSES_INCL="n"
      HAVE_NCURSES_PACK="n"

         dnl #------------------------------------------------#
         dnl # Check if the compiler find the ncurses library #
         dnl #------------------------------------------------#

      AC_CHECK_LIB(ncurses, initscr, HAVE_NCURSES_LIBS="y")

      if (test "${HAVE_NCURSES_LIBS}" = "y")
      then
         AC_CHECK_LIB(panel,
            update_panels,
                  HAVE_NCURSES_LIBS="y",
                  HAVE_NCURSES_LIBS="n",
                  -lncurses
               )
      fi

      if (test "${HAVE_NCURSES_LIBS}" = "y")
      then
         LINK_NCURSES_LIBS="-lncurses -lpanel"

         AC_CHECK_LIB(ncurses,
            resizeterm,
            AC_DEFINE(HAVE_RESIZETERM)
)
      fi

         dnl #------------------------------------------------#
         dnl # Check if the compiler find the ncurses headers #
         dnl #------------------------------------------------#

      AC_CHECK_HEADER(ncurses.h, HAVE_NCURSES_INCL="y")

      if (test "${HAVE_NCURSES_INCL}" = "y")
      then
         AC_CHECK_HEADER(panel.h,
            HAVE_NCURSES_INCL="y",
            HAVE_NCURSES_INCL="n"
      )
      fi

         dnl #--------------------------------------------------#
         dnl # If headers not found, check in some other places #
         dnl #--------------------------------------------------#

      if (test "${HAVE_NCURSES_INCL}" = "n")
      then
         AC_MSG_CHECKING(for ncurses headers in some other places)

         for I in /usr/local/include /usr/local/include/ncurses /usr/include /usr/include/ncurses
         do
            if (test -e "${I}/ncurses.h")
         then
               if (test -e "${I}/panel.h")
               then
                  AC_MSG_RESULT("${I}")

                  LINK_NCURSES_INCL="-I${I}"
                  HAVE_NCURSES_INCL="y"

                  break;
            fi
            fi
         done

         if (test "${HAVE_NCURSES_INCL}" = "n")
               then
            AC_MSG_RESULT("not here");
         fi
               fi

      if (test "${HAVE_NCURSES_LIBS}" = "y")
               then
         if (test "${HAVE_NCURSES_INCL}" = "y")
                  then
            HAVE_NCURSES_PACK="y"
         fi
      fi

      if (test "${HAVE_NCURSES_PACK}" = "n")
      then
         AC_MSG_WARN("**")
         AC_MSG_WARN("** Unable to find a installed ncurses package!")
         AC_MSG_WARN("**")

         AC_MSG_ERROR("stop")
      fi

      AC_SUBST(HAVE_NCURSES_PACK)
      AC_SUBST(LINK_NCURSES_LIBS)
      AC_SUBST(LINK_NCURSES_INCL)
   ]
)

# Like AC_CONFIG_HEADER, but automatically create stamp file.

AC_DEFUN(AM_CONFIG_HEADER,
[AC_PREREQ([2.12])
AC_CONFIG_HEADER([$1])
dnl When config.status generates a header, we must update the stamp-h file.
dnl This file resides in the same directory as the config header
dnl that is generated.  We must strip everything past the first ":",
dnl and everything past the last "/".
AC_OUTPUT_COMMANDS(changequote(<<,>>)dnl
ifelse(patsubst(<<$1>>, <<[^ ]>>, <<>>), <<>>,
<<test -z "<<$>>CONFIG_HEADERS" || echo timestamp > patsubst(<<$1>>, <<^\([^:]*/\)?.*>>, <<\1>>)stamp-h<<>>dnl>>,
<<am_indx=1
for am_file in <<$1>>; do
  case " <<$>>CONFIG_HEADERS " in
  *" <<$>>am_file "*<<)>>
    echo timestamp > `echo <<$>>am_file | sed -e 's%:.*%%' -e 's%[^/]*$%%'`stamp-h$am_indx
    ;;
  esac
  am_indx=`expr "<<$>>am_indx" + 1`
done<<>>dnl>>)
changequote([,]))])

# Do all the work for Automake.  This macro actually does too much --
# some checks are only needed if your package does certain things.
# But this isn't really a big deal.

# serial 1

dnl Usage:
dnl AM_INIT_AUTOMAKE(package,version, [no-define])

AC_DEFUN(AM_INIT_AUTOMAKE,
[AC_REQUIRE([AM_PROG_INSTALL])
PACKAGE=[$1]
AC_SUBST(PACKAGE)
VERSION=[$2]
AC_SUBST(VERSION)
dnl test to see if srcdir already configured
if test "`cd $srcdir && pwd`" != "`pwd`" && test -f $srcdir/config.status; then
  AC_MSG_ERROR([source directory already configured; run "make distclean" there first])
fi
ifelse([$3],,
AC_DEFINE_UNQUOTED(PACKAGE, "$PACKAGE")
AC_DEFINE_UNQUOTED(VERSION, "$VERSION"))
AC_REQUIRE([AM_SANITY_CHECK])
AC_REQUIRE([AC_ARG_PROGRAM])
dnl FIXME This is truly gross.
missing_dir=`cd $ac_aux_dir && pwd`
AM_MISSING_PROG(ACLOCAL, aclocal, $missing_dir)
AM_MISSING_PROG(AUTOCONF, autoconf, $missing_dir)
AM_MISSING_PROG(AUTOMAKE, automake, $missing_dir)
AM_MISSING_PROG(AUTOHEADER, autoheader, $missing_dir)
AM_MISSING_PROG(MAKEINFO, makeinfo, $missing_dir)
AC_REQUIRE([AC_PROG_MAKE_SET])])


# serial 1

AC_DEFUN(AM_PROG_INSTALL,
[AC_REQUIRE([AC_PROG_INSTALL])
test -z "$INSTALL_SCRIPT" && INSTALL_SCRIPT='${INSTALL_PROGRAM}'
AC_SUBST(INSTALL_SCRIPT)dnl
])

#
# Check to make sure that the build environment is sane.
#

AC_DEFUN(AM_SANITY_CHECK,
[AC_MSG_CHECKING([whether build environment is sane])
# Just in case
sleep 1
echo timestamp > conftestfile
# Do `set' in a subshell so we don't clobber the current shell's
# arguments.  Must try -L first in case configure is actually a
# symlink; some systems play weird games with the mod time of symlinks
# (eg FreeBSD returns the mod time of the symlink's containing
# directory).
if (
   set X `ls -Lt $srcdir/configure conftestfile 2> /dev/null`
   if test "[$]*" = "X"; then
      # -L didn't work.
      set X `ls -t $srcdir/configure conftestfile`
   fi
   if test "[$]*" != "X $srcdir/configure conftestfile" \
      && test "[$]*" != "X conftestfile $srcdir/configure"; then

      # If neither matched, then we have a broken ls.  This can happen
      # if, for instance, CONFIG_SHELL is bash and it inherits a
      # broken ls alias from the environment.  This has actually
      # happened.  Such a system could not be considered "sane".
      AC_MSG_ERROR([ls -t appears to fail.  Make sure there is not a broken
alias in your environment])
   fi

   test "[$]2" = conftestfile
   )
then
   # Ok.
   :
else
   AC_MSG_ERROR([newly created file is older than distributed files!
Check your system clock])
fi
rm -f conftest*
AC_MSG_RESULT(yes)])

dnl AM_MISSING_PROG(NAME, PROGRAM, DIRECTORY)
dnl The program must properly implement --version.
AC_DEFUN(AM_MISSING_PROG,
[AC_MSG_CHECKING(for working $2)
# Run test in a subshell; some versions of sh will print an error if
# an executable is not found, even if stderr is redirected.
# Redirect stdin to placate older versions of autoconf.  Sigh.
if ($2 --version) < /dev/null > /dev/null 2>&1; then
   $1=$2
   AC_MSG_RESULT(found)
else
   $1="$3/missing $2"
   AC_MSG_RESULT(missing)
fi
AC_SUBST($1)])

