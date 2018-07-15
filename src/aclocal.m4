AC_DEFUN(PR_PATH_TCL_TK_H, [
#
# lets find the tcl and tk headers
# the alternative search directory is involked by --with-tclinclude=dir or with-tkinclude=dir
#
no_tcl=true
AC_MSG_CHECKING(for Tcl private header)
AC_ARG_WITH(tclinclude, [  --with-tclinclude       directory where tcl private headers are], with_tclinclude=${withval}) 
AC_CACHE_VAL(ac_cv_c_tclh,[
# first check to see if --with-tclinclude was specified
if test x"${with_tclinclude}" != x ; then
  if test -f ${with_tclinclude}/tcl.h ; then
     ac_cv_c_tclh="${with_tclinclude}"
  else
     AC_MSG_ERROR([${with_tclinclude} directory doesn't contain private header])
  fi
fi
#
# check some common install locations
#
if test x"${ac_cv_c_tclh}" = x ; then
   for i in \
		`ls -dr /usr/local/src/tcl[[0-9]].* 2>/dev/null` \
                `ls -dr /usr/local/lib/tcl[[0-9]].* 2>/dev/null` \
                `ls -dr /usr/local/tcl[[0-9]].*/include 2>/dev/null` \
                `ls -dr /usr/pkg/include/tcl[[0-9]].* 2>/dev/null` \
                `ls -dr /usr/include/tcl[[0-9]].* 2>/dev/null` \
		/usr/local/include \
                /usr/local/src/tcl \
                /usr/local/lib/tcl \
                /sw/include \
                ${prefix}/include
   do
	if test -f $i/tcl.h
	then
           ac_cv_c_tclh=`echo $i`
	   break
	fi
   done
fi
#
# now check if one is installed
#
if test x"${ac_cv_c_tclh}" = x ; then
  AC_TRY_CPP([#include <tcl.h>], eval "ac_cv_c_tclh=installed",
    eval "ac_cv_c_tclh=")
#   AC_HEADER_CHECK(tcl.h, ac_cv_c_tclh=installed, ac_cv_c_tclh="")
fi
])
#end of cache

if test x"${ac_cv_c_tclh}" = x ; then
  TCLHDIR=""
  AC_MSG_RESULT([Can't find Tcl private header])
fi

if test x"${ac_cv_c_tclh}" != x ; then
  no_tcl=""
  if test x"${ac_cv_c_tclh}" = x"installed" ; then
    AC_MSG_RESULT([is installed])
    TCLHDIR=""
  else
    AC_MSG_RESULT([found in ${ac_cv_c_tclh}])
    # this hack is cause the TCLHDIR won't print if there is a "-I" in it.
    TCLHDIR="-I${ac_cv_c_tclh}"
  fi
fi

no_tcl=true
AC_MSG_CHECKING(for Tk private header)
AC_ARG_WITH(tkinclude, [  --with-tkinclude       directory where tk private headers are], with_tkinclude=${withval}) 
AC_CACHE_VAL(ac_cv_c_tkh,[
# first check to see if --with-tkinclude was specified
if test x"${with_tkinclude}" != x ; then
  if test -f ${with_tkinclude}/tk.h ; then
     ac_cv_c_tkh="${with_tkinclude}"
  else
     AC_MSG_ERROR([${with_tkinclude} directory doesn't contain private headers])
  fi
fi
#
# check some common install locations
#
if test x"${ac_cv_c_tkh}" = x ; then
   for i in \
		`ls -dr /usr/local/src/tk[[0-9]].* 2>/dev/null` \
                `ls -dr /usr/local/lib/tk[[0-9]].* 2>/dev/null` \
                `ls -dr /usr/local/tk[[0-9]].*/include 2>/dev/null` \
                `ls -dr /usr/pkg/include/tk[[0-9]].* 2>/dev/null` \
                `ls -dr /usr/include/tcl[[0-9]].* 2>/dev/null` \
                /usr/local/include \
                /usr/local/src/tk \
                /usr/local/lib/tk \
                /sw/include \
                ${prefix}/include \
                ${x_includes}
   do
	if test -f $i/tk.h
	then
	   ac_cv_c_tkh=`echo $i`
	   break
	fi
   done
fi
#
# now check if one is installed
#
if test x"${ac_cv_c_tkh}" = x ; then
  AC_TRY_CPP([#include <tk.h>], eval "ac_cv_c_tkh=installed",
    eval "ac_cv_c_tkh=")
fi
])
#end of cache

if test x"${ac_cv_c_tkh}" = x ; then
  TKDIR="# no Tk private headers found"
  AC_MSG_RESULT([Can't find Tk private header])
fi

if test x"${ac_cv_c_tkh}" != x ; then
  no_tcl=""
  if test x"${ac_cv_c_tkh}" = x"installed" ; then
    AC_MSG_RESULT([is installed])
    TKHDIR=""
  else
    AC_MSG_RESULT([found in ${ac_cv_c_tkh}])
    # this hack is cause the TKHDIR won't print if there is a "-I" in it.
    if test "${ac_cv_c_tkh}" != "${ac_cv_c_tclh}"
    then
       TKHDIR="-I${ac_cv_c_tkh}"
    else
       TKHDIR=""
      # do not make double includes
    fi
  fi
fi

#
# check out versions of headerfiles
#
if test x"${no_tcl}" = x
then
  AC_MSG_CHECKING([Tcl version])
  rm -rf tclmajor tclminor
  orig_cflags="$CFLAGS"
  orig_libs="$LIBS"
  LIBS=""
  CFLAGS="$CFLAGS $INCLUDES"
  if test x"${TCLHDIR}" != x ; then
    CFLAGS="$CFLAGS $TCLHDIR"
  fi
AC_TRY_RUN([
#include <stdio.h>
#include "tcl.h"
main() {
        FILE *maj = fopen("tclmajor","w");
        FILE *min = fopen("tclminor","w");
        fprintf(maj,"%d",TCL_MAJOR_VERSION);
        fprintf(min,"%d",TCL_MINOR_VERSION);
        fclose(maj);
        fclose(min);
        return 0;
}],
        tclmajor=`cat tclmajor`
        tclminor=`cat tclminor`
        tclversion=$tclmajor.$tclminor
        tclversion_dotstripped=$tclmajor$tclminor
        AC_MSG_RESULT($tclversion)
        rm -f tclmajor tclminor
,
        AC_MSG_RESULT([can't happen])
,
        AC_MSG_ERROR([can't be cross compiled])
)
  AC_MSG_CHECKING([Tk version])
  rm -rf tkmajor tkminor
  if test x"${TKHDIR}" != x ; then
    CFLAGS="$CFLAGS $TKHDIR"
  fi
AC_TRY_RUN([
#include <stdio.h>
#include "tk.h"
main() {
        FILE *maj = fopen("tkmajor","w");
        FILE *min = fopen("tkminor","w");
        fprintf(maj,"%d",TK_MAJOR_VERSION);
        fprintf(min,"%d",TK_MINOR_VERSION);
        fclose(maj);
        fclose(min);
        return 0;
}],
        tkmajor=`cat tkmajor`
        tkminor=`cat tkminor`
        tkversion=$tkmajor.$tkminor
        tkversion_dotstripped=$tkmajor$tkminor
        AC_MSG_RESULT($tkversion)
        rm -f tkmajor tkminor
,
        AC_MSG_RESULT([can't happen])
,
        AC_MSG_ERROR([can't be cross compiled])
)

  #restore original cflags and libs
  CFLAGS="${orig_cflags}"
  LIBS="${orig_libs}"
fi

#set INCLUDES
if test x"${TCLHDIR}" != x"${INCLUDES}" -a x"${TCLHDIR}" != x
then
   if test x"${INCLUDES}" != x
   then
      INCLUDES="${INCLUDES} ${TCLHDIR}"
   else
      INCLUDES="${TCLHDIR}"
   fi
fi

if test x"${TKHDIR}" != x"${INCLUDES}" -a x"${TKHDIR}" != x
then
   if test x"${INCLUDES}" != x
   then
      INCLUDES="${INCLUDES} ${TKHDIR}"
   else
      INCLUDES="${TKHDIR}"
   fi
fi
])



AC_DEFUN(PR_PATH_TCL_TK_LIB, [
#
# find the tcl and tk labraries
# the alternative search directory is invoked by --with-tcllib=dir or --with-tklib=dir
#
if test x"${no_tcl}" = x ; then
  # we reset no_tcl incase something fails here
  no_tcl=true
  AC_MSG_CHECKING([for Tcl library])
  AC_ARG_WITH(tcllib, [  --with-tcllib           directory where the tcl library is], with_tcllib=${withval})
  AC_CACHE_VAL(ac_cv_c_tcllib,[
  # First check to see if --with-tcllib was specified.
  if test x"${with_tcllib}" != x ; then
    if test -f "${with_tcllib}/libtcl$tclversion.so" ; then
      ac_cv_c_tcllib=`echo ${with_tcllib}`/libtcl$tclversion.so
    elif test -f "${with_tcllib}/libtcl.so" ; then
      ac_cv_c_tcllib=`echo ${with_tcllib}`/libtcl.so
    elif test -f "${with_tcllib}/libtcl.dylib" ; then
      ac_cv_c_tcllib=`echo ${with_tcllib}`/libtcl.dylib
    # then look for a statically linked library
    elif test -f "${with_tcllib}/libtcl$tclversion.a" ; then
      ac_cv_c_tcllib=`echo ${with_tcllib}`/libtcl$tclversion.a
    elif test -f "${with_tcllib}/libtcl$tclversion_dotstripped.a" ; then
      ac_cv_c_tcllib=`echo ${with_tcllib}`/libtcl$tclversion_dotstripped.a
    elif test -f "${with_tcllib}/libtcl.a" ; then
      ac_cv_c_tcllib=`echo ${with_tcllib}`/libtcl.a
    else
      AC_MSG_ERROR([${with_tcllib} directory doesn't contain libraries])
    fi
  fi
  # check in a few common install locations
  if test x"${ac_cv_c_tcllib}" = x ; then
    for i in \
		/sw/lib          \
		/usr/local/lib   \
		/usr/pkg/lib     \
		/usr/lib64       \
		/usr/lib         \
		/lib             \
                `ls -d ${prefix}/lib 2>/dev/null`
    do
      # first look for a dynamically linked library
      if test -f "$i/libtcl$tclversion.so" ; then
        ac_cv_c_tcllib=`echo $i`/libtcl$tclversion.so
        break
      elif test -f "$i/libtcl.so" ; then
        ac_cv_c_tcllib=`echo $i`/libtcl.so
        break
      elif test -f "$i/libtcl.dylib" ; then
        ac_cv_c_tcllib=`echo $i`/libtcl.dylib
        break
      # then look for a statically linked library
      elif test -f "$i/libtcl$tclversion.a" ; then
        ac_cv_c_tcllib=`echo $i`/libtcl$tclversion.a
        break
      elif test -f "$i/libtcl$tclversion_dotstripped.a" ; then
        ac_cv_c_tcllib=`echo $i`/libtcl$tclversion_dotstripped.a
        break
      elif test -f "$i/libtcl.a" ; then
        ac_cv_c_tcllib=`echo $i`/libtcl.a
	break
      fi
    done
  fi

# check for dl-lib or ld-lib 

 orig_libs="$LIBS"

 LIBS="-ldl"
    AC_TRY_LINK( , [main()] , DLLIB="-ldl", DLLIB=""; LIBS="")

 LIBS="-lld"
    AC_TRY_LINK(, [main()], LDLIB="-lld", LDLIB=""; LIBS="")

 

  # see if one is installed
  if test x"${ac_cv_c_tcllib}" = x ; then
    LIBS="-ltcl -lm"
    AC_TRY_LINK( , [main()], ac_cv_c_tcllib="-ltcl", ac_cv_c_tcllib="")
  fi
LIBS="${orig_libs}"

  ]) #end of cache

  if test x"${ac_cv_c_tcllib}" = x ; then
    TCLLIBS=""
    AC_MSG_WARN(Can't find Tcl library)
  else
    TCLLIBS="${ac_cv_c_tcllib}"
    AC_MSG_RESULT(found $TCLLIBS)
    TCLLIBS="$TCLLIBS ${DLLIB} ${LDLIB}"
    no_tcl=""
  fi
fi

if test x"${no_tcl}" = x ; then
  # we reset no_tcl incase something fails here
  no_tcl=true
  AC_MSG_CHECKING([for Tk library])
  AC_ARG_WITH(tklib, [  --with-tklib           directory where the tk library is], with_tklib=${withval})
  AC_CACHE_VAL(ac_cv_c_tklib,[
  # First check to see if --with-tklib was specified.
  if test x"${with_tklib}" != x ; then
    if test -f "${with_tklib}/libtk$tkversion.so" ; then
      ac_cv_c_tklib=`echo ${with_tklib}`/libtk$tkversion.so
    elif test -f "${with_tklib}/libtk.so" ; then
      ac_cv_c_tklib=`echo ${with_tklib}`/libtk.so
    elif test -f "${with_tklib}/libtk.dylib" ; then
      ac_cv_c_tklib=`echo ${with_tklib}`/libtk.dylib
    # then look for a statically linked library
    elif test -f "${with_tklib}/libtk$tkversion.a" ; then
      ac_cv_c_tklib=`echo ${with_tklib}`/libtk$tkversion.a
    elif test -f "${with_tklib}/libtk$tkversion_dotstripped.a" ; then
      ac_cv_c_tklib=`echo ${with_tklib}`/libtk$tkversion_dotstripped.a
    elif test -f "${with_tklib}/libtk.a" ; then
      ac_cv_c_tklib=`echo ${with_tklib}`/libtk.a
    else
      AC_MSG_ERROR([${with_tklib} directory doesn't contain libraries])
    fi
  fi
  # check in a few common install locations
  if test x"${ac_cv_c_tklib}" = x ; then
    for i in \
		/sw/lib          \
		/usr/local/lib   \
		/usr/pkg/lib     \
		/usr/lib64       \
		/usr/lib         \
		/lib             \
                ${prefix}/lib    \
		${x_libraries}
    do
      # first look for a dynamically linked library
      if test -f "$i/libtk$tkversion.so" ; then
        ac_cv_c_tklib=`echo $i`/libtk$tkversion.so
        break
      elif test -f "$i/libtk.so" ; then
        ac_cv_c_tklib=`echo $i`/libtk.so
        break
      elif test -f "$i/libtk.dylib" ; then
        ac_cv_c_tklib=`echo $i`/libtk.dylib
        break
      # then look for a statically linked library
      elif test -f "$i/libtk$tkversion.a" ; then
        ac_cv_c_tklib=`echo $i`/libtk$tkversion.a
        break
      elif test -f "$i/libtk$tkversion_dotstripped.a" ; then
        ac_cv_c_tklib=`echo $i`/libtk$tkversion_dotstripped.a
        break
      elif test -f "$i/libtk.a" ; then
        ac_cv_c_tklib=`echo $i`/libtk.a
	break
      fi
    done
  fi
  # see if one is installed
  if test x"${ac_cv_c_tklib}" = x ; then
    orig_libs="$LIBS"
    LIBS="-ltk $TCLLIBS $XLIBS"
    AC_TRY_LINK( , [main()], ac_cv_c_tklib="-ltk", ac_cv_c_tklib="")
    LIBS="${orig_libs}"
  fi
  ]) #end of cache


  if test x"${ac_cv_c_tklib}" = x ; then
    TCLLIBS=""
    AC_MSG_WARN(Can't find Tk library)
  else
    TCLLIBS="${ac_cv_c_tklib} ${TCLLIBS}"
    AC_MSG_RESULT(found ${ac_cv_c_tklib})
    no_tcl=
  fi
fi

#set CFLAGS
if test x"${no_tcl}" = x
then
  CFLAGS="$CFLAGS -DTCL"
fi

AC_SUBST(TCLLIBS)
])



AC_DEFUN(PR_PATH_TCL_TK, [
   PR_PATH_TCL_TK_H
   PR_PATH_TCL_TK_LIB
])




AC_DEFUN(PR_PATH_ZLIB_H, [
#
# find zlib header
# the alternative search directory is involked by --with-zlibinclude=dir or with-tkinclude=dir
#

no_zlib=true
AC_MSG_CHECKING(for zlib private headers)
AC_ARG_WITH(zlibinclude, [  --with-zlibinclude       directory where zlib private headers are], with_zlibinclude=${withval}) 
AC_CACHE_VAL(ac_cv_c_zlibh,[
# first check to see if --with-zlibinclude was specified
if test x"${with_zlibinclude}" != x ; then
  if test -f ${with_zlibinclude}/zlib.h ; then
     ac_cv_c_zlibh=`echo ${with_zlibinclude}`
  else
     AC_MSG_ERROR([${with_zlibinclude} directory doesn't contain private headers])
  fi
fi
#
# now check if it is running with the INCLUDES we made so far
#
if test x"${ac_cv_c_zlibh}" = x ; then
  orig_cppflags="${CPPFLAGS}"
  CPPFLAGS=$CFLAGS
  AC_TRY_CPP([#include <zlib.h>], eval "ac_cv_c_zlibh=installed",
    eval "ac_cv_c_zlibh=")
  CPPFLAGS="${orig_cppflags}"
fi

#
# now check some common places
#
if test x"${ac_cv_c_zlibh}" = x ; then
  for i in 			\
	/usr/local/include   	\
	/usr/include		\
	${prefix}/include
  do
     if test -f $i/zlib.h
     then
        ac_cv_c_zlibh=$i
	break
     fi
  done	
fi

#end of cache
])

if test x"${ac_cv_c_zlibh}" = x ; then
  ZLIBHDIR=""
  AC_MSG_RESULT([Can't find zlib private header])
fi

if test x"${ac_cv_c_zlibh}" != x ; then
  no_zlib=""
  if test x"${ac_cv_c_zlibh}" = x"installed" ; then
    AC_MSG_RESULT([is installed])
    ZLIBHDIR=""
  else
    AC_MSG_RESULT([found in ${ac_cv_c_zlibh}])
    # this hack is cause the ZLIBHDIR won't print if there is a "-I" in it.
    ZLIBHDIR="-I${ac_cv_c_zlibh}"
  fi
fi

#
# check out versions of headerfile
#
if test x"${no_zlib}" = x
then
AC_MSG_CHECKING(zlib version)
AC_CACHE_VAL(ac_cv_c_zlibversion, [
  rm -rf zlibversion
  orig_cflags="$CFLAGS"
  orig_libs="$LIBS"
  LIBS=""
  CFLAGS="$CFLAGS $INCLUDES"
  if test x"${ZLIBHDIR}" != x ; then
    CFLAGS="$CFLAGS $ZLIBHDIR"
  fi
  AC_TRY_RUN([
#include <stdio.h>
#include "zlib.h"
main() {
          FILE *zlib = fopen("zlibversion","w");
          fprintf(zlib,"%s",ZLIB_VERSION);
          fclose(zlib);
          return 0;
  }],
        ac_cv_c_zlibversion=`cat zlibversion`
        rm -f zlibversion
  ,
        AC_MSG_RESULT([can't happen])
  ,
        AC_MSG_ERROR([can't be cross compiled])
  )

#restore cflags and libs
CFLAGS="${orig_cflags}"
LIBS="${orig_libs}"
# end of cache
])
AC_MSG_RESULT($ac_cv_c_zlibversion)

#set INCLUDES
if test x"${ZLIBHDIR}" != x"${INCLUDES}" -a x"${ZLIBHDIR}" != x
then
AC_MSG_RESULT(here)
   if test x"${INCLUDES}" != x
   then
      INCLUDES="${INCLUDES} ${ZLIBHDIR}"
   else
      INCLUDES="${ZLIBHDIR}"
   fi
fi
fi
])

AC_DEFUN(PR_PATH_ZLIB_LIB, [
#
# check if --with-zlib ist specified
#
no_zlib=true
AC_MSG_CHECKING(for zlib)
AC_ARG_WITH(zlib, [  --with-zlib	directory where zlib can be found], with_zlib=${withval}) 
AC_CACHE_VAL(ac_cv_c_zlib,[
# first check to see if --with-zlib was specified
if test x"${with_zlib}" != x ; then
  if test -f ${with_zlib}/libz.so -o -f ${with_zlib}/libz.a; then
     ac_cv_c_zlib=`echo ${with_zlib}`
  else
     AC_MSG_ERROR([${with_zlib} directory doesn't contain zlib])
  fi
fi

#
# check for zlib in common places
#
if test x"${ac_cv_c_zlib}" = x
then
  for i in              \
     /usr/local/lib	\
     /usr/lib64		\
     /usr/lib
  do
     if test -f $i/libz.so
     then
        ac_cv_c_zlib=$i/libz.so
        break
     elif test -f $i/libz.a
     then
        ac_cv_c_zlib=$i/libz.a
        break
     fi
  done
fi

#
# now check if one is install or can be found in the XLIB-path 
#
if test x"${ac_cv_c_zlib}" = x ; then
  orig_libs="$LIBS"
  LIBS="-lz $XLIBS"
  AC_TRY_LINK( , [main()], ac_cv_c_zlib="-lz", ac_cv_c_zlib="")
  LIBS="${orig_libs}"
fi
]) #end of cache

if test x"${ac_cv_c_zlib}" != x
then
  ZLIBS="${ac_cv_c_zlib}"
  no_zlib=""
  AC_MSG_RESULT(found $ZLIB)
else
  ZLIBS=""
  AC_MSG_RESULT(no)  
fi

#set CFLAGS
if test x"${no_zlib}" = x
then
  CFLAGS="$CFLAGS -DZLIB"
fi

AC_SUBST(ZLIBS)

])


AC_DEFUN(PR_PATH_ZLIB, [
  PR_PATH_ZLIB_H
  PR_PATH_ZLIB_LIB
])



AC_DEFUN(PR_XLIBS, [
#
# check for Xt X11 Xext
#

AC_CACHE_VAL(ac_cv_c_xlibs, [
  xlibs_error=true
  orig_libs="$LIBS"

  # checking for Xt and X11
  LIBS="$LIBS $XLIBS"
  LIBS="$LIBS -lXt -lX11"
  AC_TRY_LINK( , [main()], xlibs_error=""; XLIBS="$XLIBS -lXt -lX11")
  
  if test x"${xlibs_error}" = x"true"
  then
     AC_MSG_ERROR(could not find Xt or X11)
  fi

  xlib_error=true
  #checking for Xext
  LIBS="$LIBS -lXext"
  AC_TRY_LINK( , [main()], xlibs_error=""; XLIBS="$XLIBS -lXext")

  xlib_error=true
  #checking for ICE
  #reset libs for next test if something went wrong
  if test x"${xlibs_errort}" = x"true"
  then
     LIBS="${orig_libs} ${XLIBS}"
  fi  

  LIBS="$LIBS -lICE"
  AC_TRY_LINK( , [main()], xlibs_error=""; XLIBS="$XLIBS -lICE")

  xlib_error=true
  #checking for SM
  #reset libs for next test if something went wrong
  if test x"${xlibs_errort}" = x"true"
  then
     LIBS="${orig_libs} ${XLIBS}"
  fi  

  LIBS="$LIBS -lSM"
  AC_TRY_LINK( , [main()], xlibs_error=""; XLIBS="$XLIBS -lSM")


#reset LIBS
LIBS="${orig_libs}"

ac_cv_c_xlibs="${XLIBS}"
#end of cache
])
AC_MSG_RESULT(using x libraries: $ac_cv_c_xlibs)
XLIBS="${ac_cv_c_xlibs}"
])


AC_DEFUN(PR_PATH_MOTIF, [
#
# check for uil and set MOTIF
#

no_motif=true
AC_MSG_CHECKING(for Motif)
AC_MSG_RESULT([{])
AC_ARG_WITH(motif, [  --with-motif       directory where motif headers are], with_motif=${withval}) 
AC_ARG_WITH(uil, [  --with-uil       directory where uil can be found], with_uil=${withval}) 

#
# first check if --with-motif was specified
#
if test x"${with_motif}" != x
then
  if test -d ${with_motif}
  then
    INLUDES="$INLCUDES -I$with_motif"
    no_motif=""
  else
    AC_MSG_ERROR([${with_motif} directory doesn't exist])
  fi
fi


#
# search for an include directory
#
if test x${no_motif} = x"true"
then
  if test -d /usr/include/Motif?.*
  then
    PATH_MOTIF="/usr/include/`ls /usr/include | grep ^Motif | sort | tail -1`"
    INCLUDES="$INCLUDES -I$PATH_MOTIF"
  elif test -d /usr/X11R6/include/Xm
  then
    true
  elif test -d /usr/dt/include
  then
    INCLUDES="$INCLUDES -I/usr/dt/include"
  elif test -d /usr/local/dt/include
  then
    INCLUDES="$INCLUDES -I/usr/local/dt/include"
  elif test -d /usr/openwin/include
  then
    INCLUDES="$INCLUDES -I/usr/openwin/include"
  fi
fi

# now check for uil
no_uil=true

# first check if --with-uil was specified
if test x"${with_uil}" != x
then
  if test -x ${with_uil}
  then
    UIL="${with_uil}"
    no_uil=""
  elif test -x ${with_uil}/uil
  then
    UIL="${with_uil}/uil"
    no_uil=""
  else
    AC_MSG_ERROR([${with_uil} doesn't exist])
  fi
fi


if test x"${no_uil}" = x"true"
then
  AC_CHECK_PROG(UIL, uil, uil)
  if test "x$UIL" != "x"
  then
     no_uil=""
  fi 
  UILDIR=/usr/include

  # search for uil include path
  if test -d /usr/include/X11/uil
  then
    UILDIR=/usr/include/X11/uil
  elif test -d /usr/include/uil
  then
    UILDIR=/usr/include/uil
  elif test -d /usr/include/Motif?.*/uil
  then
    TMP_UILDIR='/usr/include/Motif?.*/uil'
    UILDIR=`echo /usr/include/Motif?.*/uil`
  elif test -d /usr/include/Motif?.*/Xm
  then
    UILDIR=`echo /usr/include/Motif?.*/Xm`
  elif test -d /usr/X11R?/include/uil
  then
    UILDIR=`echo /usr/X11R?/include/uil`
  elif test -d /usr/dt/include/uil
  then
    UILDIR=/usr/dt/include/uil
  elif test -d /usr/local/dt/include/uil
  then
    UILDIR=/usr/local/dt/include/uil
  fi
fi

#
# at last check for the Xmlibs 
# 

AC_MSG_CHECKING(for xmlibs)
#
# search for a library directory
#
XMLIBS=
if test x"${no_motif}" = x"true"
then
  if test -d /usr/lib/Motif?.*
  then
    XMLIBS="-L/usr/lib/`ls /usr/lib | grep ^Motif | sort | tail -1`"
  elif test -f /usr/X11R6/lib/libXm.so
  then
    true
  elif test -d /usr/dt/lib
  then
    XMLIBS="-L/usr/dt/lib"
  elif test -d /usr/local/dt/lib
  then
    XMLIBS="-L/usr/local/dt/lib"
  elif test -d /usr/openwin/lib
  then
    XMLIBS="-L/usr/openwin/lib"
  elif test -d /sw/lib
  then
    XMLIBS="-L/sw/lib"
  fi
fi

orig_libs="${LIBS}"
LIBS="${XMLIBS} ${XLIBS}"

#check for Xm
LIBS="${LIBS} -lXm"
AC_TRY_LINK( , [main()], [XMLIBS="$XMLIBS -lXm"], LIBS="${XLIBS}")

#check for Mrm
LIBS="${LIBS} -lMrm"
AC_TRY_LINK( , [main()], [XMLIBS="$XMLIBS -lMrm"], LIBS="${XLIBS}")

#check for DXm
LIBS="${LIBS} -lDXm"
AC_TRY_LINK( , [main()], [XMLIBS="$XMLIBS -lDXm"], LIBS="${XLIBS}")

#check for PW
LIBS="${LIBS} -lPW"
AC_TRY_LINK( , [main()], [XMLIBS="$XMLIBS -lPW"], LIBS="${XLIBS}")

if test "x$XMLIBS" != "x"
then
  AC_MSG_RESULT(using $XMLIBS)
else
  AC_MSG_RESULT(no)
fi  

MOTIF=
if test "x$no_uil" = "x" -a "x$XLIBS" != "x"
then
  MOTIF="motif"
  no_motif=""
  CFLAGS="$CFLAGS -DMOTIF"
  AC_MSG_RESULT([ }  using Motif])
else
  AC_MSG_RESULT([ }  can't set Motif])
fi 

#restore LIBS
LIBS="${orig_libs}"

])
