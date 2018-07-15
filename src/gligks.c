/*
 *
 * FACILITY:
 *
 *	Graphics Language Interpreter (GLI)
 *
 * ABSTRACT:
 *
 *	This module contains a Graphic Kernel System (GKS) command
 *	language interpreter.
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
#include <ctype.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>

#ifndef MSDOS
#include <sys/types.h>
#endif

#if !defined(VMS) && !defined(MSDOS) && !defined(_WIN32)
#include <unistd.h>
#endif

#if defined (cray) || defined (__SVR4) || defined (MSDOS) || defined (_WIN32)
#include <fcntl.h>
#else
#include <sys/file.h>
#endif

#ifdef VMS
#include <descrip.h>
#endif

#ifdef cray
#include <fortran.h>
#endif

#include "system.h"
#include "terminal.h"
#include "variable.h"
#include "function.h"
#include "command.h"
#include "strlib.h"
#include "symbol.h"
#include "gksdefs.h"


static int errfil =	0;	    /* default error file */

static int decgks =	0;

static int wk_id =	0;
static int tnr =	1;	    /* default transformation number */
static int lcdnr = 	1;	    /* locator device number */
static int skdnr =	1;	    /* stroke device number */
static int sg_name =    1;	    /* segment name */

static int segn;

extern int max_points;
extern float *px, *py, *pz;


#define update_interval	    2	    /* workstation update interval */

#define BOOL int
#define TRUE 1
#define FALSE 0
#define NIL 0

#define MAX_COLORS 980

#define odd(status) ((status) & 01)



typedef unsigned short uword;

typedef char short_string[16];
typedef char identifier[32];
typedef char string[255];
typedef char logical_name[32];


/* GKS commands */

typedef enum { command_open_gks, command_close_gks, command_open_ws,
    command_close_ws, command_activate_ws, command_deactivate_ws,
    command_clear_ws, command_update_ws, command_set, command_polyline,
    command_polymarker, command_text, command_fill_area, command_cell_array,
    command_create_sg, command_close_sg, command_copy_sg, command_request,
    command_inquire, command_emergency_close, command_test } gks_command;

typedef enum { attribute_asf, primitive_polyline, primitive_polymarker,
    primitive_text, primitive_fill_area, xform_viewport, xform_window,
    xform_clipping, xform_select, xform_ws_viewport, xform_ws_window,
    color_rep, seg_xform } set_option;

typedef enum { type_ndc, type_wc } select_type;

typedef enum { pline_linetype, pline_linewidth, pline_color_index }
    attribute_pline;

typedef enum { pmark_type, pmark_size, pmark_color_index } attribute_pmark;

typedef enum { text_fontprec, text_expfac, text_spacing, text_color_index,
    text_height, text_upvec, text_path, text_align } attribute_text;

typedef enum { fill_int_style, fill_style_index, fill_color_index } 
    attribute_fill;

typedef enum { class_locator, class_stroke } input_class;

typedef enum { option_gks_state, option_text_extent, option_ws_conntype,
    option_ws_type } inquire_option;

typedef struct {
    float a,b,c,d;
    } norm_xform;

typedef struct {
    int conid;
    string devnam;
    string default_name;
    short_string wstype;
    int fd;
    FILE *stream;
    string path;
    } ws_cap;


static ws_cap ws_info[] = {
    {  0, "GLI_CONID", "",          "GLI_WSTYPE", -1, NULL, "" }, /* terminal */
    {  0, "",          "",          "GLI_WSTYPE", -1, NULL, "" }, /* wk1 */
    {  0, "",          "",          "GLI_WSTYPE", -1, NULL, "" }, /* wk2 */
    {  0, "",          "",          "GLI_WSTYPE", -1, NULL, "" }, /* wk3 */
    {  0, "",          "",          "GLI_WSTYPE", -1, NULL, "" }, /* wk4 */
    {  0, "",          "",          "GLI_WSTYPE", -1, NULL, "" }, /* wk5 */
    {  0, "",          "",          "GLI_WSTYPE", -1, NULL, "" }, /* wk6 */
    {  0, "",          "",          "GLI_WSTYPE", -1, NULL, "" }, /* wk7 */
    {  0, "",          "",          "GLI_WSTYPE", -1, NULL, "" }, /* wk8 */
    {  0, "",          "",          "GLI_WSTYPE", -1, NULL, "" }, /* wk9 */
    {  0, "",          "",          "GLI_WSTYPE", -1, NULL, "" }, /* wk10 */
    {  0, "",          "",          "GLI_WSTYPE", -1, NULL, "" }, /* wk11 */
    {  0, "",          "",          "GLI_WSTYPE", -1, NULL, "" }, /* wk12 */
    { 88, "",          "",          "GLI_WSTYPE", -1, NULL, "" }, /* wiss */
    {  7, "GLI_CGM",   "gli.cgm",   "",           -1, NULL, "" },
    {  3, "GLI_FIG",   "gli.fig",   "",           -1, NULL, "" },
    {  9, "GLI_GKSM",  "gli.gksm",  "",           -1, NULL, "" },
    {  4, "GLI_PL",    "gli.pl",    "",           -1, NULL, "" },
    {  8, "GLI_LW",    "gli.lw",    "",           -1, NULL, "" }
    };
    
static cli_verb_list gks_command_table = "open_gks close_gks open_ws close_ws\
 activate_ws deactivate_ws clear_ws update_ws set polyline polymarker text\
 fill_area cell_array create_sg close_sg copy_sg request inquire\
 emergency_close test";

static cli_verb_list ws_names = "terminal wk1 wk2 wk3 wk4 wk5 wk6 wk7 wk8\
 wk9 wk10 wk11 wk12 wiss cgm_file figure_file metafile plotter laser_printer";

static cli_verb_list set_options = "asf pline pmark text fill viewport window\
 clipping xform ws_viewport ws_window color_rep seg_xform";

static cli_verb_list colors = "white black red green blue cyan yellow magenta\
 color8 color9 color10 color11 color12 color13 color14 color15";

static cli_verb_list clip_switch = "off on";

static cli_verb_list xform_types = "ndc wc";

static cli_verb_list aspect_source_flags = "bundled individual";

static cli_verb_list pline_attributes = "linetype linewidth color_index";

static cli_verb_list line_types = "solid dashed dotted dash_dotted dash_2_dot\
 dash_3_dot long_dash long_short_dash spaced_dash spaced_dot double_dot\
 triple_dot";

static int av_ltype[] = { 1, 2, 3, 4, -1, -2, -3, -4, -5, -6, -7, -8 };

static cli_verb_list pmark_attributes = "type size color_index";

static cli_verb_list marker_types = "dot plus asterisk circle diagonal_cross\
 solid_circle triangle_up solid_tri_up triangle_down solid_tri_down square\
 solid_square bowtie solid_bowtie hourglass solid_hglass diamond solid_diamond\
 star solid_star tri_up_down solid_tri_left solid_tri_right hollow_plus omark";

static int av_mtype[] = { 1, 2, 3, 4, 5, -1, -2, -3, -4, -5, -6, -7, -8, 
    -9, -10, -11, -12, -13, -14, -15, -16, -17, -18, -19, -20 };

static cli_verb_list text_attributes = "fontprec expfac spacing color_index\
 height upvec path align";

static cli_verb_list text_precision = "string char stroke";

static cli_verb_list text_pathes = "right left up down";

static cli_verb_list hor_align = "normal left centre right center";

static cli_verb_list ver_align = "normal top cap half base bottom";

static cli_verb_list fill_area_attributes = "int_style style_index color_index";

static cli_verb_list int_style = "hollow solid pattern hatch";

static cli_verb_list input_classes = "locator stroke";

static cli_verb_list inquire_options = "gks_state text_extent ws_conntype\
 ws_type";

static char *rec_prompt[] = { "X_min", "X_max", "Y_min", "Y_max" };

static char *color_prompt[] = { "Index", "Red intensity", "Green intensity",
    "Blue intensity" };

static char *seg_xform_prompt[] = { "Fixed point X", "Fixed point Y", 
    "Translation X", "Translation Y", "Rotation", "Scale X", "Scale Y" };

static cli_data_descriptor x,y;

static char *pl_s[] = { "", "s" };

static norm_xform xform;

static BOOL packed_cell_array = FALSE;



static char *trnlog (char *result, char *log_name)

/*
 * trnlog - translate logical name
 */

{
    char *name;

    name = (char *) getenv (log_name);
    if (name)
	strcpy (result, name);
    else
	strcpy (result, "");

    return result;
}



static void gks_text_s (float *px, float *py, int *nchars, char *chars)
{
#ifdef VMS
    struct dsc$descriptor_s text;

    text.dsc$b_dtype = DSC$K_DTYPE_T;
    text.dsc$b_class = DSC$K_CLASS_S;
    text.dsc$w_length = *nchars;
    text.dsc$a_pointer = chars;

    GTXS (px, py, nchars, &text);
#else
#ifdef cray
    _fcd text;

    text = _cptofcd(chars, *nchars);

    GTXS (px, py, nchars, text);
#else
#if defined(_WIN32) && !defined(__GNUC__)
    unsigned short chars_len = *nchars;

    GTXS (px, py, nchars, chars, chars_len);
#else
    GTXS (px, py, nchars, chars, *nchars);
#endif /* _WIN32 */
#endif /* cray */
#endif /* VMS */
}



static void gks_inq_text_extent_s (int *wkid, float *px, float *py, int *nchars,
    char *chars, int *errind, float *cpx, float *cpy, float *tx, float *ty)
{
#ifdef VMS
    struct dsc$descriptor_s text;

    text.dsc$b_dtype = DSC$K_DTYPE_T;
    text.dsc$b_class = DSC$K_CLASS_S;
    text.dsc$w_length = *nchars;
    text.dsc$a_pointer = chars;

    GQTXXS (wkid, px, py, nchars, &text, errind, cpx, cpy, tx, ty);
#else
#ifdef cray
    _fcd text;

    text = _cptofcd(chars, *nchars);

    GQTXXS (wkid, px, py, nchars, text, errind, cpx, cpy, tx, ty);
#else
#if defined(_WIN32) && !defined(__GNUC__)
    unsigned short chars_len = *nchars;

    GQTXXS (wkid, px, py, nchars, chars, chars_len, errind, cpx, cpy, tx, ty);
#else
    GQTXXS (wkid, px, py, nchars, chars, errind, cpx, cpy, tx, ty, *nchars);
#endif /* _WIN32 */
#endif /* cray */
#endif /* VMS */
}



static void update_ws (int wkid)

/*
 * update_ws - update workstation action routine
 */

{
    int errind, conid, wtype, wkcat;

    GQWKC (&wkid, &errind, &conid, &wtype);
    GQWKCA (&wtype, &errind, &wkcat);

    if (wkcat == GOUTPT || wkcat == GOUTIN)
	GUWK (&wkid, &GPOSTP);

}  /* update_workstation */




static void update_ws_perfo (int wkid)

/*
 * update_ws - update workstation action routine
 */

{
    int errind, conid, wtype, wkcat;

    GQWKC (&wkid, &errind, &conid, &wtype);
    GQWKCA (&wtype, &errind, &wkcat);

    if (wkcat == GOUTPT || wkcat == GOUTIN || wkcat == GMO)
	GUWK (&wkid, &GPERFO);

}  /* update_workstation */




static void open_ws_server (void (*actrtn)(int))

/*
 * open_ws_server - server for all open workstations
 */

{
    int state, i, n, errind, ol, wkid;

    GQOPS (&state);
    if (state >= GWSOP)
        {
        i = 1;
        GQOPWK (&i, &errind, &ol, &wkid);

        for (i=ol; i >= 1; i--)
            {
            n = i;
            GQOPWK (&n, &errind, &ol, &wkid);

            actrtn (wkid);
            }
        }

}  /* open_ws_server */





static void active_ws_server (void (*actrtn)(int))

/*
 * active_ws_server - server for all active workstations
 */

{
    int state, i, n, errind, ol, wkid;

    GQOPS (&state);
    if (state >= GWSAC)
        {
        i = 1;
        GQACWK (&i, &errind, &ol, &wkid);

        for (i=ol; i >= 1; i--)
            {
            n = i;
            GQACWK (&n, &errind, &ol, &wkid);

            actrtn (wkid);
            }
        }

}  /* active_ws_server */




static void open_gks (int *stat)

/*
 * open_gks - open GKS
 */

{
    int bufsiz = -1;
    int errind, level;
#ifdef __alpha
    static char *env = NULL;
    char *dpy;
#endif

    if (cli_end_of_command())
        {
#ifdef __alpha
	if ((char *) getenv ("GKSconid") == NULL)
	    {
	    if (env == NULL)
		env = (char *) malloc (255);

	    strcpy (env, "GKSconid=");
	    if ((dpy = (char *) getenv ("DISPLAY")) != NULL)
		strcat (env, dpy);
	    else
		strcat (env, ":0.0");

	    putenv (env);
	    }
#endif

	GOPKS (&errfil, &bufsiz);

	GQLVKS (&errind, &level);
	decgks = (level > GL0B);

	packed_cell_array = getenv("GLI_GKS_PACKED_CELL_ARRAY") ? TRUE : FALSE;
	}
    else
        *stat = cli__maxparm; /* maximum parameter count exceeded */

}  /* open_gks */





static void close_gks (int *stat)

/*
 * close_gks - close GKS
 */

{
    if (cli_end_of_command())
        {
	GCLKS ();
	}
    else
        *stat = cli__maxparm; /* maximum parameter count exceeded */

}  /* close_gks */





static void emergency_close_ws (int id)

/*
 * emergency_close_ws - emergency close workstation
 */

{
    int state, conid, errind, ol, wkid, wtype;
    int i, n, ws_name;

    GQOPS (&state);
    if (state == GSGOP)
        GCLSG ();

    if (state >= GWSAC)
        {
        i = 1;
        GQACWK (&i, &errind, &ol, &wkid);

        for (i=ol; i >= 1; i--)
            {
            n = i;
            GQACWK (&n, &errind, &ol, &wkid);

            if ((wkid == id) || (id == 0))
                GDAWK (&wkid);
            }
        }

    if (state >= GWSOP)
        {
        i = 1;
        GQOPWK (&i, &errind, &ol, &wkid);

        for (i=ol; i >= 1; i--)
            {
            n = i;

            GQOPWK (&n, &errind, &ol, &wkid);
            GQWKC (&wkid, &errind, &conid, &wtype);

            if ((wkid == id) || (id == 0))
                {
                GCLWK (&wkid);

		if (wkid == 1)
		    wk_id = 0;

                ws_name = wkid-1;
                if (ws_info[ws_name].fd >= 0)
                    {
#if !defined(VMS) && !defined(MSDOS) && !defined(_WIN32)
                    if (*ws_info[ws_name].path == '|')
			pclose (ws_info[ws_name].stream);
		    else
#endif
			close (conid);

                    ws_info[ws_name].fd = -1;
                    ws_info[ws_name].stream = NULL;
                    }
                }
            }
        }

} /* emergency_close_ws */





static void open_workstation (int *stat)

/*
 * open_workstation - open workstation
 */

{
    int conid, wtype;
    int ws_name, wkid;
    string type, str;
    int ret_status;
    char *env, *s;

    cli_get_keyword ("Name", ws_names, &ws_name, stat);

    if (odd(*stat))
        {
        wkid = ws_name+1;
        conid = ws_info[ws_name].conid;
        wtype = GWSDEF;

        trnlog (str, ws_info[ws_name].wstype);
	if (*str)
            wtype = str_integer(str, stat);

        if (odd(*stat) && !cli_end_of_command())
            {
	    cli_get_parameter ("Type", type, " ,", FALSE, TRUE, stat);

	    if (odd(*stat))
	        {
		if (cli_end_of_command())
		    wtype = cli_integer(type, stat);
		else
		    *stat = cli__maxparm; /* maximum parameter count exceeded */
		}
            }

        if (odd(*stat))
            {
            emergency_close_ws (wkid);

	    trnlog (str, ws_info[ws_name].devnam);
	    if (!*str && conid)
		{
		switch (wtype) {
		    case 2  :
		    case 3  :	strcpy(str, "gli.gksm"); break;
		    case 7  :
		    case 8  :	strcpy(str, "gli.cgm"); break;
		    case 51 :
		    case 53 :	strcpy(str, "gli.hp"); break;
		    case 61 :
		    case 62 :	strcpy(str, "gli.eps"); break;
		    case 63 :
		    case 64 :
		    case 92 :	break;
		    case 101:
		    case 102:	strcpy(str, "gli.pdf"); break;
		    case 103:
		    case 104:	strcpy(str, "gli.pbm"); break;
		    case 215:
		    case 218:	strcpy(str, "gli.gif"); break;
		    case 214:	strcpy(str, "gli.rf"); break;
		     default:	strcpy(str, ws_info[ws_name].default_name);
		    }
		}

	    if (wtype == 63 || wtype == 64 || wtype == 92 ||
		(wtype >= 320 && wtype <= 323)) {
		conid = 0;
		}
	    else if (strcmp(str, "tmpnam") == 0)
		{
		conid = 0;
		if (*ws_info[ws_name].devnam)
		    {
		    env = (char *) malloc (255);
#if defined(__NetBSD__) || defined(linux)
		    sprintf (str, "/tmp/gli%d.tmp", getpid());
#else
		    tmpnam(str);
#endif
		    sprintf (env, "%s=%s", ws_info[ws_name].devnam, str);
		    putenv (env);
		    }
		}
	    else if (strncmp(str, "/tmp", 4) == 0)
		{
		conid = 0;
		}
	    else if (*str)
		{
		conid = str_integer(str, &ret_status);
		if (!odd(ret_status))
		    {
                    s = str;
		    while (isspace(*s))
			s++;
#if !defined(VMS) && !defined(MSDOS) && !defined(_WIN32)
		    if (*s == '|')
			{
			if ((ws_info[ws_name].stream = popen(s+1, "w")) != NULL)
			    conid = fileno (ws_info[ws_name].stream);
			else
			    conid = -1;
			}
		    else
#endif
			conid = open (s, O_CREAT | O_TRUNC | O_WRONLY
#if defined(ultrix) || defined(__OSF__)
			    | O_NDELAY | O_NONBLOCK
#endif
			    , 0644
#ifdef VAXC
			    , "rop=wbh", "mbc=16");
#else
			    );
#endif
		    ws_info[ws_name].fd = conid;
		    strcpy (ws_info[ws_name].path, s);
		    }
		}

	    if (conid >= 0)
		{
		if (decgks && wtype == GWSDEF)
		    {
		    if ((char *) getenv ("GKSwstype") == NULL)
			wtype = 0x0500000 | 231;
		    }

		GOPWK (&wkid, &conid, &wtype);
		GACWK (&wkid);

		if (wk_id == 0 || wkid == 1)
		    wk_id = wkid;

                if (wtype == GWSWIS)
                    GCRSG (&sg_name);
		else
                    GSDS (&wkid, &GBNIL, &GSUPPD);
                }
            else
		{
                *stat = RMS__ACC; /* file access failed */

		perror ("open");
		}
            }
        }

}  /* open_workstation */





static void close_workstation (int *stat)

/*
 * close_workstation - close workstation
 */

{
    int ws_name, wkid;

    if (!cli_end_of_command())
        {
        cli_get_keyword ("Name", ws_names, &ws_name, stat);

        if (odd(*stat))
            {
            wkid = ws_name+1;

            if (cli_end_of_command())
                emergency_close_ws (wkid);
            else
                *stat = cli__maxparm; /* maximum parameter count exceeded */
            }
        }
    else
        emergency_close_ws (0);

}  /* close_workstation */





static void activate_workstation (int *stat)

/*
 * activate_workstation - activate workstation
 */

{
    int ws_name, wkid;

    cli_get_keyword ("Name", ws_names, &ws_name, stat);

    if (odd(*stat))
        {
        wkid = ws_name+1;

	if (cli_end_of_command())
	    {
	    GACWK (&wkid);
	    }
	else
	    *stat = cli__maxparm; /* maximum parameter count exceeded */
        }

}  /* activate_workstation */





static void deactivate_workstation (int *stat)

/*
 * deactivate_workstation - deactivate workstation
 */

{
    int ws_name, wkid;

    cli_get_keyword ("Name", ws_names, &ws_name, stat);

    if (odd(*stat))
        {
        wkid = ws_name+1;

	if (cli_end_of_command())
	    {
	    GDAWK (&wkid);
	    }
	else
	    *stat = cli__maxparm; /* maximum parameter count exceeded */
        }

}  /* deactivate_workstation */





static void clear_ws (int wkid)

/*
 * clear_ws - clear workstation action routine
 */

{
    int state, errind, conid, wtype, wkcat;

    GQOPS (&state);
    if (state == GSGOP)
        GCLSG ();

    GQWKC (&wkid, &errind, &conid, &wtype);
    GQWKCA (&wtype, &errind, &wkcat);

    if (wkcat == GOUTPT || wkcat == GOUTIN || wkcat == GMO)
	{
	GCLRWK (&wkid, &GALWAY);
	GUWK (&wkid, &GPOSTP);
	}
}



static void clear_workstation (int *stat)

/*
 * clear_workstation - clear workstation
 */

{
    if (cli_end_of_command())
        active_ws_server (clear_ws);
    else
        *stat = cli__maxparm; /* maximum parameter count exceeded */
}





static void set_asf (int *stat)

/*
 * set_asf - set aspect source flags
 */

{
    int aspect_source, flag;
    int asf[13];

    cli_get_keyword ("Aspect source", aspect_source_flags, &aspect_source, stat);

    if (odd(*stat))
        {
        if (cli_end_of_command())
            {
            for (flag = 0; flag < 13; flag++)
                asf[flag] = aspect_source;

            GSASF (asf);
            }
        else
            *stat = cli__maxparm; /* maximum parameter count exceeded */
        }

}  /* set_asf */





static void set_pline_linetype (int *stat)

/*
 * set_pline_linetype - set polyline linetype
 */

{
    int linetype;

    cli_get_keyword ("Type", line_types, &linetype, stat);

    if (odd(*stat))
        {
        if (cli_end_of_command())
            {
	    GSLN (&av_ltype[linetype]);
	    }
        else
            *stat = cli__maxparm; /* maximum parameter count exceeded */
        }

}  /* set_pline_linetype */





static void set_pline_linewidth (int *stat)

/*
 * set_pline_linewidth - set polyline linewidth
 */

{
    string linewidth;
    float width;

    cli_get_parameter ("Width", linewidth, " ,", FALSE, TRUE, stat);

    if (odd(*stat))

        if (cli_end_of_command())
            {
            width = cli_real(linewidth, stat);

            if (odd(*stat))
                GSLWSC (&width);
            }
        else
            *stat = cli__maxparm; /* maximum parameter count exceeded */

}  /* set_pline_linewidth */





static void set_pline_color_index (int *stat)

/*
 * set_pline_color_index - set polyline color index
 */

{
    int color;

    cli_get_keyword ("Color", colors, &color, stat);

    if (odd(*stat))

        if (cli_end_of_command())
            {
	    GSPLCI (&color);
	    }
        else
            *stat = cli__maxparm; /* maximum parameter count exceeded */

}  /* set_pline_color_index */





static void set_polyline_attr (int *stat)

/*
 * set_polyline_attr - set polyline attribute
 */

{
    int attribute;
    attribute_pline pline_attr;

    cli_get_keyword ("Attribute", pline_attributes, &attribute, stat);

    pline_attr = (attribute_pline)attribute;

    if (odd(*stat))

        switch (pline_attr) {
            case pline_linetype :
		set_pline_linetype (stat);
		break;
            case pline_linewidth :
		set_pline_linewidth (stat);
		break;
            case pline_color_index :
		set_pline_color_index (stat);
		break;
            }

}  /* set_polyline_attr */





static void set_pmark_type (int *stat)

/*
 * set_pmark_type - set polymarker type
 */

{
    int marker;

    cli_get_keyword ("Type", marker_types, &marker, stat);

    if (odd(*stat))
        {
        if (cli_end_of_command())
            {
	    GSMK (&av_mtype[marker]);
	    }
        else
            *stat = cli__maxparm; /* maximum parameter count exceeded */
        }

}  /* set_pmark_type */





static void set_pmark_size (int *stat)

/*
 * set_pmark_size - set polymarker size
 */

{
    string marker_size;
    float size;

    cli_get_parameter ("Size", marker_size, " ,", FALSE, TRUE, stat);

    if (odd(*stat))

        if (cli_end_of_command())
            {
            size = cli_real(marker_size, stat);

            if (odd(*stat))
                GSMKSC (&size);
            }
        else
            *stat = cli__maxparm; /* maximum parameter count exceeded */

}  /* set_pmark_size */





static void set_pmark_color_index (int *stat)

/*
 * set_pmark_color_index - set polymarker color index
 */

{
    int color;

    cli_get_keyword ("Color", colors, &color, stat);

    if (odd(*stat))
        if (cli_end_of_command())
            {
	    GSPMCI (&color);
	    }
        else
            *stat = cli__maxparm; /* maximum parameter count exceeded */

}  /* set_pmark_color_index */





static void set_polymarker_attr (int *stat)

/*
 * set_polymarker_attr - set polymarker attribute
 */

{
    int attribute;
    attribute_pmark pmark_attr;

    cli_get_keyword ("Attribute", pmark_attributes, &attribute, stat);

    pmark_attr = (attribute_pmark)attribute;

    if (odd(*stat))

        switch (pmark_attr) {
            case pmark_type :
		set_pmark_type (stat);
                break;
            case pmark_size :
                set_pmark_size (stat);
                break;
            case pmark_color_index :
                set_pmark_color_index (stat);
                break;
            }

}  /* set_polymarker_attr */





static void set_text_fontprec (int *stat)

/*
 * set_text_fontprec - set text font and precision
 */

{
    string text_font;
    int font, precision;

    cli_get_parameter ("Font", text_font, " ,", FALSE, TRUE, stat);

    if (odd(*stat))
        {
        font = cli_integer(text_font, stat);

        if (odd(*stat))
            {
            cli_get_keyword ("Precision", text_precision, &precision, stat);

            if (odd(*stat))
                {
                if (cli_end_of_command())
                    {
		    if (decgks && font >= 1 && font <= 43 && precision == GSTRP)
			font = -100 - font;

		    GSTXFP (&font, &precision);
		    }
                else
                    *stat = cli__maxparm; /* maximum parameter count exceeded */
                }
            }
        }

}  /* set_text_fontprec */





static void set_character_expfac (int *stat)

/*
 * set_character_expfac - set character expansion factor
 */

{
    string character_expfac;
    float expfac;

    cli_get_parameter ("Factor", character_expfac, " ,",  FALSE, TRUE, stat);

    if (odd(*stat))
        {
        expfac = cli_real(character_expfac, stat);

        if (odd(*stat))
            {
            if (cli_end_of_command())
                {
		GSCHXP (&expfac);
		}
            else
                *stat = cli__maxparm; /* maximum parameter count exceeded */
            }
        }

}  /* set_character_expfac */





static void set_character_spacing (int *stat)

/*
 * set_character_spacing - set character spacing
 */

{
    string character_spacing;
    float spacing;

    cli_get_parameter ("Spacing", character_spacing, " ,", FALSE, TRUE, stat);

    if (odd(*stat))
        {
        spacing = cli_real(character_spacing, stat);

        if (odd(*stat))
            {
            if (cli_end_of_command())
                {
		GSCHSP (&spacing);
		}
            else
                *stat = cli__maxparm; /* maximum parameter count exceeded */
            }
        }

}  /* set_character_spacing */





static void set_text_color_index (int *stat)

/*
 * set_text_color_index - set text color index
 */

{
    int color;

    cli_get_keyword ("Color", colors, &color, stat);

    if (odd(*stat))

        if (cli_end_of_command())
            {
	    GSTXCI (&color);
	    }
        else
            *stat = cli__maxparm; /* maximum parameter count exceeded */

}  /* set_text_color_index */





static void set_character_height (int *stat)

/*
 * set_character_height - set character height
 */

{
    string character_height;
    float height;

    cli_get_parameter ("Height", character_height, " ,", FALSE, TRUE, stat);

    if (odd(*stat))
        {
        height = cli_real(character_height, stat);

        if (odd(*stat))
            {
            if (cli_end_of_command())
                {
		GSCHH (&height);
		}
            else
                *stat = cli__maxparm; /* maximum parameter count exceeded */
            }
        }

}  /* set_character_height */





static void set_character_upvec (int *stat)

/*
 * set_character_upvec - set character-up-vector
 */

{
    string component;
    float chux, chuy;

    cli_get_parameter ("X component", component, " ,", FALSE, TRUE, stat);

    if (odd(*stat))
        {
        chux = cli_real(component, stat);

        if (odd(*stat))
            {
            cli_get_parameter ("Y component", component, " ,", FALSE, TRUE, 
		stat);
            if (odd(*stat))
                {
                chuy = cli_real(component, stat);

                if (odd(*stat))
                    {
                    if (cli_end_of_command())
                        {
			GSCHUP (&chux, &chuy);
			}
                    else
                        /* maximum parameter count exceeded */
                        *stat = cli__maxparm;
                    }
                }
            }
        }

}  /* set_character_height */





static void set_text_path (int *stat)

/*
 * set_text_path - set text path
 */

{
    int path;

    cli_get_keyword ("Path", text_pathes, &path, stat);

    if (odd(*stat))
        {
        if (cli_end_of_command())
            {
	    GSTXP (&path);
	    }
        else
            *stat = cli__maxparm; /* maximum parameter count exceeded */
        }

}  /* set_text_path */





static void set_text_align (int *stat)

/*
 * set_text_align - set text alignment
 */

{
    int halign, valign;

    cli_get_keyword ("Horizontal alignment", hor_align, &halign, stat);

    if (odd(*stat))
        {
        if (halign == 4)
            halign = 2;

        cli_get_keyword ("Vertical alignment", ver_align, &valign, stat);

        if (odd(*stat))
            {
            if (cli_end_of_command())
                {
		GSTXAL (&halign, &valign);
		}
            else
                *stat = cli__maxparm; /* maximum parameter count exceeded */
            }
        }

}  /* set_text_align */





static void set_text_attr (int *stat)

/*
 * set_text_attr - set text attribute
 */

{
    int attribute;
    attribute_text text_attr;

    cli_get_keyword ("Attribute", text_attributes, &attribute, stat);

    text_attr = (attribute_text)attribute;

    if (odd(*stat))

        switch (text_attr) {
            case text_fontprec :
              set_text_fontprec (stat);
              break;
            case text_expfac :
              set_character_expfac (stat);
              break;
            case text_spacing :
              set_character_spacing (stat);
              break;
            case text_color_index :
              set_text_color_index (stat);
              break;
            case text_height :
              set_character_height (stat);
              break;
            case text_upvec :
              set_character_upvec (stat);
              break;
            case text_path :
              set_text_path (stat);
              break;
            case text_align :
              set_text_align (stat);
              break;
            }

}  /* set_text_attr */





static void set_fill_int_style (int *stat)

/*
 * set_fill_int_style - set fill area interior style
 */

{
    int style;

    cli_get_keyword ("Interior style", int_style, &style, stat);

    if (odd(*stat))
        {
        if (cli_end_of_command())
            {
	    GSFAIS (&style);
	    }
        else
            *stat = cli__maxparm; /* maximum parameter count exceeded */
        }

}  /* set_fill_int_style */





static void set_fill_style_index (int *stat)

/*
 * set_fill_style_index - set fill area style index
 */

{
    string style_index;
    int index;

    cli_get_parameter ("Index", style_index, " ,", FALSE, TRUE, stat);

    if (odd(*stat))

        if (cli_end_of_command())
            {
            index = cli_integer(style_index, stat);

            if (odd(*stat))
                GSFASI (&index);
            }
        else
            *stat = cli__maxparm; /* maximum parameter count exceeded */

}  /* set_fill_style_index */





static void set_fill_color_index (int *stat)

/*
 * set_fill_color_index - set fill area color index
 */

{
    int color;

    cli_get_keyword ("Color", colors, &color, stat);

    if (odd(*stat))

        if (cli_end_of_command())
            {
	    GSFACI (&color);
	    }
        else
            *stat = cli__maxparm; /* maximum parameter count exceeded */

}  /* set_fill_color_index */





static void set_fill_area_attr (int *stat)

/*
 * set_fill_area_attr - set fill area attribute
 */

{
    int attribute;
    attribute_fill fill_area_attr;

    cli_get_keyword ("Attribute", fill_area_attributes, &attribute, stat);

    fill_area_attr = (attribute_fill)attribute;

    if (odd(*stat))

        switch (fill_area_attr) {
            case fill_int_style :
		set_fill_int_style (stat);
		break;
            case fill_style_index :
		set_fill_style_index (stat);
		break;
            case fill_color_index :
		set_fill_color_index (stat);
		break;
            }

}  /* set_fill_area_attr */





static void get_rectangle_specification (float *x_min, float *x_max,
    float *y_min, float *y_max, int *stat)

/*
 * get_rectangle_specification - get a rectangle specification
 */

{
    int count;
    string rec_parm;
    float parameter[4];

    count = 0;

    while (odd(*stat) && (count < 4))
        {
        cli_get_parameter (rec_prompt[count], rec_parm, " ,", FALSE, TRUE, 
	    stat);

        if (odd(*stat))
            parameter[count] = cli_real(rec_parm, stat);

        count++;
        }

    if (!cli_end_of_command())
        *stat = cli__maxparm; /* maximum parameter count exceeded */

    if (odd(*stat))
        {
        *x_min = parameter[0]; *x_max = parameter[1];
        *y_min = parameter[2]; *y_max = parameter[3];
        }

}  /* get_rectangle_specification */





static void set_viewport (int *stat)

/*
 * set_viewport - set viewport
 */

{
    float x_min, x_max, y_min, y_max;

    get_rectangle_specification (&x_min, &x_max, &y_min, &y_max, stat);

    if (odd(*stat))
        GSVP (&tnr, &x_min, &x_max, &y_min, &y_max);

}  /* set_viewport */





static void set_window (int *stat)

/*
 * set_window - set window
 */

{
    float x_min, x_max, y_min, y_max;

    get_rectangle_specification (&x_min, &x_max, &y_min, &y_max, stat);

    if (odd(*stat))
        GSWN (&tnr, &x_min, &x_max, &y_min, &y_max);

}  /* set_window */





static void set_clipping (int *stat)

/*
 * set_clipping - set clipping
 */

{
    int switch_;

    cli_get_keyword ("Switch", clip_switch, &switch_, stat);

    if (odd(*stat))
        {
        if (cli_end_of_command())
            {
	    GSCLIP (&switch_);
	    }
        else
            *stat = cli__maxparm; /* maximum parameter count exceeded */
        }

}  /* set_clipping */





static void set_xform_type (int *stat)

/*
 * set_xform_type - select transformation number
 */

{
    int xform;

    cli_get_keyword ("Type", xform_types, &xform, stat);

    if (odd(*stat))
        {
        if (cli_end_of_command())
            {
	    GSELNT (&xform);
	    }
        else
            *stat = cli__maxparm; /* maximum parameter count exceeded */
        }

}  /* set_xform_type */





static void set_ws_viewport (int *stat)

/*
 * set_ws_viewport - set workstation viewport
 */

{
    int n, errind, ol, wkid;
    int state, conid, wtype, dcunit;
    float rx, ry, x_min, x_max, y_min, y_max;
    int i, lx, ly;

    if (cli_end_of_command())
        {
	wkid = wk_id;
	if (wkid == 0)
	    {
	    n = 1;
	    GQOPWK (&n, &errind, &ol, &wkid);
	    }

	n = wkid;
        GQWKC (&n, &errind, &conid, &wtype);
        GQDSP (&wtype, &errind, &dcunit, &rx, &ry, &lx, &ly);

        x_min = 0; x_max = rx;
        y_min = 0; y_max = ry;
        }
    else
        get_rectangle_specification (&x_min, &x_max, &y_min, &y_max, stat); 

    if (odd(*stat))
	{
        GQOPS (&state);
        if (state >= GWSAC)
            {
	    i = 1;
	    GQACWK (&i, &errind, &ol, &wkid);

	    for (i=ol; i >= 1; i--)
	        {
	        n = i;
	        GQACWK (&n, &errind, &ol, &wkid);

	        GSWKVP (&wkid, &x_min, &x_max, &y_min, &y_max);
	        }
	    }
        }

} /* set_ws_viewport */





static void set_ws_window (int *stat)

/*
 * set_ws_window - set workstation window
 */

{
    float x_min, x_max, y_min, y_max;

    get_rectangle_specification (&x_min, &x_max, &y_min, &y_max, stat);

    if (odd(*stat))
	{
	int state, i, n, errind, ol, wkid;

        GQOPS (&state);
        if (state >= GWSAC)
            {
	    i = 1;
	    GQACWK (&i, &errind, &ol, &wkid);

	    for (i=ol; i >= 1; i--)
	        {
	        n = i;
	        GQACWK (&n, &errind, &ol, &wkid);

	        GSWKWN (&wkid, &x_min, &x_max, &y_min, &y_max);
	        }
	    }
        }

} /* set_ws_window */





static void set_color_rep (int *stat)

/*
 * set_color_rep - set color representation
 */

{
    int count, index, max_colors, retlen;
    cli_data_descriptor color_parm[4];
    float color_data[4][MAX_COLORS];

    count = 0;
    max_colors = MAX_COLORS;

    while (odd(*stat) && (count < 4))
        {
        cli_get_data (color_prompt[count], &color_parm[count], stat);

        if (odd(*stat))
            {
	    cli_get_data_buffer (&color_parm[count], max_colors,
		color_data[count], &retlen, stat);
	    
	    if (odd(*stat))
		max_colors = retlen < max_colors ? retlen : max_colors;
            }
        
	count++;
        }
    
    if (cli_end_of_command())
	{
	if (odd(*stat))
	    {
	    int state, i, j, n, errind, ol, wkid;

            GQOPS (&state);
            if (state >= GWSAC)
                {
	        i = 1;
	        GQACWK (&i, &errind, &ol, &wkid);

	        for (i=ol; i >= 1; i--)
	            {
	            n = i;
	            GQACWK (&n, &errind, &ol, &wkid);

	            for (j=0; j < max_colors; j++)
		        {
		        index = (int) (color_data[0][j] + 0.5);
		        GSCR (&wkid, &index, &color_data[1][j],
                            &color_data[2][j], &color_data[3][j]);
	                }
		    }
	        }
            }
	}
    else
        *stat = cli__maxparm; /* maximum parameter count exceeded */

} /* set_color_rep */





static void set_seg_xform (int *stat)

/*
 * set_seg_xform - set segment transformation
 */

{
    int count;
    string xform_parm;

    float fixed_point_x, fixed_point_y, translation_x, translation_y, rotation;
    float scale_x, scale_y, matrix[3][2];

    count = 0;

    while (odd(*stat) && (count < 7))
        {
        count++;

        cli_get_parameter (seg_xform_prompt[count-1], xform_parm, " ,", FALSE,
            TRUE, stat);

        if (odd (*stat))

            switch (count) {

                case 1 : fixed_point_x = cli_real(xform_parm, stat); break;
                case 2 : fixed_point_y = cli_real(xform_parm, stat); break;
                case 3 : translation_x = cli_real(xform_parm, stat); break;
                case 4 : translation_y = cli_real(xform_parm, stat); break;
                case 5 : rotation = cli_real(xform_parm, stat); break;
                case 6 : scale_x = cli_real(xform_parm, stat); break;
                case 7 : scale_y = cli_real(xform_parm, stat); break;
                }
        }

    if (odd(*stat))
        {
        if (cli_end_of_command())
            {
            GEVTM (&fixed_point_x, &fixed_point_y, &translation_x,
                &translation_y, &rotation, &scale_x, &scale_y, &GNDC, matrix);
            GSSGT (&segn, matrix);
            }
        else
            *stat = cli__maxparm;  /* maximum parameter count exceeded */
        }

} /* set_seg_xform */





static void set_command (int *stat)

/*
 * set_command - evaluate set command
 */

{
    int what;
    set_option option;

    cli_get_keyword ("What", set_options, &what, stat);

    option = (set_option)what;

    if (odd(*stat))

        switch (option) {
            case attribute_asf :
		set_asf (stat);
		break;
            case primitive_polyline :
		set_polyline_attr (stat);
		break;
            case primitive_polymarker :
		set_polymarker_attr (stat);
		break;
            case primitive_text :
		set_text_attr (stat);
		break;
            case primitive_fill_area :
		set_fill_area_attr (stat);
		break;
            case xform_viewport :
		set_viewport (stat);
		break;
            case xform_window :
		set_window (stat);
		break;
            case xform_clipping :
		set_clipping (stat);
		break;
            case xform_select :
		set_xform_type (stat);
		break;
            case xform_ws_viewport :
		set_ws_viewport (stat);
		break;
            case xform_ws_window :
		set_ws_window (stat);
		break;
            case color_rep :
		set_color_rep (stat);
		break;
            case seg_xform :
		set_seg_xform (stat);
		break;
            }

}  /* set_command */





static void get_xy_spec (int *stat)

/*
 * get_xy_spec - get xy-parameter specification
 */

{
    cli_get_data ("X", &x, stat);

    if (odd(*stat))
        cli_get_data ("Y", &y, stat);

}  /* get_xy_spec */





static void get_xy_data (int max_points, float *px, float *py, int *n_points,
    int *stat)

/*
 * get_xy_data - get xy-data
 */

{
    BOOL segment;
    int key;

    segment = FALSE;

    key = x.key;
    cli_get_data_buffer (&x, max_points, px, n_points, stat);

    if (odd(*stat))
        {
        if (*stat == cli__eos)
            segment = TRUE;

        cli_get_data_buffer (&y, *n_points, py, n_points, stat);

        if (odd(*stat))
            {
            if (*stat == cli__eos)
                segment = TRUE;

            key = key + *n_points;
            if (!segment && (*n_points == max_points))
                key--;

            x.key = key;
            y.key = key;
            }
        }

}  /* get_xy_data */





static void polyline_command (int *stat)

/*
 * polyline_command - evaluate polyline command
 */

{
    int n_points;

    get_xy_spec (stat);

    if (odd(*stat))
        {
        if (cli_end_of_command())
            {
            while (odd(*stat) && !cli_b_abort)
                {
                get_xy_data (max_points, px, py, &n_points, stat);

                if (odd(*stat))
                    GPL (&n_points, px, py);
                }

            open_ws_server (update_ws);

            if (*stat == cli__nmd)
                *stat = cli__normal;
            }
        else
            *stat = cli__maxparm; /* maximum parameter count exceeded */
        }

}  /* polyline_command */





static void polymarker_command (int *stat)

/*
 * polymarker_command - evaluate polymarker command
 */

{
    int n_points;

    get_xy_spec (stat);

    if (odd(*stat))
        {
        if (cli_end_of_command())
            {
            while (odd(*stat) && !cli_b_abort)
                {
                get_xy_data (max_points, px, py, &n_points, stat);

                if (odd(*stat))
                    GPM (&n_points, px, py);
                }

            open_ws_server (update_ws);

            if (*stat == cli__nmd)
                *stat = cli__normal;
            }
        else
            *stat = cli__maxparm; /* maximum parameter count exceeded */
        }

}  /* polymarker_command */





static void text_command (int *stat)

/*
 * text_command - evaluate text command
 */

{
    string component;
    float x, y;
    string chars;
    int len;

    cli_get_parameter ("X", component, " ,", FALSE, TRUE, stat);

    if (odd(*stat))
        {
        x = cli_real(component, stat);

        if (odd(*stat))
            {
            cli_get_parameter ("Y", component, " ,", FALSE, TRUE, stat);

            if (odd(*stat))
                {
                y = cli_real(component, stat);

                if (odd(*stat))
                    {
                    cli_get_parameter ("Text", chars, "", FALSE, TRUE, stat);

                    if (odd(*stat))
                        {
			len = strlen(chars);
                        gks_text_s (&x, &y, &len, chars);
                        open_ws_server (update_ws);
                        }
                    }
                }
            }
        }

}  /* text_command */





static void fill_area_command (int *stat)

/*
 * fill_area_command - evaluate fill area command
 */

{
    int n_points;

    get_xy_spec (stat);

    if (odd(*stat))
        {
        if (cli_end_of_command())
            {
            while (odd(*stat) && (!cli_b_abort))
                {
                get_xy_data (max_points, px, py, &n_points, stat);

                if ((*stat == cli__normal) && (n_points == max_points))
                    *stat = cli__nmd;

                if (odd(*stat))
                    GFA (&n_points, px, py);
                }

            open_ws_server (update_ws);

            if (*stat == cli__nmd)
                *stat = cli__normal;
            }
        else
            *stat = cli__maxparm; /* maximum parameter count exceeded */
        }

}  /* fill_area_command */





static void cell_array_command (int *stat)

/*
 * cell_array_command - evaluate cell array command
 */

{
    string dimension_x, first_color, last_color;
    int n_points, n_dummy, max_cells;
    register int i;
    unsigned char *packed_colia;
    int *colia;

    int dimx, errind, ctnr, nx, ny, sx, sy;
    float wn[4], vp[4], dummy;
    int first, last, n;

    cli_get_data ("Array", &x, stat);

    if (odd(*stat))
	{
	cli_get_parameter ("Dimension X", dimension_x, " ,", FALSE, TRUE, stat);

	if (odd(*stat))
	    {
	    dimx = cli_integer(dimension_x, stat);

	    if (odd(*stat))
		{
	        if (!cli_end_of_command())
		    {
		    cli_get_parameter ("First Color", first_color, " ,", FALSE,
			TRUE, stat);

		    if (odd(*stat))
		        {
		        first = cli_integer(first_color, stat);

		        if (odd(*stat))
			    {
			    cli_get_parameter ("Last Color", last_color, " ,",
				FALSE, TRUE, stat);

			    if (odd(*stat))
			        last = cli_integer(last_color, stat);
			    }
		        }
		    }
		else
		    {
		    first = 0;
		    last = 65535;
		    }

		if (odd(*stat))
		    {
		    max_cells = max_points * 2;

		    cli_get_data_buffer (&x, max_cells, px, &n_points, stat);

		    if (odd(*stat))
			{
			cli_get_data_buffer (&x, 1, &dummy, &n_dummy, stat);
			if (odd(*stat))
			    /* not enough space for requested operation */
			    *stat = cli__nospace;
			else
			    *stat = cli__nmd;

			GQCNTN (&errind, &ctnr);
			GQNT (&ctnr, &errind, wn, vp);

			nx = dimx;
			ny = n_points / nx;
			n = nx * ny;
			sx = 1;
			sy = 1;

			if (packed_cell_array)
			    {
			    packed_colia = (unsigned char *) malloc (n);

			    for (i = 0; i < n; i++)
				{
				if (px[i] >= first && px[i] <= last)
				    packed_colia[i] = (unsigned char) px[i];
				else
				    packed_colia[i] = 0;
				}

			    colia = (int *) packed_colia;
			    }
			else
			    {
			    colia = (int *) malloc (n * sizeof(int));

			    for (i = 0; i < n; i++)
				{
				if (px[i] >= first && px[i] <= last)
				    colia[i] = (int) px[i];
				else
				    colia[i] = 0;
				}
			    }

			GCA (&wn[0], &wn[2], &wn[1], &wn[3], &nx, &ny,
			    &sx, &sy, &nx, &ny, colia);

			free (colia);

			open_ws_server (update_ws);
			}
		    }

		if (*stat == cli__nmd)
		    *stat = cli__normal;
		}
	    else
		*stat = cli__maxparm; /* maximum parameter count exceeded */
	    }
	}

}  /* cell_array_command */





static void get_xy_id (identifier x_id, identifier y_id, int *stat)

/*
 * get_xy_id - get xy-identifiers
 */

{

    cli_get_parameter ("X", x_id, " ,", FALSE, TRUE, stat);

    if (odd(*stat))
        {
        var_address (x_id, stat);

        if ((*stat == var__normal) || (*stat == var__undefid))
            {
            cli_get_parameter ("Y", y_id, " ,", FALSE, TRUE, stat);

            if (odd(*stat))
                {
                var_address (y_id, stat);

		if ((*stat == var__normal) || (*stat == var__undefid))
                    *stat = cli__normal;
                }
            }
        }

}  /* get_xy_id */





static void set_norm_xform()

/*
 * set_norm_xform - set up normalization transformation
 */

{
    int errind, ctnr;
    float wn[4], vp[4];

    GQCNTN (&errind, &ctnr);
    GQNT (&ctnr, &errind, wn, vp);

    xform.a = (vp[1]-vp[0])/(wn[1]-wn[0]);
    xform.b = vp[0]-wn[0]*xform.a;
    xform.c = (vp[3]-vp[2])/(wn[3]-wn[2]);
    xform.d = vp[2]-wn[2]*xform.c;

}  /* set_norm_xform */





static void update_command (int *stat)

/*
 * update_command - evaluate update workstation command
 */

{
    if (cli_end_of_command())
        active_ws_server (update_ws_perfo);
    else
        *stat = cli__maxparm; /* maximum parameter count exceeded */

} /* update_command */





static void create_sg (int *stat)

/*
 * create_sg - evaluate create segment command
 */

{
    string name;
    int state;

    if (!cli_end_of_command())
        {
	cli_get_parameter ("Name", name, " ,", FALSE, TRUE, stat);

	if (odd(*stat))
	    {
	    if (cli_end_of_command())
		segn = cli_integer(name, stat);
            else
                *stat = cli__maxparm; /* maximum parameter count exceeded */
            }
	}
    else
        segn = sg_name;

    if (odd(*stat))
        {
        GQOPS (&state);
        if (state == GSGOP)
	    GCLSG ();

        GCRSG (&segn);
        }
}




static void close_sg (int *stat)

/*
 * close_sg - evaluate close segment command
 */

{
    if (cli_end_of_command())
        {
	GCLSG ();
	}
    else
        *stat = cli__maxparm; /* maximum parameter count exceeded */
}





static void copy_sg_wk (int wkid)

/*
 * copy_sg_wk - copy segment to workstation
 */

{
    int errind, conid, wtype, wkcat;

    GQWKC (&wkid, &errind, &conid, &wtype);
    GQWKCA (&wtype, &errind, &wkcat);

    if (wkcat == GOUTPT || wkcat == GOUTIN)
	{
        GCSGWK (&wkid, &segn);
	GUWK (&wkid, &GPOSTP);
	}
}




static void copy_sg (int *stat)

/*
 * copy_sg - evaluate copy segment command
 */

{
    int state;
    string name;

    GQOPS (&state);
    if (state == GSGOP)
        GCLSG ();

    if (!cli_end_of_command())
        {
        cli_get_parameter ("Name", name, " ,", FALSE, TRUE, stat);

        if (odd(*stat))
            {
            if (cli_end_of_command())
                segn = cli_integer(name, stat);
            else
                *stat = cli__maxparm; /* maximum parameter count exceeded */
            }
        }
    else
        segn = sg_name;

    if (odd(*stat))
        active_ws_server (copy_sg_wk);
}





static void request_locator (int *stat)

/*
 * request_locator - evaluate request locator command
 */

{
    int inp_dev_stat, ctnr, ignore;
    identifier x_id, y_id;
    int n, errind, ol, wkid;
    float px, py;
    string str;

    get_xy_id (x_id, y_id, stat);

    if (odd(*stat))
        {
        if (cli_end_of_command())
	    {
	    wkid = wk_id;
	    if (wkid == 0)
		{
		n = 1;
		GQOPWK (&n, &errind, &ol, &wkid);
		}

	    GRQLC (&wkid, &lcdnr, &inp_dev_stat, &ctnr, &px, &py);

	    open_ws_server (update_ws);

	    if (inp_dev_stat == GOK)
		{
		set_norm_xform ();

		px = (px-xform.b)/xform.a;
		py = (py-xform.d)/xform.c;

		fun_delete (x_id, &ignore);
		fun_define (x_id, str_flt(str, px), NIL);

		fun_delete (y_id, &ignore);
		fun_define (y_id, str_flt(str, py), NIL);
		}
            }
	else
	    *stat = cli__maxparm; /* maximum parameter count exceeded */
        }

}  /* request_locator */





static void request_stroke (int *stat)

/*
 * request_stroke - evaluate request stroke command
 */

{
    int inp_dev_stat, ctnr, np, ip, ignore;
    identifier x_id, y_id;
    int n, errind, ol, wkid;

    get_xy_id (x_id, y_id, stat);

    if (odd(*stat))
        {
        if (cli_end_of_command())
	    {
	    wkid = wk_id;
	    if (wkid == 0)
		{
		n = 1;
		GQOPWK (&n, &errind, &ol, &wkid);
		}

	    ip = max_points;
	    GRQSK (&wkid, &skdnr, &ip, &inp_dev_stat, &ctnr, &np, px, py);

	    open_ws_server (update_ws);

	    if (inp_dev_stat == GOK)
		{
		set_norm_xform ();

		for (ip=0; ip < np; ip++)
		    {
		    px[ip] = (px[ip]-xform.b)/xform.a;
		    py[ip] = (py[ip]-xform.d)/xform.c;
		    }

		var_delete (x_id, &ignore);
		var_define (x_id, 0, np, px, FALSE, NIL);

		var_delete (y_id, &ignore);
		var_define (y_id, 0, np, py, FALSE, NIL);
		}
            }
	else
	    *stat = cli__maxparm; /* maximum parameter count exceeded */
        }

}  /* request_stroke */





static void request_command (int *stat)

/*
 * request_command - evaluate request command
 */

{
    int option;
    input_class inp_class;

    cli_get_keyword ("Input class", input_classes, &option, stat);

    inp_class = (input_class)option;

    if (odd(*stat))

        switch (inp_class) {
            case class_locator :
		request_locator (stat);
		break;
            case class_stroke :
		request_stroke (stat);
		break;
            }

}  /* request_command */





char *time_string ()

/*
 * time_string - return todays date and current time
 */

{
    time_t elapsed_time;

    time (&elapsed_time);

    return ( ctime(&elapsed_time) );

}  /* time_string */





static void inquire_gks_state (int *stat)

/*
 * inquire_gks_state - inquire current GKS state
 */

{
    static char *ltypes[12] = {
	"Dash, 2 Dots", "Dash, 3 Dots", "Long Dash", "Long, short Dash",
	"Spaced Dash", "Spaced Dot", "Double Dots", "Triple Dots",
	"Solid", "Dashed", "Dotted", "Dashed-dotted"};

    static char *colors[8] = {
	"White", "Black", "Red", "Green", "Blue", "Cyan", "Yellow", "Magenta"};
  
    static char *mtypes[25] = {
	"Solid Circle", "Triangle up", "Solid Triangle up",
	"Triangle down", "Solid Triangle down", "Square",
	"Solid Square", "Bowtie", "Solid Bowtie", "Hourglass",
	"Solid Hourglass", "Diamond", "Solid Diamond", "Star",
	"Solid Star", "Triangle up down", "Solid Triangle left",
	"Solid Triangle right", "Hollow Plus", "O-Marker", "Dot",
	"Plus", "Asterisk", "Circle", "Diagonal Cross"};
 
    static char *fonts[55] = {
	"Standard", "Mathematical", "Simplex Roman", "Simplex Greek",
	"Simplex Script", "Complex Roman", "Complex Greek", "Italic",
	"Triplex Roman", "Triplex Greek", "Triplex Italic", "Duplex Roman",
	"Complex Script", "Complex Cyrillic", "Complex Roman", "Complex Italic",
	"Gothic German", "Gothic English", "Gothic Italian",
	"Scientific", "Symbolic", "Cartographic", "",
	"Avant Garde Gothic Book", "Courier Medium", "Helvetica Medium",
	"Lubalin Graph Book", "New Century Schoolbook Medium",
	"Souvenir Light", "Symbol Medium", "Times Medium",
	"Avant Garde Gothic Demi", "Courier Bold", "Helvetica Bold",
	"Lubalin Graph Demi", "New Century Schoolbook Bold",
	"Souvenir Demi", "Symbol Medium", "Times Bold",
	"Avant Garde Gothic Book Oblique", "Courier Oblique",
	"Helvetica Oblique", "Lubalin Graph Book Oblique",
	"New Century Schoolbook Italic", "Souvenir Light Italic",
	"Symbol Medium", "Times Italic",
	"Avant Garde Gothic Demi Oblique", "Courier Bold Oblique",
	"Helvetica Bold Oblique", "Lubalin Graph Demi Oblique",
	"New Century Schoolbook Bold Italic", "Souvenir Demi Italic",
	"Symbol Medium", "Times Bold Italic"
	};

    static char *precs[3] = {"String", "Character", "Stroke"};
    static char *path[4] = {"Right", "Left", "Up", "Down"};    
    static char *hal[4] = {"Normal", "Left", "Center", "Right"};
    static char *val[6] = {"Normal", "Top", "Cap", "Half", "Base", "Bottom"};

    static char *styles[4] = {"Hollow", "Solid", "Pattern", "Hatch"};        

    static char *clip[2] = {"off", "on"};
                             
    int errind, ltype, plcoli, pmcoli, facoli, txcoli;
    int mtype, font, prec, txp, alh, alv, ints, styli, clsw, tnr;
    float lwidth, mszsc, chxp, chsp, chh, chux, chuy;
    float wn[4], vp[4], clrt[4];
      
    if (cli_end_of_command())
        {
	GQLN (&errind, &ltype);
	ltype = (ltype < 0) ? -ltype - 1 : ltype + 7;
	GQLWSC (&errind, &lwidth);
	GQPLCI (&errind, &plcoli);

	GQMK (&errind, &mtype);
	mtype = (mtype < 0) ? -mtype - 1 : mtype + 19;
	GQMKSC (&errind, &mszsc);
	GQPMCI (&errind, &pmcoli);
       
	GQTXFP (&errind, &font, &prec);
	font = (font < 0) ? -font - 1 : font + 22;
	GQCHXP (&errind, &chxp);
	GQCHSP (&errind, &chsp);  
	GQTXCI (&errind, &txcoli);
	GQCHH (&errind, &chh);
	GQCHUP (&errind, &chux, &chuy);
	GQTXP (&errind, &txp);
	GQTXAL (&errind, &alh, &alv);
	
	GQFAIS (&errind, &ints);
	GQFASI (&errind, &styli);
	GQFACI (&errind, &facoli);

	GQCNTN (&errind, &tnr);
	GQCLIP (&errind, &clsw, clrt);
	GQNT (&tnr, &errind, wn, vp);

        tt_printf ("GKS state on %s\n", time_string());
	tt_printf ("      POLYLINE  Linetype: %s\n", ltypes[ltype]);
	tt_printf ("                Linewidth scale factor: %2.2f\n", lwidth);
	tt_printf ("                Color: %s\n", colors[plcoli]);
	tt_printf ("    POLYMARKER  Marker Type: %s\n", mtypes[mtype]);
	tt_printf ("                Marker Size Scale Factor: %2.2f\n", mszsc);
	tt_printf ("                Color: %s\n", colors[pmcoli]);
	tt_printf ("          TEXT  Font: %s\n", fonts[font]);
	tt_printf ("                Precision: %s\n", precs[prec]);
	tt_printf ("                Expansion factor: %2.2f\n", chxp);
	tt_printf ("                Spacing factor: %2.2f\n", chsp);
	tt_printf ("                Color: %s\n", colors[txcoli]);
	tt_printf ("                Height: %1.3f\n", chh);
	tt_printf ("                Text Up Vector: (%1.2f, %1.2f)\n",
	   chux, chuy);
	tt_printf ("                Path: %s\n", path[txp]);
	tt_printf ("                Alignment: (%s, %s)\n",hal[alh], val[alv]);
	tt_printf ("     FILL AREA  Interior Style: %s\n", styles[ints]);
	tt_printf ("                Style Index: %d\n", styli);
	tt_printf ("                Color: %s\n", colors[facoli]);
	tt_printf ("TRANSFORMATION  Number: %d,  Clipping: %s\n", tnr,
	    clip[clsw]);
	tt_printf ("                Viewport: (%g, %g, %g, %g)\n", vp[0], vp[1],
	    vp[2], vp[3]);
	tt_printf ("                Window: (%g, %g, %g, %g)\n", wn[0], wn[1],
	    wn[2], wn[3]);
        }
    else
        *stat = cli__maxparm; /* maximum parameter count exceeded */

}  /* inquire_gks_state */




static void inquire_text_extent (int *stat)

/*
 * inquire_text_extent - evaluate inquire text extent command
 */

{
    identifier x_id, y_id;
    string component;
    float x, y, cpx, cpy, tx[4], ty[4];
    string chars;
    int n, errind, ol, wkid;
    int len, ignore;

    get_xy_id (x_id, y_id, stat);

    if (odd(*stat))
	{
	cli_get_parameter ("X", component, " ,", FALSE, TRUE, stat);

	if (odd(*stat))
	    {
	    x = cli_real(component, stat);

	    if (odd(*stat))
		{
	        cli_get_parameter ("Y", component, " ,", FALSE, TRUE, stat);

	        if (odd(*stat))
	            {
	            y = cli_real(component, stat);

	            if (odd(*stat))
	                {
                	cli_get_parameter ("Text", chars, "", FALSE, TRUE,
			    stat);

			if (odd(*stat))
			    {
			    wkid = wk_id;
			    if (wkid == 0)
				{
				n = 1;
				GQOPWK (&n, &errind, &ol, &wkid);
				}

			    len = strlen(chars);
			    gks_inq_text_extent_s (&wkid, &x, &y, &len, chars,
				&errind, &cpx, &cpy, tx, ty);

			    if (errind == 0)
				{
				var_delete (x_id, &ignore);
				var_define (x_id, 0, 4, tx, FALSE, NIL);

				var_delete (y_id, &ignore);
				var_define (y_id, 0, 4, ty, FALSE, NIL);
				}
			    }
                        }
                    }
                }
            }
        }

}  /* inquire_text_extent */




static void inquire_ws_type (int *stat)

/*
 * inquire_ws_type - inquire list element of available workstation types
 */

{
    int i, n, errind, ol, wtype;
    int dcunit, lx, ly, wkcat;
    float rx, ry;

    if (cli_end_of_command())
        {
        tt_printf ("GKS available workstation types on %s\n", time_string());
        tt_printf ("\n");
        tt_printf ("    Type  Category     Size       Unit   Device Units\n");
        tt_printf ("\n");

	i = 1;
        GQEWK (&i, &errind, &ol, &wtype);

        for (i=1; i <= ol; i++)
            {
            n = i;
            GQEWK (&n, &errind, &ol, &wtype);
            GQWKCA (&wtype, &errind, &wkcat);
            GQDSP (&wtype, &errind, &dcunit, &rx, &ry, &lx, &ly);

            if (errind == 0)
                {
                if (dcunit == GMETRE)
                    tt_printf ("%8d  %4d  %8.3f,%8.3f  cm    %6d, %d\n", 
			wtype, wkcat, rx*100, ry*100, lx, ly);
                else
                    tt_printf ("%8d  %4d%26s%6d, %d\n", 
			wtype, wkcat, " ", lx, ly);
                }
            else
                tt_printf ("%8d  %4d\n", wtype, wkcat);
            }
        tt_printf ("\n");
        tt_printf ("              Total of %d workstation%s.\n", ol,
	    pl_s[ol>1]);
        }
    else
        *stat = cli__maxparm; /* maximum parameter count exceeded */

}  /* inquire_ws_type */





static void inquire_ws_conntype (int *stat)

/*
 * inquire_ws_conntype - inquire workstation connection and type
 */

{
    int state, i, n, errind, ol, wkid;
    int conid, wtype;

    if (cli_end_of_command())
        {
        GQOPS (&state);

        if (state >= GWSOP)
            {
            tt_printf ("GKS Workstation connections on %s\n", time_string());
            tt_printf ("\n");
            tt_printf ("      Workstation ID  Stream ID       Type\n");
            tt_printf ("\n");

	    i = 1;
	    GQOPWK (&i, &errind, &ol, &wkid);

            n = 0;
	    for (i=ol; i >= 1; i--)
	        {
	        GQOPWK (&i, &errind, &ol, &wkid);

	        n++;
	        GQWKC (&wkid, &errind, &conid, &wtype);
	        if (errind == 0)
		    tt_printf ("%14d%14d%14d\n", wkid, conid, wtype);
	        }

            tt_printf ("\n");
            tt_printf ("              Total of %d workstation%s.\n", n,
		pl_s[n>1]);
            }
        else
            tt_printf ("  No open workstations found.\n");
        }
    else
        *stat = cli__maxparm; /* maximum parameter count exceeded */

} /* inquire_ws_conntype */






static void inquire_command (int *stat)

/*
 * inquire_command - evaluate inquire command
 */

{
    int option;
    inquire_option inq_option;

    cli_get_keyword ("What", inquire_options, &option, stat);

    inq_option = (inquire_option)option;

    if (odd(*stat))

        switch (inq_option) {
            case option_gks_state :
		inquire_gks_state (stat);
		break;
            case option_text_extent :
		inquire_text_extent (stat);
		break;
            case option_ws_conntype :
		inquire_ws_conntype (stat);
		break;
            case option_ws_type :
		inquire_ws_type (stat);
		break;
            }

}  /* inquire_command */





static void emergency_close (int *stat)

/*
 * emergency_close - emergency close GKS
 */

{
    if (cli_end_of_command())
        {
	GECLKS ();
	}
    else
        *stat = cli__maxparm; /* maximum parameter count exceeded */

}  /* emergency_close */





#if !defined (cray) && !(defined (_WIN32) && !defined (__GNUC__))
#if defined (VMS) || ((defined (hpux) || defined (aix)) && !defined(NAGware))
#define GKSTST gkstst
#else
#define GKSTST gkstst_
#endif
#endif /* cray */

#if defined (_WIN32) && !defined (__GNUC__)
extern void __stdcall GKSTST();
#endif



static void test (int *stat)

/*
 * test - test GKS
 */

{
    if (cli_end_of_command())
        {
	GKSTST ();
	}
    else
        *stat = cli__maxparm; /* maximum parameter count exceeded */

}  /* test */





void do_gks_command (int *stat)

/*
 * command - parse a GKS command
 */

{
    int index;
    gks_command command;

    cli_get_keyword ("GKS", gks_command_table, &index, stat);

    command = (gks_command)index;

    if (odd(*stat))

        switch (command) {
            case command_open_gks :
		open_gks (stat);
		break;
            case command_close_gks :
		close_gks (stat);
		break;
            case command_open_ws :
		open_workstation (stat);
		break;
            case command_close_ws :
		close_workstation (stat);
		break;
            case command_activate_ws :
		activate_workstation (stat);
		break;
            case command_deactivate_ws :
		deactivate_workstation (stat);
		break;
            case command_clear_ws :
		clear_workstation (stat);
		break;
            case command_update_ws :
		update_command (stat);
		break;
            case command_set :
		set_command (stat);
		break;
            case command_polyline :
		polyline_command (stat);
		break;
            case command_polymarker :
		polymarker_command (stat);
		break;
            case command_text :
		text_command (stat);
		break;
            case command_fill_area :
		fill_area_command (stat);
		break;
            case command_cell_array :
		cell_array_command (stat);
		break;
            case command_create_sg :
		create_sg (stat);
		break;
            case command_close_sg :
		close_sg (stat);
		break;
            case command_copy_sg :
		copy_sg (stat);
		break;
            case command_request :
		request_command (stat);
		break;
            case command_inquire :
		inquire_command (stat);
		break;
            case command_emergency_close :
		emergency_close (stat);
		break;
            case command_test :
		test (stat);
		break;
            }

    if (STATUS_FAC_NO(*stat) == gks__facility)
	*stat = gks__normal;

}  /* do_gks_command */
