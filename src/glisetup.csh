#! /bin/csh -f
#
if ( -e /bin/machine ) then
  set machine = `machine`
else if ( -e /bin/arch ) then
  set machine = `arch`
else if ( -e /bin/uname ) then
  set machine = `uname`
else if ( -e /usr/bin/uname ) then
  set machine = `uname`
else
  echo "Can't obtain system information."; exit
endif
#
set machine=`echo $machine | sed -e 'y/ABCDEFGHIJKLMNOPQRSTUVWXYZ/abcdefghijklmnopqrstuvwxyz/' | sed -e 's/\-//g'`
#
if ( -e ~/gli/env/$machine/gli/gli ) then
  set gli_home = ~/gli/env/$machine/gli
else if ( -e /opt/gli/gli ) then 
  set gli_home = /opt/gli
else if ( -e /usr/local/gli/gli ) then 
  set gli_home = /usr/local/gli
else 
  echo "Can't obtain GLI home directory."; exit
endif
#
setenv GLI_HOME $gli_home
#
alias gli $GLI_HOME/gli
alias gligksm $GLI_HOME/gligksm
alias glitcldemo $GLI_HOME/tcl/demo/glitcldemo
alias cgmview $GLI_HOME/cgmview
#
if ( ! $?DISPLAY ) then
  set tty_name = `who am i | awk '{ print $NF }'`
  if ( `echo $tty_name | grep -c "(*)"` == 1 ) then
    set display = `echo $tty_name | sed -e s/\(// -e s/\)// | awk '{split($0,a,"."); print a[1]}'` 
    set domainname = `/bin/domainname`
    if ( "$domainname" != "" ) then
      if ( "`echo $display | grep $domainname`" != "" ) then
        set display = `echo $display | awk -F. '{print $1}'`
      endif
    endif
    if ( `echo $display | grep -c ":"` != 1 ) then
      set display = $display":0.0"
    endif
    if ( $display != ":0.0" ) then
      setenv DISPLAY $display
    endif
  endif
endif
#
# glisetup.csh,  GLI V4.5
