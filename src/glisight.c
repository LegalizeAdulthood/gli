/*
 *
 * FACILITY:
 *
 *	Graphics Language Interpreter (GLI)
 *
 * ABSTRACT:
 *
 *	This module contains a Simple Interactive Graphics Handling Tool
 *	(Sight) command language interpreter.
 *
 * AUTHOR:
 *
 *	Josef Heinen
 *	Matthias Steinert 
 *
 * VERSION:
 *
 *	V1.3
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#if !defined(VMS) && !defined(MSDOS) && !defined(_WIN32)
#include <unistd.h>
#endif

#if defined (cray) || defined (__SVR4) || defined (MSDOS) || defined (_WIN32)
#include <fcntl.h>
#else
#include <sys/types.h>
#include <sys/file.h>
#endif

#ifdef TCL
#include <tcl.h>
#endif

#include "system.h"
#include "terminal.h"
#include "function.h"
#include "variable.h"
#include "command.h"
#include "strlib.h"
#include "symbol.h"
#include "gksdefs.h"
#include "gus.h"
#include "image.h"
#include "sight.h"


#define max_curves 	    16	    /* maximal number of curves in autoscale 
				       command */

#define EPS		    (float)1.0e-5
#define BOOL		    int
#define TRUE		    1
#define FALSE		    0
#define NIL		    0	
#define IN		    &
 
#ifndef min
#define min(x1,x2)          (((x1)<(x2)) ? (x1) : (x2))
#endif
#ifndef max
#define max(x1,x2)          (((x1)>(x2)) ? (x1) : (x2))
#endif
#define odd(status)	    ((status) & 01)

#define WindowWidth         686         /* window width */
#define WindowHeight        485         /* window height */
#define Ratio               1.41421

#define NominalMarkerSize   6           /* nominal marker size */
#define CapSize             0.75        /* size of capital letters */


typedef char identifier[32];
typedef char string[255];
typedef char logical_name[32];
typedef char file_specification[80];


/* Sight commands */

typedef enum { command_open, command_close, command_clear, 
    command_select, command_deselect, command_cut, command_paste, 
    command_pop_in_front, command_push_behind, command_set, 
    command_open_segment, command_close_segment, command_plot, command_polyline,
    command_polymarker, command_error_bars, command_fill_area,
    command_image, command_bar_graph, command_text, command_spline,
    command_remove, command_move, command_redraw, command_axes, command_grid,
    command_open_drawing, command_close_drawing, command_create_drawing,
    command_capture, command_print, command_pick_object, command_pick_element,
    command_modify_object, command_modify_element, command_pick_region, 
    command_inquire, command_request_locator, command_autoscale, 
    command_align } sight_command;

typedef enum { option_object, option_all, option_excluded, option_group, 
    option_next, option_previous, option_last, option_xform } select_option; 

typedef enum { inq_xform } inquire_option;

typedef enum { pline_type, pline_width, pline_color, pmark_type, pmark_size,
    pmark_color, fill_style, fill_index, fill_color, text_font, text_size, 
    text_halign, text_valign, text_direction, text_color, image_size, 
    image_position, window, viewport, x_tick, y_tick, x_origin, y_origin,
    major_x, major_y, tick_size, orientation_option, scale_option, smoothing,
    clipping, update, snapping
    } set_option;

static cli_verb_list sight_command_table = "open_sight close_sight clear\
 select deselect cut paste pop_in_front push_behind set open_segment\
 close_segment plot polyline polymarker error_bars fill_area image\
 bar_graph text spline remove move redraw axes grid open_drawing close_drawing\
 create_drawing capture print pick_object pick_element modify_object\
 modify_element pick_region inquire request_locator autoscale align";

static cli_verb_list set_options = "linetype linewidth linecolor\
 markertype markersize markercolor fillstyle fillindex fillcolor\
 textfont textsize texthalign textvalign textdirection textcolor\
 imagesize imageposition window viewport x_tick y_tick x_origin y_origin\
 major_x major_y tick_size orientation scale smoothing clipping update\
 snap_grid";

static cli_verb_list select_options = "object all excluded group next\
 previous last xform";

static cli_verb_list inquire_options = "xform";

static cli_verb_list error_bar_options = "vertical horizontal";

static cli_verb_list orientation_options = "portrait landscape";

static cli_verb_list scale_options = "linear x_log y_log xy_log current";

static char *scale[] = {
    "linear", "x_log", "y_log", "xy_log" };

static cli_verb_list flip_options = "none flip_x flip_y flip_xy current";

static char *flip[] = {
    "none", "flip_x", "flip_y", "flip_xy" };

static cli_verb_list clip_options = "off on";

static cli_verb_list update_options = "conditionally always";

static cli_verb_list colors = "white black red green blue cyan yellow magenta";

static char *color[] = {
    "white", "black", "red", "green", "blue", "cyan", "yellow", "magenta",
    "n/a" };

static cli_verb_list line_types = "solid dashed dotted dash_dotted dash_2_dot\
 dash_3_dot long_dash long_short_dash spaced_dash spaced_dot double_dot\
 triple_dot";

static char *ltype[] = {
    "triple_dot", "double_dot", "spaced_dot", "spaced_dash", "long_short_dash",
    "long_dash", "dash_3_dot", "dash_2_dot",
    "", "solid", "dashed", "dotted", "dash_dotted" };

static cli_verb_list line_widths = "1pt 2pt 3pt 4pt 5pt 6pt";

static int av_ltype[] = { 1, 2, 3, 4, -1, -2, -3, -4, -5, -6, -7, -8 };

static cli_verb_list marker_types = "dot plus asterisk circle diagonal_cross\
 solid_circle triangle_up solid_tri_up triangle_down solid_tri_down square\
 solid_square bowtie solid_bowtie hourglass solid_hglass diamond solid_diamond\
 star solid_star tri_up_down solid_tri_left solid_tri_right hollow_plus omark";

static char *mtype[] = {
    "omark", "hollow_plus", "solid_tri_right", "solid_tri_left",
    "tri_up_down", "solid_star", "star", "solid_diamond", "diamond",
    "solid_hglass", "hourglass", "solid_bowtie", "bowtie", "solid_square",
    "square", "solid_tri_down", "triangle_down", "solid_tri_up",
    "triangle_up", "solid_circle",
    "", "dot", "plus", "asterisk", "circle", "diagonal_cross" };

static int av_mtype[] = { 1, 2, 3, 4, 5, -1, -2, -3, -4, -5, -6, -7, -8, 
    -9, -10, -11, -12, -13, -14, -15, -16, -17, -18, -19, -20 };

static cli_verb_list marker_sizes = "4pt 6pt 8pt 10pt 12pt 14pt 18pt 24pt\
 36pt 48pt 72pt";

static float marker_pitches[] = { 4, 6, 8, 10, 12, 14, 18, 24, 36, 48, 72 };

static cli_verb_list int_styles = "hollow solid pattern hatch";

static char *intstyle[] = {
    "hollow", "solid", "pattern", "hatch" };

static cli_verb_list text_fonts = "avant_garde courier helvetica lubalin\
 schoolbook souvenir symbol times";

static char *font[] = {
    "avant_garde", "courier", "helvetica", "lubalin", "schoolbook", "souvenir",
    "symbol", "times" };

static cli_verb_list text_font_types = "normal boldface italic bold_italic";

static char *fonttype[] = {
    "normal", "boldface", "italic", "bold_italic" };

static cli_verb_list text_sizes = "8pt 10pt 12pt 14pt 18pt 24pt";

static float text_pitches[] = { 8, 10, 12, 14, 18, 24 };

static cli_verb_list text_directions = "right up left down";

static char *direction[] = {
    "right", "up", "left", "down" };

static cli_verb_list text_haligns = "normal left centre right center";

static char *halign[] = {
    "normal", "left", "center", "right" };

static cli_verb_list text_valigns = "normal top cap half base bottom";

static char *valign[] = {
    "normal", "top", "cap", "half", "base", "bottom" };

static char *orientation[] = {
    "portrait", "landscape" };

static char *rec_prompt[] = { "X-min", "X-max", "Y-min", "Y-max" };

static char *axes_prompt[] = { "X-tick", "Y-tick", "X-origin", "Y-origin",
			       "Major-X-count", "Major-Y-count", "Tick-size" };

static char *image_prompt[] = { "X-start", "Y-start", "X-size", "Y-size",
				"X-min", "X-max", "Y-min", "Y-max" };

static file_specification def_spec = "sight";

static cli_data_descriptor x,y, e1,e2, x_list[max_curves], y_list[max_curves];

extern int max_points;
extern float *px, *py, *pe1, *pe2;

extern SightGC gc;
extern BOOL sight_open, sight_gui_open;
extern int sight_orientation;

#ifdef TCL
extern Tcl_Interp *gli_tcl_interp;
extern char *sight_info_cb;
#endif


static void open_sight (int *stat)

/*
 * open_sight - open Sight 
 */

{
    if (cli_end_of_command())
        SightOpen (NIL);
    else
        *stat = cli__maxparm; /* maximum parameter count exceeded */

}  /* open_sight */



static void close_sight (int *stat)

/*
 * close_sight - close Sight 
 */

{
    if (cli_end_of_command())
        SightClose (NULL);
    else
        *stat = cli__maxparm; /* maximum parameter count exceeded */

}  /* close_sight */



static void sight_info (char *str)

{
#ifdef TCL
    char command[255];

    if (gli_tcl_interp && sight_info_cb)
	{
	sprintf (command, "%s \"%s\"", sight_info_cb, str);
	if (Tcl_Eval (gli_tcl_interp, command) != TCL_OK)
	    tt_fprintf (stderr, "%s\n", gli_tcl_interp->result);
	}
    else
#endif
	sym_define ("SIGHT_INFO", str, NIL);
}


static void clear_command ()

/*
 * clear_command - clear workstation
 */

{
    SightClear ();
    sight_info ("clear");

} /* clear_command */




static void cut_command (int *stat)

/*
 * cut_command - cut all selected objects in the paste buffer
 */

{
    string component;
    float x, y;

    if (cli_end_of_command()) {
	SightRequestLocatorNDC (&x, &y, TRUE);
	SightCut (x, y);
	sight_info ("deselect");
	}
    else
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
			if (cli_end_of_command())
			    {
			    SightCut (x, y);
			    sight_info ("deselect");
			    }
			else
			    /* maximum parameter count exceeded */
			    *stat = cli__maxparm;
			}
		    }
		}
	    }
	}

}  /* cut_command */



static void paste_command (int *stat)

/*
 * paste_command - paste all cutted objects 
 */

{
    string component;
    float x, y;

    if (cli_end_of_command()) {
	SightRequestLocatorNDC (&x, &y, TRUE);
	SightPaste (x, y);
	}
    else
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
			if (cli_end_of_command())
			    SightPaste (x, y);
			else
			    /* maximum parameter count exceeded */
			    *stat = cli__maxparm;
			}
		    }
		}
	    }
	}
	

}  /* paste_command */



static void pop_in_front (int *stat)

/*
 * pop_in_front - pop all selected objects in front 
 */

{
    if (cli_end_of_command())
	SightPopInFront ();
    else
	*stat = cli__maxparm; /* maximum parameter count exceeded */

}  /* pop_in_front */



static void push_behind (int *stat)

/*
 * push_behind - push all selected in the background
 */

{
    if (cli_end_of_command())
	SightPushBehind ();
    else
	*stat = cli__maxparm; /* maximum parameter count exceeded */

}  /* push_behind */



static void remove_command (int *stat)

/*
 * remove_command - removes all selected objects
 */

{
    if (cli_end_of_command())
	{
	SightDelete ();
	sight_info ("deselect");
	}
    else
	*stat = cli__maxparm; /* maximum parameter count exceeded */

}  /* remove_command */



static void move_command (int *stat)

/*
 * move_command - move all selected objects wich use ndc coordinates
 */

{
    string component;
    float x, y;

    if (cli_end_of_command()) {
	SightSampleLocator (&x, &y, TRUE);
	SightMove (x, y);
	}
    else
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
			if (cli_end_of_command())
			    SightMove (x, y);
			else
			    /* maximum parameter count exceeded */
			    *stat = cli__maxparm;
			}
		    }
		}
	    }
	}

}  /* move_command */



static void set_pline_type (int *stat)

/*
 * set_pline_type - set polyline linetype
 */

{
    int linetype;

    cli_get_keyword ("Type", line_types, &linetype, stat);

    if (odd(*stat))
        {
        if (cli_end_of_command())
            SightSetAttribute (LineType, (caddr_t)&av_ltype[linetype]);
        else
            *stat = cli__maxparm; /* maximum parameter count exceeded */
        }

}  /* set_pline_type */



static void set_pline_width (int *stat)

/*
 * set_pline_width - set polyline linewidth
 */

{
    int size;
    float width;


    cli_get_keyword ("Width", line_widths, &size, stat);

    if (odd(*stat))
	{
	if (cli_end_of_command()) {
	    width = size+1;
	    SightSetAttribute (LineWidth, (caddr_t)&width);
	    }
	else
	    *stat = cli__maxparm; /* maximum parameter count exceeded */
	}

}  /* set_pline_width */



static void set_pline_color (int *stat)

/*
 * set_pline_color - set polyline color index
 */

{
    int color;

    cli_get_keyword ("Color", colors, &color, stat);

    if (odd(*stat))

        if (cli_end_of_command())
            SightSetAttribute (LineColor, (caddr_t)&color);
        else
            *stat = cli__maxparm; /* maximum parameter count exceeded */

}  /* set_pline_color */



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
            SightSetAttribute (MarkerType, (caddr_t)&av_mtype[marker]);
        else
            *stat = cli__maxparm; /* maximum parameter count exceeded */
        }

}  /* set_pmark_type */



static void set_pmark_size (int *stat)

/*
 * set_pmark_size - set polymarker size
 */

{
    int size;

    cli_get_keyword ("Size", marker_sizes, &size, stat);

    if (odd(*stat))
	{
	if (cli_end_of_command()) 
	    SightSetAttribute (MarkerSize, (caddr_t)&marker_pitches[size]);
	else
	    *stat = cli__maxparm; /* maximum parameter count exceeded */
	}

}  /* set_pmark_size */



static void set_pmark_color (int *stat)

/*
 * set_pmark_color - set polymarker color index
 */

{
    int color;

    cli_get_keyword ("Color", colors, &color, stat);

    if (odd(*stat))
        if (cli_end_of_command())
            SightSetAttribute (MarkerColor, (caddr_t)&color);
        else
            *stat = cli__maxparm; /* maximum parameter count exceeded */

}  /* set_pmark_color */



static void set_text_font (int *stat)

/*
 * set_text_font - set text font
 */

{
    int font;
    int font_type;

    cli_get_keyword ("Font", text_fonts, &font, stat);

    if (odd(*stat))
	{
	font++;

	if (cli_end_of_command())
	    SightSetAttribute (TextFont, (caddr_t)&font);
	else
	    {
	    cli_get_keyword ("Type", text_font_types, &font_type, stat);
	    
	    if (odd(*stat))
		{
		font += 8*font_type;

	    	if (cli_end_of_command())
		    SightSetAttribute (TextFont, (caddr_t)&font);
		else
		    *stat = cli__maxparm; /* maximum parameter count exceeded */
		}
	    }
	}

}  /* set_text_font */



static void set_text_color (int *stat)

/*
 * set_text_color - set text color index
 */

{
    int color;

    cli_get_keyword ("Color", colors, &color, stat);

    if (odd(*stat))

        if (cli_end_of_command())
            SightSetAttribute (TextColor, (caddr_t)&color);
        else
            *stat = cli__maxparm; /* maximum parameter count exceeded */

}  /* set_text_color */



static void set_text_size (int *stat)

/*
 * set_text_size - set text size 
 */

{
    int size;

    cli_get_keyword ("Size", text_sizes, &size, stat);

    if (odd(*stat))
	{
	if (cli_end_of_command())
	    SightSetAttribute (TextSize, (caddr_t)&text_pitches[size]);
	else
	    *stat = cli__maxparm; /* maximum parameter count exceeded */
	}

}  /* set_text_size */



static void set_text_direction (int *stat)

/*
 * set_text_direction - set text-up-vector
 */

{
    int direction;

    cli_get_keyword ("Direction", text_directions, &direction, stat);

    if (odd(*stat))
	{
	if (cli_end_of_command())
	    SightSetAttribute (TextDirection, (caddr_t)&direction);
	else
	    *stat = cli__maxparm; /* maximum parameter count exceeded */
	}

}  /* set_text_direction */



static void set_text_halign (int *stat)

/*
 * set_text_halign - set text horizontal alignment
 */

{
    int halign;

    cli_get_keyword ("Horizontal alignment", text_haligns, &halign, stat);

    if (odd(*stat))
        {
        if (halign == 4)
            halign = 2;

	if (cli_end_of_command())
	    SightSetAttribute (TextHalign, (caddr_t)&halign);
	else
	    *stat = cli__maxparm; /* maximum parameter count exceeded */
        }

}  /* set_text_halign */



static void set_text_valign (int *stat)

/*
 * set_text_valign - set text vertical alignment
 */

{
    int valign;

    cli_get_keyword ("Vertical alignment", text_valigns, &valign, stat);

    if (odd(*stat))
        {
	if (cli_end_of_command())
	    SightSetAttribute (TextValign, (caddr_t)&valign);
	else
	    *stat = cli__maxparm; /* maximum parameter count exceeded */
        }

}  /* set_text_valign */



static void set_image_size (int *stat)


/*
 * set_image_size - set image size
 */

{
    string parm;
    int size[4];
    int i;
    
    for (i = 0; i < 4; i++)
	{
        cli_get_parameter (image_prompt[i], parm, " ,", FALSE, TRUE, stat);
	if (odd(*stat))
	    size[i] = cli_integer(parm, stat);

	if (!odd(*stat))
	    break;
	}

    if (size[0] < 0 || size[2] < 0 ||
	size[1] < 1 || size[3] < 1)
	*stat = SIGHT__INVARG;

    if (odd(*stat))
	{
        if (cli_end_of_command())
            SightSetImageSize (size);
        else
            *stat = cli__maxparm; /* maximum parameter count exceeded */
	}

}  /* set_image_size */



static void set_image_position (int *stat)


/*
 * set_image_position - set image position
 */

{
    string parm;
    float position[4];
    int i;
    
    for (i = 0; i < 4; i++)
	{
        cli_get_parameter (image_prompt[i+4], parm, " ,", FALSE, TRUE, stat);
	if (odd(*stat))
	    position[i]= cli_real(parm, stat);

	if (!odd(*stat))
	    break;
	}

    if (position[0] < 0 || position[1] > 1 ||
	position[2] < 0 || position[3] > 1 ||
	position[0] >= position[1] || position[2] >= position[3])
	*stat = SIGHT__INVARG;

    if (odd(*stat))
	{
        if (cli_end_of_command())
            SightSetImagePosition (position);
        else
            *stat = cli__maxparm; /* maximum parameter count exceeded */
	}

}  /* set_image_position */




static void set_fill_style (int *stat)


/*
 * set_fill_style - set fill area interior style
 */

{
    int style;

    cli_get_keyword ("Interior style", int_styles, &style, stat);

    if (odd(*stat))
        {
        if (cli_end_of_command())
            SightSetAttribute (FillStyle, (caddr_t)&style);
        else
            *stat = cli__maxparm; /* maximum parameter count exceeded */
        }

}  /* set_fill_style */



static void set_fill_index (int *stat)

/*
 * set_fill_index - set fill area style index
 */

{
	string style_index;
	int index;

    cli_get_parameter ("Style index", style_index, " ,", FALSE, TRUE, stat);

    if (odd(*stat))
        {
        if (cli_end_of_command())
	    {
	    index = cli_integer (style_index, stat);	    

	    if (odd(*stat))
		SightSetAttribute (FillIndex, (caddr_t)&index);
	    }
        else
            *stat = cli__maxparm; /* maximum parameter count exceeded */
        }

}  /* set_fill_index */



static void set_fill_color (int *stat)

/*
 * set_fill_color - set fill area color index
 */

{
    int color;

    cli_get_keyword ("Color", colors, &color, stat);

    if (odd(*stat))

        if (cli_end_of_command())
            SightSetAttribute (FillColor, (caddr_t)&color);
        else
            *stat = cli__maxparm; /* maximum parameter count exceeded */

}  /* set_fill_color */



static void set_x_tick (int *stat)

/*
 * set_x_tick - set axes x-tick space
 */

{
    string tick;
    float x_tick;


    cli_get_parameter ("X_TICK", tick, " ,", FALSE, TRUE, stat);

    if (odd(*stat))

	if (cli_end_of_command()) 
	    {
	    x_tick = cli_real (tick, stat);	    
	    
	    if (odd(*stat))
		SightSetAxesXtick (x_tick);
	    }
	else
	    *stat = cli__maxparm; /* maximum parameter count exceeded */

}  /* set_x_tick */



static void set_y_tick (int *stat)

/*
 * set_y_tick - set axes y-tick space
 */

{
    string tick;
    float y_tick;


    cli_get_parameter ("Y_TICK", tick, " ,", FALSE, TRUE, stat);

    if (odd(*stat))

	if (cli_end_of_command()) 
	    {
	    y_tick = cli_real (tick, stat);	    
	    
	    if (odd(*stat))
		SightSetAxesYtick (y_tick);
	    }
	else
	    *stat = cli__maxparm; /* maximum parameter count exceeded */

}  /* set_y_tick */



static void set_x_origin (int *stat)

/*
 * set_x_origin - set axes x-origin 
 */

{
    string origin;
    float x_org;


    cli_get_parameter ("X_ORIGIN", origin, " ,", FALSE, TRUE, stat);

    if (odd(*stat))

	if (cli_end_of_command()) 
	    {
	    x_org = cli_real (origin, stat);	    
	    
	    if (odd(*stat))
		SightSetAxesXorg (x_org);
	    }
	else
	    *stat = cli__maxparm; /* maximum parameter count exceeded */

}  /* set_x_origin */



static void set_y_origin (int *stat)

/*
 * set_y_origin - set axes y-origin 
 */

{
    string origin;
    float y_org;


    cli_get_parameter ("Y_ORIGIN", origin, " ,", FALSE, TRUE, stat);

    if (odd(*stat))

	if (cli_end_of_command()) 
	    {
	    y_org = cli_real (origin, stat);	    
	    
	    if (odd(*stat))
		SightSetAxesYorg (y_org);
	    }
	else
	    *stat = cli__maxparm; /* maximum parameter count exceeded */

}  /* set_y_org */



static void set_major_x (int *stat)

/*
 * set_major_x - set axes major x-tick count
 */

{
    string major_count;
    int major_x;

    cli_get_parameter ("MAJOR_X", major_count, " ,", FALSE, TRUE, stat);

    if (odd(*stat))

	if (cli_end_of_command()) 
	    {
	    major_x = cli_integer (major_count, stat);	    
	    
	    if (odd(*stat))
		SightSetAxesMajorX (major_x);
	    }
	else
	    *stat = cli__maxparm; /* maximum parameter count exceeded */

}  /* set_major_x */



static void set_major_y (int *stat)

/*
 * set_major_y - set axes major y-tick count 
 */

{
    string major_count;
    int major_y;


    cli_get_parameter ("MAJOR_Y", major_count, " ,", FALSE, TRUE, stat);

    if (odd(*stat))

	if (cli_end_of_command()) 
	    {
	    major_y = cli_integer (major_count, stat);
	    
	    if (odd(*stat))
		SightSetAxesMajorY (major_y);
	    }
	else
	    *stat = cli__maxparm; /* maximum parameter count exceeded */

}  /* set_major_y */



static void set_tick_size (int *stat)

/*
 * set_tick_size - set axes tick length 
 */

{
    string tick;
    float tick_size;


    cli_get_parameter ("TICK_SIZE", tick, " ,", FALSE, TRUE, stat);

    if (odd(*stat))

	if (cli_end_of_command()) 
	    {
	    tick_size = cli_real (tick, stat);	    
	    
	    if (odd(*stat))
		SightSetAxesTickSize (tick_size);
	    }
	else
	    *stat = cli__maxparm; /* maximum parameter count exceeded */

}  /* set_tick_size */



static void set_smoothing (int *stat)

/*
 * set_smoothing - set smoothing level of spline curves
 */

{
    string smoothing;
    float level;


    cli_get_parameter ("SMOOTHING", smoothing, " ,", FALSE, TRUE, stat);

    if (odd(*stat))

	if (cli_end_of_command()) 
	    {
	    level = cli_real (smoothing, stat);	    
	    
	    if (odd(*stat))
		SightSetSmoothing (level);
	    }
	else
	    *stat = cli__maxparm; /* maximum parameter count exceeded */

}  /* set_smoothing */



static void set_clipping (int *stat)

/*
 * set_clipping - set clip state 
 */

{
    int option;

    cli_get_keyword ("Clipping", clip_options, &option, stat);

    if (odd(*stat))
        {
        if (cli_end_of_command())
            SightSetClipping (option);
        else
            *stat = cli__maxparm;  /* maximum parameter count exceeded */
        }
}  /* set_clipping */



static void set_update (int *stat)

/*
 * set_update - set update flag
 */

{
    int option;

    cli_get_keyword ("Update", update_options, &option, stat);

    if (odd(*stat))
        {
        if (cli_end_of_command())
            SightSetUpdate (option);
        else
            *stat = cli__maxparm;  /* maximum parameter count exceeded */
        }
}  /* set_update */



static void set_snap_grid (int *stat)

/*
 * set_snap_grid - set snap grid width
 */

{
    string width;
    float value;

    cli_get_parameter ("WIDTH", width, " ,", FALSE, TRUE, stat);

    if (odd(*stat))
	{
	if (cli_end_of_command()) 
	    {
	    value = cli_real (width, stat);	    
	    
	    if (odd(*stat))
		if (value >= 0.0)
		    SightSetSnapGrid (value);
	    }
        else
            *stat = cli__maxparm;  /* maximum parameter count exceeded */
        }
}  /* set_snap_grid */



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



static void set_window (int *stat)

/*
 * set_window - set window
 */

{
    float x_min, x_max, y_min, y_max;
    char str[255];

    if (cli_end_of_command()) {
	if (!sight_open)
	    SightOpen (NIL);

	SightRequestRectangle (gc.tnr, &x_min, &y_min, &x_max, &y_max, TRUE);
	*stat = SightAutoscale (x_min, x_max, y_min, y_max, TRUE);
	}
    else
	{
	get_rectangle_specification (&x_min, &x_max, &y_min, &y_max, stat);

	if (odd(*stat))
	    *stat = SightAutoscale (x_min, x_max, y_min, y_max, FALSE);
	}

    if (odd(*stat)) {
	sprintf (str, "window %g %g %g %g", x_min, x_max, y_min, y_max);
	sight_info (str);
	}

}  /* set_window */



static void set_viewport (int *stat)

/*
 * set_viewport - set viewport 
 */

{
    float x_min, x_max, y_min, y_max;
    char str[255];

    if (cli_end_of_command()) {
	SightRequestRectangle (SightNDC, &x_min, &y_min, &x_max, &y_max, TRUE);

	*stat = SightMoveResizeViewport (x_min, x_max, y_min, y_max);
	}
    else
	{
	get_rectangle_specification (&x_min, &x_max, &y_min, &y_max, stat);

	if (odd(*stat))
	    *stat = SightMoveResizeViewport (x_min, x_max, y_min, y_max);
	}

    if (odd(*stat)) {
	sprintf (str, "viewport %g %g %g %g", x_min, x_max, y_min, y_max);
	sight_info (str);
	}

}  /* set_viewport */



static void set_orientation (int *stat)

/*
 * set_orientation - set orientation option
 */

{
    int option;
    char str[255];

    cli_get_keyword ("Orientation", orientation_options, &option, stat);

    if (odd(*stat))
        {
        if (cli_end_of_command())
	    {
            SightSetOrientation (option);

	    sprintf (str, "orientation %s", orientation[option]);
	    sight_info (str);
	    }
        else
            *stat = cli__maxparm;  /* maximum parameter count exceeded */
        }
}  /* set_orientation */



static void set_scale (int *stat)

/*
 * set_scale - set scale options
 */

{
    int options, flip_option;
    char str[255];

    if (!sight_open)
	SightOpen (NIL);

    cli_get_keyword ("Scale", scale_options, &options, stat);

    if (odd(*stat))
        {
	if (options > 0x3)
	    options = gc.xform[gc.tnr].scale & 0x3;

        if (!cli_end_of_command())
            {
            cli_get_keyword ("Flip", flip_options, &flip_option, stat);

            if (odd(*stat))
		{
		if (flip_option > 0x3)
		    flip_option = (gc.xform[gc.tnr].scale >> 3) & 0x3;

		options |= (flip_option << 3);
		}
            }

        if (odd(*stat))
	    {
	    if (cli_end_of_command())
		{
		SightSetScale (options);

		sprintf (str, "scale %s %s", scale[options & 0x3],
		    flip[(options >> 3) & 0x3]);
		sight_info (str);
		}
	    else
		*stat = cli__maxparm;  /* maximum parameter count exceeded */
	    }
	}
}  /* set_scale */



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
            case pline_type :
		set_pline_type (stat);
		break;
            case pline_width :
		set_pline_width (stat);
		break;
            case pline_color :
		set_pline_color (stat);
		break;
            case pmark_type :
		set_pmark_type (stat);
		break;
            case pmark_size :
		set_pmark_size (stat);
		break;
            case pmark_color :
		set_pmark_color (stat);
		break;
            case fill_style :
		set_fill_style (stat);
		break;
            case fill_index :
		set_fill_index (stat);
		break;
            case fill_color :
		set_fill_color (stat);
		break;
            case text_font :
		set_text_font (stat);
		break;
            case text_size :
		set_text_size (stat);
		break;
            case text_halign :
		set_text_halign (stat);
		break;
            case text_valign :
		set_text_valign (stat);
		break;
            case text_direction :
		set_text_direction (stat);
		break;
            case text_color :
		set_text_color (stat);
		break;
	    case image_size :
    	    	set_image_size (stat);
    	    	break;
    	    case image_position :
    	    	set_image_position (stat);
    	    	break;
	    case window :
		set_window (stat);
		break;
	    case viewport :
		set_viewport (stat);
		break;
    	    case x_tick :
		set_x_tick (stat);
		break;
    	    case y_tick :
		set_y_tick (stat);
		break;
    	    case x_origin :
		set_x_origin (stat);
		break;
    	    case y_origin :
		set_y_origin (stat);
		break;
    	    case major_x :
		set_major_x (stat);
		break;
    	    case major_y :
		set_major_y (stat);
		break;
    	    case tick_size :
		set_tick_size (stat);
		break;
    	    case smoothing :
		set_smoothing (stat);
		break;
    	    case clipping :
		set_clipping (stat);
		break;
    	    case orientation_option :
		set_orientation (stat);
		break;
    	    case scale_option :
		set_scale (stat);
		break;
    	    case update :
		set_update (stat);
		break;
    	    case snapping :
		set_snap_grid (stat);
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



static void get_xy_data (int n_max, float *px, float *py, int *n, int *stat)

/*
 * get_xy_data - get xy-data
 */

{
    BOOL segment;
    int key;

    segment = FALSE;

    key = x.key;
    cli_get_data_buffer (&x, n_max, px, n, stat);

    if (odd(*stat))
        {
        if (*stat == cli__eos)
            segment = TRUE;

        cli_get_data_buffer (&y, *n, py, n, stat);

        if (odd(*stat))
            {
            if (*stat == cli__eos)
                segment = TRUE;

            key = key + *n;
            if (!segment && (*n == n_max))
                key--;

            x.key = key;
            y.key = key;
            }
        }

}  /* get_xy_data */



static void get_xy_id (char *x_id, char *y_id, int *stat)

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
                if (cli_end_of_command())
                    {
                    var_address (y_id, stat);

                    if (*stat == var__undefid)
                        *stat = cli__normal;
                    }
                else
                    *stat = cli__maxparm; /* maximum parameter count exceeded */
                }
            }
        }

}  /* get_xy_id */



static void open_segment (int *stat)

/*
 * open_segment - begins a new segment
 */

{
    if (cli_end_of_command())
	SightOpenSegment ();
    else
	*stat = cli__maxparm; /* maximum parameter count exceeded */

}  /* close_segment */



static void close_segment (int *stat)

/*
 * close_segment - ends the last opened segment
 */

{
    if (cli_end_of_command())
	SightCloseSegment ();
    else
	*stat = cli__maxparm; /* maximum parameter count exceeded */

}  /* close_segment */



static void plot_command (int *stat)

/*
 * plot_command - plot routine 
 */

{
    int n_points, ignore;
    BOOL plot;
    char str[255];

    if (cli_end_of_command()) {
	n_points = max_points;
	SightRequestStroke (&n_points, px, py);

	if (n_points > 1) {
	    *stat = SightPlot (n_points, px, py);

	    var_delete ("X", &ignore);
	    var_define ("X", 0, n_points, px, FALSE, NIL);

	    var_delete ("Y", &ignore);
	    var_define ("Y", 0, n_points, py, FALSE, NIL);
	    }
	}
    else
	{
	get_xy_spec (stat);
	if (odd(*stat))
	    {
	    if (cli_end_of_command())
		{
		plot = TRUE;
		while (odd(*stat) && !cli_b_abort)
		    {
		    get_xy_data (max_points, px, py, &n_points, stat);

		    if (odd(*stat))
			if (plot)
			    {
			    *stat = SightPlot (n_points, px, py);
			    plot = FALSE;
			    }
			else
			    *stat = SightPolyline (n_points, px, py);
		    }

		if (*stat == cli__nmd)
		    *stat = cli__normal;
		}
	    else
		*stat = cli__maxparm; /* maximum parameter count exceeded */
	    }
	}

    if (odd(*stat)) {
	float *wn = gc.xform[gc.tnr].wn;

	sprintf (str, "window %g %g %g %g", wn[0], wn[1], wn[2], wn[3]);
	sight_info (str);
	}

}  /* plot_command */



static void polyline_command (int *stat)

/*
 * polyline_command - evaluate polyline command
 */

{
    int n_points, ignore;

    if (cli_end_of_command()) {
	n_points = max_points;
	SightRequestStroke (&n_points, px, py);

	if (n_points > 1) {
	    *stat = SightPolyline (n_points, px, py);

	    var_delete ("X", &ignore);
	    var_define ("X", 0, n_points, px, FALSE, NIL);

	    var_delete ("Y", &ignore);
	    var_define ("Y", 0, n_points, py, FALSE, NIL);
	    }
	}
    else
	{
	get_xy_spec (stat);
	if (odd(*stat))
	    {
	    if (cli_end_of_command())
		{
		while (odd(*stat) && !cli_b_abort)
		    {
		    get_xy_data (max_points, px, py, &n_points, stat);

		    if (odd(*stat))
			*stat = SightPolyline (n_points, px, py);
		    }

		if (*stat == cli__nmd)
		    *stat = cli__normal;
		}
	    else
		*stat = cli__maxparm; /* maximum parameter count exceeded */
	    }
	}

}  /* polyline_command */



static void polymarker_command (int *stat)

/*
 * polymarker_command - evaluate polymarker command
 */

{
    int n_points, ignore;

    if (cli_end_of_command()) {
	n_points = max_points;
	SightRequestMarker (&n_points, px, py);

	if (n_points > 0) {
	    *stat = SightPolymarker (n_points, px, py);

	    var_delete ("X", &ignore);
	    var_define ("X", 0, n_points, px, FALSE, NIL);

	    var_delete ("Y", &ignore);
	    var_define ("Y", 0, n_points, py, FALSE, NIL);
	    }
	}
    else
	{
	get_xy_spec (stat);
	if (odd(*stat))
	    {
	    if (cli_end_of_command())
		{
		while (odd(*stat) && !cli_b_abort)
		    {
		    get_xy_data (max_points, px, py, &n_points, stat);

		    if (odd(*stat))
			*stat = SightPolymarker (n_points, px, py);
		    }

		if (*stat == cli__nmd)
		    *stat = cli__normal;
		}
	    else
		*stat = cli__maxparm; /* maximum parameter count exceeded */
	    }
	}

}  /* polymarker_command */



static void get_xy_err (int *stat)

/*
 * get_xy_err - get xy-parameter with relative error
 */

{
    cli_get_data ("X", &x, stat);

    if (odd(*stat))
        {
        cli_get_data ("Y", &y, stat);

        if (odd(*stat))
	    {
            cli_get_data ("E1", &e1, stat);

	    if (odd(*stat))
		cli_get_data ("E2", &e2, stat);
	    }
        }
}



static void get_xy_err_data (int n, float *px, float *py, float *pe1,
    float *pe2, int *n_points, int *stat)

/*
 * get_xy_err_data - get xy and error data
 */

{
    BOOL segment;
    int key;

    segment = FALSE;

    key = x.key;
    cli_get_data_buffer (&x, n, px, n_points, stat);

    if (odd(*stat))
        {
        if (*stat == cli__eos)
            segment = TRUE;

        cli_get_data_buffer (&y, *n_points, py, n_points, stat);

        if (odd(*stat))
            {
            if (*stat == cli__eos)
                segment = TRUE;

            cli_get_data_buffer (&e1, *n_points, pe1, n_points, stat);

	    if (odd(*stat))
		{
		if (*stat == cli__eos)
		    segment = TRUE;

		cli_get_data_buffer (&e2, *n_points, pe2, n_points, stat);

		if (odd(*stat))
		    {
		    if (*stat == cli__eos)
			segment = TRUE;

		    key = key + *n_points;

		    if ((!segment) && (*n_points == n))
			key--;

		    x.key = key;
		    y.key = key;
		    e1.key = key;
		    e2.key = key;
		    }
		}
            }
        }
}



static void error_bars_command (int *stat)

/*
 * error_bars_command - evaluate error bar command
 */

{
    int n_points;
    SightBarOrientation orientation;
    int option;

    get_xy_err (stat);

    if (odd(*stat))
        {
        if (!cli_end_of_command())
            {
            cli_get_keyword ("Orientation", error_bar_options, &option, stat);

            if (odd(*stat))
                orientation = (SightBarOrientation) option;
            }
        else
            orientation = SightVerticalBar;
        }

    if (odd(*stat))
        {
        if (cli_end_of_command())
            {
            while (odd(*stat) && !cli_b_abort)
                {
                get_xy_err_data (max_points, px, py, pe1, pe2, &n_points, stat);

                if (odd(*stat))
                    *stat = SightErrorBars (n_points, px, py, pe1, pe2,
                        orientation);
                }

            if (*stat == cli__nmd)
                *stat = cli__normal;
            }
        else
            /* maximum parameter count exceeded */
            *stat = cli__maxparm;
        }

}  /* error_bars_command */



static void text_command (int *stat)

/*
 * text_command - evaluate text command
 */

{
    string component;
    float x, y;
    string chars;

    if (!cli_end_of_command()) {
	cli_get_parameter ("X", component, " ,", FALSE, TRUE, stat);

	if (odd(*stat)) {
	    x = cli_real(component, stat);

	    if (odd(*stat)) {
		cli_get_parameter ("Y", component, " ,", FALSE, TRUE, stat);

		if (odd(*stat)) 
		    y = cli_real(component, stat);
		}
	    }
	}
    else 
	SightRequestLocatorWC (&x, &y, TRUE);

    if (odd(*stat)) {
	if (!cli_end_of_command())
	    cli_get_parameter ("Text", chars, "", FALSE, TRUE, stat);
	else
	    SightRequestString (chars);
	}

    if (odd(*stat))
	*stat = SightText (x, y, chars);

}  /* text_command */



static void spline_command (int *stat)

/*
 * spline_command - evaluate spline command
 */

{
    int n_points, ignore;
    string smoothing;
    float level;

    level = 0.0;

    if (cli_end_of_command()) {
	n_points = max_points;
	SightRequestStroke (&n_points, px, py);

	if (n_points > 1) {
	    *stat = SightDrawSpline (n_points, px, py, level);

	    var_delete ("X", &ignore);
	    var_define ("X", 0, n_points, px, FALSE, NIL);

	    var_delete ("Y", &ignore);
	    var_define ("Y", 0, n_points, py, FALSE, NIL);
	    }
	}
    else
	{
	get_xy_spec (stat);

	if (odd(*stat))
	    {
	    if (!cli_end_of_command ())
		{
		cli_get_parameter ("Smoothing", smoothing, " ,", 
		    FALSE, TRUE, stat);

		if (odd(*stat))
		    {
		    if (cli_end_of_command ())
			level = cli_real(smoothing, stat);
		    else
			/* maximum parameter count exceeded */
			*stat = cli__maxparm;
		    }
		}

	    if (cli_end_of_command())
		{
		while (odd(*stat) && !cli_b_abort)
		    {
		    get_xy_data (max_points, px, py, &n_points, stat);

		    if (odd(*stat))
			*stat = SightDrawSpline (n_points, px, py, level);
		    }

		if (*stat == cli__nmd)
		    *stat = cli__normal;
		}
	    else
		*stat = cli__maxparm; /* maximum parameter count exceeded */
	    }
	}

}  /* spline_command */



static void fill_area_command (int *stat)

/*
 * fill_area_command - evaluate fill area command
 */

{
    int n_points, ignore;

    if (cli_end_of_command()) {
	n_points = max_points;
	SightRequestStroke (&n_points, px, py);

	if (n_points > 1) {
	    *stat = SightFillArea (n_points, px, py);

	    var_delete ("X", &ignore);
	    var_define ("X", 0, n_points, px, FALSE, NIL);

	    var_delete ("Y", &ignore);
	    var_define ("Y", 0, n_points, py, FALSE, NIL);
	    }
	}
    else
	{
	get_xy_spec (stat);
	if (odd(*stat))
	    {
	    if (cli_end_of_command())
		{
		while (odd(*stat) && !cli_b_abort)
		    {
		    get_xy_data (max_points, px, py, &n_points, stat);

		    if (odd(*stat))
			*stat = SightFillArea (n_points, px, py);
		    }

		if (*stat == cli__nmd)
		    *stat = cli__normal;
		}
	    else
		*stat = cli__maxparm; /* maximum parameter count exceeded */
	    }
	}

}  /* fill_area_command */



static void bar_graph_command (int *stat)

/*
 * bar_graph_command - evaluate bar graph command
 */

{
    int n_points, ignore;

    if (cli_end_of_command()) {
	n_points = max_points;
	SightRequestStroke (&n_points, px, py);

	if (n_points > 1) {
	    *stat = SightBarGraph (n_points, px, py);

	    var_delete ("X", &ignore);
	    var_define ("X", 0, n_points, px, FALSE, NIL);

	    var_delete ("Y", &ignore);
	    var_define ("Y", 0, n_points, py, FALSE, NIL);
	    }
	}
    else
	{
	get_xy_spec (stat);
	if (odd(*stat))
	    {
	    if (cli_end_of_command())
		{
		while (odd(*stat) && !cli_b_abort)
		    {
		    get_xy_data (max_points, px, py, &n_points, stat);

		    if (odd(*stat))
			*stat = SightBarGraph (n_points, px, py);
		    }

		if (*stat == cli__nmd)
		    *stat = cli__normal;
		}
	    else
		*stat = cli__maxparm; /* maximum parameter count exceeded */
	    }
	}

}  /* bar_graph_command */




static
void parse_file_specification (char *file_spec, char *def_spec, int *stat)

/*
 * parse_file_specification - parse a file specification
 */

{
    char *result_spec;

    result_spec = (char *) getenv (file_spec);
    if (result_spec != NIL)
	strcpy (file_spec, result_spec);

    if (*file_spec != '|')
	{
	str_parse (file_spec, def_spec, FAll, file_spec);
	if (access (file_spec, 0))
	    *stat = RMS__FNF;
	}
}



static void get_file_specification (char *file_spec, char *def_spec, int *stat)

/*
 * get_file_specification - get a file specification
 */

{
    cli_get_parameter ("File", file_spec, " ,", FALSE, TRUE, stat);

    if (odd(*stat))
	parse_file_specification (file_spec, def_spec, stat);
}



static void image_command (int *stat)

/*
 * image_command - evaluate image command
 */

{
    file_specification file_spec;
    int startx, starty, sizex, sizey, count;
    float x_min, x_max, y_min, y_max, *vp;
    string parm;

    get_file_specification (file_spec, ".pnm", stat);

    count = 0;

    while (odd(*stat) && (count < 8) && (!cli_end_of_command()))
        {
        cli_get_parameter (image_prompt[count], parm, " ,", FALSE, TRUE, stat);

        count++;

        if (odd (*stat))

            switch (count) {

                case 1 :
                  startx = cli_integer(parm, stat);
                  break;
                case 2 :
                  starty = cli_integer(parm, stat);
                  break;
                case 3 :
                  sizex = cli_integer(parm, stat);
                  break;
                case 4 :
                  sizey = cli_integer(parm, stat);
                  break;
                case 5 :
                  x_min = cli_real(parm, stat);
                  break;
    	    	case 6 :
    	    	  x_max = cli_real(parm, stat);
    	    	  break;
                case 7 :
                  y_min = cli_real(parm, stat);
                  break;
    	    	case 8 :
    	    	  y_max = cli_real(parm, stat);
    	    	  break;
                }
        }

    if (odd(*stat))
        {
	if (!sight_open)
	    SightOpen (NIL);

	vp = gc.xform[gc.tnr].vp;
	
        while (count < 8)
      	    {
            count++;

	    switch (count) {

                case 1 :
    	          startx = 0;
            	  break;
                case 2 :
    	    	  starty = 0;
                  break;
                case 3 :
                  sizex = 0;
    	          break;
                case 4 :
    	    	  sizey = 0;
    	          break;
                case 5 :
    	    	  x_min = vp[0];
    	          break;
                case 6 :
    	    	  x_max = vp[1];
    	          break;
                case 7 :
    	    	  y_min = vp[2];
    	          break;
                case 8 :
    	    	  y_max = vp[3];
    	          break;
                }
            }    

        if (cli_end_of_command())
	    *stat = SightImportImage(file_spec, startx, starty, sizex, sizey,
    	    	    	    	     x_min, x_max, y_min, y_max);
        else
            *stat = cli__maxparm;  /* maximum parameter count exceeded */
    	}

} /* image_command */



static void update_sight_info (BOOL selection)

{
    SightDisplayList *obj;
    char str[255];

    SightObjectID (&obj);

    if (obj != NIL) {

	switch (obj->type) {
	    case Pline :
		sprintf (str, "select polyline %s %dpt %s", 
		    ltype[obj->gc.line.linetype + 8],
		    (int)(obj->gc.line.linewidth + 0.5),
		    color[min(obj->gc.line.linecolor, 8)]);
		break;
	    case Spline :
		sprintf (str, "select spline %g %s %dpt %s", 
		    obj->obj.spline->smooth,
		    ltype[obj->gc.line.linetype + 8],
		    (int)(obj->gc.line.linewidth + 0.5),
		    color[min(obj->gc.line.linecolor, 8)]);
		break;
	    case Pmarker :
		sprintf (str, "select polymarker %s %dpt %s", 
		    mtype[obj->gc.marker.markertype + 20],
		    (int)(obj->gc.marker.markersize * NominalMarkerSize + 0.5),
		    color[min(obj->gc.marker.markercolor, 8)]);
		break;
	    case ErrorBars :
		sprintf (str, "select error_bars %s %dpt %s %dpt %s", 
		    mtype[obj->gc.bar.markertype + 20],
		    (int)(obj->gc.bar.markersize * NominalMarkerSize + 0.5),
		    color[min(obj->gc.bar.markercolor, 8)],
		    (int)(obj->gc.bar.linewidth + 0.5),
		    color[min(obj->gc.bar.linecolor, 8)]);
		break;
	    case FillArea :
	    case BarGraph :
		sprintf (str, "select %s %s %d %s", 
		    obj->type == FillArea ? "fill_area" : "bar_graph",
		    intstyle[obj->gc.fill.fillstyle],
		    obj->gc.fill.fillindex,
		    color[min(obj->gc.fill.fillcolor, 8)]);  
		break;
	    case SimpleText :
	    case Text :
		if (obj->type == Text)
		    sprintf (str, "select text %s %s %s %s %s %dpt %s", 
			font[(obj->gc.text.textfont - 1) % 8],
			fonttype[(obj->gc.text.textfont - 1) / 8],
			direction[obj->gc.text.textdirection],
			halign[obj->gc.text.texthalign],
			valign[obj->gc.text.textvalign],
			_SightGetTextSize(obj->gc.text.textsize),
			color[min(obj->gc.text.textcolor, 8)]);  
		else
		    sprintf (str, "select text %d %s %s %s %g %s", 
			obj->gc.text.textfont,
			direction[obj->gc.text.textdirection],
			halign[obj->gc.text.texthalign],
			valign[obj->gc.text.textvalign],
			obj->gc.text.textsize,
			color[min(obj->gc.text.textcolor, 8)]);  
		break;
	    case Axes :
	    case Grid :
		sprintf (str,
		    "select axes %g %g %g %g %d %d %g %dpt %s %s %s %dpt %s", 
		    obj->obj.axes->x_tick,
		    obj->obj.axes->y_tick,
		    obj->obj.axes->x_org,
		    obj->obj.axes->y_org,
		    obj->obj.axes->maj_x,
		    obj->obj.axes->maj_y,
		    obj->obj.axes->tick_size,
		    (int)(obj->gc.axes.linewidth + 0.5),
		    color[min(obj->gc.axes.linecolor, 8)],
		    font[(obj->gc.axes.textfont - 1) % 8],
		    fonttype[(obj->gc.axes.textfont - 1) / 8],
		    _SightGetTextSize(obj->gc.axes.textsize),
		    color[min(obj->gc.axes.textcolor, 8)]);  
		break;
    	    case Image :
    	    	sprintf(str, "select image %s %d %d %d %d %g %g %g %g",
    	    	    obj->obj.image->filename,
    	    	    obj->obj.image->startx,
    	    	    obj->obj.image->starty,
    	    	    obj->obj.image->sizex,
    	    	    obj->obj.image->sizey,
    	    	    obj->obj.image->x_min,
    	    	    obj->obj.image->x_max,
    	    	    obj->obj.image->y_min,
    	    	    obj->obj.image->y_max);
    	    	break;
	    }
    	    
	sight_info (selection ? str : str + 7);
	}

    else if (selection)
        sight_info ("deselect");
    else
        sight_info ("?");
}



static void select_object (int *stat)

/*
 * select_object - select an object
 */

{
    string component;
    float x, y;

    if (cli_end_of_command()) {
	SightRequestLocatorNDC (&x, &y, FALSE);
	SightSelectObject (x, y);
	}
    else
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
			if (cli_end_of_command())
			    SightSelectObject (x, y);
			else
			    /* maximum parameter count exceeded */
			    *stat = cli__maxparm;
			}
		    }
		}
	    }
	}

    if (odd(*stat))
	update_sight_info (TRUE);

}  /* select_object */



static void select_all (int *stat)

/*
 * select_all - select all object 
 */

{
    if (cli_end_of_command())
	SightSelectAll ();
    else
	*stat = cli__maxparm; /* maximum parameter count exceeded */

}  /* select_all */



static void select_excluded (int *stat)

/*
 * select_excluded - select all but the selected object
 */

{
    if (cli_end_of_command())
	SightSelectExcluded ();
    else
	*stat = cli__maxparm; /* maximum parameter count exceeded */

}  /* select_excluded */



static void select_group (int *stat)

/*
 * select_group - select group of objects
 */

{
    float x_min, x_max, y_min, y_max;

    if (cli_end_of_command()) {
	SightRequestRectangle (SightNDC, &x_min, &y_min, &x_max, &y_max, FALSE);

	SightSelectGroup (x_min, y_min, x_max, y_max);
	}
    else
	{
	get_rectangle_specification (&x_min, &x_max, &y_min, &y_max, stat);

	if (odd(*stat))
	    SightSelectGroup (x_min, y_min, x_max, y_max);
	}

}  /* select_group */



static void select_next (int *stat)

/*
 * select_next - select next object 
 */

{
    if (cli_end_of_command())
	SightSelectNext ();
    else
	*stat = cli__maxparm; /* maximum parameter count exceeded */

}  /* select_next */



static void select_previous (int *stat)

/*
 * select_previous - select previous object 
 */

{
    if (cli_end_of_command())
	SightSelectPrevious ();
    else
	*stat = cli__maxparm; /* maximum parameter count exceeded */

}  /* select_previous */



static void select_last (int *stat)

/*
 *  select_last - select last (created) object
 */

{
    if (cli_end_of_command())
	{
	SightSelectLast ();
	update_sight_info (TRUE);
	}
    else
	*stat = cli__maxparm; /* maximum parameter count exceeded */

}  /* select_last */



static void select_xform (int *stat)

/*
 * select_xform - select transformation number 
 */

{
    string xform;
    int tnr;
    char str[255];

    cli_get_parameter ("XFORM", xform, " ,", FALSE, TRUE, stat);

    if (odd(*stat))

	if (cli_end_of_command()) 
	    {
	    tnr = cli_integer (xform, stat);	    
	    
	    if (odd(*stat))
		{
		SightSelectXform (tnr);

	 	if (odd(*stat)) {
		    sprintf (str, "xform %d", tnr);
		    sight_info (str);
		    }
		}
	    }
	else
	    *stat = cli__maxparm; /* maximum parameter count exceeded */

}  /* select_xform */



static void select_command (int *stat)

/*
 * select_command - evaluate select command
 */

{
    int what;
    select_option option;

    cli_get_keyword ("What", select_options, &what, stat);

    option = (select_option)what;

    if (odd(*stat))

        switch (option) {
            case option_object :
		select_object (stat);
		break;
            case option_all :
		select_all (stat);
		break;
            case option_excluded :
		select_excluded (stat);
		break;
            case option_group :
		select_group (stat);
		break;
            case option_next :
		select_next (stat);
		break;
            case option_previous :
		select_previous (stat);
		break;
            case option_last :
		select_last (stat);
		break;
            case option_xform :
		select_xform (stat);
		break;
            }

}  /* select_command */



static void deselect_command (int *stat)

/*
 * deselect_command - deselect all object
 */

{
    if (cli_end_of_command())
	{
	SightDeselect ();
	sight_info ("deselect");
	}
    else
	*stat = cli__maxparm; /* maximum parameter count exceeded */

}  /* deselect_command */



static void redraw_command (int *stat)

/*
 * redraw_command - redraw all object
 */

{
    if (cli_end_of_command())
	*stat = SightRedraw (NIL);
    else
	*stat = cli__maxparm; /* maximum parameter count exceeded */

}  /* redraw_command */



static void align_command (int *stat)

/*
 * align_command - align selected text to snap grid
 */

{
    if (cli_end_of_command())
	*stat = SightAlign ();
    else
	*stat = cli__maxparm; /* maximum parameter count exceeded */

}  /* align_command */



static void axes_command (int *stat)

/*
 * axes command - plot a pair of axes
 */

{
    int count;
    string axes_parm;
    float *wn;
    int options;

    float x_tick, y_tick, x_org, y_org, tick_size;
    int maj_x, maj_y;

    maj_x = 0;
    maj_y = 0;

    count = 0;

    while (odd(*stat) && (count < 7) && (!cli_end_of_command()))
        {
        count++;

        cli_get_parameter (axes_prompt[count-1], axes_parm, " ,", FALSE,
	    TRUE, stat);

        if (odd (*stat))

            switch (count) {

                case 1 :
                  x_tick = cli_real(axes_parm, stat);
                  break;
                case 2 :
                  y_tick = cli_real(axes_parm, stat);
                  break;
                case 3 :
                  x_org = cli_real(axes_parm, stat);
                  break;
                case 4 :
                  y_org = cli_real(axes_parm, stat);
                  break;
                case 5 :
                  maj_x = cli_integer(axes_parm, stat);
                  break;
                case 6 :
                  maj_y = cli_integer(axes_parm, stat);
                  break;
                case 7 :
                  tick_size = cli_real(axes_parm, stat);
                  break;
                }
        }

    if (odd(*stat))
        {
	if (!sight_open)
	    SightOpen (NIL);

	wn = gc.xform[gc.tnr].wn;
	options = gc.xform[gc.tnr].scale;
	
        while (count < 7)
            {
            count++;

            switch (count) {

                case 1 :
                  x_tick = gus_tick(&wn[0], &wn[1]);
                  maj_x = 1;
                  break;
                case 2 :
                  y_tick = gus_tick(&wn[2], &wn[3]);
                  maj_y = 1;
                  break;
                case 3 :
                  x_org = option_flip_x IN options ? wn[1] : wn[0];
                  break;
                case 4 :
                  y_org = option_flip_y IN options ? wn[3] : wn[2];
                  break;
                case 5 :
                  if (maj_x == 0)
                      maj_x = 1;
                  break;
                case 6 :
                  if (maj_y == 0)
                      maj_y = 1;
                  break;
                case 7 :
                  tick_size = 0.01;
                  break;
                }
            }

        if (cli_end_of_command())
	    *stat = SightDrawAxes (x_tick, y_tick, x_org, y_org, maj_x, maj_y,
		tick_size);
        else
            *stat = cli__maxparm;  /* maximum parameter count exceeded */

	_SightRestoreXform ();
        }

}  /* axes_command */



static void grid_command (int *stat)

/*
 * grid command - plot a grid
 */

{
    int count;
    string grid_parm;
    float *wn;
    int options;

    float x_tick, y_tick, x_org, y_org;
    int maj_x, maj_y;

    maj_x = 0;
    maj_y = 0;

    count = 0;

    while (odd(*stat) && (count < 6) && (!cli_end_of_command()))
        {
        count++;

        cli_get_parameter (axes_prompt[count-1], grid_parm, " ,", FALSE,
	    TRUE, stat);

        if (odd(*stat))

            switch (count) {

                case 1 :
                  x_tick = cli_real(grid_parm, stat);
                  break;
                case 2 :
                  y_tick = cli_real(grid_parm, stat);
                  break;
                case 3 :
                  x_org = cli_real(grid_parm, stat);
                  break;
                case 4 :
                  y_org = cli_real(grid_parm, stat);
                  break;
                case 5 :
                  maj_x = cli_integer(grid_parm, stat);
                  break;
                case 6 :
                  maj_y = cli_integer(grid_parm, stat);
                  break;
                }
        }

    if (odd(*stat))
        {
	if (!sight_open)
	    SightOpen (NIL);

	wn = gc.xform[gc.tnr].wn;
	options = gc.xform[gc.tnr].scale;

        while (count < 6)
            {
            count++;

            switch (count) {

                case 1 :
                  x_tick = gus_tick(&wn[0], &wn[1]);
                  maj_x = 1;
                  break;
                case 2 :
                  y_tick = gus_tick(&wn[2], &wn[3]);
                  maj_y = 1;
                  break;
                case 3 :
                  x_org = option_flip_x IN options ? wn[1] : wn[0];
                  break;
                case 4 :
                  y_org = option_flip_y IN options ? wn[3] : wn[2];
                  break;
                case 5 :
                  if (maj_x == 0)
                      maj_x = 1;
                  break;
                case 6 :
                  if (maj_y == 0)
                      maj_y = 1;
                  break;
                }
            }

        if (cli_end_of_command())
            *stat = SightDrawGrid (x_tick, y_tick, x_org, y_org, maj_x, maj_y);
        else
            *stat = cli__maxparm;  /* maximum parameter count exceeded */
        }

}  /* grid_command */



static void open_drawing_command (int *stat)

/*
 * open_drawing_command - open drawing command
 */

{
    file_specification file_spec;
    char str[255];

    get_file_specification (file_spec, ".sight", stat);

    if (cli_end_of_command())
	{
	if (*stat == RMS__FNF)
	    *stat = RMS__NORMAL;

	if (odd(*stat))
            {
	    strcpy (def_spec, file_spec);
            str_parse (def_spec, "", FAll & ~FType & ~FVersion, def_spec);

            *stat = SightImportDrawing (file_spec);
	    if (odd(*stat))
		{
		sprintf (str, "open %s", orientation[sight_orientation]);
		sight_info (str);
		}
	    }
	}
    else
	*stat = cli__maxparm; /* maximum parameter count exceeded */

}  /* open_drawing_command */




static void close_drawing_command (int *stat)

/*
 * close_drawing_command - close drawing command
 */

{
    file_specification file_spec;

    if (cli_end_of_command())
	{
	strcpy (file_spec, def_spec);
	parse_file_specification (file_spec, ".sight", stat);
	}
    else
	get_file_specification (file_spec, ".sight", stat);

    if (cli_end_of_command())
	{
	if (*stat == RMS__FNF)
	    *stat = RMS__NORMAL;

	if (odd(*stat)) 
            {
            str_parse (file_spec, ".sight", FAll & ~FVersion, file_spec);

            *stat = SightExportDrawing (file_spec, FALSE);
            }
	}
    else
	*stat = cli__maxparm; /* maximum parameter count exceeded */

}  /* close_drawing_command */




static void create_drawing_command (int *stat)

/*
 * create_drawing_command - create a new drawing
 */

{
    file_specification file_spec;

    get_file_specification (file_spec, ".sight", stat);

    if (cli_end_of_command())
        {
        if (*stat == RMS__FNF)
            *stat = RMS__NORMAL;

	if (odd(*stat))
            {
	    strcpy (def_spec, file_spec);
            str_parse (def_spec, "", FAll & ~FType & ~FVersion, def_spec);

            SightClear ();
	    sight_info ("create");
            }
        }
    else
        *stat = cli__maxparm; /* maximum parameter count exceeded */

}  /* create_drawing_command */



static void capture_command (int *stat)

/*
 * capture_command - create a postscript figure file
 */

{
    file_specification file_spec;

    if (cli_end_of_command())
	{
	strcpy (file_spec, def_spec);
	parse_file_specification (file_spec, ".eps", stat);
	}
    else
	get_file_specification (file_spec, ".eps", stat);

    if (cli_end_of_command())
	{
	if (*stat == RMS__FNF)
	    *stat = RMS__NORMAL;

	if (odd(*stat)) 
            *stat = SightCapture (file_spec, TRUE);
	}
    else
	*stat = cli__maxparm; /* maximum parameter count exceeded */

}  /* capture_command */



static void print_command (int *stat)

/*
 * print_command - print picture
 */

{
    file_specification file_spec;
    char *print_spec;
    string command;
    
    if (cli_end_of_command())
	{
	strcpy (file_spec, def_spec);
	parse_file_specification (file_spec, ".ps", stat);
	}
    else
	get_file_specification (file_spec, ".ps", stat);

    if (cli_end_of_command())
	{
	if (*stat == RMS__FNF)
	    *stat = RMS__NORMAL;

	if (odd(*stat)) {
            *stat = SightCapture (file_spec, FALSE);

	    if (odd(*stat)) {
#ifndef _WIN32
		print_spec = (char *) getenv ("GLI_LPR");
                if (!print_spec)
#ifdef VMS
                    print_spec = "print/delete";
#else
                    print_spec = "lpr";
#endif
		strcpy (command, print_spec);
		strcat (command, " ");
		strcat (command, file_spec);

		system (command);
#else
                print_spec = (char *) getenv ("PRINTER");
		if (!print_spec)
		    print_spec = "LPT1";

		sprintf (command, "COPY %s %s", file_spec, print_spec);
		puts(command);

		system (command);
#endif
		}
	    }
	}
    else
	*stat = cli__maxparm; /* maximum parameter count exceeded */

}  /* print_command */



static void pick_object_command (int *stat)

/*
 * pick_object_command - select an object and transfer data to GLI
 */

{
    string component;
    float x, y;

    if (cli_end_of_command()) {
	SightRequestLocatorNDC (&x, &y, FALSE);
	SightPickObject (x, y);
	}
    else
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
			if (cli_end_of_command())
			    SightPickObject (x, y);
			else
			    /* maximum parameter count exceeded */
			    *stat = cli__maxparm;
			}
		    }
		}
	    }
	}

}  /* pick_object_command */



static void pick_element_command (int *stat)

/*
 * pick_element_command - select an element of an object and transfer data
 */

{
    string component;
    float x, y;

    if (cli_end_of_command()) {
	SightRequestLocatorNDC (&x, &y, FALSE);
	SightPickElement (x, y);
	}
    else
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
			if (cli_end_of_command())
			    SightPickElement (x, y);
			else
			    /* maximum parameter count exceeded */
			    *stat = cli__maxparm;
			}
		    }
		}
	    }
	}

}  /* pick_element_command */



static void modify_object_command (int *stat)

/*
 * modify_data_command - modifies a picked object
 */

{
    int np;
    float *px, *py;
    string text, equ_string;

    get_xy_spec (stat);

    if (odd(*stat)) 
	{
	if (!cli_end_of_command())
	    cli_get_parameter ("Text", text, "", FALSE, TRUE, stat);
	else
	    strcpy (text, "");

	if (odd(*stat))
	    {
	    if (cli_end_of_command())
		{
		np = 512; 

		px = (float *)malloc(np*sizeof(float));
		py = (float *)malloc(np*sizeof(float));

		get_xy_data (np, px, py, &np, stat);

		sym_translate (text, equ_string, stat);

		if (*stat != sym__normal)
		    {
		    strcpy (equ_string, text);
		    *stat = sym__normal;
		    }

		SightModifyObject (np, px, py, equ_string);

		free (px);
		free (py);
		}
	    else
		*stat = cli__maxparm; /* maximum parameter count exceeded */
	    }
	}

}  /* modify_object_command */



static void modify_element_command (int *stat)

/*
 * modify_element_command - modifies part of a picked object
 */

{
    int np;
    float *px, *py;

    if (cli_end_of_command())
	SightModifyElement (0, NIL, NIL);
    else
	{
	get_xy_spec (stat);

	if (cli_end_of_command())
	    {
	    np = 512; 

	    px = (float *)malloc(np*sizeof(float));
	    py = (float *)malloc(np*sizeof(float));

	    get_xy_data (np, px, py, &np, stat);

	    if (odd(*stat))
		SightModifyElement (np, px, py);

	    free (px);
	    free (py);

	    if (*stat == cli__nmd)
		*stat = cli__normal;
	    }
	else
	    *stat = cli__maxparm; /* maximum parameter count exceeded */
	}

}  /* modify_element_command */



static void pick_region_command (int *stat)

/*
 * pick_region_command - transfer data of selected object to GLI
 */

{
    float x_min, x_max, y_min, y_max;

    if (cli_end_of_command()) {
	SightRequestRectangle (SightNDC, &x_min, &y_min, &x_max, &y_max, FALSE);
	SightPickRegion (x_min, x_max, y_min, y_max);
	}
    else
	{
	get_rectangle_specification (&x_min, &x_max, &y_min, &y_max, stat);

	if (odd(*stat))
	    SightPickRegion (x_min, x_max, y_min, y_max);
	}

}  /* pick_region_command */



static void inquire_xform_command (int *stat)

/*
 * inquire_xform_command - inquire transformations
 */

{
    int tnr;
    SightTransformation *p;

    if (cli_end_of_command()) 
	{
	if (!sight_open)
	    SightOpen (NIL);

	for (tnr = 1; tnr <= 8; tnr++)
	    {
	    char result[255];

	    p = gc.xform + tnr;
	    sprintf (result, "  %d, %g %g %g %g, %g %g %g %g, %s %s;", tnr,
		p->wn[0], p->wn[1], p->wn[2], p->wn[3],
		p->vp[0], p->vp[1], p->vp[2], p->vp[3],
		scale[p->scale & 0x3], flip[(p->scale >> 3) & 0x3]);
#ifdef TCL
            if (gli_tcl_interp) {
                if (*result)
                    Tcl_AppendResult (gli_tcl_interp, result, (char *) NULL);
                }
            else
#endif
                tt_printf ("%s\n", result);
	    }
	}
    else
	*stat = cli__maxparm; /* maximum parameter count exceeded */

}  /* inquire_xform_command */



static void inquire_command (int *stat)

/*
 * inquire_command - inquires object attributes
 */

{
    int what;
    inquire_option option;
    char result[255];

    if (cli_end_of_command()) 
	{
	if (!sight_open)
	    SightOpen (NIL);

	*result = '\0';
	SightInquireObject (result);
	if (*result)
#ifdef TCL
            if (gli_tcl_interp) {
                if (*result)
                    Tcl_AppendResult (gli_tcl_interp, result, (char *) NULL);
                }
            else
#endif
		tt_printf ("%s\n", result);
	}
    else
	{
	cli_get_keyword ("What", inquire_options, &what, stat);

	option = (inquire_option)what;

	if (odd(*stat))
	    {
            switch (option) {
                case inq_xform :
		    inquire_xform_command (stat);
		    break;
                }
	    }
	}

}  /* inquire_command */



static void request_locator_command (int *stat)

/*
 * request_locator_command - evaluate request locator command
 */

{
    int ignore;
    identifier x_id, y_id;
    float px, py;
    string str;

    get_xy_id (x_id, y_id, stat);

    if (odd(*stat))
        {
        SightRequestLocatorWC (&px, &py, TRUE);

	fun_delete (x_id, &ignore);
	fun_define (x_id, str_flt(str, px), NIL);

	fun_delete (y_id, &ignore);
	fun_define (y_id, str_flt(str, py), NIL);
        }

}  /* request_locator_command */



static void MinMax (int n, float *x, float *xmin, float *xmax)

/*
 *  MinMax - Calculates the minimum and maximum of a vector of real values.
 */
    {
        int i;

    *xmin = x[0];
    *xmax = x[0];

    for (i = 1; i < n; i++)
        {
        *xmin = min(*xmin, x[i]);
        *xmax = max(*xmax, x[i]);
        }
    }



static void autoscale_command (int *stat)

/*
 * autoscale_command - evaluate autoscale command
 */

{
    int n_points, curve, n_curve, ignore;
    float xmin, xmax, ymin, ymax;
    float x0, x1, y0, y1;
    string str;
    BOOL first;


    curve = 0;
    while ((curve < max_curves) && !cli_end_of_command() && odd(*stat))
	{
	get_xy_spec (stat);
	if (odd(*stat))
	    {
	    x_list[curve] = x;
	    y_list[curve] = y;
	    curve++;
	    }
	}
    n_curve = curve;

    first = TRUE;
    curve = 0;
    while (odd(*stat) && curve < n_curve)
        {
	x = x_list[curve];
	y = y_list[curve];

        get_xy_data (max_points, px, py, &n_points, stat);

        if (odd(*stat))
            {
	    if (first)
		{
		MinMax (n_points, px, &xmin, &xmax);
		MinMax (n_points, py, &ymin, &ymax);
		first = FALSE;
		}
	    else
		{
		MinMax (n_points, px, &x0, &x1);
		xmin = min(xmin, x0);
		xmax = max(xmax, x1);
		MinMax (n_points, py, &y0, &y1);
		ymin = min(ymin, y0);
		ymax = max(ymax, y1);
		}

            while (odd(*stat) && (!cli_b_abort))
                {
                get_xy_data (max_points, px, py, &n_points, stat);

                if (odd(*stat))
                    {
		    MinMax (n_points, px, &x0, &x1);
		    xmin = min(xmin, x0);
		    xmax = max(xmax, x1);
		    MinMax (n_points, py, &y0, &y1);
		    ymin = min(ymin, y0);
		    ymax = max(ymax, y1);
                    }
                }

	    if (*stat == cli__nmd)
		*stat = cli__normal;
            }
	curve++;
        }
    
    if (odd(*stat))
	{
	fun_delete ("xmin", &ignore);
	fun_define ("xmin", str_flt(str, xmin), NIL);
	fun_delete ("xmax", &ignore);
	fun_define ("xmax", str_flt(str, xmax), NIL);
	fun_delete ("ymin", &ignore);
	fun_define ("ymin", str_flt(str, ymin), NIL);
	fun_delete ("ymax", &ignore);
	fun_define ("ymax", str_flt(str, ymax), NIL);

	SightAutoscale (xmin, xmax, ymin, ymax, FALSE);
	}

} /* autoscale_command */



void do_sight_command (int *stat)

/*
 * command - parse a Sight command
 */

{
    int index;
    sight_command command;

    if (!cli_end_of_command())
	{
	cli_get_keyword ("Sight", sight_command_table, &index, stat);

	command = (sight_command)index;

	if (odd(*stat))

	    switch (command) {
		case command_open :
		    open_sight (stat);
		    break;
		case command_close :
		    close_sight (stat);
		    break;
		case command_clear :
		    clear_command ();
		    break;
		case command_select :
		    select_command (stat);
		    break;
		case command_deselect :
		    deselect_command (stat);
		    break;
		case command_cut :
		    cut_command (stat);
		    break;
		case command_paste :
		    paste_command (stat);
		    break;
		case command_pop_in_front :
		    pop_in_front (stat);
		    break;
		case command_push_behind :
		    push_behind (stat);
		    break;
		case command_remove :
		    remove_command (stat);
		    break;
		case command_move :
		    move_command (stat);
		    break;
		case command_set :
		    set_command (stat);
		    break;
		case command_open_segment :
		    open_segment (stat);
		    break;
		case command_close_segment :
		    close_segment (stat);
		    break;
		case command_plot :
		    plot_command (stat);
		    break;
		case command_polyline :
		    polyline_command (stat);
		    break;
		case command_polymarker :
		    polymarker_command (stat);
		    break;
		case command_error_bars :
		    error_bars_command (stat);
		    break;
		case command_text :
		    text_command (stat);
		    break;
		case command_spline :
		    spline_command (stat);
		    break;
		case command_fill_area :
		    fill_area_command (stat);
		    break;
		case command_bar_graph :
		    bar_graph_command (stat);
		    break;
    	    	case command_image :
		    image_command (stat);
		    break;
		case command_redraw :
		    redraw_command (stat);
		    break;
		case command_axes :
		    axes_command (stat);
		    break;
		case command_grid :
		    grid_command (stat);
		    break;
		case command_open_drawing :
		    open_drawing_command (stat);
		    break;
		case command_close_drawing :
		    close_drawing_command (stat);
		    break;
		case command_create_drawing :
		    create_drawing_command (stat);
		    break;
		case command_capture :
		    capture_command (stat);
		    break;
		case command_print :
		    print_command (stat);
		    break;
		case command_pick_object :
		    pick_object_command (stat);
		    break;
		case command_pick_element :
		    pick_element_command (stat);
		    break;
		case command_modify_object :
		    modify_object_command (stat);
		    break;
		case command_modify_element :
		    modify_element_command (stat);
		    break;
		case command_pick_region :
		    pick_region_command (stat);
		    break;
		case command_inquire :
		    inquire_command (stat);
		    break;
		case command_request_locator :
		    request_locator_command (stat);
		    break;
		case command_autoscale :
		    autoscale_command (stat);
		    break;
		case command_align :
		    align_command (stat);
		    break;
		}

	if (odd(*stat))

	    switch (command) {
		case command_polyline :
		case command_polymarker :
		case command_error_bars :
		case command_text :
		case command_spline :
		case command_fill_area :
		case command_bar_graph :
		case command_axes :
		case command_image :
		    update_sight_info (FALSE);
		    break;
		}

	if (STATUS_FAC_NO(*stat) == gus__facility)
	    *stat = gus__normal;
	}

    else {
#ifdef MOTIF
	SightMainLoop ();
#else
	tt_fprintf (stderr, "Can't access OSF/MOTIF software libraries.\n");
#endif
	}

} /* do_sight_command */

