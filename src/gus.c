/*
 *
 * (C) Copyright 1991 - 1999  Josef Heinen
 *
 *
 * FACILITY:
 *
 *	Graphic Utility System (GUS)
 *
 * ABSTRACT:
 *
 *	This module contains a Graphic Utility System (GUS) based on
 *	an implementation of the Graphical Kernel System (GKS) Version 7.4
 *
 * AUTHOR(S):
 *
 *	Josef Heinen
 *	Jochen Werner (C Version)
 *
 * VERSION:
 *
 *	V4.5
 *
 * MODIFIED BY:
 *
 */


#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <limits.h>

#ifdef VAXC
#ifndef __ALPHA
#pragma builtins
#endif
#endif

#ifdef VMS
#include <descrip.h>
#endif

#ifdef cray
#include <fortran.h>
#endif

#define __gus 1

#include "system.h"
#include "strlib.h"
#include "symbol.h"
#include "variable.h"
#include "function.h"
#include "terminal.h"
#include "command.h"
#include "formdrv.h"
#include "mathlib.h"
#include "gus.h"
#include "gksdefs.h"
#include "frtl.h"

#ifndef PI
#define PI		    3.1415927
#endif
#define gks_ndc		    0		/* default GKS transformation number */
#define background	    0		/* background color index */
#define max_prim_points	    100		/* maximum number of points */
#define min_curve_points    256
#define max_real	    1E20
#define resolution_x	    4096	/* resolution for hidden-line removal */
#define first_color	    8		/* first color */
#define last_color	    79		/* last color */
#define tick_size	    0.0075	/* default tick size */
#define error_tick_size	    0.0075	/* default error bar tick size */

#define contour_lines	    16		/* default number of contour lines */
#define contour_map_rate    0.5
#define contour_min_chh     0.005       /* minimum character height */
#define contour_max_length  1.2         /* maximum length of a labelled line */
#define contour_min_length  0.15        /* minimum length of a labelled line */
#define contour_max_pts     1000        /* maximum number of points */

#define pie_radius          0.6         /* Factor for scaling a circle */
#define pie_colors          8		/* Number of colors, patterns, and */
#define hatch_styles        12		/* hatch styles */
#define patterns            16
#define pie_line	    0.80
#define pie_offset	    0.02

#define BOOL unsigned
#define NIL 0
#define TRUE 1
#define FALSE 0

#ifndef FLT_MAX
#define FLT_MAX 1.7E38
#endif

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif
#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif

#define odd(status) ((status) & 01)
#define IN &
#define present(status) ((status) != NIL)

#define left   (1<<0)
#define right  (1<<1)
#define front  (1<<2)
#define back   (1<<3)
#define bottom (1<<4)
#define top    (1<<5)

enum contour_op {CT_INIT, CT_ADD, CT_SUB, CT_EVAL, CT_END};

typedef struct {
    char   lblfmt[10];
    int    lblmjh;
    int    txtflg;
    int    xdim, ydim;
    int    wkid;
    int    tnr, ndc;
    float  *z;
    float  scale_factor, aspect_ratio, vp[4], wn[4];
    float  xmin, ymin, dx, dy;
    float  zmin, zmax;
    int    start_index;
    int    end_index;
    double *gradient_mag;
    double *variance_list;
    int    *label_map;
    int    x_map_size, y_map_size;
    double x_map_factor, y_map_factor;
    } contour_vars_t;

typedef unsigned int outcode;

typedef struct {
    float a, b, c, d;
    } norm_xform;

typedef struct {
    int scale_options;
    float x_min, x_max, y_min, y_max, z_min, z_max, a, b, c, d, e, f;
    } linear_xform;

typedef struct {
    float z_min, z_max;
    int phi, delta;
    float a1, a2, b, c1, c2, c3, d;
    } world_xform;

typedef struct {
    int sign;
    float x0, x1, y0, y1, z0, z1;
    float xmin, xmax;
    BOOL initialize;
    float *buf, *ymin, *ymax;
    } hlr_t;


/* Global Variables */

static int n_points;
static float px[max_prim_points], py[max_prim_points], pz[max_prim_points];

static int logging_switch = FALSE;
static int gus_gl_smoothing_level = 0;
static gus_primitive primitive = primitive_polyline;

float gus_gf_linreg_m = 0;
float gus_gf_linreg_b = 0;
float gus_gf_linfit_m = 0;
float gus_gf_linfit_b = 0;
float gus_gf_linfit_dev = 0;
float gus_gf_missing_value = FLT_MAX;

static norm_xform nx = {0, 1, 0, 1};
static linear_xform lx = {0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0};
static world_xform wx = {0, 1, 60, 60, 0, 0, 0, 0, 0, 0, 0};

static contour_vars_t contour_vars;

static hlr_t hlr = {1, 0, 1, 0, 1, 0, 1, 0, 1, TRUE, NULL, NULL, NULL};

/* Lookup tables */

static float vp_size[3][2] = {
    {0.21, 0.1485}, {0.297, 0.21}, {0.42, 0.297} };

static float vp_wn[7][4] = {
    {0, 1, 0, 1}, {0, 1, 0.5, 1}, {0, 1, 0, 0.5}, {0, 0.5, 0.5, 1},
    {0.5, 1, 0.5, 1}, {0, 0.5, 0, 0.5}, {0.5, 1, 0, 0.5} };

/*    0 - 20    21 - 45    46 - 70    71 - 90       rot/      */
static int rep_table[16][3] = {                    /*   tilt  */
    {2, 2, 2}, {2, 2, 2}, {1, 3, 2}, {1, 3, 3},    /*  0 - 20 */
    {2, 0, 0}, {2, 0, 0}, {1, 3, 2}, {1, 3, 1},    /* 21 - 45 */
    {2, 0, 0}, {0, 0, 0}, {1, 3, 0}, {1, 3, 1},    /* 46 - 70 */
    {0, 0, 0}, {0, 0, 0}, {1, 3, 0}, {1, 3, 1} };  /* 71 - 90 */

static int axes_rep[4][3] = {
    {1, 1, 1}, {2, 1, 1}, {0, 0, 0}, {3, 0, 0} };

static int angle[4] = {20, 45, 70, 90};

static int cmap[72][3] = {
    { 27,  27,  29},
    {  9,   9,  19},
    { 17,  17,  34},
    { 24,  24,  49},
    { 31,  31,  62},
    { 39,  39,  77},
    { 45,  45,  90},
    { 53,  53, 106},
    { 60,  60, 120},
    { 67,  67, 133},
    { 74,  74, 149},
    { 81,  81, 161},
    { 89,  89, 177},
    { 95,  95, 190},
    {103, 103, 205},
    {110, 110, 219},
    {116, 116, 233},
    {125, 124, 250},
    {123, 131, 244},
    {106, 139, 211},
    { 93, 145, 185},
    { 78, 152, 156},
    { 62, 160, 124},
    { 49, 166,  99},
    { 33, 174,  67},
    { 21, 181,  42},
    {  5, 189,  11},
    { 11, 192,   0},
    { 32, 192,   0},
    { 56, 192,   0},
    { 74, 192,   0},
    { 99, 192,   0},
    {120, 192,   0},
    {141, 192,   0},
    {163, 192,   0},
    {183, 192,   0},
    {190, 187,   5},
    {186, 181,  11},
    {182, 173,  19},
    {179, 166,  26},
    {175, 159,  33},
    {171, 150,  41},
    {168, 144,  47},
    {164, 136,  55},
    {161, 130,  61},
    {154, 122,  61},
    {147, 115,  57},
    {140, 108,  54},
    {133, 101,  50},
    {126,  94,  47},
    {118,  86,  43},
    {112,  80,  40},
    {104,  72,  36},
    { 97,  65,  32},
    { 97,  68,  39},
    {102,  76,  51},
    {105,  83,  61},
    {109,  90,  72},
    {112,  97,  82},
    {116, 104,  93},
    {120, 112, 105},
    {122, 118, 114},
    {127, 127, 126},
    {138, 138, 138},
    {153, 153, 153},
    {168, 168, 168},
    {181, 181, 181},
    {197, 197, 197},
    {209, 209, 209},
    {225, 225, 225},
    {239, 239, 239},
    {251, 251, 251}
};

#if defined(hpux) && !defined(NAGware)
static int (*draw_a)();
#endif


static int iround (float x)

/*
 * iround - return round(x)
 */

{
    if (x < 0)
	return ((int) (x - 0.5));
    else
	return ((int) (x + 0.5));
} 



static int gauss (float x)

/*
 * gauss - return gauss(x)
 */

{
    if ((x >= 0) || (x == (int) x))
        return ((int) x);
    else
        return ((int) x - 1);
}



static int ipred (float x)

/*
 * ipred - return the predecessor value of x
 */

{
    if (x == (int) x)
        return ((int) x - 1);
    else
        return (gauss(x));
}



static int isucc (float x)

/*
 * isucc - return the successor value of x
 */

{
    if (x == (int) x)
        return ((int) x);
    else
        return (gauss(x) + 1);
}



static float deg (float rad)

/*
 * deg - return deg(rad)
 */

{
    return (rad * 180./PI);
}



static float arc (int angle)

/*
 * arc - return arc(angle)
 */

{
    return (PI * (float) angle / 180.);
}



static float atan_2 (float x, float y)

/*
 * atan_2 - return atan(x/y)
 */

{
    float a;

    if (y == 0)
	if (x < 0) 
	    a = -PI;
	else
	    a = PI;
    else
	a = atan(x/y);
 
    return (a);
}



void gks_inq_text_extent_s (int *wkid, float *x, float *y, int *nchars, 
    char *chars, int *errind, float *cpx, float *cpy, float *tx, float *ty)
{
#ifdef VMS
    struct dsc$descriptor_s text;

    text.dsc$b_dtype = DSC$K_DTYPE_T;
    text.dsc$b_class = DSC$K_CLASS_S;
    text.dsc$w_length = *nchars;
    text.dsc$a_pointer = chars;

    GQTXXS (wkid, x, y, nchars, &text, errind, cpx, cpy, tx, ty);
#else
#ifdef cray
    _fcd text;

    text = _cptofcd(chars, *nchars);

    GQTXXS (wkid, x, y, nchars, text, errind, cpx, cpy, tx, ty);
#else
#if defined(_WIN32) && !defined(__GNUC__)
    unsigned short chars_len = *nchars;

    GQTXXS (wkid, x, y, nchars, chars, chars_len, errind, cpx, cpy, tx, ty);
#else
    GQTXXS (wkid, x, y, nchars, chars, errind, cpx, cpy, tx, ty, *nchars);
#endif /* _WIN32 */
#endif /* cray */
#endif /* VMS */
}



void gks_text_s (float *px, float *py, int *nchars, char *chars)
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



static char *gmalloc (int size)
{
    register char *p;

    p = (char *) malloc (size);
    if (p == NULL)
	{
	tt_fprintf (stderr, "GUS: not enough core\n");
	exit (-1);
	}

    return p;
}



#if defined(_WIN32) && !defined(__GNUC__)
void __stdcall FILL0 (int *bitmap, int *n)
#else
void FILL0 (int *bitmap, int *n)
#endif

/*
 * FILL0 - clear the first n elements of bitmap
 */

{
    memset((void *) bitmap, 0, (*n) * sizeof(int));
}



#define Z(i,j) ((double) contour_vars.z[(i) + contour_vars.xdim*(j)])

/*------------------------------------------------------------------------------
/ This gradient maintains a moving average of the magnitude of the gradient
/ vector at points along the contour line.  The idea behind this routine is that
/ at places where the magnitude is small, the countour lines will tend to be
/ farther apart thus allowing more room to write a label.
/-----------------------------------------------------------------------------*/

static void gradient (int ind, int n, float *xpts, float *ypts,
    enum contour_op op)
{
    int                 i, j;
    static int          count;
    double              t;
    double              xg1, yg1, xg2, yg2;
    double              xgrad, ygrad;
    double              txpt, typt;
    static double       *magnitude;
    static double       sum;
    static double       max_mag;

    /* Since GCONTR is called with different values than            */
    /* xmin=ymin=0 and dx=dy=1, the computed contour-lines must be  */
    /* retransformed for gradient-calculations.                     */
    /* Otherwise, selected places for label could be wrong.         */

    txpt = (double) ((xpts[ind] - contour_vars.xmin) /
                     contour_vars.dx);
    typt = (double) ((ypts[ind] - contour_vars.ymin) /
                     contour_vars.dy);

    switch (op)
    {
        case CT_INIT:
            magnitude = (double *)gmalloc (n*sizeof (double));
            count = 0;
            sum = 0.0;
            max_mag = 0.0;
            break;

        case CT_ADD:
            i = (int) txpt;
            j = (int) typt;

            if (i == 0)
                xg1 = Z(i+1,j) - Z(i,j);
            else if (i == contour_vars.xdim-1)
                xg1 = Z(i,j) - Z(i-1,j);
            else
                xg1 = (Z(i+1,j) - Z(i-1,j)) / 2.0;

            if (j == 0)
                yg1 = Z(i,j+1) - Z(i,j);
            else if (j == contour_vars.ydim-1)
                yg1 = Z(i,j) - Z(i,j-1);
            else
                yg1 = (Z(i,j+1) - Z(i,j-1)) / 2.0;

            if (i == txpt)
            {
                t = typt - (double)j;
                j++;
            }
            else
            {
                t = txpt - (double)i;
                i++;
            }

            if (i == 0)
                xg2 = Z(i+1,j) - Z(i,j);
            else if (i == contour_vars.xdim-1)
                xg2 = Z(i,j) - Z(i-1,j);
            else
                xg2 = (Z(i+1,j) - Z(i-1,j)) / 2.0;

            if (j == 0)
                yg2 = Z(i,j+1) - Z(i,j);
            else if (j == contour_vars.ydim-1)
                yg2 = Z(i,j) - Z(i,j-1);
            else
                yg2 = (Z(i,j+1) - Z(i,j-1)) / 2.0;

            xgrad = xg1 + t * (xg2 - xg1);
            ygrad = yg1 + t * (yg2 - yg1);
            magnitude [ind] = xgrad*xgrad + ygrad*ygrad;
            sum += magnitude[ind];
            ++count;
            break;

        case CT_SUB:
            sum -= magnitude[ind];
            --count;
            break;

        case CT_EVAL:
            contour_vars.gradient_mag[ind] = sum / (double)count;
            if (contour_vars.gradient_mag[ind] > max_mag)
                max_mag = contour_vars.gradient_mag[ind];
            break;

        case CT_END:
            if (contour_vars.start_index != -1)
            {
                ind = contour_vars.start_index - 1;
                do
                {
                    if (++ind >= n)
                        ind = 1;
                    contour_vars.gradient_mag[ind] /= max_mag;
                }
                while (ind != contour_vars.end_index);
            }
            free (magnitude);
            break;
    }
}

#undef Z



/*------------------------------------------------------------------------------
/ This routine calculates the variance of the points from a straight line.  The
/ idea behind this routine is that it is preferable to put a label on a straight
/ section than on a sharp bend.
/-----------------------------------------------------------------------------*/

static void variance (int ind, int n, float *xpts, float *ypts,
    enum contour_op op)
{
    double              y;
    double              Sxx, Syy, Sxy;
    static double       sigma_x, sigma_y, sigma_x2, sigma_y2, sigma_xy;
    static double       max_var;
    static int          count;

    switch (op)
    {
        case CT_INIT:
            sigma_x = sigma_y = sigma_x2 = sigma_y2 = sigma_xy = 0.0;
            max_var = 0.0;
            count = 0;
            break;

        case CT_ADD:
            y = (double)ypts[ind];
            sigma_x += (double)xpts[ind];
            sigma_y += y;
            sigma_x2 += (double) xpts[ind] * (double) xpts[ind];
            sigma_y2 += y*y;
            sigma_xy += (double) xpts[ind] * y;
            ++count;
            break;

        case CT_SUB:
            y = (double)ypts[ind];
            sigma_x -= (double) xpts[ind];
            sigma_y -= y;
            sigma_x2 -= (double) xpts[ind] * (double) xpts[ind];
            sigma_y2 -= y*y;
            sigma_xy -= (double) xpts[ind] * y;
            --count;
            break;

        case CT_EVAL:
            Sxx = sigma_x2 - sigma_x*sigma_x/(double)count;
            Syy = sigma_y2 - sigma_y*sigma_y/(double)count;
            Sxy = sigma_xy - sigma_x*sigma_y/(double)count;
            if (Sxx >= Syy)
                contour_vars.variance_list[ind] = 
                        (Syy - Sxy*Sxy/Sxx) / (double) count;
            else
                contour_vars.variance_list[ind] = 
                        (Sxx - Sxy*Sxy/Syy) / (double) count;
            if (contour_vars.variance_list[ind] > max_var)
                max_var = contour_vars.variance_list[ind];
            break;

        case CT_END:
            if (contour_vars.start_index != -1)
            {
                ind = contour_vars.start_index - 1;
                do
                {
                    if (++ind >= n)
                        ind = 1;
                    contour_vars.variance_list[ind] /= max_var;
                }
                while (ind != contour_vars.end_index);
            }
            break;
    }
}



static int find_good_place (int n, float *xpts, float *ypts, double r_sqr)
{
    int         i, i_ind;
    int         j, j_ind;
    int         k;
    int         closed;
    int         map_xpos, map_ypos;
    double      dx, dy, dx1, dy1;
    double      dist, dist1;
    double      min_t, t, min_var, var;
    unsigned short      *ind;
    float       r;

    contour_vars.gradient_mag = (double *)gmalloc (n*sizeof (double));
    contour_vars.variance_list = (double *)gmalloc (n*sizeof (double));
    ind = (unsigned short *)gmalloc (n*sizeof (short));

    contour_vars.start_index = -1;
    i = i_ind = 0;
    j = j_ind = 0;
    k = 0;
    ind[0] = 0xffff;
    closed = ((xpts[0] == xpts[n-1]) && (ypts[0] == ypts[n-1]));

    gradient (0, n, xpts, ypts, CT_INIT);
    variance (0, n, xpts, ypts, CT_INIT);
    gradient (0, n, xpts, ypts, CT_ADD);
    variance (0, n, xpts, ypts, CT_ADD);

    /*--------------------------------------------------------------------------
    / For each point on the line, find the variance and the average magnitude of
    / the gradient vector.
    /-------------------------------------------------------------------------*/
    while (!closed && i < n-1  ||  closed && j_ind != contour_vars.start_index)
    {
        dx = (double)xpts[i_ind] - (double)xpts[j_ind];
        dy = (double)ypts[i_ind] - (double)ypts[j_ind];
        dist = dx*dx + dy*dy;
        while (dist < r_sqr)
        {
            ++i;
            if (++i_ind >= n)
            {
                if (!closed || k == 0)
                    goto avg_done; /* Not enough points for moving average */
                i_ind = 1;
            }
            if (i_ind == k)
                goto avg_done;
            gradient (i_ind, n, xpts, ypts, CT_ADD);
            variance (i_ind, n, xpts, ypts, CT_ADD);
            dx = (double)xpts[i_ind] - (double)xpts[j_ind];
            dy = (double)ypts[i_ind] - (double)ypts[j_ind];
            dist = dx*dx + dy*dy;
        }

        ind[j_ind] = i;

        while (ind[k] < j)
        {
            gradient (k, n, xpts, ypts, CT_SUB);
            variance (k, n, xpts, ypts, CT_SUB);
            if (++k >= n)
                k = 1;
        }

        if (j >= ind[0])
        {
            if (contour_vars.start_index == -1)
                contour_vars.start_index = j_ind;
            contour_vars.end_index = j_ind;
            gradient (j_ind, n, xpts, ypts, CT_EVAL);
            variance (j_ind, n, xpts, ypts, CT_EVAL);
        }

        ++j;
        if (++j_ind >= n)
            j_ind = 1;
    }

avg_done:
    gradient (0, n, xpts, ypts, CT_END);
    variance (0, n, xpts, ypts, CT_END);

    /*--------------------------------------------------------------------------
    / Find a point where to place the the label by minimizing variance
    / and gradient. (Variance is more important.)
    / The text must be neither outside the current viewport nor too close to
    / previous written labels.
    / Places in the middle of a line a preferred to those at a border.
    /-------------------------------------------------------------------------*/

    if ((k = contour_vars.start_index) != -1)
    {
        k = -1;
        r = (float) sqrt (r_sqr);
        min_t = max_real;
        min_var = max_real;
        i = contour_vars.start_index - 1;
        do
        {
            if (++i >= n)
                i = 1;
            t = contour_vars.gradient_mag[i] +
                contour_vars.variance_list[i] +
                fabs (((double)i / (double)n) - 0.5);
            var = contour_vars.variance_list[i];
            map_xpos = (int) (xpts[i] * contour_vars.x_map_factor) + 1;
            map_ypos = (int) (ypts[i] * contour_vars.y_map_factor) + 1;
            dx = (double)xpts[i] - (double)xpts[0];
            dy = ((double)ypts[i] - (double)ypts[0]) * 
                                 (double)contour_vars.aspect_ratio;
            dist = dx*dx + dy*dy;
            dx1 = (double)xpts[i] - (double)xpts[n-1];
            dy1 = ((double)ypts[i] - (double)ypts[n-1]) * 
                                 (double)contour_vars.aspect_ratio;
            dist1 = dx1*dx1 + dy1*dy1;
            if ((t < 1.1 * min_t) &&
                (var < 1.1 * min_var) &&
                (xpts[i] - r > contour_vars.wn[0]) && 
                (xpts[i] + r < contour_vars.wn[1]) &&
                (ypts[i] - r > contour_vars.wn[2]) && 
                (ypts[i] + r < contour_vars.wn[3]) &&
                (dist > r_sqr) && (dist1 > r_sqr) &&
                (contour_vars.label_map [map_xpos * contour_vars.x_map_size 
                                         + map_ypos] == 0))
            {
                min_t = t;
                min_var = var;
                k = i;
            }
        }
        while (i != contour_vars.end_index);
    }

    if (k >= 0)
    {
       map_xpos = (int) (xpts[k] * contour_vars.x_map_factor) + 1;
       map_ypos = (int) (ypts[k] * contour_vars.y_map_factor) + 1;

       contour_vars.label_map 
           [(map_xpos-1) * contour_vars.x_map_size + map_ypos-1] = 1;
       contour_vars.label_map 
           [(map_xpos-1) * contour_vars.x_map_size + map_ypos] = 1;
       contour_vars.label_map 
           [(map_xpos-1) * contour_vars.x_map_size + map_ypos+1] = 1;
       contour_vars.label_map 
           [map_xpos * contour_vars.x_map_size + map_ypos-1] = 1;
       contour_vars.label_map 
           [map_xpos * contour_vars.x_map_size + map_ypos] = 1;
       contour_vars.label_map 
           [map_xpos * contour_vars.x_map_size + map_ypos+1] = 1;
       contour_vars.label_map 
           [(map_xpos+1) * contour_vars.x_map_size + map_ypos-1] = 1;
       contour_vars.label_map 
           [(map_xpos+1) * contour_vars.x_map_size + map_ypos] = 1;
       contour_vars.label_map 
           [(map_xpos+1) * contour_vars.x_map_size + map_ypos+1] = 1;
    }

    free (ind);
    free (contour_vars.gradient_mag);
    free (contour_vars.variance_list);

    return (k);
}



static void label_line (int n, float *xpts, float *ypts, float *zpts,
    char *label)
{
    int         i, j, k;
    int         error_ind;
    int         n_pts;
    int         label_length;
    double      dist;
    double      r_sqr;
    double      a, b, c;
    double      ox, oy;
    double      dx, dy, t;
    double      xtpt1, ytpt1, xtpt2, ytpt2;
    float       cpx, cpy, tx[4], ty[4];
    float       x_up_val, y_up_val;
    float       x_text_pos, y_text_pos;
    float       d, e;
    
    /*--------------------------------------------------------------------------
    / Find out how large the label is so we will know how much room to leave
    / for it.
    /-------------------------------------------------------------------------*/

    x_up_val = 0.0;
    y_up_val = 1.0;
    GSCHUP (&x_up_val, &y_up_val);

    d = 0.0;
    e = 0.0;
    label_length = strlen (label);

    GSELNT (&contour_vars.ndc);
    gks_inq_text_extent_s (&contour_vars.wkid, &d, &e, &label_length,
          label, &error_ind, &cpx, &cpy, tx, ty);
    GSELNT (&contour_vars.tnr);

    a = ((double)tx[2] - (double)tx[0]) / (double)contour_vars.scale_factor;
    b = ((double)ty[2] - (double)ty[0]) / (double)contour_vars.scale_factor;
    r_sqr = (a*a + b*b) / 4.0; /* Gap in line reduced to half size */

    /*--------------------------------------------------------------------------
    / Try to find a good place to put the label on the contour line.
    /-------------------------------------------------------------------------*/

    k = find_good_place (n, xpts, ypts, r_sqr);

    if (k != -1)
    {
        /*----------------------------------------------------------------------
        / Find the first point outside of the circle centered at 'pts[k]'
        /---------------------------------------------------------------------*/

        i = k;
        do
        {
            if (--i < 0)
            {
                i = n-2;
            }
            dx = (double)xpts[i] - (double)xpts[k];
            dy = ((double)ypts[i] - (double)ypts[k]) * 
                                  (double)contour_vars.aspect_ratio;
            dist = dx*dx + dy*dy;
        }
        while (dist < r_sqr);

        /*----------------------------------------------------------------------
        / Find the intersection of line segment p[i]--p[i+1] with the circle
        /---------------------------------------------------------------------*/

        dx = (double)xpts[i+1] - (double)xpts[i];
        dy = ((double)ypts[i+1] - (double)ypts[i]) * 
                            (double)contour_vars.aspect_ratio;
        ox = (double)xpts[i] - (double)xpts[k];
        oy = ((double)ypts[i] - (double)ypts[k]) * 
                            (double)contour_vars.aspect_ratio;
        a = dx*dx + dy*dy;
        b = ox*dx + oy*dy;
        c = ox*ox + oy*oy - r_sqr;
        t = -(b + sqrt (b*b - a*c))/a;
        xtpt1 = (double)xpts[i] + t*dx;
        ytpt1 = (double)ypts[i] + t * ((double)ypts[i+1] - (double)ypts[i]);

        /*----------------------------------------------------------------------
        / Same as above but in the other direction
        /---------------------------------------------------------------------*/

        j = k;
        do
        {
            if (++j >= n)
            {
                j = 1;
            }
            dx = (double)xpts[j] - (double)xpts[k];
            dy = ((double)ypts[j] - (double)ypts[k]) * 
                              (double)contour_vars.aspect_ratio;
            dist = dx*dx + dy*dy;
        }
        while (dist < r_sqr);

        /*----------------------------------------------------------------------
        / Find the intersection of line segment p[j]--p[j-1] with the circle
        /---------------------------------------------------------------------*/

        dx = (double)xpts[j-1] - (double)xpts[j];
        dy = ((double)ypts[j-1] - (double)ypts[j]) * 
                          (double)contour_vars.aspect_ratio;
        ox = (double)xpts[j] - (double)xpts[k];
        oy = ((double)ypts[j] - (double)ypts[k]) * 
                           (double)contour_vars.aspect_ratio;
        a = dx*dx + dy*dy;
        b = ox*dx + oy*dy;
        c = ox*ox + oy*oy - r_sqr;
        t = -(b + sqrt (b*b - a*c))/a;
        xtpt2 = (double)xpts[j] + t * dx;
        ytpt2 = (double)ypts[j] + t * ((double)ypts[j-1] - (double)ypts[j]);

        /*----------------------------------------------------------------------
        / Calculate the character up vector.
        /---------------------------------------------------------------------*/

        x_up_val = (float) ((ytpt1 - ytpt2) * contour_vars.aspect_ratio);
        y_up_val = (float) (xtpt2 - xtpt1);
        if (y_up_val < 0.0)
        {
            x_up_val = -x_up_val;
            y_up_val = -y_up_val;
        }
        GSCHUP (&x_up_val, &y_up_val);

        x_text_pos = (xpts[k]-contour_vars.wn[0]) * contour_vars.scale_factor
                     + contour_vars.vp[0];
        y_text_pos = (ypts[k]-contour_vars.wn[2]) * contour_vars.scale_factor
                     * contour_vars.aspect_ratio + contour_vars.vp[2];

        GSELNT (&contour_vars.ndc);
        gks_text_s (&x_text_pos, &y_text_pos, &label_length, label);
        GSELNT (&contour_vars.tnr);

        /*---------------------------------------------------------------------/
        / Draw the contour line leaving a gap for the text.
        /---------------------------------------------------------------------*/

        if (i >= j)
        {
            xpts[i+1] = (float)xtpt1;
            ypts[i+1] = (float)ytpt1;
            xpts[j-1] = (float)xtpt2;
            ypts[j-1] = (float)ytpt2;
            /* zpts remain the same */
            n_pts = i - j + 3;
            gus_curve (&n_pts, &(xpts[j-1]), &(ypts[j-1]), &(zpts[j-1]),
                &primitive, NIL);
        }
        else
        {
            xpts[i+1] = (float)xtpt1;
            ypts[i+1] = (float)ytpt1;
            n_pts = i + 2;
            gus_curve (&n_pts, xpts, ypts, zpts, &primitive, NIL);
            xpts[j-1] = (float)xtpt2;
            ypts[j-1] = (float)ytpt2;
            n_pts = n - j + 1;
            gus_curve (&n_pts, &(xpts[j-1]), &(ypts[j-1]), &(zpts[j-1]),
                &primitive, NIL);
        }
    }
    else
    {
        gus_curve (&n, xpts, ypts, zpts, &primitive, NIL);
    }
}



static void DRAW (float *x, float *y, float *z, int *iflag)
{
    static int          n;
    static float        xpts[contour_max_pts];
    static float        ypts[contour_max_pts];
    static float        zpts[contour_max_pts];
    static double       line_length;
    static int          z_exept_flag = 0;
    double              dx, dy;
    int                 linetype;
    char                label[20];

    switch (*iflag % 10)
    {
        case 1:         /* Continue polyline */
          if (z_exept_flag == 0)
          {  
            xpts[n] = *x;
            ypts[n] = *y;
            zpts[n] = *z;
            if ((contour_vars.txtflg == 1) && 
                ((contour_vars.lblmjh == 1) || 
                 (((*iflag/10-1) % contour_vars.lblmjh) == 1)))
             { 
               dx = (double) (xpts[n] - xpts[n-1]);
               dy = (double) (ypts[n] - ypts[n-1]) * contour_vars.aspect_ratio;
               line_length = line_length + contour_vars.scale_factor * 
                                           sqrt (dx*dx + dy*dy);
             }
            n++;

            if ((line_length >= contour_max_length) ||
                (n >= contour_max_pts))
            {
                if ((contour_vars.txtflg == 1) && 
                    ((contour_vars.lblmjh == 1) || 
                     (((*iflag/10-1) % contour_vars.lblmjh) == 1)))
                {
		    linetype = GLSOLI;
                    if (contour_vars.lblmjh > 1)
		    {
                    	GSLN (&linetype);
		    }
		    sprintf (label, contour_vars.lblfmt, *z);
                    label_line (n, xpts, ypts, zpts, label);
                }
                else
                {
                    if ((contour_vars.lblmjh <= 1) || 
                        (((*iflag/10-1) % contour_vars.lblmjh) == 1))
                    {
                       linetype = GLSOLI;
                    }
                    else
                    {
                       linetype = GLDOT;
                    }
                    if (contour_vars.lblmjh > 1)
		    {
		    	GSLN (&linetype);
		    }
		    gus_curve (&n, xpts, ypts, zpts, &primitive, NIL);
                }
                xpts[0] = *x;
                ypts[0] = *y;
                zpts[0] = *z;
                line_length = 0.0;
                n = 1;
            }
          }
          break;

        case 2:         /* New polyline */
        case 3:
          if ((*z > contour_vars.zmin) && (*z < contour_vars.zmax))
          {
            z_exept_flag = 0;
            xpts[0] = *x;
            ypts[0] = *y;
            zpts[0] = *z;
            line_length = 0.0;
            n = 1;
          }
          else
            z_exept_flag = 1;
          break;

        case 4:         /* End polyline */
        case 5:
          if (z_exept_flag == 0)
          {
            xpts[n] = *x;
            ypts[n] = *y;
            zpts[n] = *z;
            if ((contour_vars.txtflg == 1) && 
                ((contour_vars.lblmjh == 1) || 
                 (((*iflag/10-1) % contour_vars.lblmjh) == 1)))
             {
               dx = (double) (xpts[n] - xpts[n-1]);
               dy = (double) (ypts[n] - ypts[n-1]) * contour_vars.aspect_ratio;
               line_length = line_length + contour_vars.scale_factor * 
                                           sqrt (dx*dx + dy*dy);
             }
            n++;

            if ((line_length >= contour_min_length) &&
                (contour_vars.txtflg == 1) && 
                ((contour_vars.lblmjh == 1) || 
                 (((*iflag/10-1) % contour_vars.lblmjh) == 1)))
            {
                linetype = GLSOLI;
		if (contour_vars.lblmjh > 1)
		{
		    GSLN (&linetype);
                }
		sprintf (label, contour_vars.lblfmt, *z);
                label_line (n, xpts, ypts, zpts, label);
            }
            else
            {
                if ((contour_vars.lblmjh <= 1) || 
                    (((*iflag/10-1) % contour_vars.lblmjh) == 1))
                {
                   linetype = GLSOLI;
                }
                else
                {
                   linetype = GLDOT;
                }
		if (contour_vars.lblmjh > 1)
		{
		    GSLN (&linetype);
                }
                gus_curve (&n, xpts, ypts, zpts, &primitive, NIL);
            }
          }
          break;
    }
}



static float x_lin (float x, int *status)

/*
 * x_lin - return linearized value of x
 *
 */

{
    int stat;
    float result;

    stat = gus__normal;

    if (option_x_log IN lx.scale_options)
	{
	if (x > 0)
	    result = lx.a * log10(x) + lx.b;
	else
	    stat = gus__invpnt;  /* point co-ordinates must be greater than 0 */
	}
    else
	result = x;

    if (option_flip_x IN lx.scale_options)
	{
	result = lx.x_max - result + lx.x_min;
	}

    if (present(status))
        *status = stat;
    else
        if (!odd(stat))
            gus_signal (stat, "X_LIN");

    return (result);
}



static float y_lin (float y, int *status)

/*
 * y_lin - return linearized value of y
 *
 */

{
    int stat;
    float result;

    stat = gus__normal;

    if (option_y_log IN lx.scale_options)
	{
	if (y > 0)
	    result = lx.c * log10(y) + lx.d;
	else
	    stat = gus__invpnt;  /* point co-ordinates must be greater than 0 */
	}
    else
	result = y;

    if (option_flip_y IN lx.scale_options)
	{
	result = lx.y_max - result + lx.y_min;
	}

    if (present(status))
        *status = stat;
    else
        if (!odd(stat))
            gus_signal (stat, "Y_LIN");

    return (result);
}



static float z_lin (float z, int *status)

/*
 * z_lin - return linearized value of z
 *
 */

{
    int stat;
    float result;

    stat = gus__normal;

    if (option_z_log IN lx.scale_options)
	{
	if (z > 0)
	    result = lx.e * log10(z) + lx.f;
	else
	    stat = gus__invpnt;  /* point co-ordinates must be greater than 0 */
	}
    else
	result = z;

    if (option_flip_z IN lx.scale_options)
	{
	result = lx.z_max - result + lx.z_min;
	}

    if (present(status))
        *status = stat;
    else
        if (!odd(stat))
            gus_signal (stat, "Z_LIN");

    return (result);
}



static float x_log (float x)

/*
 * x_log - return logarithmic value of x
 *
 */

{
    if (option_flip_x IN lx.scale_options)
	x = lx.x_max - x + lx.x_min;

    if (option_x_log IN lx.scale_options)
	return (pow(10.0, (double)((x - lx.b) / lx.a)));
    else
	return (x);
}



static float y_log (float y)

/*
 * y_log - return logarithmic value of y
 */

{
    if (option_flip_y IN lx.scale_options)
	y = lx.y_max - y + lx.y_min;

    if (option_y_log IN lx.scale_options)
	return (pow(10.0, (double)((y - lx.d) / lx.c)));
    else
	return (y);
}



static float z_log (float z)

/*
 * z_log - return logarithmic value of z
 */

{
    if (option_flip_z IN lx.scale_options)
	z = lx.z_max - z + lx.z_min;

    if (option_z_log IN lx.scale_options)
	return (pow(10.0, (double)((z - lx.f) / lx.e)));
    else
	return (z);
}



static void minmax (float *a, int n, float *amin, float *amax)

/*
 * minmax - return minimum and maximum values
 */

{
    int index;

    *amin = a[0];
    *amax = *amin;

    for (index = 0; index <= n; index++)
        {
        *amin = min(a[index], *amin);
        *amax = max(a[index], *amax);
        }
}



static float fract (float x)

/*
 * fract - return fract(x)
 */

{
    return (x - (int) x);
}



static void apply_world_xform (float *x, float *y, float *z)

/*
 * apply_world_xform - apply world transformation
 */

{
    float xw, yw;

    xw = wx.a1 * *x + wx.a2 * *y + wx.b;
    yw = wx.c1 * *x + wx.c2 * *y + wx.c3 * *z + wx.d;
    *x = xw;
    *y = yw;
}



void gus_apply_world_xform (float *x, float *y, float *z)

/*
 * apply_world_xform - apply world transformation
 */

{
    apply_world_xform (x, y, z);
}



static void end_pline (void)

/*
 * end_pline - end polyline sequence
 */

{
    if (n_points >= 2)
        GPL (&n_points, px, py);
}



static void pline (float *x, float *y, float *z, int *status)

/*
 * pline - continue polyline sequence
 */

{
    int stat, i;

    if (n_points == max_prim_points)
        {
        end_pline ();

        px[0] = px[n_points-1];
        py[0] = py[n_points-1];
        pz[0] = pz[n_points-1];
        n_points = 1;
        }

    i = n_points;

    px[i] = x_lin(*x, &stat);

    if (odd(stat))
        {
        py[i] = y_lin(*y, &stat);

        if (present(z) && odd(stat))
            {
            pz[i] = z_lin(*z, &stat);

            if (odd(stat))
		{
                apply_world_xform (&px[i], &py[i], &pz[i]);
		}
            }
        }

    if (odd(stat))
        n_points = i + 1;

    if (present(status))
        *status = stat;
}



static void start_pline (float *x, float *y, float *z, int *status)

/*
 * start_pline - start polyline sequence
 */

{
    n_points = 0;
    pline (x, y, z, status);
}



static void grid_line (float *x0, float *y0, float *x1, float *y1, float *tick)

/*
 * grid_line - draw a grid line
 */

{
    if (*tick < 0)
        {
	GSLN (&GLSOLI);
	}
    else
        GSLN (&GLDOT);

    start_pline (x0, y0, NIL, NIL);
    pline (x1, y1, NIL, NIL);
    end_pline ();
}



static void end_pmark (void)

/*
 * end_pmark - end polymarker sequence
 */

{
    if (n_points >= 1)
        GPM (&n_points, px, py);
}



static void pmark (float *x, float *y, float *z, int *status)

/*
 * pmark - continue polymarker sequence
 */

{
    int stat, i;

    if (n_points == max_prim_points)
        {
        end_pmark ();

        px[0] = px[n_points-1];
        py[0] = py[n_points-1];
        pz[0] = pz[n_points-1];
        n_points = 1;
        }

    i = n_points;

    px[i] = x_lin(*x, &stat);

    if (odd(stat))
        {
        py[i] = y_lin(*y, &stat);

        if (present(z) && odd(stat))
            {
            pz[i] = z_lin(*z, &stat);

            if (odd(stat))
		{
                apply_world_xform (&px[i], &py[i], &pz[i]);
		}
            }
        }

    if (odd(stat))
        n_points = i + 1;

    if (present(status))
        *status = stat;
}



static void start_pmark (float *x, float *y, float *z, int *status)

/*
 * start_pmark - start polymarker sequence
 */

{
    n_points = 0;
    pmark (x, y, z, status);
}



static void write_log (float *x, float *y, int n)

/*
 * log - write data to log file
 */

{
    FILE *log_file;
    int index;

    log_file = fopen ("gus.log", "w");

    for (index = 0; index < n; index++)
        fprintf (log_file, "%e %e\n", x[index], y[index]);

    fclose (log_file);
}



int gus_apply_xform (int *tnr, float *x, float *y)

/*
 * gus_apply_xform - apply normalization transformation
 */

{
    int stat;

    gus_set_scale (&lx.scale_options, &stat);

    if (odd(stat))
	{
	if (*tnr != 0)
	    {
	    *x = x_lin(*x, &stat);
	    if (odd(stat))
		{
		*x = nx.a * *x + nx.b;

		*y = y_lin(*y, &stat);
		if (odd(stat))
		    *y = nx.c * *y + nx.d;     
		}
	    }
	}

    return (stat);
}



int gus_apply_inverse_xform (float *x, float *y)

/*
 * gus_apply_inverse_xform - apply denormalization transformation
 */

{
    int stat;

    gus_set_scale (&lx.scale_options, &stat);

    if (odd(stat))
	{
	*x = x_log((*x - nx.b) / nx.a);
	*y = y_log((*y - nx.d) / nx.c);
	}

    return (stat);
}


    
int gus_text (float *px, float *py, char *chars, int *status)

/*
 * FUNCTIONAL DESCRIPTION:
 *
 *	Output text.
 *
 * FORMAL ARGUMENT(S):
 *
 *	PX,PY		Text position in world co-ordinates
 *	CHARS		String of characters
 *	STATUS		Optional status value
 *
 * IMPLICIT INPUTS:
 *
 *	NONE
 *
 * IMPLICIT OUTPUTS:
 *
 *	NONE
 *
 * COMPLETION CODES:
 *
 *	GUS__NORNAL	normal successful completion
 *	GUS__NOTACT	GKS not in proper state. GKS must be either in the
 *			state WSAC or SGOP
 *	GUS__INVWINLIM	cannot apply logarithmic transformation to current
 *			window
 *	GUS__INVPNT	point co-ordinates must be greater than 0
 *
 * SIDE EFFECTS:
 *
 *	NONE
 *
 */

{
    int stat, opsta, errind, tnr;
    float x, y;

    int t_nr;

    /* inquire and test GKS operating state */

    GQOPS (&opsta);

    if (opsta >= GWSAC)
        {
        gus_set_scale (&lx.scale_options, &stat);

        if (odd(stat))
            {
            x = x_lin(*px, &stat);

            if (odd(stat))
                {
                y = y_lin(*py, &stat);

                if (odd(stat))
                    {
                    GQCNTN (&errind, &tnr);

                    if (tnr != gks_ndc)
                        {
			x = nx.a * x + nx.b;
                        y = nx.c * y + nx.d;

			t_nr = gks_ndc;
                        GSELNT (&t_nr);
                        }

                    gus_text_routine (&x, &y, chars, &stat);

                    if (tnr != gks_ndc)
                        GSELNT (&tnr);
                    }
                }
            }
        }
    else
        /* GKS not in proper state. GKS must be either in the state WSAC 
           or SGOP */

        stat = gus__notact;

    if (present(status))
        *status = stat;
    else
        if (!odd(stat))
            gus_signal (stat, "TEXT");

    return (stat);
}



int gus_axes (float *x_tick, float *y_tick, float *x_org, float *y_org,
    int *major_x, int *major_y, float *t_size, int *status)

/*
 * FUNCTIONAL DESCRIPTION:
 *
 *	Draw a pair of co-ordinate axes, with linearly or logarithmically
 *	spaced tick-marks.
 *
 * FORMAL ARGUMENT(S):
 *
 *	X_TICK, Y_TICK	Length of the interval between tick-marks
 *			 (0=no axis)
 *	X_ORG, Y_ORG	Co-ordinates of the origin (point of intersection)
 *			of the two axes
 *	MAJOR_X,
 *	MAJOR_Y		Number of minor tick intervals between major
 *			tick-marks
 *			 (0=no labels, 1=no minor tick-marks)
 *	T_SIZE		Length of the minor tick-marks
 *	STATUS		Optional status value
 *
 * IMPLICIT INPUTS:
 *
 *	NONE
 *
 * IMPLICIT OUTPUTS:
 *
 *	NONE
 *
 * COMPLETION CODES:
 *
 *	GUS__NORNAL	normal successful completion
 *	GUS__NOTACT	GKS not in proper state. GKS must be either in the
 *			state WSAC or SGOP
 *	GUS__INVWINLIM	cannot apply logarithmic transformation to current
 *			window
 *	GUS__INVINTLEN	invalid interval length for major tick-marks
 *	GUS__INVNUMTIC	invalid number for minor tick intervals
 *	GUS__INVTICSIZ	invalid tick-size
 *	GUS__ORGOUTWIN	origin outside current window
 *
 * SIDE EFFECTS:
 *
 *	NONE
 *
 */

{
    int stat, opsta, errind, tnr;
    int ltype, halign, valign, clsw;
    float chux, chuy, ux, uy;

    float clrt[4], wn[4], vp[4];
    float x_min, x_max, y_min, y_max;

    float tick, minor_tick, major_tick, x_label, y_label, x0, y0, xi, yi;

    int decade, exponent, i;

    char text[256], expn[256];


    if ((*x_tick < 0) || (*y_tick < 0))
        stat = gus__invintlen;  /* invalid interval length for major 
                                   tick-marks */
    else
        if (*t_size == 0)
            stat = gus__invticsiz;  /* invalid tick-size */
        else
            stat = gus__normal;

    if (odd(stat))
        {

        /* inquire and test GKS operating state */

        GQOPS (&opsta);

        if (opsta >= GWSAC)
            {
            gus_set_scale (&lx.scale_options, &stat);

            if (odd (stat))
                {

                /* inquire current normalization transformation */

                GQCNTN (&errind, &tnr);
                GQNT (&tnr, &errind, wn, vp);

                x_min = wn[0];
                x_max = wn[1];
                y_min = wn[2];
                y_max = wn[3];

                if ((x_min <= *x_org) && (*x_org <= x_max) && 
		   (y_min <= *y_org) && (*y_org <= y_max))
                    {

                    /* save linetype, text alignment, character-up vector and
		       clipping indicator */

                    GQLN (&errind, &ltype);
                    GQTXAL (&errind, &halign, &valign);
                    GQCHUP (&errind, &chux, &chuy);
                    GQCLIP (&errind, &clsw, clrt);

                    GSLN (&GLSOLI);
		    ux = 0; uy = 1;
                    GSCHUP (&ux, &uy);
                    GSCLIP (&GNCLIP);

                    if (*y_tick != 0)
                        {
                        tick = *t_size * (x_max - x_min) / (vp[1] - vp[0]);

                        minor_tick = x_log(x_lin(*x_org, NIL) + tick);
                        major_tick = x_log(x_lin(*x_org, NIL) + 2. * tick);
                        x_label    = x_log(x_lin(*x_org, NIL) + 3. * tick);

                        /* set text alignment */

                        if (x_lin(*x_org, NIL) <= 
                            (x_lin(x_min, NIL) + x_lin(x_max, NIL)) / 2.)
                            {
                            GSTXAL (&GARITE, &GAHALF);

                            if (tick > 0)
                                x_label = x_log(x_lin(*x_org, NIL) - tick);
                            }
                        else
                            {
                            GSTXAL (&GALEFT, &GAHALF);

                            if (tick < 0)
                                x_label = x_log(x_lin(*x_org, NIL) - tick);
                            }

			if (option_y_log IN lx.scale_options)
			    {
			    y0 = pow(10.0, (double)gauss(log10(y_min)));

			    i = ipred ((float) (y_min / y0));
			    yi = y0 + i * y0;
			    decade = 0;

			    /* draw Y-axis */

			    start_pline (x_org, &y_min, NIL, NIL);

			    while ((yi <= y_max) && (!cli_b_abort))
				{
				pline (x_org, &yi, NIL, NIL);

				if (i == 0)
				    {
				    xi = major_tick;

				    if (*major_y > 0)
				      if (decade % *major_y == 0)
					if ((yi != *y_org) || (*y_org == y_min)
					 || (*y_org == y_max))
					    {
					    if (*y_tick > 1)
						{
						exponent = iround(log10(yi));

						strcpy (text, "10**{");
						str_dec (expn, exponent);
						strcat (text, expn);
						strcat (text, "}");

						gus_text (&x_label, &yi, text, 
						    NIL);
						}
					    else
						gus_text (&x_label, &yi,
						  str_ftoa(text, yi, 0.), NIL);
					    }
				    }
				else
				    xi = minor_tick;

				if (i == 0 || *major_y == 1)
				    {
				    pline (&xi, &yi, NIL, NIL);
				    pline (x_org, &yi, NIL, NIL);
				    }

				if (i == 9)
				    {
				    y0 = y0 * 10.;
				    i = 0;
				    decade++;
				    }
				else
				    i++;

				yi = y0 + i * y0;
				}

			    pline (x_org, &y_max, NIL, NIL);
			    end_pline ();
			    }
			else
			    {
			    i = isucc ((float) (y_min / *y_tick));
			    yi = i * *y_tick;

			    /* draw Y-axis */

			    start_pline (x_org, &y_min, NIL, NIL);

			    while ((yi <= y_max) && (!cli_b_abort))
				{
				pline (x_org, &yi, NIL, NIL);

				if (*major_y != 0)
				    {
				    if (i % *major_y == 0)
					{
					xi = major_tick;
					if ((yi != *y_org) || (*y_org == y_min)
					 || (*y_org == y_max))
					    if (*major_y > 0)
						gus_text (&x_label, &yi,
						    str_ftoa (text, yi, *y_tick
						    * *major_y), NIL);
					}
				    else
					xi = minor_tick;
				    }
				else
				    xi = major_tick;

				pline (&xi, &yi, NIL, NIL);
				pline (x_org, &yi, NIL, NIL);

				i++;
				yi = i * *y_tick;
				}

			    if (yi > y_max)
				pline (x_org, &y_max, NIL, NIL);

			    end_pline ();
			    }
                        }

                    if (*x_tick != 0)
                        {
                        tick = *t_size * (y_max - y_min) / (vp[3] - vp[2]);

                        minor_tick = y_log(y_lin(*y_org, NIL) + tick);
                        major_tick = y_log(y_lin(*y_org, NIL) + 2. * tick);
                        y_label    = y_log(y_lin(*y_org, NIL) + 3. * tick);

                        /* set text alignment */

                        if (y_lin(*y_org, NIL) <= (y_lin(y_min, NIL)
                            + y_lin(y_max, NIL)) / 2.)
                            {
                            GSTXAL (&GACENT, &GATOP);

                            if (tick > 0)
                                y_label = y_log(y_lin(*y_org, NIL) - tick);
                            }
                        else
                            {
                            GSTXAL (&GACENT, &GABOTT);

                            if (tick < 0)
                                y_label = y_log(y_lin(*y_org, NIL) - tick);
                            }

			if (option_x_log IN lx.scale_options)
			    {
			    x0 = pow(10.0, (double)gauss(log10(x_min)));

			    i = ipred ((float) (x_min / x0));
			    xi = x0 + i * x0;
			    decade = 0;

			    /* draw X-axis */

			    start_pline (&x_min, y_org, NIL, NIL);

			    while ((xi <= x_max) && (!cli_b_abort))
				{
				pline (&xi, y_org, NIL, NIL);

				if (i == 0)
				    {
				    yi = major_tick;

				    if (*major_x > 0)
				      if (decade % *major_x == 0)
					if ((xi != *x_org) || (*x_org == x_min)
					 || (*x_org == x_max))
					    {
					    if (*x_tick > 1)
						{
						exponent = iround(log10(xi));

						strcpy (text, "10**{");
						str_dec (expn, exponent);
						strcat (text, expn);
						strcat (text, "}");

						gus_text (&xi, &y_label, text, 
						    NIL);
						}
					    else
						gus_text (&xi, &y_label,
						   str_ftoa(text, xi, 0.), NIL);
					    }
				    }
				else
				    yi = minor_tick;

				if (i == 0 || *major_x == 1)
				    {
				    pline (&xi, &yi, NIL, NIL);
				    pline (&xi, y_org, NIL, NIL);
				    }

				if (i == 9)
				    {
				    x0 = x0 * 10.;
				    i = 0;
				    decade++;
				    }
				else
				    i++;

				xi = x0 + i * x0;
				}

			    pline (&x_max, y_org, NIL, NIL);
			    end_pline ();
			    }
			else
			    {
			    i = isucc ((float) (x_min / *x_tick));
			    xi = i * *x_tick;

			    /* draw X-axis */

			    start_pline (&x_min, y_org, NIL, NIL);

			    while ((xi <= x_max) && (!cli_b_abort))
				{
				pline (&xi, y_org, NIL, NIL);

				if (*major_x != 0)
				    {
				    if (i % *major_x == 0)
					{
					yi = major_tick;
					if ((xi != *x_org) || (*x_org == x_min)
					 || (*x_org == x_max))
					    if (*major_x > 0)
						gus_text (&xi, &y_label,
						    str_ftoa (text, xi, *x_tick
						    * *major_x), NIL);
					}
				    else
					yi = minor_tick;
				    }
				else
				    yi = major_tick;

				pline (&xi, &yi, NIL, NIL);
				pline (&xi, y_org, NIL, NIL);

				i++;
				xi = i * *x_tick;
				}

			    if (xi > x_max)
				pline (&x_max, y_org, NIL, NIL);

			    end_pline ();
			    }
                        }

                    /* restore linetype, text alignment, character-up vector
		       and clipping indicator */

                    GSLN (&ltype);
                    GSTXAL (&halign, &valign);
                    GSCHUP (&chux, &chuy);
                    GSCLIP (&clsw);

                    stat = gus__normal;
                    }
                else
                    stat = gus__orgoutwin;  /* origin outside current window */
                }
            }
        else
            /* GKS not in proper state. GKS must be either in the state WSAC 
               or SGOP */

            stat = gus__notact;
        }

    if (present(status))
        *status = stat;
    else
        if (!odd (stat))
            gus_signal (stat, "AXES");

    return (stat);
}



int gus_grid (float *x_tick, float *y_tick, float *x_org, float *y_org,
    int *major_x, int *major_y, int *status)

/*
 * FUNCTIONAL DESCRIPTION:
 *
 *	Draw a grid.
 *
 * FORMAL ARGUMENT(S):
 *
 *	X_TICK, Y_TICK	Length of the interval between grid lines
 *			 (0=no grid lines)
 *	X_ORG, Y_ORG	Co-ordinates of the origin of the grid
 *	MAJOR_X,
 *	MAJOR_Y		Number of dotted line intervals between solid
 *			grid lines (0=no dotted lines)
 *	STATUS		Optional status value
 *
 * IMPLICIT INPUTS:
 *
 *	NONE
 *
 * IMPLICIT OUTPUTS:
 *
 *	NONE
 *
 * COMPLETION CODES:
 *
 *	GUS__NORNAL	normal successful completion
 *	GUS__NOTACT	GKS not in proper state. GKS must be either in the
 *			state WSAC or SGOP
 *	GUS__INVWINLIM	cannot apply logarithmic transformation to current
 *			window
 *	GUS__INVINTLEN	invalid interval length for major tick-marks
 *	GUS__INVNUMTIC	invalid number for minor tick intervals
 *	GUS__ORGOUTWIN	origin outside current window
 *
 * SIDE EFFECTS:
 *
 *	NONE
 *
 */

{
    int stat, opsta, errind, tnr;
    int ltype, clsw;

    float clrt[4], wn[4], vp[4];
    float x_min, x_max, y_min, y_max;

    float x0, y0, xi, yi, tick;

    int i;

    if ((*x_tick < 0) || (*y_tick < 0))
        stat = gus__invintlen;  /* invalid interval length for major 
				   tick-marks */
    else
        if ((*major_x < 0) || (*major_y < 0))
            stat = gus__invnumtic;  /* invalid number for minor tick 
                                       intervals */
        else
            stat = gus__normal;

    if (odd(stat))
        {

        /* inquire and test GKS operating state */

        GQOPS (&opsta);

        if (opsta >= GWSAC)
            {
            gus_set_scale (&lx.scale_options, &stat);

            if (odd(stat))
                {

                /* inquire current normalization transformation */

                GQCNTN (&errind, &tnr);
                GQNT (&tnr, &errind, wn, vp);

                x_min = wn[0];
                x_max = wn[1];
                y_min = wn[2];
                y_max = wn[3];

                if ((x_min <= *x_org) && (*x_org <= x_max) && 
		    (y_min <= *y_org) && (*y_org <= y_max))
                    {

                    /* save linetype and clipping indicator */

                    GQLN (&errind, &ltype);
                    GQCLIP (&errind, &clsw, clrt);

                    GSLN (&GLSOLI);
                    GSCLIP (&GNCLIP);

                    if (y_tick != 0)
                        {
			if (option_y_log IN lx.scale_options)
			    {
			    y0 = pow(10.0, (double)gauss(log10(y_min)));

			    i = ipred ((float) (y_min / y0));
			    yi = y0 + i * y0;

			    /* draw horizontal grid lines */

			    while ((yi <= y_max) && (!cli_b_abort))
				{
				if (i == 0 || *major_y == 1)
				    {
				    if (i == 0)
					tick = -1.;
				    else
					tick = *x_tick;

				    grid_line (&x_min, &yi, &x_max, &yi, 
					&tick);
				    }

				if (i == 9)
				    {
				    y0 = y0 * 10.;
				    i = 0;
				    }
				else
				    i++;

				yi = y0 + i * y0;
				}
			    }
			else
			    {
			    i = isucc ((float) (y_min / *y_tick));
			    yi = i * *y_tick;

			    /* draw horizontal grid lines */

			    while ((yi <= y_max) && (!cli_b_abort))
				{
				if (*major_y > 0)
				    {
				    if (i % *major_y == 0)
					tick = -1.;
				    else
					tick = *y_tick;
				    }
				else
				    tick = -1.;

				grid_line (&x_min, &yi, &x_max, &yi, &tick);

				i++;
				yi = i * *y_tick;
				}
			    }
                        }

                    if (*x_tick != 0)
                        {
			if (option_x_log IN lx.scale_options)
			    {
			    x0 = pow(10.0, (double)gauss(log10(x_min)));

			    i = ipred ((float) (x_min / x0));
			    xi = x0 + i * x0;

			    /* draw vertical grid lines */

			    while ((xi <= x_max) && (!cli_b_abort))
				{
				if (i == 0 || *major_x == 1)
				    {
				    if (i == 0)
					tick = -1.;
				    else
					tick = *x_tick;

				    grid_line (&xi, &y_min, &xi, &y_max, 
					&tick);
				    }

				if (i == 9)
				    {
				    x0 = x0 * 10.;
				    i = 0;
				    }
				else
				    i++;

				xi = x0 + i * x0;
				}
			    }
			else
			    {
			    i = isucc ((float) (x_min / *x_tick));
			    xi = i * *x_tick;

			    /* draw vertical grid lines */

			    while ((xi <= x_max) && (!cli_b_abort))
				{
				if (*major_x > 0)
				    {
				    if (i % *major_x == 0)
					tick = -1.;
				    else
					tick = *x_tick;
				    }
				else
				    tick = -1.;

				grid_line (&xi, &y_min, &xi, &y_max, &tick);

				i++;
				xi = i * *x_tick;
				}
			    }
                        }

                    /* restore linetype and clipping indicator */

                    GSLN (&ltype);
                    GSCLIP (&clsw);

                    stat = gus__normal;
                    }
                }
            else
                stat = gus__orgoutwin;  /* origin outside current window */

            }
        else
            /* GKS not in proper state. GKS must be either in the state WSAC 
               or SGOP */

            stat = gus__notact;
        }

    if (present(status))
        *status = stat;
    else
        if (!odd (stat))
            gus_signal (stat, "GRID");

    return (stat);
}



int gus_spline (int *n, float *px, float *py, int *m, float *smoothing,
    int *status)

/*
 * FUNCTIONAL DESCRIPTION:
 *
 *	Fit a spline-curve through the given points.
 *
 * FORMAL ARGUMENT(S):
 *
 *	N		Number of points
 *	PX,PY		Co-ordinates of points in world co-ordinates
 *	M		Number of domain values
 *	SMOOTHING	Smoothing level
 *	STATUS		Optional status value
 *
 * IMPLICIT INPUTS:
 *
 *	NONE
 *
 * IMPLICIT OUTPUTS:
 *
 *	NONE
 *
 * COMPLETION CODES:
 *
 *	GUS__NORNAL	normal successful completion
 *	GUS__NOTACT	GKS not in proper state. GKS must be either in the
 *			state WSAC or SGOP
 *	GUS__INVWINLIM	cannot apply logarithmic transformation to current
 *			window
 *	GUS__INVNUMPNT	invalid number of points
 *	GUS__INVPNT	point co-ordinates must be greater than 0
 *	GUS__INVNUMDOM	invalid number of domain values
 *	GUS__NOTSORASC	points ordinates not sorted in ascending order
 *
 * SIDE EFFECTS:
 *
 *	NONE
 *
 */

{

    int stat, ret_stat, opsta;
    int i, j;

    float *s, *t;
    double *sx, *sy, *x, *f, *df, *y, *c, *wk, *se, var, d;
    int ic, job, ier;

    BOOL new_line;

    t   = (float *) gmalloc(sizeof(float) * *m);
    s   = (float *) gmalloc(sizeof(float) * *m);
    sx  = (double *) gmalloc(sizeof(double) * *m);
    sy  = (double *) gmalloc(sizeof(double) * *m);
    x  = (double *) gmalloc(sizeof(double) * *n);
    f  = (double *) gmalloc(sizeof(double) * *n);
    df  = (double *) gmalloc(sizeof(double) * *n);
    y  = (double *) gmalloc(sizeof(double) * *n);
    c  = (double *) gmalloc(sizeof(double) * 3 * (*n-1));
    se  = (double *) gmalloc(sizeof(double) * *n);
    wk  = (double *) gmalloc(sizeof(double) * 7 * (*n+2));
    
    if (*n > 2)
        {
        if (*m > *n)
            {

            /* inquire and test GKS operating state */

            GQOPS (&opsta);

            if (opsta >= GWSAC)
                {
                gus_set_scale (&lx.scale_options, &stat);

                if (odd(stat))
                    {
		    for (i = 0; i < *n; i++)
			{
			x[i] = (double) ((x_lin(px[i], NIL) - lx.x_min) /
			    (lx.x_max - lx.x_min));
			f[i] = (double) ((y_lin(py[i], NIL) - lx.y_min) /
			    (lx.y_max - lx.y_min));
			df[i] = 1;
			}

		    if (*smoothing >= -1)
			{
			for (i = 1; i < *n; i++)
			    if (px[i-1] >= px[i])
				stat = gus__notsorasc;  /* points not sorted in 
							   ascending order */

			if (odd(stat) && !cli_b_abort)
			    {
			    sx[0] = x[0];
 			    for (j = 1; j < *m-1; j++)
				sx[j] = x[0] + j * (x[*n-1] - x[0]) / (*m-1);
			    sx[*m-1] = x[*n-1];

			    job = 0;
			    ic = *n-1;
			    var = (double) *smoothing;

			    CUBGCV (x, f, df, n, y, c, &ic, &var, &job, se, wk,
				&ier);

			    if (ier == 0)
				{
				*smoothing = (float) var;

				for (j = 0; j < *m; j++)
				    {
				    i = 0;
				    while ((i < ic) && (x[i] <= sx[j]))
					i++;
				    if (x[i] > sx[j]) i--;
				    if (i < 0)
					i = 0;
				    else
					if (i >= ic)
					    i = ic-1;
				    d = sx[j] - x[i];

				    s[j] = (float) (((c[i+2*ic]*d + c[i+ic])*d +
					c[i])*d + y[i]);
				    }
				}
			    else
				stat = gus__invargmat;   /* invalid argument to
							    math library */
			    }
			}
		    else
			{
			mth_b_spline (*n, x, f, *m, sx, sy);

			for (j = 0; j < *m; j++)
			    s[j] = (float) sy[j];
			}
			
		    if (odd(stat) && !cli_b_abort)
			{
			for (j = 0; j < *m; j++)
			    {
			    t[j] = x_log((float) (lx.x_min + sx[j] *
				(lx.x_max - lx.x_min)));
			    s[j] = y_log((float) (lx.y_min + s[j] *
				(lx.y_max - lx.y_min)));
			    }

			if (logging_switch && !cli_b_abort)
			    write_log (t, s, *m);

			j = 0;
			new_line = TRUE;

			while (j < *m && !cli_b_abort)
			    {
			    if (new_line)
				start_pline (&t[j], &s[j], NIL, &ret_stat);
			    else
				pline (&t[j], &s[j], NIL, &ret_stat);

			    new_line = (!odd(ret_stat));

			    if (new_line)
				{
				end_pline ();
				stat = ret_stat;
				}
			    j++;
			    }

			end_pline ();			    
			}
		    }
                }
            else
                /* GKS not in proper state. GKS must be either in the state 
                   WSAC or SGOP */

                stat = gus__notact;
            }
        else
            stat = gus__invnumdom;  /* invalid number of domain values */
        }
    else
        stat = gus__invnumpnt;  /* invalid number of points */

    if (present(status))
        *status = stat;
    else
        if (!odd (stat))
            gus_signal (stat, "SPLINE");

    free (wk);
    free (se);
    free (c);
    free (y);
    free (df);
    free (f);
    free (x);
    free (sy);
    free (sx);
    free (s);
    free (t);

    return (stat);
}



int gus_linfit (int *n, float *px, float *py, int *status)

/*
 * FUNCTIONAL DESCRIPTION:
 *
 *	Draw a robust straight-line fit.
 *
 * FORMAL ARGUMENT(S):
 *
 *	N		Number of points
 *	PX,PY		Co-ordinates of points in world co-ordinates
 *	STATUS		Optional status value
 *
 * IMPLICIT INPUTS:
 *
 *	NONE
 *
 * IMPLICIT OUTPUTS:
 *
 *	NONE
 *
 * COMPLETION CODES:
 *
 *	GUS__NORNAL	normal successful completion
 *	GUS__NOTACT	GKS not in proper state. GKS must be either in the
 *			state WSAC or SGOP
 *	GUS__INVWINLIM	cannot apply logarithmic transformation to current
 *			window
 *	GUS__INVNUMPNT	invalid number of points
 *	GUS__INVPNT	point co-ordinates must be greater than 0
 *
 * SIDE EFFECTS:
 *
 *	NONE
 *
 */

{
    int stat, opsta, i;

    float *smy;

    float m, b, dev, x0, x1, xi, yi;

    smy = (float *) gmalloc(sizeof(float) * *n);

    if (*n > 1)
        {

        /* inquire and test GKS operating state */

        GQOPS (&opsta);

        if (opsta >= GWSAC)
            {
            gus_set_scale (&lx.scale_options, &stat);

            if (odd(stat))
                {
                if (gus_gl_smoothing_level != 0)
                    {
                    mth_smooth (*n, py, smy, gus_gl_smoothing_level);

                    py = smy;

                    if (logging_switch)
                        write_log (px, py, *n);
                    }

                mth_linfit (*n, px, py, &m, &b, &dev, logging_switch);

                gus_gf_linfit_m = m;
                gus_gf_linfit_b = b;
                gus_gf_linfit_dev = dev;

                minmax (px, *n - 1, &x0, &x1);

                for (i = 0; i < min_curve_points; i++)
                    {
                    xi = x0 + i * (x1 - x0) / (min_curve_points - 1);
                    yi = m * xi + b;

                    if (i == 0)
                        start_pline (&xi, &yi, NIL, NIL);
                    else
                        pline (&xi, &yi, NIL, NIL);

                    }

                end_pline ();
                }
            }
        else
            /* GKS not in proper state. GKS must be either in the state WSAC 
	       or SGOP */

            stat = gus__notact;
        }
    else
        stat = gus__invnumpnt;  /* invalid number of points */

    if (present(status))
        *status = stat;
    else
        if (!odd(stat))
            gus_signal (stat, "LINFIT");

    free (smy);

    return (stat);
}



int gus_linreg (int *n, float *px, float *py, int *status)

/*
 * FUNCTIONAL DESCRIPTION:
 *
 *	Draw a linear regression line.
 *
 * FORMAL ARGUMENT(S):
 *
 *	N		Number of points
 *	PX,PY		Co-ordinates of points in world co-ordinates
 *	STATUS		Optional status value
 *
 * IMPLICIT INPUTS:
 *
 *	NONE
 *
 * IMPLICIT OUTPUTS:
 *
 *	NONE
 *
 * COMPLETION CODES:
 *
 *	GUS__NORNAL	normal successful completion
 *	GUS__NOTACT	GKS not in proper state. GKS must be either in the
 *			state WSAC or SGOP
 *	GUS__INVWINLIM	cannot apply logarithmic transformation to current
 *			window
 *	GUS__INVNUMPNT	invalid number of points
 *	GUS__INVPNT	point co-ordinates must be greater than 0
 *
 * SIDE EFFECTS:
 *
 *	NONE
 *
 */

{
    int stat, opsta, i;

    float *smy;

    float m, b, x0, x1, xi, yi;

    smy = (float *) gmalloc(sizeof(float) * *n);

    if (*n > 1)
        {

        /* inquire and test GKS operating state */

        GQOPS (&opsta);

        if (opsta >= GWSAC)
            {
            gus_set_scale (&lx.scale_options, &stat);

            if (odd(stat))
                {
                if (gus_gl_smoothing_level != 0)
                    {
                    mth_smooth (*n, py, smy, gus_gl_smoothing_level);

                    py = smy;

                    if (logging_switch)
                        write_log (px, py, *n);
                    }

                mth_linreg (*n, px, py, &m, &b, logging_switch);

                gus_gf_linreg_m = m;
                gus_gf_linreg_b = b;

		minmax (px, *n - 1, &x0, &x1);

                for (i = 0; i < min_curve_points; i++)
                    {
                    xi = x0 + i * (x1 - x0) / (min_curve_points - 1);
                    yi = m * xi + b;

                    if (i == 0)
                        start_pline (&xi, &yi, NIL, NIL);
                    else
                        pline (&xi, &yi, NIL, NIL);
                    }

                end_pline ();
                }
            }
        else
            /* GKS not in proper state. GKS must be either in the state WSAC 
	       or SGOP */

            stat = gus__notact;
        }
    else
        stat = gus__invnumpnt;  /* invalid number of points */

    if (present(status))
        *status = stat;
    else
        if (!odd(stat))
            gus_signal (stat, "LINREG");

    free (smy);

    return (stat);
}



int gus_histogram (int *n, float *px, int *status)

/*
 * FUNCTIONAL DESCRIPTION:
 *
 *	Draw a histogram with up to forty cells.
 *
 * FORMAL ARGUMENT(S):
 *
 *	N		Number of points
 *	PX		Data set
 *	STATUS		Optional status value
 *
 * IMPLICIT INPUTS:
 *
 *	NONE
 *
 * IMPLICIT OUTPUTS:
 *
 *	NONE
 *
 * COMPLETION CODES:
 *
 *	GUS__NORNAL	normal successful completion
 *	GUS__NOTACT	GKS not in proper state. GKS must be either in the
 *			state WSAC or SGOP
 *	GUS__INVWINLIM	cannot apply logarithmic transformation to current
 *			window
 *	GUS__INVNUMPNT	invalid number of points
 *	GUS__INVPNT	point co-ordinates must be greater than 0
 *
 * SIDE EFFECTS:
 *
 *	NONE
 *
 */

{
    int stat, opsta, errind, clsw, tnr;
    float wn[4], vp[4], clrt[4];

    float ci, cmin, cmax, fi;

    int i, m;
    float f[40];

    float x[5], y[5];

    int np;

    if (*n > 1)
        {

        /* inquire and test GKS operating state */

        GQOPS (&opsta);

        if (opsta >= GWSAC)
            {
            gus_set_scale (&lx.scale_options, &stat);

            if (odd(stat))
                {
                GQCLIP (&errind, &clsw, clrt);
                GSCLIP (&GNCLIP);

                /* inquire current normalization transformation */

                GQCNTN (&errind, &tnr);
                GQNT (&tnr, &errind, wn, vp);

                minmax (px, *n - 1, &cmin, &cmax);

                mth_histogram (*n, px, &m, f, logging_switch);

                i = 0;
                x[2] = cmin;

                while ((i < m) && (!cli_b_abort))
                    {
                    i++;

                    ci = cmin + i * (cmax - cmin) / m;
                    fi = wn[2] + f[i-1] / 100. * (wn[3] - wn[2]);

                    x[0] = x[2];
                    x[1] = x[0];
                    x[2] = ci;
                    x[3] = ci;
                    x[4] = x[0];
                    y[0] = wn[2];
                    y[1] = fi;
                    y[2] = fi;
                    y[3] = y[0];
                    y[4] = y[0];

		    np = 4;
                    GFA (&np, x, y);

		    np = 5;
                    GPL (&np, x, y);

                    }

                GSCLIP (&clsw);

                }
            }
        else
            /* GKS not in proper state. GKS must be either in the state WSAC 
               or SGOP */

            stat = gus__notact;
        }
    else
        stat = gus__invnumpnt;  /* invalid number of points */

    if (present(status))
        *status = stat;
    else
        if (!odd(stat))
            gus_signal (stat, "HISTOGRAM");

    return (stat);
}



int gus_vertical_error_bars (int *n, float *px, float *py, float *e1, float *e2,
    int *status)

/*
 * FUNCTIONAL DESCRIPTION:
 *
 *	Draw vertical error bars.
 *
 * FORMAL ARGUMENT(S):
 *
 *	N		Number of points
 *	PX, PY		X, Y - coordinates
 *	E1              Negative error
 *      E2		Positive error
 *	STATUS		Optional status value
 *
 * IMPLICIT INPUTS:
 *
 *	NONE
 *
 * IMPLICIT OUTPUTS:
 *
 *	NONE
 *
 * COMPLETION CODES:
 *
 *	GUS__NORNAL	normal successful completion
 *	GUS__NOTACT	GKS not in proper state. GKS must be either in the
 *			state WSAC or SGOP
 *	GUS__INVWINLIM	cannot apply logarithmic transformation to current
 *			window
 *	GUS__INVNUMPNT	invalid number of points
 *	GUS__INVPNT	point co-ordinates must be greater than 0
 *
 * SIDE EFFECTS:
 *
 *	NONE
 *
 */

{
    int stat, opsta, errind;
    float tick, x, x1, x2, y1, y2, marker_size;
    int i;

    if (*n >= 1)
        {

        /* inquire and test GKS operating state */

        GQOPS (&opsta);

        if (opsta == GWSAC || opsta == GSGOP)
            {
            gus_set_scale (&lx.scale_options, &stat);

            if (odd(stat))
                {
		i = 0;
		GQMKSC (&errind, &marker_size);

                while (i < *n && !cli_b_abort && odd(stat))
		    {
		    tick = marker_size * error_tick_size * (lx.x_max - 
			lx.x_min);

		    x = px[i];
		    x1 = x_log(x_lin(x, NIL) - tick);
		    x2 = x_log(x_lin(x, NIL) + tick);
		    y1 = e1[i];
		    y2 = e2[i];

                    start_pline (&x1, &y1, NIL, &stat);
                    pline (&x2, &y1, NIL, &stat);
                    end_pline ();

		    start_pline (&x, &y1, NIL, &stat);
		    pline (&x, &y2, NIL, &stat);
		    end_pline ();

                    start_pline (&x1, &y2, NIL, &stat);
                    pline (&x2, &y2, NIL, &stat);
                    end_pline ();

		    i++;
		    }

		gus_polymarker (n, px, py, &stat);
		}
            }
        else
            /* GKS not in proper state. GKS must be either in the state WSAC 
               or SGOP */

            stat = gus__notact;
        }
    else
        stat = gus__invnumpnt;  /* invalid number of points */

    if (present(status))
        *status = stat;
    else
        if (!odd(stat))
            gus_signal (stat, "VERTICAL_ERROR_BARS");

    return (stat);
}



int gus_horizontal_error_bars (int *n, float *px, float *py,
    float *e1, float *e2, int *status)

/*
 * FUNCTIONAL DESCRIPTION:
 *
 *	Draw horizontal error bars.
 *
 * FORMAL ARGUMENT(S):
 *
 *	N		Number of points
 *	PX, PY		X, Y - coordinates
 *	E1              Negative error
 *      E2		Positive error
 *	STATUS		Optional status value
 *
 * IMPLICIT INPUTS:
 *
 *	NONE
 *
 * IMPLICIT OUTPUTS:
 *
 *	NONE
 *
 * COMPLETION CODES:
 *
 *	GUS__NORNAL	normal successful completion
 *	GUS__NOTACT	GKS not in proper state. GKS must be either in the
 *			state WSAC or SGOP
 *	GUS__INVWINLIM	cannot apply logarithmic transformation to current
 *			window
 *	GUS__INVNUMPNT	invalid number of points
 *	GUS__INVPNT	point co-ordinates must be greater than 0
 *
 * SIDE EFFECTS:
 *
 *	NONE
 *
 */

{
    int stat, opsta, errind;
    float tick, x1, x2, y, y1, y2, marker_size;
    int i;

    if (*n >= 1)
        {

        /* inquire and test GKS operating state */

        GQOPS (&opsta);

        if (opsta == GWSAC || opsta == GSGOP)
            {
            gus_set_scale (&lx.scale_options, &stat);

            if (odd(stat))
                {
		i = 0;
		GQMKSC (&errind, &marker_size);

                while (i < *n && !cli_b_abort && odd(stat))
		    {
		    tick = marker_size * error_tick_size * (lx.y_max - 
			lx.y_min);

		    y = py[i];
		    y1 = y_log(y_lin(y, NIL) - tick);
		    y2 = y_log(y_lin(y, NIL) + tick);
		    x1 = e1[i];
		    x2 = e2[i];

                    start_pline (&x1, &y1, NIL, &stat);
                    pline (&x1, &y2, NIL, &stat);
                    end_pline ();

		    start_pline (&x1, &y, NIL, &stat);
		    pline (&x2, &y, NIL, &stat);
		    end_pline ();

                    start_pline (&x2, &y1, NIL, &stat);
                    pline (&x2, &y2, NIL, &stat);
                    end_pline ();

		    i++;
		    }

		gus_polymarker (n, px, py, &stat);
		}
            }
        else
            /* GKS not in proper state. GKS must be either in the state WSAC 
               or SGOP */

            stat = gus__notact;
        }
    else
        stat = gus__invnumpnt;  /* invalid number of points */

    if (present(status))
        *status = stat;
    else
        if (!odd(stat))
            gus_signal (stat, "HORIZONTAL_ERROR_BARS");

    return (stat);
}



int gus_bar_graph (int *n, float *px, float *py, float *width, int *status)

/*
 * FUNCTIONAL DESCRIPTION:
 *
 *	Draw a bar graph.
 *
 * FORMAL ARGUMENT(S):
 *
 *	N		Number of points
 *	PX, PY		X, Y - coordinates
 *	WIDTH		Width for bars
 *	STATUS		Optional status value
 *
 * IMPLICIT INPUTS:
 *
 *	NONE
 *
 * IMPLICIT OUTPUTS:
 *
 *	NONE
 *
 * COMPLETION CODES:
 *
 *	GUS__NORNAL	normal successful completion
 *	GUS__NOTACT	GKS not in proper state. GKS must be either in the
 *			state WSAC or SGOP
 *	GUS__INVWINLIM	cannot apply logarithmic transformation to current
 *			window
 *	GUS__INVNUMPNT	invalid number of points
 *	GUS__INVPNT	point co-ordinates must be greater than 0
 *
 * SIDE EFFECTS:
 *
 *	NONE
 *
 */

{
    int stat, opsta, errind, tnr, int_style;
    float wn[4], vp[4];

    float x[4], y[4], factor, w;
    int i, np;

    if (*n > 1 || width != 0)
        {

        /* inquire and test GKS operating state */

        GQOPS (&opsta);

        if (opsta >= GWSAC)
            {
            gus_set_scale (&lx.scale_options, &stat);

            if (odd(stat))
                {

		if (!(option_x_log IN lx.scale_options))
		    {

		    /* inquire current normalization transformation */

		    GQCNTN (&errind, &tnr);
		    GQNT (&tnr, &errind, wn, vp);

		    if (*width >= 0)
			{
		        /* inquire current fill area interior style */

		        GQFAIS (&errind, &int_style);
		        if (int_style == GHOLLO)
			    factor = 0.5;
		        else
			    factor = 0.3;

			i = 0;
			w = (*width > 0) ? *width : factor * (px[1] - px[0]);

		        while ((i < *n) && (!cli_b_abort) && odd(stat))
			    {
			    x[0] = px[i] - w;
			    x[1] = x[0];
			    if (i < *n-1) w = (*width > 0) ?
			        *width : factor * (px[i+1] - px[i]);
			    x[2] = px[i] + w;
			    x[3] = x[2];
			    y[0] = y_lin (wn[2], NIL);
			    y[1] = y_lin (py[i], &stat);
			    y[2] = y[1];
			    y[3] = y[0];

			    np = 4;
			    GFA (&np, x, y);

			    if (int_style == GHATCH || int_style == GPATTR)
			        {
			        GSFAIS (&GHOLLO);
			        GFA (&np, x, y);
			        GSFAIS (&int_style);
			        }

			    i++;
			    }
			}
		    else
			{
			i = 0;

		        while ((i < *n) && (!cli_b_abort) && odd(stat))
			    {
			    x[0] = px[i];
			    x[1] = x[0];
			    if (wn[2] > 0)
				y[0] = y_lin (wn[2], NIL);
			    else
				y[0] = 0;
			    y[1] = y_lin (py[i], &stat);

			    np = 2;
			    GPL (&np, x, y);

			    i++;
			    }
			}
		    }
		else
		    stat = gus__invscale; /* invalid scale options */
                }
            }
        else
            /* GKS not in proper state. GKS must be either in the state WSAC 
               or SGOP */

            stat = gus__notact;
        }
    else
        stat = gus__invnumpnt;  /* invalid number of points */

    if (present(status))
        *status = stat;
    else
        if (!odd(stat))
            gus_signal (stat, "BAR_GRAPH");

    return (stat);
}



int gus_fft (int *n, float *py, int *m, int *status)

/*
 * FUNCTIONAL DESCRIPTION:
 *
 *	Compute the Fast Fourier Transform of a real valued sequence
 *	of points draw the absolute values of the complex coefficients
 *
 * FORMAL ARGUMENT(S):
 *
 *	N		Number of points
 *	PY		Co-ordinates of points in world co-ordinates
 *	M		Number of complex coefficients to be computed
 *			(must be a power of two)
 *	STATUS		Optional status value
 *
 * IMPLICIT INPUTS:
 *
 *	NONE
 *
 * IMPLICIT OUTPUTS:
 *
 *	NONE
 *
 * REQUIRED ROUTINES:
 *
 *	MTH_REALFT
 *
 * COMPLETION CODES:
 *
 *	GUS__NORMAL	normal successful completion
 *	GUS__NOTACT	GKS not in proper state. GKS must be either in the
 *			state WSAC or SGOP
 *	GUS__INVWINLIM	cannot apply logarithmic transformation to current
 *			window
 *	GUS__INVNUMPNT	invalid number of points
 *	GUS__NOTPOWTWO	number of coefficients is not a power of two
 *
 * SIDE EFFECTS:
 *
 *	NONE
 *
 */

{

    int stat, opsta, ret_stat, i;
    BOOL new_line;

    float x;

    float *f;
    float *fr, *fi;

    f  = (float *) gmalloc(sizeof(float) * 2 * *m);
    fr = (float *) gmalloc(sizeof(float) * *m);
    fi = (float *) gmalloc(sizeof(float) * *m);

    if (*n > 2)
        {
        i = *m;
        stat = gus__normal;

        do
            {
            if (i % 2 != 0)
                stat = gus__notpowtwo;
            i = i / 2;
            }
        while (odd(stat) && (i != 1));
        }
    else
        stat = gus__invnumpnt;  /* invalid number of points */

    if (odd(stat))
        {

        /* inquire and test GKS operating state */

        GQOPS (&opsta);

        if (opsta >= GWSAC)
            {
            gus_set_scale (&lx.scale_options, &stat);

            if (odd(stat))
                {
                if (odd(stat) && (!cli_b_abort))
                    {
                    for (i = 0; i < *n; i++)
                        f[i] = py[i];
                    for (i = *n; i < 2 * *m; i++)
                        f[i] = 0;

                    /* compute the direct transform */

                    mth_realft (f, *m, 1);

                    for (i = 0; i < 2 * *m; i++)
                        f[i] = 2 * f[i] / *m;

                    for (i = 0; i < *m; i++)
                        {
                        fr[i] = f[2*i];
                        fi[i] = f[2*i+1];
                        f[i] = sqrt(pow(fr[i], 2.) + pow(fi[i], 2.));
                        }

                    if (logging_switch && (!cli_b_abort))
                        write_log (fr, fi, *m);

                    i = 0;
                    new_line = TRUE;

                    while ((i < *n) && (!cli_b_abort))
                        {
                        i++;
                        x = i;

                        if (new_line)
                            start_pline (&x, &f[i-1], NIL, &ret_stat);
                        else
                            pline (&x, &f[i-1], NIL, &ret_stat);

                        new_line = (!odd(ret_stat));

                        if (new_line)
                            {
                            end_pline ();
                            stat = ret_stat;
                            }
                        }

                    end_pline ();
                    }
                }
            }
        else
            stat = gus__notact;  /* GKS not in proper state */

        }

    if (present(status))
        *status = stat;
    else
        if (!odd (stat))
            gus_signal (stat, "FFT");

    free (fi);
    free (fr);
    free (f);

    return (stat);
}



int gus_inverse_fft (int *n, float *fr, float *fi, int *status)

/*
 * FUNCTIONAL DESCRIPTION:
 *
 *	Compute inverse Fast Fourier Transform of the given complex numbers
 *
 * FORMAL ARGUMENT(S):
 *
 *	N		Number of complex numbers, must be a power of two
 *	PR,PI		Real and imaginary part of N complex numbers,
 *			the coefficients of the Fourier Transform
 *	STATUS		Optional status value
 *
 * IMPLICIT INPUTS:
 *
 *	NONE
 *
 * IMPLICIT OUTPUTS:
 *
 *	NONE
 *
 * REQUIRED ROUTINES:
 *
 *	MTH_REALFT
 *
 * COMPLETION CODES:
 *
 *	GUS__NORMAL	normal successful completion
 *	GUS__NOTACT	GKS not in proper state. GKS must be either in the
 *			state WSAC or SGOP
 *	GUS__INVWINLIM	cannot apply logarithmic transformation to current
 *			window
 *	GUS__NOTPOWTWO	number of coefficients is not a power of two
 *
 * SIDE EFFECTS:
 *
 *	NONE
 */

{

    int stat, opsta, ret_stat, i;
    BOOL new_line;

    float x;
    float *f;

    f = (float *) gmalloc(sizeof(float) * 2 * *n);

    if (*n > 2)
        {
        i = *n;
        stat = gus__normal;

        do
            {
            if (i % 2 != 0)
                stat = gus__notpowtwo;  /* number of coefficients is not a 
					   power of two */
            i = i / 2;
            }
        while (odd (stat) && (i != 1));
        }
    else
        stat = gus__invnumpnt;  /* invalid number of points */

    if (odd(stat))
        {

        /* inquire and test GKS operating state */

        GQOPS (&opsta);

        if (opsta >= GWSAC)
            {
            gus_set_scale (&lx.scale_options, &stat);

            if (odd(stat))
                {

                /* set up working array */

                for (i = 0; i < *n; i++)
                    {
                    f[2*i] = fr[i];
                    f[2*i+1] = fi[i];
                    }

                /* compute the inverse transform */

                mth_realft (f, *n, -1);

                for (i = 0; i < 2 * *n; i++)
                    f[i] = f[i] / 2 * *n;

                i = 0;
                new_line = TRUE;

                while ((i < *n) && (!cli_b_abort))
                    {
                    i++;
                    x = i;

                    if (new_line)
                        start_pline (&x, &f[i-1], NIL, &ret_stat);
                    else
                        pline (&x, &f[i-1], NIL, &ret_stat);

                    new_line = (!odd(ret_stat));

                    if (new_line)
                        {
                        end_pline ();
                        stat = ret_stat;
                        }
                    }

                end_pline ();
                }
            }
        else
            stat = gus__notact;  /* GKS not in proper state */
        }

    if (present(status))
        *status = stat;
    else
        if (!odd(stat))
            gus_signal (stat, "INVERSE_FFT");

    free (f);

    return (stat);
}



static void clear_active_ws (void)

/*
 * clear_active_ws - clear all active workstations
 */

{
    int state, count, n, errind, ol;
    int wkid, conid, wtype, wkcat;

    GQOPS (&state);
    if (state == GSGOP)
        GCLSG ();

    if (state >= GWSAC)
        {
        n = 1;
        GQACWK (&n, &errind, &ol, &wkid);

        for (count = 1; count <= ol; count++)
            {
            n = count;
            GQACWK (&n, &errind, &ol, &wkid);

	    GQWKC (&wkid, &errind, &conid, &wtype);
	    GQWKCA (&wtype, &errind, &wkcat);

	    if (wkcat == GOUTPT || wkcat == GOUTIN || wkcat == GMO)
	        GCLRWK (&wkid, &GALWAY);
            }
        }
}



int gus_plot (int *n, float *px, float *py, int *status)

/*
 * FUNCTIONAL DESCRIPTION:
 *
 *	Produce a graph of vector data.
 *
 * FORMAL ARGUMENT(S):
 *
 *	N		Number of points
 *	PX,PY		Co-ordinates of points in world co-ordinates
 *	STATUS		Optional status value
 *
 * IMPLICIT INPUTS:
 *
 *	NONE
 *
 * IMPLICIT OUTPUTS:
 *
 *	NONE
 *
 * COMPLETION CODES:
 *
 *	GUS__NORNAL	normal successful completion
 *	GUS__NOTACT	GKS not in proper state. GKS must be either in the
 *			state WSAC or SGOP
 *	GUS__INVWINLIM	cannot apply logarithmic transformation to current
 *			window
 *	GUS__INVNUMPNT	invalid number of points
 *	GUS__INVPNT	point co-ordinates must be greater than 0
 *
 * SIDE EFFECTS:
 *
 *	NONE
 *
 */

{
    int stat, ret_stat, opsta, errind, tnr, i;

    float *smy;

    float min_x, max_x, min_y, max_y;

    float x_tick, y_tick;
    int major_x, major_y;

    BOOL new_line;

    float x_min, x_max, y_min, y_max, t_size;
    
    smy = (float *) gmalloc(sizeof(float) * *n);

    if (*n > 1)
        {

        /* inquire and test GKS operating state */

        GQOPS (&opsta);

        if (opsta >= GWSAC)
            {
            gus_set_scale (&lx.scale_options, &stat);

            if (odd(stat))
                {
                if (gus_gl_smoothing_level != 0)
                    {
                    mth_smooth (*n, py, smy, gus_gl_smoothing_level);

                    py = smy;

                    if (logging_switch)
                        write_log (px, py, *n);
                    }

                /* clear all active workstations */

                clear_active_ws ();

                gus_autoscale (n, px, py, &min_x, &max_x, &min_y, &max_y, NIL);

                x_tick = gus_tick(&min_x, &max_x);
                major_x = 1;
                y_tick = gus_tick(&min_y, &max_y);
                major_y = 1;

                GQCNTN (&errind, &tnr);

                if (tnr == 0)
                    tnr = 1;

		x_min = 0.15;
		x_max = 0.95;
		y_min = 0.15;
		y_max = 0.95;

                GSVP (&tnr, &x_min, &x_max, &y_min, &y_max);
                GSWN (&tnr, &min_x, &max_x, &min_y, &max_y);

		t_size = tick_size;
                gus_axes (&x_tick, &y_tick, &min_x, &min_y, &major_x, &major_y,
		    &t_size, NIL);

                i = 0;
                new_line = TRUE;

                while ((i < *n) && (!cli_b_abort))
                    {
                    i++;

                    if (new_line)
                        start_pline (&px[i-1], &py[i-1], NIL, &ret_stat);
                    else
                        pline (&px[i-1], &py[i-1], NIL, &ret_stat);

                    new_line = (!odd(ret_stat));

                    if (new_line)
                        {
                        end_pline ();
                        stat = ret_stat;
                        }
                    }

                end_pline ();
                }
            }
        else
            /* GKS not in proper state. GKS must be either in the state WSAC 
	       or SGOP */

            stat = gus__notact;
        }
    else
        stat = gus__invnumpnt;  /* invalid number of points */

    if (present(status))
        *status = stat;
    else
        if (!odd(stat))
            gus_signal (stat, "PLOT");

    free (smy);

    return (stat);
}



int gus_set_logging (BOOL *lswitch, int *status)

/*
 * FUNCTIONAL DESCRIPTION:
 *
 *	Reset the logging switch.
 *
 * FORMAL ARGUMENT(S):
 *
 *	LSWITCH		Logging switch
 *	STATUS		Optional status value
 *
 * IMPLICIT INPUTS:
 *
 *	NONE
 *
 * IMPLICIT OUTPUTS:
 *
 *	NONE
 *
 * COMPLETION CODES:
 *
 *	GUS__NORNAL	normal successful completion
 *	GUS__GKCL	GKS not in proper state. GKS must be in one of the
 *			states GKOP,WSOP,WSAC or SGOP
 *
 * SIDE EFFECTS:
 *
 *	NONE
 *
 */

{
    int stat, opsta;

    /* inquire and test GKS operating state */

    GQOPS (&opsta); 

    if (opsta >= GGKOP)
        {
        logging_switch = *lswitch;

        stat = gus__normal;
        }
    else
        /* GKS not in proper state. GKS must be in one of the states GKOP,WSOP,
           WSAC or SGOP */

        stat = gus__gkcl;

    if (present(status))
        *status = stat;
    else
        if (!odd(stat))
            gus_signal (stat, "SET_LOGGING");

    return (stat);
}



int gus_set_scale (int *options, int *status)

/*
 * FUNCTIONAL DESCRIPTION:
 *
 *	Set up logarithmic transformation for current GKS normalization
 *	transformation.
 *
 * FORMAL ARGUMENT(S):
 *
 *	OPTIONS		Scale options
 *	STATUS		Optional status value
 *
 * IMPLICIT INPUTS:
 *
 *	NONE
 *
 * IMPLICIT OUTPUTS:
 *
 *	NONE
 *
 * COMPLETION CODES:
 *
 *	GUS__NORNAL	normal successful completion
 *	GUS__GKCL	GKS not in proper state. GKS must be in one of the
 *			states GKOP,WSOP,WSAC or SGOP
 *	GUS__INVWINLIM	cannot apply logarithmic transformation to current
 *			window
 *
 * SIDE EFFECTS:
 *
 *	NONE
 *
 */

{
    int stat, errind, opsta, tnr;
    float wn[4], vp[4];
    int scale_options;

    /* inquire and test GKS operating state */

    GQOPS (&opsta);

    if (opsta >= GGKOP)
	{
	stat = gus__normal;

	GQCNTN (&errind, &tnr);
	GQNT (&tnr, &errind, wn, vp);

	nx.a = (vp[1] - vp[0]) / (wn[1] - wn[0]);
	nx.b = vp[0] - wn[0] * nx.a;
	nx.c = (vp[3] - vp[2]) / (wn[3] - wn[2]);
	nx.d = vp[2] - wn[2] * nx.c;

	scale_options = *options;

	if ((scale_options != lx.scale_options) ||
	    (wn[0] != lx.x_min) || (wn[1] != lx.x_max) ||
	    (wn[2] != lx.y_min) || (wn[3] != lx.y_max) ||
	    (wx.z_min != lx.z_min) || (wx.z_max != lx.z_max))
	    {
	    lx.scale_options = 0;

	    lx.x_min = wn[0];
	    lx.x_max = wn[1];

	    if (option_x_log IN scale_options)
		{
		if (wn[0] > 0)
		    {
		    lx.a = (wn[1] - wn[0]) / log10(wn[1] / wn[0]);
		    lx.b = wn[0] - lx.a * log10(wn[0]);
		    lx.scale_options |= option_x_log;
		    }
		else
		    stat = gus__invwinlim;  /* cannot apply logarithmic 
					       transformation to current 
					       window */
		}

	    lx.y_min = wn[2];
	    lx.y_max = wn[3];

	    if (option_y_log IN scale_options)
		{
		if (wn[2] > 0)
		    {
		    lx.c = (wn[3] - wn[2]) / log10(wn[3] / wn[2]);
		    lx.d = wn[2] - lx.c * log10(wn[2]);
		    lx.scale_options |= option_y_log;
		    }
		else
		    stat = gus__invwinlim;  /* cannot apply logarithmic 
					       transformation to current 
					       window */
		}

	    gus_set_space (&wx.z_min, &wx.z_max, &wx.phi, &wx.delta, NIL);

	    lx.z_min = wx.z_min;
	    lx.z_max = wx.z_max;

	    if (option_z_log IN scale_options)
		{
		if (lx.z_min > 0)
		    {
		    lx.e = (lx.z_max - lx.z_min) / log10(lx.z_max / lx.z_min);
		    lx.f = lx.z_min - lx.e * log10(lx.z_min);
		    lx.scale_options |= option_z_log;
		    }
		else
		    stat = gus__invwinlim;  /* cannot apply logarithmic 
					       transformation to current 
					       window */
		}
            }

	    if (option_flip_x IN scale_options)
		lx.scale_options |= option_flip_x;

	    if (option_flip_y IN scale_options)
		lx.scale_options |= option_flip_y;

	    if (option_flip_z IN scale_options)
		lx.scale_options |= option_flip_z;
	}
    else
        /* GKS not in proper state. GKS must be in one of the states GKOP,WSOP,
           WSAC or SGOP */

        stat = gus__gkcl;

    if (present(status))
        *status = stat;
    else
        if (!odd(stat))
            gus_signal (stat, "SET_SCALE");

    return (stat);
}



int gus_set_ws_viewport (gus_viewport_window *window,
    gus_viewport_size *size, gus_viewport_orientation *orientation, int *status)

/*
 * FUNCTIONAL DESCRIPTION:
 *
 *	Set workstation viewport to specified size.
 *
 * FORMAL ARGUMENT(S):
 *
 *	WINDOW		Viewport window
 *	SIZE		Size
 *	STATUS		Optional status value
 *
 * IMPLICIT INPUTS:
 *
 *	NONE
 *
 * IMPLICIT OUTPUTS:
 *
 *	NONE
 *
 * COMPLETION CODES:
 *
 *	GUS__NORNAL	normal successful completion
 *	GUS__NOTACT	GKS not in proper state. GKS must be either in the
 *			state WSAC or SGOP
 *
 * SIDE EFFECTS:
 *
 *	NONE
 *
 */

{
    int stat, count, errind, opsta, ol, wkid;
    int wkcat, conid, wtype, tnr;
    float r, rx, ry, x_min, x_max, y_min, y_max;

    /* inquire and test GKS operating state */

    GQOPS (&opsta);
    if (opsta >= GWSAC)
        {
	count = 1;
        GQACWK (&count, &errind, &ol, &wkid);

        for (count = 1; count <= ol; count++)
            {
            GQACWK (&count, &errind, &ol, &wkid);

	    GQWKC (&wkid, &errind, &conid, &wtype);
	    GQWKCA (&wtype, &errind, &wkcat);

	    if (wkcat == GOUTPT || wkcat == GOUTIN)
		{
		rx = vp_size[(int) (*size)][0];
		ry = vp_size[(int) (*size)][1];
		if (*orientation == orientation_portrait)
		    {
		    r = rx; rx = ry; ry = r;
		    }

		x_min = vp_wn[(int) (*window)][0]*rx;
		x_max = vp_wn[(int) (*window)][1]*rx;
		y_min = vp_wn[(int) (*window)][2]*ry;
		y_max = vp_wn[(int) (*window)][3]*ry;
		GSWKVP (&wkid, &x_min, &x_max, &y_min, &y_max);

		r = (*orientation == orientation_landscape) ? rx : ry;
		x_min /= r;
		x_max /= r;
		y_min /= r;
		y_max /= r;
		GSWKWN (&wkid, &x_min, &x_max, &y_min, &y_max);

		GQCNTN (&errind, &tnr);
		if (tnr != gks_ndc)
		    {
		    r = x_max - x_min;
		    x_min += 0.2*r;
		    x_max -= 0.1*r;
		    r = y_max - y_min;
		    y_min += 0.2*r;
		    y_max -= 0.1*r;
		    GSVP (&tnr, &x_min, &x_max, &y_min, &y_max);
		    }
		}
	    }
        stat = gus__normal;
        }
    else
        /* GKS not in proper state. GKS must be either in the state WSAC 
           or SGOP */

        stat = gus__notact;

    if (present(status))
        *status = stat;
    else
        if (!odd(stat))
            gus_signal (stat, "SET_WS_VIEWPORT");

    return (stat);
}



int gus_set_smoothing (int *level, int *status)

/*
 * FUNCTIONAL DESCRIPTION:
 *
 *	Set smoothing level
 *
 * FORMAL ARGUMENT(S):
 *
 *	LEVEL		Smoothing level
 *	STATUS		Optional status value
 *
 * IMPLICIT INPUTS:
 *
 *	NONE
 *
 * IMPLICIT OUTPUTS:
 *
 *	NONE
 *
 * COMPLETION CODES:
 *
 *	GUS__NORNAL	normal successful completion
 *	GUS__NOTOPE	GKS not in proper state. GKS must be in one of the
 *			states WSOP, WSAC or SGOP
 *
 * SIDE EFFECTS:
 *
 *	NONE
 *
 */

{
    int stat, opsta;

    /* inquire and test GKS operating state */

    GQOPS (&opsta);

    if (opsta >= GGKOP)
        {
        if ((0 <= *level) && (*level <= 20))
            {
            gus_gl_smoothing_level = *level;

            stat = gus__normal;
            }
        else
            stat = gus__invsmolev;  /* invalid smoothing level */
        }
    else
        /* GKS not in proper state. GKS must be in one of the states GKOP,WSOP,
              WSAC or SGOP */

        stat = gus__gkcl;

    if (present(status))
        *status = stat;
    else
        if (!odd (stat))
            gus_signal (stat, "SET_SMOOTHING");

    return (stat);
}



int gus_inq_smoothing (int *level, int *status)

/*
 * FUNCTIONAL DESCRIPTION:
 *
 *	Inquire smoothing level
 *
 * FORMAL ARGUMENT(S):
 *
 *	LEVEL		Smoothing level
 *	STATUS		Optional status value
 *
 * IMPLICIT INPUTS:
 *
 *	NONE
 *
 * IMPLICIT OUTPUTS:
 *
 *	NONE
 *
 * COMPLETION CODES:
 *
 *	GUS__NORNAL	normal successful completion
 *	GUS__NOTOPE	GKS not in proper state. GKS must be in one of the
 *			states WSOP, WSAC or SGOP
 *
 * SIDE EFFECTS:
 *
 *	NONE
 *
 */

{
    int stat, opsta;

    /* inquire and test GKS operating state */

    GQOPS (&opsta);

    if (opsta >= GGKOP)
	{
	*level = gus_gl_smoothing_level;
	stat = gus__normal;
        }
    else
        /* GKS not in proper state. GKS must be in one of the states GKOP,WSOP,
              WSAC or SGOP */

        stat = gus__gkcl;

    if (present(status))
        *status = stat;
    else
        if (!odd (stat))
            gus_signal (stat, "INQ_SMOOTHING");

    return (stat);
}



int gus_inq_scale (int *options, int *status)

/*
 * FUNCTIONAL DESCRIPTION:
 *
 *	Inquire scale options
 *
 * FORMAL ARGUMENT(S):
 *
 *	OPTIONS		Scale options
 *	STATUS		Optional status value
 *
 * IMPLICIT INPUTS:
 *
 *	NONE
 *
 * IMPLICIT OUTPUTS:
 *
 *	NONE
 *
 * COMPLETION CODES:
 *
 *	GUS__NORNAL	normal successful completion
 *	GUS__NOTOPE	GKS not in proper state. GKS must be in one of the
 *			states WSOP, WSAC or SGOP
 *
 * SIDE EFFECTS:
 *
 *	NONE
 *
 */

{
    int stat, opsta;

    /* inquire and test GKS operating state */

    GQOPS (&opsta);

    if (opsta >= GGKOP)
	{
	*options = lx.scale_options;
	stat = gus__normal;
        }
    else
        /* GKS not in proper state. GKS must be in one of the states GKOP,WSOP,
              WSAC or SGOP */

        stat = gus__gkcl;

    if (present(status))
        *status = stat;
    else
        if (!odd (stat))
            gus_signal (stat, "INQ_SCALE");

    return (stat);
}



int gus_polyline (int *n, float *px, float *py, int *status)

/*
 * FUNCTIONAL DESCRIPTION:
 *
 *	Draw a polyline.
 *
 * FORMAL ARGUMENT(S):
 *
 *	N		Number of points
 *	PX,PY		Co-ordinates of points in world co-ordinates
 *	STATUS		Optional status value
 *
 * IMPLICIT INPUTS:
 *
 *	NONE
 *
 * IMPLICIT OUTPUTS:
 *
 *	NONE
 *
 * COMPLETION CODES:
 *
 *	GUS__NORNAL	normal successful completion
 *	GUS__NOTACT	GKS not in proper state. GKS must be either in the
 *			state WSAC or SGOP
 *	GUS__INVWINLIM	cannot apply logarithmic transformation to current
 *			window
 *	GUS__INVNUMPNT	invalid number of points
 *	GUS__INVPNT	point co-ordinates must be greater than 0
 *
 * SIDE EFFECTS:
 *
 *	NONE
 *
 */

{
    int stat, ret_stat, opsta, i;

    float *smy;

    BOOL new_line;

    smy = (float *) gmalloc(sizeof(float) * *n);

    if (*n > 1)
        {

        /* inquire and test GKS operating state */

        GQOPS (&opsta);

        if (opsta >= GWSAC)
            {
            gus_set_scale (&lx.scale_options, &stat);

            if (odd(stat))
                {
                if (gus_gl_smoothing_level != 0)
                    {
                    mth_smooth (*n, py, smy, gus_gl_smoothing_level);

                    py = smy;

                    if (logging_switch)
                        write_log (px, py, *n);
                    }
                i = 0;
                new_line = TRUE;

                while ((i < *n) && (!cli_b_abort))
                    {
                    i++;

                    if (new_line)
                        start_pline (&px[i-1], &py[i-1], NIL, &ret_stat);
                    else
                        pline (&px[i-1], &py[i-1], NIL, &ret_stat);

                    new_line = (!odd (ret_stat));

                    if (new_line)
                        {
                        end_pline ();
                        stat = ret_stat;
                        }
                    }

                end_pline ();
                }
            }
        else
            /* GKS not in proper state. GKS must be either in the state WSAC 
	       or SGOP */

            stat = gus__notact;
        }
    else
        stat = gus__invnumpnt;  /* invalid number of points */

    if (present(status))
        *status = stat;
    else
        if (!odd (stat))
            gus_signal (stat, "POLYLINE");

    free (smy);

    return (stat);
}



int gus_polymarker (int *n, float *px, float *py, int *status)

/*
 * FUNCTIONAL DESCRIPTION:
 *
 *	Draw marker symbols.
 *
 * FORMAL ARGUMENT(S):
 *
 *	N		Number of points
 *	PX,PY		Co-ordinates of points in world co-ordinates
 *	STATUS		Optional status value
 *
 * IMPLICIT INPUTS:
 *
 *	NONE
 *
 * IMPLICIT OUTPUTS:
 *
 *	NONE
 *
 * COMPLETION CODES:
 *
 *	GUS__NORNAL	normal successful completion
 *	GUS__NOTACT	GKS not in proper state. GKS must be either in the
 *			state WSAC or SGOP
 *	GUS__INVWINLIM	cannot apply logarithmic transformation to current
 *			window
 *	GUS__INVNUMPNT	invalid number of points
 *	GUS__INVPNT	point co-ordinates must be greater than 0
 *
 * SIDE EFFECTS:
 *
 *	NONE
 *
 */

{
    int stat, ret_stat, opsta, i;
    float xw,yw;

    int np;

    if (*n > 0)
        {

        /* inquire and test GKS operating state */

        GQOPS (&opsta);

        if (opsta >= GWSAC)
            {
            gus_set_scale (&lx.scale_options, &stat);

            if (odd(stat))
                {
                i = 0;

                while ((i < *n) && (!cli_b_abort))
                    {
                    i++;
                    xw = x_lin(px[i-1], &stat);

                    if (odd(stat))
                        {
                        yw = y_lin(py[i-1], &ret_stat);

                        if (odd(ret_stat))
			    {
			    np = 1;
                            GPM (&np, &xw, &yw);
			    }
                        else
                            stat = ret_stat;
                        }
                    }
                }
            }
        else
            /* GKS not in proper state. GKS must be either in the state WSAC 
	       or SGOP */

            stat = gus__notact;
        }
    else
        stat = gus__invnumpnt;  /* invalid number of points */

    if (present(status))
        *status = stat;
    else
        if (!odd(stat))
            gus_signal (stat, "POLYMARKER");

    return (stat);
}



int gus_fill_area (int *n, float *px, float *py, int *status)

/*
 * FUNCTIONAL DESCRIPTION:
 *
 *	Draw a filled area.
 *
 * FORMAL ARGUMENT(S):
 *
 *	N		Number of points
 *	PX,PY		Co-ordinates of points in world co-ordinates
 *	STATUS		Optional status value
 *
 * IMPLICIT INPUTS:
 *
 *	NONE
 *
 * IMPLICIT OUTPUTS:
 *
 *	NONE
 *
 * COMPLETION CODES:
 *
 *	GUS__NORNAL	normal successful completion
 *	GUS__NOTACT	GKS not in proper state. GKS must be either in the
 *			state WSAC or SGOP
 *	GUS__INVWINLIM	cannot apply logarithmic transformation to current
 *			window
 *	GUS__INVNUMPNT	invalid number of points
 *	GUS__INVPNT	point co-ordinates must be greater than 0
 *
 * SIDE EFFECTS:
 *
 *	NONE
 *
 */

{
    int stat, opsta, i;

    float *smy, *tx, *ty;

    smy = (float *) gmalloc(sizeof(float) * *n);
    tx = (float *) gmalloc(sizeof(float) * *n);
    ty = (float *) gmalloc(sizeof(float) * *n);

    if (*n > 2)
        {

        /* inquire and test GKS operating state */

        GQOPS (&opsta);

        if (opsta >= GWSAC)
            {
            gus_set_scale (&lx.scale_options, &stat);

            if (odd(stat))
                {
                if (gus_gl_smoothing_level != 0)
                    {
                    mth_smooth (*n, py, smy, gus_gl_smoothing_level);

                    py = smy;

                    if (logging_switch)
                        write_log (px, py, *n);
                    }
                i = 0;

                while ((i < *n) && odd(stat) && (!cli_b_abort))
                    {
		    tx[i] = x_lin(px[i], &stat);

		    if (odd(stat))
			ty[i] = y_lin(py[i], &stat);

                    i++;
                    }

		if (odd(stat))
		    GFA (n, tx, ty);
                }
            }
        else
            /* GKS not in proper state. GKS must be either in the state WSAC 
	       or SGOP */

            stat = gus__notact;
        }
    else
        stat = gus__invnumpnt;  /* invalid number of points */

    if (present(status))
        *status = stat;
    else
        if (!odd (stat))
            gus_signal (stat, "FILL_AREA");

    free (smy);
    free (tx);
    free (ty);

    return (stat);
}



int gus_set_space (float *z_min, float *z_max, int *rotation, int *tilt,
    int *status)

/*
 * FUNCTIONAL DESCRIPTION:
 *
 *	Set up three-dimensional transformation for current GKS normalization
 *	transformation.
 *
 * FORMAL ARGUMENT(S):
 *
 *	Z_MIN, Z_MAX	Z-axis limits
 *	ROTATION, TILT	Angles for rotation and tilt
 *	STATUS		Optional status value
 *
 * IMPLICIT INPUTS:
 *
 *	NONE
 *
 * IMPLICIT OUTPUTS:
 *
 *	NONE
 *
 * COMPLETION CODES:
 *
 *	GUS__NORNAL	normal successful completion
 *	GUS__GKCL	GKS not in proper state. GKS must be in one of the
 *			states GKOP,WSOP,WSAC or SGOP
 *	GUS__INVANGLE	invalid angle
 *
 * SIDE EFFECTS:
 *
 *	NONE
 *
 */

{
    int stat, errind, opsta, tnr;
    float wn[4], vp[4];
    float x_min, x_max, y_min, y_max, r, t, a, c;

    if (*z_min < *z_max)
        {
        if ((*rotation >= 0) && (*rotation <= 90) && (*tilt >= 0) && 
	    (*tilt <= 90))
            stat = gus__normal;
        else
            stat = gus__invangle;  /* invalid angle */

        }
    else
        stat = gus__invzaxis;  /* invalid z-axis specification */

    if (odd(stat))
        {

        /* inquire and test GKS operating state */

        GQOPS (&opsta);

        if (opsta >= GGKOP)
            {
            stat = gus__normal;

            GQCNTN (&errind, &tnr);
            GQNT (&tnr, &errind, wn, vp);

            x_min = wn[0];
            x_max = wn[1];
            y_min = wn[2];
            y_max = wn[3];

	    wx.z_min = *z_min;
	    wx.z_max = *z_max;
	    wx.phi = *rotation;
	    wx.delta = *tilt;

	    r = arc(*rotation);
	    wx.a1 = cos(r);
	    wx.a2 = sin(r);

	    a = (x_max - x_min) / (wx.a1 + wx.a2);
	    wx.b = x_min;

	    wx.a1 = a * wx.a1 / (x_max - x_min);
	    wx.a2 = a * wx.a2 / (y_max - y_min);
	    wx.b = wx.b - wx.a1 * x_min-wx.a2 * y_min;

	    t = arc(*tilt);
	    wx.c1 = (pow(cos(r), 2.) - 1.) * tan(t / 2.);
	    wx.c2 = -(pow(sin(r), 2.) - 1.) * tan(t / 2.);
	    wx.c3 = cos(t);

	    c = (y_max - y_min) / (wx.c2 + wx.c3 - wx.c1);
	    wx.d = y_min - c * wx.c1;

	    wx.c1 = c * wx.c1 / (x_max - x_min);
	    wx.c2 = c * wx.c2 / (y_max - y_min);
	    wx.c3 = c * wx.c3 / (*z_max - *z_min);
	    wx.d = wx.d - wx.c1 * x_min - wx.c2 * y_min - wx.c3 * *z_min;

            }
        else
            /* GKS not in proper state. GKS must be in one of the states GKOP,
               WSOP, WSAC or SGOP */

            stat = gus__gkcl;
        }

    if (present(status))
        *status = stat;
    else
        if (!odd(stat))
            gus_signal (stat, "SET_SPACE");

    return (stat);
}



static float  cxl, cxr, cyf, cyb, czb, czt;

static void code (float *x, float *y, float *z, outcode *c)
{
    *c = 0;
    if (*x < cxl)
        *c = left;
    else
        if (*x > cxr)
            *c = right;
    if (*y < cyf)
        *c = *c | front;
    else
        if (*y > cyb)
            *c = *c | back;
    if (*z < czb)
        *c = *c | bottom;
    else
        if (*z > czt)
            *c = *c | top;
}



static void clip_3d (float *x0, float *x1, float *y0, float *y1, float *z0,
    float *z1, BOOL *visible, BOOL *clip)

/*
 * clip_3d - three-dimensional clipping routine
 */

{
    outcode c, c0, c1;
    float x, y, z;

    code (x0, y0, z0, &c0);
    code (x1, y1, z1, &c1);

    *clip = (c1 != 0);
    *visible = FALSE;

    while ((c0 | c1) != 0)
        {
        if ((c0 & c1) != 0)
            goto done;

        if (c0 != 0)
            c = c0;
        else
            c = c1;

        if (left IN c)
            {

            /* crosses left plane */

            x = cxl;
            y = *y0 + (*y1 - *y0) * (cxl - *x0) / (*x1 - *x0);
            z = *z0 + (*z1 - *z0) * (cxl - *x0) / (*x1 - *x0);
            }
        else
            if (right IN c)
                {

                /* crosses right plane */

                x = cxr;
                y = *y0 + (*y1 - *y0) * (cxr - *x0) / (*x1 - *x0);
                z = *z0 + (*z1 - *z0) * (cxr - *x0) / (*x1 - *x0);
                }
            else
                if (front IN c)
                    {

                    /* crosses front plane */

                    x = *x0 + (*x1 - *x0) * (cyf - *y0) / (*y1 - *y0);
                    y = cyf;
                    z = *z0 + (*z1 - *z0) * (cyf - *y0) / (*y1 - *y0);
                    }
                else
                    if (back IN c)
                        {

                        /* crosses back plane */

                        x = *x0 + (*x1 - *x0) * (cyb - *y0) / (*y1 - *y0);
                        y = cyb;
                        z = *z0 + (*z1 - *z0) * (cyb - *y0) / (*y1 - *y0);
                        }
                    else
                        if (bottom IN c)
                            {

                            /* crosses bottom plane */

                            x = *x0 + (*x1 - *x0) * (czb - *z0) / (*z1 - *z0);
                            y = *y0 + (*y1 - *y0) * (czb - *z0) / (*z1 - *z0);
                            z = czb;
                            }
                        else
                            if (top IN c)
                                {

                                /* crosses top plane */

                                x = *x0 + (*x1 - *x0) * (czt - *z0) / 
				    (*z1 - *z0);
                                y = *y0 + (*y1 - *y0) * (czt - *z0) / 
				    (*z1 - *z0);
                                z = czt;
                                }
        if (c == c0)
            {
            *x0 = x;
            *y0 = y;
            *z0 = z;
            code (&x, &y, &z, &c0);
            }
        else
            {
            *x1 = x;
            *y1 = y;
            *z1 = z;
            code (&x, &y, &z, &c1);
            }
        }

    /* if we reach here, the line from (x0,y0,z0) to (x1,y1,z1) is visible */

    *visible = TRUE;
    done:;
}



int gus_curve (int *n, float *px, float *py, float *pz,
    gus_primitive *primitive, int *status)

/*
 * FUNCTIONAL DESCRIPTION:
 *
 *	Draw a curve.
 *
 * FORMAL ARGUMENT(S):
 *
 *	N		Number of points
 *	PX,PY,PZ	Co-ordinates of points in world co-ordinates
 *	PRIMITIVE       Output primitive
 *	STATUS		Optional status value
 *
 * IMPLICIT INPUTS:
 *
 *	NONE
 *
 * IMPLICIT OUTPUTS:
 *
 *	NONE
 *
 * COMPLETION CODES:
 *
 *	GUS__NORNAL	normal successful completion
 *	GUS__NOTACT	GKS not in proper state. GKS must be either in the
 *			state WSAC or SGOP
 *	GUS__INVWINLIM	cannot apply logarithmic transformation to current
 *			window
 *	GUS__INVNUMPNT	invalid number of points
 *	GUS__INVPNT	point co-ordinates must be greater than 0
 *
 * SIDE EFFECTS:
 *
 *	NONE
 *
 */

{  /* curve */

    int stat, opsta, errind, clsw, i;
    float clrt[4];

    float x0, y0, z0, x1, y1, z1;
    BOOL clipped, clip, visible;

    float *x, *y, *z;

    if (*n > 1)
        {

        /* inquire and test GKS operating state */

        GQOPS (&opsta);

        if (opsta >= GWSAC)
            {
            gus_set_scale (&lx.scale_options, &stat);

            if (odd(stat))
                {
                GQCLIP (&errind, &clsw, clrt);

                if (clsw == GCLIP)
		    {
		    cxl = lx.x_min; cxr = lx.x_max;
		    cyf = lx.y_min; cyb = lx.y_max;
		    czb = lx.z_min; czt = lx.z_max;
		    }
                else
                    {
                    clip = FALSE;
                    visible = TRUE;
                    }

                x = px; y = py; z = pz;

                if (*primitive == primitive_polyline)
                    {
                    clipped = TRUE;
                    i = 1;

                    while (odd (stat) && (i < *n) && (!cli_b_abort))
                        {
                        i++;

                        x0 = x[i-2]; y0 = y[i-2]; z0 = z[i-2];
                        x1 = x[i-1]; y1 = y[i-1]; z1 = z[i-1];

                        if (clsw == GCLIP)
                            clip_3d (&x0, &x1, &y0, &y1, &z0, &z1, &visible,
                                &clip);

                        if (visible)
                            {
                            if (clipped)
                                start_pline (&x0, &y0, &z0, &stat);

                            if (odd(stat))
                                {
                                pline (&x1, &y1, &z1, &stat);
                                if (clip)
                                    end_pline ();

                                clipped = clip;
                                }
                            }
                        }

                    end_pline ();
                    }
                else
                    {
                    i = 0;

                    while (odd (stat) && (i < *n) && (!cli_b_abort))
                        {
                        x0 = x[i]; y0 = y[i]; z0 = z[i];

                        if (clsw == GCLIP)
                            visible = x0 >= cxl && x0 <= cxr && y0 >= cyf &&
                                y0 <= cyb && z0 >= czb && z0 <= czt;
                        else
                            visible = TRUE;

                        if (visible)
                            {
                            if (i == 0)
                                start_pmark (&x0, &y0, &z0, &stat);
                            else
                                pmark (&x0, &y0, &z0, &stat);
                            }
                        i++;
                        }

                    end_pmark ();
                    }
                }
            }
        else
            /* GKS not in proper state. GKS must be either in the state WSAC
               or SGOP */

            stat = gus__notact;
        }
    else
        stat = gus__invnumpnt;  /* invalid number of points */

    if (present(status))
        *status = stat;
    else
        if (!odd(stat))
            gus_signal (stat, "CURVE");

    return (stat);
}



static void text_3d (float x, float y, float z, char *chars)

/*
 * text_3d - three-dimensional text routine
 */

{
    int errind, tnr;
    int t_nr;
    float x1, y1, z1;

    x1 = x_lin(x, NIL);
    y1 = y_lin(y, NIL);
    z1 = z_lin(z, NIL);

    apply_world_xform (&x1, &y1, &z1);

    x = x1;
    y = y1;

    GQCNTN (&errind, &tnr);

    if (tnr != gks_ndc)
        {
	x = nx.a * x + nx.b;
	y = nx.c * y + nx.d;

	t_nr = gks_ndc;
        GSELNT (&t_nr);
        }

    x1 = x;
    y1 = y;

    gus_text_routine (&x1, &y1, chars, NIL);

    if (tnr != gks_ndc)
        GSELNT (&tnr);
}



int gus_axes_3d (float *x_tick, float *y_tick, float *z_tick,
    float *x_org, float *y_org, float *z_org,
    int *major_x, int *major_y, int *major_z, float *t_size, int *status)

/*
 * FUNCTIONAL DESCRIPTION:
 *
 *	Draw three-dimensional co-ordinate axes, with linearly or
 *	logarithmically spaced tick-marks.
 *
 * FORMAL ARGUMENT(S):
 *
 *	X_TICK, Y_TICK
 *	Z_TICK		Length of the interval between tick-marks
 *			 (0=no axis)
 *	X_ORG, Y_ORG,
 *	Z_ORG		Co-ordinates of the origin (point of intersection)
 *			of the three axes
 *	MAJOR_X, MAJOR_Y,
 *	MAJOR_Z		Number of minor tick intervals between major
 *			tick-marks
 *			 (0=no labels, 1=no minor tick-marks)
 *	T_SIZE		Length of the minor tick-marks
 *	STATUS		Optional status value
 *
 * IMPLICIT INPUTS:
 *
 *	NONE
 *
 * IMPLICIT OUTPUTS:
 *
 *	NONE
 *
 * COMPLETION CODES:
 *
 *	GUS__NORNAL	normal successful completion
 *	GUS__NOTACT	GKS not in proper state. GKS must be either in the
 *			state WSAC or SGOP
 *	GUS__INVWINLIM	cannot apply logarithmic transformation to current
 *			window
 *	GUS__INVINTLEN	invalid interval length for major tick-marks
 *	GUS__INVNUMTIC	invalid number for minor tick intervals
 *	GUS__INVTICSIZ	invalid tick-size
 *	GUS__ORGOUTWIN	origin outside current window
 *
 * SIDE EFFECTS:
 *
 *	NONE
 *
 */

{
    int stat, opsta, errind, tnr;
    int ltype, halign, valign, font, prec, clsw;
    float chux, chuy;

    float clrt[4], wn[4], vp[4];
    float x_min, x_max, y_min, y_max, z_min, z_max;

    float r, alpha, beta;
    float a[2], c[2];

    float tick, minor_tick, major_tick, x_label, y_label;
    float x0, y0, z0, xi, yi, zi;

    int i, decade, exponent;

    char text[256], expn[256];

    float slant, text_slant[4];
    int *anglep, which_rep, rep;

    if ((*x_tick < 0) || (*y_tick < 0) || (*z_tick < 0))
        stat = gus__invintlen;  /* invalid interval length for major 
				   tick-marks */
    else
        if (t_size == 0)
            stat = gus__invticsiz;  /* invalid tick-size */
        else
            stat = gus__normal;

    if (odd(stat))
        {

        /* inquire and test GKS operating state */

        GQOPS (&opsta);

        if (opsta >= GWSAC)
            {
            gus_set_scale (&lx.scale_options, &stat);

            if (odd(stat))
                {

                /* inquire current normalization transformation */

                GQCNTN (&errind, &tnr);
                GQNT (&tnr, &errind, wn, vp);

                x_min = wn[0];
                x_max = wn[1];
                y_min = wn[2];
                y_max = wn[3];

		z_min = wx.z_min;
		z_max = wx.z_max;

                if ((x_min <= *x_org) && (*x_org <= x_max) && 
		   (y_min <= *y_org) && (*y_org <= y_max) && 
		   (z_min <= *z_org) && (*z_org <= z_max))
                    {
		    r = (x_max - x_min) / (y_max - y_min) * (vp[3] - vp[2]) / 
			(vp[1] - vp[0]);

		    alpha = atan_2(r * wx.c1, wx.a1);
		    a[0] = -sin(alpha);
		    c[0] = cos(alpha);
		    alpha = deg(alpha);

		    beta = atan_2(r * wx.c2, wx.a2);
		    a[1] = -sin(beta);
		    c[1] = cos(beta);
		    beta = deg(beta);

                    text_slant[0] = alpha;
                    text_slant[1] = beta;
                    text_slant[2] = - (90.0 + alpha - beta);
                    text_slant[3] = 90.0 + alpha - beta;

                    /* save linetype, text alignment, text font,
		       character-up vector and clipping indicator */

                    GQLN (&errind, &ltype);
                    GQTXAL (&errind, &halign, &valign);
                    GQTXFP (&errind, &font, &prec);
                    GQCHUP (&errind, &chux, &chuy);
                    GQCLIP (&errind, &clsw, clrt);

                    GSCLIP (&GNCLIP);
                    GSTXFP (&font, &GSTRKP);
		    GSLN (&GLSOLI);

                    which_rep = 0;
                    anglep = angle;
                    while (wx.delta > *anglep++) which_rep++;
                    anglep = angle;
                    while (wx.phi > *anglep++) which_rep += 4;

		    if (*z_tick != 0)
			{
                        tick = *t_size * (y_max - y_min) / (vp[3] - vp[2]);

                        minor_tick = y_log(y_lin(*y_org, NIL) + tick);
                        major_tick = y_log(y_lin(*y_org, NIL) + 2. * tick);
                        y_label = y_log(y_lin(*y_org, NIL) + 3. * tick);

                        /* set text alignment */

                        if (y_lin(*y_org, NIL) <= 
                            (y_lin(y_min, NIL) + y_lin(y_max, NIL)) / 2.)
                            {
                            GSTXAL (&GARITE, &GAHALF);

                            if (tick > 0)
                                y_label = y_log(y_lin(*y_org, NIL) - tick);
                            }
                        else
                            {
                            GSTXAL (&GALEFT, &GAHALF);

                            if (tick < 0)
                                y_label = y_log(y_lin(*y_org, NIL) - tick);
                            }

                        rep = rep_table[which_rep][2];

			gus_set_text_slant (&text_slant[axes_rep[rep][0]]);
			GSCHUP (&a[axes_rep[rep][1]], &c[axes_rep[rep][2]]);

			if (option_z_log IN lx.scale_options)
			    {
			    z0 = pow(10.0, (double)gauss(log10(z_min)));

			    i = ipred ((float) (z_min / z0));
			    zi = z0 + i * z0;
			    decade = 0;

			    /* draw Z-axis */

			    start_pline (x_org, y_org, &z_min, NIL);

			    while ((zi <= z_max) && (!cli_b_abort))
				{
				pline (x_org, y_org, &zi, NIL);

				if (i == 0)
				    {
				    yi = major_tick;
				    if ((*major_z > 0) && (zi != *z_org))
				      if (decade % *major_z == 0)
					{
					if (*z_tick > 1)
					    {
					    exponent = iround(log10(zi));

					    strcpy (text, "10**{");
					    str_dec (expn, exponent);
					    strcat (text, expn);
					    strcat (text, "}");

					    text_3d (*x_org, y_label, zi, text);
					    }
					else
					    text_3d (*x_org, y_label, zi,
						str_ftoa (text, zi, 0.));
					}
				    }
				else
				    yi = minor_tick;

				if (i == 0 || *major_z == 1)
				    {
				    pline (x_org, &yi, &zi, NIL);
				    pline (x_org, y_org, &zi, NIL);
				    }

				if (i == 9)
				    {
				    z0 = z0 * 10.;
				    i = 0;
				    decade++;
				    }
				else
				    i++;

				zi = z0 + i * z0;
				}

			    pline (x_org, y_org, &z_max, NIL);

			    end_pline ();
			    }
			else
			    {
			    i = isucc ((float) (z_min / *z_tick));
			    zi = i * *z_tick;

			    /* draw Z-axis */

			    start_pline (x_org, y_org, &z_min, NIL);

			    while ((zi <= z_max) && (!cli_b_abort))
				{
				pline (x_org, y_org, &zi, NIL);

				if (*major_z != 0)
				    {
				    if (i % *major_z == 0)
					{
					yi = major_tick;
					if ((zi != *z_org) && (*major_z > 0))
					    text_3d (*x_org, y_label, zi,
						str_ftoa (text, zi, *z_tick
						* *major_z));
					}
				    else
					yi = minor_tick;
				    }
				else
				    yi = major_tick;

				pline (x_org, &yi, &zi, NIL);
				pline (x_org, y_org, &zi, NIL);

				i++;
				zi = i * *z_tick;
				}

			    if (zi > z_max)
				pline (x_org, y_org, &z_max, NIL);

			    end_pline ();
			    }
			}

		    if (*y_tick != 0)
			{
                        tick = *t_size * (x_max - x_min) / (vp[1] - vp[0]);

                        minor_tick = x_log(x_lin(*x_org, NIL) + tick);
                        major_tick = x_log(x_lin(*x_org, NIL) + 2. * tick);
                        x_label = x_log(x_lin(*x_org, NIL) + 3. * tick);

                        /* set text alignment */

                        if (x_lin(*x_org, NIL) <= 
                            (x_lin(x_min, NIL) + x_lin(x_max, NIL)) / 2.)
                            {
                            GSTXAL (&GARITE, &GAHALF);

                            if (tick > 0)
                                x_label = x_log(x_lin(*x_org, NIL) - tick);
                            }
                        else
                            {
                            GSTXAL (&GALEFT, &GAHALF);

                            if (tick < 0)
                                x_label = x_log(x_lin(*x_org, NIL) - tick);
                            }

                        rep = rep_table[which_rep][1];
                        if (rep == 0)
			    GSTXAL (&GACENT, &GATOP);

			gus_set_text_slant (&text_slant[axes_rep[rep][0]]);
			GSCHUP (&a[axes_rep[rep][1]], &c[axes_rep[rep][2]]);

			if (option_y_log IN lx.scale_options)
			    {
			    y0 = pow(10.0, (double)gauss(log10(y_min)));

			    i = ipred ((float) (y_min / y0));
			    yi = y0 + i * y0;
			    decade = 0;

			    /* draw Y-axis */

			    start_pline (x_org, &y_min, z_org, NIL);

			    while ((yi <= y_max) && (!cli_b_abort))
				{
				pline (x_org, &yi, z_org, NIL);

				if (i == 0)
				    {
				    xi = major_tick;
				    if ((*major_y > 0) && (yi != *y_org))
				      if (decade % *major_y == 0)
					{
					if (*y_tick > 1)
					    {
					    exponent = iround(log10(yi));

					    strcpy (text, "10**{");
					    str_dec (expn, exponent);
					    strcat (text, expn);
					    strcat (text, "}");

					    text_3d (x_label, yi, *z_org, text);
					    }
					else
					    text_3d (x_label, yi, *z_org,
						str_ftoa (text, yi, 0.));
					}
				    }
				else
				    xi = minor_tick;

				if (i == 0 || *major_y == 1)
				    {
				    pline (&xi, &yi, z_org, NIL);
				    pline (x_org, &yi, z_org, NIL);
				    }

				if (i == 9)
				    {
				    y0 = y0 * 10.;
				    i = 0;
				    decade++;
				    }
				else
				    i++;

				yi = y0 + i * y0;
				}

			    pline (x_org, &y_max, z_org, NIL);

			    end_pline ();
			    }
			else
			    {
			    i = isucc ((float) (y_min / *y_tick));
			    yi = i * *y_tick;

			    /* draw Y-axis */

			    start_pline (x_org, &y_min, z_org, NIL);

			    while ((yi <= y_max) && (!cli_b_abort))
				{
				pline (x_org, &yi, z_org, NIL);

				if (*major_y != 0)
				    {
				    if (i % *major_y == 0)
					{
					xi = major_tick;
					if ((yi != *y_org) && (*major_y > 0))
					    text_3d (x_label, yi, *z_org,
						str_ftoa (text, yi, *y_tick
						* *major_y));
					}
				    else
					xi = minor_tick;
				    }
				else
				    xi = major_tick;

				pline (&xi, &yi, z_org, NIL);
				pline (x_org, &yi, z_org, NIL);

				i++;
				yi = i * *y_tick;
				}

			    if (yi > y_max)
				pline (x_org, &y_max, z_org, NIL);

			    end_pline ();
			    }
			}

		    if (*x_tick != 0)
			{
                        tick = *t_size * (y_max - y_min) / (vp[3] - vp[2]);

                        minor_tick = y_log(y_lin(*y_org, NIL) + tick);
                        major_tick = y_log(y_lin(*y_org, NIL) + 2. * tick);
                        y_label = y_log(y_lin(*y_org, NIL) + 3. * tick);

                        /* set text alignment */

                        if (y_lin(*y_org, NIL) <= 
                            (y_lin(y_min, NIL) + y_lin(y_max, NIL)) / 2.)
                            {
                            GSTXAL (&GARITE, &GAHALF);

                            if (tick > 0)
                                y_label = y_log(y_lin(*y_org, NIL) - tick);
                            }
                        else
                            {
                            GSTXAL (&GALEFT, &GAHALF);

                            if (tick < 0)
                                y_label = y_log(y_lin(*y_org, NIL) - tick);
                            }

                        rep = rep_table[which_rep][0];
                        if (rep == 2)
			    GSTXAL (&GACENT, &GATOP);

			gus_set_text_slant (&text_slant[axes_rep[rep][0]]);
			GSCHUP (&a[axes_rep[rep][1]], &c[axes_rep[rep][2]]);

			if (option_x_log IN lx.scale_options)
			    {
			    x0 = pow(10.0, (double)gauss(log10(x_min)));

			    i = ipred ((float) (x_min / x0));
			    xi = x0 + i * x0;
			    decade = 0;

			    /* draw X-axis */

			    start_pline (&x_min, y_org, z_org, NIL);

			    while ((xi <= x_max) && (!cli_b_abort))
				{
				pline (&xi, y_org, z_org, NIL);

				if (i == 0)
				    {
				    yi = major_tick;
				    if ((*major_x > 0) && (xi != *x_org))
				      if (decade % *major_x == 0)
					{
					if (*x_tick > 1)
					    {
					    exponent = iround(log10(xi));

					    strcpy (text, "10**{");
					    str_dec (expn, exponent);
					    strcat (text, expn);
					    strcat (text, "}");

					    text_3d (xi, y_label, *z_org, text);
					    }
					else
					    text_3d (xi, y_label, *z_org,
						str_ftoa (text, xi, 0.));
					}
				    }
				else
				    yi = minor_tick;

				if (i == 0 || *major_x == 1)
				    {
				    pline (&xi, &yi, z_org, NIL);
				    pline (&xi, y_org, z_org, NIL);
				    }

				if (i == 9)
				    {
				    x0 = x0 * 10.;
				    i = 0;
				    decade++;
				    }
				else
				    i++;

				xi = x0 + i * x0;
				}

			    pline (&x_max, y_org, z_org, NIL);

			    end_pline ();
			    }
			else
			    {
			    i = isucc ((float) (x_min / *x_tick));
			    xi = i* *x_tick;

			    /* draw X-axis */

			    start_pline (&x_min, y_org, z_org, NIL);

			    while ((xi <= x_max) && (!cli_b_abort))
				{
				pline (&xi, y_org, z_org, NIL);

				if (*major_x != 0)
				    {
				    if (i % *major_x == 0)
					{
					yi = major_tick;
					if ((xi != *x_org) && (*major_x > 0))
					    text_3d (xi, y_label, *z_org,
						str_ftoa (text, xi, *x_tick
						* *major_x));
					}
				    else
					yi = minor_tick;
				    }
				else
				    yi = major_tick;

				pline (&xi, &yi, z_org, NIL);
				pline (&xi, y_org, z_org, NIL);

				i++;
				xi = i * *x_tick;
				}

			    if (xi > x_max)
				pline (&x_max, y_org, z_org, NIL);

			    end_pline ();
			    }
			}

		    slant = 0;
                    gus_set_text_slant (&slant);

                    /* restore linetype, text alignment, text font,
                       character-up vector and clipping indicator */

                    GSLN (&ltype);
                    GSTXAL (&halign, &valign);
                    GSTXFP (&font, &prec);
                    GSCHUP (&chux, &chuy);
                    GSCLIP (&clsw);

                    stat = gus__normal;
                    }
                else
                    stat = gus__orgoutwin;  /* origin outside current window */
                }
            }
        else
            /* GKS not in proper state. GKS must be either in the state WSAC 
               or SGOP */

            stat = gus__notact;
        }

    if (present(status))
        *status = stat;
    else
        if (!odd (stat))
            gus_signal (stat, "AXES_3D");

    return (stat);
}



int gus_titles_3d (char *x_title, char *y_title, char *z_title, int *status)

/*
 * FUNCTIONAL DESCRIPTION:
 *
 *	Annotate three-dimensional co-ordinate axes.
 *
 * FORMAL ARGUMENT(S):
 *
 *	X_TITLE, Y_TITLE
 *	Z_TITLE		Title strings for the three axes
 *	STATUS		Optional status value
 *
 * IMPLICIT INPUTS:
 *
 *	NONE
 *
 * IMPLICIT OUTPUTS:
 *
 *	NONE
 *
 * COMPLETION CODES:
 *
 *	GUS__NORNAL	normal successful completion
 *	GUS__NOTACT	GKS not in proper state. GKS must be either in the
 *			state WSAC or SGOP
 *	GUS__INVWINLIM	cannot apply logarithmic transformation to current
 *			window
 *
 * SIDE EFFECTS:
 *
 *	NONE
 *
 */

{
    int stat, opsta, errind, tnr;
    int halign, valign, clsw, font, prec;
    float chux, chuy;

    float clrt[4], wn[4], vp[4];
    float x_min, x_max, y_min, y_max, z_min, z_max;
    float x_rel, y_rel, z_rel, x, y, z;

    float r, t, alpha, beta;
    float a[2], c[2];

    float slant, text_slant[4];
    int *anglep, which_rep, rep;

    float x_2d, y_2d, x_2d_max, y_2d_max;
    float x_angle, y_angle;
    float x_mid_x, x_mid_y, y_mid_x, y_mid_y;
    float a1, a2, c1, c2, c3, aa, cc;
    float xr, yr, zr;

    int flip_x, flip_y, flip_z;

    /* inquire and test GKS operating state */

    GQOPS (&opsta);

    if (opsta >= GWSAC)
        {
        gus_set_scale (&lx.scale_options, &stat);

        if (odd(stat))
            {

            /* inquire current normalization transformation */

            GQCNTN (&errind, &tnr);
            GQNT (&tnr, &errind, wn, vp);

            x_min = wn[0];
            x_max = wn[1];
            y_min = wn[2];
            y_max = wn[3];

	    z_min = wx.z_min;
	    z_max = wx.z_max;

	    r = (x_max - x_min) / (y_max - y_min) * (vp[3] - vp[2]) / 
		(vp[1] - vp[0]);

	    alpha = atan_2(r * wx.c1, wx.a1);
	    a[0] = -sin(alpha);
	    c[0] = cos(alpha);
	    alpha = deg(alpha);

	    beta = atan_2(r * wx.c2, wx.a2);
	    a[1] = -sin(beta);
	    c[1] = cos(beta);
	    beta = deg(beta);

            text_slant[0] = alpha;
            text_slant[1] = beta;
            text_slant[2] = - (90.0 + alpha - beta);
            text_slant[3] = 90.0 + alpha - beta;

            /* save text alignment, text font, character-up vector and
	       clipping indicator */

            GQTXAL (&errind, &halign, &valign);
            GQTXFP (&errind, &font, &prec);
            GQCHUP (&errind, &chux, &chuy);
            GQCLIP (&errind, &clsw, clrt);

            GSTXFP (&font, &GSTRKP);
            GSCLIP (&GNCLIP);

            which_rep = 0;
            anglep = angle;
            while (wx.delta > *anglep++) which_rep++;
            anglep = angle;
            while (wx.phi > *anglep++) which_rep += 4;

            r = arc(wx.phi);
            a1 = (float) cos((double) r); 
            a2 = (float) sin((double) r); 
            aa = a1 + a2;
            a1 = a1 / aa;
            a2 = a2 / aa;            

            t = arc(wx.delta);
            c1 = (float) ((pow(cos((double) r), 2.) - 1) * tan((double)t/2.));
            c2 = (float) (-(pow(sin((double) r), 2.) - 1) * tan((double)t/2.));
            c3 = (float) cos((double) t);
            cc = (c2 + c3 - c1);
            c1 = c1 / cc;
            c2 = c2 / cc;
            c3 = c3 / cc;
    
            a1 = (float) fabs((double) a1) * (vp[1] - vp[0]);
            a2 = (float) fabs((double) a2) * (vp[1] - vp[0]);
            c1 = (float) fabs((double) c1) * (vp[3] - vp[2]);
            c2 = (float) fabs((double) c2) * (vp[3] - vp[2]);
            c3 = (float) fabs((double) c3) * (vp[3] - vp[2]);

            x_mid_x = vp[0] + a1/2.;
            x_mid_y = vp[2] + c1/2.;
            y_mid_x = 1 - vp[1] + a2/2.;
            y_mid_y = vp[2] + c2/2.;

            x_2d_max = (float) sqrt(y_mid_x*y_mid_x + y_mid_y*y_mid_y);
            y_2d_max = (float) sqrt(x_mid_x*x_mid_x + x_mid_y*x_mid_y);
            
            x_angle = atan_2(a1, c1);
            x_2d = y_mid_y / (float) cos((double) x_angle);

            if (x_2d > x_2d_max)
		{
                x_angle = PI/2. - x_angle;
                x_2d = y_mid_x / (float) cos((double) x_angle);
		}

            xr = x_2d / ( 2 * (float) sqrt(pow((double) c1, 2.) +
		pow((double) a1, 2.)));
          
            y_angle = atan_2(c2, a2);
            y_2d = x_mid_x / (float) cos((double) y_angle);

            if (y_2d > y_2d_max)
		{
		y_angle = PI/2. - y_angle;
		y_2d = x_mid_y / (float) cos((double) y_angle);
		}

            if (wx.phi + wx.delta != 0)
		yr = y_2d / ( 2 * (float) sqrt(pow((double) c2, 2.) +
		    pow((double) a2, 2.)));
            else
		yr = 0;

 	    x_rel = xr * (x_lin(x_max, NIL) - x_lin(x_min, NIL));
 	    y_rel = yr * (y_lin(y_max, NIL) - y_lin(y_min, NIL));

            flip_x = option_flip_x IN lx.scale_options;
            flip_y = option_flip_y IN lx.scale_options;
            flip_z = option_flip_z IN lx.scale_options;

	    if (*x_title)
		{
		rep = rep_table[which_rep][1];

		x = x_log(0.5 * (x_lin(x_min, NIL) + x_lin(x_max, NIL)));
                if (rep == 0)
		    {
		    GSTXAL (&GACENT, &GATOP);

		    rep = rep_table[which_rep][0];

                    if (flip_y)
                        y = y_max;
                    else
			y = y_min;

                    zr = x_mid_y / (2 * c3);
 	            z_rel = zr * (z_lin(z_max, NIL) - z_lin(z_min, NIL));

                    if (flip_z)
    		        z = z_log(z_lin(z_max, NIL) + z_rel);
                    else
			z = z_log(z_lin(z_min, NIL) - z_rel);
		    }
		else
		    {
                    if (flip_y)
			y = y_log(y_lin(y_max, NIL) + y_rel); 
		    else
			y = y_log(y_lin(y_min, NIL) - y_rel); 

                    if (flip_z)
			z = z_max;
                    else
			z = z_min;

		    GSTXAL (&GACENT, &GAHALF);
		    }

		gus_set_text_slant (&text_slant[axes_rep[rep][0]]);
		GSCHUP (&a[axes_rep[rep][1]], &c[axes_rep[rep][2]]);
    
		text_3d (x, y, z, x_title);
		}

	    if (*y_title)
		{
		rep = rep_table[which_rep][0];

		y = y_log(0.5 * (y_lin(y_min, NIL) + y_lin(y_max, NIL)));
                if (rep == 2)
		    {
		    GSTXAL (&GACENT, &GATOP);

		    rep = rep_table[which_rep][1];

		    if (flip_x)
			x = x_min;
                    else 
			x = x_max; 

                    zr = y_mid_y / (2 * c3);
 	            z_rel = zr * (z_lin(z_max, NIL) - z_lin(z_min, NIL));

                    if (flip_z)
			z = z_log(z_lin(z_max, NIL) + z_rel);
		    else
			z = z_log(z_lin(z_min, NIL) - z_rel);
		    }
		else
		    {
                    if (flip_x)
			x = x_log(x_lin(x_min, NIL) - x_rel);
                    else 
			x = x_log(x_lin(x_max, NIL) + x_rel);

                    if (flip_z) 
			z = z_max;
                    else
			z = z_min;

		    GSTXAL (&GACENT, &GAHALF);
		    }

		gus_set_text_slant (&text_slant[axes_rep[rep][0]]);
		GSCHUP (&a[axes_rep[rep][1]], &c[axes_rep[rep][2]]);

		text_3d (x, y, z, y_title);
		}

	    if (*z_title)
		{
		rep = rep_table[which_rep][2];

		gus_set_text_slant (&text_slant[axes_rep[rep][0]]);
		GSCHUP (&a[axes_rep[rep][1]], &c[axes_rep[rep][2]]);

                if (flip_x)
		    x = x_max;
                else
		    x = x_min;

                if (flip_y)
		    y = y_max;
                else
		    y = y_min;

                zr = (1 - (c1 +  c3 + vp[2])) / (2 * c3);
 	        z_rel = zr * (z_lin(z_max, NIL) - z_lin(z_min, NIL)); 

                if (flip_z)
		    z = z_log(z_lin(z_min, NIL) - 0.5 * z_rel);
                else
		    z = z_log(z_lin(z_max, NIL) + 0.5 * z_rel);

		GSTXAL (&GACENT, &GABASE);

		text_3d (x, y, z, z_title);
		}

	    slant = 0;
            gus_set_text_slant (&slant);

            /* restore text alignment, text font, character-up vector and
	       clipping indicator */

            GSTXAL (&halign, &valign);
            GSTXFP (&font, &prec);
            GSCHUP (&chux, &chuy);
            GSCLIP (&clsw);

            stat = gus__normal;
            }
        else
            /* GKS not in proper state. GKS must be either in the state WSAC 
               or SGOP */

            stat = gus__notact;
        }

    if (present(status))
        *status = stat;
    else
        if (!odd (stat))
            gus_signal (stat, "TITLES_3D");

    return (stat);
}



#define iround(x) (int)((x) + 0.5)

static void init_hlr (void)
{
    register int sign, i, j, x1, x2;
    register float *hide, a, b, m;
    float x[3], y[3], z[3], yj;
    float eps;

    eps = (lx.y_max - lx.y_min) * 1E-3;
    
    for (i = 0; i <= resolution_x; i++)
	{
	hlr.ymin[i] = -max_real;
	hlr.ymax[i] =  max_real;
	}

    for (sign = -1; sign <= 1; sign += 2)
	{
	if (sign == 1)
	    {
	    hide = hlr.ymin;
	    x[0] = hlr.x0;  y[0] = hlr.y0;  z[0] = hlr.z0;
	    x[1] = hlr.x1;  y[1] = hlr.y0;  z[1] = hlr.z0;
	    x[2] = hlr.x1;  y[2] = hlr.y1;  z[2] = hlr.z0;
	    }
	else
	    {
	    hide = hlr.ymax;
	    x[0] = hlr.x0;  y[0] = hlr.y0;  z[0] = hlr.z1;
	    x[1] = hlr.x0;  y[1] = hlr.y1;  z[1] = hlr.z1;
	    x[2] = hlr.x1;  y[2] = hlr.y1;  z[2] = hlr.z1;
	    }

	for (i = 0; i < 3; i++)
	    apply_world_xform (x+i, y+i, z+i);

	if (hlr.xmax > hlr.xmin)
	    {
	    a = resolution_x / (hlr.xmax - hlr.xmin);
	    b = -(hlr.xmin * a);
	    }
	else
	    {
	    a = 1;
	    b = 0;
	    }

	x1 = iround(a * x[0] + b);
	if (x1 < 0) x1 = 0;
	x2 = x1;

	for (i = 1; i < 3; i++)
	    {
	    x1 = x2;
	    x2 = iround(a * x[i] + b);

	    if (x1 <= x2)
		{
		if (x1 != x2)
		    m = (y[i] - y[i-1]) / (x2 - x1);

		for (j = x1; j <= x2; j++)
		    {
		    if (j == x2)
			yj = y[i];
		    else
			yj = y[i-1] + m * (j-x1);

		    hide[j] = yj - sign * eps;
		    }
                }
            }
        }
}


static void pline_hlr (int n, float *x, float *y, float *z)

/*
 *   DESCRIPTION:
 *
 *	Three-dimensional polyline routine with hidden-line removal
 *
 *   AUTHOR:
 *
 *	J.Heinen
 *
 *   DATE:
 *
 *	20-OCT-1995
 */

{
    register int i, j, x1, x2;
    register BOOL visible, draw;
    register float *hide, a, b, c, m;

    int saved_scale_options;
    float xj, yj;

    if (hlr.buf == NULL)
	{
        hlr.buf = (float *) gmalloc(sizeof(float) * (resolution_x + 1) * 2);
	hlr.ymin = hlr.buf;
	hlr.ymax = hlr.buf + resolution_x + 1;
	}

    if (hlr.sign == 1)
	hide = hlr.ymin;
    else
	hide = hlr.ymax;

    for (i = 0; i < n; i++)
	apply_world_xform (x+i, y+i, z+i);

    draw = !hlr.initialize || hlr.sign > 0;
    visible = FALSE;

    saved_scale_options = lx.scale_options;
    lx.scale_options = 0;

    if (hlr.xmax > hlr.xmin)
	{
	a = resolution_x / (hlr.xmax - hlr.xmin);
	b = -(hlr.xmin * a);
	}
    else
	{
	a = 1;
	b = 0;
	}
    c = 1.0 / a;

    x1 = iround(a * x[0] + b);
    if (x1 < 0) x1 = 0;
    x2 = x1;

    if (hlr.initialize)
        {
	init_hlr ();

        if (hlr.ymin[x1] <= y[0] && y[0] <= hlr.ymax[x1])
	    {
	    hide[x1] = y[0];

	    if (draw)
		start_pline (x, y, NIL, NIL);

	    visible = TRUE;
            }
        }

    for (i = 1; i < n; i++)
        {
        x1 = x2;
        x2 = iround(a * x[i] + b);

        if (x1 <= x2)
            {
            if (x1 != x2)
                m = (y[i] - y[i-1]) / (x2 - x1);

	    for (j = x1; j <= x2; j++)
                {
                if (j == x2)
                    yj = y[i];
                else
                    yj = y[i-1] + m * (j-x1);

	        if (hlr.ymin[j] <= yj && yj <= hlr.ymax[j])
                    {
                    if (!visible)
                        {
                        if (draw)
			    {
			    xj = c * j + hlr.xmin;

                            start_pline (&xj, &yj, NIL, NIL);
			    }

                        visible = TRUE;
                        }
                    }
                else
                    {
                    if (visible)
                        {
                        if (draw)
                            {
			    xj = c * j + hlr.xmin;

			    pline (&xj, &yj, NIL, NIL);
                            end_pline ();
                            }

                        visible = FALSE;
                        }
                    }

		if ((yj - hide[j]) * hlr.sign > 0)
                    hide[j] = yj;
                }
            }

        if (visible && draw)
	    pline (&x[i], &y[i], NIL, NIL);
        }

    if (visible && draw)
        end_pline ();

    lx.scale_options = saved_scale_options;
}

#undef iround
	

static void get_intensity (float *fx, float *fy, float *fz, float *light_source,
    float *intensity)
{
    int k;
    float max_x, max_y, max_z, min_x, min_y, min_z, norm_1, norm_2; 
    float center[4], normal[4], negated[4], oddnormal[4], negated_norm[4];

    min_x = max_x = fx[0];
    min_y = max_y = fy[0];
    min_z = max_z = fz[0];

    for (k=1; k<4; k++) {
        if (fx[k] > max_x)
	    max_x = fx[k];
        else if (fx[k] < min_x)
            min_x = fx[k]; 
        if (fy[k] > max_y)
	    max_y = fy[k];
        else if (fy[k] < min_y)
            min_y = fy[k]; 
        if (fz[k] > max_z)
	    max_z = fz[k];
        else if (fz[k] < min_z)
            min_z = fz[k]; 
 	}

    center[0] = (max_x + min_x)/2;
    center[1] = (max_y + min_y)/2;
    center[2] = (max_z + min_z)/2;

    for (k=0; k<3; k++)
	negated[k] = light_source[k] - center[k];

    norm_1 = (float) sqrt(negated[0]*negated[0]+negated[1]*negated[1]+
	negated[2]*negated[2]);

    for (k=0; k<3; k++)
	negated_norm[k] = negated[k]/norm_1;

    normal[0] = ((fy[1]-fy[0])*(fz[2]-fz[0])-(fz[1]-fz[0])*(fy[2]-fy[0]))+
		((fy[2]-fy[1])*(fz[3]-fz[1])-(fz[2]-fz[1])*(fy[3]-fy[1]));
    normal[1] = ((fz[1]-fz[0])*(fx[2]-fx[0])-(fx[1]-fx[0])*(fz[2]-fz[0]))+
		((fz[2]-fz[1])*(fx[3]-fx[1])-(fx[2]-fx[1])*(fz[3]-fz[1]));
    normal[2] = ((fx[1]-fx[0])*(fy[2]-fy[0])-(fy[1]-fy[0])*(fx[2]-fx[0]))+
		((fx[2]-fx[1])*(fy[3]-fy[1])-(fy[2]-fy[1])*(fx[3]-fx[1]));
    normal[3] = 1;

    norm_2 = (float) sqrt(normal[0]*normal[0]+normal[1]*normal[1]+
	normal[2]*normal[2]);

    for (k=0; k<3; k++)
	oddnormal[k] = normal[k]/norm_2;

    *intensity = (oddnormal[0]*negated_norm[0]+oddnormal[1]*negated_norm[1]+
	oddnormal[2]*negated_norm[2]) * 0.8 + 0.2;
}



int gus_surface (int *nx, int *ny, float *px, float *py,
    float (*fz)(int *, int *), gus_surface_option *option, int *status)

/*
 * FUNCTIONAL DESCRIPTION:
 *
 *	Draw a three-dimensional surface.
 *
 * FORMAL ARGUMENT(S):
 *
 *	NX		Number of points per line
 *	NY		Number of lines
 *	PX,PY		Co-ordinates of points in world co-ordinates
 *	FZ		Function
 *	OPTION		Display option
 *	STATUS		Optional status value
 *
 * IMPLICIT INPUTS:
 *
 *	NONE
 *
 * IMPLICIT OUTPUTS:
 *
 *	NONE
 *
 * COMPLETION CODES:
 *
 *	GUS__NORNAL	normal successful completion
 *	GUS__NOTACT	GKS not in proper state. GKS must be either in the
 *			state WSAC or SGOP
 *	GUS__INVWINLIM	cannot apply logarithmic transformation to current
 *			window
 *	GUS__INVNUMPNT	invalid number of points
 *	GUS__INVPNT	point co-ordinates must be greater than 0
 *	GUS__NOTSORASC	points ordinates not sorted in ascending order
 *
 * SIDE EFFECTS:
 *
 *	NONE
 *
 */

{
    int stat, opsta, errind, ltype, coli, int_style;

    int i, ii, j, jj, k;
    int color;

    float *xn, *yn, *zn, *x, *y, *z;
    float facex[4], facey[4], facez[4], intensity, meanz;
    float a, b, c, d, e, f;

    float ymin, ymax, zmin, zmax;

    BOOL flip_x, flip_y, flip_z;
    int np;

    int *colia, w, h, *ca, dwk, *wk1, *wk2;

    static float light_source[3] = { 0.5, -1, 2 };

    if ((*nx > 0) && (*ny > 0))
        {

        /* inquire and test GKS operating state */

        GQOPS (&opsta);

        if (opsta >= GWSAC)
            {
            gus_set_scale (&lx.scale_options, &stat);

            if (odd(stat))
                {

                /* be sure that points ordinates are sorted in ascending 
                   order */

                for (i = 1; i < *nx; i++)
                    if (px[i-1] >= px[i])
                        stat = gus__notsorasc;

                for (j = 1; j < *ny; j++)
                    if (py[j-1] >= py[j])
                        stat = gus__notsorasc;

                if (odd(stat))
                    {
		    a = 1.0 / (lx.x_max - lx.x_min);
		    b = - (lx.x_min * a);
		    c = 1.0 / (lx.y_max - lx.y_min);
		    d = - (lx.y_min * c);
		    e = 1.0 / (wx.z_max - wx.z_min);
		    f = - (wx.z_min * e);

                    /* save linetype, fill area interior style and color 
                       index */

                    GQLN (&errind, &ltype);
                    GQFAIS (&errind, &int_style);
                    GQFACI (&errind, &coli);

                    k = sizeof(float) * max((*nx + *ny) * 2, *ny * 3);
                    xn = (float *) gmalloc(k);
                    yn = (float *) gmalloc(k);
                    zn = (float *) gmalloc(k);
                    x  = (float *) gmalloc (*nx * sizeof (float));
                    y  = (float *) gmalloc (*ny * sizeof (float));
                    z  = (float *) gmalloc (*nx * *ny * sizeof (float));

		    flip_x = option_flip_x IN lx.scale_options;
                    for (i = 0; i < *nx; i++)
			x[i] = x_lin(px[flip_x ? *nx-1-i : i], NIL);

		    flip_y = option_flip_y IN lx.scale_options;
                    for (j = 0; j < *ny; j++)
			y[j] = y_lin(py[flip_y ? *ny-1-j : j], NIL);

                    k = 0;
                    for (j = 1; j <= (*ny); j++) {
			jj = flip_y ? *ny-j+1 : j;
                        for (i = 1; i <= (*nx); i++) {
			    ii = flip_x ? *nx-i+1 : i;
                            z[k++] = z_lin(fz(&ii, &jj), NIL);
			    }
			}

                    hlr.x0 = x[0];     hlr.x1 = x[*nx-1];
                    hlr.y0 = y[0];     hlr.y1 = y[*ny-1];
                    hlr.z0 = wx.z_min; hlr.z1 = wx.z_max;

		    hlr.xmin = x[0];   hlr.xmax = x[*nx-1];
                        ymin = y[0];       ymax = y[*ny-1];
                        zmin = wx.z_min;   zmax = wx.z_max;

                    apply_world_xform (&hlr.xmin, &ymin, &zmin);
                    apply_world_xform (&hlr.xmax, &ymax, &zmax);

		    flip_z = option_flip_z IN lx.scale_options;
		    GSLN (flip_z ? &GLDOT : &GLSOLI);
		    
#define Z(x, y) z[(x)+(*nx)*(y)]

                    hlr.sign = -1;

                    do
                        {
                        hlr.sign = -hlr.sign;

                        switch (*option) {

                            case option_lines :
                              {
                              j = 0;
                              hlr.initialize = TRUE;

                              while (j < *ny && !cli_b_abort)
                                  { 
				  k = 0;

				  if (j > 0)
				      {
				      xn[k] = x[0];
				      yn[k] = y[j-1];
				      zn[k] = Z(0,j-1);
				      k++;
				      }

				  for (i = 0; i < *nx; i++)
				      {
				      xn[k] = x[i];
				      yn[k] = y[j];
				      zn[k] = Z(i,j);
				      k++;
				      }

				  if (j == 0)

				      for (i = 1; i < *ny; i++)
					  {
					  xn[k] = x[*nx-1];
					  yn[k] = y[i];
					  zn[k] = Z(*nx-1,i);
					  k++;
					  }

				  pline_hlr (k, xn, yn, zn);

                                  hlr.initialize = FALSE;
				  j++;
                                  }

                              break;
                              }

                            case option_mesh :
                              {
                              k = 0;

                              for (i = 0; i < *nx; i++)
                                  {
				  xn[k] = x[i];
				  yn[k] = y[0];
				  zn[k] = Z(i,0);
				  k++;
                                  }

                              for (j = 1; j < *ny; j++)
                                  {
				  xn[k] = x[*nx-1];
				  yn[k] = y[j];
				  zn[k] = Z(*nx-1,j);
				  k++;
                                  }

                              hlr.initialize = TRUE;

			      pline_hlr (k, xn, yn, zn);

                              i = *nx-1;

                              while (i > 0 && !cli_b_abort)
                                  {
                                  k = 0;

				  for (j = 1; j < *ny; j++)
				      {
				      xn[k] = x[i-1];
				      yn[k] = y[j-1];
				      zn[k] = Z(i-1,j-1);
                                      k++;

				      xn[k] = x[i-1];
				      yn[k] = y[j];
				      zn[k] = Z(i-1,j);
                                      k++;

				      xn[k] = x[i];
				      yn[k] = y[j];
				      zn[k] = Z(i,j);
                                      k++;
				      }

				  hlr.initialize = FALSE;

				  pline_hlr (k, xn, yn, zn);

				  i--;
                                  }

                              break;
                              }

                            case option_filled_mesh :
                            case option_z_shaded_mesh :
                            case option_colored_mesh :
                            case option_shaded_mesh :
                              {
                              j = *ny-1;

                              GSFAIS (&GSOLID);

                              while (j > 0 && !cli_b_abort)
                                  {
 				  for (i = 1; i < *nx; i++)
				      {
				      xn[0] = x[i-1];
				      yn[0] = y[j];
				      zn[0] = Z(i-1,j);

				      xn[1] = x[i-1];
				      yn[1] = y[j-1];
				      zn[1] = Z(i-1,j-1);

				      xn[2] = x[i];
				      yn[2] = y[j-1];
				      zn[2] = Z(i,j-1);

				      xn[3] = x[i];
				      yn[3] = y[j];
				      zn[3] = Z(i,j);

				      xn[4] = xn[0];
				      yn[4] = yn[0];
				      zn[4] = zn[0];

				      if (*option == option_shaded_mesh)
					  {
					  for (k = 0; k < 4; k++) {
				              facex[k] = a * xn[k] + b;
					      facey[k] = c * yn[k] + d;
					      facez[k] = e * zn[k] + f;
					      }
                                          get_intensity (facex, facey, facez,
                                              light_source, &intensity);
					  }

                                      for (k = 0; k <= 4; k++)
				          apply_world_xform (xn+k, yn+k, zn+k);

				      meanz = 0.25 * (Z(i-1,j-1) + Z(i,j-1) +
					      Z(i,j) + Z(i-1,j));

				      if (*option == option_z_shaded_mesh)
					  {
                                          color = iround (meanz) + first_color;

					  if (color < first_color)
					      color = first_color;
					  else if (color > last_color)
					      color = last_color;

					  GSFACI (&color);
					  }

				      else if (*option == option_colored_mesh)
					  {
					  color = iround ((meanz - wx.z_min) /
					      (wx.z_max - wx.z_min) *
					      (last_color - first_color)) +
					      first_color;

					  if (color < first_color)
					      color = first_color;
					  else if (color > last_color)
					      color = last_color;

					  GSFACI (&color);
					  }

				      else if (*option == option_shaded_mesh)
					  {
					  color = iround (intensity *
                                              (last_color - first_color)) +
                                              first_color;

					  if (color < first_color)
					      color = first_color;
					  else if (color > last_color)
					      color = last_color;

					  GSFACI (&color);
					  }

				      np = 4;
				      GFA (&np, xn, yn);

				      if (*option == option_filled_mesh)
					  {
					  np = 5;
					  GPL (&np, xn, yn);
					  }
				      }

                                  j--;
                                  }

                              break;
                              }

			    case option_cell_array :

			      colia = (int *) gmalloc (*nx * *ny *sizeof(int));
			      k = 0;
			      for (j = 0; j < *ny; j++)
			          for (i = 0; i < *nx; i++)
			 	      {
				      if (Z(i,j) != gus_gf_missing_value)
					  {
					  color = first_color + (int) (
					      (Z(i,j) - wx.z_min) /
					      (wx.z_max - wx.z_min) *
					      (last_color - first_color));

					  if (color < first_color)
					      color = first_color;
					  else if (color > last_color)
					      color = last_color;
					  }
				      else
					  color = background;

				      colia[k++] = color;
				      }

			      w = (*nx < 256) ? *nx * (255/(*nx) + 1) - 1 :
						*nx - 1;
			      h = (*ny < 256) ? *ny * (255/(*ny) + 1) - 1 :
						*ny - 1;
			      ca = (int *) gmalloc (w*h * sizeof(int));

			      dwk = w;
			      if (h > dwk) dwk = h;
			      if (*nx > dwk) dwk = *nx;
			      if (*ny > dwk) dwk = *ny;
			      wk1 = (int *) gmalloc (dwk * sizeof(int));
			      wk2 = (int *) gmalloc (dwk * sizeof(int));

			      GPIXEL (hlr.xmin, hlr.xmax, ymin, ymax,
				  *nx, *ny, colia, w, h, ca, dwk, wk1, wk2);

			      free (wk2);
			      free (wk1);
			      free (ca);
			      free (colia);

			      break;
                            }

                        GSLN (flip_z ? &GLSOLI : &GLDOT);
                        }
                    while ((hlr.sign >= 0) && !cli_b_abort && 
                           ((int) *option <= (int) option_mesh));

#undef Z
                    free (z);
                    free (y);
                    free (x);
                    free (zn);
                    free (yn);
                    free (xn);  

                    /* restore linetype, fill area interior style and color 
		       index */

                    GSLN (&ltype);
                    GSFAIS (&int_style);
                    GSFACI (&coli);
                    }
                }
            }
        else
            /* GKS not in proper state. GKS must be either in the state WSAC
               or SGOP */

            stat = gus__notact;
        }
    else
        stat = gus__invnumpnt;  /* invalid number of points */

    if (present(status))
        *status = stat;
    else
        if (!odd(stat))
            gus_signal (stat, "SURFACE");
    
    return (stat);
}



int gus_contour (int *nx, int *ny, int *nh, float *px, float *py, float *h,
    float (*fz)(int *, int *), int *major_h, int *status)

/*
 * FUNCTIONAL DESCRIPTION:
 *
 *	Draw contours of a function of two variables whose
 *	values are specified over an n x m rectangular mesh.
 *
 * FORMAL ARGUMENT(S):
 *
 *	NX		Number of points per mesh line
 *	NY		Number of mesh lines
 *	NH		Number of contour heights
 *	PX,PY		Co-ordinates of points in world co-ordinates
 *	H		Contour heights
 *	FZ		Function
 *	MAJOR_H		Major count for contour height labels
 *	STATUS		Optional status value
 *
 * IMPLICIT INPUTS:
 *
 *	NONE
 *
 * IMPLICIT OUTPUTS:
 *
 *	NONE
 *
 * COMPLETION CODES:
 *
 *	GUS__NORNAL	normal successful completion
 *	GUS__NOTACT	GKS not in proper state. GKS must be either in the
 *			state WSAC or SGOP
 *	GUS__INVWINLIM	cannot apply logarithmic transformation to current
 *			window
 *	GUS__INVNUMPNT	invalid number of points
 *	GUS__INVPNT	point co-ordinates must be greater than 0
 *	GUS__NOTSORASC	points ordinates not sorted in ascending order
 *
 * SIDE EFFECTS:
 *
 *	NONE
 *
 */

{
    float       mmin, mmax;
    float       *cv;
    int         ncv;
    int         *bitmap;
    int         i, j, k, ii, jj;
    int         precision, max_precision;
    int         eflag;
    char        *s;
    char        buffer[80];
    int         error_ind;
    int         saved_linetype;
    int         saved_hor_align, saved_vert_align, hor_align, vert_align;
    int         stat, gks_state;
    int         rotation, tilt;
    float       saved_x_up_vec, saved_y_up_vec;
    float       char_height;


    if ((*nx > 1) && (*ny > 1))
        {

        /* inquire and test GKS operating state */

        GQOPS (&gks_state);

        if ((gks_state == GWSAC) || (gks_state == GSGOP))
            {
            gus_set_scale (&lx.scale_options, &stat);

            if (odd(stat))
                {

                /* be sure that points ordinates are sorted in ascending 
		   order */

                for (i = 1; i < *nx; i++)
                    if (px[i-1] >= px[i])
                        stat = gus__notsorasc;

                for (j = 1; j < *ny; j++)
                    if (py[j-1] >= py[j])
                        stat = gus__notsorasc;

                if (odd(stat))
                    {

                    /* Inquire workstation identifier */
                    i = 1;
                    GQACWK (&i, &error_ind, &j, &contour_vars.wkid);

                    /* Inquire current transformation */
                    contour_vars.ndc = 0;
                    GQCNTN (&error_ind, &contour_vars.tnr);
                    GQNT (&contour_vars.tnr, &error_ind, contour_vars.wn, 
                                                         contour_vars.vp);

                    contour_vars.scale_factor = 
                                   (contour_vars.vp[1] - contour_vars.vp[0])/
                                   (contour_vars.wn[1] - contour_vars.wn[0]);
                    contour_vars.aspect_ratio = 
                                   (contour_vars.vp[3] - contour_vars.vp[2])/
                                   (contour_vars.wn[3] - contour_vars.wn[2])/
                                   contour_vars.scale_factor;

                    contour_vars.xdim = *nx;
                    contour_vars.ydim = *ny;
                    contour_vars.lblmjh = *major_h;

                    GQLN (&error_ind, &saved_linetype);
                    GQCHUP (&error_ind, &saved_x_up_vec, &saved_y_up_vec);
                    GQTXAL (&error_ind, &saved_hor_align, 
                                        &saved_vert_align);

                    hor_align = GACENT;
                    vert_align = GAHALF;
                    GSTXAL (&hor_align, &vert_align);
    
                    /* Don't label any lines if a 3D-transformation */
                    /* or any scale options are in effect.          */

                    gus_inq_space (&contour_vars.zmin, &contour_vars.zmax, 
                                   &rotation, &tilt, &error_ind);

                    if ((rotation == 0) && (tilt == 90) && 
                        (contour_vars.lblmjh > 0) && (lx.scale_options == 0))
                     {
                       contour_vars.txtflg = 1;

                       /* Compute size of 'label_map' with respect to      */
                       /* viewport-size and character-height.              */
                       /* 'label_map' is used to store information about   */
                       /* previously drawn label positions to avoid        */
		       /* overlapping					   */

                       GQCHH (&error_ind, &char_height);
                       if (char_height < contour_min_chh)
                        {
                          char_height = contour_min_chh;
                        }

                       contour_vars.x_map_size = (int) (contour_map_rate * 
                               (contour_vars.vp[1] - contour_vars.vp[0]) /
                               char_height) + 2;
                       contour_vars.y_map_size = (int) (contour_map_rate *
                               (contour_vars.vp[3] - contour_vars.vp[2]) /
                               char_height) + 2;
                       contour_vars.label_map = (int *) gmalloc 
                          (contour_vars.x_map_size * contour_vars.y_map_size *
                           sizeof (int));

                       k = 0;
                       for (i = 0; i < contour_vars.x_map_size; i ++)
                          for (j = 0; j < contour_vars.y_map_size; j ++)
                           {
                             contour_vars.label_map [k] = 0;
                             k ++;
                           }
                       contour_vars.x_map_factor = 
                                   (double) (contour_vars.x_map_size-3) /
                                   (contour_vars.wn[1] - contour_vars.wn[0]);
                       contour_vars.y_map_factor = 
                                   (double) (contour_vars.y_map_size-3) /
                                   (contour_vars.wn[3] - contour_vars.wn[2]);
                     }
                    else
                     {
                       contour_vars.txtflg = 0;
                     }
 
                    mmin = max_real;
                    mmax = -max_real;
                    contour_vars.z = (float *) 
                                     gmalloc ((*nx) * (*ny) * sizeof (float));
                    k = 0;
                    for (j = 1; j <= (*ny); j ++)
                     {
                       jj = j;
                       for (i = 1; i <= (*nx); i ++)
                        {
                          ii = i;
                          contour_vars.z[k] = fz(&ii, &jj);
                          if (contour_vars.z[k] > mmax)
                           {
                             mmax = contour_vars.z[k];
                           }
                          if (contour_vars.z[k] < mmin)
                           {
                             mmin = contour_vars.z[k];
                           }
                          k ++;
                        }
                     }

                    if (*nh <= 1)
                        {
                        ncv = contour_lines;
                        cv = (float *) gmalloc (ncv * sizeof(float));
                        for (i = 0; i < ncv; i++)
                            cv[i] = mmin + (float)(i)/(ncv-1) * (mmax-mmin);
                        }
                    else
                        {
                        ncv = *nh;
                        cv = h;
                        }
 
    /*--------------------------------------------------------------------------
    / Find the maximum required precision for the labels and create the
    / appropriate format for 'sprintf'
    /-------------------------------------------------------------------------*/

                    if (contour_vars.txtflg == 1)
                    {
                      max_precision = 0;
                      eflag = 0;

                      for (i = 0; i < ncv; i++)
                        if ((contour_vars.txtflg == 1) && 
                           ((contour_vars.lblmjh == 1) || 
                           ((i % contour_vars.lblmjh) == 1)))
                        {
                          sprintf (buffer, "%g", cv[i]);
                          if ((s = (char *) strchr (buffer, '.')) != 0)
                          {
                             precision = strspn (s+1, "0123456789");
                             if (*(s+1+precision) != '\0')
                                 eflag = 1;
                             if (precision > max_precision)
                                max_precision = precision;
                          }
                        }

                      sprintf (contour_vars.lblfmt, "%%.%d%c", max_precision, 
                      	eflag ? 'e' : 'f');
                    }

                    bitmap = (int *)gmalloc ((*nx)*(*ny)*ncv*2*sizeof(int));
                    contour_vars.xmin = px[0];
                    contour_vars.ymin = py[0];
                    contour_vars.dx = px[1] - contour_vars.xmin;
                    contour_vars.dy = py[1] - contour_vars.ymin;
 
#if defined(hpux) && !defined(NAGware)
                    draw_a = (int (*)()) DRAW;
                    GCONTR (contour_vars.z, nx, nx, ny, cv, &ncv, &mmax, 
                            bitmap, &contour_vars.xmin, &contour_vars.ymin, 
                            &contour_vars.dx, &contour_vars.dy,
#if defined (__hp9000s700) || defined (__hp9000s300)
			    draw_a);
#else
			    &draw_a);
#endif /* __hp9000s700 || __hp9000s300 */
#else
                    GCONTR (contour_vars.z, nx, nx, ny, cv, &ncv, &mmax, 
                            bitmap, &contour_vars.xmin, &contour_vars.ymin, 
                            &contour_vars.dx, &contour_vars.dy, DRAW);
#endif

                    free (bitmap);
                    free (contour_vars.z);

                    if (contour_vars.txtflg == 1)
                       free (contour_vars.label_map);
                    if (cv != h)
                       free (cv);

                    /* restore GKS attributes */

                    GSLN (&saved_linetype);
                    GSCHUP (&saved_x_up_vec, &saved_y_up_vec);
                    GSTXAL (&saved_hor_align, &saved_vert_align);
                    }
                }
            }
        else
            /* GKS not in proper state. GKS must be either in the state WSAC
               or SGOP */

            stat = gus__notact;
        }
    else
        stat = gus__invnumpnt;  /* invalid number of points */

    if (present(status))
        *status = stat;
    else
        if (!odd(stat))
            gus_signal (stat, "CONTOUR");

    return (stat);
}



int gus_inq_space (float *z_min, float *z_max, int *rotation, int *tilt,
    int *status)

/*
 * FUNCTIONAL DESCRIPTION:
 *
 *	inquire three-dimensional transformation
 *
 * FORMAL ARGUMENT(S):
 *
 *	Z_MIN, Z_MAX	Z-axis limits
 *	STATUS		Optional status value
 *
 * IMPLICIT INPUTS:
 *
 *	NONE
 *
 * IMPLICIT OUTPUTS:
 *
 *	NONE
 *
 * COMPLETION CODES:
 *
 *	GUS__NORNAL	normal successful completion
 *	GUS__GKCL	GKS not in proper state. GKS must be in one of the
 *			states GKOP,WSOP,WSAC or SGOP
 *
 * SIDE EFFECTS:
 *
 *	NONE
 *
 */

{
    int stat, opsta, ignore;

    /* inquire and test GKS operating state */

    GQOPS (&opsta);

    if (opsta >= GGKOP)
        {
        gus_set_scale (&lx.scale_options, &ignore);

        *z_min = wx.z_min;
        *z_max = wx.z_max;

        *rotation = wx.phi;
        *tilt = wx.delta;

        stat = gus__normal;
        }
    else

        /* GKS not in proper state. GKS must be in one of the states GKOP,WSOP,
           WSAC or SGOP */

        stat = gus__gkcl;

    if (present(status))
        *status = stat;
    else
        if (!odd(stat))
            gus_signal (stat, "INQ_SPACE");

    return (stat);
}



float gus_tick (float *amin, float *amax)

/*
 * gus_tick - return a tick unit that evenly divides into the difference
 *	     between the minimum and maximum value
 */

{
    float tick_unit, exponent, factor;
    int n;

    if (*amax > *amin)
        {
        exponent = log10(*amax - *amin);
        n = gauss(exponent);

        factor = pow(10.0, (double)(exponent - n));

        if (factor > 5)
            tick_unit = 2;
        else

            if (factor > 2.5)
                tick_unit = 1;
            else

                if (factor > 1)
                    tick_unit = 0.5;
                else
                    tick_unit = 0.2;

        tick_unit = tick_unit * pow(10.0, (double)n);
        }
    else
        tick_unit = 0;

    return (tick_unit);
}



void gus_adjust_range (float *amin, float *amax)

/*
 * gus_adjust_range - adjust range
 */

{
    float tick;

    if (*amin == *amax)
        {
        if (*amin != 0)
            tick = pow(10.0, (double)fract(log10(fabs(*amin))));
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



int gus_autoscale (int *n, float *x, float *y,
    float *x_min, float *x_max, float *y_min, float *y_max, int *status)

/*
 * gus_autoscale - routine for automatic scaling of a pair of axis
 */

{
    int stat, i;

    if (*n > 0)
        {
        *x_min = x[0];
        *x_max = x[0];
        *y_min = y[0];
        *y_max = y[0];

        for (i = 0; i < *n; i++)
            {
            *x_min = min(*x_min, x[i]);
            *x_max = max(*x_max, x[i]);
            *y_min = min(*y_min, y[i]);
            *y_max = max(*y_max, y[i]);
            }

        if (*x_min == *x_max)
            gus_adjust_range (x_min, x_max);

        if (*y_min == *y_max)
            gus_adjust_range (y_min, y_max);

        stat = gus__normal;
        }
    else
        stat = gus__invnumpnt;  /* invalid number of points */

    if (present(status))
        *status = stat;
    else
        if (!odd(stat))
            gus_signal (stat, "AUTOSCALE");

    return (stat);
}



int gus_autoscale_3d (int *n, float *x, float *y, float *z,
    float *x_min, float *x_max, float *y_min, float *y_max,
    float *z_min, float *z_max, int *status)

/*
 * gus_autoscale - routine for automatic scaling of 3D axes
 */

{
    int stat, i;

    if (*n > 0)
        {
        *x_min = x[0];
        *x_max = x[0];
        *y_min = y[0];
        *y_max = y[0];
        *z_min = z[0];
        *z_max = z[0];

        for (i = 0; i < *n; i++)
            {
            *x_min = min(*x_min, x[i]);
            *x_max = max(*x_max, x[i]);
            *y_min = min(*y_min, y[i]);
            *y_max = max(*y_max, y[i]);
            *z_min = min(*z_min, z[i]);
            *z_max = max(*z_max, z[i]);
            }

        if (*x_min == *x_max)
            gus_adjust_range (x_min, x_max);

        if (*y_min == *y_max)
            gus_adjust_range (y_min, y_max);

        if (*z_min == *z_max)
            gus_adjust_range (z_min, z_max);

        stat = gus__normal;
        }
    else
        stat = gus__invnumpnt;  /* invalid number of points */

    if (present(status))
        *status = stat;
    else
        if (!odd(stat))
            gus_signal (stat, "AUTOSCALE_3D");

    return (stat);
}



static void update_open_ws (void)

/*
 * update_open_ws - update all open input/output workstations
 */

{
    int state, count, n, errind, ol;
    int wkid, conid, wtype, wkcat;

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



static float value (float n1, float n2, float hue)
{
    float val;

    if (hue > 360.) hue -= 360.;
    if (hue < 0.) hue += 360.;

    if (hue < 60)
        val = n1+(n2-n1)*hue/60.;
    else if (hue < 180.)
        val = n2;
    else if (hue < 240.)
        val = n1+(n2-n1)*(240.-hue)/60.;
    else
        val = n1;

    return (val);
}



static void hls_to_rgb (
    float *h, float *l, float *s, float *r, float *g, float *b)
{
    float m1,m2;

    m2 = (*l<0.5) ? (*l)*(1+*s) : *l + *s- (*l)*(*s);
    m1 = 2*(*l) - m2;

    if (*s == 0)
        { (*r)=(*g)=(*b)=(*l); }
    else
        {
        *r = value (m1, m2, *h+120.);
        *g = value (m1, m2, *h+000.);
        *b = value (m1, m2, *h-120.);
        }
    return;
}



int gus_set_colormap (gus_colormap *colormap, gus_colormode *colormode,
    int *status)

/*
 *set_colormap - set colormap
 */

{
    int stat, count, errind, opsta, ol, wkid;
    int i, j, ci;
    float r, g, b, h, l, s;
    double x;

    /* inquire and test GKS operating state */

    GQOPS (&opsta);
    if (opsta >= GWSAC)
        {
	count = 1;
        GQACWK (&count, &errind, &ol, &wkid);

        for (count = 1; count <= ol; count++)
            {
            GQACWK (&count, &errind, &ol, &wkid);

	    if (*colormode == colormode_normal)
		i = 0;
	    else
		i = last_color-first_color+1;

	    for (ci = first_color; ci <= last_color; ci++)
	        {
		x = (double)i / (double)(last_color-first_color+1);

	        switch (*colormap)
		    {
		    case colormap_uniform:
		    case colormap_temperature:
			if (*colormap == colormap_uniform)
			    h = i*360.0/(last_color-first_color+1)-120;
			else
			    h = 270-i*300.0/(last_color-first_color+1);

	        	l = 0.5;
	        	s = 0.75;

			hls_to_rgb (&h,&l,&s, &r,&g,&b);
			break;

		    case colormap_grayscale:
			r = x;
			g = x;
			b = x;
			break;

		    case colormap_glowing:
			r = pow (x, 1./4.);
			g = x;
			b = pow (x, 4.);
			break;

		    case colormap_rainbow:
		    case colormap_flame:
			if (x < 0.125)
			    r = 4. * (x + 0.125);
			else if (x < 0.375)
			    r = 1.;
			else if (x < 0.625)
			    r = 4. * (0.625 - x);
			else
			    r = 0;

			if (x < 0.125)
			    g = 0;
			else if (x < 0.375)
			    g = 4. * (x - 0.125);
			else if (x < 0.625)
			    g = 1.;
			else if (x < 0.875)
			    g = 4. * (0.875 - x);
			else
			    g = 0;
		    
			if (x < 0.375)
			    b = 0;
			else if (x < 0.625)
			    b = 4. * (x - 0.375);
			else if (x < 0.875)
			    b = 1.;
			else
			    b = 4. * (1.125 - x);

		        if (*colormap == colormap_flame)
			    {
			    r = 1.0 - r; g = 1.0 - g; b = 1.0 - b;
			    }
			break;

		    case colormap_geologic:
			if (x < 0.333333) 
			    r = 0.333333 - x;
			else if (x < 0.666666)
			    r = 3. * (x - 0.333333);
			else
			    r = 1. - (x - 0.666666);

			if (x < 0.666666)
			    g = 0.75 * x + 0.333333;
			else
			    g = 0.833333 - 1.5 * (x - 0.666666);

			if (x < 0.333333)
			    b = 1. - 2. * x;
			else if (x < 0.666666)
			    b = x;
			else
			    b = 0.666666 - 2. * (x - 0.666666);
			break;

		    case colormap_greenscale:
			r = x;
			g = pow (x, 1./4.);
			b = pow (x, 4.);
			break;

		    case colormap_cyanscale:
			r = pow (x, 4.);
			g = pow (x, 1./4.);
			b = x;
			break;

		    case colormap_bluescale:
			r = pow (x, 4.);
			g = x;
			b = pow (x, 1./4.);
			break;

		    case colormap_magentascale:
			r = x;
			g = pow (x, 4.);
			b = pow (x, 1./4.);
			break;

		    case colormap_redscale:
			r = pow (x, 1./4.);
			g = pow (x, 4.);
			b = x;
			break;

		    case colormap_brownscale:
			r = 0.55 + x * 0.45;
			g = 0.15 + x * 0.85;
			b = 0;
			break;

		    case colormap_user_defined:
			j = *colormode == colormode_normal ? i : i - 1;
			r = cmap[j][0] / 255.0;
			g = cmap[j][1] / 255.0;
			b = cmap[j][2] / 255.0;
			break;
		    };

	        GSCR (&wkid, &ci, &r, &g, &b);

		if (*colormode == colormode_normal)
		    i++;
		else
		    i--;
		}
            }

        stat = gus__normal;
        }
    else
        /* GKS not in proper state. GKS must be either in the state WSAC 
           or SGOP */

        stat = gus__notact;

    if (present(status))
        *status = stat;
    else
        if (!odd(stat))
            gus_signal (stat, "SET_COLORMAP");

    return (stat);
}



int gus_show_colormap (int *status)

/*
 * gus_show_colormap - show the colormap
 */

{
    int stat, opsta, ci, cells;
    int errind, halign, valign, clsw, tnr;
    float clrt[4], wn[4], vp[4];

    float xmin, xmax, ymin, ymax, zmin, zmax;
    int w, h, sx, sy, nx, ny, colia[last_color-first_color+1];

    int i, nz;
    float x, y, z, dy, dz;
    char text[256];

    /* inquire and test GKS operating state */

    GQOPS (&opsta);

    if (opsta >= GWSAC)
	{
	gus_set_scale (&lx.scale_options, &stat);

	if (odd(stat))
	    {
            /* save text alignment and clipping indicator */

	    GQTXAL (&errind, &halign, &valign);
            GQCLIP (&errind, &clsw, clrt);

            /* inquire current normalization transformation */

            GQCNTN (&errind, &tnr);
            GQNT (&tnr, &errind, wn, vp);

	    cells = last_color-first_color+1;
	    for (ci = first_color; ci <= last_color; ci++)
		colia[ci-first_color] = ci;

	    xmin = lx.x_min; xmax = lx.x_max;
	    ymin = lx.y_min; ymax = lx.y_max;
	    zmin = wx.z_min; zmax = wx.z_max;

	    w = nx = 1;
	    h = ny = cells;
	    sx = sy = 1;

	    GCA (&xmin, &ymin, &xmax, &ymax, &w, &h, &sx, &sy, &nx, &ny, colia);

            dz = 0.5 * gus_tick(&zmin, &zmax);
	    nz = (int)((zmax-zmin) / dz + 0.5);
	    dy = (ymax-ymin) / nz;

	    GSTXAL (&GALEFT, &GAHALF);
            GSCLIP (&GNCLIP);

	    x = xmax + 0.01 * (xmax - xmin) / (vp[1] - vp[0]);
	    for (i = 0; i <= nz; i++)
		{
		y = ymin + i*dy;
		z = zmin + i*dz;
		gus_text (&x, &y, str_ftoa (text, z, dz), NIL);
		}

            /* restore text alignment and clipping indicator */

	    GSTXAL (&halign, &valign);
            GSCLIP (&clsw);
	    }
        }
    else
        /* GKS not in proper state. GKS must be in one of the states WSOP,WSAC
           or SGOP */

        stat = gus__notope;

    if (present(status))
        *status = stat;
    else
        if (!odd(stat))
            gus_signal (stat, "COLORMAP");

    return (stat);
}



static float *get_slices (int *numslices, float *data)
{
    float total, *slices;
    int i;
 
    if (*numslices == 0)
	return (NULL);

    total = 0;
    for (i = 0; i < *numslices; i++)
	total = total + data[i];

    slices = (float *) gmalloc (sizeof(float) * (*numslices));
    for (i = 0; i < *numslices; i++)
	*(slices + i) = data[i]/total * 360.0;

    return (slices);
}



static void label (int *numslices, float *slices, float *data)
{
    int i, errind, txalh, txalv, npoints = 3;
    float cos_value, sin_value, x[3], y[3], angle = 0.0;
    float x_text, y_text, temp;
    char text[200];

    GQTXAL (&errind, &txalh, &txalv);

    for (i = 0; i < *numslices; i++)
	{
	/* Labels are drawn halfway between the edges of a slice */

	temp = slices[i]/2.0 + angle; 
	cos_value = cos(PI * temp/180.0);
	sin_value = sin(PI * temp/180.0);

	/* Calculate the coordinates needed to draw the line for the label */

	x[0] = 0.5 + pie_radius * cos_value * 0.5;
	y[0] = 0.5 + pie_radius * sin_value * 0.5; 
	x[1] = 0.5 + pie_line * cos_value * 0.5;
	y[1] = 0.5 + pie_line * sin_value * 0.5;

	if (temp >= 90.0 && temp < 270.0)
	    {
	    /* Text alignment and the position of the final line segment are
	       dependent on where you are in terms of the angle. */

	    x[2] = x[1] - pie_offset;
	    GSTXAL (&GARITE, &GAHALF);

	    x_text = x[2] - pie_offset/2.0;
	    }
	else
	    {
	    x[2] = x[1] + pie_offset;
	    GSTXAL (&GALEFT, &GAHALF);

	    x_text = x[2] + pie_offset/2.0;
	    }

	angle = angle + slices[i];

	y[2] = y[1];
	y_text =  y[2];

	GPL (&npoints, x, y);

	gus_text (&x_text, &y_text, str_ftoa (text, data[i], 0.0), NIL);
	}

    GSTXAL (&txalh, &txalv);
}



static void draw_pie (int *numslices, float *slices, float xcenter,
    float ycenter, float radius)
{
    int i, j, errind, interior, black = 1;
    int color, saved_color, style, saved_style, steps;
    float x[362], y[362], angle, first, last = 0.0;

    GQFAIS (&errind, &interior);
    GQFASI (&errind, &style);
    GQFACI (&errind, &color);

    saved_style = style;
    saved_color = color;

    angle = 0.0;
    for (i = 0; i < *numslices; i++)
	{
	first = last;

	/* Calculate how many one degree steps it takes to draw the wedge,
	   and allocate the space it will take to hold the x and y coordinates
	   for that wedge, including space for the center coordinates */

	x[0] = x[(int)slices[i] + 1] = xcenter;
	y[0] = y[(int)slices[i] + 1] = ycenter;

	for (j = 1; j <= ((int)slices[i]); j++)
	    {
	    /* Calculate the x and y coordinates for the arc of the wedge
	       and increment the angle used to generate the points */

	    *(x + j) = xcenter + radius * cos(PI*angle/180.0);
	    *(y + j) = ycenter + radius * sin(PI*angle/180.0);

	    angle = angle + 1.0;
	    }

	x[1] = xcenter + radius * cos(PI*first/180.0);
	y[1] = ycenter + radius * sin(PI*first/180.0);

	last = last + slices[i];

	x[(int)slices[i]] = xcenter + radius * cos(PI*last/180.0);
	y[(int)slices[i]] = ycenter + radius * sin(PI*last/180.0);

	/* Draw a wedge */

	steps = (int)slices[i] + 2;
	GFA (&steps, x, y);

	if (interior != GHOLLO)
	    {
	    /* If the interior style is either SOLID, PATTERN, or HATCH draw
	       a black outline around the wedge that was just drawn. Change
	       the color, pattern or hatch style that will be used to draw
	       the next wedge */

	    GSFAIS (&GHOLLO);
	    GSFACI (&black);
	    GFA (&steps, x, y);
	    GSFAIS (&interior);

	    switch (interior)
		{
		case 1:	color = (color + 1) % pie_colors;
			GSFACI (&color);
			break;
		case 2:	style = (style + 1) % patterns;
			if (style == 0) style = 1;
			GSFASI (&style);
			break;
		case 3:	style = (style + 1) % hatch_styles;
			if (style == 0) style = 1;
			GSFASI (&style);
			break;
		}
	    }
	}

    GSFASI (&saved_style);
    GSFACI (&saved_color);
}




int gus_pie_chart (int *n, float *data, int *status)

/*
 * FUNCTIONAL DESCRIPTION:
 *
 *	Draws a pie chart.
 *
 * FORMAL ARGUMENT(S):
 *
 *      N               The number of slices
 *      DATA            The data for the slices
 *	STATUS		Optional status value
 *
 * IMPLICIT INPUTS:
 *
 *	NONE
 *
 * IMPLICIT OUTPUTS:
 *
 *	NONE
 *
 * COMPLETION CODES:
 *
 *	GUS__NORNAL	normal successful completion
 *	GUS__NOTACT	GKS not in proper state. GKS must be either in the
 *			state WSAC or SGOP
 *	GUS__INVWINLIM	cannot apply logarithmic transformation to current
 *			window
 *	GUS__INVNUMPNT	invalid number of points
 *	GUS__INVPNT	point co-ordinates must be greater than 0
 *
 * SIDE EFFECTS:
 *
 *	NONE
 *
 * AUTHOR:
 *
 *      Dennis          2-Jul-1992
 *
 */

{
    int opsta, tnr, errind, stat, i;
    float wn[4], vp[4], radius, xcenter, ycenter, *slices;
    float x1, x2, y1, y2;
     
    x1 = y1 = 0.0;
    x2 = y2 = 1.0;

    if (*n > 1)
	{
	stat = gus__normal;

	for (i = 0; i < *n; i++)
	    if (data[i] <= 0)
		stat = gus__invpnt;  /* point co-ordinates must be greater
					than 0 */
	}
    else
	stat = gus__invnumpnt; /* invalid number of points */

    if (odd(stat))
	{
	GQOPS (&opsta);		/* Check GKS operating state */

	if (opsta == GWSAC || opsta == GSGOP)
	    {
	    GQCNTN (&errind, &tnr);
	    GQNT (&tnr, &errind, wn, vp);
	    GSWN (&tnr, &x1, &x2, &y1, &y2);

	    /* The center of the window is determined, as this is the
	       point at which the circle will be drawn at */

	    xcenter = ycenter = 0.5;
	    radius = pie_radius * ycenter;

	    if (!((option_x_log IN lx.scale_options) ||
		  (option_y_log IN lx.scale_options)))
		{
		slices = get_slices (n, data);

		draw_pie (n, slices, xcenter, ycenter, radius);
		label (n, slices, data);

		free (slices);
		}
	    else
		stat = gus__invwinlim;  /* cannot apply logarithmic 
					   transformation to current window */

	    GSWN (&tnr, &wn[0], &wn[1], &wn[2], &wn[3]);
	    }
	else
	    /* GKS not in proper state. GKS must be either in the state WSAC 
	       or SGOP */

	    stat = gus__notact;
	}

    if (present(status))
        *status = stat;
    else
        if (!odd(stat))
            gus_signal (stat, "PIE_CHART");

    return (stat);
}



void gus_signal (int cond_value, char *procid)

/*
 * gus_signal - signal exception condition
 */

{
    /* update all open workstations */

    update_open_ws ();
    raise_exception (cond_value, 1, procid, NULL);
}

