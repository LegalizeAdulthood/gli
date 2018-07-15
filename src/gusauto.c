/*
 *
 * FACILITY:
 *
 *	GUSAUTOPLOT
 *
 * AUTHOR(S):
 *
 *	Josef Heinen
 *	Jochen Werner (C Version)
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
#include <stdlib.h>
#include <math.h>
#include <string.h>

#if !defined(VMS) && !defined(MSDOS) && !defined(_WIN32)
#include <unistd.h>
#endif

#include "variable.h"
#include "terminal.h"
#include "formdrv.h"
#include "command.h"
#include "strlib.h"
#include "mathlib.h"
#include "system.h"
#include "gus.h"
#include "gksdefs.h"


#define BOOL int
#define NIL 0
#define TRUE 1
#define FALSE 0

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif
#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif

#define odd(status) ((status) & 01)
#define present(status) ((status) != NIL)

#ifdef VMS
#define tty "tt"
#else
#define tty "tty"
#endif


/* Characters */

#define tab		   '\t'       
#define cr		   '\r'
#define blank		   ' '
#define plus		   '+'
#define minus              '-'
#define period             '.'        /* Other */
#define hyphen		   "-"
#define max_columns        8          /* maximum number of columns */
#define gks_ndc            0          /* default GKS transformation number */
#define tick_size          0.0075     /* default tick-size */
#define min_curve_points   256
#define data_string_len    256

typedef struct data_record_def {
    float                  value[max_columns + 1];
    struct data_record_def *next;
    } data_record;

typedef char data_string[data_string_len];

typedef enum {prim_polyline, prim_polymarker, prim_spline, 
	      prim_linfit, prim_linreg, prim_fft,
	      prim_inverse_fft} output_primitive;

typedef enum {scale_linear, scale_x_log, scale_y_log, scale_xy_log} axes_scale;

typedef enum {answer_no, answer_yes} answer;

typedef char string[15];

typedef struct {
    char		file_spec[35];
    gus_viewport_window window;
    output_primitive	out_prim;
    gus_viewport_size   size;
    int			smoothing;
    axes_scale		axes;
    answer		grid;
    float		min_x, x_tick, max_x;
    int			major_x;
    float		min_y, y_tick, max_y;
    int			major_y;
    char		main[35];
    char		x_axis[21];
    char		sub[35];
    char		y_axis[21];
    string		legend[max_columns];
} menu_data;

typedef unsigned short _uword;

/* Lookup tables */

extern int max_points;
extern float *px, *py;

static menu_data form;
static int opsta, errind, tnr;
static char result_spec[256];
static data_record *first_data, *data;
static int columns;
static BOOL new_frame_action_necessary, adjust;
static int fdv_stat;
static float vp[4];


static int iround (float x)

/*
 * iround - return round(x)
 */

{
    if (x<0)
	return ((int) (x-0.5));
    else
	return ((int) (x+0.5));
} 



static int gauss (float x)

/*
 * gauss - return gauss(x)
 */

{
    if (x >= 0 || x == (int) x)
        return ((int) x);
    else
        return ((int) x - 1);
}



static float fract (float x)

/*
 * fract - return fract(x)
 */

{
    return (x - (int) x);
}



static void adjust_range (float *amin, float *amax)

/*
 * adjust_range - adjust range
 */

{
    float tick;

    if (*amin == *amax)
        {
        if (*amin != 0)
            tick = pow(10.0, fract(log10(fabs(*amin))));
        else
            tick = 0.1;

        *amin = *amin - tick;
        *amax = *amax + tick;
        }

    tick = gus_tick(amin, amax);

    if (fract(*amin / tick) != 0)
        *amin = tick * gauss(*amin / tick);

    if (fract(*amax / tick) != 0)
        *amax = tick * (gauss(*amax / tick) + 1);
}



static void dispose_data (void)

/*
 * dispose_data - dispose data records
 */

{
    data_record *next_data;

    data = first_data;

    while (data != NIL)
        {
        next_data = data->next;
        free (data);
        data = next_data;
        }

    first_data = NIL;
    data = NIL;
}



static void clear_ws (void)

/*
 * clear_ws - clear all active workstations
 */

{
    int state, n, errind, ol, wkid;
    int conid, wtype, wkcat;

    GQOPS (&state);
    if (state == GSGOP)
        GCLSG ();

    if (state >= GWSAC)
        {
        new_frame_action_necessary = TRUE;
        adjust = FALSE;

        n = 0;
        do
            {
            n++;
            GQACWK (&n, &errind, &ol, &wkid);

	    GQWKC (&wkid, &errind, &conid, &wtype);
	    GQWKCA (&wtype, &errind, &wkcat);

	    if (wkcat == GOUTPT || wkcat == GOUTIN || wkcat == GMO)
                GCLRWK (&wkid, &GALWAY);
            }
        while (n < ol);
        }
}



static void set_ws_viewport (void)

/*
 * set_ws_viewport - set workstation viewport
 */

{
    int errind, ignore;
    gus_viewport_orientation orientation = orientation_landscape;
    float wn[4];

    GSELNT (&tnr);

    gus_set_ws_viewport (&form.window, &form.size, &orientation, &ignore);

    GQNT (&tnr, &errind, wn, vp);
}



static void find_file (char *file_spec, char *result_spec, char *default_spec,
    int *stat)
{
    if ((char *) getenv(file_spec) == 0)
	strcpy (result_spec, file_spec);
    else
        strcpy (result_spec, (char *) getenv(file_spec));

    str_parse (result_spec, default_spec, FAll, result_spec);
	
    if (access (result_spec, 4) == 0)
	*stat = RMS__NORMAL;
    else
	*stat = RMS__ACC;
}



static void read_file (void)

/*
 * read_file - process data file
 */

{
    int stat;
    FILE *data_file;

    data_string input_line;
    char *line;
    int column, lines;

    data_record *prev_data;

    BOOL sign;
    float value;

    char text[256];

    find_file (form.file_spec, result_spec, ".dat", &stat);

    if (odd(stat))
	{
	strcpy (text, "Reading ");
	strcat (text, result_spec);

	fdv_message (text, NIL);

	data_file = fopen (result_spec, "r");

	dispose_data ();
	lines = 0;
	columns = 0;

	while (odd(stat) && !cli_b_abort && 
	       fgets (input_line, data_string_len, data_file))
	    {
	    line = input_line;
	    lines++;

	    if (sscanf(line, "%e", &value) == 1)
		{
		prev_data = data;
		data = (data_record *) malloc (sizeof (data_record));
		data->next = NIL;

		if (prev_data != NIL)
		    prev_data->next = data;
		else
		    first_data = data;

		column = 0;
		while (sscanf (line, "%e", &value) == 1 &&
                    column <= max_columns)
		    {
		    while (!isdigit(*line) && *line != period && 
			*line != minus && *line != plus && *line != '\0')
			line++;  

		    data->value[column++] = value;

		    line++;
		    sign = (*line == plus || *line == minus) && *(line-1) !=
			'e' && *(line-1) != 'E';

		    while (*line != blank && *line != tab && *line != '\0' &&
			    !sign)
			{
			line++;
			sign = (*line == plus || *line == minus) && *(line-1) !=
			    'e' && *(line-1) != 'E';
			}
		    }

		if (columns == 0)
		    columns = --column;

		while (column < columns)
		    data->value[column++] = 0;
		}
	    }

	if (odd(stat))
	    { 
	    str_dec (text, lines);
	    strcat (text, " lines read from file ");
	    strcat (text, result_spec);
	    tt_printf ("%s\n", text);
	    }
	else
	    stat = gus__illfmt;

	/* file has illegal format */

	fclose (data_file);

	data = first_data;
	}

    if (!odd(stat))
	{
	raise_exception (stat, 0, NULL, NULL);
	fdv_message ("?File not found", NIL);
	fdv_stat = fdv__error;
	}
}



static void update_open_ws (void)

/*
 * update_open_ws - update all open workstations
 */

{
    int state, count, n, errind, ol, wkid;
    int conid, wtype, wkcat;

    GQOPS (&state);
    if (state >= GWSOP)
        {
        n = 1;
        GQOPWK (&n, &errind, &ol, &wkid);

        for (count = 1; count <= ol; count++)
            {
            n = count;
            GQOPWK (&n, &errind, &ol, &wkid);

	    GQWKC (&wkid, &errind, &conid, &wtype);
	    GQWKCA (&wtype, &errind, &wkcat);

	    if (wkcat == GOUTPT || wkcat == GOUTIN)
	        GUWK (&wkid, &GPOSTP);
            }
        }
}



static void plot_chart (void)

/*
 * plot_chart - plot chart
 */

{
    int errind, level, ltype, mtype, halign, valign, font, prec;
    int n, m, column, curve;

    float sm, vx_min, vx_max, vx_centr, vy, vy_min, vy_max;
    int linetype, markertype;
    float chxp, factor, chh, height;

    int options;

    float x1, y1, t_size;
    int t_nr, np, m_x, m_y;

    vx_min = vp[0];
    vx_max = vp[1];
    vy_min = vp[2];
    vy_max = vp[3];

    for (column = 0; column < max_columns; column++)
	if (strlen(form.legend[column]) && strcmp(form.legend[column], hyphen))
	    {
	    vx_min += 0.1;
	    break;
	    }

    vx_centr = (vx_min + vx_max) / 2.;

    /* save smoothing level, linetype, marker type, text alignment,
       character expansion factor and character height */

    gus_inq_smoothing (&level, NIL);

    GQLN (&errind, &ltype);
    GQMK (&errind, &mtype);
    GQTXAL (&errind, &halign, &valign);
    GQCHXP (&errind, &chxp);
    GQCHH (&errind, &chh);
    GQTXFP (&errind, &font, &prec);

    if (new_frame_action_necessary)
	{
	t_nr = gks_ndc;
	GSELNT (&t_nr);
	GSTXAL (&GACENT, &GABOTT);
	if (prec == GSTRKP)
	    factor = 1/sqrt(2.0);
	else
	    factor = 1.0;
	GSCHXP (&factor);
	height = 0.032;
	GSCHH (&height);

	if (strlen (form.main) > 0)
	    {
	    vy_max -= 0.025;
	    vy = vy_max + 0.04;
	    gus_text (&vx_centr, &vy, form.main, NIL);
	    }

	if (strlen(form.sub) > 0)
	    {
	    vy = vy_max;
	    gus_text (&vx_centr, &vy, form.sub, NIL);
	    }

	GSTXAL (&GALEFT, &GABOTT);

	if (strlen(form.y_axis) > 0)
	    {
	    vy = vy_max + 0.01;
	    gus_text (&vx_min, &vy, form.y_axis, NIL);
	    }

	GSTXAL (&GACENT, &GABOTT);

	if (strlen(form.x_axis) > 0)
	    {
	    y1 = vy_min - 0.1;
	    gus_text (&vx_centr, &y1 , form.x_axis, NIL);
	    }

	linetype = ltype;
	markertype = mtype;
	curve = 0;

	for (column = 0; column < max_columns; column++)

	    if (strlen(form.legend[column]) &&
                strcmp(form.legend[column], hyphen))
		{
		curve++;

		px[0] = 0.05;
		py[0] = vy_max-curve * 0.075;
		px[1] = 0.15;
		py[1] = py[0];

		x1 = 0.1;
		y1 = py[0] + 0.01;
		gus_text (&x1, &y1, form.legend[column], NIL);

		switch (form.out_prim) {

		    case prim_polymarker :
		      GSMK (&markertype);
		      np = 2;
		      GPM (&np, px, py);
		      break;

		    case prim_polyline :
		    case prim_spline :
		    case prim_linfit :
		    case prim_linreg :
		    case prim_fft :
		    case prim_inverse_fft :
		      GSLN (&linetype);
		      np = 2;
		      GPL (&np, px, py);
		      break;
		    }

                switch (form.out_prim) {

                    case prim_polyline :
                    case prim_spline :
                    case prim_linfit :
                    case prim_linreg :
                    case prim_fft :
                    case prim_inverse_fft :
                        if (linetype > 0)
                        {
                            if (++linetype > GLDASD)
                                linetype = GLSOLI;
                        }
                        else
                        {
                            if (--linetype < GLTPDT)
                                linetype = GLDS2D;
                        }
                        break;

                    case prim_polymarker :
                        if (markertype > 0)
                        {
                            if (++markertype > GXMARK)
                                markertype = GPOINT;
                        }
                        else
                        {
                            if (--markertype < GMDIA)
                                markertype = GMSCIR;
                        }
                        break;
                    }
                }

	GSELNT (&tnr);
	GSVP (&tnr, &vx_min, &vx_max, &vy_min, &vy_max);
	GSWN (&tnr, &form.min_x, &form.max_x, &form.min_y, &form.max_y);

	options = (int) form.axes;
	gus_set_scale (&options, NIL);

	t_size = tick_size;

	gus_axes (&form.x_tick, &form.y_tick, &form.min_x,
	    &form.min_y, &form.major_x, &form.major_y, &t_size, NIL);

	m_x = -form.major_x;
	m_y = -form.major_y;
	t_size = -tick_size;

	gus_axes (&form.x_tick, &form.y_tick, &form.max_x,
	    &form.max_y, &m_x, &m_y, &t_size, NIL);

	if (form.grid == answer_yes)
	    gus_grid (&form.x_tick, &form.y_tick, &form.min_x,
		      &form.min_y, &form.major_x, &form.major_y,
		      NIL);
	}

    GSELNT (&tnr);

    /* set desired smoothing level */

    gus_set_smoothing (&form.smoothing, NIL);

    linetype = ltype;
    markertype = mtype;

    for (column = 0; column < columns; column++)

	if (!strlen(form.legend[column]) ||
            strcmp(form.legend[column], hyphen))
	    {
	    data = first_data;

	    while ((data != NIL) && (!cli_b_abort))
		{
		n = 0;
		while ((data != NIL) && (n < max_points))
		    {
		    px[n] = data->value[0];
		    py[n] = data->value[column+1];
		    if (++n < max_points)
			data = data->next;
		    }

		switch (form.out_prim) {

		    case prim_polyline :
		      GSLN (&linetype);
		      gus_polyline (&n, px, py, NIL);
		      break;

		    case prim_polymarker :
		      GSMK (&markertype);
		      gus_polymarker (&n, px, py, NIL);
		      break;

		    case prim_spline :
		      GSLN (&linetype);
		      if (n < 64)
			  m = min_curve_points;
		      else
			  m = 1024;
		      sm = 0;
		      gus_spline (&n, px, py, &m, &sm, NIL);
		      break;

		    case prim_linfit :
		      GSLN (&linetype);
		      gus_linfit (&n, px, py, NIL);
		      break;

		    case prim_linreg :
		      GSLN (&linetype);
		      gus_linreg (&n, px, py, NIL);
		      break;

		    case prim_fft :
		      GSLN (&linetype);
		      if (n > 0)
			  m = iround(pow(2.0, (int) (log(n) / log(2.))));
		      else
			  m = min_curve_points;
		      gus_fft (&n, py, &m, NIL);
		      break;

		    case prim_inverse_fft :
		      GSLN (&linetype);
		      gus_inverse_fft (&n, px, py, NIL);
		      break;
		    }
		}

	    switch (form.out_prim) {

		case prim_polyline :
		case prim_spline :
		case prim_linfit :
		case prim_linreg :
		case prim_fft :
		case prim_inverse_fft :
		  if (linetype > 0)
		  {
		      if (++linetype > GLDASD)
			  linetype = GLSOLI;
		  }
		  else
		  {
		      if (--linetype < GLTPDT)
			  linetype = GLDS2D;
		  }
		  break;

		case prim_polymarker :
		  if (markertype > 0)
		  {
		      if (++markertype > GXMARK)
			  markertype = GPOINT;
		  }
		  else
		  {
		      if (--markertype < GMDIA)
			  markertype = GMSCIR;
		  }
		  break;
		}
	    }

    /* update all open workstation */

    update_open_ws ();
    new_frame_action_necessary = FALSE;
    GSELNT (&tnr);

    /* restore smoothing level, marker type, text alignment,
       character expansion factor and character height */

    gus_set_smoothing (&level, NIL);

    GSLN (&ltype);
    GSMK (&mtype);	    
    GSTXAL (&halign, &valign);
    GSCHXP (&chxp);
    GSCHH (&chh);

    if (!cli_b_abort && (char *) getenv("GLI_GUI") == NULL)
	cli_await_key ("Press Return to continue, Control Z to exit", cr,
			&fdv_stat);

    if (!odd(fdv_stat))
	fdv_stat = fdv__quit;
}



static void plot (void)

/*
 * plot - plot routine
 */

{ 
    if (first_data == NIL)
        read_file ();

    if (odd (fdv_stat))
        fdv_call (plot_chart, NIL);
}



static void autoscale (void)

/*
 * autoscale - automatic scaling routine
 */

{
    int column;
    data_record *data;
    float x[2], y[2];

    int np;

    if (first_data == NIL)
        read_file ();

    if (odd(fdv_stat))
        {
        data = first_data;

        if (data != NIL)
            {
            x[0] = data->value[0];
            x[1] = x[0];
            y[0] = data->value[1];
            y[1] = y[0];

	    while (data != NIL)
		{
		x[0] = min(x[0], data->value[0]);
		x[1] = max(x[1], data->value[0]);

		for (column = 0; column < columns; column++)
                    if (!strlen(form.legend[column]) ||
                        strcmp(form.legend[column], hyphen))
			{
			y[0] = min (y[0], data->value[column+1]);
			y[1] = max (y[1], data->value[column+1]);
			}

		data = data->next;
		}

	    np = 2;
	    gus_autoscale (&np, x, y, &form.min_x, &form.max_x,
		&form.min_y, &form.max_y, NIL);

	    if (adjust)
		{
		adjust_range (&form.min_x, &form.max_x);
		adjust_range (&form.min_y, &form.max_y);
		}

	    form.x_tick = gus_tick(&form.min_x, &form.max_x);
	    form.major_x = 1;
	    form.y_tick = gus_tick(&form.min_y, &form.max_y);
	    form.major_y = 1;

            adjust = !adjust;
            }
        }
}



static void uar (int *efn, int *status)

/*
 * uar - user-action routine
 */

{ 
    fdv_stat = *status;

    switch (*efn) {

	case fdv__hangup :
	  fdv_stat = fdv__quit;
	  break;

	case 1 :
	  read_file ();
	  break;

	case 2 :
	  new_frame_action_necessary = TRUE;
	  set_ws_viewport ();
	  break;

	case 4 :
	  if (((form.min_x <= 0) && ((form.axes == scale_x_log) ||
	      (form.axes == scale_xy_log))) || ((form.min_y <= 0) && 
	      ((form.axes == scale_y_log) || (form.axes == scale_xy_log))))
	      {
	      fdv_message (
	          "?Cannot apply transformation to current window", NIL);
	      fdv_stat = fdv__error;
	      }
	  else
	      {
	      clear_ws ();
	      fdv_stat = fdv__normal;
	      }
	  break;

	case 5 :
	  if ((form.min_x >= form.max_x) || (form.min_y >= form.max_y))
	      {
	      fdv_message ("?Invalid window specification", NIL);
	      fdv_stat = fdv__error;
	      }
	  else
	      {
	      GSWN (&tnr, &form.min_x, &form.max_x, &form.min_y, &form.max_y);

	      fdv_message ("?Axes limits are assumed to be arbitrary",NIL);
	      fdv_stat = fdv__retain;
	      }
	  break;

	case 6 :
	  if ((form.x_tick < 0) || (form.y_tick < 0))
	      {
	      fdv_message ("?Invalid tick-length", NIL);
	      fdv_stat = fdv__error;
	      }
	  else
	      {
	      clear_ws ();
	      fdv_stat = fdv__normal;
	      }
	  break;

	case 7 :
	  clear_ws ();
	  fdv_stat = fdv__normal;
	  break;

	case fdv_c_init :
	  set_ws_viewport ();
	  break;

	case fdv_c_f1 :
	  if ((form.min_x == form.max_x) || (form.min_y == form.max_y))
	      {
	      clear_ws ();
	      autoscale ();
	      }
	  plot ();
	  break;

	case fdv_c_f2 :
	  autoscale ();

	  if (!adjust)
	      {
	      fdv_message ("?Axes limits adjusted to suitable values", NIL);
	      fdv_stat = fdv__retain;
	      }
	  else

	      if (!new_frame_action_necessary)
		  {
		  fdv_message ("?Axes limits are assumed to be arbitrary", NIL);
		  fdv_stat = fdv__retain;
		  }
	  break;

	case fdv_c_f3 :
	  clear_ws ();
	  break;

	case fdv_c_f4 :
	  fdv_stat = fdv__exit;
	  break;

	case fdv_c_ctrl_z :
	  fdv_stat = fdv__quit;
	  break;

	default :
	  fdv_stat = fdv__normal;
	}

    if (cli_b_abort)
        {
        fdv_message ("?Operation was canceled by keyboard action", NIL);
        fdv_stat = fdv__retain;

        cli_b_abort = FALSE;
        }

    *status = fdv_stat;
}



void gus_autoplot (int *status)

/*
 * gus_autoplot - display autoplot menu
 */

{
    char *path, autoplot_menu[80];
    int stat;

    path = (char *) getenv ("GLI_HOME");
    if (path)
        {
	strcpy (autoplot_menu, path);
#ifndef VMS
#ifdef _WIN32
	strcat (autoplot_menu, "\\");
#else
	strcat (autoplot_menu, "/");
#endif
#endif
	strcat (autoplot_menu, "autoplot.fdv");
        }
    else
#ifdef VMS
        strcpy (autoplot_menu, "sys$sysdevice:[gli]autoplot.fdv");
#else
#ifdef _WIN32
        strcpy (autoplot_menu, "c:\\gli\\autoplot.fdv");
#else
        strcpy (autoplot_menu, "/usr/local/gli/autoplot.fdv");
#endif
#endif

    find_file (autoplot_menu, result_spec, ".fdv", &stat);

    if (odd(stat))
        {
        fdv_load (result_spec, tty, NIL, "", &stat);

        if (odd(stat))
            {

            /* inquire and test GKS operating state */

            GQOPS (&opsta);

            if ((opsta == GWSAC) || (opsta == GSGOP))
                {
                clear_ws ();

                GQCNTN (&errind, &tnr);

                first_data = NIL;

                fdv_disp (0, uar, (unsigned *)&form, mode_conversational,
                    0, &stat);

                dispose_data ();
                }
            else

                /* GKS not in proper state. GKS must be either in the state 
		   WSAC or SGOP */

                stat = gus__notact;
            }
        }

    if (present(status))
        *status = stat;
    else
        if (!odd (stat))
            gus_signal (stat, "AUTOPLOT");
} 
