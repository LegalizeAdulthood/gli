      SHELL = /bin/sh
       DEST = /usr/local/gli
    DESTDIR = $(DEST)
         CC = cc
     CFLAGS =
        F77 = f77
   F77FLAGS =
         PC = pc
      SEGLD = cc
         LD = ld
    LDFLAGS =
         AR = ar
     RANLIB = ranlib
         CD = cd
      CHMOD = chmod
      MKDIR = mkdir
         CP = cp -p
    INSTALL = cp -p
         RM = rm
   INCLUDES =
   XLIBPATH =
      XLIBS = $(XLIBPATH) -lXt -lX11
  XMLIBPATH = 
     XMLIBS = $(XMLIBPATH) -lMrm -lXm
      ZLIBS =
    F77LIBS =
       LIBS = -lm
     UILDIR = /usr/include/X11/uil
        UIL = uil
      MOTIF =
    TERMCAP =
     GRSOFT =
     GRLIBS =
    GKSLIBS = /usr/local/gr/lib/libgks.a
       GLI5 = 
        TCL =
    TCLLIBS =
        DPS = 
 DPSLIBPATH =
    DPSLIBS = $(DPSLIBPATH) -ldps
     SHLIBS =
    SOFLAGS = -soname $(DEST)/$@
    GLIRPCD =
        DIR = src
    VERSION = 4.5.30
       HOST = pgi-jcns-ta:/WebServer/Documents/pub/gli/

    GLIOBJS = gligus.o gligr.o gligks.o glisimp.o glisight.o gliimage.o \
	      glixui.o glicgm.o glirpc.o gkstest.o
  SIGHTOBJS = sightmai.o sight.o sightdev.o
  IMAGEOBJS = imagemai.o image.o imageuti.o imageapp.o imagedev.o
    GUSOBJS = gus.o gustext.o gusauto.o gusplo10.o
    GKSOBJS = gksio.o gkscbnd.o gks.o gksinq.o gkserror.o gksroot.o gksmisc.o \
	      gksdidd.o gksdps.o gksdtek.o gksdtek2.o gksdx11.o gksdwin.o \
	      gksdcgm.o gksdpbm.o gksdwiss.o gkswiss.o gksdhpgl.o gksdvt.o \
	      gksdgksm.o gksdpdf.o gksforio.o gksafm.o compress.o
   CRTLOBJS = formdrv.o command.o terminal.o function.o variable.o symbol.o \
	      string.o math.o help.o system.o com.o
   FRTLOBJS = contour.o spline.o pixel.o gridit.o

.SUFFIXES: .a .o .uid .c .f .uil

.uil.uid:
	$(UIL) -o $@ $< -I$(UILDIR)
.c.o:
	$(CC) -c $(CFLAGS) $(INCLUDES) $<
.f.o:
	$(F77) -c $(F77FLAGS) $<

default:
	@if [ `pwd` = $(DESTDIR) ]; then \
	echo Cannot build distribution in $(DESTDIR) ; else \
	make `./Config` ; fi

usage:	@echo "Can't obtain system information."; \
"Usage: make [aix|alpha|cray|cygwin|darwin|hpux|irix|mips|netbsd|pclinux|sun]"

all:
	./configure
	make install
	make distclean

aix:
	@make targets MACHINE="aix" LD="xlf" \
	SHLIBS="libgks.so" SOFLAGS="-bM:SRE -bE:gks.exp -e gopks" \
	MOTIF="motif" INCLUDES="-I/usr/local/tcl8.3/include" \
	CFLAGS="-Daix -DMOTIF -DRPC -DGRSOFT -DTCL -DDPS" \
	F77="xlf" F77FLAGS="-NQ20000" RANLIB="ar ts" SEGLD="xlf" \
	TCLLIBS="-L/usr/local/tcl8.3/lib -ltk8.3 -ltcl8.3 -lld" \
	GRLIBS="-L../../gr/src -lgr" GLIRPCD="glirpcd" \
	UIL="/usr/bin/X11/uil" UILDIR="/usr/include/uil" \
	DPS="psprint" DPSLIBPATH="-L/usr/lpp/DPS/lib" \
	XLIBS="-lXt -lXext -lX11" XMLIBS="-lMrm -lXm"
aix-lite:
	@make targets MACHINE="aix" LD="xlf" \
	MOTIF="motif" INCLUDES="-I/usr/local/include" CFLAGS="-Daix -DMOTIF" \
	F77="xlf" F77FLAGS="-NQ20000" RANLIB="ar ts" SEGLD="xlf" \
	UIL="/usr/bin/X11/uil" UILDIR="/usr/include/uil" \
	XLIBS="-lXt -lXext -lX11" XMLIBS="-lMrm -lXm"
aix-nagware:
	@make targets MACHINE="aix" \
	MOTIF="motif" INCLUDES="-I/usr/local/include" \
	CFLAGS="-Daix -DMOTIF -DNAGware" \
	F77="nagf95 -w=obs" F77FLAGS="-O -ieee=full" SEGLD="nagf95" \
	UIL="/usr/bin/X11/uil" UILDIR="/usr/include/uil" \
	XLIBS="-lXt -lXext -lX11" XMLIBS="-lMrm -lXm"
alpha:
	@make targets SHLIBS="libcrtl.so libgks.so libgus.so" LD="ld -shared" \
	MOTIF="motif" INCLUDES="-I/usr/local/include" \
	CFLAGS="-O -DMOTIF -DRPC -DGRSOFT -DTCL -DDPS -DDXML -DXSHM -DZLIB" \
	RANLIB="ar ts" F77FLAGS="-O -automatic" ZLIBS="-lz" \
	F77LIBS="-lUfor -lfor -lFutil -lots" LIBS="-lm -lc" \
	TCLLIBS="-L/usr/local/lib -ltk -ltcl" \
	GRLIBS="-L../../gr/src -lgr" GLIRPCD="glirpcd" \
	XLIBS="-lXt -lXext -lX11" DPS="psprint"
alpha-lite:
	@make targets \
	CFLAGS="-O" RANLIB="ar ts" F77FLAGS="-O -automatic" \
	F77LIBS="-lUfor -lfor -lFutil -lots" LIBS="-lm -lc"
alpha-gcc:
	@make targets \
	CC="gcc" CFLAGS="-O -DTCL -pedantic -ansi -DNeedFunctionPrototypes" \
	RANLIB="ar ts" F77="g77" F77LIBS="-lf2c" XMLIBS="" LIBS="-lm -lc" \
	TCLLIBS="-L/usr/local/lib -ltk -ltcl"
lint:
	@make alpha-gcc >/dev/null
sn1011: sn2401
sn2401:
	@make targets MACHINE="cray" CFLAGS="-DRPC -DGRSOFT" \
	GRLIBS="-L../../gr/src -lgr" RANLIB="ar t" SEGLD="segldr" \
	F77="cf77" F77FLAGS="-Wf\"-a stack -o off -e v -dp\"" XMLIBS=""
sn6308: cray
sn7024: cray
sn7104: cray 
sn7119: cray 
sn9063: cray
sn9612: cray
cray:
	@make targets MACHINE="cray" CFLAGS="-DRPC -DTCL" \
	F77="f90" F77FLAGS="-en -dp -O3" RANLIB="ar t" SEGLD="f90" XMLIBS="" \
	LIBS="-ltk4.0 -ltcl7.4 -lICE -lSM"
cygwin32: cygwin
cygwin:
	@make targets MACHINE="PC" CC="gcc" CFLAGS="-DTCL" \
	SEGLD="g77" F77="g77" XLIBS="-lXt -lX11 -lICE -lSM" XMLIBS="" \
	TCLLIBS="-ltk80 -ltcl80" LIBS="-ladvapi32 -luser32 -lgdi32 -lcomdlg32"
#	CFLAGS="-mno-cygwin -DTCL"
mingw32: win32
win32:
	@make targets MACHINE="PC" CC="gcc" CFLAGS="-DNO_X11" \
	SEGLD="g77" F77="g77" XMLIBS="" XLIBS="" \
	LIBS="-ladvapi32 -luser32 -lgdi32 -lcomdlg32"
hpux:
	@make targets MACHINE="hpux" MOTIF="motif" \
	CFLAGS="-DMOTIF -Wp,-H200000" \
	INCLUDES="-I/usr/include/X11R5 -I/usr/include/Motif1.2" \
	XLIBPATH="-L/usr/lib/X11R5" F77FLAGS="-K" LIBS="-lcl -lm -lc" \
	XMLIBS="-L/usr/lib/Motif1.2 -lMrm -lXm -lPW" SEGLD="cc" \
	UIL="/usr/bin/X11/uil" UILDIR="/usr/include/Motif1.2/uil"
### pre HP-UX 10.01 ### SEGLD="cc -Wl,-a,archive"
hp9000s800:
	@make targets MACHINE="hpux" MOTIF="motif" \
	CFLAGS="-DMOTIF -Wp,-H200000" \
	INCLUDES="-I/usr/include/X11R4 -I/usr/include/Motif1.1" \
	XLIBPATH="-L/usr/lib/X11R4" F77FLAGS="-K +e" LIBS="-lcl -lm -lc" \
	XMLIBS="-L/usr/lib/Motif1.1 -lMrm -lXm -lPW" SEGLD="cc" \
	UIL="/usr/bin/X11/uil" UILDIR="/usr/include/Motif1.1/Xm"
i386: pclinux
i486: pclinux
i586: pclinux
i686: pclinux
x86_64: pclinux
linux: pclinux
pclinux:
	@make targets MACHINE="pclinux" MOTIF="motif" GLI5="gli5" \
	SHLIBS="libgks.so libgus.so" LD="g77 -shared" SOFLAGS="" SEGLD="g77" \
	CFLAGS="-fPIC -DMOTIF -DRPC -DTCL -DZLIB" F77="g77" F77FLAGS="-fPIC" \
	INCLUDES="-I/usr/X11R6/include -I/usr/local/dt/include" \
	GKSLIBS="/usr/local/gr/lib/libGKS.a" TCLLIBS="-ltk -ltcl -ldl" \
	XMLIBS="-L/usr/local/dt/lib -lMrm -lXm" \
	XLIBS="-L/usr/X11R6/lib -lXt -lXext -lX11 -lXft" \
	ZLIBS="-lz" LDFLAGS="-u MAIN__" LIBS="-lpthread -lcrypt -lm" \
	GLIRPCD="glirpcd" \
	UIL="/usr/local/dt/bin/uil" UILDIR="/usr/local/dt/include/uil"
pclinux32:
	@make targets MACHINE="pclinux" \
	SHLIBS="libgks.so libgus.so" LD="g77 -shared" SOFLAGS="" SEGLD="g77" \
	CFLAGS="-m32 -fPIC -DZLIB" F77="g77 -m32" F77FLAGS="-fPIC" \
	INCLUDES="-I/usr/X11R6/include" TCLLIBS="-ltk -ltcl -ldl" \
	XMLIBS="" XLIBS="-L/usr/X11R6/lib -lXt -lXext -lX11" \
	ZLIBS="-lz" LDFLAGS="-m32 -u MAIN__" LIBS="-lcrypt -lm"
pclinux-intel:
	@make targets MACHINE="pclinux" MOTIF="motif" GLI5="gli5" \
	SHLIBS="libgks.so libgus.so" LD="icc -shared" SOFLAGS="" SEGLD="icc" \
	CC="icc" CFLAGS="-fPIC -DMOTIF -DRPC -DTCL -DZLIB" \
	F77="ifort" F77FLAGS="-fPIC" \
	F77LIBS="-L/usr/local/intel/lib/intel64 -lifcore -lintlc -limf" \
	LDFLAGS="-Wl,-rpath=/usr/local/intel/lib/intel64" \
	INCLUDES="-I/usr/X11R6/include -I/usr/local/dt/include" \
	GKSLIBS="/usr/local/gr/lib/libGKS.a" TCLLIBS="-ltk -ltcl -ldl" \
	XMLIBS="-L/usr/local/dt/lib -lMrm -lXm" \
	XLIBS="-L/usr/X11R6/lib -lXt -lXext -lX11 -lXft" \
	ZLIBS="-lz" LIBS="-lpthread -lcrypt -lm" \
	GLIRPCD="glirpcd" \
	UIL="/usr/local/dt/bin/uil" UILDIR="/usr/local/dt/include/uil"
# IMPORTANT: optimization (cc, g77) causes problems with the PostScript
#            driver on RedHat 6.2
pclinux-pgi:
	@make targets MACHINE="pclinux" \
	CFLAGS="-DRPC -DTCL -DZLIB" F77="pgf77" F77FLAGS="" SEGLD="pgf77" \
	INCLUDES="-I/usr/X11R6/include" TCLLIBS="-ltk -ltcl -ldl" \
	XMLIBS="" XLIBS="-L/usr/X11R6/lib -lXt -lXext -lX11" \
	ZLIBS="-lz" LDFLAGS="-M nomain" LIBS="-lcrypt -lm" LDFLAGS="-Mnomain"
pclinux-alpha:
	@make targets MACHINE="pclinux" \
	SHLIBS="libgks.so libgus.so" LD="ld -shared" LIBS="-lcrypt -lm" \
	CC="ccc" CFLAGS="-DRPC -DTCL -DZLIB" F77="fort" \
	F77LIBS="-lUfor -lfor -lFutil -lots" SEGLD="ccc" \
	INCLUDES="-I/usr/X11R6/include" TCLLIBS="-ltk -ltcl -ldl" \
	XMLIBS="" XLIBS="-L/usr/X11R6/lib -lXt -lXext -lX11" \
	ZLIBS="-lz" LDFLAGS="-u MAIN__" GLIRPCD="glirpcd"
pclinux-nomotif:
	@make targets MACHINE="pclinux" \
	SHLIBS="libgks.so libgus.so" LD="cc -shared" SOFLAGS="" \
	CFLAGS="-DRPC -DTCL -DZLIB" F77="g77" F77LIBS="-lg2c" \
	INCLUDES="-I/usr/X11R6/include" TCLLIBS="-ltk -ltcl -ldl" \
	XMLIBS="" XLIBS="-L/usr/X11R6/lib -lXt -lXext -lX11" \
	ZLIBS="-lz" LDFLAGS="-u MAIN__" LIBS="-lcrypt -lm" GLIRPCD="glirpcd"
netbsd:
	@make targets MACHINE="netbsd" \
	CFLAGS="-DRPC -DTCL -DZLIB" F77="g77" F77LIBS="-lg2c" \
	INCLUDES="-I/usr/X11R6/include -I/usr/pkg/include/tk8.0 -I/usr/pkg/include/tcl8.0" \
	LIBS="-lcrypt -lm" TCLLIBS="-L/usr/pkg/lib -ltk80 -ltcl80" \
	XMLIBS="" XLIBS="-L/usr/X11R6/lib -lXt -lXext -lX11" \
	ZLIBS="-lz"
darwin:
	@make targets LANG="" MACHINE="darwin" MOTIF="motif" GLI5="gli5" \
	CFLAGS="-O -DMOTIF -DRPC -DTCL -DZLIB -DXFT" F77FLAGS="-O" \
	F77="gfortran" SEGLD="gfortran" \
	INCLUDES="-I/usr/X11R6/include -I/usr/local/dt/include" \
	XLIBS="-L/usr/X11R6/lib -lXt -lXext -lX11 -lXft -lfontconfig" ZLIBS="-lz" \
	XMLIBS="-L/usr/local/dt/lib -lMrm -lXm" \
	LIBS="-framework Carbon -framework Tk -framework Tcl" \
	UIL="/usr/local/dt/bin/uil" UILDIR="/usr/local/dt/include/uil"
darwin-intel:
	@make targets LANG="" MACHINE="darwin" MOTIF="motif" GLI5="gli5" \
	CFLAGS="-O -DMOTIF -DRPC -DTCL -DZLIB -DXFT" F77FLAGS="-O" \
	CC="icc" F77="ifort" SEGLD="ifort" \
	LDFLAGS="-nofor_main" \
	INCLUDES="-I/usr/X11R6/include -I/usr/local/dt/include" \
	XLIBS="-L/usr/X11R6/lib -lXt -lXext -lX11  -lXft -lfontconfig" ZLIBS="-lz" \
	XMLIBS="-L/usr/local/dt/lib -lMrm -lXm" \
	LIBS="-framework Carbon -framework Tk -framework Tcl" \
	UIL="/usr/local/dt/bin/uil" UILDIR="/usr/local/dt/include/uil"
mips:
	@make targets MOTIF="motif" INCLUDES="-I/usr/local/include" \
	CFLAGS="-O -Olimit 1000 -DMOTIF -DRPC -DTCL -DDPS" \
	F77FLAGS="-O" SEGLD="$(F77)" TCLLIBS="-ltk -ltcl" GLIRPCD="glirpcd" \
	XLIBS="-lXt -lXext -lX11" DPS="psprint" LDFLAGS="-G 8"
irix:
	@make targets MACHINE="irix" INCLUDES="-I/usr/local/include" \
	SHLIBS="libgks.so libgus.so" LD="ld -shared" \
	MOTIF="motif" CFLAGS="-DMOTIF -DRPC -DGRSOFT -DTCL" \
	RANLIB="ar ts" XMLIBS="-lMrm -lXm -lPW" \
	LIBS="-lsun -lm" SEGLD="f77" GRLIBS="-L../../gr/src -lgr" \
	TCLLIBS="-ltk -ltcl" UIL="/usr/bin/X11/uil" UILDIR="/usr/include/uil" \
	GLIRPCD="glirpcd"
sunos:
	@make targets MACHINE="sun" \
	MOTIF="motif" CFLAGS="-DMOTIF -DRPC -DTCL" TCLLIBS="-ltk -ltcl" \
	INCLUDES="-I/usr/dt/include -I/usr/openwin/include -I/usr/local/include" \
	RANLIB="ar ts" XLIBPATH="-L/usr/dt/lib -L/usr/openwin/lib" \
	UIL="/usr/dt/bin/uil" UILDIR="/usr/dt/include/uil" \
	LIBS="-lsocket -ldl -lnsl -lm" \
	SEGLD="$(F77)" LDFLAGS="-xlic_lib=sunperf" GLIRPCD="glirpcd"
sun4: sun
sun:
	@make targets MACHINE="sun" SHLIBS="libgks.so libgus.so" SOFLAGS="" \
	CFLAGS="-DRPC -DGRSOFT -DTCL -I/usr/openwin/include" \
	XMLIBS="" LDFLAGS="-Bstatic -L/usr/local/fortran/SC1.0" \
	GRLIBS="-L../../gr/src -lgr" TCLLIBS="-ltk -ltcl" \
	F77LIBS="-lF77" LIBS="-lm" GLIRPCD="glirpcd"

targets: $(MOTIF) gli gligksm $(DPS) cgmview $(SHLIBS) $(GLIRPCD) $(GLI5)

motif: sight.uid aeliter.uid deliter.uid image.uid cgmview.uid

gli: gli.o \
     libgli.a libsight.a libimage.a libgus.a libcrtl.a libfrtl.a libgks.a
	$(SEGLD) -o gli gli.o \
	libgli.a libsight.a libimage.a libgus.a libcrtl.a libfrtl.a \
	$(GRLIBS) libgks.a \
	$(LDFLAGS) $(ZLIBS) $(TCLLIBS) $(XMLIBS) $(XLIBS) $(F77LIBS) $(LIBS)

gli5: gli.o \
     libgli.a libsight.a libimage.a libgus.a libcrtl.a libfrtl.a
	$(SEGLD) -o gli5 gli.o \
	libgli.a libsight.a libimage.a libgus.a libcrtl.a libfrtl.a \
	$(GRLIBS) $(GKSLIBS) \
	$(LDFLAGS) $(ZLIBS) $(TCLLIBS) $(XMLIBS) $(XLIBS) $(F77LIBS) $(LIBS)

gligksm: gligksm.o libgks.a
	$(SEGLD) -o gligksm gligksm.o libcrtl.a libgks.a \
	$(LDFLAGS) $(ZLIBS) $(XLIBS) $(F77LIBS) $(LIBS)

glidecgks: gli.o \
     libgli.a libsight.a libimage.a libgus.a libcrtl.a libfrtl.a
	$(SEGLD) -o glidecgks gli.o \
	libgli.a libsight.a libimage.a libgus.a libcrtl.a libfrtl.a \
	-lgr -lGKSforbnd -lGKS \
	-ltk -ltcl -lMrm -lDXm -lXm -lXt -lXext -lX11 -lcda -ldnet_stub -llmf \
	-lUfor -lfor -lFutil -lots -lm -lc

psprint: psprint.o compress.o
	$(CC) -o psprint psprint.o compress.o \
	$(LDFLAGS) $(DPSLIBS) $(XLIBS) $(LIBS)

libgli.a: $(GLIOBJS)
	$(AR) crv $@ $?
	$(RANLIB) $@

libsight.a: $(SIGHTOBJS)
	$(AR) crv $@ $?
	$(RANLIB) $@

libimage.a: $(IMAGEOBJS)
	$(AR) crv $@ $?
	$(RANLIB) $@

libgus.a: $(GUSOBJS)
	$(AR) crv $@ $?
	$(RANLIB) $@
libgus.so: $(GUSOBJS)
	$(LD) -o $@ $(SOFLAGS) $(GUSOBJS) \
	-L./ -lcrtl -lfrtl -lgks \
	$(LDFLAGS) $(TCLLIBS) $(XLIBS) $(F77LIBS) $(LIBS)

libgks.a: $(GKSOBJS)
	$(AR) crv $@ $?
	$(RANLIB) $@
libgks.so: $(GKSOBJS)
	$(LD) -o $@ $(SOFLAGS) $(GKSOBJS) \
	$(LDFLAGS) $(ZLIBS) $(XLIBS) $(F77LIBS) $(LIBS)

libcrtl.a: $(CRTLOBJS)
	$(AR) crv $@ $?
	$(RANLIB) $@
libcrtl.so: $(CRTLOBJS)
	$(LD) -o $@ $(SOFLAGS) $(CRTLOBJS) \
	$(LDFLAGS) $(TCLLIBS) $(LIBS)

libfrtl.a: $(FRTLOBJS)
	$(AR) crv $@ $?
	$(RANLIB) $@

gridit.o: gridit.f
	$(F77) -c $<

cgmview.o: cgmview.c
	$(CC) -c $(CFLAGS) $< $(INCLUDES)

cgmview: cgmview.o \
	libgli.a libsight.a libgus.a libcrtl.a libfrtl.a libgks.a
	$(SEGLD) -o cgmview cgmview.o \
	libgli.a libsight.a libimage.a libgus.a libcrtl.a libfrtl.a libgks.a \
	$(LDFLAGS) $(ZLIBS) $(XMLIBS) $(XLIBS) $(F77LIBS) $(LIBS)

prof: gli.prof

gli.prof: gli.pixie
	gli.pixie
	prof -pixie gli >gli.prof
	more gli.prof
	touch gli.pixie

gli.pixie: gli
	pixie -o gli.pixie gli

glirpcd: glirpcd.o libgli.a libcrtl.a
	$(CC) -o glirpcd glirpcd.o libgli.a libcrtl.a \
	$(LDFLAGS) $(LIBS)

install: default
	@make `./Config`install

aixinstall: dist

alphainstall: dist

sn2401install: crayinstall
crayinstall:
	@make dist "CP=cp" "INSTALL=cp"

hpuxinstall: dist

i386install: pclinuxinstall
i486install: pclinuxinstall
i586install: pclinuxinstall
i686install: pclinuxinstall
x86_64install: pclinuxinstall
pclinuxinstall:
	@make dist "INSTALL=install -c -s"

darwininstall:
	@make dist "INSTALL=install -c -s"

netbsdinstall:
	@make dist "INSTALL=install -c -s"

mipsinstall:
	@make dist "INSTALL=install -c -s"

irixinstall:
	@make dist "CP=cp" "INSTALL=cp"

sun4install: suninstall
suninstall:
	@make dist "INSTALL=install -s"

modes:
	@$(CHMOD) 644 *
	@if [ -d ../font ]; then $(CHMOD) 644 ../font/*.*; else true; fi
	@if [ -d ../demo ]; then $(CHMOD) 644 ../demo/*.*; else true; fi
	@if [ -d ../demo/images ]; then $(CHMOD) 644 ../demo/images/*; \
	else true; fi
	@if [ -d ../tcl ]; then $(CHMOD) -R 755 ../tcl; else true; fi
	@$(CHMOD) 700 Config gks win pas
	@$(CHMOD) 600 *.c *.f *.uil
	@$(CHMOD) 644 glirpcd.c
	@$(CHMOD) 755 Config *.sh *.csh *.ksh config* install-sh
	@if [ -f gli ]; then \
	$(CHMOD) 755 gli; \
	else true; fi
	@if [ -f gligksm ]; then $(CHMOD) 755 gligksm; else true; fi
	@if [ -f psprint ]; then $(CHMOD) 755 psprint; else true; fi
	@if [ -f cgmview ]; then $(CHMOD) 755 cgmview; else true; fi
	@if [ -f glirpcd ]; then $(CHMOD) 755 glirpcd; else true; fi
	@if [ -f sight.uid ]; then $(CHMOD) 644 *.uid; else true; fi

dist: modes
	@if [ -d $(DESTDIR) ]; then \
	echo "Directory $(DESTDIR) already exists"; \
	else $(MKDIR) $(DESTDIR); $(CHMOD) 755 $(DESTDIR); fi 
	@if [ -d $(DESTDIR)/demo ]; then \
	echo "Directory $(DESTDIR)/demo already exists"; \
	else $(MKDIR) $(DESTDIR)/demo; $(CHMOD) 755 $(DESTDIR)/demo; fi
	@if [ -d $(DESTDIR)/demo/images ]; then \
	echo "Directory $(DESTDIR)/demo/images already exists"; \
	else $(MKDIR) $(DESTDIR)/demo/images; \
	$(CHMOD) 755 $(DESTDIR)/demo/images; fi
	@if [ -d $(DESTDIR)/tcl ]; then \
	echo "Directory $(DESTDIR)/tcl already exists"; \
	else $(MKDIR) $(DESTDIR)/tcl; $(CHMOD) 755 $(DESTDIR)/tcl; fi
	@if [ -f sight.uid ]; then \
	echo "Installing GLI Motif user interfaces"; $(CP) *.uid $(DESTDIR); \
	else true; fi
	@echo "Installing GLI executables"
	@$(INSTALL) gli $(DESTDIR)
	@if [ -f gli5 ]; then \
	$(INSTALL) gli5 $(DESTDIR); \
	else true; fi
	@$(INSTALL) gligksm $(DESTDIR)
	@$(CP) gli.o $(DESTDIR)
	@if [ -f psprint ]; then \
	$(INSTALL) psprint $(DESTDIR); \
	else true; fi
	@$(CP) *.sh *.csh *.ksh $(DESTDIR)
	@$(INSTALL) cgmview $(DESTDIR)
	@$(CP) cgmview.o $(DESTDIR)
	@if [ -f glirpcd ]; then \
	$(INSTALL) glirpcd $(DESTDIR); \
	$(CP) glirpcd.c $(DESTDIR); \
	else true; fi
	@if [ -f gli5 ]; then \
	$(INSTALL) gli5 $(DESTDIR); \
	else true; fi
	@echo "Installing GLI archive files"
	@$(CP) lib*.a* $(DESTDIR)
	@if [ -f libgks.so ]; then \
	echo "Installing GLI shared object libraries"; \
	$(CP) lib*.so $(DESTDIR); \
	else true; fi
	@echo "Installing GLI environment files"
	@$(CP) Readme $(DESTDIR)
	@$(CP) *.gli $(DESTDIR)
	@$(CP) *.fdv $(DESTDIR)
	@$(CP) *.h $(DESTDIR)
	@$(CP) *.i $(DESTDIR)
	@echo "Installing GLI help file"
	@$(CP) gli.hlp $(DESTDIR)
	@if [ -d ../font ]; then \
	echo "Installing GLI Hershey fonts"; \
	$(CP) ../font/gksfont.dat $(DESTDIR); \
	else true; fi
	@if [ -d ../demo ]; then \
	echo "Installing GLI example scripts"; \
	$(CP) ../demo/*.* $(DESTDIR)/demo; \
	$(CP) ../demo/images/*.* $(DESTDIR)/demo/images; \
	else true; fi
	@if [ -d ../tcl ]; then \
	echo "Installing Tcl/Tk environment files"; \
	$(CP) -p -r ../tcl $(DESTDIR); else true; fi

rsrc:
	Rez -t APPL -o $(DESTDIR)/gli5 gks.rsrc

profclean:
	$(RM) -f gli.Addrs gli.Counts gli.pixie gli.prof

pascal:
	@cd pas; make
pascalclean:
	@cd pas; make clean

clean: profclean pascalclean
	$(RM) -f gli gligksm glidecgks psprint cgmview glirpcd gli5 \
	so_locations
	$(RM) -f *.exe
	$(RM) -f *.o
	$(RM) -f lib*.a* lib*.so
	$(RM) -f *.uid
	$(RM) -f *.bak *.jou *.sav *.log

distclean: clean
	$(RM) -f Makefile config.status config.cache config.log
	$(CP) -p Makefile.std Makefile

ftp: clean
	cd ../..; tar cf gli-$(VERSION).tar gli/src gli/demo gli/font gli/tcl
	gzip ../../gli-$(VERSION).tar
	cd ../..; scp -p gli-$(VERSION).tar.gz $(HOST)
	rm -f ../../gli-$(VERSION).tar.gz
	scp -p Readme $(HOST)

depend:
	makedepend -Y -- cgmview.c command.c compress.c contour.f formdrv.c \
                      function.c gks.f gksafm.c gkscbnd.c gksdcgm.c gksdhpgl.f \
                      gksdidd.f gksdgksm.c gksdpbm.f gksdps.c gksdtek.f \
                      gksdtek2.f gksdwin.c gksdvt.f gksdwiss.f gksdx11.c \
                      gksdpdf.c gkserror.f gksforio.f gksinq.f gksio.c \
                      gksmisc.c gksroot.f gkswiss.c gli.c glicgm.c gligks.c \
                      gligr.c gligus.c gliimage.c gligksm.c glirpc.c \
                      glitest.f glirpcd.c glisight.c glisimp.c glixui.c \
                      gridit.f gus.c gusauto.c gusplo10.c gustext.c help.c \
                      image.c imageapp.c imagedev.c imagemai.c imageuti.c \
                      math.c pixel.c psprint.c sight.c sightdev.c \
                      sightmai.c spline.c string.c symbol.c system.c \
                      terminal.c variable.c 2> /dev/null

# DO NOT DELETE THIS LINE -- make depend depends on it.

cgmview.o: terminal.h gksdefs.h
command.o: system.h strlib.h symbol.h variable.h function.h terminal.h
command.o: command.h
formdrv.o: system.h symbol.h terminal.h strlib.h variable.h formdrv.h
function.o: strlib.h symbol.h variable.h function.h mathlib.h system.h
gksafm.o: gksdefs.h
gkscbnd.o: gks.h
gksdcgm.o: cgm.h gksdefs.h
gksdgksm.o: gksdefs.h
gksdps.o: gksdefs.h
gksdwin.o: gksdefs.h
gksdx11.o: gksdefs.h icon.bm
gksdpdf.o: gksdefs.h
gksio.o: gksdefs.h system.h
gksmisc.o: gksdefs.h
gkswiss.o: gksdefs.h
gli.o: system.h strlib.h mathlib.h symbol.h variable.h function.h terminal.h
gli.o: command.h formdrv.h gksdefs.h gus.h image.h sight.h frtl.h
glicgm.o: glicgm.h gksdefs.h image.h sight.h
gligks.o: system.h terminal.h variable.h function.h command.h strlib.h
gligks.o: symbol.h gksdefs.h
gligr.o: system.h terminal.h variable.h command.h grsoft.h
gligus.o: system.h mathlib.h strlib.h variable.h function.h terminal.h
gligus.o: command.h formdrv.h gksdefs.h gus.h
gliimage.o: system.h strlib.h variable.h function.h terminal.h command.h
gliimage.o: gksdefs.h image.h
gligksm.o: terminal.h gksdefs.h
glisight.o: system.h terminal.h function.h variable.h command.h strlib.h
glisight.o: symbol.h gksdefs.h gus.h image.h sight.h
glisimp.o: system.h mathlib.h strlib.h terminal.h variable.h command.h
glisimp.o: gksdefs.h gus.h
glixui.o: terminal.h
gus.o: system.h strlib.h symbol.h variable.h function.h terminal.h command.h
gus.o: formdrv.h mathlib.h gus.h gksdefs.h frtl.h
gusauto.o: variable.h terminal.h formdrv.h command.h strlib.h mathlib.h
gusauto.o: system.h gus.h gksdefs.h
gusplo10.o: gksdefs.h gus.h
gustext.o: terminal.h gksdefs.h gus.h
help.o: strlib.h terminal.h
image.o: system.h strlib.h image.h
imageapp.o: system.h strlib.h mathlib.h image.h
imagedev.o: system.h strlib.h image.h gksdefs.h
imagemai.o: terminal.h
imageuti.o: system.h strlib.h image.h
math.o: mathlib.h terminal.h
pixel.o: gksdefs.h
psprint.o: gksdefs.h
sight.o: system.h gksdefs.h gus.h strlib.h symbol.h variable.h terminal.h
sight.o: image.h sight.h
sightdev.o: gksdefs.h gus.h symbol.h image.h sight.h
sightmai.o: terminal.h
string.o: strlib.h system.h
symbol.o: variable.h function.h symbol.h strlib.h system.h
system.o: system.h
terminal.o: terminal.h strlib.h system.h
variable.o: strlib.h function.h variable.h system.h
