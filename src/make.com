$ if p1 .nes. "" then goto 'p1'
$!
$ define/nolog sys sys$library
$ define/nolog X11 decw$include
$ srcdir := "''f$environment("default")'"
$ define/nolog uil$include sys$sysroot:[decw$include], 'srcdir'
$!
$ if f$search("*.for") .nes. ""
$ then
$   for := "for"
$ else
$   for := "f"
$ endif
$!
$ cc :== cc/warnings=noinformational
$ if f$extract(0,2,f$getsyi("version")) .ges. "V6"
$ then
$   opt := ovmslink.opt
$ else
$   opt := vmslink.opt
$ endif
$!
$ set verify
$!
$ cc gli.c
$!
$ cc glisight.c /define=motif
$ cc gligus.c
$ cc gligr.c
$ cc gligks.c
$ cc glisimp.c
$ cc glixui.c
$ cc gliimage.c
$ cc/noopt glicgm.c
$ cc glirpc.c
$ fortran gkstest.'for'
$ library/create libgli.olb glisight.obj,gligus.obj,gligr.obj,-
  gligks.obj,glisimp.obj,glixui.obj,gliimage.obj,glicgm.obj,-
  glirpc.obj,gkstest.obj
$!
$ cc sightmai.c /define=motif
$ cc sight.c
$ cc sightdev.c
$ library/create libsight.olb sightmai.obj,sight.obj,sightdev.obj
$!
$ cc imagemai.c /define=motif
$ cc image.c
$ cc imageuti.c
$ cc imageapp.c
$ cc imagedev.c
$ library/create libimage.olb imagemai.obj,image.obj,imageuti.obj,-
  imageapp.obj,imagedev.obj
$!
$ cc gus.c
$ cc gustext.c
$ cc gusplo10.c
$ cc gusauto.c
$ library/create libgus.olb gus.obj,gustext.obj,gusplo10.obj,gusauto.obj
$!
$ cc gksio.c
$ cc gkscbnd.c
$ fortran gks.'for'
$ fortran gksinq.'for'
$ fortran gkserror.'for'
$ fortran gksroot.'for'
$ cc gksmisc.c
$ fortran gksdidd.'for'
$ cc gksdps.c
$ fortran gksdtek.'for'
$ fortran gksdtek2.'for'
$ cc gksdx11.c
$ cc gksduis.c
$ cc gksdcgm.c
$ cc gkswiss.c
$ fortran gksdwiss.'for'
$ fortran gksdhpgl.'for'
$ fortran gksdvt.'for'
$ cc gksdgksm.c
$ cc gksdpdf.c
$ fortran gksdpbm.'for'
$ fortran gksforio.'for'
$ cc gksafm.c
$ cc compress.c
$!
$ library/create libgks.olb -
 gksio.obj, gkscbnd.obj, gks.obj, gksinq.obj, gkserror.obj, gksroot.obj,-
 gksmisc.obj, gksdidd.obj, gksdps.obj, gksdtek.obj, gksdtek2.obj,-
 gksdx11.obj, gksduis.obj, gksdcgm.obj, gkswiss.obj, gksdwiss.obj,-
 gksdhpgl.obj, gksdvt.obj, gksdgksm.obj, gksdpdf.obj, gksdpbm.obj,-
 gksforio.obj, gksafm.obj, compress.obj
$!
$ cc formdrv.c
$ cc command.c
$ cc function.c
$ cc variable.c
$ cc symbol.c
$ cc terminal.c
$ cc string.c
$ cc math.c
$ cc help.c
$ cc system.c
$ cc com.c
$ library/create libcrtl.olb formdrv.obj,command.obj,function.obj,variable.obj,-
  symbol.obj,terminal.obj,string.obj,math.obj,help.obj,system.obj,com.obj
$!
$ fortran contour.'for'
$ cc spline.c
$ cc pixel.c
$ fortran gridit.'for'
$ library/create libfrtl.olb contour.obj,spline.obj,pixel.obj,gridit.obj
$!
$ link gli.obj,-
  libgli.olb/lib,libsight.olb/lib,libimage.olb/lib,-
  libgus.olb/lib,libcrtl.olb/lib,libfrtl.olb/lib,libgks.olb/lib,-
  'opt'/opt
$!
$ cc cgmview.c /define=motif
$ link cgmview.obj,-
  libgli.olb/lib,libsight.olb/lib,libimage.olb/lib,-
  libgus.olb/lib,libcrtl.olb/lib,libfrtl.olb/lib,libgks.olb/lib,-
  'opt'/opt
$!
$ cc gligksm.c
$ link gligksm.obj,libcrtl.olb/lib,libgks.olb/lib,-
  'opt'/opt
$!
$ uil/motif sight.uil
$ uil/motif aeliter.uil
$ uil/motif deliter.uil
$ uil/motif image.uil
$ uil/motif cgmview.uil
$!
$ library/create/help gli.hlb gli.hlp
$!
$ pascal [.pas]gksdefs.pas
$!
$ set noverify
$!
$ exit
$!
$clean:
$ delete/nolog *.obj;*, *.pen;*, *.olb;*, *.exe;*, *.uid;*, *.hlb;*
$!
$ exit
