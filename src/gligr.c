/*
 *
 * FACILITY:
 *
 *	Graphics Language Interpreter (GLI)
 *
 * ABSTRACT:
 *
 *	This module contains a GR/GR3 command language interpreter.
 *
 * AUTHOR:
 *
 *	Josef Heinen
 *
 * VERSION:
 *
 *	V1.0
 *
 */

#include <stdio.h>
#include <string.h>

#include "system.h"
#include "terminal.h"
#include "variable.h"
#include "command.h"
#include "grsoft.h"


#define odd(status) ((status) & 01)


#ifdef GRSOFT

/* GR/GR3 commands */

typedef struct 
{
    int (*routine)();
    char *info;
} command_info;

extern int GR00DG(), GR90DG(), GRARRW(), GRAXLIN(), GRAXLOG(), GRAXS(),
    GRAXSL(), GRBLD(), GRCHN(), GRCHNC(), GRCHRC(), GRCLP(), GRCRCL(),
    GRDEL(), GRDN(), GRDRAX(), GRDRDM(), GRDRDU(), GRDRHS(), GRDRKU(),
    GRDRLG(), GRDRNE(), GRDRW(), GRDRWS(), GRDSH(), GREND(), GRENDE(),
    GRFILL(), GRFONT(), GRFRBN(), GRGFLD(), GRJMP(), GRJMPS(), GRHHFL(),
    GRHHNL(), GRHPAN(), GRLGND(), GRLN(), GRLNCN(), GRMRKS(), GRMSKN(),
    GRMSKF(), GRNWPN(), GRNXTF(), GRPREC(), GRPTS(), GRSCAX(), GRSCDL(),
    GRSCLC(), GRSCLP(), GRSCLV(), GRSHD(), GRSHOW(), GRSPHR(), GRSPTS(),
    GRSTRT(), GRTXT(), GRTXTC(), GRVAR(), GRVFLD(), GRWIN(), GSTXAL(),
    KURVEF(), PSKURF(), PSKURL(), SKURF(), SKURL();

static
command_info grsoft_command[] = {
    { GR00DG,	""},
    { GR90DG,	""},
    { GRARRW,	"XP:R YP:R XTIP:R YTIP:R ALEN:R AWID:R ICODE:I"},
    { GRAXLIN,	"X1:R Y1:R X2:R Y2:R S1:R S2:R LINKS:L KINDAX:I"},
    { GRAXLOG,	"X1:R Y1:R X2:R Y2:R S1:R S2:R LINKS:L KINDAX:I"},
    { GRAXS,	"LOPT:I OPT:C LTXTX:I TEXTX:C LTXTXY:I TEXTXY:C"},
    { GRAXSL,	"KINDX:I KINDY:I KINDDEC:I"},
    { GRBLD,	"XCM:R=33 YCM:R=24 ISK:I=1 JSK:I=1 XMIN:R=666 XMAX:R=666\
 YMIN:R=666 YMAX:R=666 NKURV:I=-666"},
    { GRCHN,	"XX:1 YY:2 M:I=# NR:I=1"},
    { GRCHNC,	"XX:1 YY:2 M:I=# NR:I=1"},
    { GRCHRC,	"HEIGHT:R ANGLE:R IDUMMY:I=0"},
    { GRCLP,	"ICLIP:I"},
    { GRCRCL,	"XM:R YM:R R:R PH1:R PH2:R"},
    { GRDEL,	""},
    { GRDN,	"IDIN:I XCM:R YCM:R"},
    { GRDRAX,	"LTX:I TX:C LTY:I TY:C LTZ:I TZ:C SZT:R"},
    { GRDRDM,	"DRDMPA:4 NROW:I TAB:3 XX:1 YY:2"},
    { GRDRDU,	"DRDMPA:4 NROW:I TAB:3 XX:1 YY:2"},
    { GRDRHS,	"DRDMPA:4 NPN:I PNKT:3 XX:1 YY:2"},
    { GRDRKU,	"DRDMPA:4 N:I XYZ:3"},
    { GRDRLG,	"DRDMPA:4 TEXTX:C TEXTY:C TEXTZ:C IOPT:I X:R Y:R Z:R\
 IX:I IY:I IZ:I"},
    { GRDRNE,	"PARM:4 NROW:I XYZ:3"},
    { GRDRW,	"X:R Y:R"},
    { GRDRWS,	"X:R Y:R NR:I=1"},
    { GRDSH,	"A1:R A2:R A3:R"},
    { GREND,	""},
    { GRENDE,	""},
    { GRFILL,	"N:I XX:1 YY:2 ISTYLE:I ITYPE:I=1"},
    { GRFONT,	"IFONT:I"},
    { GRFRBN,	"IFU:I IKA:I IKS:I IRA:I IRI:I"},
    { GRGFLD,	"IMAX:I F:1 ISTAX:I INCX:I NX:I ISTAY:I INCY:I NY:I\
 KIND:I HOEHE:R BREITE:R WINKEL:R DICKE:R IER:I"},
    { GRJMP,	"X:R Y:R"},
    { GRJMPS,	"X:R Y:R NR:I=1"},
    { GRHHFL,	"NX:I IX:I VX:1 NY:I IY:I VY:2 NW:I VW:4 IVFA:5\
 NDX:I TAB:3 INTACT:I"},
    { GRHHNL,	"N1:I TAB:3 N3:I XX:1 N4:I YY:2 L1:I WERT:4 INT:5\
 OPTION:C ISTAX:I INCX:I ISTAY:I INCY:I"},
    { GRHPAN,	"N1:I TAB:3 N3:I XX:1 N4:I YY:2 L1:I WERT:4 INT:5\
 OPTION:C ISTAX:I INCX:I ISTAY:I INCY:I"},
    { GRLGND,	"TX:?"},
    { GRLN,	"XX:1 YY:2 M:I=#"},
    { GRLNCN,	"XX:1 YY:2 M:I=#"},
    { GRMRKS,	"HEIGHT:R"},
    { GRMSKN,	""},
    { GRMSKF,	""},
    { GRNWPN,	"I:I"},
    { GRNXTF,	""},
    { GRPREC,   "IPREC:I"},
    { GRPTS,	"XX:1 YY:2 M:I=# NR:I=1"},
    { GRSCAX,	"CX:C CY:C CZL:C CZR:C"},
    { GRSCDL,	"N:I X:1 Y:2 LINTYP:I CHA:C NH:I XH:3 YH:4"},
    { GRSCLC,	"XA:R YA:R XB:R YB:R"},
    { GRSCLP,	"XCM:R YCM:R RAHMEN:I"},
    { GRSCLV,	"XA:R YA:R XB:R YB:R"},
    { GRSHD,	"X1:1 Y1:2 X2:3 Y2:4 ABST:R WINKEL:R N1:I N2:I"},
    { GRSHOW,	""},
    { GRSPHR,	"NS:I NR:I VS:3 INFA:5 CHI:R PSI:R ZP:R HS:4 IPO:1\
 MDM:I IER:I"},
    { GRSPTS,	"INT:I"},
    { GRSTRT,	"CAMERA:I=35 DDNUMB:I=8"},
    { GRTXT,	"X:R Y:R LTEXT:I TEXT:C"},
    { GRTXTC,	"LTEXT:I TEXT:C"},
    { GRVAR,	"X:1 Y:2 VAR:3 N:I=# KOAX:I=0"},
    { GRVFLD,	"NDIM1:I VA:1 VB:2 IFA:5 ISTAX:I INCX:I NX:I ISTAY:I\
 INCY:I NY:I KIND:I HOEHE:R BREITE:R WINKEL:R DICKE:R IER:I"},
    { GRWIN,	"X1:R Y1:R X2:R Y2:R IDUMMY:I=0"},
    { GSTXAL,	"IALH:I IALV:I"},
    { KURVEF,	"X:1 Y:2 IST:I=# ISY:I=1"},
    { PSKURF,	"X:1 Y:2 IST:I=# FORM:R Z:3"},
    { PSKURL,	"X:1 Y:2 IST:I=# FORM:R Z:3 KSK:I"},
    { SKURF,	"X:1 Y:2 IST:I=# FORM:R Z:3"},
    { SKURL,	"X:1 Y:2 IST:I=# FORM:R Z:3 KSK:I"}
};

static
cli_verb_list grsoft_command_table =
"00dg 90dg arrw axlin axlog axs axsl bld chn chnc chrc clp crcl del\
 dn drax drdm drdu drhs drku drlg drne drw drws dsh end ende fill font\
 frbn gfld jmp jmps hhfl hhnl hpan lgnd ln lncn mrks mskn mskf nwpn nxtf\
 prec pts scax scdl sclc sclp sclv shd show sphr spts strt txt txtc var\
 vfld win gstxal kurvef pskurf pskurl skurf skurl";


#endif /* GRSOFT */


void do_grsoft_command (int *stat)
/*
 * command - parse a GR/GR3 command
 */
{

#ifdef GRSOFT
    int index;

    cli_get_keyword ("GRSOFT", grsoft_command_table, &index, stat);

    if (odd(*stat))
    {
        if (strchr(grsoft_command[index].info, '?') == NULL)
        {
            cli_parse_command (grsoft_command[index].info, stat);
            if (odd(*stat))
                cli_call_proc (
		    (int (*)(char *, ...))grsoft_command[index].routine);
        }
    else
        *stat = cli__comdnyi; /* command not yet implemented */
    }
#else
    tt_fprintf (stderr, "Can't access GR/GR3 software libraries.\n");
#endif

}
