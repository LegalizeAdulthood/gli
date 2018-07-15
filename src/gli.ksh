#! /bin/ksh
if [[ -a $GLI_HOME/gli ]]; then
    true
elif [[ -a /usr/local/gli/glisetup.ksh ]]; then
    . /usr/local/gli/glisetup.ksh
else
    echo "$0: Command not found."
fi
if [[ -a $GLI_HOME/gli ]]; then
    $GLI_HOME/gli $*
fi
