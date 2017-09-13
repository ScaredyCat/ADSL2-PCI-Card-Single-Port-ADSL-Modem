
AC_DEFUN(AC_FIND_FILE,
[
   $3=NO
   for i in $2;
   do
      for j in $1;
      do
         if test -r "$i/$j"
         then
            $3=$i
            break 2
         fi
      done
   done
])

AC_DEFUN(AC_PATH_PKGLOGDIR,
[
   packagelogdir="`eval echo ${packagelogdir:-/var/log}`"

   AC_ARG_WITH(packagelogdir,
      [  --with-logdir=DIR       where logs should be stored [/var/log]],
      [ packagelogdir=${withval} ])
                
   AC_SUBST(packagelogdir)
])

AC_DEFUN(AC_PATH_PKGLOCKDIR,
[
   packagelockdir="`eval echo ${packagelockdir:-/var/lock}`"

   AC_ARG_WITH(packagelockdir,
      [  --with-lockdir=DIR      where locks should be stored [/var/lock]],
      [ packagelockdir=${withval} ])
                
   AC_SUBST(packagelockdir)
])                

AC_DEFUN(AC_PATH_PKGPIDDIR,
[
   packagepiddir="`eval echo ${packagepiddir:-/var/run}`"

   AC_ARG_WITH(packagepiddir,
      [  --with-piddir=DIR       where PID's should be stored [/var/run]],
      [ packagepiddir=${withval} ])
                
   AC_SUBST(packagepiddir)
])
                         
AC_DEFUN(AC_PATH_TCL,
[
   ac_tcl_inc=""
   ac_tcl_lib=""

   AC_ARG_WITH(tcl-include,
      [  --with-tcl-include=DIR  where the tcl include is installed. ],
      [ ac_tcl_inc="$withval" ])
                   
   AC_ARG_WITH(tcl-library,
      [  --with-tcl-library=DIR  where the tcl library is installed. ],
      [ ac_tcl_lib="$withval" ])

   tcl_include=""
   tcl_library=""
                   
   AC_CHECK_LIB(m, cos,
      [
         AC_CHECK_LIB(dl, dlerror,
            [
               AC_CHECKING([whether tcl is installed in a standard location...])

               AC_CHECK_LIB(tcl, Tcl_CreateInterp,
                  [ tcl_library="-ltcl -lm -ldl" ],
                  [
                     AC_CHECKING([whether tcl is installed in a special locations...])

                     searchstring="$ac_tcl_lib /lib /usr/lib /usr/local/lib /opt/lib /opt/tcl/lib"

                     AC_FIND_FILE(libtcl.so, $searchstring, searchresult)

                     if (test ! "$searchresult" = "NO")
                     then
                        AC_CHECK_LIB(tcl, Tcl_CreateInterp,
                           [ tcl_library="-L$searchresult -ltcl -lm -ldl" ],
                           ,
                           -L$searchresult -lm -ldl)
                     fi
                  ],
                  -lm -ldl)
            ])
      ])

   if (test ! "$tcl_library" = "")
   then
      AC_CHECK_HEADERS(tcl.h,
         ,
         [
            AC_MSG_CHECKING([for tcl.h in a special location])

            searchstring="$ac_tcl_inc /usr/include /usr/local/include /opt/include /opt/tcl/include"

            AC_FIND_FILE(tcl.h, $searchstring, searchresult)

            if (test ! "$searchresult" = "NO")
            then
               AC_MSG_RESULT([$searchresult])

               tcl_include="-I$searchresult"
            else
               AC_MSG_RESULT([no])

               AC_MSG_WARN([***********************************************************])
               AC_MSG_WARN([* The tcl header file can not be located!                 *])
               AC_MSG_WARN([***********************************************************])
            fi
         ])
   else
      AC_MSG_WARN([***********************************************************])
      AC_MSG_WARN([* The tcl library can not be located!                     *])
      AC_MSG_WARN([*                                                         *])
      AC_MSG_WARN([* If tcl is installed but no tcl.so exists (but something *])
      AC_MSG_WARN([* like tcl8.0.so or tcl8.1.so), create a link 'tcl.so'    *])
      AC_MSG_WARN([* that points to your installed tcl library and start     *])
      AC_MSG_WARN([* ./configure again!                                      *])
      AC_MSG_WARN([***********************************************************])
   fi

   AC_SUBST(tcl_library)
   AC_SUBST(tcl_include)
])
