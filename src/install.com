$ root = "sys$disk:[gli.env.vms.]"
$ define/trans=(conc) usr 'root'
$ cp :== COPY/PROT=(S:RWED,O:RWED,G:RWED,W:RE)
$ echo :== WRITE SYS$OUTPUT
$ echo "Installing GLI executables"
$ cp *.exe, gli.obj, cgmview.obj, [.decw]*.exe usr:[gli]
$ cp glisetup.com, gkslogin.com, relink.com usr:[gli]
$ echo "Installing GLI archive files"
$ cp lib*.olb usr:[gli]
$ if f$search("*.uid") .nes. "" then cp *.uid usr:[gli]
$ echo "Installing GLI environment files"
$ cp glistart.gli usr:[gli]
$ cp *.fdv usr:[gli]
$ cp *.h usr:[gli]
$ cp *.i usr:[gli]
$ cp *link.opt usr:[gli]
$ cp gksdefs.p* usr:[gli]
$ echo "Installing GLI help file"
$ cp gli.hlb usr:[gli]
$ echo "Installing GLI Hershey fonts"
$ cp [-.font]gksfont.dat usr:[gli]
$ echo "Installing GLI example scripts"
$ cp [-.demo]*.* usr:[gli.demo]
$ cp [-.demo.images]*.* usr:[gli.demo.images]
