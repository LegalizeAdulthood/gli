DISK = S:
SRCDIR = ..\ 
DESTDIR = $(DISK)\Programme\gli
CC = CL
#INCLUDES = /I$(DISK)\Programme\X11R6\include /I$(DISK)\Programme\TCL\include
INCLUDES = /I$(DISK)\Programme\TCL\include
CFLAGS = /c /nologo /MD /DNO_X11 /DTCL
#FC = FL32
FC = DF
FFLAGS = /c /nologo /MD
AR = LIB /nologo
LINK = LINK /nologo /NODEFAULTLIB:libc.lib 
GRLIB =
#XPATH = $(DISK)\Programme\X11R6\lib
#XLIBS = $(XPATH)\Xt.lib $(XPATH)\SM.lib $(XPATH)\ICE.lib $(XPATH)\X11.lib
XPATH =
XLIBS =
TCLPATH = $(DISK)\Programme\TCL\lib
TCLLIBS = $(TCLPATH)\tk83.lib $(TCLPATH)\tcl83.lib
LIBS = kernel32.lib wsock32.lib advapi32.lib user32.lib gdi32.lib \
       comdlg32.lib winspool.lib winmm.lib msvcrt.lib
CP = COPY
MKDIR = MKDIR

.IGNORE:

.SUFFIXES: .lib .obj .c .f

{$(SRCDIR)}.c.obj:
        $(CC) $(CFLAGS) $(INCLUDES) $<
{$(SRCDIR)}.f.obj:
        $(FC) $(FFLAGS) $<

all: gli.exe gligksm.exe cgmview.exe

gli.exe: gli.obj \
     libgli.lib libsight.lib libimage.lib libgus.lib libcrtl.lib libfrtl.lib \
     $(GRLIB) libgks.lib
        $(LINK) /out:$@ $** $(XLIBS) $(TCLLIBS) $(LIBS)

gli5.exe: gli.obj \
     libgli.lib libsight.lib libimage.lib libgus.lib libcrtl.lib libfrtl.lib \
     $(GRLIB) S:\GKS\libgks.lib
        $(LINK) /out:$@ $** $(XLIBS) $(TCLLIBS) $(LIBS)

libgli.lib: gligus.obj gligr.obj gligks.obj glisimp.obj glisight.obj \
            gliimage.obj glixui.obj glicgm.obj glirpc.obj gkstest.obj
        $(AR) /out:$@ $**

libsight.lib: sightmai.obj sight.obj sightdev.obj
        $(AR) /out:$@ $**

libimage.lib: imagemai.obj image.obj imageuti.obj imageapp.obj \
              imagedev.obj
        $(AR) /out:$@ $**

libgus.lib: gus.obj gustext.obj gusauto.obj gusplo10.obj
        $(AR) /out:$@ $**

libgks.lib: gksio.obj gkscbnd.obj gks.obj gksinq.obj gkserror.obj \
            gksroot.obj gksmisc.obj gksdidd.obj gksdps.obj \
            gksdtek.obj gksdtek2.obj gksdx11.obj gksdwin.obj \
            gksdcgm.obj gksdpbm.obj gksdwiss.obj gkswiss.obj \
            gksdhpgl.obj gksdvt.obj gksdgksm.obj gksforio.obj \
            gksafm.obj gksdpdf.obj compress.obj
        $(AR) /out:$@ $**

libcrtl.lib: formdrv.obj command.obj terminal.obj function.obj \
             variable.obj symbol.obj string.obj math.obj help.obj \
             system.obj com.obj
        $(AR) /out:$@ $**

libfrtl.lib: contour.obj spline.obj pixel.obj gridit.obj
        $(AR) /out:$@ $**

gligksm.exe: gligksm.obj libcrtl.lib libgks.lib
        $(LINK) /out:$@ $** $(XLIBS) $(LIBS)

cgmview.exe: cgmview.obj \
     libgli.lib libsight.lib libimage.lib libgus.lib libcrtl.lib libfrtl.lib \
     libgks.lib
        $(LINK) /out:$@ $** $(XLIBS) $(LIBS)

install: all
        COPY gli.exe $(DESTDIR)
        COPY gligksm.exe $(DESTDIR)
        COPY cgmview.exe $(DESTDIR)
        COPY sight.bat $(DESTDIR)
        COPY *.lib $(DESTDIR)

sfx: gli45.exe
gli45.exe:
        PKZIP25 -add -recurse -direct=root -sfx gli45.exe $(DESTDIR)\*

clean:
        DEL gli.jou
        DEL gus.log
        DEL gli*.exe
        DEL cgmview.exe
        DEL *.lib
        DEL *.obj
        DEL *.bak
