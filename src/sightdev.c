/*
 *
 * FACILITY:
 *
 *	Graphics Language Interpreter (GLI)
 *
 * ABSTRACT:
 *
 *	This module contains a Simple Interactive Graphics Handling Tool
 *	(Sight) logical device handler for X Window displays.
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
#include <sys/file.h>
#endif

#ifdef VMS
#include <descrip.h>
static struct dsc$descriptor_s descr;
#endif

#ifdef cray
#include <fortran.h>
static _fcd descr;
#endif

#include "gksdefs.h"
#include "gus.h"
#include "symbol.h"
#include "image.h"
#include "sight.h"


#ifndef min
#define min(x1,x2)	(((x1)<(x2)) ? (x1) : (x2))
#endif
#ifndef max
#define max(x1,x2)	(((x1)>(x2)) ? (x1) : (x2))
#endif
#ifndef odd
#define odd(status)	    ((status) & 01)
#endif

#define Bool	int
#define True	1
#define False	0
#define Nil	0
#define Recl	80

#define Width	686
#define Height	485

#define EPS                 1.0e-5

typedef enum {
   TypeNone, TypeLocal, TypeCrosshair, TypeCross, TypeRubberband,
   TypeRectangle, TypeDigital, TypeCircle} PromptEchoType;


extern SightGC gc;
extern SightAnyGC DashedGC;
extern SightAnyGC SnapGC;
extern SightAnyGC PointGC;
extern Bool sight_open, sight_gui_open;
extern int sight_orientation;
extern int sight_dpi;
extern float snap_grid;

#ifdef TCL
extern char *sight_info_cb;
#endif

static int wkid = 1;
static int conid, wtype;
static float w = Width, h = Height;

static int figure_file = 3;
static int image_count = 0;


static SightLineGC saved_lngc, lngc;
static SightMarkerGC saved_mkgc, mkgc;
static SightFillGC saved_fagc, fagc;
static SightTextGC saved_txgc, txgc;
static int saved_clsw;
static int saved_tnr;
static int saved_scale;
static int cur_prec;
static float saved_wn[4], saved_vp[4];

static float chux[] = {0, -1, 0, 1};
static float chuy[] = {1, 0, -1, 0};




void _SightSetWindow (int tnr, float *wn, float *vp)
    {
	int t_nr;

    if (tnr != SightNDC)
	{
	t_nr = tnr;
	GSVP (&t_nr, &vp[0], &vp[1], &vp[2], &vp[3]);
	GSWN (&t_nr, &wn[0], &wn[1], &wn[2], &wn[3]);
	}
    }


void _SightChangeXform (int xformno)
{
	int tnr, scale;

    if (xformno == 0)
	{
	tnr = SightNDC;
	scale = 0;
	}
    else
	{
	tnr = SightWC;
	scale = gc.xform[xformno].scale;
	_SightSetWindow (tnr, gc.xform[xformno].wn, gc.xform[xformno].vp);
	}
	
    GSELNT (&tnr);
    gus_set_scale (&scale, Nil);
}



void _SightApplyXform (int tnr, float x0, float y0, float *x1, float *y1)
    {
    *x1 = x0;   
    *y1 = y0;
    if (tnr != SightNDC)
	{
	_SightChangeXform (tnr);
	gus_apply_xform (&tnr, x1, y1);
	}
    }



void _SightApplyInverseXform (int tnr, float x0, float y0, float *x1, float *y1)
    {
    *x1 = x0;
    *y1 = y0;
    if (tnr != SightNDC)
	{
	_SightChangeXform (tnr);
	gus_apply_inverse_xform (x1, y1);
	}
    }



void _SightDrawSnapGrid (int n, float *x, float *y)
    {
	int np;
	
    if (n > 0)
	{
	np = n;
	GPM (&np, x, y);
	}
    }



void _SightDrawBorder (float x0, float y0, float x1, float y1)
{
    int n;
    float x[5], y[5];

    n = 5;
    x[0] = x0; y[0] = y0;
    x[1] = x1; y[1] = y0;
    x[2] = x1; y[2] = y1;
    x[3] = x0; y[3] = y1;
    x[4] = x0; y[4] = y0;
    
    gus_polyline (&n, x, y, Nil);
}



void _SightDrawPoint (float x, float y)
{
    int n;
    float px, py;

    n = 1;
    px = x;
    py = y;
    
    gus_polymarker (&n, &px, &py, Nil);
}


void _SightChangeGC (SightObjectType type, SightAnyGC *gc)
{
    switch (type) {
	case Pline :
	case Spline :
	    if (lngc.linetype != gc->line.linetype) {
		GSLN (&gc->line.linetype);
		lngc.linetype = gc->line.linetype;
		}
	    if (lngc.linewidth != gc->line.linewidth) {
		GSLWSC (&gc->line.linewidth);
		lngc.linewidth = gc->line.linewidth;
		}
	    if (lngc.linecolor != gc->line.linecolor) {
		GSPLCI (&gc->line.linecolor);
		lngc.linecolor = gc->line.linecolor;
		}
	    break;

	case Pmarker :
	    if (mkgc.markertype != gc->marker.markertype) {
		GSMK (&gc->marker.markertype);
		mkgc.markertype =  gc->marker.markertype;
		}
	    if (mkgc.markersize !=  gc->marker.markersize) {
		GSMKSC (&gc->marker.markersize);
		mkgc.markersize =  gc->marker.markersize;
		}
	    if (mkgc.markercolor !=  gc->marker.markercolor) {
		GSPMCI (&gc->marker.markercolor);
		mkgc.markercolor =  gc->marker.markercolor;
		}
	    break;

	case ErrorBars :
	    if (lngc.linetype != GLSOLI) {
		GSLN (&GLSOLI);
		lngc.linetype = GLSOLI;
		}
	    if (lngc.linewidth != gc->bar.linewidth) {
		GSLWSC (&gc->bar.linewidth);
		lngc.linewidth = gc->bar.linewidth;
		}
	    if (lngc.linecolor != gc->bar.linecolor) {
		GSPLCI (&gc->bar.linecolor);
		lngc.linecolor = gc->bar.linecolor;
		}
	    if (mkgc.markertype != gc->bar.markertype) {
		GSMK (&gc->bar.markertype);
		mkgc.markertype =  gc->bar.markertype;
		}
	    if (mkgc.markersize !=  gc->bar.markersize) {
		GSMKSC (&gc->bar.markersize);
		mkgc.markersize =  gc->bar.markersize;
		}
	    if (mkgc.markercolor !=  gc->bar.markercolor) {
		GSPMCI (&gc->bar.markercolor);
		mkgc.markercolor =  gc->bar.markercolor;
		}
	    break;

	case FillArea :
	case BarGraph :
	    if (fagc.fillstyle !=  gc->fill.fillstyle) {
		GSFAIS (&gc->fill.fillstyle);
		fagc.fillstyle =  gc->fill.fillstyle;
		}
	    if (fagc.fillindex !=  gc->fill.fillindex) {
		GSFASI (&gc->fill.fillindex);
		fagc.fillindex =  gc->fill.fillindex;
		}
	    if (fagc.fillcolor !=  gc->fill.fillcolor) {
		GSFACI (&gc->fill.fillcolor);
		fagc.fillcolor =  gc->fill.fillcolor;
		}
	    break;

	case SimpleText :
	case Text :
	    if (txgc.textfont != gc->text.textfont) {
		GSTXFP (&gc->text.textfont, &cur_prec);
		txgc.textfont = gc->text.textfont;
		}
	    if (txgc.textsize != gc->text.textsize) {
		GSCHH (&gc->text.textsize);
		txgc.textsize = gc->text.textsize;
		}
	    if (txgc.texthalign != gc->text.texthalign) {
		GSTXAL (&gc->text.texthalign, &txgc.textvalign);
		txgc.texthalign = gc->text.texthalign;
		}
	    if (txgc.textvalign != gc->text.textvalign) {
		GSTXAL (&txgc.texthalign, &gc->text.textvalign);
		txgc.textvalign = gc->text.textvalign;
		}
	    if (txgc.textdirection != gc->text.textdirection) {
		GSCHUP (&chux[gc->text.textdirection],
			&chuy[gc->text.textdirection]);
		txgc.textdirection = gc->text.textdirection;
		}
	    if (txgc.textcolor != gc->text.textcolor) {
		GSTXCI (&gc->text.textcolor);
		txgc.textcolor = gc->text.textcolor;
		}
	    break;

	case Axes :
	case Grid :
	    if (lngc.linewidth != gc->axes.linewidth) {
		GSLWSC (&gc->axes.linewidth);
		lngc.linewidth = gc->axes.linewidth;
		}
	    if (lngc.linecolor != gc->axes.linecolor) {
		GSPLCI (&gc->axes.linecolor);
		lngc.linecolor = gc->axes.linecolor;
		}
	    if (txgc.textfont != gc->axes.textfont) {
		GSTXFP (&gc->axes.textfont, &cur_prec);
		txgc.textfont = gc->axes.textfont;
		}
	    if (txgc.textsize != gc->axes.textsize) {
		GSCHH (&gc->axes.textsize);
		txgc.textsize = gc->axes.textsize;
		}
	    if (txgc.textcolor != gc->axes.textcolor) {
		GSTXCI (&gc->axes.textcolor);
		txgc.textcolor = gc->axes.textcolor;
		}
	    break;
	}
}


void _SightChangeClipping (int clip)
{
	int clsw;

    clsw = clip;
    GSCLIP (&clsw);
}



void _SightDisplayObject (SightDisplayList *p, unsigned *obj, int *status)
{
	SightPoint *data;
	SightSpline *spline;
	SightBar *bar;
	SightString *text;
	SightAxes *axes;
	SightImage *image;
	int m, nchars;
	float width = 0;
	float *wn, x_min, y_min;
	int options;

    switch (p->type) {

	case Pline :
	    data = (SightPoint *) obj;
	    *status = gus_polyline (&data->n, data->x, data->y, Nil);
	    break;

	case Spline :
	    spline = (SightSpline *) obj;
	    if (spline->n < 64)
		m = 256;
	    else 
		m = 1024;
	    *status = gus_spline (&spline->n, spline->x, spline->y, &m, 
		&spline->smooth, Nil);
	    break;

	case Pmarker :
	    data = (SightPoint *) obj;
	    *status = gus_polymarker (&data->n, data->x, data->y, Nil);
	    break;

	case ErrorBars :
	    bar = (SightBar *) obj;
	    switch (bar->orientation) {
		case SightVerticalBar :
		    *status = gus_vertical_error_bars (&bar->n, bar->x, bar->y, 
			bar->e1, bar->e2, Nil);
		    break;
		case SightHorizontalBar :
		    *status = gus_horizontal_error_bars (&bar->n, bar->x, 
			bar->y, bar->e1, bar->e2, Nil);
		    break;
		}
	    break;

	case FillArea : 
	    data = (SightPoint *) obj;
	    *status = gus_fill_area (&data->n, data->x, data->y, Nil);
	    break;

	case BarGraph : 
	    data = (SightPoint *) obj;
	    *status = gus_bar_graph (&data->n, data->x, data->y, &width, Nil);
	    break;

	case SimpleText :
	    text = (SightString *) obj;
	    nchars = strlen(text->chars);
	    gks_text_s (&text->x, &text->y, &nchars, text->chars);
	    break;

	case Text :
	    text = (SightString *) obj;
	    *status = gus_text (&text->x, &text->y, text->chars, Nil);
	    break;

	case Axes :
	    axes = (SightAxes *) obj;
	    *status = gus_axes (&axes->x_tick, &axes->y_tick,
		&axes->x_org, &axes->y_org, &axes->maj_x, &axes->maj_y,
		&axes->tick_size, Nil);

	    wn = gc.xform[p->tnr].wn;
	    options = gc.xform[p->tnr].scale;

	    x_min = option_flip_x & options ? wn[1] : wn[0];
	    y_min = option_flip_y & options ? wn[3] : wn[2];

	    if (fabs(axes->x_org - x_min) < EPS &&
		fabs(axes->y_org - y_min) < EPS && sight_gui_open)
		{
		    float x_org, y_org, tick_size;
		    int maj_x, maj_y;

		x_org = option_flip_x & options ? wn[0] : wn[1];
		y_org = option_flip_y & options ? wn[2] : wn[3];
		maj_x = -abs(axes->maj_x);
		maj_y = -abs(axes->maj_y);
		tick_size = -axes->tick_size;

		*status = gus_axes (&axes->x_tick, &axes->y_tick,
		    &x_org, &y_org, &maj_x, &maj_y, &tick_size, Nil);
		}
	    break;

	case Grid :
	    axes = (SightAxes *) obj;
	    *status = gus_grid (&axes->x_tick, &axes->y_tick,
		&axes->x_org, &axes->y_org, &axes->maj_x, &axes->maj_y, Nil);
	    break;

	case Image :
	    image = (SightImage *) obj;

    	    if (image->img == NULL) {
		sprintf(image->name, "SightImage%d", ++image_count);

    	    	image->img = (image_dscr *) malloc(sizeof(image_dscr));
    	    	img_read(image->filename, image->img, NULL, status);

    	    	if (odd(*status))
    	    	    img_define(image->name, image->img, status);
    	    }
       	    else
		*status = img__normal;

    	    if (odd(*status))    	    	
    	    	img_cell_array(image->img, &image->startx, &image->starty,
		    &image->sizex, &image->sizey, &image->x_min, &image->x_max,
		    &image->y_min, &image->y_max, status); 
    	    break;
	}
}



void _SightSaveXform (void)
{
    int errind;

    GQCNTN (&errind, &saved_tnr);
    GQNT (&saved_tnr, &errind, saved_wn, saved_vp);

    gus_inq_scale (&saved_scale, Nil);
}


void _SightRestoreXform (void)
{
    GSELNT (&saved_tnr);
    _SightSetWindow (saved_tnr, saved_wn, saved_vp);
    gus_set_scale (&saved_scale, Nil);
}



void _SightSaveClipping (void)
{
    int errind;
    float clip_rec[4];

    GQCLIP (&errind, &saved_clsw, clip_rec);
}



void _SightRestoreClipping (void)
{
    GSCLIP (&saved_clsw);
}



void _SightSaveGC (void)
{
    int errind;

    GQLN (&errind, &lngc.linetype);
    GQLWSC (&errind, &lngc.linewidth); 
    GQPLCI (&errind, &lngc.linecolor);
 
    GQMK (&errind, &mkgc.markertype);
    GQMKSC (&errind, &mkgc.markersize);
    GQPMCI (&errind, &mkgc.markercolor);
 
    GQFAIS (&errind, &fagc.fillstyle);
    GQFASI (&errind, &fagc.fillindex);
    GQFACI (&errind, &fagc.fillcolor);
    
    GQTXFP (&errind, &txgc.textfont, &cur_prec);
    GQCHH (&errind, &txgc.textsize);
    GQTXAL (&errind, &txgc.texthalign, &txgc.textvalign);
    GQTXCI (&errind, &txgc.textcolor);

    saved_lngc = lngc;
    saved_mkgc = mkgc;
    saved_fagc = fagc;
    saved_txgc = txgc;
}


void _SightRestoreGC (void)
{
    SightAnyGC gc;

    gc.line = saved_lngc;
    _SightChangeGC (Pline, &gc);

    gc.marker = saved_mkgc;
    _SightChangeGC (Pmarker, &gc);

    gc.fill = saved_fagc;
    _SightChangeGC (FillArea, &gc);

    gc.text = saved_txgc;
    _SightChangeGC (Text, &gc);
    }


void _SightInit (void)
{
    int i;
    Bool landscape = (sight_orientation == OrientationLandscape);

    _SightSaveXform ();
    _SightSaveClipping ();

    gc.clsw = saved_clsw;
    gc.tnr = saved_tnr;
    gc.update = UpdateAlways;

    for (i = 0; i < MAX_XFORMS; i++)
	{
	gc.xform[i].wn[0] = 0.0;
	gc.xform[i].wn[1] = 1.0;
	gc.xform[i].wn[2] = 0.0;
	gc.xform[i].wn[3] = 1.0;
	gc.xform[i].vp[0] = 0.125;
	gc.xform[i].vp[1] = landscape ? 0.9 : 0.675;
	gc.xform[i].vp[2] = 0.125;
	gc.xform[i].vp[3] = landscape ? 0.675 : 0.9;
	gc.xform[i].scale = 0;
	}
    for (i = 0; i < 4; i++)
	gc.xform[SightWC].wn[i] = saved_wn[i]; 

    gc.xform[SightWC].scale = saved_scale; 

    _SightSaveGC ();

    gc.line = lngc;
    gc.marker = mkgc;
    gc.fill = fagc;
    gc.text = txgc;

    gc.bar.linewidth = lngc.linewidth;
    gc.bar.linecolor = lngc.linecolor;
    gc.bar.markertype = mkgc.markertype;
    gc.bar.markersize = mkgc.markersize;
    gc.bar.markercolor = mkgc.markercolor;

    gc.axes.linewidth = lngc.linewidth;
    gc.axes.linecolor = lngc.linecolor;
    gc.axes.textfont = txgc.textfont;
    gc.axes.textsize = txgc.textsize;
    gc.axes.textcolor = txgc.textcolor;

    DashedGC.line.linetype = GLDASH;
    DashedGC.line.linewidth = 1;
    DashedGC.line.linecolor = CBlack;

    SnapGC.marker.markertype = GPOINT;
    SnapGC.marker.markersize = 1;
    SnapGC.marker.markercolor = CBlack;
    
    PointGC.marker.markertype = GOMARK;
    PointGC.marker.markersize = 1;
    PointGC.marker.markercolor = CBlack;
}


static
int find_x_display (void)
{
    int state, i, n, errind, ol, wkid, conid, wtype;

    GQOPS (&state);
    if (state >= GWSOP)
        {
        i = 1;
        GQOPWK (&i, &errind, &ol, &wkid);
        for (i = ol; i >= 1; i--)
            {
            n = i;
            GQOPWK (&n, &errind, &ol, &wkid);
	    GQWKC (&wkid, &errind, &conid, &wtype);
	    if (wtype == 41 || (wtype >= 210 && wtype <= 212))
		return wkid;
	    }
	}

    return 1;
}


void SightOpen (int *widget)
{
    int errind;
    char *env;
#ifdef TCL
    static char result[256];
    int stat;
#endif

    wkid = find_x_display ();

    if (env = (char *) getenv("GLI_GKS_DPI"))
        sight_dpi = (atoi(env) == 100) ? 100 : 75;
 
    if (widget) {
	GQWKC (&wkid, &errind, &conid, &wtype);
	if (errind == 0) {
	    GDAWK (&wkid);
	    GCLWK (&wkid);
	    }
	GOPWK (&wkid, widget, &wtype);
	GACWK (&wkid);
	}

    if (sight_open) return;

#ifdef TCL
    sym_translate ("sight_info_cb", result, &stat);
    if (odd(stat))
	sight_info_cb = result;
    else
	sight_info_cb = NULL;
#endif

    _SightInit ();

    sight_open = True;
    image_count = 0;
}


static
int request_locator (PromptEchoType type, float *x, float *y, Bool snap)
{
    int lcdnr, tnr, pet, ld, state;
    float px, py, x0, x1, y0, y1;
    char ldr[Recl];

    lcdnr = 1;
    tnr = 0;
    px = *x; py = *y;
    pet = (int) type;
    x0 = 0; x1 = 1;
    y0 = 0; y1 = 1;
    ld = 1;

    GINLC (&wkid, &lcdnr, &tnr, &px, &py, &pet, &x0, &x1, &y0, &y1, &ld, ldr,
	Recl);
    GRQLC (&wkid, &lcdnr, &state, &tnr, x, y);

    if (snap_grid != 0.0 && snap)
	{
	*x = (int)(*x/snap_grid+0.5)*snap_grid;
	*y = (int)(*y/snap_grid+0.5)*snap_grid;
	}

    return (state);
}



void SightRequestLocatorNDC (float *x, float *y, Bool snap)
{
    *x = 0.0;
    *y = 0.0;

    request_locator (TypeCrosshair, x, y, snap);
}



void SightRequestLocatorWC (float *x, float *y, int snap)
{
    *x = 0.0;
    *y = 0.0;

    request_locator (TypeCrosshair, x, y, snap);

    _SightSaveXform ();
    _SightApplyInverseXform (gc.tnr, *x, *y, x, y);
    _SightRestoreXform ();
}



void _SightUpdate (void)
{
    GUWK (&wkid, &GPOSTP);
}



void SightRequestStroke (int *n, float *x, float *y)
{
    float px, py;
    int state, n2, np;

    if (!sight_open)
	SightOpen (Nil);

    _SightSaveXform ();
    _SightSaveClipping ();

    _SightChangeXform (gc.tnr);
    _SightChangeClipping (gc.clsw);
    
    np = 0;
    px = 0.0;
    py = 0.0;

    state = request_locator (TypeCrosshair, &px, &py, True);

    if (state == GOK) {
	if (gc.tnr == SightNDC) {
	    *x++ = px; *y++ = py;
	    }
	else {
	    _SightApplyInverseXform (gc.tnr, px, py, x, y);
	    x++; y++;
	    }
	
	_SightChangeGC (Pline, (SightAnyGC *)&gc.line);

	state = request_locator (TypeRubberband, &px, &py, True);

	n2 = 2;
	np = 1;

	while (np < *n && state == GOK)
	    {
	    if (gc.tnr == SightNDC) {
		*x++ = px; *y++ = py;
		}
	    else {
		_SightApplyInverseXform (gc.tnr, px, py, x, y);
		x++; y++;
		}
	    np++;

	    gus_polyline (&n2, x-2, y-2, &state);
	    _SightUpdate ();

	    state = request_locator (TypeRubberband, &px, &py, True);
	    }
	}

    *n = np;

    _SightRestoreClipping ();
    _SightRestoreXform ();
}


void SightRequestMarker (int *n, float *x, float *y)
{
    float px, py;
    int state, n1, np;

    if (!sight_open)
	SightOpen (Nil);

    _SightSaveXform ();
    _SightSaveClipping ();

    _SightChangeXform (gc.tnr);
    _SightChangeClipping (gc.clsw);
    _SightChangeGC (Pmarker, (SightAnyGC *)&gc.marker);

    px = 0.0;
    py = 0.0;

    state = request_locator (TypeCrosshair, &px, &py, True);

    n1 = 1;
    np = 0;

    while (np < *n && state == GOK)
	{
	if (gc.tnr == SightNDC) {
	    *x++ = px; *y++ = py;
	    }
	else {
	    _SightApplyInverseXform (gc.tnr, px, py, x, y);
	    x++; y++;
	    }
	np++;

	gus_polymarker (&n1, x-1, y-1, &state);
	_SightUpdate ();

	state = request_locator (TypeCrosshair, &px, &py, True);
	}

    *n = np;

    _SightRestoreClipping ();
    _SightRestoreXform ();
}


void SightSampleLocator (float *x, float *y, int snap)
{
    float x0, y0, x1, y1;

    x0 = 0.0;
    y0 = 0.0;

    request_locator (TypeCrosshair, &x0, &y0, snap);

    x1 = x0;
    y1 = y0;
    request_locator (TypeRubberband, &x1, &y1, snap);

    *x = x1-x0;
    *y = y1-y0;
}


void SightRequestRectangle (int tnr, float *xl, float *yb, float *xr, float *yt,
    int snap)
{
    float x0, y0, x1, y1;
    
    x0 = 0.0;
    y0 = 0.0;

    request_locator (TypeCrosshair, &x0, &y0, snap);

    x1 = x0;
    y1 = y0;

    request_locator (TypeRectangle, &x1, &y1, snap);

    *xl = min(x0, x1);
    *xr = max(x0, x1);
    *yb = min(y0, y1);
    *yt = max(y0, y1);

    if (tnr != SightNDC)
	{
	_SightSaveXform ();

	_SightApplyInverseXform (tnr, *xl, *yb, xl, yb);
	_SightApplyInverseXform (tnr, *xr, *yt, xr, yt);

	_SightRestoreXform ();
	}
}


void SightRequestString (char *chars)
{
    int lcdnr, state, n;
#if defined(VMS) || defined(cray)
    char str[256];
#endif

    lcdnr = 1;

#ifdef VMS
    descr.dsc$b_dtype = DSC$K_DTYPE_T;
    descr.dsc$b_class = DSC$K_CLASS_S;
    descr.dsc$w_length = 256;
    descr.dsc$a_pointer = str;

    GRQST (&wkid, &lcdnr, &state, &n, &descr);

    strncpy (chars, str, n);
#else
#ifdef cray
    descr = _cptofcd(str, 256);

    GRQST (&wkid, &lcdnr, &state, &n, descr);

    strncpy (chars, str, n);
#else
    GRQST (&wkid, &lcdnr, &state, &n, chars, 256);
#endif /* cray */
#endif /* VMS */

    chars[n] = '\0';
}


void _SightInquireTextExtent (SightString *text, SightTextGC *attribute,
    float *xmin, float *ymin, float *xmax, float *ymax)
{
    float extx[4], exty[4];
    int errind, i, precision;
    SightTextGC saved_gc;

    /* save text attributes */
    GQTXFP (&errind, &saved_gc.textfont, &precision);
    GQCHH (&errind, &saved_gc.textsize);
    GQTXAL (&errind, &saved_gc.texthalign, &saved_gc.textvalign);
    GQTXCI (&errind, &saved_gc.textcolor);

    /* set current text attributes */
    GSTXFP (&attribute->textfont, &precision);
    GSCHH (&attribute->textsize);
    GSTXAL (&attribute->texthalign, &attribute->textvalign);
    GSCHUP (&chux[attribute->textdirection], &chuy[attribute->textdirection]);
    GSTXCI (&attribute->textcolor);

    _SightSaveXform ();
    _SightChangeXform (SightNDC);

    gus_inq_text_extent_routine (&text->x, &text->y, text->chars, extx, exty,
	Nil);

    _SightRestoreXform ();

    /* restore saved text attributes */
    GSTXFP (&saved_gc.textfont, &precision);
    GSCHH (&saved_gc.textsize);
    GSTXAL (&saved_gc.texthalign, &saved_gc.textvalign);
    GSTXCI (&saved_gc.textcolor);

    *xmin = extx[0];
    *xmax = extx[0];
    *ymin = exty[0];
    *ymax = exty[0];

    for (i = 1; i < 4; i++) {
	*xmin = min(*xmin, extx[i]);
	*xmax = max(*xmax, extx[i]);
	*ymin = min(*ymin, exty[i]);
	*ymax = max(*ymax, exty[i]);
	}
}


void SightClose (int *widget)
{
    if (!sight_open) return;

    if (widget) {
	GDAWK (&wkid);
	GCLWK (&wkid);
	GOPWK (&wkid, &conid, &wtype);
	GACWK (&wkid);
	}
    else {
	SightClear ();
	sight_open = False;
	}
}


void SightResize (int width, int height)
{
    int errind, conid, wtype;
    int dcunit, lx, ly;
    float rx, ry, x0, x1, y0, y1;

    w = width; h = height;

    GQWKC (&wkid, &errind, &conid, &wtype);
    GQDSP (&wtype, &errind, &dcunit, &rx, &ry, &lx, &ly);
    if (dcunit == GMETRE)
	{
	x0 = 0; x1 = rx*w/lx;
	y0 = 0; y1 = ry*h/ly;
	GSWKVP (&wkid, &x0, &x1, &y0, &y1);

        if (x1 > y1) {
            y1 /= x1; x1 = 1;
            }
        else {
            x1 /= y1; y1 = 1;
            }
	GSWKWN (&wkid, &x0, &x1, &y0, &y1);
	}

    GUWK (&wkid, &GPOSTP);

    SightRedraw (Nil);
}


int _SightOpenFigureFile (char *filename, int epsf)
{
    int conid, wstype = 62; /* Color PostScript */
    float x0, x1, y0, y1;

    if ((conid = open (filename, O_CREAT | O_TRUNC | O_WRONLY, 0644)) >= 0)
        {
        GOPWK (&figure_file, &conid, &wstype);
	GACWK (&figure_file);

        x0 = y0 = 0;
        if (!epsf) {
       	    if (w > h) {
                x1 = 0.2694; y1 = 0.1905;
                }
            else {
                x1 = 0.1905; y1 = 0.2694;
                }
            }
        else {
       	    if (w > h) {
		x1 = 0.1905; y1 = 0.1347;
                }
            else {
		x1 = 0.1347; y1 = 0.1905;
                }
            }
	GSWKVP (&figure_file, &x0, &x1, &y0, &y1);

        if (w > h) {
            x1 = 1; y1 = 1/sqrt(2.0);
            }
        else {
            y1 = 1; x1 = 1/sqrt(2.0);
            }
	GSWKWN (&figure_file, &x0, &x1, &y0, &y1);
	}

    return (conid);
}


void _SightCloseFigureFile (void)
{
    int errind, conid, wtype;

    GQWKC (&figure_file, &errind, &conid, &wtype);

    if (errind == 0)
        {
	GDAWK (&figure_file);
	GCLWK (&figure_file);

	close (conid);
	}
}


void _SightClearDisplay (void)
{
    int state;

    GQOPS (&state);
    if (state == GSGOP)
        GCLSG ();

    GCLRWK (&wkid, &GALWAY);
}
