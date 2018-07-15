!CFLAGS	= $(CFLAGS)/DEFINE=(MOTIF,GRSOFT,UIS)/WARNINGS=NOINFORMATIONAL
CFLAGS	= $(CFLAGS)/DEFINE=(MOTIF,GRSOFT)/WARNINGS=NOINFORMATIONAL
FFLAGS	= /MATH_LIBRARY=V5
LIBS	= 'OPT'/OPT,[--.gr.src]libgr/LIB
GKS	= SYS$LIBRARY:GKS3D$FORBND/LIB
TAPE	= MKB500:
KIT	= GLI045
BINDIR	= USR:[GLI]
DEMODIR	= USR:[GLI.DEMO]
IMGDIR	= USR:[GLI.DEMO.IMAGES]

.SUFFIXES : .EXE,.OLB,.HLB,.OBJ,.PEN,.UID,.C,.FOR,.F,.PAS,.MAR,.HLP,.UIL

.F.OBJ :
        $(FORT) $(FFLAGS) $(MMS$SOURCE)

gliobjects = glisight.obj,gliimage.obj,gligus.obj,gligr.obj,-
	gligks.obj,glisimp.obj,glixui.obj,gliimage.obj,glicgm.obj,-
	glirpc.obj,gkstest.obj
sightobjects = sightmai.obj,sight.obj,sightdev.obj
imageobjects = imagemai.obj,image.obj,imageuti.obj,imageapp.obj,imagedev.obj
gusobjects = gus.obj,gustext.obj,gusplo10.obj,gusauto.obj
gksobjects = gksio.obj,gkscbnd.obj,gks.obj,gksinq.obj,gkserror.obj,gksroot.obj,-
	gksmisc.obj,gksdidd.obj,gksdps.obj,gksdtek.obj,gksdtek2.obj,-
	gksdx11.obj,gksduis.obj,gksdcgm.obj,gksdpbm.obj,gkswiss.obj,-
	gksdwiss.obj,gksdhpgl.obj,gksdvt.obj,gksdgksm.obj,gksdpdf.obj,-
	gksforio.obj,gksafm.obj,compress.obj
crtlobjects = formdrv.obj,command.obj,function.obj,variable.obj,symbol.obj,-
	terminal.obj,string.obj,math.obj,help.obj,system.obj,com.obj
frtlobjects = contour.obj,spline.obj,pixel.obj,gridit.obj

targets : gli
	@ CONTINUE

gli : setup,gli.exe,gligksm.exe,cgmview.exe,gks.olb,gli.hlb,gksdefs.pen,-
      rtl.pen,sight.uid,aeliter.uid,deliter.uid,image.uid,cgmview.uid
	@ CONTINUE

setup :
	@ DEFINE :== DEFINE /NOLOG
	@ DEFINE SYS SYS$LIBRARY
	@ DEFINE UIL$INCLUDE SYS$SYSROOT:[DECW$INCLUDE], []
	@ IF F$EXTRACT(0,2,F$GETSYI("VERSION")) .GES. "V6" THEN OPT := ovmslink.opt
	@ IF F$EXTRACT(0,2,F$GETSYI("VERSION")) .LES. "V5" THEN OPT := vmslink.opt

gli.exe : gli.obj,-
	libgli.olb,libsight.olb,libimage.olb,libgus.olb,libcrtl.olb,-
	libfrtl.olb,libgks.olb
	LINK /NOUSER gli.obj,-
	libgli/LIB,libsight/LIB,libimage/LIB,libgus/LIB,libcrtl/LIB,-
	libfrtl/LIB,libgks/LIB,$(LIBS)

gligksm.exe : gligksm.obj,libcrtl.olb,libgks.olb
	LINK /NOUSER gligksm.obj,libcrtl/LIB,libgks/LIB,$(LIBS)

glidecgks.exe : gli.obj,-
	libgli.olb,libsight.olb,libimage.olb,libgus.olb,libcrtl.olb,-
	libfrtl.olb
	LINK /NOUSER/EXECUTABLE=glidecgks.exe gli.obj,-
	libgli/LIB,libsight/LIB,libimage/LIB,libgus/LIB,libcrtl/LIB,-
	libfrtl/LIB,$(LIBS),$(GKS)

glicgm.obj : glicgm.c
	CC /NOLIST/NOOPTIMIZE/OBJECT=glicgm.obj glicgm.c

libgli.olb : libgli.olb($(gliobjects))
	@ CONTINUE
libsight.olb : libsight.olb($(sightobjects))
	@ CONTINUE
libimage.olb : libimage.olb($(imageobjects))
	@ CONTINUE
libgus.olb : libgus.olb($(gusobjects))
	@ CONTINUE
libgks.olb : libgks.olb($(gksobjects))
	@ CONTINUE
libcrtl.olb : libcrtl.olb($(crtlobjects))
	@ CONTINUE
libfrtl.olb : libfrtl.olb($(frtlobjects))
	@ CONTINUE

cgmview.exe : cgmview.obj,-
	libgli.olb,libsight.olb,libimage.olb,libgus.olb,libcrtl.olb,-
	libfrtl.olb,libgks.olb
	LINK /NOUSER cgmview.obj,-
	libgli/LIB,libsight/LIB,libimage/LIB,libgus/LIB,libcrtl/LIB,-
	libfrtl/LIB,libgks/LIB,$(LIBS)

gks.olb : gligksshr.exe
	LIBRARY /CREATE/SHAREABLE GKS.OLB gligksshr.exe
gligksshr.exe : gksxfr.obj,$(gksobjects)
	LINK /NOUSER/SHARE=gligksshr.exe gksxfr.obj,-
	$(gksobjects),'OPT'/OPT

gli.hlb : gli.hlp
	LIBRARY /CREATE/HELP gli.hlb gli.hlp

gksdefs.pen : [.pas]gksdefs.pas
	PASCAL /ENVIRONMENT [.pas]gksdefs.pas
rtl.pen : [.pas]rtl.pas
	PASCAL /ENVIRONMENT [.pas]rtl.pas

sight.uid : sight.uil
	UIL /MOTIF sight.uil
aeliter.uid : aeliter.uil
	UIL /MOTIF aeliter.uil
deliter.uid : deliter.uil
	UIL /MOTIF deliter.uil
image.uid : image.uil
	UIL /MOTIF image.uil
cgmview.uid : cgmview.uil
	UIL /MOTIF cgmview.uil

install : gli
	@ cp :== COPY /PROT=(S:RWED,O:RWED,G:RWED,W:RE)
	@ echo :== WRITE SYS$OUTPUT
	@ echo "Installing GLI executables"
	@ cp *.exe, gli.obj, cgmview.obj $(BINDIR)
	@ cp glisetup.com, gkslogin.com, relink.com $(BINDIR)
	@ echo "Installing GLI archive files"
	@ cp *.olb $(BINDIR)
	@ cp *.uid $(BINDIR)
	@ echo "Installing GLI environment files"
	@ cp glistart.gli $(BINDIR)
	@ cp *.fdv $(BINDIR)
	@ cp *.h $(BINDIR)
	@ cp *.i $(BINDIR)
	@ cp *link.opt $(BINDIR)
	@ cp *.pen $(BINDIR)
	@ echo "Installing GLI help file"
	@ cp gli.hlb $(BINDIR)
	@ echo "Installing GLI Hershey fonts"
	@ cp [-.font]gksfont.dat $(BINDIR)
	@ echo "Installing GLI example scripts"
	@ cp [-.demo]*.* $(DEMODIR)
	@ cp [-.demo.images]*.* $(IMGDIR)
	@ echo "Installing GR shareable image"
	@ cp [--.gr.src]*.exe $(BINDIR)
	@ echo "Installing GR archive files"
	@ cp [--.gr.src]*.olb $(BINDIR)
	@ echo "Installing UIS shareable image"
	@ cp sys$share:uisshr.exe $(BINDIR)
	@ echo "Installing VMS Math Library shareable image"
	@ cp sys$share:uvmthrtl.exe $(BINDIR)
	
clean :
	DELETE /NOLOG *.OBJ;*, *.PEN;*, *.OLB;*, *.EXE;*, *.HLB;*, *.UID;*

