   SHELL = /bin/sh

gksfont.dat : mkfont hershey.dat fillfont.dat
	mkfont

mkfont : mkfont.f
	f77 -o mkfont mkfont.f

clean :
	rm -f gksfont.dat mkfont *.bak
