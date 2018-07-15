/*
 *
 * FACILITY:
 *
 *	Graphics Language Interpreter (GLI)
 *
 * ABSTRACT:
 *
 *	This module contains the Simple Plot command interpreter.
 *
 * AUTHOR:
 *
 *	Josef Heinen
 *
 * VERSION:
 *
 *	V1.0-00
 *
 */


#include <math.h>
#include <stdio.h>
#include <string.h>

#if !defined(VMS) && !defined(MSDOS) && !defined(_WIN32)
#include <unistd.h>
#endif

#include "system.h"
#include "mathlib.h"
#include "strlib.h"
#include "terminal.h"
#include "variable.h"
#include "command.h"
#include "gksdefs.h"
#include "gus.h" 


#define BOOL unsigned
#define NIL 0
#define TRUE 1
#define FALSE 0
#define odd(status) ((status) & 01)


#define pi 3.14159265
#define eps 1E-5
#define wkid 1
#define tnr 1
#define lcdnr 1
#define max_out_prim_points 1024
#define tick_size 0.01
#define major_count 5
#define sm_level 2

#define bold	01<<0
#define reverse 01<<1
#define color	01<<2


typedef char string[255];
typedef float out_prim_array[max_out_prim_points];


static tt_cid *conid;

static out_prim_array x, y, xw, yw, xs, ys;
static float f[2*max_out_prim_points];

static int first_point, last_point, n, nw, ns;
static int start_range, end_range, nr;
static int wk_id, t_nr, lcd_nr;
 

static char *choice_list =
   "\33[4;64H\33[1;4m SimplePlot V1.0 \33[m\
    \33[5;66H\33[1;4mA\33[mmplitude      \
    \33[6;66H\33[1;4mB\33[mandpass       \
    \33[7;66H\33[1;4mC\33[mubic Spline   \
    \33[8;66H\33[1;4mD\33[melete         \
    \33[9;66H\33[1;4mF\33[mFT            \
   \33[10;66H\33[1;4mH\33[migh Pass      \
   \33[11;66H\33[1;4mI\33[mnverse FFT    \
   \33[12;66H\33[1;4mL\33[minear Trend   \
   \33[13;66H\33[1;4mM\33[modify         \
   \33[14;66H\33[1;4mN\33[mew file       \
   \33[15;66H\33[1;4mO\33[mld file       \
   \33[16;66H\33[1;4mP\33[mhase          \
   \33[17;66H\33[1;4mR\33[mange          \
   \33[18;66H\33[1;4mS\33[mmooth         \
   \33[19;66H\33[1;4mT\33[mruncate       \
   \33[20;66H\33[1;4mU\33[mndo           \
   \33[21;66H\33[1;4mW\33[mrite file     \
   \33[22;66H\33[1;4mQ\33[muit           \
   \33[23;66H\33[1;4mZ\33[moom           \
   \33[1;3r\33[3H";

static char *pl_s[] = {"", "s"};



static int sign (float x)

/*
 * sign - return sign(x)
 */

{
    if (x == 0)
        return (0);
    else
        if (x < 0)
            return (-1);
        else
            return (1);
}


#ifndef min

static float min (float a, float b)

/*
 * min - return min(a, b)
 */

{
    if (a < b)
        return (a);
    else
        return (b);
}

#endif


#ifndef max

static float max (float a, float b)

/*
 * max - return max(a, b)
 */

{
    if (a > b)
        return (a);
    else
        return (b);
}

#endif


static void error (int status)

/*
 * error - signal error condition
 */

{
    status = status & ~STS_M_SEVERITY | STS_K_WARNING;
    raise_exception (status, 0, NULL, NULL);
}



static void open_ws_server (void (*actrtn) (int))

/*
 * open_ws_server - server for all open workstations
 */

{
    int state, i, n, errind, ol, wk_id;

    GQOPS (&state);
    if (state >= GWSOP)
        {
        n = 1;
        GQOPWK (&n, &errind, &ol, &wk_id);
        for (i = ol; i >= 1; i--)
            {
            n = i;
            GQOPWK (&n, &errind, &ol, &wk_id);
            actrtn (wk_id);
            }
        }
}



static void clear_ws (int wk_id)

/*
 * clear_ws - clear workstation action routine
 */

{
    int state;

    GQOPS (&state);
    if (state == GSGOP)
        GCLSG ();

    GCLRWK (&wk_id, &GALWAY);

    tt_printf (choice_list);
    tt_fflush (NULL);
}



static void update_ws (int wk_id)

/*
 * update_ws - update workstation action routine
 */

{
    GUWK (&wk_id, &GPOSTP);
}
                                                                  


static void request_locator (float *x, float *y)

/*
 * request_locator - request locator
 */

{
    int inp_dev_stat, errind, t_nr;
    float wn[4], vp[4];
    float a, b, c, d;

    GRQLC (&wk_id, &lcd_nr, &inp_dev_stat, &t_nr, x, y);
    GQCNTN (&errind, &t_nr);
    GQNT (&t_nr, &errind, wn, vp);

    a = (vp[1]-vp[0])/(wn[1]-wn[0]);
    b = vp[0]-wn[0]*a;
    c = (vp[3]-vp[2])/(wn[3]-wn[2]);
    d = vp[2]-wn[2]*c;

    *x = (*x-b)/a;
    *y = (*y-d)/c;
}



static void get_file_specification (char *file_spec, char *default_spec,
    int *stat)

/*
 * get_file_specification - get a file specification
 */

{
    string path;
    
    tt_get_line (conid, "Data file: ", file_spec, 0, TRUE, FALSE, NIL, stat);

    str_parse (file_spec, default_spec, FAll, path);
 
    if (!access (path, 4))
	*stat = RMS__NORMAL;
    else
	if (!access (path, 0))
	    *stat = RMS__ACC;
	else
	    *stat = RMS__FNF;

    strcpy (file_spec, path); 
}



static void read_file (char *file_spec, int *stat)

/*
 * read_file - read data file
 */

{
    FILE *data_file;
    int input_fields;

    data_file = fopen (file_spec, "r");
    if (data_file == NULL)
	{
	*stat = RMS__ACC;
        
	tt_fprintf (stderr, "Can't open file %s\n", file_spec);
	return;
	}

    n = 0;
    nw = 0;
    ns = 0;
    nr = 0;
    *stat = cli__normal;

    while (odd(*stat) && (n < max_out_prim_points) && (!feof(data_file)))
        {
        input_fields = fscanf (data_file, "%e %e", &x[n], &y[n]);

        if (input_fields == 2)
            {        
	    xw[n] = x[n];
            yw[n] = y[n];
            n++;
            }
	else if (input_fields >= 0)
	    {
	    *stat = RMS__ACC;
	    tt_fprintf (stderr, "File read error\n");
	    }
	}

    if (odd(*stat) && (!feof(data_file)))
        tt_fprintf (stderr, "%s%d%s\n", "File contains more than ",
	    max_out_prim_points, " points");

    fclose (data_file);

    if (n > 0)
        {
	*stat = cli__normal;
        
	tt_printf ("%d%s%s%s%s\n", n, " line", pl_s[n > 0], " read from file ", 
	    file_spec);

        first_point = 1;
        last_point = n;
        nw = n;
        start_range = 1;
        end_range = n;
        nr = n;
        }
}





static void plot_data (void)

/*
 * plot_data - plot data
 */

{
    float xmin, xmax, ymin, ymax, t_size;
    int i, m_count ;
    float x_tick, y_tick;
    float x1, x2, y1, y2;

    if (nw > 0)
        {
        t_nr = tnr; 
        open_ws_server (clear_ws);

        GSELNT (&t_nr);
        xmin = xw[first_point-1];
        xmax = xmin;
        ymin = yw[first_point-1];
        ymax = ymin;

        for (i = first_point; i <= last_point-1; i++)
            {
            xmin = min(xmin, xw[i]);
            xmax = max(xmax, xw[i]);
            ymin = min(ymin, yw[i]);
            ymax = max(ymax, yw[i]);
            }

        x1 = 0.1;
	x2 = 0.75;
	y1 = 0.1;
	y2 = 0.85;
        GSVP (&t_nr, &x1, &x2, &y1, &y2);
        GSWN (&t_nr, &xmin, &xmax, &ymin, &ymax);

        x_tick = gus_tick(&xmin, &xmax)/major_count;
        y_tick = gus_tick(&ymin, &ymax)/major_count;
        m_count = major_count;
	t_size = tick_size;
	gus_axes (&x_tick, &y_tick, &xmin, &ymin, &m_count, &m_count, &t_size,
	    NIL);

        GPL (&nw, xw, yw);
        }
    else
        tt_fprintf(stderr, "Data buffer is empty\n");
}



static void save_data (void)

/*
 * save_data - save data
 */

{
    int i;

    ns = nw;
    for (i = 0; i <= ns-1; i++)
        {
        xs[i] = xw[i];
        ys[i] = yw[i];
        }
}



static void plot_range (unsigned int options)

/*
 * plot_range - plot select range
 */

{
    float pm_size, pl_width;
    int pm_color, pl_color;

    pm_size   = 2.0;
    pm_color  = 0;
    pl_width  = 2.0;
    pl_color  = 0;

    if (nr == 1)
        {
        if (bold & options)
            GSMKSC (&pm_size);
        if (reverse & options)
            GSPMCI (&pm_color);
        pm_color = 2; 
        if (color & options)
            GSPMCI (&pm_color);

        GSMK (&GPLUS);
        GPM (&nr, &xw[start_range-1], &yw[start_range-1]);

        pm_size = 1.0;
        pm_color = 1;
        if (bold & options)
            GSMKSC (&pm_size);
        if (reverse & options)
            GSPMCI (&pm_color);
        if (color & options)
            GSPMCI (&pm_color);
        }
    else
        if (nr > 1)
            {
            if (bold & options)
                GSLWSC (&pl_width);
            if (reverse & options)
                GSPLCI (&pl_color);
            pl_color = 2;
            if (color & options)
                GSPLCI (&pl_color);

            GPL (&nr, &xw[start_range-1], &yw[start_range-1]);

	    pl_width  = 1.0;
	    pl_color  = 1; 
            if (bold & options)
                GSLWSC (&pl_width);
            if (reverse & options)
                GSPLCI (&pl_color);
            if (color & options)
                GSPLCI (&pl_color);
            }
}



static void amplitude (void)

/*
 * amplitude - compute amplitude
 */

{
    int j, m;

    tt_printf ("Amplitude\n");

    if (nw > 0)
        {
        m = 2;
        while (m < nw)
            m = m*2;

        for (j = 1; j <= nw; j++)
            f[j-1] = yw[j-1];
        for (j = nw+1; j <= 2*m; j++)
            f[j-1] = 0;

        mth_realft (f, m, 1);
        mth_fft (f, m, -1);

        for (j = 1; j <= 2*m; j++)
            f[j-1] = f[j-1]/m;

        save_data ();

        nw = nw/2;
        for (j = 1; j <= nw; j++)
            {
            xw[j-1] = 2*j-1;
            yw[j-1] = sqrt(pow(f[2*j-2], 2.0)+pow(f[2*j-1], 2.0));
            }

        first_point = 1;
        last_point = nw;
        start_range = 1;
        end_range = nw;
        nr = nw;

        plot_range (bold);
        }
    else
        tt_fprintf (stderr, "Data buffer is empty\n");
}



static void bandpass (void)

/*
 * bandpass - apply bandpass filter
 */

{
    float deltax, w, ymax;
    int j, m;

    tt_printf ("Bandpass filter\n");

    if (nw > 0)
        {
        m = 2;
        while (m < nw)
            m = m*2;

        ymax = yw[0];
        for (j = 2; j <= nw; j++)
            ymax = max(ymax, yw[j-1]);

        deltax = 0.1*(xw[end_range-1]-xw[start_range-1]);

        for (j = 1; j <= m; j++)
            {
            if (j > nw)
                w = 0;
            else if (xw[j-1] < xw[start_range-1])
          	w = 0;
            else if (xw[j-1] < xw[start_range-1] + deltax)
                w = 0.5*(1.0-cos(pi*(xw[j-1]-xw[start_range-1])/deltax));
            else if (xw[j-1] < xw[end_range-1] - deltax)
                w = 1;
            else if (xw[j-1] < xw[end_range-1])
                w = 0.5*(1.0-cos(pi*(xw[end_range-1]-xw[j-1])/deltax));
            else
                w = 0;

            f[2*j-2] = w*f[2*j-2];
            f[2*j-1] = w*f[2*j-1];
            yw[j-1] = w*ymax;
            }

        start_range = 1;
        end_range = nw;
        nr = nw;

        plot_range (bold);
        }
    else
        tt_fprintf (stderr, "Data buffer is empty\n");
}



static void cubic_spline (void)

/*
 * cubic_spline - compute cubic spline-fit
 */

{
    double dxw[max_out_prim_points], dyw[max_out_prim_points];
    double d[max_out_prim_points], f[max_out_prim_points];
    int i, m;

    tt_printf ("Cubic spline interpolation\n");

    if (nw > 2)
        {
        for (i = 1; i <= nw; i++)
            {
	    dxw[i] = xw[i];
	    dyw[i] = yw[i];
	    }

        m = 2;
        while (m < nw)
            m = m*2;

        for (i = 1; i <= m; i++)
            d[i-1] = xw[0]+(i-1)*(xw[nw-1]-xw[0])/(m-1);

        mth_spline (nw, dxw, dyw, m, d, f);

        save_data ();

        nw = m;
        for (i = 1; i <= nw; i++)
            {
            xw[i-1] = d[i-1];
            yw[i-1] = f[i-1];
            }

        first_point = 1;
        last_point = nw;
        start_range = 1;
        end_range = nw;
        nr = nw;

        plot_data ();
        }
}



static void delete_data (void)

/*
 * delete_data - delete data
 */

{
    float m, b;
    int i;

    tt_printf ("Delete\n");

    if (nr > 1)
        {
        m = (yw[end_range-1]-yw[start_range-1])/
	    (xw[end_range-1]-xw[start_range-1]);
        b = yw[start_range-1]-m*xw[start_range-1];

        plot_range (bold | reverse);

        save_data ();

        for (i = start_range; i <= end_range; i++)
            yw[i-1] = m*xw[i-1]+b;

        plot_range (bold | color);
        }
    else
        tt_fprintf (stderr, "No select range active\n");
}



static void fft (void)

/*
 * fft - Fast Fourier Transform
 */

{
    int j, m;

    tt_printf ("FFT\n");

    if (nw > 0)
        {
        m = 2;
        while (m < nw)
            m = m*2;

        for (j = 1; j <= nw; j++)
            f[j-1] = yw[j-1];
        for (j = nw+1; j <= 2*m; j++)
            f[j-1] = 0;

        mth_realft (f, m, 1);

        for (j = 1; j <= 2*m; j++)
            f[j-1] = 2*f[j-1]/m;

        save_data ();

        for (j = 1; j <= nw; j++)
            {
            xw[j-1] = j-1;
            yw[j-1] = sqrt(pow(f[2*j-2], 2.0)+pow(f[2*j-1], 2.0));
	    }

        first_point = 1;
        last_point = nw;
        start_range = 1;
        end_range = nw;
        nr = nw;

        plot_data ();
        }
    else
        tt_fprintf (stderr, "Data buffer is empty\n");
}



static void high_pass (void)

/*
 * high_pass - apply high pass filter
 */

{
    float deltax, w, ymax;
    int j, m;

    tt_printf ("High pass filter\n");

    if (nw > 0)
        {
        m = 2;
        while (m < nw)
            m = m*2;

        ymax = yw[0];
        for (j = 2; j <= nw; j++)
            ymax = max(ymax, yw[j-1]);

        deltax = xw[end_range-1]-xw[start_range-1];

        for (j = 1; j <= m; j++)
            {
            if (j > nw)
                w = 1;
            else if (xw[j-1] < xw[start_range-1])
                w = 0;
            else if (xw[j-1] < xw[start_range-1] + deltax)
                w = 0.5*(1-cos(pi*(xw[j]-xw[start_range])/deltax));
            else
                w = 1;

            f[2*j-2] = w*f[2*j-2];
            f[2*j-1] = w*f[2*j-1];
            yw[j-1] = w*ymax;
            }

        start_range = 1;
        end_range = nw;
        nr = nw;

        plot_range (bold);
        }
    else
        tt_fprintf (stderr, "Data buffer is empty\n");
}



static void inverse_fft (void)

/*
 * inverse_fft - inverse Forurier Transform
 */

{
    int j, m;

    tt_printf ("Inverse FFT\n");

    if (nw > 0)
        {
        m = 2;
        while (m < nw)
            m = m*2;

        mth_realft (f, m, -1);

        for (j = 1; j <= 2*m; j++)
            f[j-1] = f[j-1]/2*m;

        save_data ();

        for (j = 1; j <= nw; j++)
            {
            xw[j-1] = x[j-1];
            yw[j-1] = f[j-1];
	    }

        first_point = 1;
        last_point = nw;
        start_range = 1;
        end_range = nw;
        nr = nw;

        plot_data ();
        }
    else
        tt_fprintf (stderr, "Data buffer is empty\n");
}



static void linear_trend (void)

/*
 * linear_trend - remove linear trend
 */

{
    float x0, x1, y0, y1, m, b;
    int i;

    tt_printf ("Remove linear trend\n");

    if (nw > 1)
        {
        request_locator (&x0, &y0);
        request_locator (&x1, &y1);

        if (x0 != x1)
            {
            m = (y1-y0)/(x1-x0);
            b = y0-m*x0;
            }
        else
            mth_linreg (nw, xw, yw, &m, &b, FALSE);

        save_data ();

        for (i = 1; i <= nw; i++)
            yw[i-1] = yw[i-1]-(m*xw[i-1]+b);

        plot_data ();
        }
    else
        tt_fprintf (stderr, "Data buffer is empty\n");
}



static void modify (void)

/*
 * modify - modify data point
 */

{
    int i, nn;
    float xi, yi;
    int  pl_color;

    tt_printf ("Modify\n");

    if (nw > 0)
        {
        if (nr == 0)
            {
            request_locator (&xi, &yi);

            i = 1;
            while ((i < nw) && (xw[i-1] < xi))
                i = i+1;
            if ((i < nw) && (xi-xw[i-1] > xw[i]-xi))
                i = i+1;
            }
        else
	    {
            if (nr == 1)
                {
                i = start_range;
                request_locator (&xi, &yi);
                }
            else
                if (nr > 1)
                    {
                    i = start_range;

                    plot_range (bold | reverse);
                    plot_range (0);

                    end_range = start_range;
                    nr = 1;

                    plot_range (bold | color);

                    request_locator (&xi, &yi);
                    }
	     }	     

        if (i < nw)
            nn = 3;
        else
            nn = 2;

        pl_color = 0; 
        GSPLCI (&pl_color);
        GPL (&nn, &xw[i-2], &yw[i-2]);

        if (nr == 1)
            plot_range (bold | reverse);

        save_data ();

        yw[i-1] = yi;
        pl_color  = 1;
        GSPLCI (&pl_color);
        GPL (&nn, &xw[i-2], &yw[i-2]);

        if ((nr == 1) && (start_range < nw))
            {
            start_range = start_range+1;
            end_range = start_range;
            plot_range (bold | color);
            }
        }
    else
        tt_fprintf (stderr, "Data buffer is empty\n");
}



static void new_file (void)

/*
 * new_file - read new data file
 */

{
    string file_spec;
    int stat;

    tt_printf ("New file\n");

    get_file_specification (file_spec, ".dat", &stat);
    if (odd(stat))
        {
        read_file (file_spec, &stat);
        if (odd(stat))
            plot_data ();
        }

    if (!odd(stat))
        error (stat);
}



static void old_file (void)

/*
 * old_file - old data file
 */

{
    int i;

    tt_printf ("Old file\n");

    save_data ();

    nw = n;
    for (i = 1; i <= nw; i++)
        {
        xw[i-1] = x[i-1];
        yw[i-1] = y[i-1];
        }

    first_point = 1;
    last_point = nw;
    start_range = 1;
    end_range = nw;
    nr = nw;

    plot_data ();
}



static void phase (void)

/*
 * phase - phase spectrum
 */

{
    int j, m, npi;
    float phi0, phi;

    tt_printf ("Phase spectrum\n");

    if (nw > 0)
        {
        m = 2;
        while (m < nw)
            m = m*2;

        for (j = 1; j <= nw; j++)
            f[j-1] = yw[j-1];
        for (j = nw+1; j <= 2*m; j++)
            f[j-1] = 0;

        mth_realft (f, m, 1);
        mth_fft (f, m, -1);

        for (j = 1; j <= 2*m; j++)
            f[j-1] = f[j-1]/m;

        save_data ();

        nw = nw/2;
        npi = 0;
        phi0 = atan(f[0]/f[1]);

        for (j = 1; j <= nw; j++)
            {
            phi = atan(f[2*j-2]/f[2*j-1]);
            if (fabs(phi0-phi) > pi/2)
                npi = npi+sign(phi0-phi);
            phi0 = phi;

            xw[j-1] = 2*j-1;
            yw[j-1] = phi+npi*pi;
            }

        first_point = 1;
        last_point = nw;
        start_range = 1;
        end_range = nw;
        nr = nw;

        plot_data ();
        }
    else
        tt_fprintf (stderr, "Data buffer is empty\n");
}



static void range (void)

/*
 * range - select range
 */

{
    float x0, x1, y0, y1, temp;

    tt_printf ("Range\n");

    if (nw > 0)
        {
        request_locator (&x0, &y0);
        request_locator (&x1, &y1);

        if (x0 > x1)
            {
            temp = x0;
            x0 = x1;
            x1 = temp;
            temp = y0;
            y0 = y1;
            y1 = temp;
            }

        if (nr < nw)
            {
            plot_range (bold | reverse);
            plot_range (0);
            }

        start_range = 1;
        while ((start_range < nw) && (xw[start_range-1] < x0))
            start_range = start_range+1;

        end_range = nw;
        while ((end_range > 1) && (xw[end_range-1] > x1))
            end_range = end_range-1;

        nr = end_range-start_range+1;

        plot_range (bold | color);
        }
    else
        tt_fprintf (stderr, "Data buffer is empty\n");
}



static void smooth (void)

/*
 * smooth - smooth data
 */

{
    float smy[max_out_prim_points];
    int i;

    tt_printf ("Smooth\n");

    if (nr > sm_level)
        {
        mth_smooth (nr, &yw[start_range-1], &smy[start_range-1], sm_level);

        plot_range (bold|reverse);

        save_data ();

        for (i = start_range; i <= end_range; i++)
            yw[i-1] = smy[i-1];

        plot_range (bold|color);
        }
}



static void truncate_data (void)

/*
 * truncate_data - truncate data
 */

{
    float x0, y0, x1, y1, temp;
    int first, last, i;

    tt_printf ("Truncate\n");

    if (nw > 0)
        {
        request_locator (&x0, &y0);
        request_locator (&x1, &y1);

        if (x0 > x1)
            {
            temp = x0;
            x0 = x1;
            x1 = temp;
            }

        if (y0 > y1)
            {
            temp = y0;
            y0 = y1;
            y1 = temp;
            }

        first = 1;
        while ((first < nw) && (xw[first-1] < x0))
            first = first+1;

        last = nw;
        while ((last > 1) && (xw[last-1] > x1))
            last = last-1;

        if (first < last)
            {
            save_data ();

            nw = last-first+1;
            for (i = 1; i <= nw; i++)
                {
                xw[i-1] = xw[first+i-2];
                yw[i-1] = yw[first+i-2];
                }

            first_point = 1;
            last_point = nw;
            start_range = 1;
            end_range = nw;
            nr = nw;

            plot_data ();
            }
        }
    else
        tt_fprintf (stderr, "Data buffer is empty\n");
}



static void undo (void)

/*
 * undo - undo last operation
 */

{
    float temp;
    int i, m;

    tt_printf ("Undo\n");

    if (ns > 0)
        {
        m = nw;
        nw = ns;
        ns = m;
        if (ns > m)
            m = ns;

        for (i = 1; i <= m; i++)
            {
            temp = xw[i-1];
            xw[i-1] = xs[i-1];
            xs[i-1] = temp;
            temp = yw[i-1];
            yw[i-1] = ys[i-1];
            ys[i-1] = temp;
            }

        first_point = 1;
        last_point = nw;
        start_range = 1;
        end_range = nw;
        nr = nw;

        plot_data ();
        }
}



static void write_file (void)

/*
 * write - write data to file
 */

{
    string file_spec;
    int stat, i;
    FILE *data_file;

    get_file_specification (file_spec, ".dat", &stat);
    if (stat == RMS__FNF)
        stat = RMS__NORMAL;

    if (odd(stat))
        {
        data_file = fopen (file_spec, "w");
        i = 0;

        while ((i < nw) && (!cli_b_abort))
            {
            i = i+1;
            fprintf (data_file, "%e %e\n", xw[i-1], yw[i-1]);
            }

        fclose (data_file);

        if (i > 0)
            tt_printf ("%d%s%s%s%s\n", i, " line", pl_s[i > 0], 
		" written to file ", file_spec);
        }

    if (!odd(stat))
        error (stat);
}



static void zoom (void)

/*
 * zoom - zoom data
 */

{
    float x0, y0, x1, y1, temp;
    int first, last;

    tt_printf ("Zoom\n");

    if (nw > 0)
        {
        request_locator (&x0, &y0);
        request_locator (&x1, &y1);

        if (x0 > x1)
            {
            temp = x0;
            x0 = x1;
            x1 = temp;
            }

        if (y0 > y1)
            {
            temp = y0;
            y0 = y1;
            y1 = temp;
            }

        first = 1;
        while ((first < nw) && (xw[first-1] < x0))
            first = first+1;

        last = nw;
        while ((last > 1) && (xw[last-1] > x1))
            last = last-1;

        if (first < last)
            {
            first_point = first;
            last_point = last;
            plot_data ();
            }
        }
    else
        tt_fprintf (stderr, "Data buffer is empty\n");
}



static void left (void)

/*
 * left - zoom left half
 */

{
    int last;

    tt_printf ("Zoom left half\n");

    if (nw > 0)
        {
        last = last_point-(last_point-first_point+1)/2;
        if (first_point < last)
            {
            last_point = last;
            plot_data ();
            }
        }
    else
        tt_fprintf (stderr, "Data buffer is empty\n");
}



static void right (void)

/*
 * right - zoom right half
 */

{
    int first;

    tt_printf ("Zoom right half\n");

    if (nw > 0)
        {
        first = first_point+(last_point-first_point+1)/2;
        if (first < last_point)
            {
            first_point = first;
            plot_data ();
            }
        }
    else
        tt_fprintf (stderr, "Data buffer is empty\n");
}



static void plot (void)

/*
 * plot - plot data
 */

{
    tt_printf ("Plot\n");
    first_point = 1;
    last_point = nw;

    plot_data ();
}





void simpleplot (int *stat)

/*
 * command - parse a simpleplot command
 */

{

    char choice;

    conid = tt_connect ("tt", NIL);
    tt_clear_home (conid, NIL);

    tt_printf (choice_list);
    tt_fflush (NULL);

    wk_id = wkid;
    lcd_nr = lcdnr;
    t_nr = tnr;
 
    do
        {
        tt_printf ("\rSimplePlot> ");
	tt_fflush (NULL);

        tt_get_key (conid, &choice, 0, stat);

        if (odd(*stat))

            switch (choice) {

                case 'A' : case 'a' :   amplitude (); break;
                case 'B' : case 'b' :   bandpass (); break;
                case 'C' : case 'c' :   cubic_spline (); break;
                case 'D' : case 'd' :   delete_data (); break;
                case 'F' : case 'f' :   fft (); break;
                case 'H' : case 'h' :   high_pass (); break;
                case 'I' : case 'i' :   inverse_fft (); break;
                case 'L' : case 'l' :   linear_trend (); break;
                case 'M' : case 'm' :   modify (); break;
                case 'N' : case 'n' :   new_file (); break; 
                case 'O' : case 'o' :   old_file (); break;
                case 'P' : case 'p' :   phase (); break;
                case 'Q' : case 'q' :   *stat = tt__eof; break;
                case 'R' : case 'r' :   range (); break;
                case 'S' : case 's' :   smooth (); break;
                case 'T' : case 't' :   truncate_data (); break;
                case 'U' : case 'u' :   undo (); break;
                case 'W' : case 'w' :   write_file (); break;
                case 'Z' : case 'z' :   zoom (); break;

                case tt_k_ar_left :     left (); break;
                case tt_k_ar_right :    right (); break;
                case '\r' :             plot (); break;

                default :               break;
                }

        open_ws_server (update_ws);
        }

    while (odd(*stat));
 
    if (*stat == tt__eof)
        *stat = tt__normal;

    tt_clear_home (conid, stat);
    tt_set_terminal (conid, mode_numeric, FALSE, 0, 0, stat);
    tt_disconnect (conid, stat);
}
