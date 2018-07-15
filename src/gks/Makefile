# 
#  Copyright @ 1984 - 1996   Josef Heinen
# 
#  Permission to use, copy, and distribute this software and its
#  documentation for any purpose with or without fee is hereby granted,
#  provided that the above copyright notice appear in all copies and
#  that both that copyright notice and this permission notice appear
#  in supporting documentation.
# 
#  Permission to modify the software is granted, but not the right to
#  distribute the modified code.  Modifications are to be distributed
#  as patches to released version.
# 
#  This software is provided "as is" without express or implied warranty.
# 
#  Send your comments or suggestions to
#   J.Heinen@kfa-juelich.de.
# 
# 

       CC = cc
   CFLAGS = 
      F77 = f77
 F77FLAGS =
       AR = ar
       RM = rm
   RANLIB = ranlib
    SEGLD = f77
       LD = ld
  LDFLAGS = 
    XLIBS = -lXt -lX11
 XLIBPATH = 
     LIBS = 
  INSTALL = install
  DESTDIR = /usr/local/lib
     COPY = mcopy -tnvm
  COPYBIN = mcopy -nvm
   SHLIBS =
  SOFLAGS = -soname $(DESTDIR)/libgks.so

     OBJS = gksio.o gkscbnd.o gks.o gksinq.o gkserror.o gksroot.o gksmisc.o \
            gksdidd.o gksdps.o gksdtek.o gksdtek2.o gksdx11.o gksduis.o \
            gksdcgm.o gksdwiss.o gkswiss.o gksdhpgl.o gksdvt.o gksdpbm.o \
            gksdgksm.o gksdpdf.o gksforio.o gksafm.o compress.o

.SUFFIXES: .a .o .c .f

.c.o:
	$(CC) -c $(CFLAGS) $<
.f.o:
	$(F77) -c $(F77FLAGS) $<

default:
	@make mod
	@make `./Config`
	@chmod 444 libgks.* gksfont.dat

mod:
	@chmod 644 *
	@if [ -f int86.obj ]; then chmod 444 int86.obj; else true; fi
	@chmod 555 Config
	@if [ -d win ]; then chmod 700 win; else true; fi
	@if [ -d win ]; then chmod 644 win/*; else true; fi
	@if [ -d dos ]; then chmod 700 dos; else true; fi
	@if [ -d dos ]; then chmod 644 dos/*; else true; fi

usage:  @echo "Can't obtain system information."; \
"Usage: make [aix|alpha|cray|hpux|irix|mips|pclinux|netbsd|sun]"

aix:
	@make all CFLAGS="-Daix" F77="xlf" F77FLAGS="-NQ20000" \
	RANLIB="ar ts" LD="xlf" \
	SHLIBS="libgks.so" SOFLAGS="-bM:SRE -bE:gks.exp -e gopks"
aix-nagware:
	@make all CFLAGS="-DNAGware" F77="nagf95" F77FLAGS="-w=obs" \
	RANLIB="ar ts" SEGLD="nagf95"
alpha:
	@make all SHLIBS="libgks.so" LD="ld -shared" \
	LIBS="-lUfor -lfor -lFutil -lots -lm -lc"
darwin:
	@make all CFLAGS="-I/usr/X11R6/include" XLIBPATH="-L/usr/X11R6/lib" \
	F77="gfortran" SEGLD="gfortran"
sn1011: sn2401
sn2401:
	@make all F77="cf77" F77FLAGS="-Wf\"-a stack -o off -e v -dp\"" \
	RANLIB="ar t"
sn6308: cray
sn7024: cray
sn7104: cray 
sn7119: cray 
sn9612: cray
cray:
	@make all F77="f90" F77FLAGS="-en -dp -O3" RANLIB="ar t" SEGLD="f90" \
	XLIBS="-lXt -lX11 -lSM -lICE"
hpux:
	@make all CFLAGS="-Wp,-H200000 -I/usr/include/X11R5" \
	SEGLD="ld /lib/crt0.o" XLIBPATH="-L/usr/lib/X11R5" LIBS="-lcl -lm -lc"
hpux-nagware:
	@make all CFLAGS="-Wp,-H200000 -Uhpux -I/usr/include/X11R5" \
	F77="f95" F77FLAGS="-w=obs" SEGLD="f95" XLIBPATH="-L/usr/lib/X11R5" \
	LIBS="-L/opt/fortran90/lib -lF90 -lcl -lm -lc"
irix:
	@make all RANLIB="ar ts" SHLIBS="libgks.so" LD="ld -shared"
i386: pclinux
i486: pclinux
i586: pclinux
i686: pclinux
x86_64: pclinux
pclinux:
	@make all CFLAGS="-I/usr/X11R6/include" XLIBPATH="-L/usr/X11R6/lib" \
	F77="g77" SEGLD="g77"
pclinux-pgi:
	@make all CFLAGS="-I/usr/X11R6/include" XLIBPATH="-L/usr/X11R6/lib" \
	F77="pgf77" SEGLD="pgf77"
netbsd:
	@make all CFLAGS="-I/usr/X11R6/include" XLIBPATH="-L/usr/X11R6/lib"
mips:
	@make all
sunos:
	@make all CFLAGS="-I/usr/openwin/include" RANLIB="ar ts" \
	SHLIBS="libgks.so" SOFLAGS="-G" XLIBPATH="-L/usr/openwin/lib" \
	LIBS="-lnsl"
sun4: sun
sun:
	@make all CFLAGS="-I/usr/openwin/include -DBSD" LDFLAGS="-Bstatic" \
	SHLIBS="libgks.so" SOFLAGS="" XLIBPATH="-L/usr/openwin/lib"

all: libgks.a $(SHLIBS) demo

libgks.a: $(OBJS)
	$(AR) rv $@ $?
	$(RANLIB) $@
libgks.so: $(OBJS)
	$(LD) -o $@ $(SOFLAGS) $(OBJS) $(LDFLAGS) \
	$(XLIBPATH) $(XLIBS) $(LIBS)

demo: demo.o libgks.a
	$(SEGLD) -o $@ demo.o libgks.a $(LDFLAGS) $(XLIBPATH) $(XLIBS) $(LIBS)

install:
	$(INSTALL) -m 644 libgks.* $(DESTDIR)
	$(INSTALL) -m 644 gksfont.dat $(DESTDIR)

clean:
	$(RM) -f *.bak *.o libgks.* so_locations demo
