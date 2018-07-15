CC = CL
CFLAGS = /c /nologo /DNO_X11 /D_DLL /MD
#FC = FL32
FC = DF
FFLAGS = /c /nologo /D_DLL /MD
AR = LIB /nologo
#LINK = LINK /nologo
LINK = LINK /nologo /entry:mainCRTStartup
DLLLFLAGS = /nodefaultlib -dll
#DLLLIBS = msfrt.lib msvcrt.lib oldnames.lib wsock32.lib advapi32.lib user32.lib gdi32.lib winspool.lib
DLLLIBS = dfordll.lib dfconsol.lib msvcrt.lib oldnames.lib kernel32.lib wsock32.lib advapi32.lib user32.lib gdi32.lib comdlg32.lib winspool.lib
DLLLINK = LINK /nologo
SRCDIR = ..\ 
DESTDIR = S:\gli

.SUFFIXES: .obj .c .f

{$(SRCDIR)}.c.obj:
	$(CC) $(CFLAGS) $<
{$(SRCDIR)}.f.obj:
	$(FC) $(FFLAGS) $<

all: gks.lib demo.exe

gks.lib: gks.dll

gks.dll: gksdll.obj gksio.obj gks.obj gksinq.obj gkserror.obj \
	    gksroot.obj gksmisc.obj gksdidd.obj gksdps.obj \
	    gksdtek.obj gksdtek2.obj gksdx11.obj gksdwin.obj \
	    gksdcgm.obj gksdpbm.obj gksdwiss.obj gkswiss.obj \
	    gksdhpgl.obj gksdvt.obj gksdgksm.obj gksdpdf.obj \
	    gksforio.obj gksafm.obj compress.obj
	$(DLLLINK) /out:$@ $** -def:gks.def $(DLLLFLAGS) $(DLLLIBS)

demo.exe: demo.obj gks.lib
	$(LINK) /out:$@ demo.obj gks.lib

install: all
        COPY gks.lib $(DESTDIR)
        COPY gks.dll $(DESTDIR)
        
clean:
	DEL demo.exe
	DEL *.lib
	DEL *.exp
	DEL *.dll
	DEL *.obj
	DEL *.bak
