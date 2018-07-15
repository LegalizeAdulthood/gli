$ say := write sys$output
$ if f$trnlnm("GLI_HOME") .eqs. ""
$ then
$   say " "
$   say "The GLI environment is not setup correctly. You should execute the"
$   say "GLI startup procedure using before invoking this script."
$   say " "
$   exit
$ endif
$!
$ if p2 .nes. "" then exit 229528
$ gkslib = p1
$ if gkslib .nes. ""
$ then
$   if f$parse(gkslib,,,,"SYNTAX_ONLY") .eqs. "" then exit 100052
$   if f$search(gkslib) .eqs. "" then exit 98962
$ else
$   gkslib = "libgks.olb"
$ endif
$!
$ decgks = f$search("sys$library:gksforbnd.olb") .nes. ""
$ decgks3d = f$search("sys$library:gks3d$forbnd.olb") .nes. ""
$!
$ version = f$getsyi("version")
$ version = f$extract(f$locate(version,"V"),3,version)
$!
$ call link "''gkslib'" "gli" "gli"
$ if decgks then -
  call link "sys$library:gksforbnd.olb" "gli" "glidecgks"
$ if decgks3d then -
  call link "sys$library:gks3d$forbnd.olb" "gli" "glidecgks"
$ call link "''gkslib'" "cgmview" "cgmview"
$!
$!
$ exit
$!
$link:
$!
$ subroutine
$!
$ lib = "''p1'"
$ obj = "''p2'.obj"
$ exe = "''p3'.exe"
$ if f$search(exe) .nes. "" then delete/nolog 'exe';*
$
$ say "Re-link " + f$edit(exe,"upcase")
$
$ link/nouser/exec='exe' 'obj', 'gli_libs'
$!
$ set file/prot=(s:rwed,o:rwed,g:rwed,w:re) 'exe'
$!
$ endsubroutine
