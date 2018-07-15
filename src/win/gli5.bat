@echo off
set GKS_FONTPATH=s:\gks
set GLI_HOME=s:\gli
set TK_LIBRARY=%GLI_HOME%\tk8.3
set TCL_LIBRARY=%GLI_HOME%\tcl8.3
%GLI_HOME%\gli5 %1 %2 %3 %4 %5 %6 %7 %8
