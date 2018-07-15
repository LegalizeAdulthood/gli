/*
 * (C) Copyright 1991  Josef Heinen
 *
 *
 * FACILITY:
 *
 *	GLI (Graphics Language Interpreter)
 *
 * ABSTRACT:
 *
 *	This module contains a CGM viewer.
 *
 * AUTHOR:
 *
 *	Phil Andrews, Pittsburgh Supercomputing
 *	Josef Heinen, FZ Juelich
 *      Jens Kuenne, FZ Juelich
 *
 * VERSION:
 *
 *	V1.0
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#if !defined(VMS) && !defined(MSDOS) && !defined(_WIN32)
#include <unistd.h>
#endif

#ifndef MSDOS
#include <sys/types.h>
#endif

#ifdef VMS
#include <descrip.h>
#endif

#ifdef cray
#include <fortran.h>
#endif

#include "glicgm.h"
#include "gksdefs.h"
#include "image.h"
#include "terminal.h"
#ifndef NO_SIGHT
#include "sight.h"
#endif

extern int cli_b_abort;

static int cgm_command();

typedef int     (*funptr) ();

#define WindowHeight        485         /* window height */
#define Ratio               1.41421
#define CapSize             0.75

#define NominalMarkerSize   6           /* nominal marker size */

#define max_str 128
#define max_fonts 100
#define get_all (0)
#define bytes_per_word sizeof(int)
#define byte_size 8
#define float_size (sizeof(float))
#define max_digits 10

/* macros to handle direct colours */
#define dcr(ival) dcind(0, ival, glbl1->c_v_extent)
#define dcg(ival) dcind(1, ival, glbl1->c_v_extent)
#define dcb(ival) dcind(2, ival, glbl1->c_v_extent)

#define e_size 2
#define intalloc (int *) malloc	       /* for convenience */

/* set up a couple of work arrays for polygons, etc. */
#define wk_ar_size 2024

#define newx(xin, yin) ((int) (sx * xp0 + (sx * cosr * xin - sy * sinr * yin)))
#define newy(xin, yin) ((int) (sy * yp0 + (sy * cosr * yin + sx * sinr * xin)))

#define use_random (random_input && (index_present == 1))
#define p_s_size sizeof(struct p_struct)
#ifdef MSDOS
#define max_pages 5000
#else
#define max_pages 10000
#endif

#define bytes_per_word sizeof(int)
#define float_size (sizeof(float))

#define max_cgm_str 1024

/* Default storage space for rows of color data */
#define rowmax 255

/* translate an int to a floating-point color value */
#define i_to_red(int) ( ( (float) (int - glbl1->c_v_extent.min[0]) )/ 	\
( (float) (glbl1->c_v_extent.max[0] - glbl1->c_v_extent.min[0]) ) )

#define i_to_grn(int) ( ( (float) (int - glbl1->c_v_extent.min[1]) )/	\
( (float) (glbl1->c_v_extent.max[1] - glbl1->c_v_extent.min[1]) ) )

#define i_to_blu(int) ( ( (float) (int - glbl1->c_v_extent.min[2]) )/	\
( (float) (glbl1->c_v_extent.max[2] - glbl1->c_v_extent.min[2]) ) )

/* translate R, G, B values to a gray scale value (NTSC standard) */
#define mcr_gray(r,g,b) ( 0.3 * r + 0.59 * g + 0.11 * b )

/* Add two 2-D vectors to produce a third */
#define mcr_addvec(a,b,c) {a[0]= b[0] + c[0]; a[1]= b[1] + c[1];}

/* Get an arbitrary precision color value, byte aligned or not */
#define mcr_gtcv(ptr, precision, out, bit) out = 0; switch (precision){	\
case 32: out = *ptr++;							\
case 24: out = (out << byte_size) | (*ptr++ & 255);			\
case 16: out = (out << byte_size) | (*ptr++ & 255);			\
case 8: out = (out << byte_size) | (*ptr++ & 255); break;     		\
case 4: out= ( (*ptr >> (4-bit)) & 15 ); bit= bit + 4; 			\
if (bit == 8) { bit = 0; ++ptr; }; break;				\
case 2: out= ( (*ptr >> (6-bit)) & 3 ); bit= bit + 2; 			\
if (bit == 8) { bit = 0; ++ptr; }; break;				\
case 1: out= ( (*ptr >> (7-bit)) & 1 ); bit= bit + 1;			\
if (bit == 8) { bit = 0; ++ptr; }; }					\

/* Get a 2-octet integer, byte aligned or not */
#define mcr_gtei(ptr,out,bit)  switch (bit) {				\
case 0: out = ((*ptr << byte_size) | (*(ptr+1) & 255)); 		\
	ptr= ptr+2; break;						\
case 1: out = (((*ptr & 127) << 9)|(*(ptr+1) << 1)|(*(ptr+2) >> 7)); 	\
	ptr= ptr+2; break;						\
case 2: out = (((*ptr & 63) << 10)|(*(ptr+1) << 2)|(*(ptr+2) >> 6)); 	\
	ptr= ptr+2; break;						\
case 3: out = (((*ptr & 31) << 11)|(*(ptr+1) << 3)|(*(ptr+2) >> 5)); 	\
	ptr= ptr+2; break;						\
case 4: out = (((*ptr & 15) << 12)|(*(ptr+1) << 4)|(*(ptr+2) >> 4)); 	\
	ptr= ptr+2; break;						\
case 5: out = (((*ptr & 7) << 13)|(*(ptr+1) << 5)|(*(ptr+2) >> 3)); 	\
	ptr= ptr+2; break;						\
case 6: out = (((*ptr & 3) << 14)|(*(ptr+1) << 6)|(*(ptr+2) >> 2)); 	\
	ptr= ptr+2; break;						\
case 7: out = (((*ptr & 1) << 15)|(*(ptr+1) << 7)|(*(ptr+2) >> 1));	\
	ptr= ptr+2; };

#define int_size (sizeof(int))
#define float_size (sizeof(float))

#define MAX_LINETYPES	4
#define MAX_MARKERTYPES 5

#define ROWSIZE 1024

/* Array for line types list, set by gks_setup */
#define NLINES 6

/* Array for marker types list, set by gks_setup */
#define NMARKERS 10

#define BEST_C_SZ 32768		       /* = 32**3;  not a free
					* parameter! */
#define f_to_b(f) ( (unsigned char) ((int)((f)*255.0) % 256 ))
#define b_to_f(b) ( (1./255.)*((float) (b)) )
#define mask_8_thru_4 (248) 
#define pack_clr(ir,ig,ib) \
	( ( ( (ir) & mask_8_thru_4 )<<7 ) \
	| ( ( (ig) & mask_8_thru_4 )<<2 ) \
	| ( ( (ib) & mask_8_thru_4 )>>3 ) )

#define lf      '\012'		       /* line feed, end of record */

#define max_pwrs 8
#define max_b_size 1024

#define map_size 4		       /* may have to remap the bytes */
#define poss_maps 4		       /* for now */

/* NCAR header flags */
#define m_header                4
#define m_ncar_printer          8
#define m_old_meta              2
#define m_ext_cgm               3
#define m_new_frame             8
#define m_beg_meta              4
#define m_end_meta              2
#define m_cont_frame            0

/* some systems don't define HZ */
#ifndef HZ
#define HZ 60
#endif

/* need this for VMS */
#define toint(inchar) ((int) inchar - (int) '0')


typedef struct fa_attr_struct {
    enum is_enum    int_style;     	/* interior style	 */
    struct rgbi_struct fill_colour;	/* for polygons		 */
    enum bool_enum  edge_vis;      	/* edge visibility	 */
    } fa_attr;

static struct info_struct new_info;
static struct one_opt mnew_opt[opt_size];
int             (*get_command) (), (*do_command) ();

/* global pointers that use storage from the main program */
static struct mf_d_struct *glbl1;      /* the class 1 elements */
static struct pic_d_struct *glbl2, *dflt2, *a2;	/* class 2 elements */
static struct control_struct *glbl3, *dflt3, *a3;	/* class 3 elements */
static struct attrib_struct *glbl5, *dflt5, *a5;	/* class 5 elements */
static struct mf_d_struct mglbl1;      /* the class 1 elements */
static struct pic_d_struct mglbl2, mdflt2;	/* class 2 elements */
static struct control_struct mglbl3, mdflt3;	/* class 3 elements */
static struct attrib_struct mglbl5, mdflt5;	/* class 5 elements */
/*
 * g stands for global, d for default, and a for active, we normally
 * use the a pointer, with an = glbln except in the middle of a
 * metafile defaults replacement element, when an = dfltn
 */


/*
 * The following pointers provide storage for the driver's own copies
 * of several important structure pointers.  The struct definitions
 * come from defs.h .
 */
static struct mf_d_struct *State_c1;   /* Current CGM metafile desc.
					* data */
static struct pic_d_struct *State_c2;  /* Current CGM picture descrip.
					* data */
static struct attrib_struct *State_c5; /* Current CGM attribute data */
static struct info_struct *Dev_info;   /* Device characteristics
					* communicated via this struct */

/* these will tell us what the device can do */
static int      (**p_gprim) ();	       /* graphical primitives */

/* now the device-specific function array pointers */
static int      (*mdelim[Delim_Size]) ();	/* delimiter functions */
static int      (*mmfdesc[MfDesc_Size]) ();	/* metafile descriptor
						 * functions */
static int      (*mpdesc[PDesc_Size]) ();	/* page descriptor
						 * functions */
static int      (*mmfctrl[Control_Size]) ();	/* mf control functions */
static int      (*mgprim[GPrim_Size]) ();	/* graphical primitives */
static int      (*mattr[Att_Size]) (); /* the attribute functions */
static int      (*mescfun[Esc_Size]) ();	/* the escape functions */
static int      (*mextfun[Ext_Size]) ();	/* the external
						 * functions */
static int      (*mctrl[Delim_Size]) ();	/* external controller
						 * functions */
static int      (**delim) ();	       /* delimiter functions */
static int      (**mfdesc) ();	       /* metafile descriptor functions */
static int      (**pdesc) ();	       /* page descriptor functions */
static int      (**mfctrl) ();	       /* mf control functions */
static int      (**gprim) ();	       /* graphical primitives */
static int      (**attr) ();	       /* the attribute functions */
static int      (**escfun) ();	       /* the escape functions */
static int      (**extfun) ();	       /* the external functions */
static int      (**ctrl) ();	       /* external controller functions */

/* the device info structure */
static struct info_struct *dev_info;

/* the command line options */
static struct one_opt *opt;	       /* the command line options */


/* the external font functions */
static int      (*font_check) () = NULL;
static int      (*font_text) () = NULL;



/* basic stuff */
static char     digits[max_digits] =
{'0', '1', '2', '3', '4', '5', '6', '7',
 '8', '9'};


/* CGM specific functions */
static char    *cc_str();	       /* does the necessary
					* translation */
static float    cc_real();	       /* does the necessary
					* translation */
static double   pxl_vdc;	       /* conversion from VDC units to
					* pixels */
static double   sx, sy;		       /* allow individual scaling */
static double   cosr;		       /* value of cos theta */
static double   sinr;		       /* value of sin theta */

/* now my globals */
static int      pages_done = 0;
static int      page_empty = 1;
static int      picture_nr = 0;
static char    *g_in_name;	       /* the full input file_name */
static char     pic_name[max_str];     /* picture name */

/* stuff to take care of individual pages */
static int      in_page_no = 0;	       /* page no in the metafile */
static int      skipping = 0;	       /* are we skipping this page ? */
static int      x_wk_ar[wk_ar_size], y_wk_ar[wk_ar_size];

static float    xp0;		       /* internal xoffset in pixels */
static float    yp0;		       /* internal yoffset in pixels */

static int      xoffset, yoffset;      /* external offsets in pixels */
static int      xsize, ysize;	       /* requested page size in pixels */

static float	wn_x0 = 0, wn_x1 = 1, wn_y0 = 0, wn_y1 = 1;
static float	scale_factor = 1;

/* indexing info */
static struct phead_struct
{
    int             no_pages;
    struct p_struct *first;
    struct p_struct *last;
}               p_header;
static struct ad_struct last_ad;       /* address of the last command
					* read */
static struct ad_struct first_index;   /* address of the first index
					* read */
static int      random_input, index_present, index_read;
struct index_struct
{
    int             rec_no;
    int             byte_offset;
    int             total_bytes;
    int             pic_l;
    char           *pic_ptr;
};


/* basic stuff */
static char     buffer_str[max_cgm_str + 1];

static int      irow_default[rowmax], *irow = irow_default, rowsize = rowmax;
static float    rrow_default[rowmax], grow_default[rowmax], brow_default[rowmax],
	       *rrow = rrow_default, *grow = grow_default, *brow = brow_default;

static double   pi;		       /* guess */
static int      setup = 0;

#ifndef NO_SIGHT
static int	sight = 0;
#endif
static int      wc = 1;
static int      state_level;
static float    xscale, yscale;

/* Data concerning the physical display device, set by gks_setup */
static int      ws_id = 1, conid;
static float    dev_x, dev_y;

/* Coordinate buffers, enlarged as needed by gks_getcmem */
static float   *xbuf, *ybuf;
static int      xbuf_sz = 0, ybuf_sz = 0;

/* Image buffer, enlarged as needed by gks_getimem */
static int     *ibuf;
static int      ibuf_sz = 0;



/*
 * Values for character heights available.
 */
static int      num_c_heights, c_h_min, c_h_max;

/* Total number of color indices available, set by gks_setup */
static int      clr_t_sz = 128;

static int      dc_ready_flag = 0;
static int	dc_init_flag = 0, dc_totclrs;
static float   *clr_tbl;
static int     *best_clr;

static FILE    *inptr = NULL;	       /* input pointer */

/* record buffer stuff */
static unsigned char *start_buf, *end_buf, *buf_ptr;
static int      buf_size;	       /* input buffer size */
static int      recs_got;	       /* how many records read so far */
static int      bytes_read;	       /* how many bytes read so far */
static int      need_map;
static unsigned char poss_map[poss_maps][map_size] =
{
    {0, 1, 2, 3},
    {1, 0, 3, 2},
    {3, 2, 1, 0},
    {2, 3, 0, 1}};

static int      ncar = 0;	       /* is this an ncar file ? */
static int      skip_header = 0;       /* do we want to skip some bytes
					* ? */

/* now the NCAR header format */
struct header_type
{				       /* the structure for the header */
    unsigned char   byte1;	       /* real bytes (2) used in record */
    unsigned char   byte2;	       /* real bytes (2) used in record */
    unsigned char   flags;	       /* various flags */
    char            dummy;	       /* low-order byte (1) not used */
};



static int 
s_error(char *str, ...)
{
    va_list args;
    char fmt[max_str], s[max_str];

    sprintf (fmt, "CGM: %s\n", str);

    va_start (args, str);
    vsprintf (s, fmt, args);
    va_end (args);

    tt_fprintf (stderr, s);

    return (1);
}




/* emulate the rectangle function */
static int em_rectangle(x1, y1, x2, y2)
    int             x1, y1, x2, y2;
{
    int             xarray[5], yarray[5];

    if (!setup)
    {
	return (2);
    }
    if (p_gprim[(int) Polygon])
    {
	xarray[0] = x1;
	yarray[0] = y1;
	xarray[2] = x2;
	yarray[2] = y2;
	xarray[1] = x2;
	yarray[1] = y1;
	xarray[3] = x1;
	yarray[3] = y2;
	(*p_gprim[(int) Polygon]) (4, xarray, yarray);
    } else if (p_gprim[(int) PolyLine])
    {
	xarray[0] = x1;
	yarray[0] = y1;
	xarray[2] = x2;
	yarray[2] = y2;
	xarray[1] = x2;
	yarray[1] = y1;
	xarray[3] = x1;
	yarray[3] = y2;
	xarray[4] = x1;
	yarray[4] = y1;
	(*p_gprim[(int) PolyLine]) (5, xarray, yarray);
    } else
	return (2);		       /* can't do anything */

    return (1);
}

/*
 * take care of necessary memory, note must clear by hand for
 * compatibility
 */
static unsigned char  *allocate_mem(size, clear)
    int             size, clear;
{
    unsigned char  *mem_ptr;
    int             i;

    if (!(mem_ptr = (unsigned char *) malloc(size)))
    {
	s_error ("couldn't allocate memory");
	return (NULL);
    } else if (clear)
	for (i = 0; i < size; ++i)
	    *(mem_ptr + i) = 0;

    return (mem_ptr);
}



/* basic circle routine */
static int em_arc(x, y, r, theta0, theta1, no_points, extra_points,
       x_ptr, y_ptr)
    int             x, y, r, *no_points, extra_points, **x_ptr, **y_ptr;
    double          theta0, theta1;
{
    double          dtheta, dctheta, dstheta, cth0, sth0, cth1, sth1;
    int             i;

    /*
     * first find a reasonable no. points, get angles between 0 and 2
     * pi
     */
    while (theta0 < 0.0)
	theta0 += 2 * pi;
    while (theta1 < 0.0)
	theta1 += 2 * pi;
    while (theta0 > 2.0 * pi)
	theta0 -= 2 * pi;
    while (theta1 > 2.0 * pi)
	theta1 -= 2 * pi;

    /* how many points ? */
    *no_points = (theta1 - theta0) * r / pi;	/* reasonable */
    if (*no_points < 0)
	*no_points = (2 * pi + theta0 - theta1) * r / pi;
    if (*no_points < 2)
	*no_points = 2;		       /* must be > 2 */
    /* now order them correctly */
    if (theta0 > theta1)
	theta0 -= 2.0 * pi;
    /* make some memory */
    if (!(*x_ptr = (int *)
	  allocate_mem((*no_points + extra_points) * int_size, 0)) ||
	!(*y_ptr = (int *)
	  allocate_mem((*no_points + extra_points) * int_size, 0)))
	return (0);

    /* now fill out *no_points worth */
    cth0 = cos(theta0);
    sth0 = sin(theta0);

    dtheta = (theta1 - theta0) / (*no_points - 1);
    dctheta = cos(dtheta);
    dstheta = sin(dtheta);

    /* starting point */
    (*x_ptr)[0] = x + r * cth0;
    (*y_ptr)[0] = y + r * sth0;

    /* double precision should take care of roundoff */
    for (i = 1; i < *no_points; ++i)
    {
	/* use recursion relation */
	cth1 = cth0 * dctheta - sth0 * dstheta;
	sth1 = sth0 * dctheta + cth0 * dstheta;
	(*x_ptr)[i] = x + r * cth1;
	(*y_ptr)[i] = y + r * sth1;
	/* update the point */
	cth0 = cth1;
	sth0 = sth1;
    }
    return (1);
}



/* emulate a circle */
static int em_circle(x, y, r)
    int             x, y, r;	       /* center (x,y) and radius r */
{
    int             no_points, *xptr, *yptr, extra_points, ret;

    if (!setup)
    {
	return (2);
    }
    if (!p_gprim[(int) PolyLine] && !p_gprim[(int) Polygon])
	return (2);		       /* can't do anything */
    extra_points = (p_gprim[(int) Polygon]) ? 0 : 1;

    /* get the points on the circumference */
    if (!em_arc(x, y, r, 0.0, 2.0 * pi, &no_points, extra_points,
		&xptr, &yptr))
	return (2);

    /* do we have a polygon ? */
    if (p_gprim[(int) Polygon])
    {
	ret = (*p_gprim[(int) Polygon]) (no_points, xptr, yptr);
    } else if (p_gprim[(int) PolyLine])
    {
	xptr[no_points] = xptr[0];
	yptr[no_points] = yptr[0];
	++no_points;
	ret = (*p_gprim[(int) PolyLine]) (no_points, xptr, yptr);
    }
    free(xptr);
    free(yptr);
    return (ret);
}



/* routines to help out with circle and ellipse calculations */
/*
 * set an arc, we get the positions of 1st pt, intermediate pt, end pt,
 * all angles from x axis anticlockwise
 */
/*
 * for equations we have: x[0] = xc + r * cos(theta0) y[0] = yc + r *
 * sin(theta0) x[1] = xc + r * cos(theta1) y[1] = yc + r * sin(theta1)
 * x[2] = xc + r * cos(theta2) y[2] = yc + r * sin(theta2)
 */
static int get_c_info(pt_ptr, xc, yc, r, theta, deg)
    int            *pt_ptr, *xc, *yc, *r;
    float          *deg;
    double         *theta;
{
    int             it1, i;
    double          t1, rxc, ryc, rr, t2, t3, rsq, pi;

    /* first make sure they're not co-linear */
    it1 = (pt_ptr[4] - pt_ptr[0]) * (pt_ptr[3] - pt_ptr[1]) -
	(pt_ptr[2] - pt_ptr[0]) * (pt_ptr[5] - pt_ptr[1]);
    if (it1 == 0)
	return (0);		       /* colinear */
    /* can get xc and yc now */
    t1 = 2.0 * it1;
    t2 = (pt_ptr[2] * pt_ptr[2] - pt_ptr[0] * pt_ptr[0]) +
	(pt_ptr[3] * pt_ptr[3] - pt_ptr[1] * pt_ptr[1]);
    t3 = (pt_ptr[4] * pt_ptr[4] - pt_ptr[0] * pt_ptr[0]) +
	(pt_ptr[5] * pt_ptr[5] - pt_ptr[1] * pt_ptr[1]);

    /* first yc */
    ryc = ((pt_ptr[4] - pt_ptr[0]) * t2 -
	   (pt_ptr[2] - pt_ptr[0]) * t3) / t1;
    /* now xc */
    rxc = ((pt_ptr[5] - pt_ptr[1]) * t2 -
	   (pt_ptr[5] - pt_ptr[1]) * t3) / t1;

    /* now pick up the radius */
    rsq = (pt_ptr[0] - rxc) * (pt_ptr[0] - rxc) +
	(pt_ptr[1] - ryc) * (pt_ptr[1] - ryc);

    rr = sqrt(rsq);		       /* non-negative by definition */

    /* now the angles */
    pi = 4.0 * atan(1.0);
    for (i = 0; i < 3; ++i)
    {
	theta[i] = atan2((pt_ptr[2 * i + 1] - ryc) / rr,
			 (pt_ptr[2 * i] - rxc) / rr);
	deg[i] = 180.0 * theta[i] / pi;
    }
    /* finally the integer forms, do a simple truncate */
    *xc = (int) rxc;
    *yc = (int) ryc;
    *r = (int) rr;
    return (1);
}





/* emulate an arc, specified by 3 pts */
static int em_c3(pt_ptr)
    int            *pt_ptr;
{
    int             x, y, r;	       /* circle centre and radius */
    float           deg[3];	       /* starting and ending degrees
					* (0 at top) */
    double          theta[3];
    int             no_points, *xptr, *yptr, extra_points, ret;

    if (!setup)
    {
	return (2);
    }
    if (!p_gprim[(int) PolyLine])
	return (2);		       /* can't do anything */
    extra_points = 0;

    if (!get_c_info(pt_ptr, &x, &y, &r, theta, deg))
	return (2);

    /* get the points on the circumference */
    if (!em_arc(x, y, r, theta[0], theta[2], &no_points, extra_points,
		&xptr, &yptr))
	return (2);

    ret = (*p_gprim[(int) PolyLine]) (no_points, xptr, yptr);

    free(xptr);
    free(yptr);
    return (ret);
}



/* emulate an arc, specified by 3 pts, close it */
static int em_c3_close(pt_ptr, chord)
    int            *pt_ptr;
    enum bool_enum  chord;
{
    int             x, y, r;	       /* circle centre and radius */
    float           deg[3];	       /* starting and ending degrees
					* (0 at top) */
    double          theta[3];
    int             no_points, *xptr, *yptr, extra_points, ret;

    if (!setup)
    {
	return (2);
    }
    if (!p_gprim[(int) PolyLine] && !p_gprim[(int) Polygon])
	return (2);		       /* can't do anything */

    extra_points = (chord) ? 0 : 1;
    if (!p_gprim[(int) Polygon])
	++extra_points;

    if (!get_c_info(pt_ptr, &x, &y, &r, theta, deg))
	return (2);

    /* get the points on the circumference */
    if (!em_arc(x, y, r, theta[0], theta[2], &no_points, extra_points,
		&xptr, &yptr))
	return (2);

    if (!(int) chord)
    {
	xptr[no_points] = x;
	yptr[no_points] = y;
	++no_points;
    }
    if (p_gprim[(int) Polygon])
	ret = (*p_gprim[(int) Polygon])
	    (no_points, xptr, yptr);
    else
    {
	xptr[no_points] = pt_ptr[4];
	yptr[no_points] = pt_ptr[5];
	++no_points;
	ret = (*p_gprim[(int) PolyLine]) (no_points, xptr, yptr);
    }
    free(xptr);
    free(yptr);
    return (ret);
}



/*
 * routine to take a vector and return a degree/radian, measured from
 * the x axis in an anticlockwise direction
 */
static int get_angle(ptr, deg, theta)
    int            *ptr;
    float          *deg;
    double         *theta;
{
    double          pi, dx, dy;

    pi = 4.0 * atan(1.0);

    if (ptr[0] == 0)
    {
	if (ptr[1] >= 0)
	{
	    *deg = 90.0;
	    *theta = 0.5 * pi;
	} else
	{
	    *deg = 270.0;
	    *theta = 1.5 * pi;
	}
	return (2);
    }
    dx = (double) ptr[0];
    dy = (double) ptr[1];

    *theta = atan2(dy, dx);
    *deg = 180.0 * (*theta / pi);
    return (1);
}




/* set an arc, ends specified by vectors */
static int em_c_centre(x, y, vec_array, r)
    int             x, y, *vec_array, r;
{
    float           deg0;
    double          theta0, theta1;
    int             no_points, *xptr, *yptr, extra_points, ret;

    if (!setup)
    {
	return (2);
    }
    if (!p_gprim[(int) PolyLine])
	return (2);		       /* can't do anything */
    extra_points = 0;

    get_angle(vec_array, &deg0, &theta0);
    get_angle(vec_array + 2, &deg0, &theta1);

    /* get the points on the circumference */
    if (!em_arc(x, y, r, theta0, theta1, &no_points, extra_points,
		&xptr, &yptr))
	return (2);

    ret = (*p_gprim[(int) PolyLine]) (no_points, xptr, yptr);

    free(xptr);
    free(yptr);
    return (ret);


}



/* set an arc, ends specified by vectors, but close it */
static int em_c_c_close(x, y, vec_array, r, chord)
    int             x, y, *vec_array, r;
    enum bool_enum  chord;
{
    float           deg0;
    double          theta0, theta1;
    int             no_points, *xptr, *yptr, extra_points, ret, x_1, y_1;

    if (!setup)
    {
	return (2);
    }
    if (!p_gprim[(int) PolyLine] && !p_gprim[(int) Polygon])
	return (2);		       /* can't do anything */

    extra_points = (chord) ? 0 : 1;
    if (!p_gprim[(int) Polygon])
	++extra_points;

    get_angle(vec_array, &deg0, &theta0);
    get_angle(vec_array + 2, &deg0, &theta1);
    x_1 = x + r * cos(theta1);
    y_1 = y + r * sin(theta1);


    /* get the points on the circumference */
    if (!em_arc(x, y, r, theta0, theta1, &no_points, extra_points,
		&xptr, &yptr))
	return (2);

    if (!(int) chord)
    {
	xptr[no_points] = x;
	yptr[no_points] = y;
	++no_points;
    }
    if (p_gprim[(int) Polygon])
	ret = (*p_gprim[(int) Polygon])
	    (no_points, xptr, yptr);
    else
    {
	xptr[no_points] = x_1;
	yptr[no_points] = y_1;
	++no_points;
	ret = (*p_gprim[(int) PolyLine]) (no_points, xptr, yptr);
    }
    free(xptr);
    free(yptr);
    return (ret);
}



/* emulate an ellipse, implement later */
static int em_ellipse(pt_array)
    int            *pt_array;
{
    return (1);
}
/* emulate an elliptical arc, implement later */
static int em_ell_arc(pt_array, vec_array)
    int            *pt_array, *vec_array;
{
    return (1);
}
/* emulate a closed elliptical arc, implement later */
static int em_e_a_close(pt_array, vec_array, chord)
    int            *pt_array, *vec_array;
    enum bool_enum  chord;
{
    return (1);
}





/* This routine initializes the cell array utility software */
static int cla_init(pc1, pc2, pc5, pgprim, pattr)
    struct mf_d_struct *pc1;
    struct pic_d_struct *pc2;
    struct attrib_struct *pc5;
    funptr         *pgprim, *pattr;
{
    glbl1 = pc1;
    glbl2 = pc2;
    glbl5 = pc5;

    gprim = pgprim;
    attr = pattr;

    return (1);
}

/* Creates a row's worth of dc color values (floats) */
static unsigned char  *cla_dc_row(dat_ptr, nx, rbuf, gbuf, bbuf, col_prec, mode)
    unsigned char  *dat_ptr;
    int             nx, col_prec, mode;
    float          *rbuf, *gbuf, *bbuf;
{
    int             i, ir, ig, ib, index, ctboff, count, idup;
    float           r, g, b;
    unsigned char  *old_ptr = dat_ptr;
    unsigned int    bit = 0;

    switch (glbl2->c_s_mode)
    {
    case i_c_mode:
	switch (mode)
	{
	case 0:
	    for (i = 0; i < nx;)
	    {
		mcr_gtei(dat_ptr, count, bit);
		mcr_gtcv(dat_ptr, col_prec, index, bit);
		/*
		 * The following test is done to prevent bad metafiles
		 * from crashing the routine by writing beyond the end
		 * of the result buffers.
		 */
		if (count > (nx - i))
		    count = nx - i;
		i = i + count;
		ctboff =
		    (index <= glbl1->max_c_index) ?
		    3 * index - 1 : 0;
		r = glbl5->ctab[ctboff++];
		g = glbl5->ctab[ctboff++];
		b = glbl5->ctab[ctboff];
		for (idup = 0; idup < count; idup++)
		{
		    *rbuf++ = r;
		    *gbuf++ = g;
		    *bbuf++ = b;
		};
	    };
	    break;
	case 1:
	    for (i = 0; i < nx; i++)
	    {
		mcr_gtcv(dat_ptr, col_prec, index, bit);
		ctboff =
		    (index <= glbl1->max_c_index) ?
		    3 * index - 1 : 0;
		*rbuf++ = glbl5->ctab[ctboff++];
		*gbuf++ = glbl5->ctab[ctboff++];
		*bbuf++ = glbl5->ctab[ctboff];
	    };
	};
	break;
    case d_c_mode:
	switch (mode)
	{
	case 0:
	    for (i = 0; i < nx;)
	    {
		mcr_gtei(dat_ptr, count, bit);
		mcr_gtcv(dat_ptr, col_prec, ir, i);
		mcr_gtcv(dat_ptr, col_prec, ig, i);
		mcr_gtcv(dat_ptr, col_prec, ib, i);
		/*
		 * The following test is done to prevent bad metafiles
		 * from crashing the routine by writing beyond the end
		 * of the result buffers.
		 */
		if (count > (nx - i))
		    count = nx - i;
		i = i + count;
		r = i_to_red(ir);
		g = i_to_grn(ig);
		b = i_to_blu(ib);
		for (idup = 0; idup < count; idup++)
		{
		    *rbuf++ = r;
		    *gbuf++ = g;
		    *bbuf++ = b;
		};
	    };
	    break;
	case 1:
	    for (i = 0; i < nx; i++)
	    {
		mcr_gtcv(dat_ptr, col_prec, ir, i);
		*rbuf++ = i_to_red(ir);
		mcr_gtcv(dat_ptr, col_prec, ig, i);
		*gbuf++ = i_to_grn(ig);
		mcr_gtcv(dat_ptr, col_prec, ib, i);
		*bbuf++ = i_to_blu(ib);
	    };
	};
    };

    /* Now we return an updated pointer, making sure it's word aligned */
    if (bit != 0)
	++dat_ptr;
    if ((int) (dat_ptr - old_ptr) % 2)
	++dat_ptr;
    return (dat_ptr);

}

/*
 * Creates a row's worth of color indices (ints);  returns NULL if
 * called with csm= direct color
 */
static unsigned char  *cla_i_row(dat_ptr, nx, rowbuf, col_prec, mode)
    unsigned char  *dat_ptr;
    int             nx, col_prec, mode, *rowbuf;
{
    int             i, index, count, idup;
    unsigned char  *old_ptr = dat_ptr;
    unsigned int    bit = 0;

    switch (glbl2->c_s_mode)
    {
    case i_c_mode:
	switch (mode)
	{
	case 0:
	    for (i = 0; i < nx;)
	    {
		mcr_gtei(dat_ptr, count, bit);
		mcr_gtcv(dat_ptr, col_prec, index, bit);
		/*
		 * The following test is done to prevent bad metafiles
		 * from crashing the routine by writing beyond the end
		 * of the result buffer.
		 */
		if (count > (nx - i))
		    count = nx - i;
		i = i + count;
		if (index > glbl1->max_c_index)
		    index = 0;
		for (idup = 0; idup < count; idup++)
		    *rowbuf++ = index;
	    };
	    break;
	case 1:
	    for (i = 0; i < nx; i++)
	    {
		mcr_gtcv(dat_ptr, col_prec, index, bit);
		if (index > glbl1->max_c_index)
		    index = 0;
		*rowbuf++ = index;
	    };
	};
	break;
    case d_c_mode:
	return (NULL);
    };

    /* Now we return an updated pointer, making sure it's word aligned */
    if (bit != 0)
	++dat_ptr;
    if ((int) (dat_ptr - old_ptr) % 2)
	++dat_ptr;
    return (dat_ptr);

}



/*
 * This is a portable version of the 'free' routine
 */
static int cla_free(ptr)
    char           *ptr;
{
    free(ptr);
    return (0);
}


/*
 * This routine checks to see if new row memory buffers have been
 * allocated, and if so deallocates them and aims the appropriate
 * pointers back at the default buffers.
 */
static int cla_resetrowsize()
{
    if (irow != irow_default)
    {
	if (cla_free((char *) irow))
	{
	    s_error ("error freeing row buffer memory");
	    return (2);
	};
	irow = irow_default;
	if (cla_free((char *) rrow))
	{
	    s_error ("error freeing row buffer memory");
	    return (2);
	};
	rrow = rrow_default;
	if (cla_free((char *) grow))
	{
	    s_error ("error freeing row buffer memory");
	    return (2);
	};
	grow = grow_default;
	if (cla_free((char *) brow))
	{
	    s_error ("error freeing row buffer memory");
	    return (2);
	};
	brow = brow_default;

	rowsize = rowmax;
    };

    return (1);
}


/*
 * This routine checks to see if sufficient row buffer memory is
 * available, allocating more as necessary.
 */
static int cla_checkrowsize(newsize)
    int             newsize;
{
    if (newsize > rowsize)
    {
	if (!cla_resetrowsize())
	    exit(2);

	if ((irow = (int *)
	     malloc((unsigned int) newsize * sizeof(int))) == 0)
	{
	    s_error ("error allocating row buffer memory");
	    exit(2);
	}
	if ((rrow = (float *)
	     malloc((unsigned int) newsize * sizeof(float))) == 0)
	{
	    s_error ("error allocating row buffer memory");
	    exit(2);
	}
	if ((grow = (float *)
	     malloc((unsigned int) newsize * sizeof(float))) == 0)
	{
	    s_error ("error allocating row buffer memory");
	    exit(2);
	}
	if ((brow = (float *)
	     malloc((unsigned int) newsize * sizeof(float))) == 0)
	{
	    s_error ("error allocating row buffer memory");
	    exit(2);
	}
	rowsize = newsize;

    };

    return (1);
}



/*
 * This routine stores the current global polygon attributes in a
 * temporary structure.
 */
static int cla_sv_pattr(poldattr)
    fa_attr *poldattr;
{
    poldattr->int_style = glbl5->int_style;
    poldattr->edge_vis = glbl5->edge_vis;
    poldattr->fill_colour.red = glbl5->fill_colour.red;
    poldattr->fill_colour.green = glbl5->fill_colour.green;
    poldattr->fill_colour.blue = glbl5->fill_colour.blue;
    poldattr->fill_colour.ind = glbl5->fill_colour.ind;

    glbl5->int_style = solid_i;
    if (*(attr + (int) IntStyle))
	(**(attr + (int) IntStyle)) (glbl5->int_style);
    glbl5->edge_vis = off;
    if (*(attr + (int) EdVis))
	(**(attr + (int) EdVis)) (glbl5->edge_vis);

    return (1);
}


static unsigned char  *cla_dorow(origin, alpha, beta, nx, prec, image_ptr, mode)
    int             nx, prec, mode;
    float           origin[2], alpha[2], beta[2];
    unsigned char  *image_ptr;
{
    float           topleft[2], topright[2], botleft[2], botright[2], *clrptr;
    int             xlist[4], ylist[4], i, inow, ilast;

    /* get color values and indices */

    if (glbl2->c_s_mode)	       /* Direct color */
    {
	image_ptr = cla_dc_row(image_ptr, nx, rrow, grow,
			       brow, prec, mode);
	for (i = 0; i < nx; i++)
	    irow[i] = -1;
    } else
	/* Indexed color */
    {
	image_ptr = cla_i_row(image_ptr, nx, irow,
			      prec, mode);
	for (i = 0; i < nx; i++)
	{
	    clrptr = glbl5->ctab + 3 * irow[i];
	    rrow[i] = *clrptr++;
	    grow[i] = *clrptr++;
	    brow[i] = *clrptr;
	};
    };

    topleft[0] = origin[0];
    topleft[1] = origin[1];
    mcr_addvec(botleft, topleft, beta);
    mcr_addvec(topright, topleft, alpha);
    mcr_addvec(botright, botleft, alpha);
    ilast = 0;
    inow = 0;
    for (i = 0; i++ < nx;)
    {
	inow++;

	if ((i < nx) &&
	    (irow[inow] == irow[ilast]) &&
	    (rrow[inow] == rrow[ilast]) &&
	    (grow[inow] == grow[ilast]) &&
	    (brow[inow] == brow[ilast]))
	{
	    mcr_addvec(topright, topright, alpha);
	    mcr_addvec(botright, botright, alpha);
	} else
	{
	    xlist[0] = (int) topleft[0];
	    ylist[0] = (int) topleft[1];
	    xlist[1] = (int) topright[0];
	    ylist[1] = (int) topright[1];
	    xlist[2] = (int) botright[0];
	    ylist[2] = (int) botright[1];
	    xlist[3] = (int) botleft[0];
	    ylist[3] = (int) botleft[1];
	    /* Call color and polygon routines */
	    glbl5->fill_colour.red = rrow[ilast];
	    glbl5->fill_colour.green = grow[ilast];
	    glbl5->fill_colour.blue = brow[ilast];
	    glbl5->fill_colour.ind = irow[ilast];
	    if (*(attr + (int) FillColour))
		(**(attr + (int) FillColour)) (rrow[ilast],
			       grow[ilast], brow[ilast], irow[ilast]);
	    (**(gprim + (int) Polygon)) (4, xlist, ylist);

	    ilast = inow;
	    topleft[0] = topright[0];
	    topleft[1] = topright[1];
	    botleft[0] = botright[0];
	    botleft[1] = botright[1];
	    mcr_addvec(topright, topleft, alpha);
	    mcr_addvec(botright, botleft, alpha);
	};
    };

    return (image_ptr);

}

/*
 * This routine stores the current global polygon attributes in a
 * temporary structure.
 */
static int cla_rs_pattr(poldattr)
    fa_attr *poldattr;
{
    glbl5->int_style = poldattr->int_style;
    glbl5->edge_vis = poldattr->edge_vis;
    glbl5->fill_colour.red = poldattr->fill_colour.red;
    glbl5->fill_colour.green = poldattr->fill_colour.green;
    glbl5->fill_colour.blue = poldattr->fill_colour.blue;
    glbl5->fill_colour.ind = poldattr->fill_colour.ind;

    if (*(attr + (int) IntStyle))
	(**(attr + (int) IntStyle)) (glbl5->int_style);
    if (*(attr + (int) EdVis))
	(**(attr + (int) EdVis)) (glbl5->edge_vis);
    if (*(attr + (int) FillColour))
	(**(attr + (int) FillColour))
	    (glbl5->fill_colour.red, glbl5->fill_colour.green,
	     glbl5->fill_colour.blue, glbl5->fill_colour.ind);

    return (1);
}



/*
 * Draw a cell array using polygon primitives.  The acronym is 'cell
 * array- polygon-ballback'.
 */
static int cla_p_fb(p, q, r, nx, ny, prec, image_ptr, mode)
    int             p[2], q[2], r[2], nx, ny, prec, mode;
    unsigned char  *image_ptr;
{
    float           topleft[2], alpha[2], beta[2];
    int             j;
    fa_attr	    oldattr;

    /* Check for presence of needed routines */
    if (!*(gprim + (int) Polygon))
    {
	s_error ("cla_l_fb failed for lack of polygon or fill color routine");
	return (8);
    };

    /*
     * Check availability of row data buffers, and fix if necessary.
     */
    if (!cla_checkrowsize(nx))
	exit(2);

    /* Save the current fill attributes, and set appropriate new ones */
    if (!cla_sv_pattr(&oldattr))
	exit(2);

    /* Set up step sizes */
    alpha[0] = ((float) (r[0] - p[0])) / nx;
    alpha[1] = ((float) (r[1] - p[1])) / nx;
    beta[0] = ((float) (q[0] - r[0])) / ny;
    beta[1] = ((float) (q[1] - r[1])) / ny;

    /* Set initial point */
    topleft[0] = (float) p[0];
    topleft[1] = (float) p[1];

    for (j = 0; j < ny; j++)
    {

	/* actually output the row */
	image_ptr = cla_dorow(topleft, alpha, beta, nx, prec,
			      image_ptr, mode);

	/* Advance to next row */
	mcr_addvec(topleft, topleft, beta);

    };

    /* Reset the fill attributes (this includes some driver calls) */
    if (!cla_rs_pattr(&oldattr))
	exit(2);

    /* Reset the row buffers, if they were changed. */
    return (cla_resetrowsize());
}




/*
 * Set the clipping rectangle
 */
static int 
gks_cliprect(cor_int, cor_float)
    int            *cor_int;
    float          *cor_float;
{
    float           x1, y1, x2, y2, d;


    switch (State_c1->vdc_type)
    {
    case vdc_int:
	{
            if (State_c2->vdc_extent.i[2] - State_c2->vdc_extent.i[0] >
                State_c2->vdc_extent.i[3] - State_c2->vdc_extent.i[1])
                d = (float) (State_c2->vdc_extent.i[2] -
                    State_c2->vdc_extent.i[0]);
            else
                d = (float) (State_c2->vdc_extent.i[3] -
                    State_c2->vdc_extent.i[1]);

	    x1 = cor_int[0] / d;
	    y1 = cor_int[1] / d;
	    x2 = cor_int[2] / d;
	    y2 = cor_int[3] / d;
	    break;
	}
    case vdc_real:
	{
            if (State_c2->vdc_extent.r[2] - State_c2->vdc_extent.r[0] >
                State_c2->vdc_extent.r[3] - State_c2->vdc_extent.r[1])
                d = (State_c2->vdc_extent.r[2] - State_c2->vdc_extent.r[0]);
            else
                d = (State_c2->vdc_extent.r[3] - State_c2->vdc_extent.r[1]);

	    x1 = cor_float[0] / d;
	    y1 = cor_float[1] / d;
	    x2 = cor_float[2] / d;
	    y2 = cor_float[3] / d;
	    break;
	}
    }
#ifndef NO_SIGHT
    if (!sight)
	{
#endif
        GSVP(&wc, &x1, &x2, &y1, &y2);
	GSWN(&wc, &x1, &x2, &y1, &y2);
#ifndef NO_SIGHT
	}
    else
	{
	SightSetViewport(x1, x2, y1, y2);
	SightSetWindow(x1, x2, y1, y2);
	}
#endif
    return (1);
}



/*
 * Set workstation window/viewport
 */

static int
gks_vdc_extent(cor_int, cor_float)
    int            *cor_int;
    float          *cor_float;
{
    switch (State_c1->vdc_type)
    {
    case vdc_int:
	{
            if (cor_int[2] - cor_int[0] > cor_int[3] - cor_int[1])
                wn_y1 = (float) (cor_int[3] - cor_int[1]) /
                    (cor_int[2] - cor_int[0]);
            else
                wn_x1 = (float) (cor_int[2] - cor_int[0]) /
                    (cor_int[3] - cor_int[1]);
	    break;
	}
    case vdc_real:
	{
            if (cor_float[2] - cor_float[0] > cor_float[3] - cor_float[1])
                wn_y1 = (float) (cor_float[3] - cor_float[1]) /
                    (cor_float[2] - cor_float[0]);
            else
                wn_x1 = (float) (cor_float[2] - cor_float[0]) /
                    (cor_float[3] - cor_float[1]);
	    break;
	}
    }

    return (1);
}



/*
 * Set clipping on/off
 */
static int 
gks_clipindic(clip)
    int             clip;
{
    int             clsw;

    clsw = clip;

#ifndef NO_SIGHT
    if (!sight)
	{
#endif
	GSCLIP(&clsw);
#ifndef NO_SIGHT
	}
    else
	{
	SightSetClipping (clsw);
	}
#endif

    return (1);
}

/*
 * Set the text colour.
 */
static int 
gks_t_colour(r, g, b, index)
    float           r, g, b;
    int             index;
{
    int             rc = 1;
    int             coli;

    if (State_c2->c_s_mode == d_c_mode)/* Direct color */
    {
	if (dc_ready_flag)
	    index = best_clr[
			   pack_clr(f_to_b(r), f_to_b(g), f_to_b(b))];
	else
	{
	    s_error ("direct text color with indexed color set");
	    rc = 4;
	    index = 1;
	}
    } else
	/* Indexed color */
    {
	if (dc_ready_flag)
	{
	    s_error ("indexed text color with direct color set");
	    rc = 4;
	};
    };
    coli = index;

#ifndef NO_SIGHT
    if (!sight)
	{
#endif
	GSTXCI(&coli);
#ifndef NO_SIGHT
	}
    else
	{
	SightSetAttribute (TextColor, (caddr_t)&coli);
	}
#endif


    return (rc);
}

/*
 * Write text.
 */
static int 
gks_text(x, y, final, buffer)
    int             x, y;
    enum bool_enum  final;
    char           *buffer;
{
    float           xloc, yloc;
    int             str_l;
    char            str[max_str];
#ifdef VMS
    struct dsc$descriptor_s text;
#endif /* VMS */
#ifdef cray
    _fcd text;
#endif /* cray */

    str_l = 0;
    while (*buffer && str_l < max_str)
        {
        if (isprint(*buffer & 0177))
            str[str_l++] = *buffer++;
        else
            ++buffer;
        }
    str[str_l] = '\0';

    xloc = x * xscale;
    yloc = y * yscale;

#ifdef VMS
    text.dsc$b_dtype = DSC$K_DTYPE_T;
    text.dsc$b_class = DSC$K_CLASS_S;
    text.dsc$w_length = str_l;
    text.dsc$a_pointer = str;

#ifndef NO_SIGHT
    if (!sight)
#endif
	GTXS(&xloc, &yloc, &str_l, &text);
#ifndef NO_SIGHT
    else
	SightSimpleText (xloc, yloc, str);
#endif
#else
#ifdef cray
    text = _cptofcd(str, str_l);

#ifndef NO_SIGHT
    if (!sight)
#endif
	GTXS(&xloc, &yloc, &str_l, text);
#ifndef NO_SIGHT
    else
	SightSimpleText (xloc, yloc, str);
#endif
#else
#ifndef NO_SIGHT
    if (!sight)
#endif
#if defined(_WIN32) && !defined(__GNUC__)
	GTXS(&xloc, &yloc, &str_l, str, (unsigned short)str_l);
#else
	GTXS(&xloc, &yloc, &str_l, str, str_l);
#endif /* _WIN32 */
#ifndef NO_SIGHT
    else
	SightSimpleText (xloc, yloc, str);
#endif
#endif /* cray */
#endif /* VMS */

    page_empty = 0;

    return (1);
}

static int 
gks_rex_text(width, height, x, y, final, buffer)
    int             width, height;
    int             x, y;
    enum bool_enum  final;
    char           *buffer;
{
    return (gks_text(x, y, final, buffer));
}

/*
 * Set the text alignment.
 */
static int 
gks_t_align(hor, ver, cont_hor, cont_ver)
    enum hor_align  hor;
    enum ver_align  ver;
    float           cont_hor, cont_ver;
{
    int             ih, iv;

    ih = (enum hor_align)hor;
    iv = (enum ver_align)ver;

#ifndef NO_SIGHT
    if (!sight)
	{
#endif
	GSTXAL(&ih, &iv);
#ifndef NO_SIGHT
	}
    else
	{
	SightSetAttribute (TextHalign, (caddr_t)&hor);
	SightSetAttribute (TextValign, (caddr_t)&ver);
	}
#endif

    return (1);
}



/*
 * Set gks text upvec
 */
static int 
gks_t_upvec(xup, yup, xbas, ybas)
    int             xup, yup, xbas, ybas;
{
    float           xupv, yupv;
#ifndef NO_SIGHT
    float           angle, Pi= 4.0 * atan (1.0);
    int		    path;
#endif
    xupv = xscale * xup;
    yupv = yscale * yup;

#ifndef NO_SIGHT
    if (!sight)
	{
#endif
	GSCHUP(&xupv, &yupv);
#ifndef NO_SIGHT
	}
    else
	{
        angle = (int)(-atan2(xupv,yupv)*180/Pi + 0.5);
	if (angle < 0) angle += 360;
	path = (angle+45) / 90;
	SightSetAttribute (TextDirection, (caddr_t)&path);
	}
#endif

    return (1);
}


/*
 * This function checks that sufficient memory is available for
 * coordinate storage, allocating more as needed.  Returns 1 if
 * successful.
 */
static int 
gks_getcmem(nx, ny)
    int             nx, ny;
{

    if (nx > xbuf_sz)
    {
	if (xbuf_sz > 0)
	{
	    free(xbuf);
	    xbuf_sz = 0;
	};
	if ((xbuf = (float *) malloc(nx * sizeof(float))) == 0)
	{
	    s_error ("unable to allocate x memory");
	    return (2);
	};
	xbuf_sz = nx;
    };
    if (ny > ybuf_sz)
    {
	if (ybuf_sz > 0)
	{
	    free(ybuf);
	    ybuf_sz = 0;
	};
	if ((ybuf = (float *) malloc(ny * sizeof(float))) == 0)
	{
	    s_error ("unable to allocate y memory");
	    return (2);
	};
	ybuf_sz = ny;
    };
    return (1);
}

/*
 * This function checks that sufficient memory is available for image
 * storage, allocating more as needed.  Returns 1 if successful.
 */
static int 
gks_getimem(isize)
    int             isize;
{

    if (isize > ibuf_sz)
    {
	if (ibuf_sz > 0)
	{
	    free(ibuf);
	    ibuf_sz = 0;
	};
	if ((ibuf = (int *) malloc(isize * sizeof(int))) == 0)
	{
	    s_error ("unable to allocate image memory");
	    return (2);
	};
	ibuf_sz = isize;
    };
    return (1);
}


/*
 * Plot a set of lines.
 */
static int 
gks_pline(no_pairs, x1_ptr, y1_ptr)
    int             no_pairs, *x1_ptr, *y1_ptr;
{
    int             i, *x1_ptr_cpy = x1_ptr, *y1_ptr_cpy = y1_ptr;
    float          *xcpy, *ycpy;

    if (no_pairs <= 1)
	return (1);

    if (gks_getcmem(no_pairs, no_pairs) != 1)
    {
	s_error ("error allocating memory for pline buffer");
	return (2);
    };

    xcpy = xbuf;
    ycpy = ybuf;
    for (i = 0; i < no_pairs; i++)
    {
	*xcpy++ = xscale * *x1_ptr_cpy++;
	*ycpy++ = yscale * *y1_ptr_cpy++;
    };

#ifndef NO_SIGHT
    if (!sight)
	{
#endif
	GPL(&no_pairs, xbuf, ybuf);
#ifndef NO_SIGHT
	}
    else
	{
	SightPolyline (no_pairs, xbuf, ybuf);
	}
#endif

    page_empty = 0;

    return (1);
}

/* plot a set of lines between alternate points */
static int 
gks_dpline(no_pairs, x1_ptr, y1_ptr)
    int             no_pairs, *x1_ptr, *y1_ptr;
{
    int             i, two = 2;
    float           x[2], y[2];

    if (no_pairs <= 1)
	return (1);

    for (i = 0; i < no_pairs; i += 2)
    {
	x[0] = x1_ptr[i] * xscale;
	x[1] = x1_ptr[i + 1] * xscale;
	y[0] = y1_ptr[i] * yscale;
	y[1] = y1_ptr[i + 1] * yscale;
#ifndef NO_SIGHT
	if (!sight)
	    {
#endif
	    GPL(&two, x, y);
#ifndef NO_SIGHT
	    }
	else
	    {
	    SightPolyline (two, x, y);
	    }
#endif
    };

    page_empty = 0;

    return (1);
}

/*
 * Set the marker colour.
 */
static int 
gks_mk_colour(r, g, b, index)
    float           r, g, b;
    int             index;
{
    int             rc = 1;
    int             coli;

    if (State_c2->c_s_mode == d_c_mode)/* Direct color */
    {
	if (dc_ready_flag)
	    index = best_clr[
			   pack_clr(f_to_b(r), f_to_b(g), f_to_b(b))];
	else
	{
	    s_error ("direct marker color with indexed color set");
	    rc = 4;
	    index = 1;
	}
    } else
	/* Indexed color */
    {
	if (dc_ready_flag)
	{
	    s_error ("indexed marker color with direct color set");
	    rc = 4;
	};
    };
    coli = index;

#ifndef NO_SIGHT
    if (!sight)
	{
#endif
	GSPMCI(&coli);
#ifndef NO_SIGHT
	}
    else
	{
	SightSetAttribute (MarkerColor, (caddr_t)&coli);
	}
#endif


    return (rc);
}

/*
 * Set marker size.
 */
static int 
gks_mk_size(mk_a_size, mk_s_size)
    int             mk_a_size;
    float           mk_s_size;
{
    float           mksize;
#ifndef NO_SIGHT
    float	    pitch;
#endif
    mksize = mk_s_size;
#ifndef NO_SIGHT
    if (!sight)
	{
#endif
	GSMKSC(&mksize);
#ifndef NO_SIGHT
	}
    else
	{
	pitch = mksize * NominalMarkerSize;
	SightSetAttribute (MarkerSize, (caddr_t)&pitch);
	}
#endif

    return (1);
}

/*
 * Set marker type.
 */
static int 
gks_mk_type(marker)
    int             marker;
{
    int             type;

    type = (marker > MAX_MARKERTYPES) ? MAX_MARKERTYPES - marker : marker;

#ifndef NO_SIGHT
    if (!sight)
	{
#endif
	GSMK(&type);
#ifndef NO_SIGHT
	}
    else
	{
	SightSetAttribute (MarkerType, (caddr_t)&type);
	}
#endif


    return (1);
}

/*
 * Put up a series of markers.
 */
static int 
gks_pmarker(no_pairs, x1_ptr, y1_ptr)
    int             no_pairs, *x1_ptr, *y1_ptr;
{
    int             i, *x1_ptr_cpy, *y1_ptr_cpy;
    float          *xcpy, *ycpy;

    x1_ptr_cpy = x1_ptr;
    y1_ptr_cpy = y1_ptr;

    if (no_pairs < 1)
	return (1);

    if (gks_getcmem(no_pairs, no_pairs) != 1)
    {
	s_error ("error allocating memory for pmarker buffer");
	return (2);
    };

    xcpy = xbuf;
    ycpy = ybuf;
    for (i = 0; i < no_pairs; i++)
    {
	*xcpy++ = xscale * *x1_ptr_cpy++;
	*ycpy++ = yscale * *y1_ptr_cpy++;
    };

#ifndef NO_SIGHT
    if (!sight)
	{
#endif
	GPM(&no_pairs, xbuf, ybuf);
#ifndef NO_SIGHT
	}
    else
	{
	SightPolymarker (no_pairs, xbuf, ybuf);
	}
#endif

    page_empty = 0;

    return (1);
}

/*
 * Set the text font.
 */
static int 
gks_t_font(index)
    int             index;
{
    int             errind, dummy, font, prec;

    font = index;

#ifndef NO_SIGHT
    if (!sight)
	{
#endif
	GQTXFP(&errind, &dummy, &prec);
	GSTXFP(&font, &prec);
#ifndef NO_SIGHT
	}
    else
	{
        if (font < 0)
	    {
	    GQTXFP(&errind, &dummy, &prec);
	    GSTXFP(&dummy, &GSTRKP);
	    }
	SightSetAttribute (TextFont, (caddr_t)&font);
	}
#endif

    return (1);
}

/*
 * Set the text precision.
 */
static int 
gks_t_prec(index)
    int             index;
{
    int             errind, dummy, font, prec;

    prec = index;

#ifndef NO_SIGHT
    if (!sight)
#endif
	{
	GQTXFP(&errind, &font, &dummy);
	GSTXFP(&font, &prec);
	}

    return (1);
}

/*
 * Set character height, and set Dev_info->c_height and c_width to the
 * values which result.
 */
static int 
gks_c_height(t_a_height)
    int             t_a_height;
{
    float           height;
#ifndef NO_SIGHT
    float           size;
#endif

    height = t_a_height * yscale;

#ifndef NO_SIGHT
    if (!sight)
	{
#endif
        GSCHH(&height);
#ifndef NO_SIGHT
	}
    else
	{
        size = height * WindowHeight / CapSize * Ratio;
	SightSetAttribute (TextSize, (caddr_t)&size);
	}
#endif


    /*
     * Can't actually check character height and width now in effect,
     * but a good guess is the next height down from the one requested
     * (if possible).
     */
    if (t_a_height < c_h_min)
	Dev_info->c_height = c_h_min;
    else if (t_a_height > c_h_max)
	Dev_info->c_height = c_h_max;
    else if (num_c_heights == 0)
	Dev_info->c_height = t_a_height;
    else
	Dev_info->c_height =
	    (num_c_heights * (t_a_height - c_h_min))
	    / (c_h_max - c_h_min)
	    + c_h_min;
    /* Assume square characters */
    Dev_info->c_width = Dev_info->c_height;

    return (1);
}

/*
 * Set filled area interior style.
 */
static int 
gks_fl_style(style)
    enum is_enum    style;
{
    int             istyle;

    switch ((int) style)
    {
    case (int) hollow:
	istyle = 0;
	break;
    case (int) solid_i:
	istyle = 1;
	break;
    case (int) pattern:
	istyle = 2;
	break;
    case (int) hatch:
	istyle = 3;
	break;
    case (int) empty:
	istyle = 0;
	break;
    default:
	s_error ("unknown or unsupported interior style; using hollow");
	gks_fl_style(hollow);
	return (2);
    };

#ifndef NO_SIGHT
    if (!sight)
	{
#endif
	GSFAIS(&istyle);
#ifndef NO_SIGHT
	}
    else
	{
	SightSetAttribute (FillStyle, (caddr_t)&istyle);
	}
#endif


    return (1);
}

/*
 * Set polygon fill colour.
 */
static int 
gks_fl_colour(r, g, b, index)
    float           r, g, b;
    int             index;
{
    int             rc = 1;
    int             coli;

    if (State_c2->c_s_mode == d_c_mode)/* Direct color */
    {
	if (dc_ready_flag)
	    index = best_clr[
			   pack_clr(f_to_b(r), f_to_b(g), f_to_b(b))];
	else
	{
	    s_error ("direct fill color with indexed color set");
	    rc = 4;
	    index = 1;
	}
    } else
	/* Indexed color */
    {
	if (dc_ready_flag)
	{
	    s_error ("indexed fill color with direct color set");
	    rc = 4;
	};
    };
    coli = index;

#ifndef NO_SIGHT
    if (!sight)
	{
#endif
	GSFACI(&coli);
#ifndef NO_SIGHT
	}
    else
	{
	SightSetAttribute (FillColor, (caddr_t)&coli);
	}
#endif


    return (rc);
}

/*
 * Set the line type.
 */
static int 
gks_l_type(l_type)
    int             l_type;
{
    int             itype;

    itype = (l_type > MAX_LINETYPES) ? MAX_LINETYPES - l_type : l_type;
#ifndef NO_SIGHT
    if (!sight)
	{
#endif
	GSLN(&itype);
#ifndef NO_SIGHT
	}
    else
	{
	SightSetAttribute (LineType, (caddr_t)&itype);
	}
#endif


    return (1);
}

/*
 * Set the line width.
 */
static int 
gks_l_width(l_a_width, l_s_width)
    int             l_a_width;
    float           l_s_width;
{
    float           lwidth;

    lwidth = l_s_width;
#ifndef NO_SIGHT
    if (!sight)
	{
#endif
	GSLWSC(&lwidth);
#ifndef NO_SIGHT
	}
    else
	{
	SightSetAttribute (LineWidth, (caddr_t)&lwidth);
	}
#endif


    return (1);
}

/*
 * Set the line colour.
 */
static int 
gks_l_colour(r, g, b, index)
    float           r, g, b;
    int             index;
{
    int             rc = 1;
    int             coli;

    if (State_c2->c_s_mode == d_c_mode)/* Direct color */
    {
	if (dc_ready_flag)
	    index = best_clr[
			   pack_clr(f_to_b(r), f_to_b(g), f_to_b(b))];
	else
	{
	    s_error ("direct line color with indexed color set");
	    rc = 4;
	    index = 1;
	}
    } else
	/* Indexed color */
    {
	if (dc_ready_flag)
	{
	    s_error ("indexed line color with direct color set");
	    rc = 4;
	};
    };
    coli = index;

#ifndef NO_SIGHT
    if (!sight)
	{
#endif
	GSPLCI(&coli);
#ifndef NO_SIGHT
	}
    else
	{
	SightSetAttribute (LineColor, (caddr_t)&coli);
	}
#endif


    return (rc);
}


/*
 * Draw a polygon.
 */
static int 
gks_pgon(no_pairs, x1_ptr, y1_ptr)
    int             no_pairs, *x1_ptr, *y1_ptr;
{
    int             i, rc, *x1_ptr_cpy = x1_ptr, *y1_ptr_cpy = y1_ptr;
    float          *xcpy, *ycpy;

    if (no_pairs <= 1)
	return (1);

    if (gks_getcmem(no_pairs + 1, no_pairs + 1) != 1)
    {
	s_error ("error allocating memory for pgon buffer");
	return (2);
    };

    xcpy = xbuf;
    ycpy = ybuf;
    for (i = 0; i < no_pairs; i++)
    {
	*xcpy++ = xscale * *x1_ptr_cpy++;
	*ycpy++ = yscale * *y1_ptr_cpy++;
    };

    /*
     * Make another copy of the initial points, to close the polyline
     * in case edge visibility is set.
     */
    *xcpy = *xbuf;
    *ycpy = *ybuf;

#ifndef NO_SIGHT
    if (!sight)
	{
#endif
	GFA(&no_pairs, xbuf, ybuf);
#ifndef NO_SIGHT
	}
    else
	{
	SightFillArea (no_pairs, xbuf, ybuf);
	}
#endif

    page_empty = 0;

    /* Draw edges if appropriate */
    if (State_c5->edge_vis)
    {
	rc = gks_l_type(State_c5->edge_type);
	if (!rc)
	    return (rc);
	rc = gks_l_width(State_c5->edge_width.i, State_c5->edge_width.r);
	if (!rc)
	    return (rc);
	rc = gks_l_colour(State_c5->edge_colour.red,
			  State_c5->edge_colour.green,
			  State_c5->edge_colour.blue,
			  State_c5->edge_colour.ind);
	if (!rc)
	    return (rc);

	no_pairs = no_pairs + 1;
#ifndef NO_SIGHT
	if (!sight)
	    {
#endif
	    GPL(&no_pairs, xbuf, ybuf);
#ifndef NO_SIGHT
	    }
	else
	    {
	    SightPolyline (no_pairs, xbuf, ybuf);
	    }
#endif

        page_empty = 0;

	rc = gks_l_type(State_c5->line_type);
	if (!rc)
	    return (rc);
	rc = gks_l_width(State_c5->line_width.i, State_c5->line_width.r);
	if (!rc)
	    return (rc);
	rc = gks_l_colour(State_c5->line_colour.red,
			  State_c5->line_colour.green,
			  State_c5->line_colour.blue,
			  State_c5->line_colour.ind);
	if (!rc)
	    return (rc);
    };

    return (1);

}

/*
 * This routine implements the copying of an indexed color cell array
 * into the (previously allocated) image memory buffer. Return 1 if
 * successful.  The acronym is 'gks_copy_indexed_ cell_array'.
 */
static int 
gks_cica(nx, ny, prec, mode, image_ptr)
    int             nx, ny, prec, mode;
    unsigned char  *image_ptr;
{
    int             i, j, *cellptr;
    unsigned char  *cla_i_row();
    static int      rowbuf[ROWSIZE];   /* buffer for a row of image
					* data */

    for (j = 0; j < ny; j++)
    {
	image_ptr = cla_i_row(image_ptr, nx, rowbuf, prec, mode);
	cellptr = ibuf + j * nx;
	for (i = 0; i < nx; i++)
	{
	    *cellptr = (rowbuf[i] >= 0) ? rowbuf[i] : 0;
	    cellptr++;
	};
    };

    return (1);
}

/*
 * This routine implements the copying of a direct color cell array
 * into the (previously allocated) image memory buffer. Return 1 if
 * successful.  The acronym is 'gks_copy_direct_ cell_array'.
 */
static int 
gks_cdca(nx, ny, prec, mode, image_ptr)
    int             nx, ny, prec, mode;
    unsigned char  *image_ptr;
{
    int             i, j, pixel, *cellptr;
    unsigned char  *cla_dc_row();
    static float    redbuf[ROWSIZE], greenbuf[ROWSIZE], bluebuf[ROWSIZE];
    /* buffers for a row of image data */

    for (j = 0; j < ny; j++)
    {
	image_ptr = cla_dc_row(image_ptr, nx, redbuf, greenbuf,
			       bluebuf, prec, mode);
	cellptr = ibuf + j;
	for (i = 0; i < nx; i++)
	{
	    pixel = best_clr[pack_clr(
					 f_to_b(redbuf[i]),
					 f_to_b(greenbuf[i]),
					 f_to_b(bluebuf[i]))];
	    *cellptr = (pixel >= 0) ? pixel : 0;
	    cellptr = cellptr + ny;
	};
    };

    return (1);
}


/*
 * Draw a cell array.
 */
static int 
gks_carray(p, q, r, nx, ny, prec, image_ptr, mode, no_bytes)
    int             p[2], q[2], r[2], nx, ny, prec, mode;
    unsigned char  *image_ptr;
    long int        no_bytes;
{
    int             imagesize, one = 1;
    float           px, py, qx, qy;

    /*
     * For skewed or rotated cell arrays, use the polygon fallback
     * routine.
     */
    if ((p[1] != r[1]) || (q[0] != r[0]))
    {
	cla_p_fb(p, q, r, nx, ny, prec, image_ptr, mode);
	return (1);
    };

    /* Check row buffer space availability */
    if (nx > ROWSIZE)
    {
	s_error ("cell array too wide for internal buffer; call ignored");
	return (8);
    };

    /* Make sure buffer memory is available */
    imagesize = nx * ny;
    if (gks_getimem(imagesize) != 1)
    {
	s_error ("error allocating memory for image buffer");
	return (2);
    };

    /* Set up coordinates */
    px = xscale * p[0];
    py = yscale * p[1];
    qx = xscale * q[0];
    qy = yscale * q[1];

    /* Copy the image into the integer image buffer */
    if (State_c2->c_s_mode)	       /* Direct color */
    {
	if (dc_ready_flag)
	    gks_cdca(nx, ny, prec, mode, image_ptr);
	else
	{
	    s_error ("direct color cell array with indexed color set");
	    return (4);
	}
    } else
	/* Indexed color */
    {
	if (!dc_ready_flag)
	    gks_cica(nx, ny, prec, mode, image_ptr);
	else
	{
	    s_error ("indexed color cell array with direct color set");
	    return (4);
	}
    }

    /* Draw the cell array */
    GCA(&px, &py, &qx, &qy, &nx, &ny, &one, &one, &nx, &ny, ibuf);

#ifndef NO_SIGHT
    if (sight)
	{
	s_error ("no SIGHT equivalent for GKS cell array output primitive");
	}
#endif

    page_empty = 0;

    if (ibuf != (int *)0)
	{
	free(ibuf);
	ibuf_sz = 0;
	}

    return (1);
}


/*
 * Get a colour table update here.  Return 1 if successful.
 */
static int 
gks_ctab(beg_index, no_entries, pctab)
    int             beg_index,	       /* beginning index */
		    no_entries;	       /* number of entries to add
					* starting at beg_index */
    float          *pctab;	       /* direct colour array, *(pctab
					* + i*3) is the red entry for
					* the i'th index, followed by g
					* and b */
{
    int             loopmax, index;
    float          *cptr;

    cptr = pctab + 3 * beg_index;
    loopmax = beg_index + no_entries;
    if (loopmax > clr_t_sz)
	loopmax = clr_t_sz;
    for (index = beg_index; index < loopmax; index++)
    {
#ifndef NO_SIGHT
	if (!sight)
 	    {
#endif
	    GSCR(&ws_id, &index, cptr, cptr + 1, cptr + 2);
#ifndef NO_SIGHT
	    }
	else
	    {
	    GSCR(&ws_id, &index, cptr, cptr + 1, cptr + 2);
	    }
#endif

	cptr += 3;
    }

    /*
     * A side effect of this routine is that it leaves GKS unready to
     * simulate direct color;  set flag saying so.
     */
    dc_ready_flag = 0;

    return (1);
}

/* 
	This routine returns the greatest integer less than or equal to
	the cube root of the small integer i.
*/
static int
gks_cube_rt(int i)
{
	int j;
	for (j=0; j<i; j++) if (j*j*j>i) return(j-1);
	return(i);
}

/* 
	This routine returns the greatest integer less than or equal to 
	the square root of the small integer i.
*/

static
int gks_sqr_rt(int i)
{
	int j;
	for (j=0; j<i; j++) if (j*j>i) return(j-1);
	return(i);
}

/*                              
	This routine sets up a color table with which to 'fake' direct color
  	on an indexed color device.  Return 1 if setup was successful.
*/

static int
gks_fake_dc()  
{
	float r,g,b,rtbl,gtbl,btbl,rnxt,gnxt,bnxt;
	int ir,ig,ib,nr,ng,nb,index,itbl,irtbl,igtbl,ibtbl,roff,goff,ctoffset;

	if (dc_init_flag) return(1);

	if (clr_t_sz==0)
		{
		fprintf(stderr," GKS driver not initialized.\n");
		return(2);
		};
	if (clr_t_sz<8)
		{
		fprintf(stderr,
			" Color table too small to fake direct color\n");
		return(2);
		};

	nb= gks_cube_rt(clr_t_sz);
	ng= gks_sqr_rt(clr_t_sz/nb);
	nr= clr_t_sz/(nb*ng);
	dc_totclrs= nr*ng*nb;
	ctoffset= clr_t_sz - dc_totclrs;

	/* Allocate memory for color table and best-color correspondence
		table */
	if ( (clr_tbl= (float *)malloc(3*dc_totclrs*sizeof(float))) 
		== 0 )
		{ 
		fprintf(stderr,
			" Unable to allocate color table memory.\n");
		return(2);
		};
	if ( (best_clr= (int *)malloc(BEST_C_SZ*sizeof(int))) == 0 )
		{ 
		fprintf(stderr,
			" Unable to allocate best-color list memory.\n");
		return(2);
		};

	/* Build the color table */
	itbl= 0;
	for (ir=0; ir<nr; ir++)
		for (ig=0; ig<ng; ig++)
			for (ib=0; ib<nb; ib++)
			   {
		 	   clr_tbl[3*itbl]= ((float) ir)/((float)(nr-1));
		 	   clr_tbl[3*itbl+1]= ((float) ig)/((float)(ng-1));
		 	   clr_tbl[3*itbl+2]= ((float) ib)/((float)(nb-1));
			   itbl++;
			   };             
                         
	/* Build the nearest-color-index table */
	irtbl= 0;  roff= 0;
	rtbl= clr_tbl[0];
	rnxt= clr_tbl[3*ng*nb];
	for ( ir=7; ir<256; ir+=8 )
		{
		r= b_to_f(ir);
		if ( (r-rtbl>rnxt-r) && ( irtbl < nr-1 ) )
		      	{
			irtbl++;  roff += ng*nb;
			rtbl= rnxt;
			rnxt= clr_tbl[ 3*(irtbl+1)*ng*nb ];
			};
		igtbl= 0;  goff= 0;
		gtbl= clr_tbl[3*roff+1];
		gnxt= clr_tbl[3*(roff+nb)+1];
		for ( ig=7; ig<256; ig+=8 )
			{
			g= b_to_f(ig);
			if ( (g-gtbl>gnxt-g) && (igtbl < ng-1 ) )
				{
				igtbl++;  goff += nb;
				gtbl= gnxt;
				gnxt= clr_tbl[ 3*(roff+(igtbl+1)*nb) +1];
				};
			ibtbl= 0;
			btbl= clr_tbl[3*(roff+goff)+2];
			bnxt= clr_tbl[3*(roff+goff+1)+2];
			for ( ib=7; ib<256; ib+=8 )
				{                      
				b= b_to_f(ib);
				if ( (b-btbl>bnxt-b) && ( ibtbl < nb-1 ) )
					{
				     	ibtbl++;
					btbl= bnxt;
					bnxt= clr_tbl[3*(roff+goff+ibtbl+1)+2];
					};
				index= pack_clr(ir,ig,ib);
				best_clr[index]= ctoffset+roff+goff+ibtbl;
			 	};
			};
		};

	dc_init_flag= 1;

	return(1);
}

/*                        
	This routine swaps in the direct color simulation color map
	if it is not currently in effect.  If necessary, the map and
	related data structures are generated by calling gks_fake_dc.
	Returns 1 if successful.
*/
static int 
gks_dc_colors()
{
	int index, rc=1;
	float *cptr;

	if (dc_ready_flag) return(1);

	if ( (dc_init_flag) || ( (rc = gks_fake_dc()) == 1 ) )
	    {
 	    cptr = clr_tbl;
	    for (index = clr_t_sz - dc_totclrs; index < clr_t_sz; index++)
		{
#ifndef NO_SIGHT
		if (!sight)
		    {
#endif
		    GSCR(&ws_id, &index, cptr, cptr + 1, cptr + 2);
#ifndef NO_SIGHT
		    }
		else
		    {
		    GSCR(&ws_id, &index, cptr, cptr + 1, cptr + 2);
		    }
#endif
		cptr+=3;
		};

	    dc_ready_flag= 1;
	    };

	return(rc);
}

static int 
gks_begin(comment, file_name, prog_name)
    char           *comment, *file_name, *prog_name;
{
   
    float           x1, x2, y1, y2;

    x1 = 0.0;
    x2 = 1.0;
    y1 = 0.0;
    y2 = 1.0;


    state_level++;
    if (state_level > 0)
	return (1);		       /* already set up */

#ifndef NO_SIGHT
    if (!sight)
        {
#endif
	GSELNT(&wc);
	GSVP(&wc, &x1, &x2, &y1, &y2);
	GSWN(&wc, &x1, &x2, &y1, &y2);
#ifndef NO_SIGHT
	}
    else
	{
	SightSetUpdate(UpdateConditionally);
	SightSelectXform(wc);
	SightSetViewport(x1, x2, y1, y2);
	SightSetWindow(x1, x2, y1, y2);
	}
#endif

    return (1);
}

static void
new_page()
{
    int             lc_dev_nr = 1, inp_dev_stat, inp_tnr;
    float           px, py;

    if (conid || picture_nr == 0)
        return;

    if (isatty(0))
    {
        GRQLC (&ws_id, &lc_dev_nr, &inp_dev_stat, &inp_tnr, &px, &py);
        if (inp_dev_stat == 0)
            exit (0);
    }
    else if (isatty(1))
        sleep(2);

    GCLRWK (&ws_id, &GALWAY);
    page_empty = 1;
}

/*
 * This routine shuts down the workstation and GKS.
 */
static int 
gks_end(pages_done)
    int             pages_done;
{
#ifndef NO_SIGHT
    SightSetUpdate(UpdateAlways);
#endif
    if (!page_empty)
        new_page();

    state_level--;
    if (state_level > 0)
	return (1);		       /* not yet done */

    if (xbuf_sz > 0)
    {
	free(xbuf);
	xbuf_sz = 0;
    };
    if (ybuf_sz > 0)
    {
	free(ybuf);
	ybuf_sz = 0;
    };

    return (1);
}

static int 
gks_bpage(pic_title, xoff, yoff, rotation, rb, gb, bb, pgnum, xsize, ysize)
    char           *pic_title;
    float           rotation, rb, gb, bb;
    int             xoff, yoff, pgnum;
    int             xsize, ysize;
{
   
  
    int             rc;

    if (state_level > 0)
	return (1);		       /* The page is already set up */

    if (!page_empty)
        new_page();

    /*
     * Set proper color selection mode for this frame, skipping
     * index 0 (just set) if it's direct color
     */
    if (State_c2->c_s_mode)         /* Direct color */
        {
        if ( !dc_ready_flag ) rc = gks_dc_colors();
        else rc= 1;
        }
    else            /* Indexed color - load color table */
	rc = gks_ctab(1, State_c1->max_c_index, State_c5->ctab);

    /* Reset a number of attributes to their proper values */
    rc = gks_t_colour(State_c5->text_colour.red, State_c5->text_colour.green,
	       State_c5->text_colour.blue, State_c5->text_colour.ind);
    if (!rc)
	return (rc);
    rc = gks_t_align(State_c5->text_align.hor, State_c5->text_align.ver,
	State_c5->text_align.cont_hor, State_c5->text_align.cont_ver);
    if (!rc)
	return (rc);
    rc = gks_c_height(State_c5->c_height);
    if (!rc)
	return (rc);
    rc = gks_t_font(State_c5->t_f_index);
    if (!rc)
	return (rc);
    rc = gks_t_prec(State_c5->t_prec);
    if (!rc)
	return (rc);
    rc = gks_mk_colour(State_c5->mk_colour.red, State_c5->mk_colour.green,
		   State_c5->mk_colour.blue, State_c5->mk_colour.ind);
    if (!rc)
	return (rc);
    rc = gks_mk_size(State_c5->mk_size.i, State_c5->mk_size.r);
    if (!rc)
	return (rc);
    rc = gks_mk_type(State_c5->mk_type);
    if (!rc)
	return (rc);
    rc = gks_fl_colour(State_c5->fill_colour.red, State_c5->fill_colour.green,
	       State_c5->fill_colour.blue, State_c5->fill_colour.ind);
    if (!rc)
	return (rc);
    rc = gks_fl_style(State_c5->int_style);
    if (!rc)
	return (rc);
    rc = gks_l_type(State_c5->line_type);
    if (!rc)
	return (rc);
    rc = gks_l_width(State_c5->line_width.i, State_c5->line_width.r);
    if (!rc)
	return (rc);
    rc = gks_l_colour(State_c5->line_colour.red, State_c5->line_colour.green,
	       State_c5->line_colour.blue, State_c5->line_colour.ind);

    return (rc);
}

/*
 * This routine ends the page.
 */
static int 
gks_epage(copies)
    int             copies;
{
#ifndef NO_SIGHT
    if (!sight)
#endif
	GUWK(&ws_id, &GPERFO);	

    return (1);
}





/*
 * This routine sets up the interface to the GKS driver.
 * It appears at the end of the module so that it will have access to
 * the symbolic names of functions defined so far.
 */
static void 
gks_setup(pOp, pDev_info, pc1, pc2, pc3, pc5, pdelim, mfdesc, pdesc,
	  mfctrl, pgprim, pattr, escfun, extfun, ctrl)
    struct one_opt *pOp;	       /* the command line options, in
					* only */
    struct info_struct *pDev_info;     /* device info to fill out, out
					* only */
    struct mf_d_struct *pc1;	       /* the class 1 elements, in only */
    struct pic_d_struct *pc2;	       /* the class 2 elements, in only */
    struct control_struct *pc3;	       /* the class 3 elements, in only */
    struct attrib_struct *pc5;	       /* the class 5 elements, in only */
    int             (*pdelim[]) ();    /* delimiter functions, out only */
    int             (*mfdesc[]) ();    /* metafile descriptor functions */
    int             (*pdesc[]) ();     /* page descriptor functions */
    int             (*mfctrl[]) ();    /* metafile control functions */
    int             (*pgprim[]) ();    /* graphical primitives, out
					* only */
    int             (*pattr[]) ();     /* the attribute functions, out
					* only */
    int             (*escfun[]) ();    /* the escape functions */
    int             (*extfun[]) ();    /* the external functions */
    int             (*ctrl[]) ();      /* controller functions */
{
    int             err, wstype, dcunit, raster_x, raster_y;
    float           nom_l_width, nom_m_size,
		    h_min, h_max;

    /* Initialize the cell array utilities */
    cla_init(pc1, pc2, pc5, pgprim, pattr);

    /* store the CGM data structure and device info pointers */
    State_c1 = pc1;
    State_c2 = pc2;
    State_c5 = pc5;
    Dev_info = pDev_info;

    /* now fill out the function pointer arrays for CGM */
    /* the delimiter functions */
    pdelim[(int) B_Mf] = (int (*) ()) gks_begin;
    pdelim[(int) E_Mf] = gks_end;
    pdelim[(int) B_Pic_Body] = gks_bpage;
    pdelim[(int) E_Pic] = gks_epage;

    /* the page descriptor elements */
    pdesc[(int) vdcExtent] = gks_vdc_extent;

    /* the graphical primitives */
    pgprim[(int) PolyLine] = gks_pline;
    pgprim[(int) Dis_Poly] = gks_dpline;
    pgprim[(int) PolyMarker] = gks_pmarker;
    pgprim[(int) TeXt] = gks_text;
    pgprim[(int) Rex_Text] = gks_rex_text;
    pgprim[(int) Polygon] = gks_pgon;
    pgprim[(int) Cell_Array] = gks_carray;

    /* the attributes */
    pattr[(int) LType] = gks_l_type;
    pattr[(int) LWidth] = gks_l_width;
    pattr[(int) LColour] = gks_l_colour;
    pattr[(int) MColour] = gks_mk_colour;
    pattr[(int) MType] = gks_mk_type;
    pattr[(int) MSize] = gks_mk_size;
    pattr[(int) TFIndex] = gks_t_font;
    pattr[(int) TPrec] = gks_t_prec;
    pattr[(int) TColour] = gks_t_colour;
    pattr[(int) CHeight] = gks_c_height;
    pattr[(int) TAlign] = gks_t_align;
    pattr[(int) COrient] = gks_t_upvec;
    pattr[(int) FillColour] = gks_fl_colour;
    pattr[(int) IntStyle] = gks_fl_style;
    pattr[(int) ColTab] = gks_ctab;


    /* control */
    mfctrl[(int) ClipRect] = gks_cliprect;
    mfctrl[(int) ClipIndic] = gks_clipindic;

    /*
     * fill out the device info structure, as far as now known. Some
     * data will be set later in the routine.
     */
    Dev_info->x_offset = 0.0;
    Dev_info->y_offset = 0.0;
    Dev_info->capability = h_center + v_center + string_text;
    strcpy(Dev_info->out_name, ".GKS");/* This won't be used */
    Dev_info->rec_size = 80;

    Dev_info->pxl_in = 1.0;
    Dev_info->ypxl_in = 1.0;
    Dev_info->x_size = 32767.0;
    Dev_info->y_size = 32767.0;

    GQWKC (&ws_id, &err, &conid, &wstype);
    GQDSP (&wstype, &err, &dcunit, &dev_x, &dev_y, &raster_x, &raster_y);

    /* Set up default line width, and edge width (assumed same) */
    nom_l_width = 1;
    Dev_info->d_l_width = 3.28 * 12.0 * nom_l_width * Dev_info->pxl_in;
    Dev_info->d_e_width = Dev_info->d_l_width;

    /* Set up default marker size */
    nom_m_size = 1;
    Dev_info->d_m_size = 3.28 * 12.0 * nom_m_size * Dev_info->pxl_in;

    /* Set up global list of available fonts */
    h_min = -1000.0;
    h_max = 1000.0;
    /* Rescale min and max heights to pixels and save copies */
    c_h_min = (int) (3.28 * 12.0 * h_min * Dev_info->ypxl_in);
    c_h_max = (int) (3.28 * 12.0 * h_max * Dev_info->ypxl_in);
    /* Character height default is 1% of the default norm. window */
    Dev_info->c_height = (int) (0.01 * Dev_info->y_size * Dev_info->ypxl_in);
    /* Assume square characters to get character width */
    Dev_info->c_width = Dev_info->c_height;

    /* Set up coordinate information (driver static variables). */
    xscale = 1.0 / (Dev_info->x_size * Dev_info->pxl_in);
    yscale = 1.0 / (Dev_info->y_size * Dev_info->ypxl_in);

    state_level = -1;		       /* Just starting driver */

}
/* grab a decimal, optimised, assumed setup O.K., steps pointer */
static
get_decimal(inptr)
    char          **inptr;
{
    register int    ret, i;

    while (**inptr == '0')
	++* inptr;		       /* skip zeros */
    ret = 0;
    while ((**inptr >= '0') && (**inptr <= '9'))
    {
	for (i = 0; (digits[i] != **inptr); ++i);
	ret = 10 * ret + i;
	++*inptr;
    }
    return (ret);
}

/* grab a based integer, steps pointer */
static
get_based(inptr, inbase, outval)
    char          **inptr;
    int             inbase, *outval;
{
#define max_extend 16
    static char     ext_digits[max_extend] =
    {'0', '1', '2', '3', '4', '5', '6', '7',
     '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
    register int    ret, i, is_neg;

    /* is it signed ? */
    if (**inptr == '-')
    {
	is_neg = 1;
	++*inptr;
    } else if (**inptr == '+')
    {
	++*inptr;
	is_neg = 0;
    } else
	is_neg = 0;

    /* quick check that we have a legit number */
    for (i = 0; (i < inbase) && (ext_digits[i] != **inptr); ++i);
    if (i >= inbase)
	return (0);

    ret = 0;
    while (1)
    {
	for (i = 0; (i < inbase) && (ext_digits[i] != **inptr); ++i);
	if (i >= inbase)
	{
	    *outval = (is_neg) ? -ret : ret;
	    return (1);
	} else
	{
	    ret = inbase * ret + i;
	    ++*inptr;
	}
    }
}

/*
 * scan an input string for a CGM clear text integer, steps the pointer
 * fwd
 */
static
my_scan_int(inptr, outval)
    char          **inptr;
    int            *outval;
{
    register int    is_neg, first_int;
#define NUMBER_SIGN '#'
#define MIN_BASE 2
#define MAX_BASE 16

    /* skip spaces */
    while ((**inptr == ' ') && (**inptr))
	++* inptr;
    if (!**inptr)
	return (0);		       /* empty string */

    /* is it signed ? */
    if (**inptr == '-')
    {
	is_neg = 1;
	++*inptr;
    } else if (**inptr == '+')
    {
	++*inptr;
	is_neg = 0;
    } else
	is_neg = 0;

    if ((**inptr < '0') || (**inptr > '9'))
	return (0);		       /* no digits */
    /* now get the first (only ?) integer */
    first_int = (is_neg) ? -get_decimal(inptr) : get_decimal(inptr);

    /* do we have a base ? */
    if ((**inptr == NUMBER_SIGN) && (first_int <= MAX_BASE) &&
	(first_int >= MIN_BASE))
    {				       /* based integer */
	++*inptr;
	return (get_based(inptr, first_int, outval));
    } else
    {
	*outval = first_int;
	return (1);
    }
}


/* some local I/O */
/* return the next integer */
static int 
cc_int(in_ptr)
    char          **in_ptr;
{
    char           *my_ptr;
    int             ret;

    my_ptr = *in_ptr;
    /* simple implementation first */
    if (!my_scan_int(in_ptr, &ret))
    {
	/* terminate this command */
	*my_ptr = term_char;
	return (0);
    }
    while ((**in_ptr) && (**in_ptr == ' '))
	++* in_ptr;
    return (ret);
}
/* get a floating point value */
static
my_scan_float(inptr, outval)
    char          **inptr;
    float          *outval;
{
    register int    i, is_neg, first_int, exponent, neg_exp;
    register float  fract, pwr10;

    /* is it signed ? */
    if (**inptr == '-')
    {
	is_neg = 1;
	++*inptr;
    } else if (**inptr == '+')
    {
	++*inptr;
	is_neg = 0;
    } else
	is_neg = 0;

    /* do we have a legit number ? */
    if (((**inptr > '9') || (**inptr < '0')) && (**inptr != '.'))
	return (0);

    if ((**inptr <= '9') && (**inptr >= '0'))
    {				       /* a number here */
	first_int = get_decimal(inptr);
    } else
	first_int = 0;


    if (**inptr == '.')
    {				       /* explicit point number or
					* scaled real */
	++*inptr;
	pwr10 = 10.0;
	fract = 0.0;
	while ((**inptr >= '0') && (**inptr <= '9'))
	{
	    for (i = 0; (digits[i] != **inptr); ++i);
	    fract += i / pwr10;
	    pwr10 *= 10.0;
	    ++*inptr;
	}
	if (**inptr == 'E')
	{			       /* scaled real */
	    ++*inptr;
	    /* is exponent signed ? */
	    if (**inptr == '-')
	    {
		neg_exp = 1;
		++*inptr;
	    } else if (**inptr == '+')
	    {
		++*inptr;
		neg_exp = 0;
	    } else
		neg_exp = 0;
	    exponent = get_decimal(inptr);
	    pwr10 = 1.0;
	    if (neg_exp)
	    {
		for (i = 0; i < exponent; ++i)
		    pwr10 /= 10.0;
	    } else
	    {
		for (i = 0; i < exponent; ++i)
		    pwr10 *= 10.0;
	    }
	    *outval = (is_neg) ? -(first_int + fract) * pwr10 :
		(first_int + fract) * pwr10;
	    return (1);
	} else
	{			       /* explicit point */
	    *outval = (is_neg) ? -(first_int + fract) : first_int + fract;
	    return (1);
	}
    } else if (**inptr == 'E')
    {				       /* scaled real number, no fract */
	++*inptr;
	/* is exponent signed ? */
	if (**inptr == '-')
	{
	    neg_exp = 1;
	    ++*inptr;
	} else if (**inptr == '+')
	{
	    ++*inptr;
	    neg_exp = 0;
	} else
	    neg_exp = 0;
	exponent = get_decimal(inptr);
	pwr10 = 1.0;
	if (neg_exp)
	{
	    for (i = 0; i < exponent; ++i)
		pwr10 /= 10.0;
	} else
	{
	    for (i = 0; i < exponent; ++i)
		pwr10 *= 10.0;
	}
	*outval = (is_neg) ? -first_int * pwr10 : first_int * pwr10;
	return (1);
    } else
    {				       /* just an integer */
	*outval = (float) (is_neg) ? -first_int : first_int;
	return (1);
    }

}
/* return the next float */
static float 
cc_real(in_ptr)
    char          **in_ptr;
{
    float           ret;

    /* skip spaces */
    while ((**in_ptr == ' ') && (**in_ptr))
	++* in_ptr;
    if (!my_scan_float(in_ptr, &ret))
    {
	return (0);
    }
    while ((**in_ptr) && (**in_ptr == ' '))
	++* in_ptr;
    return (ret);
}
/* get the general vdc value */
static int
cc_vdc(in_ptr)
    char          **in_ptr;
{
    int             xi;
    float           xr;

    switch (glbl1->vdc_type)
    {
    case vdc_int:
	xi = cc_int(in_ptr);
	return ((int) (xi * pxl_vdc));

    case vdc_real:
	xr = cc_real(in_ptr);
	return ((int) (xr * pxl_vdc));
    }
    return (0);			       /* trouble */
}

/* read in an integer point */
static int
get_ipoint(in_ptr, xptr, yptr)
    char          **in_ptr;
    int            *xptr, *yptr;
{
    int             use_paren;

    while (**in_ptr == ' ')
	++* in_ptr;		       /* skip spaces */

    if ((use_paren = (**in_ptr == '(')))
	++* in_ptr;

    while (**in_ptr == ' ')
	++* in_ptr;		       /* skip spaces */

    *xptr = cc_int(in_ptr);

    while (**in_ptr == ' ')
	++* in_ptr;		       /* skip spaces */

    if (**in_ptr == ',')
	++* in_ptr;

    while (**in_ptr == ' ')
	++* in_ptr;		       /* skip spaces */

    *yptr = cc_int(in_ptr);

    while (**in_ptr == ' ')
	++* in_ptr;		       /* skip spaces */

    if ((use_paren) && (**in_ptr == ')'))
    {
	++*in_ptr;

	while (**in_ptr == ' ')
	    ++* in_ptr;		       /* skip spaces */
    }
    return (1);
}
/* read in a real point */
static int
get_rpoint(in_ptr, xptr, yptr)
    char          **in_ptr;
    float          *xptr, *yptr;
{
    int             use_paren;
    while (**in_ptr == ' ')
	++* in_ptr;		       /* skip spaces */
    if ((use_paren = (**in_ptr == '(')))
	++* in_ptr;
    while (**in_ptr == ' ')
	++* in_ptr;		       /* skip spaces */
    *xptr = cc_real(in_ptr);
    while (**in_ptr == ' ')
	++* in_ptr;		       /* skip spaces */
    if (**in_ptr == ',')
	++* in_ptr;
    while (**in_ptr == ' ')
	++* in_ptr;		       /* skip spaces */
    *yptr = cc_real(in_ptr);
    while (**in_ptr == ' ')
	++* in_ptr;		       /* skip spaces */
    if ((use_paren) && (**in_ptr == ')'))
    {
	++*in_ptr;
	while (**in_ptr == ' ')
	    ++* in_ptr;		       /* skip spaces */
    }
    return (1);
}
/* get a general point */
static int
get_vpoint(in_ptr, xptr, yptr)
    char          **in_ptr;
    int            *xptr, *yptr;
{
    int             xi, yi;
    float           xr, yr;

    switch (glbl1->vdc_type)
    {
    case vdc_int:
	get_ipoint(in_ptr, &xi, &yi);
	*xptr = xi * pxl_vdc;
	*yptr = yi * pxl_vdc;
	return (1);
    case vdc_real:
	get_rpoint(in_ptr, &xr, &yr);
	*xptr = xr * pxl_vdc;
	*yptr = yr * pxl_vdc;
	return (1);
    }
    return (0);			       /* trouble */
}

/* the setup procedure */
static int ccgm_setup(full_oname, new_info, new_opt,
	   gl1, gl2, df2, gl3, df3, gl5, df5,
	   pf0, pf1, pf2, pf3, pf4, pf5, pf6, pf7, pfct)
    char           *full_oname;
    struct info_struct *new_info;
    struct one_opt *new_opt;
    struct mf_d_struct *gl1;	       /* the class 1 elements */
    struct pic_d_struct *gl2, *df2;    /* the class 2 elements */
    struct control_struct *gl3, *df3;  /* the class 3 elements */
    struct attrib_struct *gl5, *df5;   /* the class 5 elements */
    int             (**pf0) (), (**pf1) (), (**pf2) (), (**pf3) (), (**pf4) (), (**pf5) (),
		(**pf6) (), (**pf7) (), (**pfct) ();	/* the function pointer
							 * arrays */
{
    /* store globals */
    g_in_name = full_oname;
    dev_info = new_info;
    opt = new_opt;
    glbl1 = gl1;
    glbl2 = gl2;
    dflt2 = df2;
    glbl3 = gl3;
    dflt3 = df3;
    glbl5 = gl5;
    dflt5 = df5;

    delim = pf0;
    mfdesc = pf1;
    pdesc = pf2;
    mfctrl = pf3;
    gprim = pf4;
    attr = pf5;
    escfun = pf6;
    extfun = pf7;
    ctrl = pfct;

    /* make the active pointers refer to the globals */
    a2 = glbl2;
    a3 = glbl3;
    a5 = glbl5;

    return (1);
}




/*
 * function to do any smart type things necessary to get the device
 * ready
 */
static void
get_smart(started, gsptr1, gsptr2, opt, dev_info, cosr, sinr,
	pxl_vdc, xoffset, yoffset, xp0, yp0, xsize, ysize, font_check,
	  font_text, sx, sy)
    int             started;
    struct mf_d_struct *gsptr1;	       /* the class 1 element pointer */
    struct pic_d_struct *gsptr2;       /* the class 2 element pointer */
    struct one_opt *opt;	       /* the command line options, in
					* only */
    struct info_struct *dev_info;      /* device info to fill out, out
					* only */
    double         *cosr, *sinr, *pxl_vdc, *sx, *sy;
    int            *xoffset, *yoffset, *xsize, *ysize;
    float          *xp0, *yp0;
    int             (**font_check) (), (**font_text) ();
{
    float           vdc_y, vdc_x;
    float           vdc_w, vdc_h, x_scale, y_scale;
 
    float           dev_ht, dev_wd;    /* the device parameters */
    double          in_vdc;	       /* conversion from VDC units to
					* inches */
    int             ca_check(), ca_text(), hl_check(), hl_text();	/* font routines */
    float           bl_x, bl_y;	       /* bottom left x and y co-ords */
    char           *namptr = NULL;
    int             to_cgm = 0;
    float	    vp_x0, vp_y0, vp_x1, vp_y1, size;

    /* get the device and VDC sizes */

    dev_wd = dev_info->x_size;
    dev_ht = dev_info->y_size;

    /* now sort out the vdc dimensions */

    switch (gsptr1->vdc_type)
    {
    case vdc_int:
	if (gsptr2->vdc_extent.i[2] > gsptr2->vdc_extent.i[0])
	{
	    vdc_x = gsptr2->vdc_extent.i[2] - gsptr2->vdc_extent.i[0];
	    bl_x = gsptr2->vdc_extent.i[0];
	    *sx = 1.0;
	} else
	{
	    vdc_x = gsptr2->vdc_extent.i[0] - gsptr2->vdc_extent.i[2];
	    bl_x = gsptr2->vdc_extent.i[2];
	    *sx = -1.0;
	}
	if (gsptr2->vdc_extent.i[3] > gsptr2->vdc_extent.i[1])
	{
	    vdc_y = gsptr2->vdc_extent.i[3] - gsptr2->vdc_extent.i[1];
	    bl_y = gsptr2->vdc_extent.i[1];
	    *sy = 1.0;
	} else
	{
	    vdc_y = gsptr2->vdc_extent.i[1] - gsptr2->vdc_extent.i[3];
	    bl_y = gsptr2->vdc_extent.i[1];
	    *sy = -1.0;
	}
	break;
    case vdc_real:
	if (gsptr2->vdc_extent.r[2] > gsptr2->vdc_extent.r[0])
	{
	    vdc_x = gsptr2->vdc_extent.r[2] - gsptr2->vdc_extent.r[0];
	    bl_x = gsptr2->vdc_extent.r[0];
	    *sx = 1.0;
	} else
	{
	    vdc_x = gsptr2->vdc_extent.r[0] - gsptr2->vdc_extent.r[2];
	    bl_x = gsptr2->vdc_extent.r[2];
	    *sx = -1.0;
	}
	if (gsptr2->vdc_extent.r[3] > gsptr2->vdc_extent.r[1])
	{
	    vdc_y = gsptr2->vdc_extent.r[3] - gsptr2->vdc_extent.r[1];
	    bl_y = gsptr2->vdc_extent.r[1];
	    *sy = 1.0;
	} else
	{
	    vdc_y = gsptr2->vdc_extent.r[1] - gsptr2->vdc_extent.r[3];
	    bl_y = gsptr2->vdc_extent.r[1];
	    *sy = -1.0;
	}
	break;
    default:
	s_error ("illegal vdc_type ");
    }

    /* now take care of centering, etc */

    *cosr = 1.0;
    *sinr = 0.0;
    /* these are the real height and width of the vdc image */
    vdc_w = fabs(vdc_x * (*cosr)) + fabs(vdc_y * (*sinr));
    vdc_h = fabs(vdc_x * (*sinr)) + fabs(vdc_y * (*cosr));

    /* now figure out the scaling */

    if ((gsptr2->scale_mode.s_mode == 1))
	scale_factor = gsptr2->scale_mode.m_scaling * 1000 / 3.28 / 2.54;
    else
	scale_factor = 1;

#ifndef NO_SIGHT
    if (!sight)
#endif
        {
        GSWKWN (&ws_id, &wn_x0, &wn_x1, &wn_y0, &wn_y1);

        if (dev_x > dev_y)
            size = dev_y * scale_factor;
        else
            size = dev_x * scale_factor;
	vp_x0 = wn_x0;
	vp_y0 = wn_y0;
	vp_x1 = wn_x1 * size;
	vp_y1 = wn_y1 * size;

        GSWKVP (&ws_id, &vp_x0, &vp_x1, &vp_y0, &vp_y1);
        }

    x_scale = dev_wd / vdc_w;
    y_scale = dev_ht / vdc_h;
    /* which is the smallest ? */
    if (x_scale < y_scale)
    {
	*xsize = dev_wd * dev_info->pxl_in;
	*ysize = dev_ht * (x_scale / y_scale) * dev_info->ypxl_in;
	in_vdc = y_scale = x_scale;
    } else
    {
	*xsize = dev_wd * (y_scale / x_scale) * dev_info->pxl_in;
	*ysize = dev_ht * dev_info->ypxl_in;
	in_vdc = x_scale = y_scale;
    }
    *pxl_vdc = dev_info->pxl_in * in_vdc;

    if (!started)
	return;

    /* get the origin to the right place */
    if ((*cosr >= 0.0) && (*sinr >= 0.0))
    {
	*xoffset = dev_info->pxl_in * in_vdc * vdc_y * (*sinr);
	*yoffset = 0.0;
    } else if ((*cosr < 0.0) && (*sinr >= 0.0))
    {
	*xoffset = dev_info->pxl_in * in_vdc *
	    (vdc_y * (*sinr) - vdc_x * (*cosr));
	*yoffset = -dev_info->ypxl_in * in_vdc * vdc_x * (*cosr);
    } else if ((*cosr < 0.0) && (*sinr < 0.0))
    {
	*xoffset = -dev_info->pxl_in * in_vdc * vdc_x * (*cosr);
	*yoffset = -dev_info->ypxl_in * in_vdc *
	    (vdc_x * (*sinr) + vdc_y * (*cosr));
    } else if ((*cosr >= 0.0) && (*sinr < 0.0))
    {
	*xoffset = 0.0;
	*yoffset = -dev_info->ypxl_in * in_vdc * vdc_x * (*sinr);
    }
    /* and add any externally required offsets */
    *xoffset += dev_info->pxl_in * dev_info->x_offset;
    *yoffset += dev_info->ypxl_in * dev_info->y_offset;

    /* may be able to do some stuff at device level */

    /* if we're going to another CGM file, no internal offsets */
    if (opt[(int) screen].set)
	namptr = opt[(int) screen].val.str;

    to_cgm = namptr &&
	((*namptr == 'C') || (*namptr == 'c')) &&
	((namptr[1] == 'G') || (namptr[1] == 'g')) &&
	((namptr[2] == 'M') || (namptr[2] == 'm'));

    if (to_cgm)
    {				       /* pass everything thru */
	*xp0 = 0;
	*yp0 = 0;
	*sx = 1.0;
	*sy = 1.0;
	*pxl_vdc = 1.0;
    } else if (dev_info->capability & arb_trans)
    {				       /* device can do it */
	*xp0 = -*pxl_vdc * bl_x;
	*yp0 = -*pxl_vdc * bl_y;
    } else
    {
	*xp0 = *xoffset - *pxl_vdc * bl_x;
	*yp0 = *yoffset - *pxl_vdc * bl_y;
    }

    if (dev_info->capability & arb_rot)
    {				       /* device can do any rotn */
	*cosr = 1.0;
	*sinr = 0.0;
    }
    /* now take care of the font functions */
    return;
}





/*
 * function to reset CGM status structures to the correct default
 * values (preumably at the beginning of a new page). The cn variables
 * are the ones to be set, the dn are defaults that may have been
 * changed by a metafile defaults replacement element.
 */

static void rs_defaults(c1, c2, c3, c5, d2, d3, d5, pxlvdc_in)
    struct mf_d_struct *c1;	       /* the class 1 elements */
    struct pic_d_struct *c2, *d2;      /* the class 2 elements */
    struct control_struct *c3, *d3;    /* the class 3 elements */
    struct attrib_struct *c5, *d5;     /* the class 5 elements */
    double          pxlvdc_in;	       /* the pxl_vdc value */
{
    int             i;
    double          vdc_long, vdc_try;

    /* first class2, the  picture descriptor elements */
    c2->scale_mode.s_mode = d2->scale_mode.s_mode;
    c2->scale_mode.m_scaling = d2->scale_mode.m_scaling;
    c2->c_s_mode = d2->c_s_mode;
    c2->l_w_s_mode = d2->l_w_s_mode;
    c2->m_s_s_mode = d2->m_s_s_mode;
    c2->e_w_s_mode = d2->e_w_s_mode;
    for (i = 0; i < 2; ++i)
    {
	c2->vdc_extent.i[i] = d2->vdc_extent.i[i];
	c2->vdc_extent.r[i] = d2->vdc_extent.r[i];
	c2->vdc_extent.i[i + 2] = d2->vdc_extent.i[i + 2];
	c2->vdc_extent.r[i + 2] = d2->vdc_extent.r[i + 2];
    }
    c2->back_col.red = d2->back_col.red;
    c2->back_col.green = d2->back_col.green;
    c2->back_col.blue = d2->back_col.blue;

    /* now the control elements, class 3 */
    c3->vdc_i_prec = d3->vdc_i_prec;
    c3->vdc_r_prec.fixed = d3->vdc_r_prec.fixed;
    c3->vdc_r_prec.exp = d3->vdc_r_prec.exp;
    c3->vdc_r_prec.fract = d3->vdc_r_prec.fract;
    c3->aux_col.red = d3->aux_col.red;
    c3->aux_col.green = d3->aux_col.green;
    c3->aux_col.blue = d3->aux_col.blue;
    c3->aux_col.ind = d3->aux_col.ind;
    c3->transparency = d3->transparency;
    for (i = 0; i < 4; ++i)
    {
	c3->clip_rect.i[i] = d3->clip_rect.i[i];
	c3->clip_rect.r[i] = d3->clip_rect.r[i];
    }
    c3->clip_ind = d3->clip_ind;

    /* class 5, the attribute elements */
    /* these include some that depend on earlier defaults, so first: */
    switch (c1->vdc_type)
    {
    case vdc_int:
	vdc_long = abs(d2->vdc_extent.i[2] - d2->vdc_extent.i[0]);
	vdc_try = abs(d2->vdc_extent.i[3] - d2->vdc_extent.i[1]);
	break;
    case vdc_real:
	vdc_long = fabs(d2->vdc_extent.r[2] - d2->vdc_extent.r[0]);
	vdc_try = fabs(d2->vdc_extent.r[3] - d2->vdc_extent.r[1]);
	break;
    }
    vdc_long = (vdc_long >= vdc_try) ? vdc_long : vdc_try;	/* longest side */

    /* now make the setting */
    c5->l_b_index = d5->l_b_index;
    c5->line_type = d5->line_type;
    c5->line_width.i = pxlvdc_in * vdc_long / 1000 + 0.5;
    c5->line_width.r = d5->line_width.r;
    c5->line_colour.red = d5->line_colour.red;
    c5->line_colour.green = d5->line_colour.green;
    c5->line_colour.blue = d5->line_colour.blue;
    c5->line_colour.ind = d5->line_colour.ind;
    c5->mk_b_index = d5->mk_b_index;
    c5->mk_type = d5->mk_type;
    c5->mk_size.i = pxlvdc_in * vdc_long / 100 + 0.5;
    c5->mk_size.r = d5->mk_size.r;
    c5->mk_colour.red = d5->mk_colour.red;
    c5->mk_colour.green = d5->mk_colour.green;
    c5->mk_colour.blue = d5->mk_colour.blue;
    c5->mk_colour.ind = d5->mk_colour.ind;
    c5->t_b_index = d5->t_b_index;
    c5->t_f_index = d5->t_f_index;
    c5->t_prec = d5->t_prec;
    c5->c_exp_fac = d5->c_exp_fac;
    c5->c_space = d5->c_space;
    c5->text_colour.red = d5->text_colour.red;
    c5->text_colour.green = d5->text_colour.green;
    c5->text_colour.blue = d5->text_colour.blue;
    c5->text_colour.ind = d5->text_colour.ind;
    c5->c_height = pxlvdc_in * vdc_long / 100 + 0.5;
    c5->c_orient.x_up = d5->c_orient.x_up;
    c5->c_orient.y_up = d5->c_orient.y_up;
    c5->c_orient.x_base = d5->c_orient.x_base;
    c5->c_orient.y_base = d5->c_orient.y_base;
    c5->text_path = d5->text_path;
    c5->text_align.hor = d5->text_align.hor;
    c5->text_align.ver = d5->text_align.ver;
    c5->text_align.cont_hor = d5->text_align.cont_hor;
    c5->text_align.cont_ver = d5->text_align.cont_ver;
    c5->c_set_index = d5->c_set_index;
    c5->a_c_set_index = d5->a_c_set_index;
    c5->f_b_index = d5->f_b_index;
    c5->int_style = d5->int_style;
    c5->fill_colour.red = d5->fill_colour.red;
    c5->fill_colour.green = d5->fill_colour.green;
    c5->fill_colour.blue = d5->fill_colour.blue;
    c5->fill_colour.ind = d5->fill_colour.ind;
    c5->hatch_index = d5->hatch_index;
    c5->pat_index = d5->pat_index;
    c5->e_b_index = d5->e_b_index;
    c5->edge_type = d5->edge_type;
    c5->edge_width.i = pxlvdc_in * vdc_long / 1000 + 0.5;
    c5->edge_width.r = d5->edge_width.r;
    c5->edge_colour.red = d5->edge_colour.red;
    c5->edge_colour.green = d5->edge_colour.green;
    c5->edge_colour.blue = d5->edge_colour.blue;
    c5->edge_colour.ind = d5->edge_colour.ind;
    c5->edge_vis = d5->edge_vis;
    for (i = 0; i < 2; ++i)
    {
	c5->fill_ref.i[i] = d5->fill_ref.i[i];
	c5->fill_ref.r[i] = d5->fill_ref.r[i];
    }
    for (i = 0; i < 4; ++i)
    {
	c5->pat_size.i[i] = d5->pat_size.i[i];
	c5->pat_size.r[i] = d5->pat_size.r[i];
    }
    for (i = 0; i < 3 * (c1->max_c_index + 1); ++i)
	*(c5->ctab + i) = *(d5->ctab + i);
    /* tables later */
}



/* now the routines that do the work, class by class */
/* class 0 routines */
/* function to start a metafile */
static
f_b_mf(dat_ptr, p_len, dev_func, ctrl_func)
    int             (*ctrl_func) ();
    int             (*dev_func) ();
    char           *dat_ptr;
    int             p_len;
{
    int             ret = 1;
    char            buffer[2 * max_str + 1], prog_name[max_str];

    static char	    env[80];

    /* first put in the defaults */
    cosr = 1.0;
    sinr = 0.0;
    xp0 = 0;
    yp0 = 0;
    pxl_vdc = 1.0;
    pages_done = 0;
    /* set the defaults in case anything happens before the first pic */
    rs_defaults(glbl1, glbl2, glbl3, glbl5, dflt2, dflt3, dflt5, pxl_vdc);

    strcpy(prog_name, "GLI ");
    buffer[0] = '\0';
    if (p_len > 0)
	cc_str(&dat_ptr, buffer);
    in_page_no = 0;

    if (isatty(1))
        tt_printf("<%s>\n", buffer);

    if (strncmp(buffer,"GKSGRAL", 7) == 0)
	strcpy(env, "GLI_GKS=GKSGRAL");
    else if (strncmp(buffer, "GLI GKS", 7) == 0)
	strcpy(env, "GLI_GKS=GLIGKS");
    else
	strcpy(env, "GLI_GKS=GKGKS");
    putenv(env);

    if (dev_func)
    {
	strcat(prog_name, "2.0");
	ret = (*dev_func) (buffer, g_in_name, prog_name);
    }
    if (ctrl_func)
	(*ctrl_func) ();
    return (ret);
}
/* function to end a metafile */
static
f_e_mf(dev_func, ctrl_func)
    int             (*ctrl_func) ();
    int             (*dev_func) ();
{
    int             ret = 1;

    if (dev_func)
	ret = (*dev_func) (pages_done);
    if (ctrl_func)
	(*ctrl_func) ();
    ret = 0;
    return (ret);			       /* time to stop */
}
/* function to reset all of the defaults before starting picture body */
static
f_b_p(dat_ptr, p_len, dev_func, ctrl_func)
    int             (*ctrl_func) ();
    int             (*dev_func) ();
    char           *dat_ptr;
    int             p_len;
{
    int             ret = 1;

    if (p_len > 1)
    {
	cc_str(&dat_ptr, pic_name);
	pic_name[max_str - 1] = '\0';
    } else
	pic_name[0] = '\0';

    if (dev_func)
	ret = (*dev_func) (pic_name);
    if (ctrl_func)
	(*ctrl_func) ();

    get_smart(0, glbl1, a2, opt, dev_info, &cosr, &sinr, &pxl_vdc,
	  &xoffset, &yoffset, &xp0, &yp0, &xsize, &ysize, &font_check,
	      &font_text, &sx, &sy);

    /* go back to the defaults */
    rs_defaults(glbl1, glbl2, glbl3, glbl5, dflt2, dflt3, dflt5, pxl_vdc);

    return (ret);
}
/*
 * function to get the printer ready to start drawing; implement
 * defaults
 */
static
f_b_p_body(dev_func, ctrl_func)
    int             (*ctrl_func) ();
    int             (*dev_func) ();
{
    int             ret = 1;
    

    get_smart(1, glbl1, a2, opt, dev_info, &cosr, &sinr, &pxl_vdc,
	  &xoffset, &yoffset, &xp0, &yp0, &xsize, &ysize, &font_check,
	      &font_text, &sx, &sy);


    /*
     * we have to override the index 0 colour by the background colour
     * (sounds screwy, but I think correct)
     */

    *(a5->ctab) = a2->back_col.red;
    *(a5->ctab + 1) = a2->back_col.green;
    *(a5->ctab + 2) = a2->back_col.blue;

    if (dev_func)
	ret = (*dev_func) (pic_name, xoffset, yoffset,
			   0.0,
			   a2->back_col.red, a2->back_col.green,
			   a2->back_col.blue,
			   in_page_no, xsize, ysize);

    if (ctrl_func)
	(*ctrl_func) ();

    return (ret);
}
/* function to end a picture */
static
f_e_pic(dev_func, ctrl_func)
    int             (*ctrl_func) ();
    int             (*dev_func) ();
{
    int             ret = 1;
    ++pages_done;
    if (dev_func)
	ret = (*dev_func) (1);
    if (ctrl_func)
	(*ctrl_func) (1);
    return (ret);
}
/* now the class1 functions */
/* read the metafile version number (if present) */
static
rd_mf_version(dat_ptr, p_len, dev_func)
    char           *dat_ptr;
    int             p_len;
    int             (*dev_func) ();
{
    int             ret = 1, vers_no;
    if (p_len > 0)
	vers_no = cc_int(&dat_ptr);

    if ((p_len > 0) && dev_func)
	ret = (*dev_func) (vers_no);

    return (ret);
}

/* read the metafile descriptor */
static
rd_mf_descriptor(dat_ptr, p_len, dev_func)
    char           *dat_ptr;
    int             p_len;
    int             (*dev_func) ();
{
    int             ret = 1;
    char           *char_ptr;
    if (p_len > 1)
	char_ptr = cc_str(&dat_ptr, NULL);
    else
	char_ptr = NULL;
    /* send off to logging line */

    if ((p_len > 1) && dev_func)
	ret = (*dev_func) (char_ptr);
    return (ret);
}

/* get the next token, should be a string */
static int
get_token(in_ptr, out_str)
    char          **in_ptr, *out_str;
{
    int             i;
    while (**in_ptr == ' ')
	++* in_ptr;		       /* skip spaces */
    for (i = 0; (((**in_ptr >= 'A') && (**in_ptr <= 'Z')) ||
      ((**in_ptr >= '0') && (**in_ptr <= '9'))) && (i < max_str); ++i)
	out_str[i] = *((*in_ptr)++);
    out_str[i] = '\0';
    return (i);
}
/*
 * function to compare two strings, will assume that 1) both in same
 * case 2) at least one terminated correctly
 */
static int
cc_same(str1, str2)
    char           *str1, *str2;
{
    while ((*str1 == *str2) && (*str1) && (*str2))
    {
	++str1;
	++str2;
    }
    return (*str1 == *str2);
}

/* set the VDC type */
static
s_vdc_type(dat_ptr, dev_func)
    char           *dat_ptr;
    int             (*dev_func) ();
{
    enum vdc_enum   new_type;
    int             ret = 1;
    char            buffer[max_str];

    if (!get_token(&dat_ptr, buffer))
	return (2);
    if (cc_same(buffer, "INTEGER"))
	new_type = vdc_int;
    else if (cc_same(buffer, "REAL"))
	new_type = vdc_real;
    else
    {
	s_error ("unrecognised token in s_vdc_type %s", buffer);
	return (2);
    }
    glbl1->vdc_type = (enum vdc_enum) new_type;
    if (dev_func)
	ret = (*dev_func) (new_type);
    return (ret);
}

/* figure out the CGM binary precision from clear text version */
static int
get_prec(new_min, new_max, for_carray)
    int             new_min, new_max, for_carray;
{
    int             i, start_prec;
    start_prec = (for_carray) ? 1 : 8;
    /* possible values are 8, 16, 24 and 32, and 1, 2, 4 for carrays */
    if (new_min > new_max)
    {
	return (8);
    }
    if (new_max < 0)
	new_max = -new_max;
    if (-new_min > new_max)
	new_max = -new_min;

    for (i = start_prec; i < 8; i *= 2)
	if (!((~0 << i) & new_max))
	    return (i);

    for (i = 8; i <= 32; i += 8)
	if (!((~0 << i) & new_max))
	    return (i);

    return (32);		       /* best we can do (may not be an
					* error) */
}

/* set the integer precision */
static
s_int_prec(dat_ptr, dev_func)
    char           *dat_ptr;
    int             (*dev_func) ();
{
    int             new_max, new_min, new_prec, ret = 1;

    new_min = cc_int(&dat_ptr);
    new_max = cc_int(&dat_ptr);
    new_prec = get_prec(new_min, new_max, 0);
    glbl1->ind_prec = new_prec;

    if (dev_func)
	ret = (*dev_func) (new_prec);
    return (ret);
}
static int get_rprec(minreal, maxreal, no_digits, new_fixed, new_exp, new_fract)
    float           minreal, maxreal;
    int             no_digits, *new_fixed, *new_exp, *new_fract;
{
    /* for now */
    *new_fixed = 0;
    *new_exp = 9;
    *new_fract = 23;

    return (1);
}
/* set the real  precision */
static
s_real_prec(dat_ptr, dev_func)
    char           *dat_ptr;
    int             (*dev_func) ();
{
    int             new_fixed, new_exp, new_fract, ret = 1;
    float           minreal, maxreal;
    int             no_digits;

    minreal = cc_real(&dat_ptr);
    maxreal = cc_real(&dat_ptr);
    no_digits = cc_int(&dat_ptr);

    get_rprec(minreal, maxreal, no_digits, &new_fixed, &new_exp, &new_fract);

    glbl1->real_prec.fixed = new_fixed;
    glbl1->real_prec.exp = new_exp;
    glbl1->real_prec.fract = new_fract;

    if (dev_func)
	ret = (*dev_func) (new_fixed, new_exp, new_fract);
    return (ret);
}
/* set the index precision */
static
s_index_prec(dat_ptr, dev_func)
    char           *dat_ptr;
    int             (*dev_func) ();
{
    int             new_max, new_min, new_prec, ret = 1;

    new_min = cc_int(&dat_ptr);
    new_max = cc_int(&dat_ptr);
    new_prec = get_prec(new_min, new_max, 0);
    glbl1->ind_prec = new_prec;
    if (dev_func)
	ret = (*dev_func) (new_prec);
    return (ret);
}
/* set the colour precision */
static
s_col_prec(dat_ptr, dev_func)
    char           *dat_ptr;
    int             (*dev_func) ();
{
    int             new_max, new_prec, ret = 1;

    new_max = cc_int(&dat_ptr);
    new_prec = get_prec(0, new_max, 0);
    glbl1->col_prec = new_prec;
    if (dev_func)
	ret = (*dev_func) (new_prec);
    return (ret);
}
/* set the colour index precision */
static
s_cind_prec(dat_ptr, dev_func)
    char           *dat_ptr;
    int             (*dev_func) ();
{
    int             new_max, new_prec, ret = 1;

    new_max = cc_int(&dat_ptr);
    new_prec = get_prec(0, new_max, 0);
    glbl1->col_i_prec = new_prec;
    if (dev_func)
	ret = (*dev_func) (new_prec);
    return (ret);
}
/* set the colour value extent */
static
s_cvextent(dat_ptr, dev_func)
    char           *dat_ptr;
    int             (*dev_func) ();
{
    int             cvmin[3], cvmax[3], ret = 1, i;

    for (i = 0; i < 3; ++i)
    {
	cvmin[i] = cc_int(&dat_ptr);
    }
    for (i = 0; i < 3; ++i)
    {
	cvmax[i] = cc_int(&dat_ptr);
    }

    for (i = 0; i < 3; ++i)
    {
	glbl1->c_v_extent.min[i] = cvmin[i];
	glbl1->c_v_extent.max[i] = cvmax[i];
    }

    if (dev_func)
	ret = (*dev_func) (cvmin, cvmax);

    return (ret);
}


static
do_mcind(new_index)
    int             new_index;
{
    int             i;
    float          *new_d_ptr, *new_g_ptr;

    if (new_index > glbl1->max_c_index)
    {				       /* need to make some new memory */
	new_d_ptr =
	    (float *) allocate_mem(float_size * 3 * (new_index + 1), 0);
	new_g_ptr =
	    (float *) allocate_mem(float_size * 3 * (new_index + 1), 0);
	if ((!new_d_ptr) || (!new_g_ptr))
	    return (0);
	/* move over the old data */
	for (i = 0; i < (glbl1->max_c_index + 1) * 3; ++i)
	{
	    *(new_d_ptr + i) = *(a5->ctab + i);
	    *(new_g_ptr + i) = *(a5->ctab + i);
	}
	/* free up memory */
	free(dflt5->ctab);
	free(glbl5->ctab);
	/* and reassign the pointers */
	dflt5->ctab = new_d_ptr;
	glbl5->ctab = new_g_ptr;
    }
    glbl1->max_c_index = new_index;
    return (1);
}

/* set the maximum colour index */
/*
 * split into two functions as we may have to handle illegal metafiles
 * (DI3000) that add c table entries before increasing
 * glbl1->max_c_index
 */
static
s_mcind(dat_ptr, dev_func)
    char           *dat_ptr;
    int             (*dev_func) ();
{
    int             new_index, ret = 1;
    new_index = cc_int(&dat_ptr);
    if (!do_mcind(new_index))
	s_error ("trouble setting max colind");
    if (dev_func)
	ret = (*dev_func) (new_index);
    return (ret);
}



/* clean up a string into standard format */
static int clean_string(out_ptr)
    char           *out_ptr;
{
    char           *in_ptr, c;

    /* just march thru doing conversions */

    in_ptr = out_ptr;

    while ((c = *(in_ptr++)) != 0)
    {
	if (((c >= 'A') && (c <= 'Z')) || ((c >= '0') && (c <= '9')))
	    *(out_ptr++) = c;
	else if ((c >= 'a') && (c <= 'z'))
	    *(out_ptr++) = c + ('A' - 'a');
	else if ((c == ',') || is_sep(c))
	    *(out_ptr++) = ' ';
    }
    *out_ptr = '\0';

    return (1);
}

/* read the metafile element list (may be useful) */
static
rd_mf_list(dat_ptr, dev_func)
    char           *dat_ptr;
    int             (*dev_func) ();
{
#define max_buffer 1024
    int             no_pairs, i, j, k, done, class, el, ret = 1;
    int             out_array[max_buffer];
    char           *cptr;
    char            buffer[max_buffer], cmd_name[max_str];

    /* lets see if we have a string */
    while (*dat_ptr == ' ')
	++dat_ptr;
    if (is_term(*dat_ptr))
    {
	s_error ("empty mf element list");
	return (2);
    } else if (is_quote(*dat_ptr))
    {
	++dat_ptr;
	i = 0;
	while ((!is_quote(*dat_ptr)) && (i < (max_buffer - 1)))
	    buffer[i++] = *(dat_ptr++);
	buffer[i] = '\0';
    } else
    {
	return (2);
    }

    /* now clean up the string */
    clean_string(buffer);

    /* and pick out the tokens */
    cptr = buffer;
    no_pairs = 0;
    while ((*cptr) && get_token(&cptr, cmd_name))
    {
	if (cc_same(cmd_name, "DRAWINGSET"))
	{
	    class = -1;
	    el = 0;
	} else if (cc_same(cmd_name, "DRAWINGPLUS"))
	{
	    class = -1;
	    el = 1;
	} else
	    /* may be replacing defaults */
	if (cc_same(cmd_name, "BEGMFDEFAULTS"))
	{
	    class = 2;
	    el = 13;
	} else if (cc_same(cmd_name, "ENDMFDEFAULTS"))
	{
	    class = 2;
	    el = 13;
	} else
	{
	    done = 0;
	    for (i = 0; (i < 8) && (!done); ++i)
	    {
		for (j = 1; (j < cc_size[i]) && (!done); ++j)
		{
		    k = 0;
		    while ((CC_cptr[i][j][k]) && (cmd_name[k]) &&
			   (CC_cptr[i][j][k] == cmd_name[k]))
			++k;
		    done = ((!CC_cptr[i][j][k]) && (!cmd_name[k]) && (k));
		}
	    }
	    if (!done)
	    {
		s_error ("couldn't find a match for %s in defrep",
			cmd_name);
		return (2);
	    }
	    class = i - 1;
	    el = j - 1;
	}
	out_array[2 * no_pairs] = class;
	out_array[2 * no_pairs + 1] = el;
	++no_pairs;
	while (*cptr == ' ')
	    ++cptr;
    }

    if (dev_func)
	ret = (*dev_func) (no_pairs, out_array);
    return (ret);
}
#undef max_buffer
/* replace the metafile defaults */
static
s_mf_defs(dat_ptr, p_len, dev_func, df_call)
    char           *dat_ptr;
    int             p_len;
    int             (*dev_func) ();
    int             df_call;
{
    int             ret = 1;

    if (df_call == 0)
    {
	a2 = dflt2;
	a3 = dflt3;
	a5 = dflt5;
	if (dev_func)
	    ret = (*dev_func) (0);     /* start */
	/* default values are active */
	return (ret);
    } else if (df_call == 1)
    {
	a2 = glbl2;
	a3 = glbl3;
	a5 = glbl5;
	/* globals are now active again */
	if (dev_func)
	    ret = (*dev_func) (1);     /* end */
	return (ret);
    } else
    {
	s_error ("illegal argument to s_mf_defs");
	return (2);
    }
}
/* read the font list (implement fully later) */
static
do_font_list(dat_ptr, p_len, dev_func)
    char           *dat_ptr;
    int             p_len;
    int             (*dev_func) ();
{
    int             ret = 1;
    int             no_strings;
    char            font_list[max_fonts][max_str + 1], *new_ptr;

    new_ptr = dat_ptr;
    no_strings = 0;
    while ((no_strings < max_fonts) && (new_ptr < dat_ptr + p_len - 3))
    {
	cc_str(&new_ptr, font_list[no_strings]);
	font_list[no_strings][max_str] = '\0';	/* for safety */
	++no_strings;
    }
    if (no_strings > max_fonts)
	no_strings = max_fonts;

    if (dev_func)
	ret = (*dev_func) (no_strings, font_list);
    return (ret);
}
/* read the character list (implement fully later) */
static
do_char_list(dat_ptr, p_len, dev_func)
    char           *dat_ptr;
    int             p_len;
    int             (*dev_func) ();
{
    int             ret = 1;
    int             no_strings, type_array[max_fonts];
    char            font_list[max_fonts][max_str + 1];
    char           *new_ptr;
    char            buffer[max_str];

    new_ptr = dat_ptr;
    no_strings = 0;
    while (*new_ptr == ' ')
	++new_ptr;		       /* skip spaces */

    while ((no_strings < max_fonts) && (!is_term(*new_ptr)))
    {
	if (!get_token(&new_ptr, buffer))
	    return (2);

	if (cc_same(buffer, "STD94"))
	    type_array[no_strings] = 0;
	else if (cc_same(buffer, "STD96"))
	    type_array[no_strings] = 1;
	else if (cc_same(buffer, "STD94MULTIBYTE"))
	    type_array[no_strings] = 2;
	else if (cc_same(buffer, "STD96MULTIBYTE"))
	    type_array[no_strings] = 3;
	else if (cc_same(buffer, "COMPLETECODE"))
	    type_array[no_strings] = 4;
	else
	{
	    s_error ("unknown character set %s", buffer);
	    return (2);
	}

	cc_str(&new_ptr, font_list[no_strings]);
	font_list[no_strings][max_str] = '\0';	/* for safety */
	++no_strings;
    }

    if (no_strings > max_fonts)
	no_strings = max_fonts;

    if (dev_func)
	ret = (*dev_func) (no_strings, type_array, font_list);
    return (ret);
}
/* do the character announcer */
static
do_cannounce(dat_ptr, dev_func)
    char           *dat_ptr;
    int             (*dev_func) ();
{
    int             ret = 1;
    int             new_announce;
    char            buffer[max_str];

    if (!get_token(&dat_ptr, buffer))
	return (2);
    if (cc_same(buffer, "BASIC7BIT"))
	new_announce = 0;
    else if (cc_same(buffer, "BASIC8BIT"))
	new_announce = 1;
    else if (cc_same(buffer, "EXTD7BIT"))
	new_announce = 2;
    else if (cc_same(buffer, "EXTD8BIT"))
	new_announce = 3;
    else
    {
	s_error ("unknown character announcer %s", buffer);
	return (2);
    }
    glbl1->char_c_an = new_announce;
    if (dev_func)
	ret = (*dev_func) (new_announce);
    return (ret);
}
/* now the class2 functions */
/* set the scaling mode */
static
s_scalmode(dat_ptr, dev_func)
    char           *dat_ptr;
    int             (*dev_func) ();
{
    char            buffer[max_str];
    int             ret = 1, new_mode;
    float           my_scale = 1;

    if (!get_token(&dat_ptr, buffer))
	return (2);
    if (cc_same(buffer, "ABSTRACT") || cc_same(buffer, "ABS"))
	new_mode = 0;
    else if (cc_same(buffer, "METRIC"))
    {
	new_mode = 1;
	my_scale = cc_real(&dat_ptr);
    } else
    {
	s_error ("unrecognised token in s_scalmode %s", buffer);
	return (2);
    }

    a2->scale_mode.s_mode = new_mode;
    a2->scale_mode.m_scaling = my_scale;

    if (dev_func)
	ret = (*dev_func) (new_mode, my_scale);

    return (ret);
}

/* set the colour selection mode */
static
s_c_s_mode(dat_ptr, dev_func)
    char           *dat_ptr;
    int             (*dev_func) ();
{
    int             ret = 1;
    enum cs_enum    new_c_s_mode;
    char            buffer[max_str];

    if (!get_token(&dat_ptr, buffer))
	return (2);
    if (cc_same(buffer, "INDEXED"))
	new_c_s_mode = i_c_mode;
    else if (cc_same(buffer, "DIRECT"))
	new_c_s_mode = d_c_mode;
    else
    {
	s_error ("unrecognised token in s_c_s_mode %s", buffer);
	return (2);
    }

    a2->c_s_mode = new_c_s_mode;
    if (dev_func)
	ret = (*dev_func) (new_c_s_mode);
    return (ret);
}
/* set the line width specification mode */
static
s_lws_mode(dat_ptr, dev_func)
    char           *dat_ptr;
    int             (*dev_func) ();
{
    int             ret = 1;
    enum spec_enum  new_lws_mode;
    char            buffer[max_str];

    if (!get_token(&dat_ptr, buffer))
	return (2);
    if (cc_same(buffer, "ABSTRACT") || cc_same(buffer, "ABS"))
	new_lws_mode = absolute;
    else if (cc_same(buffer, "SCALED"))
	new_lws_mode = scaled;
    else
    {
	s_error ("unrecognised token in s_lws_mode %s", buffer);
	return (2);
    }
    a2->l_w_s_mode = new_lws_mode;
    if (dev_func)
	ret = (*dev_func) (new_lws_mode);
    return (ret);
}
/* set the marker size specification mode */
static
s_ms_mode(dat_ptr, dev_func)
    char           *dat_ptr;
    int             (*dev_func) ();
{
    int             ret = 1;
    enum spec_enum  new_mode;
    char            buffer[max_str];

    if (!get_token(&dat_ptr, buffer))
	return (2);
    if (cc_same(buffer, "ABSTRACT") || cc_same(buffer, "ABS"))
	new_mode = absolute;
    else if (cc_same(buffer, "SCALED"))
	new_mode = scaled;
    else
    {
	s_error ("unrecognised token in s_ms_mode %s", buffer);
	return (2);
    }

    a2->m_s_s_mode = new_mode;
    if (dev_func)
	ret = (*dev_func) (new_mode);
    return (ret);
}
/* set the edge width specification mode */
static
s_ew_mode(dat_ptr, dev_func)
    char           *dat_ptr;
    int             (*dev_func) ();
{
    int             ret = 1;
    enum spec_enum  new_mode;
    char            buffer[max_str];

    if (!get_token(&dat_ptr, buffer))
	return (2);
    if (cc_same(buffer, "ABSTRACT") || cc_same(buffer, "ABS"))
	new_mode = absolute;
    else if (cc_same(buffer, "SCALED"))
	new_mode = scaled;
    else
    {
	s_error ("unrecognised token in s_ew_mode %s", buffer);
	return (2);
    }

    a2->e_w_s_mode = new_mode;
    if (dev_func)
	ret = (*dev_func) (new_mode);
    return (ret);
}
/* set the VDC extent */
static
s_vdc_extent(dat_ptr, dev_func)
    char           *dat_ptr;
    int             (*dev_func) ();
{
    int             new_coords[4], i, ret = 1;
    float           new_real[4];

    switch (glbl1->vdc_type)
    {
    case vdc_int:
	{
	    get_ipoint(&dat_ptr, new_coords, new_coords + 1);
	    get_ipoint(&dat_ptr, new_coords + 2, new_coords + 3);
	    for (i = 0; i < 4; ++i)
		a2->vdc_extent.i[i] = new_coords[i];
	    break;
	}
    case vdc_real:
	{
	    get_rpoint(&dat_ptr, new_real, new_real + 1);
	    get_rpoint(&dat_ptr, new_real + 2, new_real + 3);
	    for (i = 0; i < 4; ++i)
		a2->vdc_extent.r[i] = new_real[i];
	    break;
	}
    default:
	s_error ("illegal vdc_type ");
    }

    if (dev_func)
	ret = (*dev_func) (new_coords, new_real);
    return (ret);
}
/* set the background colour */
static
s_back_col(dat_ptr, dev_func)
    char           *dat_ptr;
    int             (*dev_func) ();
{
    int             ret = 1;
    unsigned int    ir, ig, ib;
    float           r, g, b;

    ir = cc_int(&dat_ptr);
    r = dcr(ir);
    ig = cc_int(&dat_ptr);
    g = dcg(ig);
    ib = cc_int(&dat_ptr);
    b = dcb(ib);
    a2->back_col.red = r;
    a2->back_col.green = g;
    a2->back_col.blue = b;
    if (dev_func)
	ret = (*dev_func) (ir, ig, ib);
    return (ret);
}
/* now the class3 functions */
/* set the vdc integer precision */
static
s_vdc_i_p(dat_ptr, dev_func)
    char           *dat_ptr;
    int             (*dev_func) ();
{
    int             new_max, new_min, new_prec, ret = 1;

    new_min = cc_int(&dat_ptr);
    new_max = cc_int(&dat_ptr);
    new_prec = get_prec(new_min, new_max, 0);
    a3->vdc_i_prec = new_prec;
    if (dev_func)
	ret = (*dev_func) (new_prec);
    return (ret);
}
/* set the vdc real precision */
static
s_vdc_r_p(dat_ptr, dev_func)
    char           *dat_ptr;
    int             (*dev_func) ();
{
    int             new_fixed, new_exp, new_fract, ret = 1;
    float           minreal, maxreal;
    int             no_digits;

    minreal = cc_real(&dat_ptr);
    maxreal = cc_real(&dat_ptr);
    no_digits = cc_int(&dat_ptr);

    get_rprec(minreal, maxreal, no_digits, &new_fixed, &new_exp, &new_fract);

    a3->vdc_r_prec.fixed = new_fixed;
    a3->vdc_r_prec.exp = new_exp;
    a3->vdc_r_prec.fract = new_fract;

    if (dev_func)
	ret = (*dev_func) (new_fixed, new_exp, new_fract);
    return (ret);
}
/* set the auxiliary colour */
static
s_aux_col(dat_ptr, dev_func)
    char           *dat_ptr;
    int             (*dev_func) ();
{
    int             ret = 1;
    int             new_index = 0, ir, ig, ib;
    float           r, g, b, *rptr;

    switch ((int) a2->c_s_mode)
    {
    case (int) i_c_mode:	       /* indexed mode */
	new_index = cc_int(&dat_ptr);
	a3->aux_col.ind = new_index;
	rptr = a5->ctab + a3->aux_col.ind * 3;
	r = *rptr;
	g = *++rptr;
	b = *++rptr;
	break;
    case (int) d_c_mode:	       /* direct mode */
	ir = cc_int(&dat_ptr);
	r = dcr(ir);
	ig = cc_int(&dat_ptr);
	g = dcg(ig);
	ib = cc_int(&dat_ptr);
	b = dcb(ib);
	a3->aux_col.red = r;
	a3->aux_col.green = g;
	a3->aux_col.blue = b;
	break;
    default:
	s_error ("illegal colour mode ");
    }
    if (dev_func)
	ret = (*dev_func) (r, g, b, new_index);
    return (ret);
}
/* set the transparency */
static
s_transp(dat_ptr, dev_func)
    char           *dat_ptr;
    int             (*dev_func) ();
{
    int             ret = 1;
    enum bool_enum  new_trans;
    char            buffer[max_str];

    if (!get_token(&dat_ptr, buffer))
	return (2);
    if (cc_same(buffer, "ON"))
	new_trans = on;
    else if (cc_same(buffer, "OFF"))
	new_trans = off;
    else
    {
	s_error ("unrecognised token in s_transp %s", buffer);
	return (2);
    }
    a3->transparency = new_trans;
    if (dev_func)
	ret = (*dev_func) (new_trans);
    return (ret);
}


/* set the clipping rectangle */
static
s_clip_rec(dat_ptr, dev_func)
    char           *dat_ptr;
    int             (*dev_func) ();
{
    int             new_coords[4], i;
    float           new_real[4], ret = 1;


    switch (glbl1->vdc_type)
    {
    case vdc_int:
	{
	    get_ipoint(&dat_ptr, new_coords, new_coords + 1);
	    get_ipoint(&dat_ptr, new_coords + 2, new_coords + 3);
	    for (i = 0; i < 4; ++i)
		a3->clip_rect.i[i] = new_coords[i];

	    break;
	}
    case vdc_real:
	{
	    get_rpoint(&dat_ptr, new_real, new_real + 1);
	    get_rpoint(&dat_ptr, new_real + 2, new_real + 3);
	    for (i = 0; i < 4; ++i)
		a3->clip_rect.r[i] = new_real[i];
	    break;
	}
    default:
	s_error ("illegal vdc_type ");
    }

    if (dev_func)
	ret = (*dev_func) (new_coords, new_real);

    return (ret);
}


/* set the clipping indicator */
static
s_clip_ind(dat_ptr, dev_func)
    char           *dat_ptr;
    int             (*dev_func) ();
{
    int             ret = 1;
    enum bool_enum  new_clip;
    char            buffer[max_str];

    if (!get_token(&dat_ptr, buffer))
	return (2);
    if (cc_same(buffer, "ON"))
	new_clip = on;
    else if (cc_same(buffer, "OFF"))
	new_clip = off;
    else
    {
	s_error ("unrecognised token in s_clip_ind %s", buffer);
	return (2);
    }
    a3->clip_ind = new_clip;
    if (dev_func)
	ret = (*dev_func) (new_clip);
    return (ret);
}
/* function to return the number of points in a string */
static int get_pairs(in_ptr)
    char           *in_ptr;
{
    int             no_pts;
    enum
    {
	between, in_number
    }               my_state;

    no_pts = 0;
    my_state = between;
    while ((*in_ptr) && (!is_term(*in_ptr)))
    {
	if (is_sep(*in_ptr) || (*in_ptr == ',') || (*in_ptr == '(')
	    | (*in_ptr == ')'))
	{
	    if (my_state == in_number)
	    {
		my_state = between;
		++no_pts;
	    }
	} else if (my_state == between)
	{
	    my_state = in_number;
	    ++no_pts;
	}
	++in_ptr;
    }
    return ((no_pts + 1) / 2);
}


/* now the class 4 functions */
/* take care of a series of points that need a line between them */
static
do_polyline(dat_ptr, p_len, dev_func)
    int             (*dev_func) ();
    char           *dat_ptr;
    int             p_len;
{
    int             ret = 1;
    int             no_pairs, x, y, *x_ptr, *y_ptr, *x1_ptr, *y1_ptr;

    no_pairs = get_pairs(dat_ptr);

    /* first get the memory */
    if (no_pairs > wk_ar_size)
    {
	x1_ptr = intalloc(no_pairs * bytes_per_word);
	y1_ptr = intalloc(no_pairs * bytes_per_word);
    } else
    {
	x1_ptr = x_wk_ar;
	y1_ptr = y_wk_ar;
    }
    if ((!x1_ptr) || (!y1_ptr))
	s_error ("trouble with polyline memory");
    x_ptr = x1_ptr;
    y_ptr = y1_ptr;

    /* get the data */
    no_pairs = 0;
    while (!is_term(*dat_ptr))
    {
	get_vpoint(&dat_ptr, &x, &y);
	*x_ptr++ = newx(x, y);
	*y_ptr++ = newy(x, y);
	++no_pairs;
    }

    if (dev_func)
	ret = (*dev_func) (no_pairs, x1_ptr, y1_ptr);

    if (x1_ptr != x_wk_ar)
	free(x1_ptr);
    if (y1_ptr != y_wk_ar)
	free(y1_ptr);

    return (ret);
}
/*
 * take care of a series of points that need a line between alternate
 * points
 */
static
do_dis_polyline(dat_ptr, p_len, dev_func)
    int             (*dev_func) ();
    char           *dat_ptr;
    int             p_len;
{
    int             ret = 1;
    int             no_pairs, x, y, *x_ptr, *y_ptr, *x1_ptr, *y1_ptr;

    no_pairs = get_pairs(dat_ptr);
    /* some arrays have odd number of points ! */
    if (no_pairs % 2)
	--no_pairs;

    /* first get the memory */
    if (no_pairs > wk_ar_size)
    {
	x1_ptr = intalloc(no_pairs * bytes_per_word);
	y1_ptr = intalloc(no_pairs * bytes_per_word);
    } else
    {
	x1_ptr = x_wk_ar;
	y1_ptr = y_wk_ar;
    }
    if ((!x1_ptr) || (!y1_ptr))
	s_error ("trouble with disjoint polyline memory");
    x_ptr = x1_ptr;
    y_ptr = y1_ptr;

    /* now grab the data */
    no_pairs = 0;
    while (!is_term(*dat_ptr))
    {
	get_vpoint(&dat_ptr, &x, &y);
	*x_ptr++ = newx(x, y);
	*y_ptr++ = newy(x, y);
	++no_pairs;
    }

    ret = (*dev_func) (no_pairs, x1_ptr, y1_ptr);

    if (x1_ptr != x_wk_ar)
	free(x1_ptr);
    if (y1_ptr != y_wk_ar)
	free(y1_ptr);

    return (ret);
}
/* do a series of markers at the specified points */
static
do_polymarker(dat_ptr, p_len, dev_func)
    char           *dat_ptr;
    int             p_len;
    int             (*dev_func) ();
{
    int             ret = 1;
    int             no_pairs, x, y, *x_ptr, *y_ptr, *x1_ptr, *y1_ptr;

    no_pairs = get_pairs(dat_ptr);

    /* first get the memory */
    if (no_pairs > wk_ar_size)
    {
	x1_ptr = intalloc(no_pairs * bytes_per_word);
	y1_ptr = intalloc(no_pairs * bytes_per_word);
    } else
    {
	x1_ptr = x_wk_ar;
	y1_ptr = y_wk_ar;
    }
    if ((!x1_ptr) || (!y1_ptr))
	s_error ("trouble with polymarker memory");
    x_ptr = x1_ptr;
    y_ptr = y1_ptr;

    /* now grab the data */
    no_pairs = 0;
    while (!is_term(*dat_ptr))
    {
	get_vpoint(&dat_ptr, &x, &y);
	*x_ptr++ = newx(x, y);
	*y_ptr++ = newy(x, y);
	++no_pairs;
    }
    ret = (*dev_func) (no_pairs, x1_ptr, y1_ptr);

    if (x1_ptr != x_wk_ar)
	free(x1_ptr);
    if (y1_ptr != y_wk_ar)
	free(y1_ptr);

    return (ret);
}
/* set actual text, also take care of raster character descriptions */
static
s_text(dat_ptr, dev_func)
    char           *dat_ptr;
    int             (*dev_func) ();
{
    int             ret = 1;
    int             x, y, xin, yin, no_chars, device_ok;
    enum bool_enum  final;
    char            txt_buffer[2 * max_str + 1];
    char            buffer[max_str];
 





    /* first see if the device can handle it */
    device_ok =
	((a5->t_prec == string) && (dev_info->capability & string_text)) ||
	((a5->t_prec == character) && (dev_info->capability & char_text)) ||
	((a5->t_prec == stroke) && (dev_info->capability & stroke_text)) ||
	(font_check == NULL) || (font_text == NULL);

    /* may be overrriding */
    if (font_check && font_text)
	device_ok = 0;

    /* now ready to procede */

    get_vpoint(&dat_ptr, &xin, &yin);
    x = newx(xin, yin);
    y = newy(xin, yin);

    if (!get_token(&dat_ptr, buffer))
	return (2);
    if (cc_same(buffer, "FINAL"))
	final = on;
    else if (cc_same(buffer, "NOTFINAL"))
	final = off;
    else
    {
	s_error ("unrecognised token in s_text %s", buffer);
	return (2);
    }

    cc_str(&dat_ptr, txt_buffer);
    no_chars = strlen(txt_buffer);


    /* may have to do the positioning adjustment */
    if (!(dev_info->capability & v_center) && (device_ok))
    {

	switch (a5->text_align.ver)
	{			       /* fix later */
	case normal_v:
	    break;
	case top_v:
	    y -= 1.1 * dev_info->c_height;
	    break;
	case cap_v:
	    y -= dev_info->c_height;
	    break;
	case half_v:
	    y -= 0.5 * dev_info->c_height;
	    break;
	case base_v:
	    break;
	case bottom_v:
	    y -= 0.1 * dev_info->c_height;
	    break;
	case cont_v:
	    break;
	}
    }
    if (!(dev_info->capability & h_center) && (device_ok))
    {
	switch (a5->text_align.hor)
	{			       /* fix later */
	case normal_h:
	    break;
	case left_h:
	    break;
	case center_h:
	    x -= 0.5 * no_chars * dev_info->c_width;
	    break;
	case right_h:
	    x -= no_chars * dev_info->c_width;
	    break;
	case cont_h:
	    break;
	}
    }
    /* set the text */
    if (device_ok)
    {
	ret = (dev_func) ? (*dev_func) (x, y, final, txt_buffer) : 2;
    }
    return (ret);
}
/* restricted text */
static
s_rex_text(dat_ptr, dev_func)
    char           *dat_ptr;
    int             (*dev_func) ();
{
    int             ret = 1;
    int             width, height, xin, yin, x, y;
    enum bool_enum  final;
    char            txt_buffer[2 * max_str + 1];
    char            buffer[max_str];

    width = cc_vdc(&dat_ptr);
    height = cc_vdc(&dat_ptr);
    get_vpoint(&dat_ptr, &xin, &yin);
    x = newx(xin, yin);
    y = newy(xin, yin);

    if (!get_token(&dat_ptr, buffer))
	return (2);
    if (cc_same(buffer, "FINAL"))
	final = on;
    else if (cc_same(buffer, "NOTFINAL"))
	final = off;
    else
    {
	s_error ("unrecognised token in s_text %s", buffer);
	return (2);
    }

    cc_str(&dat_ptr, txt_buffer);

    if (dev_func)
	ret = (*dev_func) (width, height, x, y,
			   (int) final, txt_buffer);

    return (ret);
}
/* appended text */
static
s_app_text(dat_ptr, dev_func)
    char           *dat_ptr;
    int             (*dev_func) ();
{
    int             ret = 1;
    enum bool_enum  final;
    char            txt_buffer[2 * max_str + 1];
    char            buffer[max_str];

    if (!get_token(&dat_ptr, buffer))
	return (2);
    if (cc_same(buffer, "FINAL"))
	final = on;
    else if (cc_same(buffer, "NOTFINAL"))
	final = off;
    else
    {
	s_error ("unrecognised token in s_text %s", buffer);
	return (2);
    }

    cc_str(&dat_ptr, txt_buffer);

    if (dev_func)
	ret = (*dev_func) ((int) final, txt_buffer);
    return (ret);
}

/* handle a polygon */
static
do_polygon(dat_ptr, p_len, dev_func)
    char           *dat_ptr;
    int             p_len;
    int             (*dev_func) ();
{
    int             ret = 1;
    int             no_pairs, x, y, *x_ptr, *y_ptr, *x1_ptr, *y1_ptr;

    no_pairs = get_pairs(dat_ptr);

    /* first get the memory */
    if (no_pairs > wk_ar_size)
    {
	x1_ptr = intalloc(no_pairs * bytes_per_word);
	y1_ptr = intalloc(no_pairs * bytes_per_word);
    } else
    {
	x1_ptr = x_wk_ar;
	y1_ptr = y_wk_ar;
    }
    if ((!x1_ptr) || (!y1_ptr))
	s_error ("trouble with polyline memory");
    x_ptr = x1_ptr;
    y_ptr = y1_ptr;
    if ((!x1_ptr) || (!y1_ptr))
	s_error ("trouble with polygon memory");
    x_ptr = x1_ptr;
    y_ptr = y1_ptr;

    /* now grab the data */
    no_pairs = 0;
    while (!is_term(*dat_ptr))
    {
	get_vpoint(&dat_ptr, &x, &y);
	*x_ptr++ = newx(x, y);
	*y_ptr++ = newy(x, y);
	++no_pairs;
    }

    if (dev_func)
	ret = (*dev_func) (no_pairs, x1_ptr, y1_ptr);

    if (x1_ptr != x_wk_ar)
	free(x1_ptr);
    if (y1_ptr != y_wk_ar)
	free(y1_ptr);

    return (ret);
}
/* do a polyset */
static
do_polyset(dat_ptr, p_len, dev_func)
    char           *dat_ptr;
    int             p_len;
    int             (*dev_func) ();
{
    int             ret = 1;
    int             no_pairs, x, y, *x_ptr, *y_ptr, *x1_ptr, *y1_ptr;
    char           *edge_ptr, buffer[max_str];

    /* figure out how many pairs */
    no_pairs = get_pairs(dat_ptr);

    /* first get the memory */
    if (no_pairs > wk_ar_size)
    {
	x1_ptr = intalloc(no_pairs * bytes_per_word);
	y1_ptr = intalloc(no_pairs * bytes_per_word);
    } else
    {
	x1_ptr = x_wk_ar;
	y1_ptr = y_wk_ar;
    }
    edge_ptr = (char *) allocate_mem(no_pairs, 0);
    if (!edge_ptr)
	return (2);
    if ((!x1_ptr) || (!y1_ptr))
	s_error ("trouble with polyset memory");
    x_ptr = x1_ptr;
    y_ptr = y1_ptr;

    /* now grab the data */
    no_pairs = 0;
    while (!is_term(*dat_ptr))
    {
	get_vpoint(&dat_ptr, &x, &y);
	*x_ptr++ = newx(x, y);
	*y_ptr++ = newy(x, y);
	if (!get_token(&dat_ptr, buffer))
	    return (2);
	if (cc_same(buffer, "INVIS"))
	    edge_ptr[no_pairs] = 0;
	else if (cc_same(buffer, "VIS"))
	    edge_ptr[no_pairs] = 1;
	else if (cc_same(buffer, "CLOSEINVIS"))
	    edge_ptr[no_pairs] = 2;
	else if (cc_same(buffer, "CLOSEVIS"))
	    edge_ptr[no_pairs] = 3;
	else
	{
	    s_error ("unrecognised token in do_polyset %s",
		    buffer);
	    return (2);
	}
	++no_pairs;
    }

    if (dev_func)
	ret = (*dev_func) (no_pairs, x1_ptr, y1_ptr, edge_ptr);

    free(edge_ptr);
    if (x1_ptr != x_wk_ar)
	free(x1_ptr);
    if (y1_ptr != y_wk_ar)
	free(y1_ptr);
    return (ret);
}
/* read in a cell list */
static int get_clist(in_ptr, out_ptr, nx, c_s_mode, col_prec)
    char          **in_ptr, *out_ptr;
    enum cs_enum    c_s_mode;	       /* the colour selection mode,
					* indexed or direct */
    int             col_prec;	       /* colour precision in bits */
    int             nx;
{
    char            paren_flag;
    int             i, k, b_entry;
    unsigned int    inval;

    b_entry = (col_prec / byte_size);  /* bytes per entry */
    while (**in_ptr == ' ')
	++(*in_ptr);		       /* skip spaces */

    paren_flag = (**in_ptr == '(');
    if (paren_flag)
	++(*in_ptr);

    switch (c_s_mode)
    {
    case d_c_mode:		       /* direct colour */
	for (i = 0; (i < 3 * nx) && (**in_ptr != ')') && (**in_ptr); ++i)
	{
	    while (**in_ptr == ' ')
		++(*in_ptr);	       /* skip spaces */
	    inval = cc_int(in_ptr);
	    for (k = b_entry - 1; k >= 0; --k)
	    {
		*(out_ptr + k) = inval & 255;
		inval >>= byte_size;
	    }
	    out_ptr += b_entry;
	    while ((**in_ptr == ' ') || (**in_ptr == ','))
		++(*in_ptr);	       /* skip spaces */
	}
	break;
    case i_c_mode:		       /* indexed colour */
	for (i = 0; (i < nx) && (**in_ptr != ')') && (**in_ptr); ++i)
	{
	    while (**in_ptr == ' ')
		++(*in_ptr);	       /* skip spaces */
	    inval = cc_int(in_ptr);
	    for (k = b_entry - 1; k >= 0; --k)
	    {
		*(out_ptr + k) = inval & 255;
		inval >>= byte_size;
	    }
	    out_ptr += b_entry;
	    while ((**in_ptr == ' ') || (**in_ptr == ','))
		++(*in_ptr);	       /* skip spaces */
	}
	break;
    }
    if (**in_ptr == ')')
    {
	if (!paren_flag)
	    s_error ("unmatched parens in clist");
	++(*in_ptr);
    }
    while (**in_ptr == ' ')
	++(*in_ptr);		       /* skip spaces */
    return (1);
}

/* now try to take care of a general cell array */
static
do_cell_array(dat_ptr, plen, dev_func)
    char           *dat_ptr;
    int             plen, (*dev_func) ();
{
    int             ret = 1, new_max;
    int             p0[2], cp[2];      /* corner p */
    int             q0[2], cq[2];      /* corner q */
    int             r0[2], cr[2];      /* corner r */
    /*
     * this is a parallelogram, diagonal between p and q, first row is
     * p-r
     */
    unsigned int    nx;		       /* columns of data */
    unsigned int    ny;		       /* rows of data */
    int             l_col_prec;	       /* local colour precision */
    int             rep_mode;	       /* cell representation mode */
    int             i, row_size, c_size;
    char           *new_ptr, *my_ptr;
    long int        no_bytes;	       /* number of bytes of data */

    get_vpoint(&dat_ptr, p0, p0 + 1);
    get_vpoint(&dat_ptr, q0, q0 + 1);
    get_vpoint(&dat_ptr, r0, r0 + 1);

    nx = cc_int(&dat_ptr);
    ny = cc_int(&dat_ptr);

    new_max = cc_int(&dat_ptr);
    l_col_prec = (new_max) ? get_prec(0, new_max, 1) : 0;
    l_col_prec = (l_col_prec) ? l_col_prec : glbl1->col_prec;
    if (l_col_prec < 8)
	l_col_prec = 8;		       /* fix later */
    rep_mode = 1;		       /* only possibility for clear
					* text */

    /* get the precision right */
    switch (a2->c_s_mode)
    {
    case d_c_mode:
	c_size = 3 * l_col_prec;
	break;
    case i_c_mode:
	c_size = l_col_prec;
	break;
    }
    cp[0] = newx(p0[0], p0[1]);
    cp[1] = newy(p0[0], p0[1]);
    cq[0] = newx(q0[0], q0[1]);
    cq[1] = newy(q0[0], q0[1]);
    cr[0] = newx(r0[0], r0[1]);
    cr[1] = newy(r0[0], r0[1]);

    /* we will encode the data in the binary CGM format */
    /* figure how much memory we need */
    row_size = (nx * c_size + byte_size - 1) / byte_size;
    row_size = (row_size % 2) ? row_size + 1 : row_size;	/* round up */

    no_bytes = ny * row_size;
    /* zero the memory in case of incomplete lists */
    if (!(new_ptr = (char *) allocate_mem(no_bytes, 1)))
	return (2);

    /* now read in the cell rows */
    my_ptr = new_ptr;
    for (i = 0; (i < ny) && (*dat_ptr); ++i)
    {
	get_clist(&dat_ptr, my_ptr, nx, a2->c_s_mode, l_col_prec);
	my_ptr += row_size;
    }


    if (dev_func)
	ret = (*dev_func) (cp, cq, cr, nx, ny, l_col_prec,
			   new_ptr, rep_mode, no_bytes);
    if ((new_ptr) && (no_bytes > 0))
	free(new_ptr);
    return (ret);
}
/* generalised drawing primitive */
/* format is identifier, no_pairs, list of points, set of strings */
static
do_g_d_p(dat_ptr, plen, dev_func)
    char           *dat_ptr;
    int             plen, (*dev_func) ();
{
    int             ret = 1;
    int             gdp_id, no_pairs, *x_ptr, *y_ptr, x, y;
    char           *data_record;


    gdp_id = cc_int(&dat_ptr);

    no_pairs = cc_int(&dat_ptr);

    /* first get the memory */
    if (no_pairs > wk_ar_size)
    {
	x_ptr = intalloc(no_pairs * bytes_per_word);
	y_ptr = intalloc(no_pairs * bytes_per_word);
    } else
    {
	x_ptr = x_wk_ar;
	y_ptr = y_wk_ar;
    }
    /* now grab the data */
    no_pairs = 0;
    while (!is_term(*dat_ptr))
    {
	get_vpoint(&dat_ptr, &x, &y);
	x_ptr[no_pairs] = newx(x, y);
	y_ptr[no_pairs] = newy(x, y);
	++no_pairs;
    }
    data_record = cc_str(&dat_ptr, NULL);

    if (dev_func)
	ret = (*dev_func)
	    (gdp_id, no_pairs, x_ptr, y_ptr, data_record);

    if (x_ptr != x_wk_ar)
	free(x_ptr);
    if (y_ptr != y_wk_ar)
	free(y_ptr);

    return (ret);
}
/* do a rectangle */
static
do_rectangle(dat_ptr, dev_func)
    char           *dat_ptr;
    int             (*dev_func) ();
{
    int             ret = 1;
    int             x_1, y_1, x_2, y_2;

    get_vpoint(&dat_ptr, &x_1, &y_1);
    get_vpoint(&dat_ptr, &x_2, &y_2);

    ret = (dev_func) ? (*dev_func) (x_1, y_1, x_2, y_2) :
	em_rectangle(x_1, y_1, x_2, y_2);
    return (ret);
}
/* set a circle */
static
do_circle(dat_ptr, dev_func)
    char           *dat_ptr;
    int             (*dev_func) ();
{
    int             ret = 1, x1, y1, r, x, y;


    get_vpoint(&dat_ptr, &x1, &y1);
    x = newx(x1, y1);
    y = newy(x1, y1);

    r = cc_vdc(&dat_ptr);

    ret = (dev_func) ? (*dev_func) (x, y, r) : em_circle(x, y, r);

    return (ret);
}
/* set an arc, get the positions of 1st pt, intermdiate pt, end pt */
static
do_c3(dat_ptr, dev_func)
    char           *dat_ptr;
    int             (*dev_func) ();
{
    int             ret = 1, x_array[6], i, first_array[6];

    for (i = 0; i < 6; i += 2)
	get_vpoint(&dat_ptr, first_array + i, first_array + i + 1);

    for (i = 0; i < 3; ++i)
    {
	x_array[2 * i] = newx(first_array[2 * i], first_array[2 * i + 1]);
	x_array[2 * i + 1] = newy(first_array[2 * i], first_array[2 * i + 1]);
    }

    ret = (dev_func) ? (*dev_func) (x_array) : em_c3(x_array);
    return (ret);
}
/*
 * set a closed arc, get the positions of 1st pt, intermdiate pt, end
 * pt
 */
static
do_c3_close(dat_ptr, dev_func)
    char           *dat_ptr;
    int             (*dev_func) ();
{
    int             ret = 1, x_array[6], i, first_array[6];
    enum bool_enum  chord;
    char            buffer[max_str];
    for (i = 0; i < 6; i += 2)
	get_vpoint(&dat_ptr, first_array + i, first_array + i + 1);

    if (!get_token(&dat_ptr, buffer))
	return (2);
    if (cc_same(buffer, "CHORD"))
	chord = on;
    else if (cc_same(buffer, "PIE"))
	chord = off;
    else
    {
	s_error ("unrecognised token in do_c3_close %s", buffer);
	return (2);
    }

    for (i = 0; i < 3; ++i)
    {
	x_array[2 * i] = newx(first_array[2 * i], first_array[2 * i + 1]);
	x_array[2 * i + 1] = newy(first_array[2 * i], first_array[2 * i + 1]);
    }

    ret = (dev_func) ? (*dev_func) (x_array, chord) :
	em_c3_close(x_array, chord);
    return (ret);
}
/* set an arc, ends specified by vectors */
static
do_c_centre(dat_ptr, dev_func)
    char           *dat_ptr;
    int             (*dev_func) ();
{
    int             ret = 1, vec_array[4], i, x, y, r, x1, y1;

    get_vpoint(&dat_ptr, &x1, &y1);
    for (i = 0; i < 4; i += 2)
	get_vpoint(&dat_ptr, vec_array + i,
		   vec_array + i + 1);
    r = cc_vdc(&dat_ptr);
    x = newx(x1, y1);
    y = newy(x1, y1);

    ret = (dev_func) ? (*dev_func) (x, y, vec_array, r) :
	em_c_centre(x, y, vec_array, r);
    return (ret);
}
/* set an arc, ends specified by vectors, close it */
static
do_c_c_close(dat_ptr, dev_func)
    char           *dat_ptr;
    int             (*dev_func) ();
{
    int             ret = 1, vec_array[4], i, x, y, r, x1, y1;
    enum bool_enum  chord;
    char            buffer[max_str];

    get_vpoint(&dat_ptr, &x1, &y1);
    x = newx(x1, y1);
    y = newy(x1, y1);

    for (i = 0; i < 4; i += 2)
	get_vpoint(&dat_ptr, vec_array + i,
		   vec_array + i + 1);
    r = cc_vdc(&dat_ptr);
    if (!get_token(&dat_ptr, buffer))
	return (2);
    if (cc_same(buffer, "CHORD"))
	chord = on;
    else if (cc_same(buffer, "PIE"))
	chord = off;
    else
    {
	s_error ("unrecognised token in do_c_c_close %s", buffer);
	return (2);
    }
    ret = (dev_func) ? (*dev_func) (x, y, vec_array, r, chord) :
	em_c_c_close(x, y, vec_array, r, chord);
    return (ret);
}
/*
 * set an ellipse, specify centre, two conjugate diameters (see the
 * book !)
 */
static
do_ellipse(dat_ptr, dev_func)
    char           *dat_ptr;
    int             (*dev_func) ();
{
    int             ret = 1, pt_array[6], i;

    for (i = 0; i < 6; i += 2)
	get_vpoint(&dat_ptr, pt_array + i, pt_array + i + 1);

    if (dev_func)
	ret = (*dev_func) (pt_array);
    return (ret);
}
/*
 * set an elliptical arc, specify centre two conjugate diameters end
 * vectors
 */
static
do_ell_arc(dat_ptr, dev_func)
    char           *dat_ptr;
    int             (*dev_func) ();
{
    int             ret = 1, pt_array[6], vec_array[4], i;

    for (i = 0; i < 6; i += 2)
	get_vpoint(&dat_ptr, pt_array + i, pt_array + i + 1);

    for (i = 0; i < 4; i += 2)
	get_vpoint(&dat_ptr, vec_array + i, vec_array + i + 1);

    if (dev_func)
	ret = (*dev_func) (pt_array, vec_array);
    return (ret);
}
/* set an elliptical arc, close it */
static
do_e_a_close(dat_ptr, dev_func)
    char           *dat_ptr;
    int             (*dev_func) ();
{
    int             ret = 1, pt_array[6], vec_array[4], i;
    enum bool_enum  chord;
    char            buffer[max_str];

    for (i = 0; i < 6; i += 2)
	get_vpoint(&dat_ptr, pt_array + i, pt_array + i + 1);

    for (i = 0; i < 4; i += 2)
	get_vpoint(&dat_ptr, vec_array + i, vec_array + i + 1);

    if (!get_token(&dat_ptr, buffer))
	return (2);
    if (cc_same(buffer, "CHORD"))
	chord = on;
    else if (cc_same(buffer, "PIE"))
	chord = off;
    else
    {
	s_error ("unrecognised token in do_e_a_close %s", buffer);
	return (2);
    }

    if (dev_func)
	ret = (*dev_func) (pt_array, vec_array, chord);
    return (ret);
}

/* now the class5 functions */
/* set the line bundle index */
static
s_lbindex(dat_ptr, dev_func)
    int             (*dev_func) ();
    char           *dat_ptr;
{
    int             ret = 1;
    int             new_lbindex;
    new_lbindex = cc_int(&dat_ptr);
    a5->l_b_index = new_lbindex;

    if (dev_func)
	ret = (*dev_func) (a5->l_b_index);

    return (ret);
}
/* set the line type */
static
s_l_type(dat_ptr, dev_func)
    int             (*dev_func) ();
    char           *dat_ptr;
{
    int             ret = 1;
    int             new_l_type;
    new_l_type = cc_int(&dat_ptr);
    a5->line_type = (int) new_l_type;

    if (dev_func)
	ret = (*dev_func) (a5->line_type);

    return (ret);
}
/* set the line width */
static
s_l_width(dat_ptr, dev_func)
    int             (*dev_func) ();
    char           *dat_ptr;
{
    int             ret = 1;
    float           rmul;
    int             new_l_width;

    if (!dev_info->d_l_width)
	dev_info->d_l_width = 1;       /* safety */

    if (a2->l_w_s_mode == absolute)
    {
	new_l_width = cc_vdc(&dat_ptr) + 0.5;
	a5->line_width.i = new_l_width;
    } else if (a2->l_w_s_mode == scaled)
    {
	rmul = cc_real(&dat_ptr);
	a5->line_width.r = rmul;
	a5->line_width.i = rmul * dev_info->d_l_width;
    } else
	s_error ("illegal line spec mode ");

    if (dev_func)
	ret = (*dev_func)
	    (a5->line_width.i, (float) a5->line_width.i / dev_info->d_l_width);

    return (ret);
}
/* set the line colour */
static
s_l_colour(dat_ptr, dev_func)
    int             (*dev_func) ();
    char           *dat_ptr;
{
    int             ret = 1;
    int             new_index = -1, ir, ig, ib;
    float           r, g, b, *rptr;
    switch ((int) a2->c_s_mode)
    {
    case (int) i_c_mode:	       /* indexed mode */
	new_index = cc_int(&dat_ptr);
	a5->line_colour.ind = new_index;
	rptr = a5->ctab + a5->line_colour.ind * 3;
	r = *rptr;
	g = *++rptr;
	b = *++rptr;
	break;
    case (int) d_c_mode:	       /* direct mode */
	ir = cc_int(&dat_ptr);
	r = dcr(ir);
	ig = cc_int(&dat_ptr);
	g = dcg(ig);
	ib = cc_int(&dat_ptr);
	b = dcb(ib);
	a5->line_colour.red = r;
	a5->line_colour.green = g;
	a5->line_colour.blue = b;
	break;
    default:
	s_error ("illegal colour mode ");
    }
    if (dev_func)
	ret = (*dev_func) (r, g, b, new_index);
    return (ret);
}
/* set the marker bundle index */
static
s_mbindex(dat_ptr, dev_func)
    int             (*dev_func) ();
    char           *dat_ptr;
{
    int             ret = 1;
    int             new_index;
    new_index = cc_int(&dat_ptr);
    a5->mk_b_index = new_index;

    if (dev_func)
	ret = (*dev_func) (a5->mk_b_index);

    return (ret);
}
/* set the marker type */
static
s_mk_type(dat_ptr, dev_func)
    int             (*dev_func) ();
    char           *dat_ptr;
{
    int             ret = 1;
    int             new_mk_type;
    new_mk_type = cc_int(&dat_ptr);
    a5->mk_type = new_mk_type;
    if (dev_func)
	ret = (*dev_func) (a5->mk_type);
    return (ret);
}
/* set the marker size */
static
s_mk_size(dat_ptr, dev_func)
    int             (*dev_func) ();
    char           *dat_ptr;
{
    int             ret = 1;
    float           rmul;
    int             new_mk_size;

    if (!dev_info->d_m_size)
	dev_info->d_m_size = 1;	       /* safety */

    if (a2->m_s_s_mode == absolute)
    {
	new_mk_size = cc_vdc(&dat_ptr);
	a5->mk_size.i = new_mk_size;
    } else if (a2->m_s_s_mode == scaled)
    {
	rmul = cc_real(&dat_ptr);
	a5->mk_size.r = rmul;
	a5->mk_size.i = rmul * dev_info->d_m_size;
    } else
	s_error ("illegal marker size mode ");

    if (dev_func)
	ret = (*dev_func)
	    (a5->mk_size.i, (float) a5->mk_size.i / dev_info->d_m_size);

    return (ret);
}
/* set the marker colour */
static
s_mk_colour(dat_ptr, dev_func)
    int             (*dev_func) ();
    char           *dat_ptr;
{
    int             ret = 1;
    int             new_index = -1, ir, ig, ib;
    float           r, g, b, *rptr;


    switch ((int) a2->c_s_mode)
    {
    case (int) i_c_mode:	       /* indexed mode */
	new_index = cc_int(&dat_ptr);
	a5->mk_colour.ind = new_index;
	rptr = a5->ctab + a5->mk_colour.ind * 3;
	r = *rptr;
	g = *++rptr;
	b = *++rptr;
	break;
    case (int) d_c_mode:	       /* direct mode */
	ir = cc_int(&dat_ptr);
	r = dcr(ir);
	ig = cc_int(&dat_ptr);
	g = dcg(ig);
	ib = cc_int(&dat_ptr);
	b = dcb(ib);
	a5->mk_colour.red = r;
	a5->mk_colour.green = g;
	a5->mk_colour.blue = b;
	break;
    default:
	s_error ("illegal colour mode ");
    }
    if (dev_func)
	ret = (*dev_func) (r, g, b, new_index);
    return (ret);
}
/* set the text bundle index */
static
s_tbindex(dat_ptr, dev_func)
    int             (*dev_func) ();
    char           *dat_ptr;
{
    int             ret = 1;
    int             new_index;
    new_index = cc_int(&dat_ptr);
    a5->t_b_index = new_index;

    if (dev_func)
	ret = (*dev_func) (a5->t_b_index);

    return (ret);
}
/* set the text font index */
static
s_t_index(dat_ptr, dev_func)
    int             (*dev_func) ();
    char           *dat_ptr;
{
    int             ret = 1;
    int             new_font_index;
    new_font_index = cc_int(&dat_ptr);
    a5->t_f_index = new_font_index;
    if (dev_func)
	ret = (*dev_func) (new_font_index);
    return (ret);
}
/* set the text precision */
static
s_t_prec(dat_ptr, dev_func)
    int             (*dev_func) ();
    char           *dat_ptr;
{
    int             ret = 1;
    enum txt_enum   new_t_prec;
    char            buffer[max_str];

    if (!get_token(&dat_ptr, buffer))
	return (2);
    if (cc_same(buffer, "STRING"))
	new_t_prec = string;
    else if (cc_same(buffer, "CHARACTER"))
	new_t_prec = character;
    else if (cc_same(buffer, "STROKE"))
	new_t_prec = stroke;
    else
    {
	s_error ("unrecognised token in s_t_prec %s", buffer);
	return (2);
    }

    a5->t_prec = new_t_prec;
    if (dev_func)
	ret = (*dev_func) (new_t_prec);
    return (ret);
}
/* set the character expansion factor */
static
s_c_exp(dat_ptr, dev_func)
    int             (*dev_func) ();
    char           *dat_ptr;
{
    int             ret = 1;
    float           new_c_exp_fac;
    new_c_exp_fac = cc_real(&dat_ptr);
    a5->c_exp_fac = new_c_exp_fac;
    if (dev_func)
	ret = (*dev_func) (new_c_exp_fac);
    return (ret);
}
/* set the character space */
static
s_c_space(dat_ptr, dev_func)
    int             (*dev_func) ();
    char           *dat_ptr;
{
    int             ret = 1;
    float           new_c_space;
    new_c_space = cc_real(&dat_ptr);
    a5->c_space = new_c_space;
    if (dev_func)
	ret = (*dev_func) (a5->c_space);
    return (ret);
}
/* set the text colour */
static
s_t_colour(dat_ptr, dev_func)
    int             (*dev_func) ();
    char           *dat_ptr;
{
    int             ret = 1;
    int             new_index = -1, ir, ig, ib;
    float           r, g, b, *rptr;

    switch ((int) a2->c_s_mode)
    {
    case 0:			       /* indexed mode */
	new_index = cc_int(&dat_ptr);
	a5->text_colour.ind = new_index;
	rptr = a5->ctab + a5->text_colour.ind * 3;
	r = *rptr;
	g = *++rptr;
	b = *++rptr;
	break;
    case 1:			       /* direct mode */
	ir = cc_int(&dat_ptr);
	r = dcr(ir);
	ig = cc_int(&dat_ptr);
	g = dcg(ig);
	ib = cc_int(&dat_ptr);
	b = dcb(ib);
	a5->text_colour.red = r;
	a5->text_colour.green = g;
	a5->text_colour.blue = b;
	break;
    default:
	s_error ("illegal colour mode ");
    }

    if (dev_func)
	ret = (*dev_func) (r, g, b, new_index);

    return (ret);
}
/* set character height */
static
s_c_height(dat_ptr, dev_func)
    int             (*dev_func) ();
    char           *dat_ptr;
{
    int             ret = 1;

    a5->c_height = cc_vdc(&dat_ptr) + 0.5;
    if (dev_func)
	ret = (*dev_func) (a5->c_height);
    return (ret);
}
/* set the character orientation structure */
static
s_c_orient(dat_ptr, dev_func)
    int             (*dev_func) ();
    char           *dat_ptr;
{
    int             ret = 1;
    struct orient_struct new_orient;


    new_orient.x_up = cc_vdc(&dat_ptr);
    new_orient.y_up = cc_vdc(&dat_ptr);
    new_orient.x_base = cc_vdc(&dat_ptr);
    new_orient.y_base = cc_vdc(&dat_ptr);

    a5->c_orient.x_up = new_orient.x_up * sy;
    a5->c_orient.y_up = new_orient.y_up * sy;
    a5->c_orient.x_base = new_orient.x_base * sx;
    a5->c_orient.y_base = new_orient.y_base * sx;
    if (dev_func)
	ret = (*dev_func) (new_orient.x_up,
	       new_orient.y_up, new_orient.x_base, new_orient.y_base);
    return (ret);
}
/* set the text path */
static
s_tpath(dat_ptr, dev_func)
    int             (*dev_func) ();
    char           *dat_ptr;
{
    int             ret = 1;
    enum path_enum  new_path;
    char            buffer[max_str];

    if (!get_token(&dat_ptr, buffer))
	return (2);
    if (cc_same(buffer, "RIGHT"))
	new_path = right;
    else if (cc_same(buffer, "LEFT"))
	new_path = left;
    else if (cc_same(buffer, "UP"))
	new_path = up;
    else if (cc_same(buffer, "DOWN"))
	new_path = down;
    else
    {
	s_error ("unrecognised token in s_tpath %s", buffer);
	return (2);
    }
    a5->text_path = new_path;
    if (dev_func)
	ret = (*dev_func) (new_path);
    return (ret);
}
/* set the text alignment */
static
s_t_align(dat_ptr, dev_func)
    int             (*dev_func) ();
    char           *dat_ptr;
{
    int             ret = 1;
    struct align_struct new_align;
    char            buffer[max_str];

    if (!get_token(&dat_ptr, buffer))
	return (2);
    if (cc_same(buffer, "NORMHORIZ"))
	new_align.hor = normal_h;
    else if (cc_same(buffer, "LEFT"))
	new_align.hor = left_h;
    else if (cc_same(buffer, "CTR"))
	new_align.hor = center_h;
    else if (cc_same(buffer, "RIGHT"))
	new_align.hor = right_h;
    else if (cc_same(buffer, "CONTHORIZ"))
	new_align.hor = cont_h;
    else
    {
	s_error ("unrecognised token in s_t_align %s", buffer);
	return (2);
    }

    if (!get_token(&dat_ptr, buffer))
	return (2);
    if (cc_same(buffer, "NORMVERT"))
	new_align.ver = normal_v;
    else if (cc_same(buffer, "TOP"))
	new_align.ver = top_v;
    else if (cc_same(buffer, "CAP"))
	new_align.ver = cap_v;
    else if (cc_same(buffer, "HALF"))
	new_align.ver = half_v;
    else if (cc_same(buffer, "BASE"))
	new_align.ver = base_v;
    else if (cc_same(buffer, "BOTTOM"))
	new_align.ver = bottom_v;
    else if (cc_same(buffer, "CONTVERT"))
	new_align.ver = cont_v;
    else
    {
	s_error ("unrecognised token in s_t_align %s", buffer);
	return (2);
    }

    new_align.cont_hor = cc_real(&dat_ptr);
    new_align.cont_ver = cc_real(&dat_ptr);

    a5->text_align.hor = new_align.hor;
    a5->text_align.ver = new_align.ver;
    a5->text_align.cont_hor = new_align.cont_hor;
    a5->text_align.cont_ver = new_align.cont_ver;

    if (dev_func)
	ret = (*dev_func) (a5->text_align.hor,
			   a5->text_align.ver, a5->text_align.cont_hor,
			   a5->text_align.cont_ver);

    return (ret);
}
/* set the character set index */
static
s_csindex(dat_ptr, dev_func)
    int             (*dev_func) ();
    char           *dat_ptr;
{
    int             ret = 1, new_index;

    new_index = cc_int(&dat_ptr);
    a5->c_set_index = new_index;
    if (dev_func)
	ret = (*dev_func) (new_index);
    return (ret);
}
/* set the alternate character set index */
static
s_acsindex(dat_ptr, dev_func)
    int             (*dev_func) ();
    char           *dat_ptr;
{
    int             ret = 1, new_index;

    new_index = cc_int(&dat_ptr);
    a5->a_c_set_index = new_index;
    if (dev_func)
	ret = (*dev_func) (new_index);
    return (ret);
}
/* set the fill bundle index */
static
s_fbindex(dat_ptr, dev_func)
    int             (*dev_func) ();
    char           *dat_ptr;
{
    int             ret = 1, new_index;

    new_index = cc_int(&dat_ptr);
    a5->f_b_index = new_index;
    if (dev_func)
	ret = (*dev_func) (new_index);
    return (ret);
}

/* set the interior style */
static
s_interior_style(dat_ptr, dev_func)
    int             (*dev_func) ();
    char           *dat_ptr;
{
    int             ret = 1;
    enum is_enum    new_style;
    char            buffer[max_str];

    if (!get_token(&dat_ptr, buffer))
	return (2);
    if (cc_same(buffer, "HOLLOW"))
	new_style = hollow;
    else if (cc_same(buffer, "SOLID"))
	new_style = solid_i;
    else if (cc_same(buffer, "PAT"))
	new_style = pattern;
    else if (cc_same(buffer, "HATCH"))
	new_style = hatch;
    else if (cc_same(buffer, "EMPTY"))
	new_style = empty;
    else
    {
	s_error ("unrecognised token in s_interior_style %s",
		buffer);
	return (2);
    }
    a5->int_style = (enum is_enum) new_style;
    if (dev_func)
	ret = (*dev_func) (a5->int_style);
    return (ret);
}
/* set the fill colour */
static
int s_fill_colour(dat_ptr, dev_func)
    int             (*dev_func) ();
    char           *dat_ptr;
{
    int             ret = 1;
    int             new_index = -1, ir, ig, ib;
    float           r, g, b, *rptr;


    switch ((int) a2->c_s_mode)
    {
    case (int) i_c_mode:	       /* indexed mode */
	new_index = cc_int(&dat_ptr);
	a5->fill_colour.ind = new_index;
	rptr = a5->ctab + a5->fill_colour.ind * 3;
	r = *rptr;
	g = *++rptr;
	b = *++rptr;
	break;
    case (int) d_c_mode:	       /* direct mode */
	ir = cc_int(&dat_ptr);
	r = dcr(ir);
	ig = cc_int(&dat_ptr);
	g = dcg(ig);
	ib = cc_int(&dat_ptr);
	b = dcb(ib);
	a5->fill_colour.red = r;
	a5->fill_colour.green = g;
	a5->fill_colour.blue = b;
	break;
    default:
	s_error ("illegal colour mode");
    }

    if (dev_func)
	ret = (*dev_func) (r, g, b, new_index);
    return (ret);
}
/* set the hatch index */
static
s_hindex(dat_ptr, dev_func)
    int             (*dev_func) ();
    char           *dat_ptr;
{
    int             ret = 1, new_index;

    new_index = cc_int(&dat_ptr);
    a5->hatch_index = new_index;
    if (dev_func)
	ret = (*dev_func) (new_index);
    return (ret);
}
/* set the pattern index */
static
s_pindex(dat_ptr, dev_func)
    int             (*dev_func) ();
    char           *dat_ptr;
{
    int             ret = 1, new_index;

    new_index = cc_int(&dat_ptr);
    a5->pat_index = new_index;
    if (dev_func)
	ret = (*dev_func) (new_index);
    return (ret);
}
/* set the edge bundle index */
static
s_e_b_index(dat_ptr, dev_func)
    int             (*dev_func) ();
    char           *dat_ptr;
{
    int             ret = 1, new_index;

    new_index = cc_int(&dat_ptr);
    a5->e_b_index = new_index;
    if (dev_func)
	ret = (*dev_func) (new_index);
    return (ret);
}
/* set the edge type */
static
s_edge_t(dat_ptr, dev_func)
    int             (*dev_func) ();
    char           *dat_ptr;
{
    int             ret = 1;
    int             new_l_type;
    new_l_type = cc_int(&dat_ptr);
    a5->edge_type = (int) new_l_type;

    if (dev_func)
	ret = (*dev_func) (a5->edge_type);

    return (ret);
}

/* set the edge width */
static
s_edge_w(dat_ptr, dev_func)
    int             (*dev_func) ();
    char           *dat_ptr;
{
    int             ret = 1;
 
    float           rmul;
    int             new_e_width;

    if (!dev_info->d_e_width)
	dev_info->d_e_width = 1;       /* safety */

    if (a2->e_w_s_mode == absolute)
    {
	new_e_width = cc_vdc(&dat_ptr) + 0.5;
	a5->edge_width.i = new_e_width;
    } else if (a2->e_w_s_mode == scaled)
    {
	rmul = cc_real(&dat_ptr);
	a5->edge_width.r = rmul;
	a5->edge_width.i = rmul * dev_info->d_e_width;
    } else
	s_error ("illegal edge spec mode ");

    if (dev_func)
	ret = (*dev_func)
	    (a5->edge_width.i, (float) a5->edge_width.i / dev_info->d_e_width);

    return (ret);
}
/* set the edge colour */
static
s_edge_c(dat_ptr, dev_func)
    int             (*dev_func) ();
    char           *dat_ptr;
{
    int             ret = 1;
    int             new_index = -1, ir, ig, ib;
    float           r, g, b, *rptr;
    switch ((int) a2->c_s_mode)
    {
    case (int) i_c_mode:	       /* indexed mode */
	new_index = cc_int(&dat_ptr);
	a5->edge_colour.ind = new_index;
	rptr = a5->ctab + a5->line_colour.ind * 3;
	r = *rptr;
	g = *++rptr;
	b = *++rptr;
	break;
    case (int) d_c_mode:	       /* direct mode */
	ir = cc_int(&dat_ptr);
	r = dcr(ir);
	ig = cc_int(&dat_ptr);
	g = dcg(ig);
	ib = cc_int(&dat_ptr);
	b = dcb(ib);
	a5->edge_colour.red = r;
	a5->edge_colour.green = g;
	a5->edge_colour.blue = b;
	break;
    default:
	s_error ("illegal colour mode ");
    }
    if (dev_func)
	ret = (*dev_func) (r, g, b, new_index);
    return (ret);
}
/* set the edge visibility */
static
s_edge_v(dat_ptr, dev_func)
    int             (*dev_func) ();
    char           *dat_ptr;
{
    int             ret = 1;
    enum bool_enum  new_flag;
    char            buffer[max_str];

    if (!get_token(&dat_ptr, buffer))
	return (2);
    if (cc_same(buffer, "ON"))
	new_flag = on;
    else if (cc_same(buffer, "OFF"))
	new_flag = off;
    else
    {
	s_error ("unrecognised token in s_edge_v %s", buffer);
	return (2);
    }
    a5->edge_vis = new_flag;
    if (dev_func)
	ret = (*dev_func) (new_flag);
    return (ret);
}

/* set the fill reference point */
static
s_fill_ref(dat_ptr, dev_func)
    int             (*dev_func) ();
    char           *dat_ptr;
{
    int             ret = 1;
    int             new_fill[2], i, x, y;

    get_vpoint(&dat_ptr, &x, &y);

    new_fill[0] = newx(x, y);
    new_fill[1] = newy(x, y);

    for (i = 0; i < 2; ++i)
	a5->fill_ref.i[i] = new_fill[i];
    if (dev_func)
	ret = (*dev_func) (a5->fill_ref.i[0], a5->fill_ref.i[1]);
    return (ret);
}
/* make a pattern table entry */
/* not really implemented yet */
static
p_tab_entry(dat_ptr, p_len, dev_func)
    int             p_len;
    int             (*dev_func) ();
    char           *dat_ptr;
{
    int             ret = 1;
    int             index, nx, ny, col_prec, no_bytes, l_col_prec;

    index = cc_int(&dat_ptr);
    nx = cc_int(&dat_ptr);
    ny = cc_int(&dat_ptr);
    col_prec = cc_int(&dat_ptr);
    if (a2->c_s_mode == d_c_mode)
    {				       /* direct */
	l_col_prec = (col_prec) ? col_prec : glbl1->col_prec;
    } else
    {				       /* indexed */
	l_col_prec = (col_prec) ? col_prec : glbl1->col_i_prec;
    }

    no_bytes = nx * ny * l_col_prec / byte_size;

    if (dev_func)
	ret = (*dev_func) (index, nx, ny, col_prec, dat_ptr,
			   no_bytes);
    return (ret);
}

/* set the pattern size */
static
s_pat_size(dat_ptr, dev_func)
    int             (*dev_func) ();
    char           *dat_ptr;
{
    int             ret = 1;
    int             new_size[4], i;

    for (i = 0; i < 4; ++i)
    {
	new_size[i] = cc_vdc(&dat_ptr) + 0.5;
    }
    for (i = 0; i < 4; ++i)
	a5->pat_size.i[i] = new_size[i];
    if (dev_func)
	ret = (*dev_func) (a5->pat_size.i);
    return (ret);
}

/* make a colour table entry */
static
c_tab_entry(dat_ptr, p_len, dev_func)
    int             p_len;
    int             (*dev_func) ();
    char           *dat_ptr;
{
    int             ret = 1;
    int             beg_index, i, no_entries, col[3], j;
    float           rcol[3];

    beg_index = cc_int(&dat_ptr);

    no_entries = 0;
    for (i = beg_index; (!is_term(*dat_ptr)) && (i < glbl1->max_c_index);
	 ++i)
    {
	for (j = 0; j < 3; ++j)
	{
	    col[j] = cc_int(&dat_ptr);
	    rcol[j] = dcind(j, col[j], glbl1->c_v_extent);
	}

	for (j = 0; j < 3; ++j)
	    *(a5->ctab + 3 * i + j) = rcol[j];
	++no_entries;
	while (*dat_ptr == ' ')
	    ++dat_ptr;
    }
    if (dev_func)
	ret = (*dev_func) (beg_index, no_entries, a5->ctab);
    return (ret);
}
/* take care of the asps flags */
static
do_aspsflags(dat_ptr, p_len, dev_func)
    int             p_len;
    int             (*dev_func) ();
    char           *dat_ptr;
{
#define MAX_PAIRS 1024		       /* should be enough ! */
    int             ret = 1, bundled, no_pairs, flags[2 * MAX_PAIRS], i,
		    j;
    char            buffer[max_str], buffer1[max_str];

    no_pairs = 0;
    while (*dat_ptr == ' ')
	++dat_ptr;
    while ((!is_term(*dat_ptr)) && (no_pairs < (MAX_PAIRS - NO_ASPS_FLAGS)))
    {
	if (!get_token(&dat_ptr, buffer))
	    return (2);
	if (!get_token(&dat_ptr, buffer1))
	    return (2);
	if (cc_same(buffer1, "INDIV"))
	    bundled = 0;
	else if (cc_same(buffer1, "BUNDLED"))
	    bundled = 1;
	else
	{
	    s_error ("unknown flag in cc_aspsflags [%s]",
		    buffer1);
	    return (2);
	}
	/* now figure out what we have */
	for (i = 0; (i < NO_ASPS_FLAGS) && (!cc_same(asps_flags[i], buffer));
	     ++i);
	if (i < NO_ASPS_FLAGS)
	{			       /* found one */
	    flags[2 * no_pairs] = i;
	    flags[2 * no_pairs + 1] = bundled;
	} else if (cc_same(buffer, "ALL"))
	{			       /* do them all */
	    for (j = 0; j < NO_ASPS_FLAGS; ++j)
	    {
		flags[2 * no_pairs] = j;
		flags[2 * no_pairs + 1] = bundled;
		++no_pairs;
	    }
	} else if (cc_same(buffer, "ALLLINE"))
	{			       /* all line stuff */
	    for (j = 0; j < 3; ++j)
	    {
		flags[2 * no_pairs] = j;
		flags[2 * no_pairs + 1] = bundled;
		++no_pairs;
	    }
	} else if (cc_same(buffer, "ALLMARKER"))
	{			       /* all marker stuff */
	    for (j = 3; j < 6; ++j)
	    {
		flags[2 * no_pairs] = j;
		flags[2 * no_pairs + 1] = bundled;
		++no_pairs;
	    }
	} else if (cc_same(buffer, "ALLTEXT"))
	{			       /* all text stuff */
	    for (j = 6; j < 11; ++j)
	    {
		flags[2 * no_pairs] = j;
		flags[2 * no_pairs + 1] = bundled;
		++no_pairs;
	    }
	} else if (cc_same(buffer, "ALLFILL"))
	{			       /* all fill stuff */
	    for (j = 11; j < 15; ++j)
	    {
		flags[2 * no_pairs] = j;
		flags[2 * no_pairs + 1] = bundled;
		++no_pairs;
	    }
	} else if (cc_same(buffer, "ALLEDGE"))
	{			       /* all edge stuff */
	    for (j = 15; j < 18; ++j)
	    {
		flags[2 * no_pairs] = j;
		flags[2 * no_pairs + 1] = bundled;
		++no_pairs;
	    }
	} else
	{
	    s_error ("unknown ASPS flag [%s]", buffer);
	    return (2);
	}
	while (*dat_ptr == ' ')
	    ++dat_ptr;
	++no_pairs;
    }
    if (dev_func)
	ret = (*dev_func) (no_pairs, flags);
    return (ret);
}
/* now the class6 functions */
/* do the special command */
static
do_escape(dat_ptr, dev_func)
    char           *dat_ptr;
    int             (*dev_func) ();
{
    int             ret = 1;
    int             escape_id;
    char           *str_ptr;

    escape_id = cc_int(&dat_ptr);
    str_ptr = cc_str(&dat_ptr, NULL);

    if (dev_func)
	ret = (*dev_func) (escape_id, str_ptr);

    ret = 1;
    return (ret);
}
/* now the class 7 functions */
/* message command */
static
do_message(dat_ptr, dev_func)
    char           *dat_ptr;
    int             (*dev_func) ();
{
    int             ret = 1;
    char           *str_ptr;
    enum bool_enum  action;

    action = (enum bool_enum) cc_int(&dat_ptr);
    str_ptr = cc_str(&dat_ptr, NULL);

    if (dev_func)
	ret = (*dev_func) (action, str_ptr);

    ret = 1;
    return (ret);
}
/* application data */
static
do_apdata(dat_ptr, dev_func)
    char           *dat_ptr;
    int             (*dev_func) ();
{
    int             ret = 1;
    char           *str_ptr;
    int             id;

    id = cc_int(&dat_ptr);
    str_ptr = cc_str(&dat_ptr, NULL);

    if (dev_func)
	ret = (*dev_func) (id, str_ptr);

    ret = 1;
    return (ret);
}
/* function to read in a complete data record */
static int get_record(local_rec, size)
    unsigned char  *local_rec;
    int             size;
{
    int             ret;
    ++recs_got;			       /* at least tried */
    ret = fread(local_rec, 1, size, inptr);
    bytes_read += ret;
    return (ret);
}

/* get the next clear text byte */
static char clear_byte(out_ad)
    struct ad_struct *out_ad;	       /* address of command */
{
    int             bytes_got;
    char            cret;

    /* fill out the command address, may be about to get another record */
    out_ad->r_ad = recs_got;
    out_ad->offset = buf_ptr - start_buf;
    if (buf_ptr >= end_buf)
    {				       /* need new record */
	out_ad->offset = 0;
	++out_ad->r_ad;
    }
    out_ad->b_ad = bytes_read - (end_buf - buf_ptr);

    if (buf_ptr >= end_buf)
    {				       /* need new record */
	bytes_got = get_record(start_buf, buf_size);
	if (bytes_got < 0)
	{
	    s_error ("trouble getting the next clear record");
	    return (0);
	}
	buf_ptr = start_buf;
	end_buf = buf_ptr + bytes_got;
	/* send back a space for the end of record for VMS */
    }
    cret = *buf_ptr++;
    return (cret);
}

/*
 * take care of getting the next clear text command and its data into
 * memory
 */
/* need to be careful about recognising quotes, see CGM standard */
static int get_clear(mem_ptr, mem_size, this_ad)
    int            *mem_size;	       /* how much memory we have
					* available */
    char           *(*mem_ptr);	       /* ptr to a ptr */
    struct ad_struct *this_ad;	       /* general address structure */
{
    static struct ad_struct last_ad;   /* last address */
    char           *my_ptr, last_quote = '\"';
    int             p_len, done, new_size;
    enum
    {
	normal, quoting, spacing, commenting
    }               my_state;

    my_ptr = *mem_ptr;		       /* beginning of allocated memory */
    /* save the present address */
    this_ad->b_ad = last_ad.b_ad;
    this_ad->r_ad = last_ad.r_ad;
    this_ad->offset = last_ad.offset;

    my_state = normal;
    done = 0;
    while (!done)
    {				       /* loop until done */
	p_len = my_ptr - *mem_ptr;
	if (p_len >= *mem_size)
	{			       /* leave room for null */
	    new_size = p_len + 1024;
	    *mem_ptr = (char *) realloc(*mem_ptr, new_size);	/* get more memory */
	    if (!*mem_ptr)
	    {
		s_error ("couldn't allocate enough memory, aborting");
		exit(0);
	    } else
		my_ptr = *mem_ptr + p_len;
	    *mem_size = new_size;
	}
	/* now stick the next byte in, but may end up overriding it */
	if (!(*my_ptr = clear_byte(&last_ad)))
	    return (-1);
	/* now the big decision tree, don't trust C RTL */
	if (my_state == commenting)
	{			       /* middle of comment */
	    if is_comment
		(*my_ptr) my_state = normal;	/* end comment */
	} else if is_comment
	    (*my_ptr)
	{			       /* start comment */
	    my_state = commenting;
	} else if (my_state == quoting)
	{
	    if (*my_ptr == last_quote)
		my_state = normal;
	    ++my_ptr;
	} else if is_quote
	    (*my_ptr)
	{
	    last_quote = *my_ptr;
	    my_state = quoting;
	    ++my_ptr;
	} else if is_term
	    (*my_ptr)
	{			       /* end of command */
	    *(my_ptr + 1) = '\0';      /* for cleanliness */
	    done = 1;
	} else if (is_sep(*my_ptr) || (*my_ptr == ','))
	{
	    if (my_state != spacing)
	    {
		my_state = spacing;
		*my_ptr = sep_char;
		++my_ptr;
	    }
	} else if is_null
	    (*my_ptr)
	{
	    if (my_state == quoting)
		++my_ptr;
	} else if ((*my_ptr >= 'A') && (*my_ptr <= 'Z'))
	{
	    if (my_state != normal)
		my_state = normal;
	    ++my_ptr;		       /* everything fine */
	} else if ((*my_ptr >= 'a') && (*my_ptr <= 'z'))
	{
	    if (my_state != quoting)
		*my_ptr += ('A' - 'a');
	    if (my_state != normal)
		my_state = normal;
	    ++my_ptr;		       /* now everything fine */
	} else if ((*my_ptr == '+') || (*my_ptr == '-') || (*my_ptr == '#')
		   || ((*my_ptr >= '0') && (*my_ptr <= '9'))
	    || (*my_ptr == '(') || (*my_ptr == ')') || (*my_ptr == ',')
		   || (*my_ptr == '.'))
	{
	    if (my_state != normal)
		my_state = normal;
	    ++my_ptr;
	}
    }
    return (my_ptr - *mem_ptr);
}

/* copy in the next string from the clear text file */
/* also step the in pointer forward */
static char    *
cc_str(in_ptr, out_ptr)
    char          **in_ptr, *out_ptr;
{
    char            quote_flag, *start_ptr;
    int             done;
    static char     out_buffer[max_str];

    if (!out_ptr)
	out_ptr = out_buffer;
    start_ptr = out_ptr;
    while (**in_ptr == ' ')
	++(*in_ptr);		       /* skip spaces */
    quote_flag = *((*in_ptr)++);
    if (!is_quote(quote_flag))
    {
	s_error ("expecting a quoted string, got %s",
		(*in_ptr - 1));
	*out_ptr = '\0';
	return (start_ptr);
    }
    done = 0;
    while ((!done) && (**in_ptr))
    {
	if (**in_ptr == quote_flag)
	{
	    if (*(*in_ptr + 1) == quote_flag)
	    {
		*out_ptr++ = quote_flag;
		*in_ptr += 2;
	    } else
	    {
		*out_ptr = '\0';
		++(*in_ptr);
		done = 1;
	    }
	} else
	{
	    *out_ptr++ = *(*in_ptr)++;
	}
    }
    if (!done)
    {
	s_error ("unterminated string constant: %s", *in_ptr);
	*out_ptr = '\0';
	return (NULL);
    }
    while ((**in_ptr) && (**in_ptr == ' '))
	++(*in_ptr);
    return (start_ptr);
}

/* class 0, the delimiter elements */
static
ccgm_class0(el, p_len, dat_ptr)
    char           *dat_ptr;
    enum cgmcls0    el;
    int             p_len;
{

    switch (el)
    {
    case No_Op:
	return (1);
    case B_Mf:
	return (f_b_mf(dat_ptr, p_len, delim[(int) el],
		       ctrl[(int) el]));
    case E_Mf:
	return (f_e_mf(delim[(int) el], ctrl[(int) el]));
    case B_Pic:
	return (f_b_p(dat_ptr, p_len, delim[(int) el],
		      ctrl[(int) el]));
    case B_Pic_Body:
	return (f_b_p_body(delim[(int) el], ctrl[(int) el]));
    case E_Pic:
	return (f_e_pic(delim[(int) el], ctrl[(int) el]));
    default:
	s_error ("illegal CCGM class 0 command");


    }
    return (get_all);		       /* may or may not want to
					* terminate on error */
}
/* class 1, the metafile descriptor elements */
static
ccgm_class1(el, p_len, dat_ptr, df_call)
    char           *dat_ptr;
    int             p_len, df_call;
    enum cgmcls1    el;
{

    switch (el)
    {
    case MfVersion:
	return (rd_mf_version(dat_ptr, p_len,
			      mfdesc[(int) el]));
    case MfDescrip:
	return (rd_mf_descriptor(dat_ptr, p_len,
				 mfdesc[(int) el]));
    case vdcType:
	return (s_vdc_type(dat_ptr, mfdesc[(int) el]));
    case IntPrec:
	return (s_int_prec(dat_ptr, mfdesc[(int) el]));
    case RealPrec:
	return (s_real_prec(dat_ptr, mfdesc[(int) el]));
    case IndexPrec:
	return (s_index_prec(dat_ptr, mfdesc[(int) el]));
    case ColPrec:
	return (s_col_prec(dat_ptr, mfdesc[(int) el]));
    case CIndPrec:
	return (s_cind_prec(dat_ptr, mfdesc[(int) el]));
    case MaxCInd:
	return (s_mcind(dat_ptr, mfdesc[(int) el]));
    case CVExtent:
	return (s_cvextent(dat_ptr, mfdesc[(int) el]));
    case MfElList:
	return (rd_mf_list(dat_ptr, mfdesc[(int) el]));
    case MfDefRep:
	return (s_mf_defs(dat_ptr, p_len,
			  mfdesc[(int) el], df_call));
    case FontList:
	return (do_font_list(dat_ptr, p_len,
			     mfdesc[(int) el]));
    case CharList:
	return (do_char_list(dat_ptr, p_len,
			     mfdesc[(int) el]));
    case CharAnnounce:
	return (do_cannounce(dat_ptr, mfdesc[(int) el]));
    default:
	s_error ("illegal CCGM class 1 command");

    }
    return (get_all);		       /* may or may not want to
					* terminate on error */
}

/* class 2, the picture descriptor elements */
static
ccgm_class2(el, dat_ptr)
    char           *dat_ptr;
    enum cgmcls2    el;
{
    /*
     * the device may not want calls during the metafile defaults
     * replacement
     */

#define maybe2 (((a2 == dflt2) && (dev_info->capability & no_def_calls)) \
		? NULL : pdesc[(int) el])

    switch (el)
    {
    case ScalMode:
	return (s_scalmode(dat_ptr, maybe2));
    case ColSelMode:
	return (s_c_s_mode(dat_ptr, maybe2));
    case LWidSpecMode:
	return (s_lws_mode(dat_ptr, maybe2));
    case MarkSizSpecMode:
	return (s_ms_mode(dat_ptr, maybe2));
    case EdWidSpecMode:
	return (s_ew_mode(dat_ptr, maybe2));
    case vdcExtent:
	return (s_vdc_extent(dat_ptr, maybe2));
    case BackCol:
	return (s_back_col(dat_ptr, maybe2));
    default:
	s_error ("illegal CCGM class 2 command");
    }
    return (get_all);
#undef maybe2
}
/* class 3, the control elements */
static
ccgm_class3(el, dat_ptr)
    char           *dat_ptr;
    enum cgmcls3    el;
{
    /*
     * the device may not want calls during the metafile defaults
     * replacement
     */

#define maybe3 (((a3 == dflt3) && (dev_info->capability & no_def_calls)) \
		? NULL : mfctrl[(int) el])

    switch (el)
    {
    case vdcIntPrec:
	return (s_vdc_i_p(dat_ptr, maybe3));
    case vdcRPrec:
	return (s_vdc_r_p(dat_ptr, maybe3));
    case AuxCol:
	return (s_aux_col(dat_ptr, maybe3));
    case Transp:
	return (s_transp(dat_ptr, maybe3));
    case ClipRect:
	return (s_clip_rec(dat_ptr, maybe3));
    case ClipIndic:
	return (s_clip_ind(dat_ptr, maybe3));
    default:
	s_error ("illegal CCGM class 3 command");
    }
    return (get_all);
#undef maybe3
}
/* class 4, the graphical primitive elements */
static
ccgm_class4(el, p_len, dat_ptr)
    char           *dat_ptr;
    int             p_len;
    enum cgmcls4    el;
{
    /* now go do it */

    switch (el)
    {
    case PolyLine:
	return (do_polyline(dat_ptr, p_len,
			    gprim[(int) el]));
    case Dis_Poly:
	return (do_dis_polyline(dat_ptr, p_len,
				gprim[(int) el]));
    case PolyMarker:
	return (do_polymarker(dat_ptr, p_len,
			      gprim[(int) el]));
    case TeXt:
	return (s_text(dat_ptr, gprim[(int) el]));
    case Rex_Text:
	return (s_rex_text(dat_ptr, gprim[(int) el]));
    case App_Text:
	return (s_app_text(dat_ptr, gprim[(int) el]));
    case Polygon:
	return (do_polygon(dat_ptr, p_len,
			   gprim[(int) el]));
    case Poly_Set:
	return (do_polyset(dat_ptr, p_len,
			   gprim[(int) el]));
    case Cell_Array:
	return (do_cell_array(dat_ptr, p_len,
			      gprim[(int) el]));
    case Gen_D_Prim:
	return (do_g_d_p(dat_ptr, p_len,
			 gprim[(int) el]));
    case RectAngle:
	return (do_rectangle(dat_ptr, gprim[(int) el]));
    case Cgm_Circle:
	return (do_circle(dat_ptr, gprim[(int) el]));
    case Circ_3:
	return (do_c3(dat_ptr, gprim[(int) el]));
    case Circ_3_Close:
	return (do_c3_close(dat_ptr, gprim[(int) el]));
    case Circ_Centre:
	return (do_c_centre(dat_ptr, gprim[(int) el]));
    case Circ_C_Close:
	return (do_c_c_close(dat_ptr, gprim[(int) el]));
    case Ellipse:
	return (do_ellipse(dat_ptr, gprim[(int) el]));
    case Ellip_Arc:
	return (do_ell_arc(dat_ptr, gprim[(int) el]));
    case El_Arc_Close:
	return (do_e_a_close(dat_ptr, gprim[(int) el]));
    default:
	s_error ("illegal CCGM class 4 command");
    }
    return (get_all);
}
/* class 5, the attribute elements */
static
ccgm_class5(el, p_len, dat_ptr)
    char           *dat_ptr;
    int             p_len;
    enum cgmcls5    el;
{
    /*
     * the device may not want calls during the metafile defaults
     * replacement
     */

#define maybe5 (((a5 == dflt5) && (dev_info->capability & no_def_calls)) \
		? NULL : attr[(int) el])

    switch (el)
    {
    case LBIndex:
	return (s_lbindex(dat_ptr, maybe5));
    case LType:
	return (s_l_type(dat_ptr, maybe5));
    case LWidth:
	return (s_l_width(dat_ptr, maybe5));
    case LColour:
	return (s_l_colour(dat_ptr, maybe5));
    case MBIndex:
	return (s_mbindex(dat_ptr, maybe5));
    case MType:
	return (s_mk_type(dat_ptr, maybe5));
    case MSize:
	return (s_mk_size(dat_ptr, maybe5));
    case MColour:
	return (s_mk_colour(dat_ptr, maybe5));
    case TBIndex:
	return (s_tbindex(dat_ptr, maybe5));
    case TFIndex:
	return (s_t_index(dat_ptr, maybe5));
    case TPrec:
	return (s_t_prec(dat_ptr, maybe5));
    case CExpFac:
	return (s_c_exp(dat_ptr, maybe5));
    case CSpace:
	return (s_c_space(dat_ptr, maybe5));
    case TColour:
	return (s_t_colour(dat_ptr, maybe5));
    case CHeight:
	return (s_c_height(dat_ptr, maybe5));
    case COrient:
	return (s_c_orient(dat_ptr, maybe5));
    case TPath:
	return (s_tpath(dat_ptr, maybe5));
    case TAlign:
	return (s_t_align(dat_ptr, maybe5));
    case CSetIndex:
	return (s_csindex(dat_ptr, maybe5));
    case AltCSetIndex:
	return (s_acsindex(dat_ptr, maybe5));
    case FillBIndex:
	return (s_fbindex(dat_ptr, maybe5));
    case IntStyle:
	return (s_interior_style(dat_ptr, maybe5));
    case FillColour:
	return (s_fill_colour(dat_ptr, maybe5));
    case HatchIndex:
	return (s_hindex(dat_ptr, maybe5));
    case PatIndex:
	return (s_pindex(dat_ptr, maybe5));
    case EdBIndex:
	return (s_e_b_index(dat_ptr, maybe5));
    case EType:
	return (s_edge_t(dat_ptr, maybe5));
    case EdWidth:
	return (s_edge_w(dat_ptr, maybe5));
    case EdColour:
	return (s_edge_c(dat_ptr, maybe5));
    case EdVis:
	return (s_edge_v(dat_ptr, maybe5));
    case FillRef:
	return (s_fill_ref(dat_ptr, maybe5));
    case PatTab:
	return (p_tab_entry(dat_ptr, p_len, maybe5));
    case PatSize:
	return (s_pat_size(dat_ptr, maybe5));
    case ColTab:
	return (c_tab_entry(dat_ptr, p_len, maybe5));
    case AspsFlags:
	return (do_aspsflags(dat_ptr, p_len, maybe5));
    default:
	s_error ("illegal CCGM class 5 command");
    }
    return (get_all);
#undef maybe5
}
/* class 6, the escape element */
static
ccgm_class6(el, dat_ptr)
    char           *dat_ptr;
    enum cgmcls6    el;
{

    switch (el)
    {
    case Escape:
	return (do_escape(dat_ptr, escfun[(int) el]));
    default:
	s_error ("illegal CCGM class 6 command");
    }
    return (get_all);
}
/* class 7, the external elements */
/* not really implemented yet */
static
ccgm_class7(el, dat_ptr)
    enum cgmcls7    el;
    char           *dat_ptr;
{

    switch (el)
    {
    case Message:
	return (do_message(dat_ptr, extfun[(int) el]));
    case Ap_Data:
	return (do_apdata(dat_ptr, extfun[(int) el]));
    default:
	s_error ("illegal CCGM class 7 command");
    }
    return (get_all);
}




/* utility, given a string decides whether page_no is desired */
/* help file entry follows: */
static int want_page(page_no, page_string)
    int             page_no;
    char            page_string[];
{
    char           *sptr;
    int             n1, n2;

    if (page_string[0] == '*')
	return (1);		       /* default */

    /* read first number */
    if (0 == sscanf(page_string, "%d", &n1))
	return (0);
    if (page_no == n1)
	return (1);
    sptr = page_string;
    while ((sptr = strpbrk(sptr, ",_")) != NULL)
	switch (*sptr)
	{
	case ',':
	    ++sptr;
	    if (0 == sscanf(sptr, "%d", &n1))
		return (0);
	    if (n1 == page_no)
		return (1);
	    break;

	case '_':
	    ++sptr;
	    if (0 == sscanf(sptr, "%d", &n2))
		return (0);
	    if ((n1 <= page_no) && (page_no <= n2))
		return (1);
	    break;
	default:
	    return (0);
	}

    return (0);			       /* no go */
}


/* parcel out the commands here */
static int do_clear(s_ptr, p_len, this_ad, out_class, out_element)
    unsigned char  *s_ptr;
    int             p_len;
    struct ad_struct *this_ad;	       /* unused here */
    int            *out_class, *out_element;
{
    unsigned char   cmd_name[max_str];
    unsigned int    class, element;
    int             i, j, k, done, cmd_len, df_call;
    /* are we replacing defaults ? */
#define no_defs(cl) if (a2 == dflt2)\
  { s_error ("illegal class cl while replacing defaults");\
     return(2);}

    /* may be running over nulls */
    if (!p_len || !s_ptr || !*s_ptr)
	return (1);

    /* first find the command */
    i = 0;
    while ((i < p_len) && (s_ptr[i] == ' '))
	++i;
    j = 0;
    while ((i < p_len) && (j < (max_str - 1)) &&
	   (((s_ptr[i] >= 'A') && (s_ptr[i] <= 'Z')) ||
	    ((s_ptr[i] >= '0') && (s_ptr[i] <= '9'))))
	cmd_name[j++] = s_ptr[i++];
    cmd_name[j] = '\0';
    cmd_len = i;
    done = 0;
    for (i = 0; (i < 8) && (!done); ++i)
    {
	for (j = 1; (j < cc_size[i]) && (!done); ++j)
	{
	    k = 0;
	    while ((CC_cptr[i][j][k]) && (cmd_name[k]) &&
		   (CC_cptr[i][j][k] == cmd_name[k]))
		++k;
	    done = ((!CC_cptr[i][j][k]) && (!cmd_name[k]) && (k));
	}
    }
    /* may be replacing defaults */
    if (!done)
    {
	if (cc_same((char *) cmd_name, "BEGMFDEFAULTS"))
	{
	    i = 2;
	    j = 13;
	    done = 1;
	    df_call = 0;
	} else if (cc_same((char *) cmd_name, "ENDMFDEFAULTS"))
	{
	    i = 2;
	    j = 13;
	    done = 1;
	    df_call = 1;
	}
    }
    /* still didn't find it ? */
    if (!done)
    {
	s_error ("couldn't find a match for [%s]", (char *) cmd_name);
	return (2);
    }
    *out_class = class = i - 1;
    *out_element = element = j - 1;
    /* are we starting a new page ? */
    if ((class == 0) && ((enum cgmcls0) element == B_Pic))
    {
	++in_page_no;		       /* new page */
	skipping = (!want_page(in_page_no, opt[(int) pages].val.str));
    }
    /* if skipping is on, turn it off at the end of the page */
    if ((skipping) && (class == 0) && ((enum cgmcls0) element == E_Pic))
    {
	skipping = 0;
	return (1);
    }
    /* now if we don't want this page, don't do anything until ended */
    if (skipping)
	return (1);
    /* note trouble if we were to get a B_Pic inside a metafile def rep */

    i = cmd_len;
    while ((i < p_len) && (s_ptr[i] == ' '))
	++i;
    /* now nudge forward */
    s_ptr += i;
    p_len -= i;

    switch (class)
    {
    case 0:
	no_defs(0);
	return (ccgm_class0((enum cgmcls0) element, p_len, (char *) s_ptr));
    case 1:
	return (ccgm_class1((enum cgmcls0) element, p_len, (char *) s_ptr,
	    df_call));
    case 2:
	return (ccgm_class2((enum cgmcls0) element, (char *) s_ptr));
    case 3:
	return (ccgm_class3((enum cgmcls0) element, (char *) s_ptr));
    case 4:
	no_defs(4);
	return (ccgm_class4((enum cgmcls0) element, p_len, (char *) s_ptr));
    case 5:
	return (ccgm_class5((enum cgmcls0) element, p_len, (char *) s_ptr));
    case 6:
	no_defs(6);
	return (ccgm_class6((enum cgmcls0) element, (char *) s_ptr));
    case 7:
	no_defs(7);
	return (ccgm_class7((enum cgmcls0) element, (char *) s_ptr));
    default:
	s_error ("illegal class");
    }

    return (get_all);		       /* may or may not want to
					* terminate on error */
}




/* format of a CGM string is n, nC, may want to step forward the outptr */
static char    *
cgm_str(s_ptr, out_ptr)
    unsigned char  *s_ptr, **out_ptr;
{
    int             i;
    unsigned int    length;
    char           *cptr;
    static int      chars_allocated = 0;
    static char    *my_ptr, *use_ptr;

    length = (int) *s_ptr;
    cptr = (char *) s_ptr;
    if (length == 255)
    {
	length = (s_ptr[1] << 8) + (int) (s_ptr[2]);
	cptr += 2;
    }
    /* use compile-time array, unless not big enougth */
    if (length > max_cgm_str)
    {
	if ((length + 1) > chars_allocated)
	{
	    if (chars_allocated == 0)
		my_ptr = (char *) allocate_mem(length + 1, 0);
	    else
		my_ptr = (char *) realloc(my_ptr, length + 1);
	    if (my_ptr)
		chars_allocated = length + 1;
	    else
	    {			       /* failure, must truncate */
		length = max_cgm_str;
		my_ptr = buffer_str;
		chars_allocated = 0;
	    }
	}
	use_ptr = my_ptr;
    } else
	use_ptr = buffer_str;
    for (i = 0; i < length; ++i)
	use_ptr[i] = *++cptr;
    use_ptr[length] = 0;
    if (out_ptr)
	*out_ptr = (unsigned char *) (cptr + 1);

    return (use_ptr);
}




/* CGM specific integers and reals */
/* grab a CGM signed integer at general precision */
static int
cgm_gint(dat_ptr, precision)
    unsigned char  *dat_ptr;
    int             precision;
{
#define SIGN_MASK (-1 ^ 255)
    int             ret, no_chars;

    ret = (*dat_ptr & 128) ? SIGN_MASK | *dat_ptr : *dat_ptr;

    for (no_chars = precision / byte_size; no_chars > 1; --no_chars)
	ret = (ret << byte_size) | *++dat_ptr;

    return (ret);
}
/* grab a CGM unsigned integer at general precision */
static unsigned int
cgm_guint(dat_ptr, precision)
    unsigned char  *dat_ptr;
    int             precision;
{
    unsigned int    ret, no_chars;
    ret = *dat_ptr;
    for (no_chars = precision / byte_size; no_chars > 1; --no_chars)
	ret = (ret << byte_size) | (*++dat_ptr & 255);

    return (ret);
}




/* grab a CGM signed integer */
static int
cgm_sint(dat_ptr)
    unsigned char  *dat_ptr;
{
    return (cgm_gint(dat_ptr, glbl1->int_prec));
}
/* grab a CGM signed integer at index precision */
static int
cgm_xint(dat_ptr)
    unsigned char  *dat_ptr;
{
    return (cgm_gint(dat_ptr, glbl1->ind_prec));
}
/* grab a CGM colour index */
static unsigned int
cgm_cxint(dat_ptr)
    unsigned char  *dat_ptr;
{
    return (cgm_guint(dat_ptr, glbl1->col_i_prec));
}
/* grab a CGM direct colour */
static unsigned int
cgm_dcint(dat_ptr)
    unsigned char  *dat_ptr;
{
    return (cgm_guint(dat_ptr, glbl1->col_prec));
}
/* grab a CGM signed integer at VDC integer precision */
static int
cgm_vint(dat_ptr)
    unsigned char  *dat_ptr;
{
    return ((int) (pxl_vdc * cgm_gint(dat_ptr, glbl3->vdc_i_prec)));
}
/* grab a CGM signed integer at VDC integer precision */
static int
cgm_newvint(dat_ptr)
    unsigned char  *dat_ptr;
{
    return ((int) (cgm_gint(dat_ptr, glbl3->vdc_i_prec)));
}
/* grab a CGM signed integer at fixed E (16 bit) precision */
static int
cgm_eint(dat_ptr)
    unsigned char  *dat_ptr;
{
    return (cgm_gint(dat_ptr, e_size * byte_size));
}




static double
cgm_fixed(dat_ptr, r_prec)
    unsigned char  *dat_ptr;
    struct r_struct *r_prec;
{
    double          ret, exp_part, fract_part;
    int             cgm_gint();

    exp_part = cgm_gint(dat_ptr, r_prec->exp);
    dat_ptr += glbl1->real_prec.exp / byte_size;
    fract_part = cgm_guint(dat_ptr, r_prec->fract);
    fract_part /= 1 << r_prec->fract;

    ret = exp_part + fract_part;

    return (ret);
}
/* an IEEE floating point */
static double
cgm_float(dat_ptr, r_prec)
    unsigned char  *dat_ptr;
    struct r_struct *r_prec;
{
    int             sign_bit, i, j;
    unsigned int    exponent;
    unsigned long   fract;
    double          ret, dfract;
    unsigned char  *new_ptr;

    sign_bit = (*dat_ptr >> 7) & 1;
    new_ptr = (unsigned char *) dat_ptr;

    switch (r_prec->exp + r_prec->fract)
    {
	/* first 32 bit precision */
    case 32:
	exponent = ((*new_ptr & 127) << 1) +
	    ((*(new_ptr + 1) >> 7) & 1);
	fract = ((*(new_ptr + 1) & 127) << 16) + (*(new_ptr + 2) << 8)
	    + *(new_ptr + 3);
	if (exponent == 255)
	{
	    if (fract == 0)
	    {
		fract = ~((~0) << 23);
		exponent = 254;
	    } else
	    {
		s_error ("undefined IEEE number");
		return (0.0);
	    }
	}
	if (exponent == 0)
	{
	    if (fract == 0)
		ret = 0.0;
	    else
	    {
		ret = fract;
		for (i = 0; i < 149; ++i)
		    ret /= 2.0;
	    }
	} else
	{
	    dfract = (double) fract;
	    for (i = 0; i < 23; ++i)
		dfract /= 2.0;
	    ret = 1.0 + dfract;
	    if (exponent < 127)
		for (i = 0; i < (127 - exponent); ++i)
		    ret /= 2.0;
	    else if (exponent > 127)
		for (i = 0; i < (exponent - 127); ++i)
		    ret *= 2.0;
	}
	break;

	/* now 64 bit precision, may not fit in integer, do with FP */
    case 64:
	exponent = ((*new_ptr & 127) << 4) +
	    ((*(new_ptr + 1) >> 4) & 15);

	dfract = (double) (*(new_ptr + 1) & 15);
	for (j = 2; j < 8; ++j)
	{
	    for (i = 0; i < byte_size; ++i)
		dfract *= 2;
	    dfract += (double) *(new_ptr + j);
	}

	if (exponent == 2047)
	{
	    if (dfract == 0.0)
	    {			       /* want big number */
		dfract = 100000000.0;
		exponent = 2046;
	    } else
	    {
		s_error ("undefined IEEE number");
		return (0.0);
	    }
	}
	if (exponent == 0)
	{
	    if (dfract == 0.0)
		ret = 0.0;
	    else
	    {
		ret = dfract;
		for (i = 0; i < 1074; ++i)
		    ret /= 2.0;
	    }
	} else
	{
	    for (i = 0; i < 52; ++i)
		dfract /= 2.0;
	    ret = 1.0 + dfract;
	    if (exponent < 1023)
		for (i = 0; i < (1023 - exponent); ++i)
		    ret /= 2.0;
	    else if (exponent > 1023)
		for (i = 0; i < (exponent - 1023); ++i)
		    ret *= 2.0;
	}
	break;
    default:
	s_error ("illegal real precisions");
	ret = 0.0;
    }
    return ((sign_bit) ? -ret : ret);
}




/* handle the general real variable */
static double
cgm_real(dat_ptr)
    unsigned char  *dat_ptr;
{
    double          ret;

    switch (glbl1->real_prec.fixed)
    {
    case 0:
	ret = cgm_float(dat_ptr, &glbl1->real_prec);
	break;			       /* floating */
    case 1:
	ret = cgm_fixed(dat_ptr, &glbl1->real_prec);
	break;			       /* fixed */
    default:
	s_error ("illegal real flag");
	ret = 0.0;
    }

    return (ret);
}
/* handle the vdc real variable */
static double
cgm_vreal(dat_ptr)
    unsigned char  *dat_ptr;
{
    double          ret;

    switch (glbl3->vdc_r_prec.fixed)
    {
    case 0:
	ret = cgm_float(dat_ptr, &glbl3->vdc_r_prec);
	break;			       /* floating */
    case 1:
	ret = cgm_fixed(dat_ptr, &glbl3->vdc_r_prec);
	break;			       /* fixed */
    default:
	s_error ("illegal vdc real flag");
	ret = 0.0;
    }

    return (ret);
}
/* get a real vdc and change it to integer */
static int
cgm_vireal(dat_ptr)
    unsigned char  *dat_ptr;
{
    double          get_real;
    get_real = cgm_vreal(dat_ptr);
    /* now convert it */
    return ((int) (get_real * pxl_vdc));
}

/* our general vdc getter */
static int      (*cgm_vdc) () = cgm_vint;	/* default to integer
						 * VDC's */
/* need a macro to get the step size right */
#define vdc_step ((glbl1->vdc_type == vdc_int) ? a3->vdc_i_prec :\
	a3->vdc_r_prec.exp + a3->vdc_r_prec.fract)

/* the setup procedure */
static int cgm_setup(full_oname, new_info, new_opt,
	  gl1, gl2, df2, gl3, df3, gl5, df5,
	  pf0, pf1, pf2, pf3, pf4, pf5, pf6, pf7, pfct, random_file)
    char           *full_oname;
    struct info_struct *new_info;
    struct one_opt *new_opt;
    struct mf_d_struct *gl1;	       /* the class 1 elements */
    struct pic_d_struct *gl2, *df2;    /* the class 2 elements */
    struct control_struct *gl3, *df3;  /* the class 3 elements */
    struct attrib_struct *gl5, *df5;   /* the class 5 elements */
    int             (**pf0) (), (**pf1) (), (**pf2) (), (**pf3) (), (**pf4) (), (**pf5) (),
		(**pf6) (), (**pf7) (), (**pfct) ();	/* the function pointer
							 * arrays */
    int             random_file;       /* can use random access */
{
    /* store globals */
    g_in_name = full_oname;
    dev_info = new_info;
    opt = new_opt;
    glbl1 = gl1;
    glbl2 = gl2;
    dflt2 = df2;
    glbl3 = gl3;
    dflt3 = df3;
    glbl5 = gl5;
    dflt5 = df5;

    delim = pf0;
    mfdesc = pf1;
    pdesc = pf2;
    mfctrl = pf3;
    gprim = pf4;
    attr = pf5;
    escfun = pf6;
    extfun = pf7;
    ctrl = pfct;
    random_input = random_file;
    /* make the active pointers refer to the globals */
    a2 = glbl2;
    a3 = glbl3;
    a5 = glbl5;
    return (1);
}



/* now the routines that do the work, class by class */
/* class 0 routines */
/* function to start a metafile */
static
cgm_f_b_mf(dat_ptr, p_len, dev_func, ctrl_func)
    int             (*ctrl_func) ();
    int             (*dev_func) ();
    unsigned char  *dat_ptr;
    int             p_len;
{
    int             ret = 1;
    char            buffer[2 * max_str + 1], prog_name[max_str];

    static char	    env[80];

    /* indexing stuff */
    index_present = 0;
    index_read = 0;
    p_header.no_pages = 0;
    p_header.first = NULL;
    p_header.last = NULL;

    /* first put in the defaults */
    cosr = 1.0;
    sinr = 0.0;
    xp0 = 0;
    yp0 = 0;
    pxl_vdc = 1.0;
    sx = sy = 1.0;
    pages_done = 0;
    /* set the defaults in case anything happens before the first pic */
    rs_defaults(glbl1, glbl2, glbl3, glbl5, dflt2, dflt3, dflt5, pxl_vdc);

    strcpy(prog_name, "GLI ");
    buffer[0] = '\0';
    if (p_len > 1)
	strcpy(buffer, cgm_str((unsigned char *) dat_ptr, NULL));
    in_page_no = 0;

    if (isatty(1))
        tt_printf("<%s>\n", buffer);

    if (strncmp(buffer,"GKSGRAL", 7) == 0)
	strcpy(env, "GLI_GKS=GKSGRAL");
    else if (strncmp(buffer, "GLI GKS", 7) == 0)
	strcpy(env, "GLI_GKS=GLIGKS");
    else
	strcpy(env, "GLI_GKS=GKGKS");
    putenv(env);

    if (dev_func)
	ret = (*dev_func)
	    (buffer, g_in_name, prog_name);

    if (ctrl_func)
	(*ctrl_func) ();
    return (ret);
}



/* function to end a metafile */
static
cgm_f_e_mf(dev_func, ctrl_func)
    int             (*ctrl_func) ();
    int             (*dev_func) ();
{
    int             ret = 1;

    if (dev_func)
	ret = (*dev_func) (pages_done);
    if (ctrl_func)
	(*ctrl_func) ();
    ret = 0;
    return (ret);			       /* time to stop */
}



/* function to reset all of the defaults before starting picture body */
static
cgm_f_b_p(dat_ptr, p_len, dev_func, ctrl_func)
    int             (*ctrl_func) ();
    int             (*dev_func) ();
    char           *dat_ptr;
    int             p_len;
{
    int             ret = 1;

    if (p_len > 1)
    {
	strncpy(pic_name, cgm_str((unsigned char *) dat_ptr, NULL), max_str);
	pic_name[max_str - 1] = '\0';
    } else
	pic_name[0] = '\0';

    if (dev_func)
	ret = (*dev_func) (pic_name);
    if (ctrl_func)
	(*ctrl_func) ();

    get_smart(0, glbl1, a2, opt, dev_info, &cosr, &sinr, &pxl_vdc,
	  &xoffset, &yoffset, &xp0, &yp0, &xsize, &ysize, &font_check,
	      &font_text, &sx, &sy);

    /* go back to the defaults */
    rs_defaults(glbl1, glbl2, glbl3, glbl5, dflt2, dflt3, dflt5, pxl_vdc);

    return (ret);
}



/*
 * function to get the printer ready to start drawing; implement
 * defaults
 */
static
cgm_f_b_p_body(dev_func, ctrl_func)
    int             (*ctrl_func) ();
    int             (*dev_func) ();
{
    int             ret = 1;


    get_smart(1, glbl1, a2, opt, dev_info, &cosr, &sinr, &pxl_vdc,
	  &xoffset, &yoffset, &xp0, &yp0, &xsize, &ysize, &font_check,
	      &font_text, &sx, &sy);

    if (dev_func)
	ret = (*dev_func) (pic_name, xoffset, yoffset,
			   0.0,
			*(a5->ctab), *(a5->ctab + 1), *(a5->ctab + 2),
	 (use_random && index_read) ? p_header.first->no : in_page_no,
			   xsize, ysize);

    if (ctrl_func)
	(*ctrl_func) ();

    return (ret);
}


#define ncar_size 1440

/* function to position in input file */
static int cgm_goto(in_ad)
    struct ad_struct *in_ad;
{

    /* try quick fix for ncar files */
    int             byte_ad;
    byte_ad = (ncar) ? (in_ad->b_ad / ncar_size) * ncar_size :
	in_ad->b_ad;
    if (fseek(inptr, byte_ad, 0))
    {
	s_error ("illegal fseek");
	perror("cgm_goto");
	return (0);
    }
    bytes_read = byte_ad;	       /* where we've got to */
    buf_ptr = end_buf;		       /* need more bytes */
    return (1);
}


/* function to end a picture */
static
cgm_f_e_pic(dev_func, ctrl_func)
    int             (*ctrl_func) ();
    int             (*dev_func) ();
{
    int             ret = 1;
    ++pages_done;
    if (dev_func)
	ret = (*dev_func) (1);
    if (ctrl_func)
	(*ctrl_func) (1);

    /* now handle random stuff */
    if (use_random && index_read)
    {
	if (p_header.first)
	    p_header.first = p_header.first->next;
	else
	    return (f_e_mf(delim[(int) E_Mf], ctrl[(int) E_Mf]));
	if (p_header.first)
	    cgm_goto(&(p_header.first->ad));
	else
	    return (f_e_mf(delim[(int) E_Mf], ctrl[(int) E_Mf]));
    }
    return (ret);
}



/* now the class1 functions */
/* read the metafile version number (if present) */
static
cgm_rd_mf_version(dat_ptr, p_len, dev_func)
    unsigned char  *dat_ptr;
    int             p_len;
    int             (*dev_func) ();
{
    int             ret = 1, vers_no;
    if (p_len > 0)
	vers_no = cgm_sint(dat_ptr);

    if ((p_len > 0) && dev_func)
	ret = (*dev_func) (vers_no);

    return (ret);
}

/* read the metafile descriptor */
static
cgm_rd_mf_descriptor(dat_ptr, p_len, dev_func)
    unsigned char  *dat_ptr;
    int             p_len;
    int             (*dev_func) ();
{
    int             ret = 1;
    char           *char_ptr;
    if (p_len > 1)
	char_ptr = cgm_str(dat_ptr, NULL);
    else
	char_ptr = NULL;

    if ((p_len > 1) && dev_func)
	ret = (*dev_func) (char_ptr);
    return (ret);
}

/* set the VDC type */
static
cgm_s_vdc_type(dat_ptr, dev_func)
    unsigned char  *dat_ptr;
    int             (*dev_func) ();
{
    int             new_type, ret = 1;

    new_type = cgm_eint(dat_ptr);
    glbl1->vdc_type = (enum vdc_enum) new_type;

    switch (glbl1->vdc_type)
    {
    case vdc_int:
	cgm_vdc = cgm_vint;
	break;
    case vdc_real:
	cgm_vdc = cgm_vireal;
	break;
    default:
	s_error ("illegal vdc_type ");
    }

    if (dev_func)
	ret = (*dev_func) (new_type);
    return (ret);
}



/* set the integer precision */
static
cgm_s_int_prec(dat_ptr, dev_func)
    unsigned char  *dat_ptr;
    int             (*dev_func) ();
{
    int             new_prec, ret = 1;

    new_prec = cgm_sint(dat_ptr);
    glbl1->ind_prec = new_prec;

    if (dev_func)
	ret = (*dev_func) (new_prec);
    return (ret);
}
/* set the real  precision */
static
cgm_s_real_prec(dat_ptr, dev_func)
    unsigned char  *dat_ptr;
    int             (*dev_func) ();
{
    int             step_size, new_fixed, new_exp, new_fract, ret = 1;

    step_size = glbl1->int_prec / byte_size;
    new_fixed = cgm_sint(dat_ptr);
    dat_ptr += step_size;
    new_exp = cgm_sint(dat_ptr);
    dat_ptr += step_size;
    new_fract = cgm_sint(dat_ptr);

    glbl1->real_prec.fixed = new_fixed;
    glbl1->real_prec.exp = new_exp;
    glbl1->real_prec.fract = new_fract;

    if (dev_func)
	ret = (*dev_func) (new_fixed, new_exp, new_fract);
    return (ret);
}



/* set the index precision */
static
cgm_s_index_prec(dat_ptr, dev_func)
    unsigned char  *dat_ptr;
    int             (*dev_func) ();
{
    int             new_prec, ret = 1;

    new_prec = cgm_sint(dat_ptr);
    glbl1->ind_prec = new_prec;
    if (dev_func)
	ret = (*dev_func) (new_prec);
    return (ret);
}
/* set the colour precision */
static
cgm_s_col_prec(dat_ptr, dev_func)
    unsigned char  *dat_ptr;
    int             (*dev_func) ();
{
    int             new_prec, ret = 1;

    new_prec = cgm_sint(dat_ptr);
    glbl1->col_prec = new_prec;
    if (dev_func)
	ret = (*dev_func) (new_prec);
    return (ret);
}
/* set the colour index precision */
static
cgm_s_cind_prec(dat_ptr, dev_func)
    unsigned char  *dat_ptr;
    int             (*dev_func) ();
{
    int             new_prec, ret = 1;

    new_prec = cgm_sint(dat_ptr);
    glbl1->col_i_prec = new_prec;
    if (dev_func)
	ret = (*dev_func) (new_prec);
    return (ret);
}



/* set the colour value extent */
static
cgm_s_cvextent(dat_ptr, dev_func)
    unsigned char  *dat_ptr;
    int             (*dev_func) ();
{
    int             cvmin[3], cvmax[3], ret = 1, step_size, i;

    step_size = (glbl1->col_prec / byte_size);

    for (i = 0; i < 3; ++i)
    {
	cvmin[i] = cgm_dcint(dat_ptr);
	dat_ptr += step_size;
    }
    for (i = 0; i < 3; ++i)
    {
	cvmax[i] = cgm_dcint(dat_ptr);
	dat_ptr += step_size;
    }

    for (i = 0; i < 3; ++i)
    {
	glbl1->c_v_extent.min[i] = cvmin[i];
	glbl1->c_v_extent.max[i] = cvmax[i];
    }

    if (dev_func)
	ret = (*dev_func) (cvmin, cvmax);

    return (ret);
}



static
cgm_do_mcind(new_index)
    int             new_index;
{
    int             i;
    float          *new_d_ptr, *new_g_ptr;

    if (new_index > glbl1->max_c_index)
    {				       /* need to make some new memory */
	new_d_ptr =
	    (float *) allocate_mem(float_size * 3 * (new_index + 1), 1);
	new_g_ptr =
	    (float *) allocate_mem(float_size * 3 * (new_index + 1), 1);
	if ((!new_d_ptr) || (!new_g_ptr))
	    return (0);
	/* move over the old data */
	for (i = 0; i < (glbl1->max_c_index + 1) * 3; ++i)
	{
	    *(new_d_ptr + i) = *(a5->ctab + i);
	    *(new_g_ptr + i) = *(a5->ctab + i);
	}
	/* free up memory */
	free(dflt5->ctab);
	free(glbl5->ctab);
	/* and reassign the pointers */
	dflt5->ctab = new_d_ptr;
	glbl5->ctab = new_g_ptr;
    }
    glbl1->max_c_index = new_index;
    return (1);
}


/* set the maximum colour index */
/*
 * split into two functions as we may have to handle illegal metafiles
 * (DI3000) that add c table entries before increasing
 * glbl1->max_c_index
 */
static
cgm_s_mcind(dat_ptr, dev_func)
    unsigned char  *dat_ptr;
    int             (*dev_func) ();
{
    int             new_index, ret = 1;
    new_index = cgm_cxint(dat_ptr);
    if (!cgm_do_mcind(new_index))
	s_error ("trouble setting max colind");
    if (dev_func)
	ret = (*dev_func) (new_index);
    return (ret);
}


/* read the metafile element list (may be useful) */
static
cgm_rd_mf_list(dat_ptr, dev_func)
    unsigned char  *dat_ptr;
    int             (*dev_func) ();
{
    int             no_pairs, i, ret = 1, *array_ptr = NULL, *cur_ptr;

    no_pairs = cgm_sint(dat_ptr);
    if (no_pairs > 0)
	cur_ptr = array_ptr =
	    (int *) allocate_mem(2 * no_pairs * sizeof(int), 0);

    dat_ptr += glbl1->int_prec / byte_size;
    for (i = 0; i < no_pairs; ++i)
    {
	*cur_ptr++ = cgm_xint(dat_ptr);
	dat_ptr += glbl1->ind_prec / byte_size;
	*cur_ptr++ = cgm_xint(dat_ptr);
	dat_ptr += glbl1->ind_prec / byte_size;
    }
    if (dev_func)
	ret = (*dev_func) (no_pairs, array_ptr);
    if (array_ptr)
	free(array_ptr);
    return (ret);
}


/* read the font list */
static
cgm_do_font_list(dat_ptr, p_len, dev_func)
    unsigned char  *dat_ptr;
    int             p_len;
    int             (*dev_func) ();
{
    int             ret = 1;
    int             no_strings;
    char            font_list[max_fonts][max_str + 1];
    unsigned char  *new_ptr;

    new_ptr = dat_ptr;
    no_strings = 0;
    while ((no_strings < max_fonts) && (new_ptr < dat_ptr + p_len - 3))
    {
	strncpy(font_list[no_strings], cgm_str(new_ptr, &new_ptr),
		max_str);
	font_list[no_strings][max_str] = '\0';	/* for safety */
	++no_strings;
    }
    if (no_strings > max_fonts)
	no_strings = max_fonts;

    if (dev_func)
	ret = (*dev_func) (no_strings, font_list);
    return (ret);
}



/* read the character list (implement fully later) */
static
cgm_do_char_list(dat_ptr, p_len, dev_func)
    unsigned char  *dat_ptr;
    int             p_len;
    int             (*dev_func) ();
{
    int             ret = 1;
    int             no_strings, type_array[max_fonts];
    char            font_list[max_fonts][max_str + 1];
    unsigned char  *new_ptr;

    new_ptr = dat_ptr;
    no_strings = 0;
    while ((no_strings < max_fonts) && (new_ptr < dat_ptr + p_len - 3))
    {
	type_array[no_strings] = cgm_eint(new_ptr);
	new_ptr += e_size;
	strncpy(font_list[no_strings], cgm_str(new_ptr, &new_ptr),
		max_str);
	font_list[no_strings][max_str] = '\0';	/* for safety */
	++no_strings;
    }
    if (dev_func)
	ret = (*dev_func) (no_strings, type_array, font_list);
    return (ret);
}
/* do the character announcer */
static
cgm_do_cannounce(dat_ptr, dev_func)
    unsigned char  *dat_ptr;
    int             (*dev_func) ();
{
    int             ret = 1;
    int             new_announce;

    new_announce = cgm_eint(dat_ptr);
    glbl1->char_c_an = new_announce;
    if (dev_func)
	ret = (*dev_func) (new_announce);
    return (ret);
}



/* now the class2 functions */
/* set the scaling mode */
static
cgm_s_scalmode(dat_ptr, dev_func)
    unsigned char  *dat_ptr;
    int             (*dev_func) ();
{
    int             ret = 1, new_mode;
    float           my_scale = 1;

    new_mode = cgm_eint(dat_ptr);
    dat_ptr += e_size;
    if (new_mode)
	my_scale = cgm_float(dat_ptr, &glbl1->real_prec);

    a2->scale_mode.s_mode = new_mode;
    a2->scale_mode.m_scaling = my_scale;

    if (dev_func)
	ret = (*dev_func) (new_mode, my_scale);

    return (ret);
}

/* set the colour selection mode */
static
cgm_s_c_s_mode(dat_ptr, dev_func)
    unsigned char  *dat_ptr;
    int             (*dev_func) ();
{
    int             new_c_s_mode, ret = 1;
    new_c_s_mode = cgm_eint(dat_ptr);
    a2->c_s_mode = (enum cs_enum) new_c_s_mode;
    if (dev_func)
	ret = (*dev_func) ((enum cs_enum) new_c_s_mode);
    return (ret);
}



/* set the line width specification mode */
static
cgm_s_lws_mode(dat_ptr, dev_func)
    unsigned char  *dat_ptr;
    int             (*dev_func) ();
{
    int             new_lws_mode, ret = 1;
    new_lws_mode = cgm_eint(dat_ptr);
    a2->l_w_s_mode = (enum spec_enum) new_lws_mode;
    if (dev_func)
	ret = (*dev_func) ((enum spec_enum) new_lws_mode);
    return (ret);
}
/* set the marker size specification mode */
static
cgm_s_ms_mode(dat_ptr, dev_func)
    unsigned char  *dat_ptr;
    int             (*dev_func) ();
{
    int             new_mode, ret = 1;
    new_mode = cgm_eint(dat_ptr);
    a2->m_s_s_mode = (enum spec_enum) new_mode;
    if (dev_func)
	ret = (*dev_func) ((enum spec_enum) new_mode);
    return (ret);
}
/* set the edge width specification mode */
static
cgm_s_ew_mode(dat_ptr, dev_func)
    unsigned char  *dat_ptr;
    int             (*dev_func) ();
{
    int             new_mode, ret = 1;
    new_mode = cgm_eint(dat_ptr);
    a2->e_w_s_mode = (enum spec_enum) new_mode;
    if (dev_func)
	ret = (*dev_func) ((enum spec_enum) new_mode);
    return (ret);
}



/* set the VDC extent */
static
cgm_s_vdc_extent(dat_ptr, dev_func)
    unsigned char  *dat_ptr;
    int             (*dev_func) ();
{
    int             new_coords[4], i, ret = 1;
    float           new_real[4];

    switch (glbl1->vdc_type)
    {
    case vdc_int:
	for (i = 0; i < 4; ++i)
	{
	    new_coords[i] = cgm_gint(dat_ptr, glbl3->vdc_i_prec);
	    new_real[i] = new_coords[i] / pxl_vdc;
	    dat_ptr += a3->vdc_i_prec / byte_size;
	}
	break;
    case vdc_real:
	for (i = 0; i < 4; ++i)
	{
	    new_real[i] = cgm_vreal(dat_ptr);
	    new_coords[i] = new_real[i] * pxl_vdc;
	    dat_ptr += (a3->vdc_r_prec.exp + a3->vdc_r_prec.fract)
		/ byte_size;
	}
	break;
    default:
	s_error ("illegal vdc_type ");
    }
    for (i = 0; i < 4; ++i)
    {
	a2->vdc_extent.i[i] = new_coords[i];
	a2->vdc_extent.r[i] = new_real[i];
    }
    if (dev_func)
	ret = (*dev_func) (new_coords, new_real);
    return (ret);
}



/* set the background colour */
static
cgm_s_back_col(dat_ptr, dev_func)
    unsigned char  *dat_ptr;
    int             (*dev_func) ();
{
    int             step_size, ret = 1;
    unsigned int    ir, ig, ib;
    float           r, g, b;

    step_size = (glbl1->col_prec / byte_size);
    ir = cgm_dcint(dat_ptr);
    r = dcr(ir);
    dat_ptr += step_size;
    ig = cgm_dcint(dat_ptr);
    g = dcg(ig);
    dat_ptr += step_size;
    ib = cgm_dcint(dat_ptr);
    b = dcb(ib);
    a2->back_col.red = r;
    a2->back_col.green = g;
    a2->back_col.blue = b;

    /*
     * we have to override the index 0 colour by the background colour
     * (sounds screwy, but I think correct)
     */

    *(a5->ctab) = a2->back_col.red;
    *(a5->ctab + 1) = a2->back_col.green;
    *(a5->ctab + 2) = a2->back_col.blue;

    if (dev_func)
	ret = (*dev_func) (ir, ig, ib);
    return (ret);
}



/* now the class3 functions */
/* set the vdc integer precision */
static
cgm_s_vdc_i_p(dat_ptr, dev_func)
    unsigned char  *dat_ptr;
    int             (*dev_func) ();
{
    int             precision, ret = 1;
    precision = cgm_sint(dat_ptr);
    a3->vdc_i_prec = precision;
    if (dev_func)
	ret = (*dev_func) (precision);
    return (ret);
}
/* set the vdc real precision */
static
cgm_s_vdc_r_p(dat_ptr, dev_func)
    unsigned char  *dat_ptr;
    int             (*dev_func) ();
{
    int             step_size, new_fixed, new_exp, new_fract, ret = 1;

    step_size = glbl1->int_prec / byte_size;
    new_fixed = cgm_sint(dat_ptr);
    dat_ptr += step_size;
    new_exp = cgm_sint(dat_ptr);
    dat_ptr += step_size;
    new_fract = cgm_sint(dat_ptr);

    a3->vdc_r_prec.fixed = new_fixed;
    a3->vdc_r_prec.exp = new_exp;
    a3->vdc_r_prec.fract = new_fract;

    if (dev_func)
	ret = (*dev_func) (new_fixed, new_exp, new_fract);
    return (ret);
}



/* set the auxiliary colour */
static
cgm_s_aux_col(dat_ptr, dev_func)
    unsigned char  *dat_ptr;
    int             (*dev_func) ();
{
    int             ret = 1;
    int             new_index = 0, step_size, ir, ig, ib;
    float           r, g, b, *rptr;

    switch ((int) a2->c_s_mode)
    {
    case (int) i_c_mode:	       /* indexed mode */
	new_index = cgm_cxint(dat_ptr);
	a3->aux_col.ind = new_index;
	rptr = a5->ctab + a3->aux_col.ind * 3;
	r = *rptr;
	g = *++rptr;
	b = *++rptr;
	break;
    case (int) d_c_mode:	       /* direct mode */
	step_size = (glbl1->col_prec / byte_size);
	ir = cgm_dcint(dat_ptr);
	r = dcr(ir);
	dat_ptr += step_size;
	ig = cgm_dcint(dat_ptr);
	g = dcg(ig);
	dat_ptr += step_size;
	ib = cgm_dcint(dat_ptr);
	b = dcb(ib);
	a3->aux_col.red = r;
	a3->aux_col.green = g;
	a3->aux_col.blue = b;
	break;
    default:
	s_error ("illegal colour mode ");
    }
    if (dev_func)
	ret = (*dev_func) (r, g, b, new_index);
    return (ret);
}



/* set the transparency */
static
cgm_s_transp(dat_ptr, dev_func)
    unsigned char  *dat_ptr;
    int             (*dev_func) ();
{
    int             new_trans, ret = 1;
    new_trans = cgm_eint(dat_ptr);
    a3->transparency = (enum bool_enum) new_trans;
    if (dev_func)
	ret = (*dev_func) (a3->transparency);
    return (ret);
}


/* set the clipping rectangle */
static
cgm_s_clip_rec(dat_ptr, dev_func)
    unsigned char  *dat_ptr;
    int             (*dev_func) ();
{
    int             new_coords[4], i;
    float           new_real[4], ret = 1;


    switch (glbl1->vdc_type)
    {
    case vdc_int:
	{
	    for (i = 0; i < 4; ++i)
	    {
		new_coords[i] = cgm_newvint(dat_ptr);
		dat_ptr += (unsigned char) ((a3->vdc_i_prec) / (byte_size));
	    }


	    for (i = 0; i < 4; ++i)
		a3->clip_rect.i[i] = new_coords[i];
	    break;
	}

    case vdc_real:
	{
	    for (i = 0; i < 4; ++i)
	    {
		new_real[i] = cgm_vreal(dat_ptr);
		dat_ptr += (a3->vdc_r_prec.exp + a3->vdc_r_prec.fract)
		    / byte_size;
	    }
	    for (i = 0; i < 4; ++i)
		a3->clip_rect.r[i] = new_real[i];
	    break;
	}

    default:
	s_error ("illegal vdc_type ");

    }

    if (dev_func)
	ret = (*dev_func) (new_coords, new_real);

    return (ret);
}



/* set the clipping indicator */
static
cgm_s_clip_ind(dat_ptr, dev_func)
    unsigned char  *dat_ptr;
    int             (*dev_func) ();
{
    int             new_clip, ret = 1;
    new_clip = cgm_eint(dat_ptr);
    a3->clip_ind = (enum bool_enum) new_clip;
    if (dev_func)
	ret = (*dev_func) (a3->clip_ind);
    return (ret);
}



/* now the class 4 functions */
/* take care of a series of points that need a line between them */
static
cgm_do_polyline(dat_ptr, p_len, dev_func)
    int             (*dev_func) ();
    unsigned char  *dat_ptr;
    int             p_len;
{
    int             ret = 1;
    int             no_pairs, step_size, x, y, i, *x_ptr, *y_ptr, *x1_ptr,
		   *y1_ptr;

    step_size = (vdc_step / byte_size);
    no_pairs = p_len / (step_size * 2);

    /* first get the memory */
    if (no_pairs > wk_ar_size)
    {
	x1_ptr = intalloc(no_pairs * bytes_per_word);
	y1_ptr = intalloc(no_pairs * bytes_per_word);
    } else
    {
	x1_ptr = x_wk_ar;
	y1_ptr = y_wk_ar;
    }
    if ((!x1_ptr) || (!y1_ptr))
	s_error ("trouble with polyline memory");
    x_ptr = x1_ptr;
    y_ptr = y1_ptr;

    /* now grab the data */
    for (i = 0; i < no_pairs; ++i)
    {
	x = (*cgm_vdc) (dat_ptr);
	dat_ptr += step_size;
	y = (*cgm_vdc) (dat_ptr);
	dat_ptr += step_size;
	*x_ptr++ = newx(x, y);
	*y_ptr++ = newy(x, y);
    }

    /* NO emulation function, can't do anything without a polyline */
    if (dev_func)
	ret = (*dev_func) (no_pairs, x1_ptr, y1_ptr);

    if (x1_ptr != x_wk_ar)
	free(x1_ptr);
    if (y1_ptr != y_wk_ar)
	free(y1_ptr);

    return (ret);
}




/*
 * take care of a series of points that need a line between alternate
 * points
 */
static
cgm_do_dis_polyline(dat_ptr, p_len, dev_func)
    int             (*dev_func) ();
    unsigned char  *dat_ptr;
    int             p_len;
{
    int             ret = 1;
    int             no_pairs, step_size, x, y, i, *x_ptr, *y_ptr, *x1_ptr,
		   *y1_ptr;

    step_size = (vdc_step / byte_size);
    no_pairs = p_len / (step_size * 2);

    /* some cell arrays have odd number of points ! */
    if (no_pairs % 2)
	--no_pairs;

    /* first get the memory */
    if (no_pairs > wk_ar_size)
    {
	x1_ptr = intalloc(no_pairs * bytes_per_word);
	y1_ptr = intalloc(no_pairs * bytes_per_word);
    } else
    {
	x1_ptr = x_wk_ar;
	y1_ptr = y_wk_ar;
    }
    if ((!x1_ptr) || (!y1_ptr))
	s_error ("trouble with polyline memory");
    x_ptr = x1_ptr;
    y_ptr = y1_ptr;

    /* now grab the data */
    for (i = 0; i < no_pairs; ++i)
    {
	x = (*cgm_vdc) (dat_ptr);
	dat_ptr += step_size;
	y = (*cgm_vdc) (dat_ptr);
	dat_ptr += step_size;
	*x_ptr++ = newx(x, y);
	*y_ptr++ = newy(x, y);
    }

    /* might emulate */
    ret = (*dev_func) (no_pairs, x1_ptr, y1_ptr);

    if (x1_ptr != x_wk_ar)
	free(x1_ptr);
    if (y1_ptr != y_wk_ar)
	free(y1_ptr);

    return (ret);
}



/* do a series of markers at the specified points */
static
cgm_do_polymarker(dat_ptr, p_len, dev_func)
    unsigned char  *dat_ptr;
    int             p_len;
    int             (*dev_func) ();
{
    int             ret = 1;
    int             no_pairs, step_size, x, y, i, *x_ptr, *y_ptr, *x1_ptr,
		   *y1_ptr;

    step_size = (vdc_step / byte_size);
    no_pairs = p_len / (step_size * 2);

    /* first get the memory */
    if (no_pairs > wk_ar_size)
    {
	x1_ptr = intalloc(no_pairs * bytes_per_word);
	y1_ptr = intalloc(no_pairs * bytes_per_word);
    } else
    {
	x1_ptr = x_wk_ar;
	y1_ptr = y_wk_ar;
    }
    if ((!x1_ptr) || (!y1_ptr))
	s_error ("trouble with polymarker memory");
    x_ptr = x1_ptr;
    y_ptr = y1_ptr;

    /* now grab the data */
    for (i = 0; i < no_pairs; ++i)
    {
	x = (*cgm_vdc) (dat_ptr);
	dat_ptr += step_size;
	y = (*cgm_vdc) (dat_ptr);
	dat_ptr += step_size;
	*x_ptr++ = newx(x, y);
	*y_ptr++ = newy(x, y);
    }
    /* might emulate */
    ret = (*dev_func) (no_pairs, x1_ptr, y1_ptr);

    if (x1_ptr != x_wk_ar)
	free(x1_ptr);
    if (y1_ptr != y_wk_ar)
	free(y1_ptr);

    return (ret);
}



/* set actual text, also take care of raster character descriptions */
static
cgm_s_text(dat_ptr, dev_func)
    unsigned char  *dat_ptr;
    int             (*dev_func) ();
{
    int             step_size, x, y, final, xin, yin, no_chars, device_ok,
		    ret = 1, xdelta, ydelta;
    char            buffer[2 * max_str + 1];

    /* first see if the device can handle it */
    device_ok =
	((a5->t_prec == string) && (dev_info->capability & string_text)) ||
	((a5->t_prec == character) && (dev_info->capability & char_text)) ||
	((a5->t_prec == stroke) && (dev_info->capability & stroke_text)) ||
	(font_check == NULL) || (font_text == NULL);

    /* may be overrriding */
    if (font_check && font_text)
	device_ok = 0;


    /* now ready to procede */
    step_size = (vdc_step / byte_size);

    xin = (*cgm_vdc) (dat_ptr);
    dat_ptr += step_size;
    yin = (*cgm_vdc) (dat_ptr);
    dat_ptr += step_size;
    x = newx(xin, yin);
    y = newy(xin, yin);

    step_size = e_size;
    final = cgm_eint(dat_ptr);
    dat_ptr += step_size;

    strcpy(buffer, cgm_str(dat_ptr, NULL));
    no_chars = strlen(buffer);

    if ((final != 0) && (final != 1))
    {
	s_error ("illegal final in TEXT");
	return (0);
    }
    /* may have to do the positioning adjustment */
    xdelta = ydelta = 0;
    if (!(dev_info->capability & v_center) && (device_ok))
    {
	switch (a5->text_align.ver)
	{
	case top_v:
	    ydelta = -1.1 * dev_info->c_height;
	    break;
	case cap_v:
	    ydelta = -dev_info->c_height;
	    break;
	case half_v:
	    ydelta = -0.5 * dev_info->c_height;
	    break;
	case bottom_v:
	    ydelta = -0.1 * dev_info->c_height;
	    break;
	default:
	    ydelta = 0;
	}
    }
    if (!(dev_info->capability & h_center) && (device_ok))
    {
	switch (a5->text_align.hor)
	{			       /* fix later */
	case center_h:
	    xdelta = -0.5 * no_chars * dev_info->c_width;
	    break;
	case right_h:
	    xdelta = -no_chars * dev_info->c_width;
	    break;
	default:
	    xdelta = 0;
	}
    }
    /* adjust the position */
    if (xdelta || ydelta)
    {
	x += xdelta * cosr - ydelta * sinr;
	y += ydelta * cosr + xdelta * sinr;
    }
    /* now do the actual text */
    if (device_ok)
    {
	ret = (dev_func) ? (*dev_func) (x, y, final, buffer) : 2;
    }
    return (ret);
}




/* restricted text */
static
cgm_s_rex_text(dat_ptr, dev_func)
    unsigned char  *dat_ptr;
    int             (*dev_func) ();
{
    int             ret = 1;
    int             step_size, width, height, xin, yin, x, y;
    enum bool_enum  final;
    char            buffer[2 * max_str + 1];

    step_size = (vdc_step / byte_size);

    width = (*cgm_vdc) (dat_ptr);
    dat_ptr += step_size;
    height = (*cgm_vdc) (dat_ptr);
    dat_ptr += step_size;
    xin = (*cgm_vdc) (dat_ptr);
    dat_ptr += step_size;
    yin = (*cgm_vdc) (dat_ptr);
    dat_ptr += step_size;
    x = newx(xin, yin);
    y = newy(xin, yin);

    step_size = e_size;
    final = (enum bool_enum) cgm_eint(dat_ptr);
    dat_ptr += step_size;

    strcpy(buffer, cgm_str(dat_ptr, NULL));

    if (dev_func)
	ret = (*dev_func) (width, height, x, y, final, buffer);

    return (ret);
}
/* appended text */
static
cgm_s_app_text(dat_ptr, dev_func)
    unsigned char  *dat_ptr;
    int             (*dev_func) ();
{
    int             ret = 1;
    int             step_size;
    enum bool_enum  final;
    char            buffer[2 * max_str + 1];
    step_size = e_size;
    final = (enum bool_enum) cgm_eint(dat_ptr);
    dat_ptr += step_size;

    strcpy(buffer, cgm_str(dat_ptr, NULL));

    if (dev_func)
	ret = (*dev_func) (final, buffer);
    return (ret);
}




/* handle a polygon */
static
cgm_do_polygon(dat_ptr, p_len, dev_func)
    unsigned char  *dat_ptr;
    int             p_len;
    int             (*dev_func) ();
{
    int             ret = 1;
    int             no_pairs, step_size, x, y, i, *x_ptr, *y_ptr, *x1_ptr,
		   *y1_ptr;

    step_size = (vdc_step / byte_size);
    no_pairs = p_len / (step_size * 2);

    /* first get the memory */
    if (no_pairs > wk_ar_size)
    {
	x1_ptr = intalloc(no_pairs * bytes_per_word);
	y1_ptr = intalloc(no_pairs * bytes_per_word);
    } else
    {
	x1_ptr = x_wk_ar;
	y1_ptr = y_wk_ar;
    }
    if ((!x1_ptr) || (!y1_ptr))
	s_error ("trouble with polygon memory");
    x_ptr = x1_ptr;
    y_ptr = y1_ptr;

    /* now grab the data */
    for (i = 0; i < no_pairs; ++i)
    {
	x = (*cgm_vdc) (dat_ptr);
	dat_ptr += step_size;
	y = (*cgm_vdc) (dat_ptr);
	dat_ptr += step_size;
	*x_ptr++ = newx(x, y);
	*y_ptr++ = newy(x, y);
    }

    /* might emulate */
    ret = (*dev_func) (no_pairs, x1_ptr, y1_ptr);

    if (x1_ptr != x_wk_ar)
	free(x1_ptr);
    if (y1_ptr != y_wk_ar)
	free(y1_ptr);

    return (ret);
}



/* do a polyset */
static
cgm_do_polyset(dat_ptr, p_len, dev_func)
    unsigned char  *dat_ptr;
    int             p_len;
    int             (*dev_func) ();
{
    int             ret = 1;
    int             no_pairs, step_size, x, y, i, *x_ptr, *y_ptr, *x1_ptr,
		   *y1_ptr;
    unsigned char  *edge_ptr;

    /* figure out how many pairs */
    no_pairs = p_len * byte_size / (vdc_step * 2 + e_size * byte_size);

    /* first get the memory */
    if (no_pairs > wk_ar_size)
    {
	x1_ptr = intalloc(no_pairs * bytes_per_word);
	y1_ptr = intalloc(no_pairs * bytes_per_word);
    } else
    {
	x1_ptr = x_wk_ar;
	y1_ptr = y_wk_ar;
    }
    edge_ptr = (unsigned char *) allocate_mem(no_pairs, 0);
    if (!edge_ptr)
	return (2);
    if ((!x1_ptr) || (!y1_ptr))
	s_error ("trouble with polyset memory");
    x_ptr = x1_ptr;
    y_ptr = y1_ptr;

    /* now grab the data */
    step_size = (vdc_step / byte_size);
    for (i = 0; i < no_pairs; ++i)
    {
	x = (*cgm_vdc) (dat_ptr);
	dat_ptr += step_size;
	y = (*cgm_vdc) (dat_ptr);
	dat_ptr += step_size;
	*x_ptr++ = newx(x, y);
	*y_ptr++ = newy(x, y);
	edge_ptr[i] = cgm_eint(dat_ptr);
	dat_ptr += e_size;
    }

    /* might emulate */
    ret = (*dev_func) (no_pairs, x1_ptr, y1_ptr, edge_ptr);

    free(edge_ptr);
    if (x1_ptr != x_wk_ar)
	free(x1_ptr);
    if (y1_ptr != y_wk_ar)
	free(y1_ptr);
    return (ret);
}



/* now try to take care of a general cell array */
static
cgm_do_cell_array(dat_ptr, plen, dev_func)
    unsigned char  *dat_ptr;
    int             plen, (*dev_func) ();
{
    int             ret = 1;
    int             p0[2], cp[2];      /* corner p */
    int             q0[2], cq[2];      /* corner q */
    int             r0[2], cr[2];      /* corner r */
    /*
     * this is a parallelogram, diagonal between p and q, first row is
     * p-r
     */
    unsigned int    nx;		       /* columns of data */
    unsigned int    ny;		       /* rows of data */
    int             l_col_prec;	       /* local colour precision */
    int             rep_mode;	       /* cell representation mode */
    int             step_size, i;
    unsigned char  *orig_ptr;	       /* original pointer */
    long int        no_bytes;	       /* number of bytes of data */

    orig_ptr = dat_ptr;
    step_size = (vdc_step / byte_size);
    for (i = 0; i < 2; ++i)
    {
	p0[i] = (*cgm_vdc) (dat_ptr);
	dat_ptr += step_size;
    }
    for (i = 0; i < 2; ++i)
    {
	q0[i] = (*cgm_vdc) (dat_ptr);
	dat_ptr += step_size;
    }
    for (i = 0; i < 2; ++i)
    {
	r0[i] = (*cgm_vdc) (dat_ptr);
	dat_ptr += step_size;
    }

    step_size = (glbl1->int_prec / byte_size);

    nx = cgm_sint(dat_ptr);
    dat_ptr += step_size;

    ny = cgm_sint(dat_ptr);
    dat_ptr += step_size;

    l_col_prec = cgm_sint(dat_ptr);
    dat_ptr += step_size;

    step_size = e_size;
    rep_mode = cgm_eint(dat_ptr);
    dat_ptr += step_size;


    /* get the precion right */
    if (a2->c_s_mode)
    {				       /* direct */
	l_col_prec = (l_col_prec) ? l_col_prec : glbl1->col_prec;
    } else
    {				       /* indexed */
	l_col_prec = (l_col_prec) ? l_col_prec : glbl1->col_i_prec;
    }
    cp[0] = newx(p0[0], p0[1]);
    cp[1] = newy(p0[0], p0[1]);
    cq[0] = newx(q0[0], q0[1]);
    cq[1] = newy(q0[0], q0[1]);
    cr[0] = newx(r0[0], r0[1]);
    cr[1] = newy(r0[0], r0[1]);

    if ((rep_mode > 1) || (rep_mode < 0))
    {
	s_error ("illegal rep_mode ");
	return (2);
    }
    /*
     * just pass them the whole shebang, let them use cell array
     * library
     */

    no_bytes = plen - (dat_ptr - orig_ptr);
    /* might emulate */
    ret = (*dev_func) (cp, cq, cr, nx, ny, l_col_prec,
		       dat_ptr, rep_mode, no_bytes);
    return (ret);
}



/* generalised drawing primitive */
/* format is identifier, no_pairs, list of points, set of strings */
static
cgm_do_g_d_p(dat_ptr, plen, dev_func)
    unsigned char  *dat_ptr;
    int             plen, (*dev_func) ();
{
    int             ret = 1;
    int             gdp_id, no_pairs, *x_ptr, *y_ptr, step_size, i,
		    x, y;
    char           *data_record;

    step_size = (glbl1->int_prec / byte_size);

    gdp_id = cgm_sint(dat_ptr);
    dat_ptr += step_size;

    no_pairs = cgm_sint(dat_ptr);
    dat_ptr += step_size;

    /* first get the memory */
    if (no_pairs > wk_ar_size)
    {
	x_ptr = intalloc(no_pairs * bytes_per_word);
	y_ptr = intalloc(no_pairs * bytes_per_word);
    } else
    {
	x_ptr = x_wk_ar;
	y_ptr = y_wk_ar;
    }
    /* now grab the data */
    step_size = (vdc_step / byte_size);
    for (i = 0; i < no_pairs; ++i)
    {
	x = (*cgm_vdc) (dat_ptr);
	dat_ptr += step_size;
	y = (*cgm_vdc) (dat_ptr);
	dat_ptr += step_size;
	x_ptr[i] = newx(x, y);
	y_ptr[i] = newy(x, y);
    }
    data_record = cgm_str(dat_ptr, NULL);

    /* might emulate */
    ret = (*dev_func) (gdp_id, no_pairs, x_ptr, y_ptr, data_record);

    if (x_ptr != x_wk_ar)
	free(x_ptr);
    if (y_ptr != y_wk_ar)
	free(y_ptr);

    return (ret);
}



/* do a rectangle */
static
cgm_do_rectangle(dat_ptr, dev_func)
    unsigned char  *dat_ptr;
    int             (*dev_func) ();
{
    int             ret = 1;
    int             x1, y1, x2, y2, step_size, x_1, y_1, x_2, y_2;
    step_size = (vdc_step / byte_size);
    x1 = (*cgm_vdc) (dat_ptr);
    dat_ptr += step_size;
    y1 = (*cgm_vdc) (dat_ptr);
    dat_ptr += step_size;
    x2 = (*cgm_vdc) (dat_ptr);
    dat_ptr += step_size;
    y2 = (*cgm_vdc) (dat_ptr);
    x_1 = newx(x1, y1);
    y_1 = newy(x1, y1);
    x_2 = newx(x2, y2);
    y_2 = newy(x2, y2);


    /* might emulate */
    ret = (*dev_func) (x_1, y_1, x_2, y_2);

    return (ret);
}



/* set a circle */
static
cgm_do_circle(dat_ptr, dev_func)
    unsigned char  *dat_ptr;
    int             (*dev_func) ();
{
    int             ret = 1, step_size, x, y, r, x1, y1;
    int             em_circle();       /* emul.c */

    step_size = (vdc_step / byte_size);
    x1 = (*cgm_vdc) (dat_ptr);
    dat_ptr += step_size;
    y1 = (*cgm_vdc) (dat_ptr);
    dat_ptr += step_size;
    r = (*cgm_vdc) (dat_ptr);
    x = newx(x1, y1);
    y = newy(x1, y1);
    /* might emulate */
    ret = (dev_func) ? (*dev_func) (x, y, r) : em_circle(x, y, r);

    return (ret);
}



/* set an arc, get the positions of 1st pt, intermdiate pt, end pt */
static
cgm_do_c3(dat_ptr, dev_func)
    unsigned char  *dat_ptr;
    int             (*dev_func) ();
{
    int             ret = 1, step_size, x_array[6], i, first_array[6];
    int             em_c3();	       /* in emul.c */

    step_size = (vdc_step / byte_size);
    for (i = 0; i < 6; ++i)
    {
	first_array[i] = (*cgm_vdc) (dat_ptr);
	dat_ptr += step_size;
    }
    for (i = 0; i < 3; ++i)
    {
	x_array[2 * i] = newx(first_array[2 * i], first_array[2 * i + 1]);
	x_array[2 * i + 1] = newy(first_array[2 * i], first_array[2 * i + 1]);
    }
    ret = (dev_func) ? (*dev_func) (x_array) : em_c3(x_array);
    return (ret);
}



/*
 * set a closed arc, get the positions of 1st pt, intermdiate pt, end
 * pt
 */
static
cgm_do_c3_close(dat_ptr, dev_func)
    unsigned char  *dat_ptr;
    int             (*dev_func) ();
{
    int             ret = 1, step_size, x_array[6], i, first_array[6];
    enum bool_enum  chord;
    int             em_c3_close();     /* in emul.c */
    step_size = (vdc_step / byte_size);
    for (i = 0; i < 6; ++i)
    {
	first_array[i] = (*cgm_vdc) (dat_ptr);
	dat_ptr += step_size;
    }
    for (i = 0; i < 3; ++i)
    {
	x_array[2 * i] = newx(first_array[2 * i], first_array[2 * i + 1]);
	x_array[2 * i + 1] = newy(first_array[2 * i], first_array[2 * i + 1]);
    }
    chord = (enum bool_enum) cgm_eint(dat_ptr);
    ret = (dev_func) ? (*dev_func) (x_array, chord) :
	em_c3_close(x_array, chord);
    return (ret);
}



/* set an arc, ends specified by vectors */
static
cgm_do_c_centre(dat_ptr, dev_func)
    unsigned char  *dat_ptr;
    int             (*dev_func) ();
{
    int             ret = 1, step_size, vec_array[4], i, x, y, r, x1,
		    y1;
    int             em_c_centre();     /* in emul.c */

    step_size = (vdc_step / byte_size);
    x1 = (*cgm_vdc) (dat_ptr);
    dat_ptr += step_size;
    y1 = (*cgm_vdc) (dat_ptr);
    dat_ptr += step_size;
    for (i = 0; i < 4; ++i)
    {
	vec_array[i] = (*cgm_vdc) (dat_ptr);
	dat_ptr += step_size;
    }
    r = (*cgm_vdc) (dat_ptr);
    x = newx(x1, y1);
    y = newy(x1, y1);

    /* might emulate */
    ret = (dev_func) ? (*dev_func) (x, y, vec_array, r) :
	em_c_centre(x, y, vec_array, r);
    return (ret);
}
/* set an arc, ends specified by vectors, close it */
static
cgm_do_c_c_close(dat_ptr, dev_func)
    unsigned char  *dat_ptr;
    int             (*dev_func) ();
{
    int             ret = 1, step_size, vec_array[4], i, x, y, r, x1,
		    y1;
    enum bool_enum  chord;
    int             em_c_c_close();    /* in emul.c */
    step_size = (vdc_step / byte_size);
    x1 = (*cgm_vdc) (dat_ptr);
    dat_ptr += step_size;
    y1 = (*cgm_vdc) (dat_ptr);
    dat_ptr += step_size;
    for (i = 0; i < 4; ++i)
    {
	vec_array[i] = (*cgm_vdc) (dat_ptr);
	dat_ptr += step_size;
    }
    r = (*cgm_vdc) (dat_ptr);
    x = newx(x1, y1);
    y = newy(x1, y1);

    dat_ptr += step_size;
    chord = (enum bool_enum) cgm_eint(dat_ptr);
    /* might emulate */
    ret = (dev_func) ? (*dev_func) (x, y, vec_array, r, chord) :
	em_c_c_close(x, y, vec_array, r, chord);
    return (ret);
}



/*
 * set an ellipse, specify centre, two conjugate diameters (see the
 * book !)
 */
static
cgm_do_ellipse(dat_ptr, dev_func)
    unsigned char  *dat_ptr;
    int             (*dev_func) ();
{
    int             ret = 1, step_size, pt_array[6], i;

    step_size = (vdc_step / byte_size);
    for (i = 0; i < 6; ++i)
    {
	pt_array[i] = (*cgm_vdc) (dat_ptr);
	dat_ptr += step_size;
    }

    /* might emulate */
    ret = (dev_func) ? (*dev_func) (pt_array) :
	em_ellipse(pt_array);
    return (ret);
}
/*
 * set an elliptical arc, specify centre two conjugate diameters end
 * vectors
 */
static
cgm_do_ell_arc(dat_ptr, dev_func)
    unsigned char  *dat_ptr;
    int             (*dev_func) ();
{
    int             ret = 1, step_size, pt_array[6], vec_array[4], i;
    int             em_all_arc();      /* in emul.c */

    step_size = (vdc_step / byte_size);
    for (i = 0; i < 6; ++i)
    {
	pt_array[i] = (*cgm_vdc) (dat_ptr);
	dat_ptr += step_size;
    }
    for (i = 0; i < 4; ++i)
    {
	vec_array[i] = (*cgm_vdc) (dat_ptr);
	dat_ptr += step_size;
    }
    /* might emulate */
    ret = (dev_func) ? (*dev_func) (pt_array, vec_array) :
	em_ell_arc(pt_array, vec_array);

    return (ret);
}



/* set an elliptical arc, close it */
static
cgm_do_e_a_close(dat_ptr, dev_func)
    unsigned char  *dat_ptr;
    int             (*dev_func) ();
{
    int             ret = 1, step_size, pt_array[6], vec_array[4], i;
    enum bool_enum  chord;

    step_size = (vdc_step / byte_size);
    for (i = 0; i < 6; ++i)
    {
	pt_array[i] = (*cgm_vdc) (dat_ptr);
	dat_ptr += step_size;
    }
    for (i = 0; i < 4; ++i)
    {
	vec_array[i] = (*cgm_vdc) (dat_ptr);
	dat_ptr += step_size;
    }
    chord = (enum bool_enum) cgm_eint(dat_ptr);

    /* might emulate */
    ret = (dev_func) ? (*dev_func) (pt_array, vec_array, chord) :
	em_e_a_close(pt_array, vec_array, chord);

    return (ret);
}

/* now the class5 functions */
/* set the line bundle index */
static
cgm_s_lbindex(dat_ptr, dev_func)
    int             (*dev_func) ();
    unsigned char  *dat_ptr;
{
    int             ret = 1;
    int             new_lbindex;
    new_lbindex = cgm_xint(dat_ptr);
    a5->l_b_index = new_lbindex;

    if (dev_func)
	ret = (*dev_func) (a5->l_b_index);

    return (ret);
}
/* set the line type */
static
cgm_s_l_type(dat_ptr, dev_func)
    int             (*dev_func) ();
    unsigned char  *dat_ptr;
{
    int             ret = 1;
    int             new_l_type;
    new_l_type = cgm_xint(dat_ptr);
    a5->line_type = (int) new_l_type;

    if (dev_func)
	ret = (*dev_func) (a5->line_type);

    return (ret);
}



/* set the line width */
static
cgm_s_l_width(dat_ptr, dev_func)
    int             (*dev_func) ();
    unsigned char  *dat_ptr;
{
    int             ret = 1;
    float           rmul;
    int             new_l_width;
    float           dummy;

    if (!dev_info->d_l_width)
	dev_info->d_l_width = 1;       /* safety */

    if (a2->l_w_s_mode == absolute)
    {
	new_l_width = (*cgm_vdc) (dat_ptr) + 0.5;
	a5->line_width.i = new_l_width;
    } else if (a2->l_w_s_mode == scaled)
    {
	rmul = cgm_real(dat_ptr);
	a5->line_width.r = rmul;
	a5->line_width.i = (int) (rmul * dev_info->d_l_width);
    } else
	s_error ("illegal line spec mode ");

    dummy = ((float) a5->line_width.i) / dev_info->d_l_width;

    if (dev_func)
	ret = (*dev_func)
	    (a5->line_width.i, dummy);

    return (ret);
}



/* set the line colour */
static
cgm_s_l_colour(dat_ptr, dev_func)
    int             (*dev_func) ();
    unsigned char  *dat_ptr;
{
    int             ret = 1;
    int             new_index = -1, step_size, ir, ig, ib;
    float           r, g, b, *rptr;
    switch ((int) a2->c_s_mode)
    {
    case (int) i_c_mode:	       /* indexed mode */
	new_index = cgm_cxint(dat_ptr);
	a5->line_colour.ind = new_index;
	rptr = a5->ctab + a5->line_colour.ind * 3;
	r = *rptr;
	g = *++rptr;
	b = *++rptr;
	break;
    case (int) d_c_mode:	       /* direct mode */
	step_size = (glbl1->col_prec / byte_size);
	ir = cgm_dcint(dat_ptr);
	r = dcr(ir);
	dat_ptr += step_size;
	ig = cgm_dcint(dat_ptr);
	g = dcg(ig);
	dat_ptr += step_size;
	ib = cgm_dcint(dat_ptr);
	b = dcb(ib);
	a5->line_colour.red = r;
	a5->line_colour.green = g;
	a5->line_colour.blue = b;
	break;
    default:
	s_error ("illegal colour mode ");
    }
    if (dev_func)
	ret = (*dev_func) (r, g, b, new_index);
    return (ret);
}



/* set the marker bundle index */
static
cgm_s_mbindex(dat_ptr, dev_func)
    int             (*dev_func) ();
    unsigned char  *dat_ptr;
{
    int             ret = 1;
    int             new_index;
    new_index = cgm_xint(dat_ptr);
    a5->mk_b_index = new_index;

    if (dev_func)
	ret = (*dev_func) (a5->mk_b_index);

    return (ret);
}
/* set the marker type */
static
cgm_s_mk_type(dat_ptr, dev_func)
    int             (*dev_func) ();
    unsigned char  *dat_ptr;
{
    int             ret = 1;
    int             new_mk_type;
    new_mk_type = cgm_xint(dat_ptr);
    a5->mk_type = new_mk_type;
    if (dev_func)
	ret = (*dev_func) (a5->mk_type);
    return (ret);
}



/* set the marker size */
static
cgm_s_mk_size(dat_ptr, dev_func)
    int             (*dev_func) ();
    unsigned char  *dat_ptr;
{
    int             ret = 1;
    float           rmul;
    int             new_mk_size;

    if (!dev_info->d_m_size)
	dev_info->d_m_size = 1;	       /* safety */

    if (a2->m_s_s_mode == absolute)
    {
	new_mk_size = (*cgm_vdc) (dat_ptr);
	a5->mk_size.i = new_mk_size;
    } else if (a2->m_s_s_mode == scaled)
    {
	rmul = cgm_real(dat_ptr);
	a5->mk_size.r = rmul;
	a5->mk_size.i = rmul * dev_info->d_m_size;
    } else
	s_error ("illegal marker size mode ");

    if (dev_func)
    {
	float           dummy;

	dummy = (float) a5->mk_size.i / dev_info->d_m_size;
	ret = (*dev_func) (a5->mk_size.i, dummy);
    }
    return (ret);
}



/* set the marker colour */
static
cgm_s_mk_colour(dat_ptr, dev_func)
    int             (*dev_func) ();
    unsigned char  *dat_ptr;
{
    int             ret = 1;
    int             new_index = -1, step_size, ir, ig, ib;
    float           r, g, b, *rptr;


    switch ((int) a2->c_s_mode)
    {
    case (int) i_c_mode:	       /* indexed mode */
	new_index = cgm_cxint(dat_ptr);
	a5->mk_colour.ind = new_index;
	rptr = a5->ctab + a5->mk_colour.ind * 3;
	r = *rptr;
	g = *++rptr;
	b = *++rptr;
	break;
    case (int) d_c_mode:	       /* direct mode */
	step_size = (glbl1->col_prec / byte_size);
	ir = cgm_dcint(dat_ptr);
	r = dcr(ir);
	dat_ptr += step_size;
	ig = cgm_dcint(dat_ptr);
	g = dcg(ig);
	dat_ptr += step_size;
	ib = cgm_dcint(dat_ptr);
	b = dcb(ib);
	a5->mk_colour.red = r;
	a5->mk_colour.green = g;
	a5->mk_colour.blue = b;
	break;
    default:
	s_error ("illegal colour mode ");
    }
    if (dev_func)
	ret = (*dev_func) (r, g, b, new_index);
    return (ret);
}



/* set the text bundle index */
static
cgm_s_tbindex(dat_ptr, dev_func)
    int             (*dev_func) ();
    unsigned char  *dat_ptr;
{
    int             ret = 1;
    int             new_index;
    new_index = cgm_xint(dat_ptr);
    a5->t_b_index = new_index;

    if (dev_func)
	ret = (*dev_func) (a5->t_b_index);

    return (ret);
}
/* set the text font index */
static
cgm_s_t_index(dat_ptr, dev_func)
    int             (*dev_func) ();
    unsigned char  *dat_ptr;
{
    int             ret = 1;
    int             new_font_index;
    new_font_index = cgm_xint(dat_ptr);
    a5->t_f_index = new_font_index;
    if (dev_func)
	ret = (*dev_func) (new_font_index);
    return (ret);
}
/* set the text precision */
static
cgm_s_t_prec(dat_ptr, dev_func)
    int             (*dev_func) ();
    unsigned char  *dat_ptr;
{
    int             ret = 1;
    int             new_t_prec;
    new_t_prec = cgm_eint(dat_ptr);
    a5->t_prec = (enum txt_enum) new_t_prec;
    if (dev_func)
	ret = (*dev_func) (a5->t_prec);
    return (ret);
}



/* set the character expansion factor */
static
cgm_s_c_exp(dat_ptr, dev_func)
    int             (*dev_func) ();
    unsigned char  *dat_ptr;
{
    int             ret = 1;
    float           new_c_exp_fac;
    new_c_exp_fac = cgm_real(dat_ptr);
    a5->c_exp_fac = new_c_exp_fac;
    if (dev_func)
	ret = (*dev_func) (new_c_exp_fac);
    return (ret);
}
/* set the character space */
static
cgm_s_c_space(dat_ptr, dev_func)
    int             (*dev_func) ();
    unsigned char  *dat_ptr;
{
    int             ret = 1;
    float           new_c_space;
    new_c_space = cgm_real(dat_ptr);
    a5->c_space = new_c_space;
    if (dev_func)
	ret = (*dev_func) (a5->c_space);
    return (ret);
}



/* set the text colour */
static
cgm_s_t_colour(dat_ptr, dev_func)
    int             (*dev_func) ();
    unsigned char  *dat_ptr;
{
    int             ret = 1;
    int             new_index = -1, step_size, ir, ig, ib;
    float           r, g, b, *rptr;

    switch ((int) a2->c_s_mode)
    {
    case 0:			       /* indexed mode */
	new_index = cgm_cxint(dat_ptr);
	a5->text_colour.ind = new_index;
	rptr = a5->ctab + a5->text_colour.ind * 3;
	r = *rptr;
	g = *++rptr;
	b = *++rptr;
	break;
    case 1:			       /* direct mode */
	step_size = (glbl1->col_prec / byte_size);
	ir = cgm_dcint(dat_ptr);
	r = dcr(ir);
	dat_ptr += step_size;
	ig = cgm_dcint(dat_ptr);
	g = dcg(ig);
	dat_ptr += step_size;
	ib = cgm_dcint(dat_ptr);
	b = dcb(ib);
	a5->text_colour.red = r;
	a5->text_colour.green = g;
	a5->text_colour.blue = b;
	break;
    default:
	s_error ("illegal colour mode ");
    }

    if (dev_func)
	ret = (*dev_func) (r, g, b, new_index);

    return (ret);
}



/* set character height */
static
cgm_s_c_height(dat_ptr, dev_func)
    int             (*dev_func) ();
    unsigned char  *dat_ptr;
{
    int             ret = 1;

    a5->c_height = (*cgm_vdc) (dat_ptr) + 0.5;
    if (dev_func)
	ret = (*dev_func) (a5->c_height);
    return (ret);
}



/* set the character orientation structure */
static
cgm_s_c_orient(dat_ptr, dev_func)
    int             (*dev_func) ();
    unsigned char  *dat_ptr;
{
    int             ret = 1;
    struct orient_struct new_orient;
    int             step_size;

    step_size = (vdc_step / byte_size);

    new_orient.x_up = (*cgm_vdc) (dat_ptr);
    dat_ptr += step_size;
    new_orient.y_up = (*cgm_vdc) (dat_ptr);
    dat_ptr += step_size;
    new_orient.x_base = (*cgm_vdc) (dat_ptr);
    dat_ptr += step_size;
    new_orient.y_base = (*cgm_vdc) (dat_ptr);
    dat_ptr += step_size;

    a5->c_orient.x_up = new_orient.x_up * sy;
    a5->c_orient.y_up = new_orient.y_up * sy;
    a5->c_orient.x_base = new_orient.x_base * sx;
    a5->c_orient.y_base = new_orient.y_base * sx;
    if (dev_func)
	ret = (*dev_func) (new_orient.x_up,
	       new_orient.y_up, new_orient.x_base, new_orient.y_base);
    return (ret);
}



/* set the text path */
static
cgm_s_tpath(dat_ptr, dev_func)
    int             (*dev_func) ();
    unsigned char  *dat_ptr;
{
    int             ret = 1;
    enum path_enum  new_path;

    new_path = (enum path_enum) cgm_eint(dat_ptr);
    a5->text_path = new_path;
    if (dev_func)
	ret = (*dev_func) (new_path);
    return (ret);
}



/* set the text alignment */
static
cgm_s_t_align(dat_ptr, dev_func)
    int             (*dev_func) ();
    unsigned char  *dat_ptr;
{
    int             ret = 1;
    int             step_size;
    struct align_struct new_align;

    step_size = e_size;
    new_align.hor = (enum hor_align) cgm_eint(dat_ptr);
    dat_ptr += step_size;

    new_align.ver = (enum ver_align) cgm_eint(dat_ptr);
    dat_ptr += step_size;

    step_size = (glbl1->real_prec.exp + glbl1->real_prec.fract)
	/ byte_size;
    new_align.cont_hor = cgm_real(dat_ptr);
    dat_ptr += step_size;
    new_align.cont_ver = cgm_real(dat_ptr);
    dat_ptr += step_size;

    a5->text_align.hor = new_align.hor;
    a5->text_align.ver = new_align.ver;
    a5->text_align.cont_hor = new_align.cont_hor;
    a5->text_align.cont_ver = new_align.cont_ver;

    if (dev_func)
	ret = (*dev_func) (a5->text_align.hor,
			   a5->text_align.ver, a5->text_align.cont_hor,
			   a5->text_align.cont_ver);

    return (ret);
}



/* set the character set index */
static
cgm_s_csindex(dat_ptr, dev_func)
    int             (*dev_func) ();
    unsigned char  *dat_ptr;
{
    int             ret = 1, new_index;

    new_index = cgm_xint(dat_ptr);
    a5->c_set_index = new_index;
    if (dev_func)
	ret = (*dev_func) (new_index);
    return (ret);
}
/* set the alternate character set index */
static
cgm_s_acsindex(dat_ptr, dev_func)
    int             (*dev_func) ();
    unsigned char  *dat_ptr;
{
    int             ret = 1, new_index;

    new_index = cgm_xint(dat_ptr);
    a5->a_c_set_index = new_index;
    if (dev_func)
	ret = (*dev_func) (new_index);
    return (ret);
}
/* set the fill bundle index */
static
cgm_s_fbindex(dat_ptr, dev_func)
    int             (*dev_func) ();
    unsigned char  *dat_ptr;
{
    int             ret = 1, new_index;

    new_index = cgm_xint(dat_ptr);
    a5->f_b_index = new_index;
    if (dev_func)
	ret = (*dev_func) (new_index);
    return (ret);
}



/* set the interior style */
static
cgm_s_interior_style(dat_ptr, dev_func)
    int             (*dev_func) ();
    unsigned char  *dat_ptr;
{
    int             ret = 1;
    int             new_style;
    new_style = cgm_eint(dat_ptr);
    a5->int_style = (enum is_enum) new_style;
    if (dev_func)
	ret = (*dev_func) (a5->int_style);
    return (ret);
}



/* set the fill colour */
static
cgm_s_fill_colour(dat_ptr, dev_func)
    int             (*dev_func) ();
    unsigned char  *dat_ptr;
{
    int             ret = 1;
    int             new_index = -1, step_size, ir, ig, ib;
    float           r, g, b, *rptr;


    switch ((int) a2->c_s_mode)
    {
    case (int) i_c_mode:	       /* indexed mode */
	new_index = cgm_cxint(dat_ptr);
	a5->fill_colour.ind = new_index;
	rptr = a5->ctab + a5->fill_colour.ind * 3;
	r = *rptr;
	g = *++rptr;
	b = *++rptr;
	break;
    case (int) d_c_mode:	       /* direct mode */
	step_size = (glbl1->col_prec / byte_size);
	ir = cgm_dcint(dat_ptr);
	r = dcr(ir);
	dat_ptr += step_size;
	ig = cgm_dcint(dat_ptr);
	g = dcg(ig);
	dat_ptr += step_size;
	ib = cgm_dcint(dat_ptr);
	b = dcb(ib);
	a5->fill_colour.red = r;
	a5->fill_colour.green = g;
	a5->fill_colour.blue = b;
	break;
    default:
	s_error ("illegal colour mode ");
    }

    if (dev_func)
	ret = (*dev_func) (r, g, b, new_index);
    return (ret);
}



/* set the hatch index */
static
cgm_s_hindex(dat_ptr, dev_func)
    int             (*dev_func) ();
    unsigned char  *dat_ptr;
{
    int             ret = 1, new_index;

    new_index = cgm_xint(dat_ptr);
    a5->hatch_index = new_index;
    if (dev_func)
	ret = (*dev_func) (new_index);
    return (ret);
}
/* set the pattern index */
static
cgm_s_pindex(dat_ptr, dev_func)
    int             (*dev_func) ();
    unsigned char  *dat_ptr;
{
    int             ret = 1, new_index;

    new_index = cgm_xint(dat_ptr);
    a5->pat_index = new_index;
    if (dev_func)
	ret = (*dev_func) (new_index);
    return (ret);
}



/* set the edge bundle index */
static
cgm_s_e_b_index(dat_ptr, dev_func)
    int             (*dev_func) ();
    unsigned char  *dat_ptr;
{
    int             ret = 1, new_index;

    new_index = cgm_xint(dat_ptr);
    a5->e_b_index = new_index;
    if (dev_func)
	ret = (*dev_func) (new_index);
    return (ret);
}

/* set the edge type */
static
cgm_s_edge_t(dat_ptr, dev_func)
    int             (*dev_func) ();
    unsigned char  *dat_ptr;
{
    int             ret = 1;
    int             new_l_type;
    new_l_type = cgm_sint(dat_ptr);
    a5->edge_type = (int) new_l_type;

    if (dev_func)
	ret = (*dev_func) (a5->edge_type);

    return (ret);
}




/* set the edge width */
static
cgm_s_edge_w(dat_ptr, dev_func)
    int             (*dev_func) ();
    unsigned char  *dat_ptr;
{
    int             ret = 1;

    float           rmul;
    int             new_e_width;

    if (!dev_info->d_e_width)
	dev_info->d_e_width = 1;       /* safety */

    if (a2->e_w_s_mode == absolute)
    {
	new_e_width = (*cgm_vdc) (dat_ptr) + 0.5;
	a5->edge_width.i = new_e_width;
    } else if (a2->e_w_s_mode == scaled)
    {
	rmul = cgm_real(dat_ptr);
	a5->edge_width.r = rmul;
	a5->edge_width.i = rmul * dev_info->d_e_width;
    } else
	s_error ("illegal edge spec mode ");

    if (dev_func)
    {
	float           dummy;

	dummy = (float) a5->edge_width.i / dev_info->d_e_width;
	ret = (*dev_func) (a5->edge_width.i, dummy);
    }
    return (ret);
}



/* set the edge colour */
static
cgm_s_edge_c(dat_ptr, dev_func)
    int             (*dev_func) ();
    unsigned char  *dat_ptr;
{
    int             ret = 1;
    int             new_index = -1, step_size, ir, ig, ib;
    float           r, g, b, *rptr;
    switch ((int) a2->c_s_mode)
    {
    case (int) i_c_mode:	       /* indexed mode */
	new_index = cgm_cxint(dat_ptr);
	a5->edge_colour.ind = new_index;
	rptr = a5->ctab + a5->line_colour.ind * 3;
	r = *rptr;
	g = *++rptr;
	b = *++rptr;
	break;
    case (int) d_c_mode:	       /* direct mode */
	step_size = (glbl1->col_prec / byte_size);
	ir = cgm_dcint(dat_ptr);
	r = dcr(ir);
	dat_ptr += step_size;
	ig = cgm_dcint(dat_ptr);
	g = dcg(ig);
	dat_ptr += step_size;
	ib = cgm_dcint(dat_ptr);
	b = dcb(ib);
	a5->edge_colour.red = r;
	a5->edge_colour.green = g;
	a5->edge_colour.blue = b;
	break;
    default:
	s_error ("illegal colour mode ");
    }
    if (dev_func)
	ret = (*dev_func) (r, g, b, new_index);
    return (ret);
}



/* set the edge visibility */
static
cgm_s_edge_v(dat_ptr, dev_func)
    int             (*dev_func) ();
    unsigned char  *dat_ptr;
{
    int             ret = 1;
    int             new_flag;
    new_flag = cgm_eint(dat_ptr);
    a5->edge_vis = (enum bool_enum) new_flag;
    if (dev_func)
	ret = (*dev_func) (a5->edge_vis);
    return (ret);
}




/* set the fill reference point */
static
cgm_s_fill_ref(dat_ptr, dev_func)
    int             (*dev_func) ();
    unsigned char  *dat_ptr;
{
    int             ret = 1;
    int             new_fill[2], i, step_size, x, y;

    step_size = (vdc_step / byte_size);
    x = (*cgm_vdc) (dat_ptr);
    dat_ptr += step_size;
    y = (*cgm_vdc) (dat_ptr);
    dat_ptr += step_size;

    new_fill[0] = newx(x, y);
    new_fill[1] = newy(x, y);

    for (i = 0; i < 2; ++i)
	a5->fill_ref.i[i] = new_fill[i];
    if (dev_func)
	ret = (*dev_func) (a5->fill_ref.i[0], a5->fill_ref.i[1]);
    return (ret);
}



/* make a pattern table entry */
/* not really implemented yet */
static
cgm_p_tab_entry(dat_ptr, p_len, dev_func)
    int             p_len;
    int             (*dev_func) ();
    unsigned char  *dat_ptr;
{
    int             ret = 1;
    int             step_size, index, nx, ny, col_prec, no_bytes, l_col_prec;

    step_size = glbl1->ind_prec / byte_size;
    index = cgm_xint(dat_ptr);
    dat_ptr += step_size;
    step_size = glbl1->int_prec / byte_size;
    nx = cgm_sint(dat_ptr);
    dat_ptr += step_size;
    ny = cgm_sint(dat_ptr);
    dat_ptr += step_size;
    col_prec = cgm_sint(dat_ptr);
    dat_ptr += step_size;
    if (a2->c_s_mode == d_c_mode)
    {				       /* direct */
	l_col_prec = (col_prec) ? col_prec : glbl1->col_prec;
    } else
    {				       /* indexed */
	l_col_prec = (col_prec) ? col_prec : glbl1->col_i_prec;
    }

    no_bytes = nx * ny * l_col_prec / byte_size;

    if (dev_func)
	ret = (*dev_func) (index, nx, ny, col_prec, dat_ptr,
			   no_bytes);
    return (ret);
}




/* set the pattern size */
static
cgm_s_pat_size(dat_ptr, dev_func)
    int             (*dev_func) ();
    unsigned char  *dat_ptr;
{
    int             ret = 1;
    int             new_size[4], i, step_size;

    step_size = (vdc_step / byte_size);
    for (i = 0; i < 4; ++i)
    {
	new_size[i] = (*cgm_vdc) (dat_ptr) + 0.5;
	dat_ptr += step_size;
    }
    for (i = 0; i < 4; ++i)
	a5->pat_size.i[i] = new_size[i];
    if (dev_func)
	ret = (*dev_func) (a5->pat_size.i);
    return (ret);
}



/* make a colour table entry */
static
cgm_c_tab_entry(dat_ptr, p_len, dev_func)
    int             p_len;
    int             (*dev_func) ();
    unsigned char  *dat_ptr;
{
    int             ret = 1;
    int             step1_size, step2_size, beg_index, i, no_entries, col[3],
		    j, max_index;
    float           rcol[3];

    step1_size = (glbl1->col_i_prec / byte_size);
    step2_size = (glbl1->col_prec / byte_size);
    beg_index = cgm_cxint(dat_ptr);
    dat_ptr += step1_size;
    no_entries = (p_len - step1_size) / (3 * step2_size);
    max_index = beg_index + no_entries - 1;
    if (max_index > glbl1->max_c_index)
    {
	if (!cgm_do_mcind(max_index))
	    s_error ("trouble setting mcind");
    }
    for (i = 0; i < no_entries; ++i)
    {
	for (j = 0; j < 3; ++j)
	{
	    col[j] = cgm_dcint(dat_ptr);
	    dat_ptr += step2_size;
	    rcol[j] = dcind(j, col[j], glbl1->c_v_extent);
	}
	for (j = 0; j < 3; ++j)
	    *(a5->ctab + 3 * (beg_index + i) + j) = rcol[j];
    }
    if (dev_func)
	ret = (*dev_func) (beg_index, no_entries, a5->ctab);
    return (ret);
}



/* take care of the asps flags */
/* not really implemented yet */
static
cgm_do_aspsflags(dat_ptr, p_len, dev_func)
    int             p_len;
    int             (*dev_func) ();
    unsigned char  *dat_ptr;
{
    int             ret = 1;
#define max_pairs 18		       /* maximum no of pairs */
    int             no_pairs, flags[max_pairs * 2], i;

    no_pairs = p_len / (2 * e_size);
    if (no_pairs > max_pairs)
    {
	s_error ("illegal no_pairs in aspsflags ");
	no_pairs = max_pairs;
    }
    for (i = 0; i < (no_pairs * 2); ++i)
    {
	flags[i] = cgm_eint(dat_ptr);
	dat_ptr += e_size;
    }
    if (dev_func)
	ret = (*dev_func) (no_pairs, flags);
    return (ret);
}


/*
 * function to fill out the address pointers in a new address given an
 * old address and a byte offset from it
 */
/*
 * a little tricky because we want to handle record/offset only cases
 * also
 */
static int fill_ad(new_ad, last_ad, byte_offset)
    struct ad_struct *new_ad, *last_ad;
    long int        byte_offset;
{
    int             new_offset;
    long            new_byte;
    if ((new_byte = byte_offset + last_ad->b_ad) < 0)
    {
	return (0);
    } else
	new_ad->b_ad = new_byte;

    /* just for consistency */
    new_ad->r_ad = (new_byte + buf_size - 1) / buf_size;
    new_offset = new_byte % buf_size;
    /* may be greater than buf_size */
    if (new_offset >= buf_size)
	new_offset %= buf_size;
    /* may be negative */
    while (new_offset < 0)
	new_offset += buf_size;
    new_ad->offset = new_offset;
    return (1);
}


/* process an index block here */
static
read_index(str_ptr)
    char           *str_ptr;
{
    int             i, j, no_pages, page_no;
    long int        byte_ad, next_index;
    struct p_struct *this_page, *last_page;
    struct ad_struct next_ad;
#define have_number(c) (((c)=='-')||((c)=='+')||(('0'<=(c))&&((c)<='9')))

    if (index_present == 0)
    {
	index_present = 1;
	if (!random_input)
	    s_error ("indexed file, but no random access");
	/* and mark where we are */
	first_index.r_ad = last_ad.r_ad;
	first_index.b_ad = last_ad.b_ad;
	first_index.offset = last_ad.offset;
    }
    while (*str_ptr == ' ')
	++str_ptr;
    sscanf(str_ptr, "%d", &no_pages);
    while have_number
	(*str_ptr)++ str_ptr;
    for (i = 0; i < no_pages; ++i)
    {
	if ((this_page = (struct p_struct *) allocate_mem(p_s_size, 0)) != NULL)
	{
	    if (p_header.first == NULL)
	    {			       /* first time */
		p_header.first = this_page;
		this_page->no = 1;
	    } else
	    {
		this_page->no = p_header.last->no + 1;
		p_header.last->next = this_page;
	    }
	    p_header.last = this_page;
	    this_page->next = NULL;
	    while (*str_ptr == ' ')
		++str_ptr;
	    sscanf(str_ptr, "%ld", &byte_ad);
	    if (!fill_ad(&(this_page->ad), &last_ad, byte_ad))
	    {
		s_error ("illegal offset ");
	    }
	    while have_number
		(*str_ptr)++ str_ptr;
	    while (*str_ptr == ' ')
		++str_ptr;
	    sscanf(str_ptr, "%d", &this_page->len);
	    while have_number
		(*str_ptr)++ str_ptr;

	    /* now take care of the string */
	    ++str_ptr;		       /* one-char gap */
	    if ((this_page->len > 0) &&
		(this_page->str = (char *)
		 allocate_mem(this_page->len, 0)))
		for (j = 0; j < this_page->len; ++j)
		    this_page->str[j] = *str_ptr++;
	} else
	    s_error ("couldn't allocate this_page");
    }
    /* read the indices */
    if (1 != sscanf(str_ptr, " %ld", &next_index))
	s_error ("couldn't read next index");
    if (use_random && next_index)
    {
	/* want to jump to next block */
	/* last_ad holds the current command address */
	if (fill_ad(&next_ad, &last_ad, next_index))
	{
	    if (use_random)
		cgm_goto(&next_ad);
	} else
	    s_error ("trouble with fill_ad");
    }
    if (next_index == 0)	       /* all done */
	index_read = 1;
    if (next_index)
	return (1);

    /* have read last index so may show the data */
    /* may trim the list of pages */
    /* first trim by page numbers */
    if (opt[(int) pages].set)
    {
	page_no = 1;
	this_page = p_header.first;
	/* get first wanted page */
	while ((this_page) &&
	       (!want_page(page_no, opt[(int) pages].val.str)))
	{
	    ++page_no;
	    this_page = this_page->next;
	}
	last_page = p_header.first = this_page;
	/* now trim off unwanted pages */
	while (this_page)
	{
	    this_page = this_page->next;
	    ++page_no;
	    if (this_page && want_page(page_no, opt[(int) pages].val.str))
	    {
		last_page->next = this_page;
		last_page = this_page;
	    }
	}
	if (last_page)
	    last_page->next = NULL;
    }
    if (!opt[(int) screen].set)	       /* no output */
	return (f_e_mf(delim[(int) E_Mf], ctrl[(int) E_Mf]));
    /* else go back to where we started */
    if (use_random)
	cgm_goto(&first_index);
    return (1);
}



/* now the class6 functions */
/* do the special command */
static
cgm_do_escape(dat_ptr, dev_func)
    unsigned char  *dat_ptr;
    int             (*dev_func) ();
{
    int             ret = 1;
    int             escape_id, step_size;
    char           *str_ptr;

    escape_id = cgm_sint(dat_ptr);
    step_size = (glbl1->int_prec / byte_size);
    dat_ptr += step_size;
    str_ptr = cgm_str(dat_ptr, NULL);
    if ((escape_id == INDEX_FLAG) ||
	(escape_id == -1))
    {				       /* our index format */
	if (!index_read)
	{
	    read_index(str_ptr);
	}
    } else
    {
	if (dev_func)
	    ret = (*dev_func) (escape_id, str_ptr);
    }

    return (ret);
}

/* now the class 7 functions */
/* message command */
static
cgm_do_message(dat_ptr, dev_func)
    unsigned char  *dat_ptr;
    int             (*dev_func) ();
{
    int             ret = 1;
    char           *str_ptr;
    enum bool_enum  action;

    action = (enum bool_enum) cgm_eint(dat_ptr);
    dat_ptr += e_size;
    str_ptr = cgm_str(dat_ptr, NULL);

    if (dev_func)
	ret = (*dev_func) (action, str_ptr);

    return (ret);
}



/* application data */
static
cgm_do_apdata(dat_ptr, dev_func)
    unsigned char  *dat_ptr;
    int             (*dev_func) ();
{
    int             ret = 1;
    char           *str_ptr;
    int             id, step_size;

    id = cgm_sint(dat_ptr);
    step_size = (glbl1->int_prec / byte_size);
    dat_ptr += step_size;
    str_ptr = cgm_str(dat_ptr, NULL);
    if (dev_func)
	ret = (*dev_func) (id, str_ptr);

    return (ret);
}


/* replace the metafile defaults */
#define round_up(a) ((a) = ((a) % 2) ? (a) + 1 : (a))
static
cgm_s_mf_defs(dat_ptr, tot_len, dev_func)
    unsigned char  *dat_ptr;
    int             tot_len;
    int             (*dev_func) ();
{
    unsigned char  *my_ptr, *new_mem, *start_mem, *out_ptr;
    int             to_go, p_len, i, int_len, done, ret = 1, class, element;
    unsigned char   sec_word[2];

    if (tot_len <= 0)
    {
	return (1);
    }
    if (dev_func)
	(*dev_func) (0);	       /* start */
    a2 = dflt2;
    a3 = dflt3;
    a5 = dflt5;
    /* default values are active */
    /* first make some new memory */
    /* ptrs to outgoing data */
    out_ptr = (unsigned char *) allocate_mem(tot_len, 0);
    /* ptrs to outgoing data */
    new_mem = (unsigned char *) allocate_mem(tot_len, 0);
    if (!out_ptr)
	return (2);
    my_ptr = dat_ptr;
    to_go = tot_len;
    /* now move args into it */
    while (to_go > 0)
    {
	start_mem = my_ptr;	       /* ptrs into the incoming data */
	out_ptr = new_mem;	       /* ptrs into the outgoing data */
	*out_ptr++ = *my_ptr++;
	*out_ptr++ = *my_ptr++;
	p_len = *(new_mem + 1) & 31;

	if (p_len < 31)
	{			       /* short form command */
	    round_up(p_len);
	    for (i = 0; i < p_len; ++i)
		*out_ptr++ = *my_ptr++;
	} else
	{			       /* long form */
	    p_len = 0;
	    done = 0;
	    while (!done)
	    {
		for (i = 0; i < 2; ++i)
		    sec_word[i] = *my_ptr++;
		int_len = ((sec_word[0] << 8) + (sec_word[1])) % (1 << 15);
		round_up(int_len);
		for (i = 0; i < int_len; ++i)
		    *out_ptr++ = *my_ptr++;
		p_len += int_len;
		done = !(sec_word[0] & (1 << 7));
	    }
	}
	to_go -= (my_ptr - start_mem);
	if (!cgm_command(new_mem, p_len, &last_ad, &class, &element))
	    s_error ("trouble setting defaults");
    }
    a2 = glbl2;
    a3 = glbl3;
    a5 = glbl5;
    /* globals are now active again */
    if (dev_func)
	ret = (*dev_func) (1);	       /* end */
    return (ret);
}
#undef round_up


/* class 0, the delimiter elements */
static
cgm_class0(el, p_len, dat_ptr)
    unsigned char  *dat_ptr;
    enum cgmcls0    el;
    int             p_len;
{
    switch (el)
    {
    case No_Op:
	return (1);
    case B_Mf:
	return (cgm_f_b_mf(dat_ptr, p_len, delim[(int) el],
			   ctrl[(int) el]));
    case E_Mf:
	return (cgm_f_e_mf(delim[(int) el], ctrl[(int) el]));
    case B_Pic:
	return (cgm_f_b_p((char *) dat_ptr, p_len, delim[(int) el],
			  ctrl[(int) el]));
    case B_Pic_Body:
	return (cgm_f_b_p_body(delim[(int) el], ctrl[(int) el]));
    case E_Pic:
	return (cgm_f_e_pic(delim[(int) el], ctrl[(int) el]));
    default:
	s_error ("illegal CGM class 0 command");


    }
    return (get_all);		       /* may or may not want to
					* terminate on error */
}



/* class 1, the metafile descriptor elements */
static
cgm_class1(el, p_len, dat_ptr)
    unsigned char  *dat_ptr;
    int             p_len;
    enum cgmcls1    el;
{

    switch (el)
    {
    case MfVersion:
	return (cgm_rd_mf_version(dat_ptr, p_len,
				  mfdesc[(int) el]));
    case MfDescrip:
	return (cgm_rd_mf_descriptor(dat_ptr, p_len,
				     mfdesc[(int) el]));
    case vdcType:
	return (cgm_s_vdc_type(dat_ptr, mfdesc[(int) el]));
    case IntPrec:
	return (cgm_s_int_prec(dat_ptr, mfdesc[(int) el]));
    case RealPrec:
	return (cgm_s_real_prec(dat_ptr, mfdesc[(int) el]));
    case IndexPrec:
	return (cgm_s_index_prec(dat_ptr, mfdesc[(int) el]));
    case ColPrec:
	return (cgm_s_col_prec(dat_ptr, mfdesc[(int) el]));
    case CIndPrec:
	return (cgm_s_cind_prec(dat_ptr, mfdesc[(int) el]));
    case MaxCInd:
	return (cgm_s_mcind(dat_ptr, mfdesc[(int) el]));
    case CVExtent:
	return (cgm_s_cvextent(dat_ptr, mfdesc[(int) el]));
    case MfElList:
	return (cgm_rd_mf_list(dat_ptr, mfdesc[(int) el]));
    case MfDefRep:
	return (cgm_s_mf_defs(dat_ptr, p_len,
			      mfdesc[(int) el]));
    case FontList:
	return (cgm_do_font_list(dat_ptr, p_len,
				 mfdesc[(int) el]));
    case CharList:
	return (cgm_do_char_list(dat_ptr, p_len,
				 mfdesc[(int) el]));
    case CharAnnounce:
	return (cgm_do_cannounce(dat_ptr, mfdesc[(int) el]));
    default:
	s_error ("illegal CGM class 1 command");

    }
    return (get_all);		       /* may or may not want to
					* terminate on error */
}




/* class 2, the picture descriptor elements */
static
cgm_class2(el, dat_ptr)
    unsigned char  *dat_ptr;
    enum cgmcls2    el;
{
    /*
     * the device may not want calls during the metafile defaults
     * replacement
     */

#define maybe2 (((a2 == dflt2) && (dev_info->capability & no_def_calls)) \
	? NULL : pdesc[(int) el])

    switch (el)
    {
    case ScalMode:
	return (cgm_s_scalmode(dat_ptr, maybe2));
    case ColSelMode:
	return (cgm_s_c_s_mode(dat_ptr, maybe2));
    case LWidSpecMode:
	return (cgm_s_lws_mode(dat_ptr, maybe2));
    case MarkSizSpecMode:
	return (cgm_s_ms_mode(dat_ptr, maybe2));
    case EdWidSpecMode:
	return (cgm_s_ew_mode(dat_ptr, maybe2));
    case vdcExtent:
	return (cgm_s_vdc_extent(dat_ptr, maybe2));
    case BackCol:
	return (cgm_s_back_col(dat_ptr, maybe2));
    default:
	s_error ("illegal CGM class 2 command");
    }
    return (get_all);
#undef maybe2
}



/* class 3, the control elements */
static
cgm_class3(el, dat_ptr)
    unsigned char  *dat_ptr;
    enum cgmcls3    el;
{
    /*
     * the device may not want calls during the metafile defaults
     * replacement
     */

#define maybe3 (((a3 == dflt3) && (dev_info->capability & no_def_calls)) \
	? NULL : mfctrl[(int) el])

    switch (el)
    {
    case vdcIntPrec:
	return (cgm_s_vdc_i_p(dat_ptr, maybe3));
    case vdcRPrec:
	return (cgm_s_vdc_r_p(dat_ptr, maybe3));
    case AuxCol:
	return (cgm_s_aux_col(dat_ptr, maybe3));
    case Transp:
	return (cgm_s_transp(dat_ptr, maybe3));
    case ClipRect:
	return (cgm_s_clip_rec(dat_ptr, maybe3));
    case ClipIndic:
	return (cgm_s_clip_ind(dat_ptr, maybe3));
    default:
	s_error ("illegal CGM class 3 command");
    }
    return (get_all);
#undef maybe3
}



/* class 4, the graphical primitive elements */
static
cgm_class4(el, p_len, dat_ptr)
    unsigned char  *dat_ptr;
    int             p_len;
    enum cgmcls4    el;
{
    /* now go do it */

    switch (el)
    {
    case PolyLine:
	return (cgm_do_polyline(dat_ptr, p_len,
				gprim[(int) el]));
    case Dis_Poly:
	return (cgm_do_dis_polyline(dat_ptr, p_len,
				    gprim[(int) el]));
    case PolyMarker:
	return (cgm_do_polymarker(dat_ptr, p_len,
				  gprim[(int) el]));
    case TeXt:
	return (cgm_s_text(dat_ptr, gprim[(int) el]));
    case Rex_Text:
	return (cgm_s_rex_text(dat_ptr, gprim[(int) el]));
    case App_Text:
	return (cgm_s_app_text(dat_ptr, gprim[(int) el]));
    case Polygon:
	return (cgm_do_polygon(dat_ptr, p_len,
			       gprim[(int) el]));
    case Poly_Set:
	return (cgm_do_polyset(dat_ptr, p_len,
			       gprim[(int) el]));
    case Cell_Array:
	return (cgm_do_cell_array(dat_ptr, p_len,
				  gprim[(int) el]));
    case Gen_D_Prim:
	return (cgm_do_g_d_p(dat_ptr, p_len,
			     gprim[(int) el]));
    case RectAngle:
	return (cgm_do_rectangle(dat_ptr, gprim[(int) el]));
    case Cgm_Circle:
	return (cgm_do_circle(dat_ptr, gprim[(int) el]));
    case Circ_3:
	return (cgm_do_c3(dat_ptr, gprim[(int) el]));
    case Circ_3_Close:
	return (cgm_do_c3_close(dat_ptr, gprim[(int) el]));
    case Circ_Centre:
	return (cgm_do_c_centre(dat_ptr, gprim[(int) el]));
    case Circ_C_Close:
	return (cgm_do_c_c_close(dat_ptr, gprim[(int) el]));
    case Ellipse:
	return (cgm_do_ellipse(dat_ptr, gprim[(int) el]));
    case Ellip_Arc:
	return (cgm_do_ell_arc(dat_ptr, gprim[(int) el]));
    case El_Arc_Close:
	return (cgm_do_e_a_close(dat_ptr, gprim[(int) el]));
    default:
	s_error ("illegal CGM class 4 command");
    }
    return (get_all);
}



/* class 5, the attribute elements */
static
cgm_class5(el, p_len, dat_ptr)
    unsigned char  *dat_ptr;
    int             p_len;
    enum cgmcls5    el;
{
    /*
     * the device may not want calls during the metafile defaults
     * replacement
     */

#define maybe5 (((a5 == dflt5) && (dev_info->capability & no_def_calls)) \
	? NULL : attr[(int) el])

    switch (el)
    {
    case LBIndex:
	return (cgm_s_lbindex(dat_ptr, maybe5));
    case LType:
	return (cgm_s_l_type(dat_ptr, maybe5));
    case LWidth:
	return (cgm_s_l_width(dat_ptr, maybe5));
    case LColour:
	return (cgm_s_l_colour(dat_ptr, maybe5));
    case MBIndex:
	return (cgm_s_mbindex(dat_ptr, maybe5));
    case MType:
	return (cgm_s_mk_type(dat_ptr, maybe5));
    case MSize:
	return (cgm_s_mk_size(dat_ptr, maybe5));
    case MColour:
	return (cgm_s_mk_colour(dat_ptr, maybe5));
    case TBIndex:
	return (cgm_s_tbindex(dat_ptr, maybe5));
    case TFIndex:
	return (cgm_s_t_index(dat_ptr, maybe5));
    case TPrec:
	return (cgm_s_t_prec(dat_ptr, maybe5));
    case CExpFac:
	return (cgm_s_c_exp(dat_ptr, maybe5));
    case CSpace:
	return (cgm_s_c_space(dat_ptr, maybe5));
    case TColour:
	return (cgm_s_t_colour(dat_ptr, maybe5));
    case CHeight:
	return (cgm_s_c_height(dat_ptr, maybe5));
    case COrient:
	return (cgm_s_c_orient(dat_ptr, maybe5));
    case TPath:
	return (cgm_s_tpath(dat_ptr, maybe5));
    case TAlign:
	return (cgm_s_t_align(dat_ptr, maybe5));
    case CSetIndex:
	return (cgm_s_csindex(dat_ptr, maybe5));
    case AltCSetIndex:
	return (cgm_s_acsindex(dat_ptr, maybe5));
    case FillBIndex:
	return (cgm_s_fbindex(dat_ptr, maybe5));
    case IntStyle:
	return (cgm_s_interior_style(dat_ptr, maybe5));
    case FillColour:
	return (cgm_s_fill_colour(dat_ptr, maybe5));
    case HatchIndex:
	return (cgm_s_hindex(dat_ptr, maybe5));
    case PatIndex:
	return (cgm_s_pindex(dat_ptr, maybe5));
    case EdBIndex:
	return (cgm_s_e_b_index(dat_ptr, maybe5));
    case EType:
	return (cgm_s_edge_t(dat_ptr, maybe5));
    case EdWidth:
	return (cgm_s_edge_w(dat_ptr, maybe5));
    case EdColour:
	return (cgm_s_edge_c(dat_ptr, maybe5));
    case EdVis:
	return (cgm_s_edge_v(dat_ptr, maybe5));
    case FillRef:
	return (cgm_s_fill_ref(dat_ptr, maybe5));
    case PatTab:
	return (cgm_p_tab_entry(dat_ptr, p_len, maybe5));
    case PatSize:
	return (cgm_s_pat_size(dat_ptr, maybe5));
    case ColTab:
	return (cgm_c_tab_entry(dat_ptr, p_len, maybe5));
    case AspsFlags:
	return (cgm_do_aspsflags(dat_ptr, p_len, maybe5));
    default:
	s_error ("illegal CGM class 5 command");
    }
    return (get_all);
#undef maybe5
}



/* class 6, the escape element */
static
cgm_class6(el, dat_ptr)
    unsigned char  *dat_ptr;
    enum cgmcls6    el;
{

    switch (el)
    {
    case Escape:
	return (cgm_do_escape(dat_ptr, escfun[(int) el]));
    default:
	s_error ("illegal CGM class 6 command");
    }
    return (get_all);
}



/* class 7, the external elements */
/* not really implemented yet */
static
cgm_class7(el, dat_ptr)
    enum cgmcls7    el;
    unsigned char  *dat_ptr;
{

    switch (el)
    {
    case Message:
	return (cgm_do_message(dat_ptr, extfun[(int) el]));
    case Ap_Data:
	return (cgm_do_apdata(dat_ptr, extfun[(int) el]));
    default:
	s_error ("illegal CGM class 7 command");
    }
    return (get_all);
}


/* parcel out the commands here */
static int cgm_command(s_ptr, p_len, this_ad, out_class, out_element)
    unsigned char  *s_ptr;
    int             p_len;
    struct ad_struct *this_ad;
    int            *out_class, *out_element;
{
    unsigned int    byte1, byte2, class, element;
    /* are we replacing defaults ? */

    /* mark our spot */
    last_ad.r_ad = this_ad->r_ad;
    last_ad.b_ad = this_ad->b_ad;
    last_ad.offset = this_ad->offset;


    byte1 = *s_ptr;
    byte2 = *(s_ptr + 1);
    *out_class = class = byte1 >> 4;
    *out_element = element = ((byte1 << 3) & 127) + (byte2 >> 5);


    /* are we starting a new page ? */
    if ((class == 0) && ((enum cgmcls0) element == B_Pic))
    {
	++in_page_no;		       /* new page */
	if (use_random && index_read)
	{			       /* all set up for random access */
	    if (p_header.first)
	    {			       /* a real live page */
		if ((in_page_no == 1) &&
		    (opt[(int) pages].set))
		{
		    cgm_goto(&(p_header.first->ad));
		    return (1);
		}
	    }
	}
	skipping = (!want_page(in_page_no, opt[(int) pages].val.str)) &&
	    !(use_random && index_read);
    }
    /* now check for the end of the page */
    if ((class == 0) && ((enum cgmcls0) element == E_Pic))
    {
	/* if skipping is on, turn it off at the end of the page */
	if (skipping)
	{
	    skipping = 0;
	    return (1);
	}
    }
    /* now if we don't want this page, don't do anything until ended */
    if (skipping)
	return (1);
    /* note trouble if we were to get a B_Pic inside a metafile def rep */

    s_ptr += 2;

    switch (class)
    {
    case 0:
	no_defs(0);
	return (cgm_class0(element, p_len, s_ptr));
    case 1:
	no_defs(1);
	return (cgm_class1(element, p_len, s_ptr));
    case 2:
	return (cgm_class2(element, s_ptr));
    case 3:
	return (cgm_class3(element, s_ptr));
    case 4:
	no_defs(4);
	return (cgm_class4(element, p_len, s_ptr));
    case 5:
	return (cgm_class5(element, p_len, s_ptr));
    case 6:
	no_defs(6);
	return (cgm_class6(element, s_ptr));
    case 7:
	no_defs(7);
	return (cgm_class7(element, s_ptr));
    default:
	s_error ("illegal cgm_class");
    }

    return (get_all);		       /* may or may not want to
					* terminate on error */
}



/*
 * this module contains one function, dev_setup, which is called by the
 * main modules at startup time. It is the only existing piece of
 * code which must be modified to add a new device driver
 */
/* here is the common argument list for all of these routines */

/* this is the routine that sets everything up */
/* the initialising routine for the emulation module */
static void
em_setup(opt, dev_info, c1, c2, c3, c5, delim, mfdesc, pdesc, mfctrl,
	 gprim, attr, escfun, extfun, ctrl)

    struct one_opt *opt;	       /* the command line options, in
					* only */
    struct info_struct *dev_info;      /* device info  */
    struct mf_d_struct *c1;	       /* the class 1 elements, in only */
    struct pic_d_struct *c2;	       /* the class 2 elements, in only */
    struct control_struct *c3;	       /* the class 3 elements, in only */
    struct attrib_struct *c5;	       /* the class 5 elements, in only */
    int             (*delim[]) ();     /* delimiter functions */
    int             (*mfdesc[]) ();    /* metafile descriptor functions */
    int             (*pdesc[]) ();     /* page descriptor functions */
    int             (*mfctrl[]) ();    /* controller functions */
    int             (*gprim[]) ();     /* graphical primitives */
    int             (*attr[]) ();      /* the attribute functions */
    int             (*escfun[]) ();    /* the escape functions */
    int             (*extfun[]) ();    /* the external functions */
    int             (*ctrl[]) ();      /* controller functions */
{
    /* and the device function array pointers */
    p_gprim = gprim;

    pi = 4.0 * atan(1.0);

    setup = 1;
    return;
}

#define arg_list opt, dev_info, c1, c2, c3, c5, delim, mfdesc, pdesc, mfctrl, \
  gprim, attr, escfun, extfun, ctrl

static int dev_setup(arg_list)

    struct one_opt *opt;	       /* the command line options, in
					* only */
    struct info_struct *dev_info;      /* device info to fill out, out
					* only */
    struct mf_d_struct *c1;	       /* the class 1 elements, in only */
    struct pic_d_struct *c2;	       /* the class 2 elements, in only */
    struct control_struct *c3;	       /* the class 3 elements, in only */
    struct attrib_struct *c5;	       /* the class 5 elements, in only */
    int             (*delim[]) ();     /* delimiter functions */
    int             (*mfdesc[]) ();    /* metafile descriptor functions */
    int             (*pdesc[]) ();     /* page descriptor functions */
    int             (*mfctrl[]) ();    /* controller functions */
    int             (*gprim[]) ();     /* graphical primitives */
    int             (*attr[]) ();      /* the attribute functions */
    int             (*escfun[]) ();    /* the escape functions */
    int             (*extfun[]) ();    /* the external functions */
    int             (*ctrl[]) ();      /* controller functions */

{
#define dev_length 20		       /* maximum device name length */
    static char     dev_name[dev_length + 1];
    int             i;


    /* now see if we asked for a device */
    if (opt[(int) screen].set)
	strncpy(dev_name, opt[(int) screen].val.str, dev_length);
    else
    {				       /* no device set, listing only */
	dev_info->pxl_in = 1.0;
	dev_info->ypxl_in = 1.0;
	dev_info->x_size = 3276.0;
	dev_info->y_size = 3276.0;
	dev_info->x_offset = 0.0;
	dev_info->y_offset = 0.0;
	*dev_info->out_name = '\0';    /* no output file */
	dev_info->capability = 0;
	dev_info->rec_size = 80;
	return (1);
    }
    /*
     * so we actually have a device requested, now need to figure out
     * who to call
     */

    dev_name[dev_length] = '\0';       /* for safety */

    /* let's convert to lower case */
    for (i = 0; i < strlen(dev_name); ++i)
	dev_name[i] = (isupper(dev_name[i])) ? tolower(dev_name[i]) :
	    dev_name[i];

    /*
     * before talking to devices, we'll send all the pointers to the
     * emulation package
     */
    em_setup(arg_list);

    gks_setup(arg_list);

    return (1);
}



/* massage record into correct format */
static int massage_record(local_rec, size)
    unsigned char  *local_rec;
    int             size;
{
    int             i, j, header_size;
    unsigned char  *cptr;
    unsigned char   new_set[map_size];

    if (need_map)
    {				       /* need to rmap the bytes */
	cptr = local_rec;
	for (i = 0; i < ((size + map_size - 1) / map_size); ++i)
	{
	    for (j = 0; j < map_size; ++j)
		new_set[j] = cptr[poss_map[need_map][j]];
	    for (j = 0; j < map_size; ++j)
		cptr[j] = new_set[j];
	    cptr += map_size;
	}
    }
    /* may have a variable header size here */
    if (ncar)
	header_size = 4;
    else if (skip_header > 0)
	header_size = skip_header;
    else
	header_size = 0;
    return (header_size);
}
/* read any implementation-specific record header */
static int process_header(header_ptr, total_bytes)
    char           *header_ptr;
    int             total_bytes;
{
    struct header_type *header_record;
    int             ret1, ret2, tot_bytes;
    if (!ncar)
	return (total_bytes - skip_header);
    header_record = (struct header_type *) header_ptr;

    ret1 = header_record->byte1 << byte_size;
    ret2 = (header_record->byte2 & 255);
    tot_bytes = ret1 + ret2;
    if (tot_bytes % 2)
	++tot_bytes;		       /* must be even */

    return (tot_bytes);
}

/*
 * function to get some more binary CGM bytes, returns zero if have
 * problem
 */
/* assume already massaged */
static int cgm_bytes(mem_ptr, no_bytes)
    char           *mem_ptr;	       /* where to start putting the
					* bytes */
    int             no_bytes;	       /* how many to put */
{
    int             bytes_done = 0, bytes_got, header_bytes, data_bytes;
    char           *my_ptr;

    if (no_bytes == 0)
	return (1);		       /* nothing to do */
    if (no_bytes < 0)
    {
	s_error ("illegal cgm_byte request ");
	return (0);
    }
    /* now real requests */

    my_ptr = mem_ptr;
    while (bytes_done < no_bytes)
    {
	if (buf_ptr >= end_buf)
	{			       /* need new record */
	    bytes_got = get_record(start_buf, buf_size);
	    if (!bytes_got)
		return (0);
	    header_bytes = massage_record(start_buf, bytes_got);
	    data_bytes = process_header((char *) start_buf, bytes_got);
	    buf_ptr = start_buf + header_bytes;
	    end_buf = buf_ptr + data_bytes;
	} else
	{
	    *my_ptr++ = *buf_ptr++;
	    ++bytes_done;
	}
    }
    return (1);
}

/* need to set NCAR variable and byte map */
static int check_format(local_rec, size)
    unsigned char  *local_rec;
    int             size;
{

    int             i, j, k;
    unsigned int    class, element;
    unsigned char   new_set[map_size];
    struct header_type *header_record;

    /*
     * there are three issues: the necessary mapping to get the byte
     * order correct, whether it is an NCAR file with its own record
     * structure, or whether it ha a single header at the beginning
     * that must be skipped
     */

    if (size < map_size)
    {
	s_error ("too few bytes");
	return (0);
    }
    /* look for NCAR type files first */
    for (i = 0; i < poss_maps; ++i)
    {
	for (j = 0; j < map_size; ++j)
	    new_set[j] = local_rec[poss_map[i][j]];

	header_record = (struct header_type *) new_set;
	if (((header_record->flags >> 4) == m_ext_cgm) &&
	    (header_record->dummy == 0) &&
	    ((header_record->flags % 16) == m_beg_meta))
	{
	    ncar = 1;
	    need_map = i;
	    return (1);
	}
    }
    /* now look for a beginning B_Mf (vanilla format) */
    for (i = 0; i < poss_maps; ++i)
    {
	for (j = 0; j < map_size; ++j)
	    new_set[j] = local_rec[poss_map[i][j]];

	class = new_set[0] >> 4;
	element = ((new_set[0] << 3) & 127) + (new_set[1] >> 5);

	if ((class == 0) && (element == (int) B_Mf))
	{			       /* no header */
	    ncar = 0;
	    need_map = i;
	    return (1);
	}
    }
    /* do we have a military header ? */
    for (i = 0; i < poss_maps; ++i)
    {
	for (j = 0; j < map_size; ++j)
	    new_set[j] = local_rec[poss_map[i][j]];
	/* for now define a military type header as being all printable */
	for (k = 0; (k < map_size) && (new_set[k] >= ' ') && (new_set[k] <= 'z');
	     ++k);
	if (k == map_size)
	{
	    skip_header = 7 * 80;      /* bytes */
	    return (1);
	}
    }
    /* maybe a LANL header ? */
    for (i = 0; i < poss_maps; ++i)
    {
	for (j = 0; j < map_size; ++j)
	    new_set[j] = local_rec[poss_map[i][j]];
	if (new_set[0] == 1)
	{
	    skip_header = 360;
	    return (1);
	}
    }

    /* probably can't recover */
    s_error ("unknown header format");

    return (0);
}



/* open the meta file */
static int open_metafile(file_name, def_name, o_name, full_oname, last_try, talk,
	      random_file)
    char           *file_name, *def_name, *o_name, *full_oname;
    int             last_try, talk, *random_file;
{
   

    if (file_name[0] == '-')
	inptr = stdin;		       /* purposely redirected */
    else if (*file_name)
    {
	if (NULL == (inptr = fopen(file_name, "r")))
	{
	    s_error ("couldn't fopen to read %s",
		    file_name);
	    perror("open_metafile");
	    return (0);
	}
    } else
    {
	s_error ("no file name, will use stdin");
	inptr = stdin;
    }
#ifdef VAXC
    fgetname(inptr, unix_name);
    strcpy(full_oname, unix_name);
    *random_file = 0;		       /* no random access */
#else
    strcpy(full_oname, file_name);
    *random_file = 1;		       /* UNIX can do it (I hope) */
#endif
    strncpy(o_name, file_name, max_str);
    return (BUFSIZ);
}


/* open up the input file */
static int open_input_file(file_name, o_name, talk, def_name, full_oname, in_info,
		c_text, random_file)
    char           *file_name, *o_name, *def_name, *full_oname;
    int             talk, c_text, *random_file;
    struct info_struct *in_info;
{
    int             bytes_got, header_bytes, data_bytes;

    recs_got = 0;		       /* no records yet */
    bytes_read = 0;		       /* nor bytes */

    if (!(buf_size = open_metafile(file_name,
		 def_name, o_name, full_oname, 1, talk, random_file)))
	return (0);


    start_buf = (unsigned char *) allocate_mem(buf_size, 0);
    if (!start_buf)
	exit(0);

    /* get the first non-zero record */
    while (!(bytes_got = get_record(start_buf, buf_size)));
    if (bytes_got < 0)
    {
	s_error ("can't get the first record");
	return (0);
    }
    /* what type of file is it ? */
    ncar = skip_header = need_map = 0;
    if ((!c_text) && (!check_format(start_buf, bytes_got)))
	return (0);		       /* can't recognise it */
    /* have recognized the file, but may have to deal with a header */

    /* have to concern ourselves about NCAR format files */
    if (ncar && (buf_size != ncar_size))
    {
	if (buf_size < ncar_size)
	{
	    start_buf = (unsigned char *) realloc(start_buf, ncar_size);
	    if (!start_buf)
		exit(0);
	}
	buf_size = ncar_size;
	rewind(inptr);		       /* start again */
	/* get the first non-zero record */
	while (!(bytes_got = get_record(start_buf, buf_size)));
	if (bytes_got < 0)
	{
	    s_error ("can't get the first record");
	    return (0);
	}
    }
    /* may be a one time header */
    if (skip_header)
    {
	rewind(inptr);
	while (skip_header >= buf_size)
	{
	    if ((bytes_got = get_record(start_buf, buf_size)) < 0)
	    {
		s_error ("couldn't get header ");
		return (0);
	    }
	    skip_header -= bytes_got;
	}
	while (skip_header > 0)
	{
	    if ((bytes_got = get_record(start_buf, skip_header)) < 0)
	    {
		s_error ("couldn't get header ");
		return (0);
	    }
	    skip_header -= bytes_got;
	}
	/* now get a fresh set */
	bytes_got = get_record(start_buf, buf_size);
    }
    /* carry on, possibly handling a header */
    header_bytes = massage_record(start_buf, bytes_got);
    data_bytes = process_header((char *) start_buf, bytes_got);
    /* initialise the pointers */
    buf_ptr = start_buf + header_bytes;
    end_buf = buf_ptr + data_bytes;
    if (skip_header)
	skip_header = 0;	       /* one-time charge */

    return (buf_size);
}



/* finish up */
static int close_up(inc)
    int             inc;
{

    if ((inptr != NULL) && (inptr != stdin))
	fclose(inptr);
    return (1);
}

/*
 * take care of getting the next command and all of its data into
 * memory
 */
/* note that we must always round up the no. of bytes to an even no. */
#define round_up(a) ((a) = ((a) % 2) ? (a) + 1 : (a))
static int get_cmd(mem_ptr, mem_size, out_ad)
    int            *mem_size;	       /* how much memory got */
    char           *(*mem_ptr);	       /* ptr to a ptr */
    struct ad_struct *out_ad;	       /* address of command */
{
    char           *my_ptr, second_byte;
    int             p_len, int_len, done = 0, new_size;
    unsigned char   sec_word[2];

    my_ptr = *mem_ptr;     /* beginning of allocated memory */
    /* fill out the command address, may be about to get another record */
    out_ad->r_ad = recs_got;
    out_ad->offset = buf_ptr - start_buf;
    if (buf_ptr >= end_buf)
    {				       /* need new record */
	out_ad->offset = 0;
	++out_ad->r_ad;
    }
    out_ad->b_ad = bytes_read - (end_buf - buf_ptr);
    if (!cgm_bytes(my_ptr, 2))
	return (-1);

    second_byte = *(my_ptr + 1);
    p_len = second_byte & 31;
    my_ptr += 2;		       /* step forward */

    if (p_len < 31)
    {				       /* short form command */
	round_up(p_len);
	if (!cgm_bytes(my_ptr, p_len))
	    return (-1);
	else
	    return (p_len);
    }				       /* now handle long format */
    p_len = 0;
    while (!done)
    {
	if (!cgm_bytes(sec_word, 2))
	    return (-1);
	int_len = ((sec_word[0] & 127) << byte_size) | sec_word[1];
	round_up(int_len);
	if ((new_size = (p_len + 2 + int_len)) > *mem_size)
	{
	    *mem_ptr = (char *) realloc(*mem_ptr, new_size);
	    if (!*mem_ptr)
	    {
		s_error ("couldn't allocate enough memory, aborting");
		exit(0);
	    } else
		s_error ("grabbing more memory");
	    my_ptr = *mem_ptr + 2 + p_len;
	    *mem_size = new_size;
	}
	if (!cgm_bytes(my_ptr, int_len))
	    return (-1);
	p_len += int_len;
	my_ptr += int_len;
	done = !(sec_word[0] & (1 << 7));
    }

    return (p_len);
}
#undef round_up

/*
 * function to set CGM status structures to their startup default
 * values
 * note that we do not do this at compile time because of the
 * possibility of processing multiple CGM files.
 */

static void
s_defaults(c1, c2, c3, c5, g5, pxlvdc_in)
    struct mf_d_struct *c1;	       /* the class 1 elements */
    struct pic_d_struct *c2;	       /* the class 2 elements */
    struct control_struct *c3;	       /* the class 3 elements */
    struct attrib_struct *c5, *g5;     /* the class 5 elements */
    double          pxlvdc_in;	       /* the pxl_vdc value */
{
    int             i, nsteps;
    /* and set up the initial colour table values */
#define init_csize 13		       /* how many entries */
    static float    init_ctab[3 * init_csize]	/* global memory */
    =
    {				       /* my defaults */
	1.0, 1.0, 1.0,		       /* white */
	0.0, 0.0, 0.0,		       /* black */
	1.0, 0.0, 0.0,		       /* red */
	0.0, 1.0, 0.0,		       /* green */
	0.0, 0.0, 1.0,		       /* blue */
	1.0, 1.0, 0.0,		       /* yellow */
	1.0, 0.0, 1.0,		       /* magenta */
	0.0, 1.0, 1.0,		       /* cyan */
	1.0, 0.5, 0.0,		       /* orange */
	1.0, 0.0, 0.5,		       /* purple-red */
	0.5, 0.0, 1.0,		       /* blue-purple */
	0.5, 1.0, 0.0,		       /* yellow-green */
	0.0, 1.0, 0.5		       /* bluish-green */
    };

    /* first the class1, metafile descriptor elements */
    c1->vdc_type = vdc_int;
    c1->int_prec = 16;
    c1->real_prec.fixed = 1;
    c1->real_prec.exp = 16;
    c1->real_prec.fract = 16;
    c1->ind_prec = 16;
    c1->col_prec = 8;
    c1->col_i_prec = 8;
    c1->max_c_index = 63;
    for (i = 0; i < 3; ++i)
    {
	c1->c_v_extent.min[i] = 0;
	c1->c_v_extent.max[i] = 255;
    }
    c1->char_c_an = 0;

    /* now class2, the  picture descriptor elements */
    c2->scale_mode.s_mode = 0;
    c2->scale_mode.m_scaling = 0.0;
    c2->c_s_mode = i_c_mode;
    c2->l_w_s_mode = scaled;
    c2->m_s_s_mode = scaled;
    c2->e_w_s_mode = scaled;
    for (i = 0; i < 2; ++i)
    {
	c2->vdc_extent.i[i] = 0;
	c2->vdc_extent.r[i] = 0.0;
	c2->vdc_extent.i[i + 2] = 32767;
	c2->vdc_extent.r[i + 2] = 1.0;
    }
    c2->back_col.red = 1.0;
    c2->back_col.green = 1.0;
    c2->back_col.blue = 1.0;

    /* now the control elements, class 3 */
    /* note that everything except vdc_extent is kept in pixel units */
    c3->vdc_i_prec = 16;
    c3->vdc_r_prec.fixed = 1;
    c3->vdc_r_prec.exp = 16;
    c3->vdc_r_prec.fract = 16;
    c3->aux_col.red = 1.0;
    c3->aux_col.green = 1.0;
    c3->aux_col.blue = 1.0;
    c3->aux_col.ind = 0;
    c3->transparency = on;
    for (i = 0; i < 4; ++i)
    {
	c3->clip_rect.i[i] = pxlvdc_in * c2->vdc_extent.i[i];
	c3->clip_rect.r[i] = pxlvdc_in * c2->vdc_extent.r[i];
    }
    c3->clip_ind = on;

    /* class 5, the attribute elements */
    c5->l_b_index = 1;
    c5->line_type = 1;
    c5->line_width.i = pxlvdc_in * 33;
    c5->line_width.r = 1.0;
    c5->line_colour.red = 0.0;
    c5->line_colour.green = 0.0;
    c5->line_colour.blue = 0.0;
    c5->line_colour.ind = 1;
    c5->mk_b_index = 1;
    c5->mk_type = 3;
    c5->mk_size.i = pxlvdc_in * 328;   /* standard has error  */
    c5->mk_size.r = 1.0;
    c5->mk_colour.red = 0.0;
    c5->mk_colour.green = 0.0;
    c5->mk_colour.blue = 0.0;
    c5->mk_colour.ind = 1;
    c5->t_b_index = 1;
    c5->t_f_index = 1;
    c5->t_prec = string;
    c5->c_exp_fac = 1.0;
    c5->c_space = 0.0;
    c5->text_colour.red = 0.0;
    c5->text_colour.green = 0.0;
    c5->text_colour.blue = 0.0;
    c5->text_colour.ind = 1;
    c5->c_height = pxlvdc_in * 328;
    c5->c_orient.x_up = 0;
    c5->c_orient.y_up = 1;
    c5->c_orient.x_base = 1;
    c5->c_orient.y_base = 0;
    c5->text_path = right;
    c5->text_align.hor = normal_h;
    c5->text_align.ver = normal_v;
    c5->text_align.cont_hor = 0.0;
    c5->text_align.cont_ver = 0.0;
    c5->c_set_index = 1;
    c5->a_c_set_index = 1;
    c5->f_b_index = 1;
    c5->int_style = hollow;
    c5->fill_colour.red = 0.0;
    c5->fill_colour.green = 0.0;
    c5->fill_colour.blue = 0.0;
    c5->fill_colour.ind = 1;
    c5->hatch_index = 1;
    c5->pat_index = 1;
    c5->e_b_index = 1;
    c5->edge_type = 1;
    c5->edge_width.i = pxlvdc_in * 33;
    c5->edge_width.r = 1.0;
    c5->edge_colour.red = 0.0;
    c5->edge_colour.green = 0.0;
    c5->edge_colour.blue = 0.0;
    c5->edge_colour.ind = 1;
    c5->edge_vis = off;
    for (i = 0; i < 2; ++i)
    {
	c5->fill_ref.i[i] = 0;
	c5->fill_ref.r[i] = 0.0;
    }
    for (i = 0; i < 4; ++i)
    {
	c5->pat_size.i[i] = pxlvdc_in * c2->vdc_extent.i[i];
	c5->pat_size.r[i] = pxlvdc_in * c2->vdc_extent.r[i];
    }
    /* now allocate both colour tables */
    c5->ctab =
	(float *) allocate_mem((c1->max_c_index + 1) * 3 * float_size, 1);
    g5->ctab =
	(float *) allocate_mem((c1->max_c_index + 1) * 3 * float_size, 1);

    /* and fill them in with the defaults */

    for (i = 0; (i < (3 * init_csize)) && (i < 3 * (c1->max_c_index + 1)); ++i)
	*(g5->ctab + i) = *(c5->ctab + i) = init_ctab[i];
    nsteps = (c1->max_c_index + 1 - init_csize) / 3;
    if (nsteps <= 0)
    {
	s_error ("illegal nsteps ");
	return;
    }
    for (i = 3 * init_csize; i < 3 * (init_csize + nsteps); i += 3)
    {
	*(g5->ctab + i) = *(c5->ctab + i) =
	    (float) (i - 3 * init_csize) / (3 * nsteps);
	*(g5->ctab + i + 1) = *(c5->ctab + i + 1) = 0.0;
	*(g5->ctab + i + 2) = *(c5->ctab + i + 2) = 0.0;
    }
    for (i = 3 * (init_csize + nsteps); i < 3 * (init_csize + 2 * nsteps);
	 i += 3)
    {
	*(g5->ctab + i) = *(c5->ctab + i) = 0.0;
	*(g5->ctab + i + 1) = *(c5->ctab + i + 1) =
	    (float) (i - 3 * (init_csize + nsteps)) / (3 * nsteps);
	*(g5->ctab + i + 2) = *(c5->ctab + i + 2) = 0.0;
    }
    for (i = 3 * (init_csize + 2 * nsteps); i <= 3 * c1->max_c_index;
	 i += 3)
    {
	*(g5->ctab + i) = *(c5->ctab + i) = 0.0;
	*(g5->ctab + i + 1) = *(c5->ctab + i + 1) = 0.0;
	*(g5->ctab + i + 2) = *(c5->ctab + i + 2) =
	    (float) (i - 3 * (init_csize + 2 * nsteps)) /
	    (3 * (c1->max_c_index - (init_csize + 2 * nsteps)));
    }
    /* pattern, bundle tables later */

}

cgm_import (filename, encoding, picture, interface)

    char  *filename;
    int   encoding;
    int   picture;
    int   interface;

{
    int             to_screen = 0, random_file, pages_done;
    char            in_fname[max_str]; /* the input file name */
    char            open_name[max_str];/* the opened file name */
    char            full_oname[max_str];	/* full input file
						 * specification */
    unsigned char  *start_mem = NULL;  /* for holding commands, data */
    int             mem_size = 512 * 1000;	/* intial mem we will
						 * use */
    int             p_len;	       /* parameter length */
    static struct ad_struct last_ad;   /* address of the last command
					* read */
    int             class, element;



    pages_done = 0;
    picture_nr = picture;


    /* get the command line */
    /* check options */
    in_fname[0] = '\0';
    mnew_opt[ (int) screen].set = 1;
    to_screen = 1;
#ifndef NO_SIGHT
    sight = interface;
#endif
    if (filename != (char *)0)
	{
	strncpy (mnew_opt[ (int) in_name].val.str, filename, max_str);
	mnew_opt[ (int) in_name].set = 1;
	}
    else
	{
	return (1);
	}

    switch (encoding)
	{
	case 0:
	    {
	    /* char */
	    s_error ("unsupported CGM type");
	    return (1);
	    }
	case 1:
            {
	    /* binary */
	    mnew_opt[ (int) clear_text].set = 0;
	    mnew_opt[ (int) clear_text].val.i = 0;
	    break;
	    }
	case 2:
            {
	    /* clear text */
	    mnew_opt[ (int) clear_text].set = 1;
	    mnew_opt[ (int) clear_text].val.i = 1;
	    break;
	    }
	default:
	    s_error ("unknown CGM type");
	}
	

    if (picture == 0)
	{
	strncpy (mnew_opt[ (int) pages ].val.str, "*\0", max_str);
	mnew_opt[ (int) pages ].set = 0;
	}
    else
	{
	sprintf (mnew_opt[ (int) pages ].val.str, "%d\0", picture);
	   /*
	      ignore warning about embedded \0;
	      here \0 is always at the end of the string,
	      so there is no danger of losing what's beyond \0
	    */
	mnew_opt[ (int) pages ].set = 1;
	}    



    /* now go ask the device what it thinks */
    if (!dev_setup(mnew_opt, &new_info, &mglbl1, &mglbl2, &mglbl3, 
	&mglbl5, mdelim, mmfdesc, mpdesc, mmfctrl, mgprim, mattr, 
	mescfun, mextfun, mctrl))
	{
	exit(0);
	}


    if (mnew_opt[(int) in_name].set)
	strncpy(in_fname, mnew_opt[(int) in_name].val.str, max_str);

    /* open the input file */
    if (!open_input_file(in_fname, open_name, !to_screen,
		  (mnew_opt[(int) clear_text].set) ? ".cgmc" : ".cgm",
		full_oname, &new_info, mnew_opt[(int) clear_text].set,
			 &random_file))
    {
	s_error ("couldn't open your file");
	exit(0);
    }
    /* set the defaults */
    s_defaults(&mglbl1, &mdflt2, &mdflt3, &mdflt5, &mglbl5, 1.0);

    /* now the ongoing loop */

    /* allocate main memory */
    start_mem = (unsigned char *) allocate_mem(mem_size, 0);
    if (!start_mem)
	exit(0);


    /* setup the command interpreters */
    if (mnew_opt[(int) clear_text].set)
    {
	if (!ccgm_setup(full_oname, &new_info,
	mnew_opt, &mglbl1, &mglbl2, &mdflt2, &mglbl3, &mdflt3, &mglbl5,
	     &mdflt5, mdelim, mmfdesc, mpdesc, mmfctrl, mgprim, mattr,
			mescfun, mextfun, mctrl))
	{
	    s_error ("couldn't setup clear text");
	    exit(0);
	} else
	{			       /* setup pointers */
	    get_command = get_clear;
	    do_command = do_clear;
	}
    } else
    {
	if (!cgm_setup(full_oname, &new_info,
	mnew_opt, &mglbl1, &mglbl2, &mdflt2, &mglbl3, &mdflt3, &mglbl5,
	     &mdflt5, mdelim, mmfdesc, mpdesc, mmfctrl, mgprim, mattr,
		       mescfun, mextfun, mctrl, random_file))
	{
	    s_error ("couldn't setup binary");
	    exit(0);
	} else
	{			       /* setup pointers */
	    get_command = get_cmd;
	    do_command = cgm_command;
	}
    }


    /* do the main loop while there is both input and not end_metafile */
    while (((p_len = (*get_command) (&start_mem, &mem_size, &last_ad))
	    >= 0) && (!cli_b_abort) &&
       ((*do_command) (start_mem, p_len, &last_ad, &class, &element)))
    {
	if ((class == 0) && (element == (int) E_Pic))
	{
	    /* just finished page */
	    ++pages_done;
	}
    }

    if (cli_b_abort)
	cli_b_abort = 0;
    else if (p_len < 0)
	s_error ("premature end of file");

    close_up(0);		       /* clean up */
    return (1);			       /* may not get here */

}
