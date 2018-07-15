gksfont.dat : mkfont.exe hershey.dat fillfont.dat
	run mkfont.exe

mkfont.exe : mkfont.obj
	link mkfont

mkfont.obj : mkfont.for
	fortran mkfont

clean :
	delete/nolog gksfont.dat;*, *.obj;*, *.exe;*
