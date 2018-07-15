/*
 *
 * FACILITY:
 *
 *	GLI IMAGE
 *
 * ABSTRACT:
 *
 *	This module contains the display interface for the GLI IMAGE system
 *
 * AUTHOR:
 *
 *	Jochen Werner
 *
 * VERSION:
 *
 *	V1.0-00
 *
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

#if !defined(VMS) && !defined(MSDOS) && !defined(_WIN32)
#include <unistd.h>
#endif

#if defined (cray) || defined (__SVR4) || defined(MSDOS) || defined(_WIN32)
#include <fcntl.h>
#else
#include <sys/file.h>
#endif

#include "system.h"
#include "strlib.h"
#include "image.h"
#include "gksdefs.h"

#define MAX_COLORS	72
#define BOOL		unsigned int
#define BYTE		unsigned char

#define NIL		0
#define TRUE            1
#define FALSE           0

#define Width   512
#define Height  512

#define Recl    80

#define odd(status)	((status) & 01)
#define round(x)	(int)((x)+0.5)

#ifndef max
#define max(a, b)   ((a) > (b) ? (a) : (b))
#endif
#ifndef min
#define min(a, b)   ((a) < (b) ? (a) : (b))
#endif

typedef enum {
   TypeNone, TypeLocal, TypeCrosshair, TypeCross, TypeRubberband,
   TypeRectangle, TypeDigital, TypeCircle} PromptEchoType;


BOOL image_open = FALSE;

static int wkid = 1;
static int conid, wtype;

static int figure_file = 3;

static BOOL init = FALSE;
static BOOL packed_cell_array;
static int first_color, max_colors;

static BOOL cell_array_method = FALSE;
static int image_startx, image_starty;
static int image_sizex, image_sizey;
static float image_x_min, image_x_max;
static float image_y_min, image_y_max;


static
int request_locator (PromptEchoType type, float *x, float *y)
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

    return (state);
}


void ImageRequestRectangle (float *xl, float *yb, float *xr, float *yt)
{
    int errind, tnr, ndc = 0;
    float x0, y0, x1, y1;

    GQCNTN (&errind, &tnr);
    if (tnr != ndc)
	GSELNT (&ndc);

    x0 = 0.0;
    y0 = 0.0;
    request_locator (TypeCrosshair, &x0, &y0);

    x1 = x0;
    y1 = y0;
    request_locator (TypeRectangle, &x1, &y1);

    if (tnr != ndc)
	GSELNT (&tnr);

    *xl = min(x0, x1);
    *xr = max(x0, x1);
    *yb = min(y0, y1);
    *yt = max(y0, y1);
}


void ImageRequestCircle (float *radius)
{
    int errind, tnr, ndc = 0;
    float x0, y0, x1, y1, dx, dy;

    GQCNTN (&errind, &tnr);
    if (tnr != ndc)
	GSELNT (&ndc);

    x0 = x1 = 0.5;
    y0 = y1 = 0.5;
    request_locator (TypeCircle, &x1, &y1);

    dx = x1 - x0;
    dy = y1 - y0;
    *radius = (float) sqrt((double) (dx*dx + dy*dy));

    if (tnr != ndc)
	GSELNT (&tnr);
}


static
void gks_display_pcm (image_dscr *img, int *stat)

/*
 *  gks_display_pcm - display a pcm image using GKS
 */

{
    int count, k;
    int n, state, errind, ol, wkid;
    int conid, wtype, wkcat;
    int ctnr, dx, dy, sx, sy, nx, ny;
    float wn[4], vp[4], r, g, b;
    register int *icarray;
    register BYTE *ccarray;
    register int i, j;

    *stat = img__normal;

    GQOPS (&state);
    if (state >= GWSOP)
        {
        n = 1;
        GQOPWK (&n, &errind, &ol, &wkid);

        for (count = 1; count <= ol; count++)
            {
            n = count;
            GQOPWK (&n, &errind, &ol, &wkid);

	    for (i = 0; i < img->color_t->ncolors; i++)
	        {
	        r = img->color_t->rgb[i].r / 255.;
	        g = img->color_t->rgb[i].g / 255.;
	        b = img->color_t->rgb[i].b / 255.;
                k = i + first_color;
	        GSCR (&wkid, &k, &r, &g, &b);
	        }
            }
        }

    if (!packed_cell_array)
        {
        icarray = (int *) malloc (img->width * img->height * sizeof (int));
        if (icarray != NULL)
            {
            j = img->height * img->width;
            for (i = 0; i < j; i++)
	        icarray[i] = img->data[i] + first_color;
            }
        else
            {
            *stat = img__nomem;
            return;
            }
        }
    else
        {
        ccarray = (BYTE *) malloc (img->width * img->height);
        if (ccarray != NULL)
            {
            j = img->height * img->width;
            for (i = 0; i < j; i++)
	        ccarray[i] = img->data[i] + first_color;
            }
        else
            {
            *stat = img__nomem;
            return;
            }
        }

    dx = img->width;
    dy = img->height;

    if (cell_array_method)
	{
	wn[0] = image_x_min;
	wn[1] = image_x_max;
	wn[2] = image_y_min;
	wn[3] = image_y_max;

	sx = image_startx;
	sy = image_starty;
	nx = image_sizex;
	ny = image_sizey;
	}
    else
	{
	GQCNTN (&errind, &ctnr);
	GQNT (&ctnr, &errind, wn, vp);

	sx = 1;
	sy = 1;
	nx = img->width;
	ny = img->height;
    }

    if (!packed_cell_array)
        {
        GCA (&wn[0], &wn[3], &wn[1], &wn[2], &dx, &dy, &sx, &sy,
	    &nx, &ny, icarray);
        free (icarray);
        }
    else
        {
        GCA (&wn[0], &wn[3], &wn[1], &wn[2], &dx, &dy, &sx, &sy,
	    &nx, &ny, (int *)ccarray);
        free (ccarray);
        }

    if (state >= GWSOP)
        {
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
    


static
void gks_display_pgm (image_dscr *img, int *stat)

/*
 *  gks_display_pgm - display a pgm image using GKS
 */

{
    int ngrays, count, k;
    int n, state, errind, ol, wkid;
    int conid, wtype, wkcat;
    int ctnr, dx, dy, sx, sy, nx, ny;
    float wn[4], vp[4], r, g, b;
    register int *icarray;
    register BYTE *ccarray;
    register float fac;
    register int i, j;

    *stat = img__normal;

    GQOPS (&state);
    if (state >= GWSOP)
        {
        ngrays = min (img->maxgray + 1, max_colors);

        n = 1;
        GQOPWK (&n, &errind, &ol, &wkid);

        for (count = 1; count <= ol; count++)
            {
            n = count;
            GQOPWK (&n, &errind, &ol, &wkid);

	    for (i = 0; i < ngrays; i++)
	        {
	        r = g = b = (float)i / (float)(ngrays - 1);
	        k = i + first_color;
	        GSCR (&wkid, &k, &r, &g, &b);
	        }
            }
        }

    j = img->width * img->height;

    if (!packed_cell_array)
        {
        icarray = (int *) malloc (j * sizeof (int));
        if (icarray != NULL)
            {
            if (img->maxgray + 1 > max_colors)
		{
		fac = 1.0 / (float)(img->maxgray) * (max_colors - 1);
		for (i = 0; i < j; i++)
		    icarray[i] = round (img->data[i] * fac + first_color);
		}
            else
		{
		for (i = 0; i < j; i++)
		    icarray[i] = img->data[i] + first_color;
		}
            }
        else
            {
            *stat = img__nomem;
            return;
            }
        }
    else
        {
        ccarray = (BYTE *) malloc (j);
        if (ccarray != NULL)
            {
            if (img->maxgray + 1 > max_colors)
		{
		fac = 1.0 / (float)(img->maxgray) * (max_colors - 1);
		for (i = 0; i < j; i++)
		    ccarray[i] = round (img->data[i] * fac + first_color);
		}
            else
		{
		for (i = 0; i < j; i++)
		    ccarray[i] = img->data[i] + first_color;
		}
            }
        else
            {
            *stat = img__nomem;
            return;
            }
        }

    dx = img->width;
    dy = img->height;

    if (cell_array_method)
	{
	wn[0] = image_x_min;
	wn[1] = image_x_max;
	wn[2] = image_y_min;
	wn[3] = image_y_max;

	sx = image_startx;
	sy = image_starty;
	nx = image_sizex;
	ny = image_sizey;
	}
    else
	{
	GQCNTN (&errind, &ctnr);
	GQNT (&ctnr, &errind, wn, vp);

	sx = 1;
	sy = 1;
	nx = img->width;
	ny = img->height;
    }

    if (!packed_cell_array)
        {
        GCA (&wn[0], &wn[3], &wn[1], &wn[2], &dx, &dy, &sx, &sy,
	    &nx, &ny, icarray);
        free (icarray);
        }
    else
        {
        GCA (&wn[0], &wn[3], &wn[1], &wn[2], &dx, &dy, &sx, &sy,
	    &nx, &ny, (int *)ccarray);
        free (ccarray);
        }

    if (state >= GWSOP)
        {
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
    
	

static
void display_fft (image_dscr *img, int *stat)

/*
 *  display_fft - display fourier transformed pgm image
 */

{
    image_dscr img_new;
    register float *coeff, *data;
    register int i, j;

    img_create (&img_new, pgm, img->dimx, img->dimy, stat);
    j = img->dimx * img->dimy;

    img_new.data = (BYTE *) malloc (j);
    if (img_new.data != NULL)
	{
	data = (float *) malloc (j * sizeof(float));
	if (data != NULL)
	    {
	    coeff = img->coeff;
	    for (i = 0; i < j; i++)
		{
		data[i] = (float) sqrt((double) (
		    coeff[2*i] * coeff[2*i] + coeff[2*i+1] * coeff[2*i+1]));
		data[i] = 50 * log (data[i] * 512.0 / j + 1);
		img_new.data[i] = (data[i] > 255) ? 255 : (BYTE) data[i];
		}
	    free(data);

	    img_new.maxgray = 255;

	    gks_display_pgm (&img_new, stat);
	    img_free (&img_new);
	    }
	}
    else
	*stat = img__nomem;
}



static
void gks_display_pbm (image_dscr *img, int *stat)

/*
 *  gks_display_pbm - display a pbm image using GKS
 */

{
    int n, state, errind, ol, wkid;
    int conid, wtype, wkcat;
    int count, ctnr, dx, dy, sx, sy, nx, ny;
    float wn[4], vp[4];
    register int i, j;
    register int *icarray;
    register BYTE *ccarray;

    *stat = img__normal;

    if (!packed_cell_array)
        {
        icarray = (int *) malloc (img->width * img->height * sizeof (int));
        if (icarray != NULL)
            {
            j = img->height * img->width;
            for (i = 0; i < j; i++)
                if (img->data[i>>03] & (01 << (7 - i % 8)))
                    icarray[i] = 1;
                else 
                    icarray[i] = 0;
            }
        else
            {
            *stat = img__nomem;
            return;
            }
        }
    else
        {
        ccarray = (BYTE *) malloc (img->width * img->height);
        if (ccarray != NULL)
            {
            j = img->height * img->width;
            for (i = 0; i < j; i++)
                if (img->data[i>>03] & (01 << (7 - i % 8)))
                    ccarray[i] = 1;
                else 
                    ccarray[i] = 0;
            }
        else
            {
            *stat = img__nomem;
            return;
            }
        }

    dx = img->width;
    dy = img->height;

    if (cell_array_method)
	{
	wn[0] = image_x_min;
	wn[1] = image_x_max;
	wn[2] = image_y_min;
	wn[3] = image_y_max;

	sx = image_startx;
	sy = image_starty;
	nx = image_sizex;
	ny = image_sizey;
	}
    else
	{
	GQCNTN (&errind, &ctnr);
	GQNT (&ctnr, &errind, wn, vp);

	sx = 1;
	sy = 1;
	nx = img->width;
	ny = img->height;
    }

    if (!packed_cell_array)
        {
        GCA (&wn[0], &wn[3], &wn[1], &wn[2], &dx, &dy, &sx, &sy,
	    &nx, &ny, icarray);
        free (icarray);
        }
    else
        {
        GCA (&wn[0], &wn[3], &wn[1], &wn[2], &dx, &dy, &sx, &sy,
	    &nx, &ny, (int *)ccarray);
        free (ccarray);
        }

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
    
	

void ImageResize (int width, int height)
{
    float w, h;
    int n, state, errind, ol, wkid;
    int conid, wtype, wkcat;
    int count, dcunit, lx, ly;
    float rx, ry, x0, x1, y0, y1;

    GQOPS (&state);
    if (state >= GWSAC)
        {
        w = width; h = height;

        n = 1;
        GQACWK (&n, &errind, &ol, &wkid);

        for (count = 1; count <= ol; count++)
            {
            n = count;
            GQACWK (&n, &errind, &ol, &wkid);

	    GQWKC (&wkid, &errind, &conid, &wtype);
	    GQWKCA (&wtype, &errind, &wkcat);

            if (wkcat == GOUTPT || wkcat == GOUTIN)
                {
                GQWKC (&wkid, &errind, &conid, &wtype);
            
                switch (wtype) {
    
                    case 41:
                    case 210:
                    case 211:
                    case 212:
                    case 213:
                    case 214:
                    case 215:
                    case 216:
                    case 217:
                    case 230:
                    case 231:
                    case 232:
                    case 233:
                        GQDSP (&wtype, &errind, &dcunit, &rx, &ry, &lx, &ly);
                        if (dcunit == GMETRE)
	                    {
	                    x0 = 0; x1 = rx*w/lx;
	                    y0 = 0; y1 = ry*h/ly;
	                    GSWKVP (&wkid, &x0, &x1, &y0, &y1);
	                    }
                        break;

                    default:
                        GQDSP (&wtype, &errind, &dcunit, &rx, &ry, &lx, &ly);
                        if (ry/rx > (float)height/(float)width)
                            {
                            x0 = 0; x1 = rx;
                            y0 = 0; y1 = rx * height / width;
                            }
                        else
                            {
                            x0 = 0; x1 = ry * width / height;
                            y0 = 0; y1 = ry;
                            }
                        GSWKVP (&wkid, &x0, &x1, &y0, &y1);
                        break;
                    }

                GUWK (&wkid, &GPOSTP);
                }
            }
        }
}



void img_display (image_dscr *img, display_mode mode, int *status)

/*
 *  img_display - display an image on the specified output device
 */

{
    int stat, errind, tnr, ndc = 0;
    image_dscr img_new;

    stat = img__normal;
    GQCNTN (&errind, &tnr);

    if (!init) {
        packed_cell_array = (char *) getenv ("GLI_GKS_PACKED_CELL_ARRAY") ?
            TRUE : FALSE;

        first_color = (char *) getenv ("GLI_IMG_DIRECT_COLOR") ? 0 : 8;
        max_colors = MAX_COLORS + 8 - first_color;

        init = TRUE;
        }

    switch (mode) {

	case normal:
	    break;

	case pixel:
            ImageResize (img->width, img->height);
	    GSELNT (&ndc);
	    break;

        case NDC:
            GSELNT (&ndc);
            break;

	default:
	    stat = img__invmode;
	    break;
	}

    if (odd (stat))
	{
	switch (img->type) {

	    case ppm:
		img_colorquant (img, &img_new, max_colors, &stat);
		if (!odd (stat)) break;

		gks_display_pcm (&img_new, &stat);
		img_free (&img_new);
		break;

	    case pcm:
		if (img->color_t->ncolors > max_colors)
		    {
		    img_colorquant (img, &img_new, max_colors, &stat);
		    if (!odd (stat)) break;

		    gks_display_pcm (&img_new, &stat);
		    img_free (&img_new);
		    }
		else
		    {
		    gks_display_pcm (img, &stat);
		    }
		break;

	    case pgm:
		if (img->coeff == NULL)
		    {
		    gks_display_pgm (img, &stat);
		    }
		else
		    {
		    display_fft (img, &stat);
		    }
		break;

	    case pbm:
		gks_display_pbm (img, &stat);
		break;
	    }
	}

    GSELNT (&tnr);

    if (status != NIL)
	*status = stat;
    else
	if (!odd (stat))
	    raise_exception (stat, 0, NULL, NULL);
}



void img_cell_array (image_dscr *img, int *startx, int *starty, int *sizex,
    int *sizey, float *x_min, float *x_max, float *y_min, float *y_max,
    int *status)

/*
 *  img_cell_array - display a cell array
 */

{
    int stat, errind, tnr, ndc = 0;
    image_dscr img_new;
    float ratio, img_ratio, delta, center;

    stat = img__normal;
    GQCNTN (&errind, &tnr);

    if (!init) {
        packed_cell_array = (char *) getenv ("GLI_GKS_PACKED_CELL_ARRAY") ?
            TRUE : FALSE;

        first_color = (char *) getenv ("GLI_IMG_DIRECT_COLOR") ? 0 : 8;
        max_colors = MAX_COLORS + 8 - first_color;

        init = TRUE;
        }

    cell_array_method = TRUE;

    if (*startx > 0 && *startx < img->width)
	image_startx = *startx;
    else
	*startx = image_startx = 1;

    if (*sizex > 0 && *sizex <= img->width - image_startx + 1)
	image_sizex = *sizex;
    else
	*sizex = image_sizex = img->width - image_startx + 1;

    if (*starty > 0 && *starty < img->height)
	image_starty = *starty;
    else
	*starty = image_starty = 1;

    if (*sizey > 0 && *sizey <= img->height - image_starty + 1)
	image_sizey = *sizey;
    else
	*sizey = image_sizey = img->height - image_starty + 1;

    image_x_min = *x_min;
    image_x_max = *x_max;
    image_y_min = *y_min;
    image_y_max = *y_max;

    ratio = (image_x_max - image_x_min) / (image_y_max - image_y_min);
    img_ratio = (float)*sizex / (float)*sizey;

    if (ratio - img_ratio > 0.001)
	{
	center = 0.5 * (image_x_max + image_x_min);
	delta = 0.5 * img_ratio / ratio * (image_x_max - image_x_min);
	*x_min = center - delta;
	*x_max = center + delta;
	image_x_min = *x_min;
	image_x_max = *x_max;
	}
    else if (ratio - img_ratio < 0.001)
	{
	center = 0.5 * (image_y_max + image_y_min);
	delta = 0.5 * ratio / img_ratio * (image_y_max - image_y_min);
	*y_min = center - delta;
	*y_max = center + delta;
	image_y_min = *y_min;
	image_y_max = *y_max;
	}

    if (tnr != ndc)
	GSELNT (&ndc);

    switch (img->type) {

	case ppm:
	    img_colorquant (img, &img_new, max_colors, &stat);
	    if (!odd (stat)) break;

	    gks_display_pcm (&img_new, &stat);
	    img_free (&img_new);
	    break;

	case pcm:
	    if (img->color_t->ncolors > max_colors)
		{
		img_colorquant (img, &img_new, max_colors, &stat);
		if (!odd (stat)) break;

		gks_display_pcm (&img_new, &stat);
		img_free (&img_new);
		}
	    else
		{
		gks_display_pcm (img, &stat);
		}
	    break;

	case pgm:
	    gks_display_pgm (img, &stat);
	    break;

	case pbm:
	    gks_display_pbm (img, &stat);
	    break;
	}

    GSELNT (&tnr);

    cell_array_method = FALSE;

    if (status != NIL)
	*status = stat;
    else
	if (!odd (stat))
	    raise_exception (stat, 0, NULL, NULL);
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
	    if (wtype >= 210 && wtype <= 212)
		return wkid;
	    }
	}

    return 1;
}


void ImageOpen (int *widget)
{
    int errind;
    int dcunit, lx, ly;
    float rx, ry, x0, x1, y0, y1;
 
    wkid = find_x_display ();

    if (widget) {
	GQWKC (&wkid, &errind, &conid, &wtype);
	if (errind == 0) {
	    GDAWK (&wkid);
	    GCLWK (&wkid);
	    }
	GOPWK (&wkid, widget, &wtype);
	GACWK (&wkid);

	GQDSP (&wtype, &errind, &dcunit, &rx, &ry, &lx, &ly);
	if (dcunit == GMETRE)
	    {
	    x0 = 0; x1 = rx*Width/lx;
	    y0 = 0; y1 = ry*Height/ly;
	    GSWKVP (&wkid, &x0, &x1, &y0, &y1);
	    }

        GCLRWK (&wkid, &GALWAY);
	GUWK (&wkid, &GPOSTP);
	}

    if (image_open) return;

    image_open = TRUE;
}


void ImageClose (int *widget)
{
    if (!image_open) return;

    if (widget) 
        {
	GDAWK (&wkid);
	GCLWK (&wkid);
	GOPWK (&wkid, &conid, &wtype);
	GACWK (&wkid);
	}
    else 
        {
	image_open = FALSE;
	}
}


int _ImageOpenFigureFile (char *filename, int epsf)
{
    int conid, wstype = 62; /* Color PostScript */

    if ((conid = open (filename, O_CREAT | O_TRUNC | O_WRONLY, 0644)) >= 0)
	{
	GOPWK (&figure_file, &conid, &wstype);
	GACWK (&figure_file);
	}

    return (conid);
}


void _ImageCloseFigureFile (void)
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



void _ImageDisplay (char *name, int *status)

/*
 *  img_display - display an image
 */

{
    image_dscr img;
    int errind, saved_tnr, tnr = 1;
    float wn[4], vp[4];
    float x0, x1, y0, y1, size;
    int stat;

    img_translate (name, &img, &stat);

    if (odd (stat))
	{
	GQCNTN (&errind, &saved_tnr);
	GQNT (&tnr, &errind, wn, vp);

	GSELNT (&tnr);

	x0 = 0; x1 = 1;
	y0 = 0; y1 = 1;
	GSVP (&tnr, &x0, &x1, &y0, &y1);

	if (img.width > img.height)
	    {
	    size = 0.5 * (float) (img.height) / img.width;
	    x0 = 0;		 x1 = 1;
	    y0 = 0.5 - size; y1 = 0.5 + size;
	    }
	else if (img.width < img.height)
	    {
	    size = 0.5 * (float) (img.width) / img.height;
	    x0 = 0.5 - size; x1 = 0.5 + size;
	    y0 = 0;		 y1 = 1;
	    }
	GSWN (&tnr, &x0, &x1, &y0, &y1);

	img_display (&img, normal, &stat);

	GSWN (&tnr, &wn[0], &wn[1], &wn[2], &wn[3]);
	GSVP (&tnr, &vp[0], &vp[1], &vp[2], &vp[3]);
	GSELNT (&saved_tnr);
	}

    if (status != NIL)
	*status = stat;
    else
	if (!odd (stat))
	    raise_exception (stat, 0, NULL, NULL);
}
