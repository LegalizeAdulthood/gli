#! /bin/ksh
#
if [[ -a /bin/machine ]]; then
  machine=`machine`
elif [[ -a /bin/arch ]]; then
  machine=`arch`
elif [[ -a /bin/uname ]]; then
  machine=`uname`
else
  echo "Can't obtain system information."; exit
fi
#
machine=`echo $machine | sed -e 'y/ABCDEFGHIJKLMNOPQRSTUVWXYZ/abcdefghijklmnopqrstuvwxyz/' | sed -e 's/\-//g'`
#
if [[ -a $HOME/gli/env/$machine/gli/gli ]]; then
  gli_home=$HOME/gli/env/$machine/gli
elif [[ -a /opt/gli/gli ]]; then
  gli_home=/opt/gli
elif [[ -a /usr/local/gli/gli ]]; then
  gli_home=/usr/local/gli
else
  echo "Can't obtain GLI home directory."; exit
fi
#
export GLI_HOME; GLI_HOME=$gli_home
#
alias gli=$GLI_HOME/gli
alias gligksm=$GLI_HOME/gligksm
alias glitcldemo=$GLI_HOME/tcl/demo/glitcldemo
alias cgmview=$GLI_HOME/cgmview
#
if [[ $DISPLAY = "" ]]; then
  tty_name=`who am i | awk '{ print $NF }'`
  if [[ `echo $tty_name | grep -c "(*)"` = 1 ]]; then
    display=`echo $tty_name | sed -e s/\(// -e s/\)// | awk '{split($0,a,"."); print a[1]}'`
    if [[ `echo $display | grep -c ":"` != 1 ]]; then
      display=$display":0.0"
    fi
    if [[ $display != ":0.0" ]]; then
      export DISPLAY=$display
    fi
  fi
fi
#
# glisetup.ksh,  GLI V4.5
