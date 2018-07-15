$!
$! This command procedure makes the necessary definitions
$! for GLI to run.
$!
$! Find the directory specification for the setup procedure
$!
$ if f$trnlnm("gli_home") .nes. "" then exit
$!
$ Arch = f$getsyi("arch_name")
$ if Arch .nes. "Alpha"
$ then
$   Min_Version := "5.5"
$   OS := "VAX/VMS"
$   opt := "vmslink.opt"
$ else
$   Min_Version := "7.0"
$   OS := "OpenVMS"
$   opt := "ovmslink.opt"
$ endif
$!
$ Procedure := 'f$environment("PROCEDURE")'
$ Device := 'f$parse(Procedure,,,"DEVICE","NO_CONCEAL")'
$ Directory := 'f$parse(Procedure,,,"DIRECTORY","NO_CONCEAL")'
$ Path := "''Device'''Directory'"
$!
$! Check for "rooted" directory specifications
$!
$ l = 'f$length(Path)'
$ tmp = 'f$locate(".][",Path)'
$ if 'tmp' .ne. 'l' then goto 10$
$ tmp = 'f$locate(".><",Path)'
$ if 'tmp' .ne. 'l' then goto 10$
$ goto 100$
$!
$! Eliminate rooted directory specifications
$!
$ 10$:
$ if "''f$extract(tmp,255,Path)'" .eqs. ".][000000]" then goto 20$
$ if "''f$extract(tmp,255,Path)'" .eqs. ".><000000>" then goto 20$
$ l = tmp + 3
$ Path := "''f$extract(0,tmp,Path)'.''f$extract(l,255,Path)'"
$ goto 100$
$ 20$:
$ l = tmp + 1
$ Path := "''f$extract(0,tmp,Path)'''f$extract(l,1,Path)'"
$ 100$:
$ define/nolog	gli_home 'Path'
$!
$! Define foreign commands and symbols
$!
$ gli :== $gli_home:gli
$ glid*ecgks :== $gli_home:glidecgks
$ cgmview :== $gli_home:cgmview
$ gli_libs :== gli_home:libgli/lib,libgr/lib,libsight/lib,libimage/lib,-
libgus/lib,libcrtl/lib,libfrtl/lib,libgks/lib,'opt'/opt
$ gli_slibs :== gli_home:libgli/lib,gr/lib,libsight/lib,libimage/lib,-
libgus/lib,libcrtl/lib,libfrtl/lib,gks/lib,'opt'/opt
$ glidecgks_libs :== gli_home:libgli/lib,libgr/lib,libsight/lib,libimage/lib,-
libgus/lib,libcrtl/lib,libfrtl/lib,'opt'/opt,sys$library:gks3d$forbnd/lib
$!
$! Define GR/GR3 logical name
$!
$ define/nolog	grgks GLIGKS
$!
$! Define GLI logical names
$!
$ if f$search("gli_home:gligksshr.exe") .nes. ""
$ then
$   define/nolog gligksshr gli_home:gligksshr
$ endif
$ if f$search("gli_home:grshr.exe") .nes. ""
$ then
$   define/nolog grshr gli_home:grshr
$ endif
$ define/nolog	gli_pl $plotter
$ define/nolog	gli_lw $laser_printer
$!
$ if Arch .eqs. "Alpha" then goto Check_Version
$!
$! Check for UIS
$!
$ if f$search("sys$share:uisshr.exe") .eqs. "" .and. -
    f$search("sys$share:uisxshr.exe") .eqs. ""
$ then
$   define/nolog uisshr gli_home:uisshr
$ endif
$!
$! Check for revised version of VMS Math Library
$!
$ if f$search("sys$share:fortran$uvmthrtl-vms.exe") .eqs. ""
$ then
$   define/nolog mthrtl gli_home:uvmthrtl
$ endif
$!
$Check_Version:
$!
$! Check for valid VMS version
$!
$ version = f$getsyi("version")
$ version = f$extract(f$locate(version,"V"),3,version)
$!
$ if version .lts. Min_Version
$ then
$   write sys$output "For the GLI software to function properly, you need to"
$   write sys$output "install VMS V''Min_Version' or later."
$   exit
$ endif
$!
$ if f$mode() .nes. "INTERACTIVE" then exit
$ set term/nowrap
$!
$ node = f$trnlnm("sys$rem_node")
$ if node .nes. ""
$ then
$   set display/create/node='node'
$ endif
$!
$ write sys$output "GLI Version 4.5 is now setup for ''OS'"
$!
$ exit
