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
         AC_CHECK_LIB(dl,
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
