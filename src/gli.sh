#! /bin/sh
if [ `uname` = "Darwin" ]
then
  exec /usr/local/gli/gli.app/Contents/MacOS/gli $*
else
  if [ -f $GLI_HOME/gli ]
  then
    true
  elif [ -f /usr/local/gli/glisetup.sh ]
  then
    . /usr/local/gli/glisetup.sh
  else
    echo "$0: Command not found."
  fi
  if [ -f $GLI_HOME/gli ]
  then
    $GLI_HOME/gli $*
  fi
fi
