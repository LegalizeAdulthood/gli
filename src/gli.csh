#! /bin/csh -f
if ( $?GLI_HOME ) then
    true
else if ( -f /usr/local/gli/glisetup.csh ) then
    source /usr/local/gli/glisetup.csh
else
    echo "$0: Command not found."
endif
if ( -f $GLI_HOME/gli ) then
    $GLI_HOME/gli $*
endif
