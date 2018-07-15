/*
 *
 * FACILITY:
 *
 *	Graphics Language Interpreter (GLI)
 *
 * ABSTRACT:
 *
 *	This module contains a Graphic Utility System (GUS) command language
 *	interpreter.
 *
 * AUTHOR:
 *
 *	Josef Heinen
 *
 * VERSION:
 *
 *	V1.0
 *
 * MODIFIED BY:
 *
 */


#include <ctype.h>
#include <stdio.h>
#include <math.h>
#include <string.h>


#include "system.h"
#include "mathlib.h"
#include "strlib.h"
#include "variable.h"
#include "function.h"
#include "terminal.h"
#include "command.h"
#include "formdrv.h"
#include "gksdefs.h"
#include "gus.h"


#define BOOL unsigned
#define NIL 0
#define TRUE 1
#define FALSE 0
#define odd(status) ((status) & 01)
#define IN &
#define present(status) ((status) != NIL)


#define tnr		    1	    /* default transformation number */
#define max_curves	    8	    /* maximum number of curves */

typedef char identifier[32];

typedef char string[256];

typedef enum {command_autoplot, command_autoscale_2d, command_autoscale_3d,
	      command_axes_2d, command_axes_3d, command_bar_graph,
	      command_colormap, command_contour, command_curve,
	      command_error_bars, command_fft, command_grid, command_histogram,
	      command_inverse_fft, command_linfit, command_linreg,
              command_pie_chart, command_plot, command_polyline,
              command_polymarker, command_set, command_spline, command_surface,
              command_text, command_titles_3d}
	      gus_command;

typedef enum {option_colormap, option_log, option_nolog, option_scale,
	      option_smoothing, option_space, option_text_slant,
	      option_ws_viewport}
	      set_option;

typedef enum {bar_vertical, bar_horizontal} bar_orientation;

static cli_verb_list gus_command_table = "autoplot autoscale_2d autoscale_3d\
 axes_2d axes_3d bar_graph colormap contour curve error_bars fft grid histogram\
 inverse_fft linfit linreg pie_chart plot polyline polymarker set spline\
 surface text titles_3d";

static cli_verb_list set_options = "colormap log nolog scale smoothing space\
 text_slant ws_viewport";

static cli_verb_list colormaps = "uniform temperature grayscale glowing\
 rainbow geologic greenscale cyanscale bluescale magentascale redscale flame\
 brownscale user_defined";

static cli_verb_list colormodes = "normal inverted";

static cli_verb_list bar_orientations = "vertical horizontal";

static cli_verb_list scale_options_list = "linear x_log y_log xy_log z_log\
 xz_log yz_log xyz_log";

static cli_verb_list flip_options_list = "none flip_x flip_y flip_xy flip_z\
 flip_xz flip_yz flip_xyz";

static cli_verb_list surface_options_list = "lines mesh filled_mesh\
 z_shaded_mesh colored_mesh cell_array shaded_mesh";

static cli_verb_list output_primitives = "polyline polymarker";

static cli_verb_list window_options_list = "full upper_half lower_half\
 upper_left upper_right lower_left lower_right";

static cli_verb_list size_options_list = "A5 A4 A3";

static cli_verb_list orientation_options_list = "landscape portrait";

static cli_verb space_prompt[] = {"Z_min", "Z_max", "Rotation", "Tilt"};

static cli_verb axes_2d_prompt[] = {"X-tick", "Y-tick", "X-origin", "Y-origin",
			     "Major-X-count", "Major-Y-count", "Tick-size"};

static cli_verb axes_3d_prompt[] = {"X-tick", "Y-tick", "Z-tick", "X-origin", 
                             "Y-origin", "Z-origin", "Major-X-count",
			     "Major-Y-count", "Major-Z-count", "Tick-size" };

static cli_verb titles_3d_prompt[] = {"X-title", "Y-title", "Z-title"};

static cli_data_descriptor x, y, z;
static cli_data_descriptor y_list[max_curves];
static int n_curves = 0, curve = 0;

extern int max_points;
extern float *px, *py, *pz, *pe1, *pe2, z_min;


static int sign (int x)

/*
 * sign - return sign(x)
 */

{
    if (x < 0)
        return (-1);
    else
        if (x == 0)
            return (0);
        else
            return (1);
}



static void get_y_list (int list_length, int *stat)

/*
 * get_y_list - get y-parameter specification
 */

{
    curve = 0;

    do
        {
        curve++;
        cli_get_data ("Y", &y_list[curve-1], stat);
        }
    while (odd(*stat) && (curve != list_length) && !cli_end_of_command());

    n_curves = curve;
    curve = 1;

    if (!cli_end_of_command())
        *stat = cli__maxparm;  /* maximum parameter count exceeded */
}



static void get_y_data (int max_points, float *py, int *n_points, int *stat)

/*
 * get_y_data - get y-data
 */

{
    BOOL segment;
    int key;

    segment = FALSE;

    key = y_list[curve-1].key;
    cli_get_data_buffer (&y_list[curve-1], max_points, py, n_points, stat);

    if (odd(*stat))
        {
        if (*stat == cli__eos)
            segment = TRUE;

        key = key + *n_points;

        if ((!segment) && (*n_points == max_points))
            key--;

        y_list[curve-1].key = key;
        }
}



static void get_xy_list (int list_length, int *stat)

/*
 * get_xy_list - get xy-parameter specification
 */

{
    cli_get_data ("X", &x, stat);

    if (odd(*stat))
        {
        curve = 0;

        do
            {
            curve++;
            cli_get_data ("Y", &y_list[curve-1], stat);
            }
        while (odd(*stat) && (curve != list_length) && !cli_end_of_command());

        n_curves = curve;
        curve = 1;

        if (!cli_end_of_command())
            *stat = cli__maxparm;  /* maximum parameter count exceeded */
        }
}



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

    if ((*stat == cli__nmd) && (curve < n_curves))
        {
        curve++;

        x.key = 1;
        key = 1;

        cli_get_data_buffer (&x, max_points, px, n_points, stat);
        }

    if (odd(*stat))
        {
        if (*stat == cli__eos)
            segment = TRUE;

        cli_get_data_buffer (&y_list[curve-1], *n_points, py, n_points, stat);
  
        if (odd(*stat))
            {
            if (*stat == cli__eos)
                segment = TRUE;

            key = key + *n_points;

            if ((!segment) && (*n_points == max_points))
                key--;

            x.key = key;
            y_list[curve-1].key = key;
            }
        }
}



static void get_xyz (int *stat)

/*
 * get_xyz - get xyz-parameter specification
 */

{
    cli_get_data ("X", &x, stat);

    if (odd(*stat))
        {
        cli_get_data ("Y", &y, stat);

        if (odd(*stat))
            cli_get_data ("Z", &z, stat);
        }
}



static void get_xyz_data (int max_points, float *px, float *py, float *pz,
    int *n_points, int *stat)

/*
 * get_xyz_data - get xyz-data
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

            cli_get_data_buffer (&z, *n_points, pz, n_points, stat);

            if (odd(*stat))
                {
                if (*stat == cli__eos)
                    segment = TRUE;

                key = key + *n_points;

                if ((!segment) && (*n_points == max_points))
                    key--;

                x.key = key;
                y.key = key;
                z.key = key;
                }
            }
        }
}



static void open_ws_server (void (*actrtn)(int))

/*
 * open_ws_server - server for all open workstations
 */

{
    int state, i, n, errind, ol, wkid;

    GQOPS (&state);
    if (state >= GWSOP)
        {
        n = 1;
        GQOPWK (&n, &errind, &ol, &wkid);

        for (i = ol; i >= 1; i--)
            {
            n = i;
            GQOPWK (&n, &errind, &ol, &wkid);

            actrtn (wkid);
            }
        }
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
}



static void set_scale (int *stat)

/*
 * set_scale - set scale options
 */

{
    int options, flip;

    cli_get_keyword ("Scale", scale_options_list, &options, stat);

    if (odd(*stat))
        {
	if (!cli_end_of_command())
	    {
	    cli_get_keyword ("Flip", flip_options_list, &flip, stat);

	    if (odd(*stat))
		options |= (flip << 3);
	    }

	if (odd(*stat))
	    {
	    if (cli_end_of_command())
		gus_set_scale (&options, NIL);
	    else
		*stat = cli__maxparm;  /* maximum parameter count exceeded */
	    }
        }
}



static void set_space (int *stat)

/*
 * set_space - set space options
 */

{
    int count;
    string space_parm;
    float z_max;
    int rotation, tilt;

    count = 0;

    while (odd(*stat) && (count < 4) && (!cli_end_of_command()))
        {
        count++;

        cli_get_parameter (space_prompt[count-1], space_parm, " ,", FALSE, 
	    TRUE, stat);

        if (odd(*stat))

            switch (count) {
 
               case 1 :
                  z_min = cli_real(space_parm, stat);
                  break;

                case 2 :
                  z_max = cli_real(space_parm, stat);
                  break;

                case 3 :
                  rotation = cli_integer(space_parm, stat);
                  break;

                case 4 :
                  tilt = cli_integer(space_parm, stat);
                  break;
                }
        }

    if (odd(*stat))
        {

        while (count < 4)
            {
            count++;

            switch (count) {

                case 1 :
                  z_min = 0;
                  break;

                case 2 :
                  z_max = 1.;
                  break;

                case 3 :
                  rotation = 60;
                  break;

                case 4 :
                  tilt = 60;
                  break;
                }
            }  

        if (cli_end_of_command())
            gus_set_space (&z_min, &z_max, &rotation, &tilt, NIL);
        else
            *stat = cli__maxparm;  /* maximum parameter count exceeded */
        }
}



static void set_ws_viewport (int *stat)

/*
 * set_ws_viewport - set workstation viewport
 */

{
    int window, size, orientation;
    gus_viewport_window vp_window;
    gus_viewport_size vp_size;
    gus_viewport_orientation vp_orientation = orientation_landscape;

    cli_get_keyword ("Window", window_options_list, &window, stat);

    vp_window = (gus_viewport_window) window;

    if (odd(*stat))
        {
        cli_get_keyword ("Size", size_options_list, &size, stat);

        vp_size = (gus_viewport_size) size;

        if (odd(*stat))
            {
            if (!cli_end_of_command())
		{
	        cli_get_keyword ("Orientation", orientation_options_list,
		    &orientation, stat);

	        vp_orientation = (gus_viewport_orientation) orientation;
		}

	    if (odd(*stat))
		{
		if (cli_end_of_command())
		    gus_set_ws_viewport (&vp_window, &vp_size, &vp_orientation,
			NIL);
		else
		    *stat = cli__maxparm;  /* maximum parameter count
					      exceeded */
		}
            }
        }
}



static void set_smoothing (int *stat)

/*
 * set_smoothing - set smoothing level
 */

{
    string smoothing;
    int level;

    cli_get_parameter ("Smoothing", smoothing, " ,", FALSE, TRUE, stat);

    if (odd(*stat))
        {
        if (cli_end_of_command())
            {
            level = cli_integer(smoothing, stat);

            if (odd(*stat))
                gus_set_smoothing (&level, NIL);
            }
        else
            *stat = cli__maxparm;  /* maximum parameter count exceeded */
        }
}



static void set_text_slant (int *stat)

/*
 * set_text_slant - set text slant
 */

{
    string text_slant;
    float slant;

    cli_get_parameter ("Text slant", text_slant, " ,", FALSE, TRUE, stat);

    if (odd(*stat))
        {
        if (cli_end_of_command ())
            {
            slant = cli_real(text_slant, stat);

            if (odd(*stat))
                gus_set_text_slant (&slant);
            }
        else
            *stat = cli__maxparm;  /* maximum parameter count exceeded */
        }
}



static void set_colormap (int *stat)

/*
 * set_colormap - set colormap
 */

{
    gus_colormap colormap;
    gus_colormode colormode;
    int cmap, cmode;

    cli_get_keyword ("Colormap", colormaps, &cmap, stat);

    if (odd(*stat))
        {
        colormap = (gus_colormap) cmap;

	if (!cli_end_of_command())
	    {
	    cli_get_keyword ("Mode", colormodes, &cmode, stat);

	    if (odd(*stat))
	        colormode = (gus_colormode) cmode;
	    }
	else
	    colormode = colormode_normal;

	if (odd(*stat))
	    {
	    if (cli_end_of_command())
		gus_set_colormap (&colormap, &colormode, NIL);
	    else
		*stat = cli__maxparm;  /* maximum parameter count exceeded */
	    }
        }
}



static void set_command (int *stat)

/*
 * set_command - evaluate set command
 */

{
    BOOL lswitch;
    int what;
    set_option option;

    cli_get_keyword ("What", set_options, &what, stat);

    option = (set_option)what;

    if (odd(*stat))

        switch (option) {

            case option_colormap :
              set_colormap (stat);
              break;

            case option_log :
	      lswitch = TRUE;
              gus_set_logging (&lswitch, stat);
              break;

            case option_nolog :
	      lswitch = FALSE;
              gus_set_logging (&lswitch, stat);
              break;

            case option_scale :
              set_scale (stat);
              break;

            case option_space :
              set_space (stat);
              break;

            case option_ws_viewport :
              set_ws_viewport (stat);
              break;

            case option_smoothing :
              set_smoothing (stat);
              break;

            case option_text_slant :
              set_text_slant (stat);
              break;
            }
}



static void autoscale_2d_command (int *stat)

/*
 * autoscale_2d_command - evaluate 2D autoscale command
 */

{
    int n_points, t_nr;

    float x_min, x_max, y_min, y_max;
    float x0, x1, y0, y1;

    get_xy_list (max_curves, stat);

    if (odd(*stat))
        {
        get_xy_data (max_points, px, py, &n_points, stat);

        if (odd(*stat))
            {
            gus_autoscale (&n_points, px, py, &x_min, &x_max, &y_min, &y_max, 
		NIL);

            while (odd(*stat) && (!cli_b_abort))
                {
                get_xy_data (max_points, px, py, &n_points, stat);

                if (odd(*stat))
                    {
                    gus_autoscale (&n_points, px, py, &x0, &x1, &y0, &y1, NIL);

                    if (x0 < x_min)
                        x_min = x0;
                    if (x1 > x_max)
                        x_max = x1;

                    if (y0 < y_min)
                        y_min = y0;
                    if (y1 > y_max)
                        y_max = y1;
                    }
                }

            if (*stat == cli__nmd)
                {
		t_nr = tnr;
                GSWN (&t_nr, &x_min, &x_max, &y_min, &y_max);
                *stat = cli__normal;
                }
            }
        }
}



static void autoplot_command (int *stat)

/*
 * autoplot_command - evaluate autoplot command
 */

{
    if (cli_end_of_command())
        {
        gus_autoplot (NIL);
        open_ws_server (update_ws);
        }
    else
        *stat = cli__maxparm;  /* maximum parameter count exceeded */
}



static void polyline_command (int *stat)

/*
 * polyline_command - evaluate polyline command
 */

{
    int errind, ltype, linetype;
    int n_points;

    get_xy_list (max_curves, stat);

    if (odd(*stat))
        {
        GQLN (&errind, &ltype);
        linetype = ltype;

        while (odd(*stat) && (!cli_b_abort))
            {
            get_xy_data (max_points, px, py, &n_points, stat);

            if (odd(*stat))
                {
                linetype = ltype + sign(ltype) * (curve - 1);
                GSLN (&linetype);

                gus_polyline (&n_points, px, py, NIL);
                }
            }

        GSLN (&ltype);
        open_ws_server (update_ws);

        if (*stat == cli__nmd)
            *stat = cli__normal;
        }
}



static void bar_graph_command (int *stat)

/*
 * bar_graph_command - evaluate bar graph command
 */

{
    int n_points;
    string bar_width;
    float width;

    get_xy_list (1, stat);

    if (*stat == cli__maxparm)
	*stat = cli__normal;

    if (odd(*stat))
	{
	get_xy_data (max_points, px, py, &n_points, stat);

	if (odd(*stat))
	    {
	    if (!cli_end_of_command())
		{
		cli_get_parameter ("Width", bar_width, " ,", FALSE, TRUE, stat);

		width = cli_real(bar_width, stat);
		}
	    else
		width = 0;

	    if (odd(*stat))
		gus_bar_graph (&n_points, px, py, &width, NIL);
	    }
	}

    open_ws_server (update_ws);

    if (*stat == cli__nmd)
	*stat = cli__normal;
}



static void error_bars_command (int *stat)

/*
 * error_bars_command - evaluate error_bars command
 */

{
    int n_points, n;
    int what;
    bar_orientation orientation;

    get_xy_list (3, stat);

    if (*stat == cli__maxparm)
	*stat = cli__normal;

    if (odd(*stat))
        {
	get_xy_data (max_points, px, py, &n_points, stat);

	if (odd(*stat) && (!cli_b_abort))
	    {
	    get_xy_data (max_points, px, pe1, &n, stat);

	    if (odd(*stat) && (!cli_b_abort))
		{
		while (n < n_points)
		    pe1[n++] = 0;

		get_xy_data (max_points, px, pe2, &n, stat);

		if (odd(*stat) && (!cli_b_abort))
		    {
		    while (n < n_points)
			pe2[n++] = 0;

		    cli_get_keyword ("Orientation", bar_orientations, &what,
			stat);

		    if (odd(*stat))
			{
			if (cli_end_of_command())
			    {
			    orientation = (bar_orientation)what;

			    switch (orientation)
				{
				case bar_vertical: gus_vertical_error_bars (
				    &n_points, px, py, pe1, pe2, NIL); break;

				case bar_horizontal: gus_horizontal_error_bars (
				    &n_points, px, py, pe1, pe2, NIL); break;
				}
			    }
			else
			    *stat = cli__maxparm;  /* maximum parameter count
						      exceeded */
			}
		    }
		}
	    }

        open_ws_server (update_ws);

        if (*stat == cli__nmd)
            *stat = cli__normal;
        }
}



static void polymarker_command (int *stat)

/*
 * polymarker_command - evaluate polymarker command
 */

{
    int errind, mtype, markertype;
    int n_points;

    get_xy_list (max_curves, stat);

    if (odd(*stat))
        {
        GQMK (&errind, &mtype);
        markertype = mtype;

        while (odd(*stat) && (!cli_b_abort))
            {
            get_xy_data (max_points, px, py, &n_points, stat);

            if (odd(*stat))
                {
                markertype = mtype + sign(mtype) * (curve - 1);
                GSMK (&markertype);

                gus_polymarker (&n_points, px, py, NIL);
                }
            }

        GSMK (&mtype);
        open_ws_server (update_ws);

        if (*stat == cli__nmd)
            *stat = cli__normal;
        }
}



static void text_command (int *stat)

/*
 * text_command - evaluate text command
 */

{
    string component;
    float x, y;
    string chars;

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
                        gus_text (&x, &y, chars, NIL);
                        open_ws_server (update_ws);
                        }
                    }
                }
            }
        }
}



static void spline_command (int *stat)

/*
 * spline_command - evaluate spline command
 */

{
    int n_points, m;
    string smoothing;
    float level;

    get_xy_list (1, stat);

    if (*stat == cli__maxparm)
	*stat = cli__normal;

    if (odd(*stat))
        {
	get_xy_data (max_points, px, py, &n_points, stat);

	if (odd(*stat))
	    {
	    if (!cli_end_of_command ())
		{
		cli_get_parameter ("Smoothing", smoothing, " ,", FALSE, TRUE,
		    stat);

		if (odd(*stat))
		    {
		    if (cli_end_of_command ())
			level = cli_real(smoothing, stat);
		    else
			/* maximum parameter count exceeded */
			*stat = cli__maxparm;
		    }
		}
	    else
		level = 0;
	    }

	if (odd(*stat))
	    {
	    if (n_points < 64)
		m = 256;
	    else
		m = 1024;

	    gus_spline (&n_points, px, py, &m, &level, NIL);
	    }

        open_ws_server (update_ws);

        if (*stat == cli__nmd)
            *stat = cli__normal;
        }
}



static void linfit_command (int *stat)

/*
 * linfit_command - evaluate linfit command
 */

{
    int errind, ltype, linetype;
    int n_points, ignore;

    get_xy_list (1, stat);

    if (odd(*stat))
        {
        GQLN (&errind, &ltype);
        linetype = ltype;

        while (odd(*stat) && (!cli_b_abort))
            {
            get_xy_data (max_points, px, py, &n_points, stat);

            if (odd(*stat))
                {
                linetype = ltype + sign(ltype) * (curve - 1);
                GSLN (&linetype);

                gus_linfit (&n_points, px, py, NIL);

		var_delete ("LINFIT_M", &ignore);
		var_define ("LINFIT_M", 0, 1, &gus_gf_linfit_m, FALSE, NIL);
		var_delete ("LINFIT_B", &ignore);
		var_define ("LINFIT_B", 0, 1, &gus_gf_linfit_b, FALSE, NIL);
		var_delete ("LINFIT_DEV", &ignore);
		var_define ("LINFIT_DEV", 0, 1, &gus_gf_linfit_dev, FALSE, NIL);
                }
            }

        GSLN (&ltype);
        open_ws_server (update_ws);

        if (*stat == cli__nmd)
            *stat = cli__normal;
        }
}



static void linreg_command (int *stat)

/*
 * linreg_command - evaluate linreg command
 */

{
    int errind, ltype, linetype;
    int n_points, ignore;

    get_xy_list (1, stat);

    if (odd(*stat))
        {
        GQLN (&errind, &ltype);
        linetype = ltype;

        while(odd(*stat) && (!cli_b_abort))
            {
            get_xy_data (max_points, px, py, &n_points, stat);

            if (odd(*stat))
                {
                linetype = ltype + sign(ltype) * (curve - 1);
                GSLN (&linetype);

                gus_linreg (&n_points, px, py, NIL);

		var_delete ("LINREG_M", &ignore);
		var_define ("LINREG_M", 0, 1, &gus_gf_linreg_m, FALSE, NIL);
		var_delete ("LINREG_B", &ignore);
		var_define ("LINREG_B", 0, 1, &gus_gf_linreg_b, FALSE, NIL);
                }
            }

        GSLN (&ltype);
        open_ws_server (update_ws);

        if (*stat == cli__nmd)
            *stat = cli__normal;
        }
}



static void histogram_command (int *stat)

/*
 * histogram_command - evaluate histogram command
 */

{
    int errind, styli;
    int n_points;

    get_y_list (max_curves, stat);

    if (odd(*stat))
        {
        GQFASI (&errind, &styli);

        while (odd(*stat) && (!cli_b_abort))
            {
            get_y_data (max_points, py, &n_points, stat);

            if (odd(*stat))
                {
                if (styli == 1)
                    GSFASI (&curve);

                gus_histogram (&n_points, py, NIL);
                }
            }

        GSFASI (&styli);
        open_ws_server (update_ws);

        if (*stat == cli__nmd)
            *stat = cli__normal;
        }
}



static void fft_command (int *stat)

/*
 * fft_command - evaluate fft command
 */

{
    int n_points, m;

    get_y_list (1, stat);

    if (odd(*stat))
        {
        get_y_data (max_points/2, py, &n_points, stat);

        if (odd(*stat))
            {
            if (n_points > 0)
		for (m = 1; m < n_points; m = 2*m);

            gus_fft (&n_points, py, &m, NIL);
            }

        open_ws_server (update_ws);

        if (*stat == cli__nmd)
            *stat = cli__normal;
        }
}



static void inverse_fft_command (int *stat)

/*
 * inverse_fft_command - evaluate inverse fft command
 */

{
    int n_points;

    get_xy_list (max_curves, stat);

    if (odd(*stat))
        {
        get_xy_data (max_points, px, py, &n_points, stat);

        if (odd(*stat))
            gus_inverse_fft (&n_points, px, py, NIL);

        open_ws_server (update_ws);

        if (*stat == cli__nmd)
            *stat = cli__normal;
        }
}



static void pie_chart_command (int *stat)

{
    int npoints;

    get_y_list (1, stat);

    if (odd(*stat))
        {
        get_y_data (max_points, px, &npoints, stat);

        if (odd(*stat))
            {
            if (cli_end_of_command())
                gus_pie_chart (&npoints, px, NIL);
            else
                *stat = cli__maxparm;
            }

        open_ws_server (update_ws);

        if (*stat == cli__nmd)
            *stat = cli__normal;
        }
}



static void plot_command (int *stat)

/*
 * plot_command - evaluate plot command
 */

{
    int n_points;
    BOOL plot;

    get_xy_list (max_curves, stat);

    if (odd(*stat))
        {
	plot = TRUE;
        while (odd(*stat) && !cli_b_abort)
            {
            get_xy_data (max_points, px, py, &n_points, stat);

	    if (odd(*stat))
		if (plot)
		    {
		    gus_plot (&n_points, px, py, NIL);
		    plot = FALSE;
		    }
		else
		    gus_polyline (&n_points, px, py, NIL);
            }

        open_ws_server (update_ws);

        if (*stat == cli__nmd)
            *stat = cli__normal;
        }
}



static void plot_2d_axes (int *stat)

/*
 * plot_2d_axes - plot a pair of axes
 */

{
    int count;
    string axes_parm;

    int errind, t_nr;
    float wn[4], vp[4];
    int options;

    float x_tick, y_tick, x_org, y_org, tick_size;
    int maj_x, maj_y;

    maj_x = 0;
    maj_y = 0;

    GQCNTN (&errind, &t_nr);
    GQNT (&t_nr, &errind, wn, vp);

    gus_inq_scale (&options, NIL);

    count = 0;

    while (odd(*stat) && (count < 7) && (!cli_end_of_command()))
        {
        count++;

        cli_get_parameter (axes_2d_prompt[count-1], axes_parm, " ,", FALSE,
	    TRUE, stat);

        if (odd(*stat))

            switch (count) {

                case 1 :
                  x_tick = cli_real(axes_parm, stat);
		  if (x_tick == -1)
		      x_tick = gus_tick(&wn[0], &wn[1]);
                  break;

                case 2 :
                  y_tick = cli_real(axes_parm, stat);
		  if (y_tick == -1)
		      y_tick = gus_tick(&wn[2], &wn[3]);
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
                  tick_size = -0.0075;
                  break;
                }
            }

        if (cli_end_of_command())
            {
            gus_axes (&x_tick, &y_tick, &x_org, &y_org, &maj_x,&maj_y,
		&tick_size, NIL);

            open_ws_server (update_ws);
            }
        else
            *stat = cli__maxparm;  /* maximum parameter count exceeded */
        }
}



static void plot_grid (int *stat)

/*
 * plot grid - plot a grid
 */

{
    int count;
    string grid_parm;

    int errind, t_nr;
    float wn[4], vp[4];
    int options;

    float x_tick, y_tick, x_org, y_org;
    int maj_x, maj_y;

    maj_x = 0;
    maj_y = 0;

    GQCNTN (&errind, &t_nr);
    GQNT (&t_nr, &errind, wn, vp);

    gus_inq_scale (&options, NIL);

    count = 0;

    while (odd(*stat) && (count < 6) && (!cli_end_of_command()))
        {
        count++;

        cli_get_parameter (axes_2d_prompt[count-1], grid_parm, " ,", FALSE,
	    TRUE, stat);

        if (odd(*stat))

            switch (count) {

                case 1 :
                  x_tick = cli_real(grid_parm, stat);
		  if (x_tick == -1)
		      x_tick = gus_tick(&wn[0], &wn[1]);
                  break;

                case 2 :
                  y_tick = cli_real(grid_parm, stat);
		  if (y_tick == -1)
		      y_tick = gus_tick(&wn[2], &wn[3]);
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
            {
            gus_grid (&x_tick, &y_tick, &x_org, &y_org, &maj_x, &maj_y, NIL);
            open_ws_server (update_ws);
            }
        else
            *stat = cli__maxparm;  /* maximum parameter count exceeded */
        }
}



static void curve_command (int *stat)

/*
 * curve_command - evaluate curve command
 */

{
    int n_points, index;
    gus_primitive primitive;

    get_xyz (stat);

    if (odd(*stat))
        {
        if (!cli_end_of_command())
            {
            cli_get_keyword ("Primitive", output_primitives, &index,
                stat);

            primitive = (gus_primitive) index;
            }
        else
            primitive = primitive_polyline;

        if (odd(*stat))
            {
            if (cli_end_of_command())
                {
                while (odd(*stat) && (!cli_b_abort))
                    {
                    get_xyz_data (max_points, px, py, pz, &n_points, stat);

                    if (odd(*stat))
                        gus_curve (&n_points, px, py, pz, &primitive, NIL);
                    }

                open_ws_server (update_ws);

                if (*stat == cli__nmd)
                    *stat = cli__normal;
                }
            else
                *stat = cli__maxparm;  /* maximum parameter count
                                          exceeded */
            }
        }
}



static void read_variable (var_variable_descr *variable, int *n, float *p)

/*
 * read_variable - read variable into buffer
 */

{
    int i;

    *n = (variable->allocn < max_points) ? variable->allocn : max_points;

    for (i = 0; i < *n; i++)
	p[i] = variable->data[i];
}



static var_variable_descr *x_var, *y_var, *z_var;
static fun_function_descr *addr;
static BOOL variable, no_dimx;
static int ret_stat, dimx;



static float f (int *ix, int *iy)

/*
 * f - f (x,y)
 */

{
    int index, stat;
    float value = 0;

    if (variable)
        {
	if (no_dimx)
	    index = *ix + (*iy - 1) * dimx;
	else
	    index = (int)(px[*ix - 1] + (py[*iy - 1] - 1) * dimx);

        if (index > z_var->allocn)
            {
            index = z_var->allocn;
            stat = var__undefind;
            }
        else
            stat = var__normal;

        value = *(z_var->data + index - 1);
        }
    else
        {
        *(x_var->data) = px[*ix - 1];
        *(y_var->data) = py[*iy - 1];

        fun_evaluate (addr, &value, &stat);
        }

    if (!odd(stat))
        ret_stat = stat;

    return (value);
}



static void autoscale_3d_command (int *stat)

/*
 * autoscale_3d_command - evaluate 3D autoscale command
 */

{
    string x_variable, y_variable, z_variable, dimension_x;
    int i, j, ii, jj, nx, ny, t_nr;

    float x_min, x_max, y_min, y_max, z_max, z;
    int rotation, tilt;

    cli_get_parameter ("X", x_variable, " ,", FALSE, TRUE, stat);

    if (odd(*stat))
        {
        x_var = var_address(x_variable, stat);

        if (odd(*stat))
            {
            cli_get_parameter ("Y", y_variable, " ,", FALSE, TRUE, stat);

            if (odd(*stat))
                {
                y_var = var_address(y_variable, stat);

                if (odd(*stat))
                    {
                    cli_get_parameter ("Z", z_variable, " ,", FALSE, TRUE,
			stat);

                    if (odd(*stat))
                        {
                        z_var = var_address(z_variable, stat);

                        if (*stat == var__undefid)
                            {
                            addr = fun_address(z_variable, stat);

                            variable = FALSE;
                            }
                        else
                            variable = TRUE;

			if (odd(*stat))
			    {
			    if (!cli_end_of_command())
				{
				cli_get_parameter ("Dimension X",
				    dimension_x, " ,", FALSE, TRUE, stat);

				dimx = cli_integer (dimension_x, stat);
				}
			    else
				dimx = 0;
			    }

			if (odd(*stat))
			    {
			    if (cli_end_of_command())
				{
				read_variable (x_var, &nx, px);
				read_variable (y_var, &ny, py);

				if (no_dimx = (dimx == 0))
				    dimx = nx;

				if (!variable)
				    var_find (1, NIL);

				gus_inq_space (&z_min, &z_max, &rotation,
				    &tilt, NIL);

				x_min = x_max = px[0];
				for (i = 0; i < nx; i++)
				    {
				    if (px[i] < x_min)
					x_min = px[i];
				    else if (px[i] > x_max)
					x_max = px[i];
				    }

				y_min = y_max = py[0];
				for (j = 0; j < ny; j++)
				    {
				    if (py[j] < y_min)
					y_min = py[j];
				    else if (py[j] > y_max)
					y_max = py[j];
				    }

				ret_stat = cli__normal;

				ii = jj = 1;
				z_min = z_max = f(&ii, &jj);
                                for (i = 1; i <= nx; i++)
				    {
				    ii = i;
				    for (j = 1; j <= ny; j++)
					{
					jj = j;
					z = f(&ii, &jj);
					if (z < z_min)
					    z_min = z;
					else if (z > z_max)
					    z_max = z;
					}
				    }

                                if (odd(ret_stat))
                                    {
				    t_nr = tnr;
				    GSWN (&t_nr, &x_min, &x_max, &y_min,
                                        &y_max);

				    gus_set_space (&z_min, &z_max, &rotation,
				        &tilt, NIL);

				    if (!variable)
				        {
				        var_deposit (x_var, px[0], NIL);
				        var_deposit (y_var, py[0], NIL);
				        }
				    }
                                else
                                    *stat = ret_stat;
                                }
			    else

				/* maximum parameter count exceeded */

				*stat = cli__maxparm;  
			    }
                        }
                    }
                }
            }
        }
}



static void plot_surface (int *stat)

/*
 * plot_surface - plot three-dimensional surface
 */

{
    string x_variable, y_variable, z_variable, dimension_x;
    int index;
    gus_surface_option option;
    int nx, ny;

    /* plot_surface */

    cli_get_parameter ("X", x_variable, " ,", FALSE, TRUE, stat);

    if (odd(*stat))
        {
        x_var = var_address(x_variable, stat);

        if (odd(*stat))
            {
            cli_get_parameter ("Y", y_variable, " ,", FALSE, TRUE, stat);

            if (odd(*stat))
                {
                y_var = var_address(y_variable, stat);

                if (odd(*stat))
                    {
                    cli_get_parameter ("Z", z_variable, " ,", FALSE, TRUE,
			stat);

                    if (odd(*stat))
                        {
                        z_var = var_address(z_variable, stat);

                        if (*stat == var__undefid)
                            {
                            addr = fun_address(z_variable, stat);

                            variable = FALSE;
                            }
                        else
                            variable = TRUE;

                        if (odd(*stat))
			    {

			    if (!cli_end_of_command())
				{
				cli_get_keyword ("Option", surface_options_list,
				    &index, stat);

				option = (gus_surface_option) index;
				}
			    else
				option = option_lines;

			    if (odd(*stat))
				{

				if (!cli_end_of_command())
				    {
				    cli_get_parameter ("Dimension X",
					dimension_x, " ,", FALSE, TRUE, stat);

				    dimx = cli_integer (dimension_x, stat);
				    }
				else
				    dimx = 0;

				if (odd(*stat))
				    {
				    int ignore;

				    if (cli_end_of_command())
					{
					read_variable (x_var, &nx, px);
					read_variable (y_var, &ny, py);

					if (no_dimx = (dimx == 0))
					    dimx = nx;

					if (!variable)
					    var_find (1, NIL);

					fun_parse ("GUS_MISSING_VALUE",
					    &gus_gf_missing_value, FALSE,
					    &ignore);

					gus_surface (&nx, &ny, px, py, f,
					    &option, NIL);

					if (!variable)
					    {
					    var_deposit (x_var, px[0], NIL);
					    var_deposit (y_var, py[0], NIL);
					    }

					open_ws_server (update_ws);
					}
				    else

					/* maximum parameter count exceeded */

					*stat = cli__maxparm;  
				    }
				}
			    }
                        }
                    }
                }
            }
        }
}



static void colormap_command (int *stat)

/*
 * colormap_command - evaluate colormap command
 */

{
    if (cli_end_of_command ())
	{
	gus_show_colormap (NIL);
        open_ws_server (update_ws);
	}
    else
        *stat = cli__maxparm;  /* maximum parameter count exceeded */
}



static void plot_contour (int *stat)

/*
 * plot_contour - plot contour lines
 */

{
    string x_variable, y_variable, z_variable, h_variable;
    string dimension_x, major_h;

    var_variable_descr *h_var;

    float *h;
    int nx, ny, nh, mh;

    /* plot_contour */

    cli_get_parameter ("X", x_variable, " ,", FALSE, TRUE, stat);

    if (odd(*stat))
        {
        x_var = var_address(x_variable, stat);

        if (odd(*stat))
            {
            cli_get_parameter ("Y", y_variable, " ,", FALSE, TRUE, stat);

            if (odd(*stat))
                {
                y_var = var_address(y_variable, stat);

                if (odd(*stat))
                    {
                    cli_get_parameter ("H", h_variable, " ,", FALSE, TRUE,
			stat);

                    if (odd(*stat))
                        {
                        h_var = var_address(h_variable, stat);

                        if (odd(*stat))
                            {
                            cli_get_parameter ("Z", z_variable, " ,", FALSE,
				TRUE, stat);

                            if (odd(*stat))
				{
				z_var = var_address(z_variable, stat);

				if (*stat == var__undefid)
				    {
				    addr = fun_address(z_variable, stat);

				    variable = FALSE;
				    }
				else
				    variable = TRUE;

				if (odd(*stat))
				    {
				    if (!cli_end_of_command())
					{
					cli_get_parameter ("Dimension X",
					    dimension_x, " ,", FALSE, TRUE, 
					    stat);

					dimx = cli_integer (dimension_x, stat);
					}
				    else
					dimx = 0;

				    if (odd(*stat))
					{
				        if (!cli_end_of_command())
					    {
					    cli_get_parameter ("Major-H-count",
					        major_h, " ,", FALSE, TRUE, 
					        stat);

					    mh = cli_integer (major_h, stat);
					    }
				        else
					    mh = 0;

				        if (odd(*stat))
					    {
					    if (cli_end_of_command())
					        {
					        read_variable (x_var, &nx, px);
					        read_variable (y_var, &ny, py);

					        nh = h_var->allocn;
					        h = h_var->data;

					        if (no_dimx = (dimx == 0))
						    dimx = nx;

					        if (!variable)
						    var_find (1, NIL);

					        gus_contour (&nx, &ny, &nh, px, 
						    py, h, f, &mh, NIL);

					        if (!variable)
						    {
						    *(x_var->data) = px[0];
						    *(y_var->data) = py[0];
						    }

					        open_ws_server (update_ws);
					        }
					    else

					        *stat = cli__maxparm;
                                            }
					}
				    }
				}
                            }
                        }
                    }
                }
            }
        }
}



static void plot_3d_axes (int *stat)

/*
 * plot_3d_axes - plot three-dimensional axes
 */

{
    int count, argc;
    string axes_parm;

    int errind, t_nr;
    float wn[4], vp[4];

    float zero, x_max, z_min, z_max;
    int rotation, tilt, options;

    float x_tick, y_tick, z_tick, x_org, y_org, z_org, tick_size;
    int maj_x, maj_y, maj_z;

    maj_x = 0;
    maj_y = 0;
    maj_z = 0;

    GQCNTN (&errind, &t_nr);
    GQNT (&t_nr, &errind, wn, vp);

    gus_inq_space (&z_min, &z_max, &rotation, &tilt, NIL);
    gus_inq_scale (&options, NIL);

    count = 0;

    while (odd(*stat) && (count < 10) && (!cli_end_of_command()))
        {
        count++;

	cli_get_parameter (axes_3d_prompt[count-1], axes_parm, " ,", FALSE, 
	    TRUE, stat);

        if (odd(*stat))

            switch (count) {

                case 1 :
                  x_tick = cli_real(axes_parm, stat);
		  if (x_tick == -1)
		      x_tick = (rotation <= 70 || tilt >= 20) ?
			  gus_tick(&wn[0], &wn[1]) : wn[1] - wn[0];
                  break;

                case 2 :
                  y_tick = cli_real(axes_parm, stat);
		  if (y_tick == -1)
		      y_tick = (rotation >= 20 || tilt >= 20) ?
			  gus_tick(&wn[2], &wn[3]) : wn[3] - wn[2];
                  break;

                case 3 :
                  z_tick = cli_real(axes_parm, stat);
		  if (z_tick == -1)
		      z_tick = (tilt <= 70) ?
			  gus_tick(&z_min, &z_max) : z_max - z_min;
                  break;

                case 4 :
                  x_org = cli_real(axes_parm, stat);
                  break;

                case 5 :
                  y_org = cli_real(axes_parm, stat);
                  break;

                case 6 :
                  z_org = cli_real(axes_parm, stat);
                  break;

                case 7 :
                  maj_x = cli_integer(axes_parm, stat);
                  break;

                case 8 :
                  maj_y = cli_integer(axes_parm, stat);
                  break;

                case 9 :
                  maj_z = cli_integer(axes_parm, stat);
                  break;

                case 10 :
                  tick_size = cli_real(axes_parm, stat);
                  break;
                }
        }

    if (odd(*stat))
        {
	argc = count;

        while (count < 10)
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
                  z_tick = gus_tick(&z_min, &z_max);
                  maj_z = 1;
                  break;

                case 4 :
                  x_org = option_flip_x IN options ? wn[1] : wn[0];
                  break;

                case 5 :
                  y_org = option_flip_y IN options ? wn[3] : wn[2];
                  break;

                case 6 :
                  z_org = option_flip_z IN options ? z_max : z_min;
                  break;

                case 7 :
                  if (maj_x == 0)
                      maj_x = 1;
                  break;

                case 8 :
                  if (maj_y == 0)
                      maj_y = 1;
                  break;

                case 9 :
                  if (maj_z == 0)
                      maj_z = 1;
                  break;

                case 10 :
                  tick_size = -0.015;
                  break;
                }
            }

        if (cli_end_of_command())
            {
            if (argc <= 3)
		{
		zero = 0;
		x_max = option_flip_x IN options ? wn[0] : wn[1];
		
		gus_axes_3d (&x_tick, &zero, &z_tick, &x_org, &y_org, &z_org,
		    &maj_x, &maj_y, &maj_z, &tick_size, NIL);
		
		tick_size = -tick_size;
		gus_axes_3d (&zero, &y_tick, &zero, &x_max, &y_org, &z_org,
		    &maj_x, &maj_y, &maj_z, &tick_size, NIL);
                }
	    else
	        gus_axes_3d (&x_tick, &y_tick, &z_tick, &x_org, &y_org, &z_org,
		    &maj_x, &maj_y, &maj_z, &tick_size, NIL);

            open_ws_server (update_ws);
            }
        else
            *stat = cli__maxparm;  /* maximum parameter count exceeded */
        }
}



static void plot_3d_titles (int *stat)

/*
 * plot_3d_titles - plot three-dimensional axis titles
 */

{
    int count;
    string titles_parm, x_title, y_title, z_title;

    strcpy (x_title, "X");
    strcpy (y_title, "Y");
    strcpy (z_title, "Z");

    count = 0;

    while (odd(*stat) && (count < 3) && (!cli_end_of_command()))
        {
        count++;

	cli_get_parameter (titles_3d_prompt[count-1], titles_parm, " ,", FALSE, 
	    TRUE, stat);

        if (odd(*stat))

            switch (count) {
                case 1 : strcpy (x_title, titles_parm); break;
                case 2 : strcpy (y_title, titles_parm); break;
                case 3 : strcpy (z_title, titles_parm); break;
                }
        }

    if (odd(*stat))
        {
        if (cli_end_of_command())
            {
	    gus_titles_3d (x_title, y_title, z_title, NIL);

            open_ws_server (update_ws);
            }
        else
            *stat = cli__maxparm;  /* maximum parameter count exceeded */
        }
}



void do_gus_command (int *stat)

/*
 * do_gus_command - parse a GUS command
 */

{
    int index;
    gus_command command;

    cli_get_keyword ("GUS", gus_command_table, &index, stat);

    command = (gus_command) index;

    if (odd(*stat))

        switch (command) {

            case command_set :
              set_command (stat);
              break;

            case command_autoscale_2d :
              autoscale_2d_command (stat);
              break;

	    case command_autoscale_3d :
              autoscale_3d_command (stat);
              break;

            case command_autoplot :
              autoplot_command (stat);
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

            case command_spline :
              spline_command (stat);
              break;

            case command_linfit :
              linfit_command (stat);
              break;

            case command_linreg :
              linreg_command (stat);
              break;

            case command_histogram :
              histogram_command (stat);
              break;

            case command_fft :
              fft_command (stat);
              break;

            case command_inverse_fft :
              inverse_fft_command (stat);
              break;

	    case command_pie_chart :
              pie_chart_command (stat);
              break;

	    case command_plot :
              plot_command (stat);
              break;

            case command_axes_2d :
              plot_2d_axes (stat);
              break;

            case command_grid :
              plot_grid (stat);
              break;

            case command_curve :
              curve_command (stat);
              break;

            case command_surface :
              plot_surface (stat);
              break;

            case command_colormap :
              colormap_command (stat);
              break;

            case command_contour :
              plot_contour (stat);
              break;

            case command_axes_3d :
              plot_3d_axes (stat);
              break;

            case command_error_bars:
              error_bars_command (stat);
              break;

            case command_bar_graph :
              bar_graph_command (stat);
              break;

            case command_titles_3d :
              plot_3d_titles (stat);
              break;
            }
}
