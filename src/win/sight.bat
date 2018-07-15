@echo off
set GLI_HOME=s:\gli
set TK_LIBRARY=%GLI_HOME%\tk8.3
set TCL_LIBRARY=%GLI_HOME%\tcl8.3
%GLI_HOME%\gli -file %GLI_HOME%\tcl\modules\modules.tcl
