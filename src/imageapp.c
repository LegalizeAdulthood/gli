/*
 *
 * FACILITY:
 *
 *	GLI IMAGE
 *
 * ABSTRACT:
 *
 *	This module contains the application functions for the GLI IMAGE
 *      system
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


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "system.h"
#include "strlib.h"
#include "mathlib.h"
#include "image.h"

#ifndef PI
#define PI		3.14159265358979323846
#endif
#define NIL		0

#define BYTE		unsigned char
#define ULONG		unsigned long

#define MAX_COLORS	256

#define odd(status)	((status) & 01)
#define round(x)	(int)((x)+0.5)

#ifndef max
#define max(a, b)   ((a) > (b) ? (a) : (b))
#endif
#ifndef min
#define min(a, b)   ((a) < (b) ? (a) : (b))
#endif


static BYTE filter_tab[256];
static hls_color hls_filter_tab[256];


void img_shearx (image_dscr *img_old, image_dscr *img_new, double angle,
    int antialias, int *status)

/*
 *  img_shearx - shear an image in x direction by the specified angle
 */

{
    image_dscr img;
    register int i, j, k, l, n;
    register int skewi, nwidth;
    register double shearfac;
    register double skew, skewf;
    register BYTE pixel, left, oleft;
    rgb_color rgb_pixel, rgb_left, rgb_oleft;
    int stat;

    stat = img__normal;
 
    if (angle < -60 || angle > 60)
	stat = img__invangle;
    else
	{
	shearfac = fabs (tan (angle * PI / 180.0));

	nwidth = (int)ceil ((double)(img_old->height * fabs (shearfac) + 
	    img_old->width));

	switch (img_old->type) {
	    
	    case ppm:
		img_create (img_new, ppm, nwidth, img_old->height, &stat);
		if (!odd (stat)) break;

		for (i = 0; i < img_old->height; i++)
		    {
		    if (angle > 0)
			{
			skew = i * shearfac;
			}
		    else
			{
			skew = (img_old->height - i - 1) * shearfac;
			}
		    skewi = (int)skew;

		    for (j = 0; j < skewi; j++)
			{
			k = 3 * (j + nwidth * i);
			img_new->data[k] = 0;
			img_new->data[k+1] = 0;
			img_new->data[k+2] = 0;
			}

		    if (antialias)
			{
			skewf = skew - skewi;
			rgb_oleft.r = 0;
			rgb_oleft.g = 0;
			rgb_oleft.b = 0;

                        l = skewi + img_old->width;
			for (j = skewi; j < l; j++)
			    {
			    k = 3 * (j - skewi + img_old->width * i);
			    rgb_pixel.r = img_old->data[k];
			    rgb_pixel.g = img_old->data[k+1];
			    rgb_pixel.b = img_old->data[k+2];
			    rgb_left.r = (ULONG)(rgb_pixel.r * skewf);
			    rgb_left.g = (ULONG)(rgb_pixel.g * skewf);
			    rgb_left.b = (ULONG)(rgb_pixel.b * skewf);
			    rgb_pixel.r = rgb_pixel.r - rgb_left.r + 
				rgb_oleft.r;
			    rgb_pixel.g = rgb_pixel.g - rgb_left.g + 
				rgb_oleft.g;
			    rgb_pixel.b = rgb_pixel.b - rgb_left.b + 
				rgb_oleft.b;
			    k = 3 * (j + nwidth * i);
			    img_new->data[k] = rgb_pixel.r; 
			    img_new->data[k+1] = rgb_pixel.g; 
			    img_new->data[k+2] = rgb_pixel.b; 
			    rgb_oleft = rgb_left;
			    }
			}
		    else
			{
                        n = skewi + img_old->width;
			for (j = skewi; j < n; j++)
			    {
			    k = 3 * (j + nwidth * i);
			    l = 3 * (j - skewi + img_old->width * i);
			    img_new->data[k] = img_old->data[l];
			    img_new->data[k+1] = img_old->data[l+1];
			    img_new->data[k+2] = img_old->data[l+2];
			    }
			}

                    n = skewi + img_old->width;
		    for (j = n; j < nwidth; j++)
			{
			k = 3 * (j + nwidth * i);
			img_new->data[k] = 0;
			img_new->data[k+1] = 0;
			img_new->data[k+2] = 0;
			}
		    }
		break;

	    case pcm:
		if (antialias)
		    {
		    img_ppm (img_old, &img, &stat);
		    if (!odd (stat)) break;
		    img_shearx (&img, img_new, angle, antialias, &stat);
		    img_free (&img);
		    }
		else
		    {
		    img_create (img_new, pcm, nwidth, img_old->height, &stat);
		    if (!odd (stat)) break;

		    for (i = 0; i < img_old->color_t->ncolors; i++)
			img_new->color_t->rgb[i] = img_old->color_t->rgb[i];
		    img_new->color_t->ncolors = img_old->color_t->ncolors;

		    for (i = 0; i < img_old->height; i++)
			{
			if (angle > 0)
			    {
			    skew = i * shearfac;
			    }
			else
			    {
			    skew = (img_old->height - i - 1) * shearfac;
			    }
			skewi = (int)skew;

			for (j = 0; j < skewi; j++)
			    img_new->data[j + nwidth * i] = 0;

                        n = skewi + img_old->width;
			for (j = skewi; j < n; j++)
			    img_new->data[j + nwidth * i] = 
				img_old->data[j - skewi + img_old->width * i];

                        n = skewi + img_old->width;
			for (j = n; j < nwidth; j++)
			    img_new->data[j + nwidth * i] = 0;
			}
		    }
		break;

	    case pgm:
		img_create (img_new, pgm, nwidth, img_old->height, &stat);
		if (!odd (stat)) break;

		img_new->maxgray = img_old->maxgray;

		for (i = 0; i < img_old->height; i++)
		    {
		    if (angle > 0)
			{
			skew = i * shearfac;
			}
		    else
			{
			skew = (img_old->height - i - 1) * shearfac;
			}
		    skewi = (int)skew;

		    for (j = 0; j < skewi; j++)
			img_new->data[j + nwidth * i] = 0;

		    if (antialias)
			{
			skewf = skew - skewi;
			oleft = 0;

                        n = skewi + img_old->width;
			for (j = skewi; j < n; j++)
			    {
			    pixel = img_old->data[j - skewi + 
				img_old->width * i];
			    left = (BYTE)(pixel * skewf);
			    pixel = pixel - left + oleft;
			    img_new->data[j + nwidth * i] = pixel;
			    oleft = left;
			    }
			}
		    else
			{
                        n = skewi + img_old->width;
			for (j = skewi; j < n; j++)
			    img_new->data[j + nwidth * i] = 
				img_old->data[j - skewi + img_old->width * i];
			}

                    n = skewi + img_old->width;
		    for (j = n; j < nwidth; j++)
			img_new->data[j + nwidth * i] = 0;
		    }
		break;

	    case pbm:
		if (antialias)
		    {
		    img_pgm (img_old, &img, 255, &stat);
		    if (!odd (stat)) break;
		    img_shearx (&img, img_new, angle, antialias, &stat);
		    img_free (&img);
		    }
		else
		    {
		    img_create (img_new, pbm, nwidth, img_old->height, &stat);
		    if (!odd (stat)) break;

		    for (i = 0; i < img_old->height; i++)
			{
			if (angle > 0)
			    {
			    skew = i * shearfac;
			    }
			else
			    {
			    skew = (img_old->height - i - 1) * shearfac;
			    }
			skewi = (int)skew;

                        n = skewi + img_old->width;
			for (j = skewi; j < n; j++)
			    {
			    k = j + nwidth * i;
			    l = j - skewi + img_old->width * i;
			    if (img_old->data[l>>03] & (01 << (7 - l % 8)))
				img_new->data[k>>03] |= (01 << (7 - k % 8));
			    }
			}
		    }
		break;
	    
	    default:
		stat = img__invimgtyp;
		break;
	    }
	}

    if (status != NIL)
	*status = stat;
    else
	if (!odd (stat))
	    raise_exception (stat, 0, NULL, NULL);
}



void img_sheary (image_dscr *img_old, image_dscr *img_new, double angle,
    int antialias, int *status)

/*
 *  img_sheary - shear an image in y direction by the specified angle
 */

{
    image_dscr img;
    register int i, j, k, l, n;
    register int skewi, nheight;
    register double shearfac;
    register double skew, skewf;
    register double pixel, left, oleft;
    rgb_color rgb_pixel, rgb_left, rgb_oleft;
    int stat;

    stat = img__normal;
 
    if (angle < -60 || angle > 60)
	stat = img__invangle;
    else
	{
	shearfac = fabs (tan (angle * PI / 180.0));

	nheight = (int)ceil ((double)(img_old->width * fabs (shearfac) + 
	    img_old->height));
	    
	switch (img_old->type) {
	    
	    case ppm:
		img_create (img_new, ppm, img_old->width, nheight, &stat);
		if (!odd (stat)) break;

		for (j = 0; j < img_old->width; j++)
		    {
		    if (angle > 0)
			{
			skew = j * shearfac;
			}
		    else
			{
			skew = (img_old->width - j - 1) * shearfac;
			}
		    skewi = (int)skew;

		    for (i = 0; i < skewi; i++)
			{
			k = 3 * (j + img_old->width * i);
			img_new->data[k] = 0;
			img_new->data[k+1] = 0;
			img_new->data[k+2] = 0;
			}

		    if (antialias)
			{
			skewf = skew - skewi;
			rgb_oleft.r = 0;
			rgb_oleft.g = 0;
			rgb_oleft.b = 0;

                        n = skewi + img_old->height;
			for (i = skewi; i < n; i++)
			    {
			    k = 3 * (j + img_old->width * (i - skewi));
			    rgb_pixel.r = img_old->data[k];
			    rgb_pixel.g = img_old->data[k+1];
			    rgb_pixel.b = img_old->data[k+2];
			    rgb_left.r = (ULONG)(rgb_pixel.r * skewf);
			    rgb_left.g = (ULONG)(rgb_pixel.g * skewf);
			    rgb_left.b = (ULONG)(rgb_pixel.b * skewf);
			    rgb_pixel.r = rgb_pixel.r - rgb_left.r + 
				rgb_oleft.r;
			    rgb_pixel.g = rgb_pixel.g - rgb_left.g + 
				rgb_oleft.g;
			    rgb_pixel.b = rgb_pixel.b - rgb_left.b + 
				rgb_oleft.b;
			    k = 3 * (j + img_old->width * i);
			    img_new->data[k] = rgb_pixel.r; 
			    img_new->data[k+1] = rgb_pixel.g; 
			    img_new->data[k+2] = rgb_pixel.b; 
			    rgb_oleft = rgb_left;
			    }
			}
		    else
			{
                        n = skewi + img_old->height;
			for (i = skewi; i < n; i++)
			    {
			    k = 3 * (j + img_old->width * i);
			    l = 3 * (j + img_old->width * (i - skewi));
			    img_new->data[k] = img_old->data[l];
			    img_new->data[k+1] = img_old->data[l+1];
			    img_new->data[k+2] = img_old->data[l+2];
			    }
			}

                    n = skewi + img_old->height;
		    for (i = n; i < nheight; i++)
			{
			k = 3 * (j + img_old->width * i);
			img_new->data[k] = 0;
			img_new->data[k+1] = 0;
			img_new->data[k+2] = 0;
			}
		    }
		break;

	    case pcm:
		if (antialias)
		    {
		    img_ppm (img_old, &img, &stat);
		    if (!odd (stat)) break;
		    img_sheary (&img, img_new, angle, antialias, &stat);
		    img_free (&img);
		    }
		else
		    {
		    img_create (img_new, pcm, img_old->width, nheight, &stat);
		    if (!odd (stat)) break;
		    for (i = 0; i < img_old->color_t->ncolors; i++)
			img_new->color_t->rgb[i] = img_old->color_t->rgb[i];
		    img_new->color_t->ncolors = img_old->color_t->ncolors;

		    for (j = 0; j < img_old->width; j++)
			{
			if (angle > 0)
			    {
			    skew = j * shearfac;
			    }
			else
			    {
			    skew = (img_old->width - j - 1) * shearfac;
			    }
			skewi = (int)skew;

			for (i = 0; i < skewi; i++)
			    img_new->data[j + img_old->width * i] = 0;

                        n = skewi + img_old->height;
			for (i = skewi; i < n; i++)
			    img_new->data[j + img_old->width * i] = 
				img_old->data[j + img_old->width * (i - skewi)];

                        n = skewi + img_old->height;
			for (i = n; i < nheight; i++)
			    img_new->data[j + img_old->width * i] = 0;
			}
		    }
		break;

	    case pgm:
		img_create (img_new, pgm, img_old->width, nheight, &stat);
		if (!odd (stat)) break;
		img_new->maxgray = img_old->maxgray;

		for (j = 0; j < img_old->width; j++)
		    {
		    if (angle > 0)
			{
			skew = j * shearfac;
			}
		    else
			{
			skew = (img_old->width - j - 1) * shearfac;
			}
		    skewi = (int)skew;

		    for (i = 0; i < skewi; i++)
			img_new->data[j + img_old->width * i] = 0;

		    if (antialias)
			{
			skewf = skew - skewi;
			oleft = 0;

                        n = skewi + img_old->height;
			for (i = skewi; i < n; i++)
			    {
			    pixel = img_old->data[j + img_old->width * (i - 
				skewi)];
			    left = pixel * skewf;
			    pixel = pixel - left + oleft;
			    img_new->data[j + img_old->width * i] = (BYTE)pixel;
			    oleft = left;
			    }
			}
		    else
			{
                        n = skewi + img_old->height;
			for (i = skewi; i < n; i++)
			    img_new->data[j + img_old->width * i] = 
				img_old->data[j + img_old->width * (i - skewi)];
			}

                    n = skewi + img_old->height;
		    for (i = n; i < nheight; i++)
			img_new->data[j + img_old->width * i] = 0;
		    }
		break;

	    case pbm:
		if (antialias)
		    {
		    img_pgm (img_old, &img, 255, &stat);
		    if (!odd (stat)) break;
		    img_sheary (&img, img_new, angle, antialias, &stat);
		    img_free (&img);
		    }
		else
		    {
		    img_create (img_new, pbm, img_old->width, nheight, &stat);
		    if (!odd (stat)) break;

		    for (j = 0; j < img_old->width; j++)
			{
			if (angle > 0)
			    {
			    skew = j * shearfac;
			    }
			else
			    {
			    skew = (img_old->width - j - 1) * shearfac;
			    }
			skewi = (int)skew;

                        n = skewi + img_old->height;
			for (i = skewi; i < n; i++)
			    {
			    k = j + img_old->width * i;
			    l = j + img_old->width * (i - skewi);
			    if (img_old->data[l>>03] & (01 << (7 - l % 8)))
				img_new->data[k>>03] |= (01 << (7 - k % 8));
			    }
			}
		    }
		break;

	    default:
		stat = img__invimgtyp;
		break;
	    }
	}

    if (status != NIL)
	*status = stat;
    else
	if (!odd (stat))
	    raise_exception (stat, 0, NULL, NULL);
}



void img_rot (image_dscr *img_old, image_dscr *img_new, int angle, int *status)

/*
 *  img_rot - rotate an image in mathematical order by 90, 180 or 270 degree 
 */

{
    register int i, j, k, l;
    int stat;

    stat = img__normal;

    switch (img_old->type) {

	case ppm:
	    switch (angle) {
	    
		case 270:
		    img_create (img_new, img_old->type, img_old->height, 
			img_old->width, &stat);
		    if (!odd (stat)) break;
    
		    for (i = 0; i < img_old->height; i++)
			for (j = 0; j < img_old->width; j++)
			    {
			    k = 3 * (j * img_new->width + img_new->width - i - 
				1);
			    l = 3 * (i * img_old->width + j);
			    img_new->data[k] = img_old->data[l];
			    img_new->data[k+1] = img_old->data[l+1];
			    img_new->data[k+2] = img_old->data[l+2];
			    }
		    break;

		case 180:
		    img_create (img_new, img_old->type, img_old->width,
			img_old->height, &stat);
		    if (!odd (stat)) break;

		    for (i = 0; i < img_old->height; i++)
			for (j = 0; j < img_old->width; j++)
			    {
			    k = 3 * ((img_new->height - i - 1) * 
				img_new->width + img_new->width - j - 1);
			    l = 3 * (i * img_old->width + j);
			    img_new->data[k] = img_old->data[l];
			    img_new->data[k+1] = img_old->data[l+1];
			    img_new->data[k+2] = img_old->data[l+2];
			    }
		    break;

		case 90:
		    img_create (img_new, img_old->type, img_old->height, 
			img_old->width, &stat);
		    if (!odd (stat)) break;

		    for (i = 0; i < img_old->height; i++)
			for (j = 0; j < img_old->width; j++)
			    {
			    k = 3 * ((img_new->height - j - 1) * 
				img_new->width + i);
			    l = 3 * (i * img_old->width + j);
			    img_new->data[k] = img_old->data[l];
			    img_new->data[k+1] = img_old->data[l+1];
			    img_new->data[k+2] = img_old->data[l+2];
			    }
		    break;
	    
		default:
		    stat = img__invangle;
		    break;
		}
	    break;

	case pcm:
	    switch (angle) {
	    
		case 270:
		    img_create (img_new, img_old->type, img_old->height, 
			img_old->width, &stat);
		    if (!odd (stat)) break;

		    img_new->color_t->ncolors = img_old->color_t->ncolors;
		    for (i = 0; i < img_new->color_t->ncolors; i++)
			img_new->color_t->rgb[i] = img_old->color_t->rgb[i];

		    for (i = 0; i < img_old->height; i++)
			for (j = 0; j < img_old->width; j++)
			    {
			    k = j * img_new->width + img_new->width - i - 1;
			    l = i * img_old->width + j;
			    img_new->data[k] = img_old->data[l];
			    }
		    break;

		case 180:
		    img_create (img_new, img_old->type, img_old->width,
			img_old->height, &stat);
		    if (!odd (stat)) break;

		    img_new->color_t->ncolors = img_old->color_t->ncolors;
		    for (i = 0; i < img_new->color_t->ncolors; i++)
			img_new->color_t->rgb[i] = img_old->color_t->rgb[i];

		    for (i = 0; i < img_old->height; i++)
			for (j = 0; j < img_old->width; j++)
			    {
			    k = (img_new->height - i - 1) * 
				img_new->width + img_new->width - j - 1;
			    l = i * img_old->width + j;
			    img_new->data[k] = img_old->data[l];
			    }
		    break;

		case 90:
		    img_create (img_new, img_old->type, img_old->height, 
			img_old->width, &stat);
		    if (!odd (stat)) break;

		    img_new->color_t->ncolors = img_old->color_t->ncolors;
		    for (i = 0; i < img_new->color_t->ncolors; i++)
			img_new->color_t->rgb[i] = img_old->color_t->rgb[i];

		    for (i = 0; i < img_old->height; i++)
			for (j = 0; j < img_old->width; j++)
			    {
			    k = (img_new->height - j - 1) * img_new->width + i;
			    l = i * img_old->width + j;
			    img_new->data[k] = img_old->data[l];
			    }
		    break;

		default:
		    stat = img__invangle;
		    break;
		}
	    break;

	case pgm:
	    switch (angle) {
	    
		case 270:
		    img_create (img_new, img_old->type, img_old->height, 
			img_old->width, &stat);
		    if (!odd (stat)) break;

		    img_new->maxgray = img_old->maxgray;

		    for (i = 0; i < img_old->height; i++)
			for (j = 0; j < img_old->width; j++)
			    {
			    k = j * img_new->width + img_new->width - i - 1;
			    l = i * img_old->width + j;
			    img_new->data[k] = img_old->data[l];
			    }
		    break;

		case 180:
		    img_create (img_new, img_old->type, img_old->width,
			img_old->height, &stat);
		    if (!odd (stat)) break;
			
		    img_new->maxgray = img_old->maxgray;

		    for (i = 0; i < img_old->height; i++)
			for (j = 0; j < img_old->width; j++)
			    {
			    k = (img_new->height - i - 1) * 
				img_new->width + img_new->width - j - 1;
			    l = i * img_old->width + j;
			    img_new->data[k] = img_old->data[l];
			    }
		    break;

		case 90:
		    img_create (img_new, img_old->type, img_old->height, 
			img_old->width, &stat);
		    if (!odd (stat)) break;

		    img_new->maxgray = img_old->maxgray;

		    for (i = 0; i < img_old->height; i++)
			for (j = 0; j < img_old->width; j++)
			    {
			    k = (img_new->height - j - 1) * img_new->width + i;
			    l = i * img_old->width + j;
			    img_new->data[k] = img_old->data[l];
			    }
		    break;
	
		default:
		    stat = img__invangle;
		    break;
		}
	    break;

	case pbm:
	    switch (angle) {
	    
		case 270:
		    img_create (img_new, img_old->type, img_old->height, 
			img_old->width, &stat);
		    if (!odd (stat)) break;

		    for (i = 0; i < img_old->height; i++)
			for (j = 0; j < img_old->width; j++)
			    {
			    k = j * img_new->width + img_new->width - i - 1;
			    l = i * img_old->width + j;
			    if (img_old->data[l>>03] & (01 << (7 - l % 8)))
				img_new->data[k>>03] |= (01 << (7 - k % 8));
			    }
		    break;

		case 180:
		    img_create (img_new, img_old->type, img_old->width,
			img_old->height, &stat);
		    if (!odd (stat)) break;

		    for (i = 0; i < img_old->height; i++)
			for (j = 0; j < img_old->width; j++)
			    {
			    k = (img_new->height - i - 1) * img_new->width + 
				img_new->width - j - 1;
			    l = i * img_old->width + j;
			    if (img_old->data[l>>03] & (01 << (7 - l % 8)))
				img_new->data[k>>03] |= (01 << (7 - k % 8));
			    }
		    break;

		case 90:
		    img_create (img_new, img_old->type, img_old->height, 
			img_old->width, &stat);
		    if (!odd (stat)) break;

		    for (i = 0; i < img_old->height; i++)
			for (j = 0; j < img_old->width; j++)
			    {
			    k = (img_new->height - j - 1) * img_new->width + i;
			    l = i * img_old->width + j;
			    if (img_old->data[l>>03] & (01 << (7 - l % 8)))
				img_new->data[k>>03] |= (01 << (7 - k % 8));
			    }
		    break;

		default:
		    stat = img__invangle;
		    break;
		}
	    break;

	default:
	    stat = img__invangle;
	    break;
	}

    if (status != NIL)
	*status = stat;
    else
	if (!odd (stat))
	    raise_exception (stat, 0, NULL, NULL);
}



void img_rotate (image_dscr *img_old, image_dscr *img_new, double angle,
    int antialias, int *status)

/*
 *  img_rotate - rotate an image in mathematical order by an arbitary angle
 *               between 0.0 and 360.0
 */

{
    image_dscr img_h1, img_h2, img;
    register double x_shear, y_shear;
    register int i, j, k, l, m, n;
    register int tmp_width, x_shear_junk, y_shear_junk;
    register int new_height, new_width;
    register double skew, skewf;
    register int skewi;
    register double pixel, left, oleft;
    rgb_color rgb_pixel, rgb_left, rgb_oleft;
    int stat;

    stat = img__normal;

    if (angle == 360.) angle = 0.;

    if (angle < 0 || angle > 360)
	stat = img__invangle;
    else
	{
	if (angle >= 270)
	    {
	    img_rot (img_old, &img, 270, &stat);

	    if (odd (stat))
		{
		if (angle == 270)
		    {
		    *img_new = img;
		    }
		else
		    {
		    img_rotate (&img, img_new, angle - 270, antialias, &stat);
		    img_free (&img);
		    }
		}
	    return;
	    }
	else if (angle >= 180)
	    {
	    img_rot (img_old, &img, 180, &stat);

	    if (odd (stat))
		{
		if (angle == 180)
		    {
		    *img_new = img;
		    }
		else
		    {
		    img_rotate (&img, img_new, angle - 180, antialias, &stat);
		    img_free (&img);
		    }
		}
	    return;
	    }
	else if (angle >= 90)
	    {
	    img_rot (img_old, &img, 90, &stat);

	    if (odd (stat))
		{
		if (angle == 90)
		    {
		    *img_new = img;
		    }
		else
		    {
		    img_rotate (&img, img_new, angle - 90, antialias, &stat);
		    img_free (&img);
		    }
		return;
		}
	    }

        if (angle == 0.)
            img_copy(img_old, img_new, &stat);

	if (odd (stat) && angle != 0.)
	    {
	    x_shear = fabs (tan (angle * PI / 360.0));
	    y_shear = fabs (sin (angle * PI / 180.0));

	    tmp_width = (int)ceil ((double)(img_old->height * x_shear + 
		img_old->width));
	    y_shear_junk = (int)((tmp_width - img_old->width) * y_shear);
	    new_height = (int)ceil (tmp_width * y_shear + img_old->height);
	    x_shear_junk = (int)((new_height - img_old->height - 
		y_shear_junk) * x_shear);
	    new_height = new_height - 2 * y_shear_junk;
	    new_width = (int)ceil (new_height * x_shear + tmp_width - 2 * 
		x_shear_junk);

	    switch (img_old->type) {
		
		case ppm:
		    img_create (&img_h1, ppm, tmp_width, img_old->height, 
			&stat);
		    if (!odd (stat)) break;

		    for (i = 0; i < img_old->height; i++)
			{
			skew = i * x_shear;
			skewi = (int)skew;

			for (j = 0; j < skewi; j++)
			    {
			    l = 3 * (j + tmp_width * i);
			    img_h1.data[l] = 0;
			    img_h1.data[l+1] = 0;
			    img_h1.data[l+2] = 0;
			    }

			if (antialias)
			    {
			    skewf = skew - skewi;
			    rgb_oleft.r = 0;
			    rgb_oleft.g = 0;
			    rgb_oleft.b = 0;

                            n = skewi + img_old->width;
			    for (j = skewi; j < n; j++)
				{
				m = 3 * (j - skewi + img_old->width * i);
				rgb_pixel.r = img_old->data[m];
				rgb_pixel.g = img_old->data[m+1];
				rgb_pixel.b = img_old->data[m+2];
				rgb_left.r = (ULONG)(rgb_pixel.r * skewf);
				rgb_left.g = (ULONG)(rgb_pixel.g * skewf);
				rgb_left.b = (ULONG)(rgb_pixel.b * skewf);
				rgb_pixel.r = rgb_pixel.r - rgb_left.r + 
				    rgb_oleft.r;
				rgb_pixel.g = rgb_pixel.g - rgb_left.g + 
				    rgb_oleft.g;
				rgb_pixel.b = rgb_pixel.b - rgb_left.b + 
				    rgb_oleft.b;
				l = 3 * (j + tmp_width * i);
				img_h1.data[l] = rgb_pixel.r;
				img_h1.data[l+1] = rgb_pixel.g;
				img_h1.data[l+2] = rgb_pixel.b;
				rgb_oleft = rgb_left;
				}
			    }
			else
			    {
                            n = skewi + img_old->width;
			    for (j = skewi; j < n; j++)
				{
				m = 3 * (j - skewi + img_old->width * i);
				l = 3 * (j + tmp_width * i);
				img_h1.data[l] = img_old->data[m];
				img_h1.data[l+1] = img_old->data[m+1];
				img_h1.data[l+2] = img_old->data[m+2];
				}
			    }

                        n = skewi + img_old->width;
			for (j = n; j < tmp_width; j++)
			    {
			    l = 3 * (j + tmp_width * i);
			    img_h1.data[l] = 0;
			    img_h1.data[l+1] = 0;
			    img_h1.data[l+2] = 0;
			    }
			}

		    img_create (&img_h2, ppm, tmp_width, new_height, &stat);
		    if (!odd (stat)) 
                        {
                        img_free (&img_h1);
                        break;
                        }

		    for (j = 0; j < tmp_width; j++)
			{
			skew = (tmp_width - j) * y_shear;
			skewi = (int)skew;
			skewf = skew - skewi;
			skewi = skewi - y_shear_junk;

			for (i = 0; i < new_height; i++)
			    {
			    l = 3 * (j + tmp_width * i);
			    img_h2.data[l] = 0;
			    img_h2.data[l+1] = 0;
			    img_h2.data[l+2] = 0;
			    }

			if (antialias)
			    {
			    rgb_oleft.r = 0;
			    rgb_oleft.g = 0;
			    rgb_oleft.b = 0;


			    for (i = 0; i < img_old->height; i++)
				{
				k = i + skewi;
				if (k >= 0 && k < new_height)
				    {
				    m = 3 * (j + tmp_width * i);
				    rgb_pixel.r = img_h1.data[m];
				    rgb_pixel.g = img_h1.data[m+1];
				    rgb_pixel.b = img_h1.data[m+2];
				    rgb_left.r = (ULONG)(rgb_pixel.r * skewf);
				    rgb_left.g = (ULONG)(rgb_pixel.g * skewf);
				    rgb_left.b = (ULONG)(rgb_pixel.b * skewf);
				    rgb_pixel.r = rgb_pixel.r - rgb_left.r + 
					rgb_oleft.r;
				    rgb_pixel.g = rgb_pixel.g - rgb_left.g + 
					rgb_oleft.g;
				    rgb_pixel.b = rgb_pixel.b - rgb_left.b + 
					rgb_oleft.b;
				    l = 3 * (j + tmp_width * k);
				    img_h2.data[l] = rgb_pixel.r;
				    img_h2.data[l+1] = rgb_pixel.g;
				    img_h2.data[l+2] = rgb_pixel.b;
				    rgb_oleft = rgb_left;
				    }
				}
			    }
			else
			    {
			    for (i = 0; i < img_old->height; i++)
				{
				k = i + skewi;
				if (k >= 0 && k < new_height)
				    {
				    m = 3 * (j + tmp_width * i);
				    l = 3 * (j + tmp_width * k);
				    img_h2.data[l] = img_h1.data[m];
				    img_h2.data[l+1] = img_h1.data[m+1];
				    img_h2.data[l+2] = img_h1.data[m+2];
				    }
				}
			    }
			}

		    img_create (img_new, ppm, new_width, new_height, &stat);
		    if (!odd (stat)) 
                        {
                        img_free (&img_h2);
                        img_free (&img_h1);
                        break;
                        }

		    for (i = 0; i < new_height; i++)
			{
			skew = i * x_shear;
			skewi = (int)skew;
			skewf = skew - skewi;
			skewi = skewi - x_shear_junk;

			for (j = 0; j < new_width; j++)
			    {
			    l = 3 * (j + new_width * i);
			    img_new->data[l] = 0;
			    img_new->data[l+1] = 0;
			    img_new->data[l+2] = 0;
			    }

			if (antialias)
			    {
			    rgb_oleft.r = 0;
			    rgb_oleft.g = 0;
			    rgb_oleft.b = 0;

			    for (j = 0; j < tmp_width; j++)
				{
				k = j + skewi;
				if ( k >= 0 && k < new_width)
				    {
				    m = 3 * (j + tmp_width * i);
				    rgb_pixel.r = img_h2.data[m];
				    rgb_pixel.g = img_h2.data[m+1];
				    rgb_pixel.b = img_h2.data[m+2];
				    rgb_left.r = (ULONG)(rgb_pixel.r * skewf);
				    rgb_left.g = (ULONG)(rgb_pixel.g * skewf);
				    rgb_left.b = (ULONG)(rgb_pixel.b * skewf);
				    rgb_pixel.r = rgb_pixel.r - rgb_left.r + 
					rgb_oleft.r;
				    rgb_pixel.g = rgb_pixel.g - rgb_left.g + 
					rgb_oleft.g;
				    rgb_pixel.b = rgb_pixel.b - rgb_left.b + 
					rgb_oleft.b;
				    l = 3 * (k + new_width * i);
				    img_new->data[l] = rgb_pixel.r;
				    img_new->data[l+1] = rgb_pixel.g;
				    img_new->data[l+2] = rgb_pixel.b;
				    rgb_oleft = rgb_left;
				    }
				}
			    }
			else
			    {
			    for (j = 0; j < tmp_width; j++)
				{
				k = j + skewi;
				if ( k >= 0 && k < new_width)
				    {
				    l = 3 * (k + new_width * i);
				    m = 3 * (j + tmp_width * i);
				    img_new->data[l] = img_h2.data[m];
				    img_new->data[l+1] = img_h2.data[m+1];
				    img_new->data[l+2] = img_h2.data[m+2];
				    }
				}
			    }
			}

		    img_free (&img_h2);
		    img_free (&img_h1);
		    break;

		case pcm:
		    if (antialias)
			{
			img_ppm (img_old, &img, &stat);
			if (!odd (stat)) break;

			img_rotate (&img, img_new, angle, antialias, &stat);
			if (!odd (stat)) break;

			img_free (&img);
			}
		    else
			{
			img_create (&img_h1, pcm, tmp_width, img_old->height,
			    &stat);
			if (!odd (stat)) break;

			for (i = 0; i < img_old->color_t->ncolors; i++)
			    img_h1.color_t->rgb[i] = img_old->color_t->rgb[i];
			img_h1.color_t->ncolors = img_old->color_t->ncolors;

			for (i = 0; i < img_old->height; i++)
			    {
			    skew = i * x_shear;
			    skewi = (int)skew;

			    for (j = 0; j < skewi; j++)
				img_h1.data[j + tmp_width * i] = 0;

                            n = skewi + img_old->width;
			    for (j = skewi; j < n; j++)
				img_h1.data[j + tmp_width * i] = 
				    img_old->data[j - skewi + 
				    img_old->width * i];

                            n = skewi + img_old->width;
			    for (j = n; j < tmp_width; j++)
				img_h1.data[j + tmp_width * i] = 0;
			    }

			img_create (&img_h2, pcm, tmp_width, new_height, &stat);
			if (!odd (stat)) 
                            {
                            img_free (&img_h1);
                            break;
                            }

			for (i = 0; i < img_old->color_t->ncolors; i++)
			    img_h2.color_t->rgb[i] = img_old->color_t->rgb[i];
			img_h2.color_t->ncolors = img_old->color_t->ncolors;

			for (j = 0; j < tmp_width; j++)
			    {
			    skew = (tmp_width - j) * y_shear;
			    skewi = (int)skew;
			    skewf = skew - skewi;
			    skewi = skewi - y_shear_junk;

			    for (i = 0; i < new_height; i++)
				img_h2.data[j + tmp_width * i] = 0;

			    for (i = 0; i < img_old->height; i++)
				{
				k = i + skewi;
				if (k >= 0 && k < new_height)
				    {
				    img_h2.data[j + tmp_width * k] = 
					img_h1.data[j + tmp_width * i];
				    }
				}
			    }

			img_create (img_new, pcm, new_width, new_height, &stat);
			if (!odd (stat)) 
                            {
                            img_free (&img_h2);
                            img_free (&img_h1);  
                            break;
                            }

			for (i = 0; i < img_old->color_t->ncolors; i++)
			    img_new->color_t->rgb[i] = img_old->color_t->rgb[i];
			img_new->color_t->ncolors = img_old->color_t->ncolors;

			for (i = 0; i < new_height; i++)
			    {
			    skew = i * x_shear;
			    skewi = (int)skew;
			    skewf = skew - skewi;
			    skewi = skewi - x_shear_junk;

			    for (j = 0; j < new_width; j++)
				img_new->data[j + new_width * i] = 0;

			    for (j = 0; j < tmp_width; j++)
				{
				k = j + skewi;
				if ( k >= 0 && k < new_width)
				    {
				    img_new->data[k + new_width * i] = 
					img_h2.data[j + tmp_width * i];
				    }
				}
			    }

			img_free (&img_h2);
			img_free (&img_h1);
			}
		    break;

		case pgm:
		    img_create (&img_h1, pgm, tmp_width, img_old->height, 
			&stat);
		    if (!odd (stat)) break;

		    img_h1.maxgray = img_old->maxgray;

		    for (i = 0; i < img_old->height; i++)
			{
			skew = i * x_shear;
			skewi = (int)skew;

			for (j = 0; j < skewi; j++)
			    img_h1.data[j + tmp_width * i] = 0;

			if (antialias)
			    {
			    skewf = skew - skewi;
			    oleft = 0;

                            n = skewi + img_old->width;
			    for (j = skewi; j < n; j++)
				{
				pixel = img_old->data[j - skewi + 
				    img_old->width * i];
				left = pixel * skewf;
				pixel = pixel - left + oleft;
				img_h1.data[j + tmp_width * i] = (BYTE)pixel;
				oleft = left;
				}
			    }
			else
			    {
                            n = skewi + img_old->width;
			    for (j = skewi; j < n; j++)
				img_h1.data[j + tmp_width * i] = 
				    img_old->data[j - skewi + 
				    img_old->width * i];
			    }

                        n = skewi + img_old->width;
			for (j = n; j < tmp_width; j++)
			    img_h1.data[j + tmp_width * i] = 0;
			}

		    img_create (&img_h2, pgm, tmp_width, new_height, &stat);
		    if (!odd (stat)) 
                        {
                        img_free (&img_h1);
                        break;
                        }

		    img_h2.maxgray = img_old->maxgray;

		    for (j = 0; j < tmp_width; j++)
			{
			skew = (tmp_width - j) * y_shear;
			skewi = (int)skew;
			skewf = skew - skewi;
			skewi = skewi - y_shear_junk;

			for (i = 0; i < new_height; i++)
			    img_h2.data[j + tmp_width * i] = 0;

			if (antialias)
			    {
			    oleft = 0;

			    for (i = 0; i < img_old->height; i++)
				{
				k = i + skewi;
				if (k >= 0 && k < new_height)
				    {
				    pixel = img_h1.data[j + tmp_width * i];
				    left = pixel * skewf;
				    pixel = pixel - left + oleft;
				    img_h2.data[j + tmp_width * k] =
					(BYTE)pixel;
				    oleft = left;
				    }
				}
			    }
			else
			    {
			    for (i = 0; i < img_old->height; i++)
				{
				k = i + skewi;
				if (k >= 0 && k < new_height)
				    {
				    img_h2.data[j + tmp_width * k] = 
					img_h1.data[j + tmp_width * i];
				    }
				}
			    }
			}

		    img_create (img_new, pgm, new_width, new_height, &stat);
		    if (!odd (stat)) 
                        {
                        img_free (&img_h2);
                        img_free (&img_h1);
                        break;
                        }

		    img_new->maxgray = img_old->maxgray;

		    for (i = 0; i < new_height; i++)
			{
			skew = i * x_shear;
			skewi = (int)skew;
			skewf = skew - skewi;
			skewi = skewi - x_shear_junk;

			for (j = 0; j < new_width; j++)
			    img_new->data[j + new_width * i] = 0;

			if (antialias)
			    {
			    oleft = 0;

			    for (j = 0; j < tmp_width; j++)
				{
				k = j + skewi;
				if ( k >= 0 && k < new_width)
				    {
				    pixel = img_h2.data[j + tmp_width * i];
				    left = pixel * skewf;
				    pixel = pixel - left + oleft;
				    img_new->data[k + new_width * i] =
					(BYTE)pixel;
				    oleft = left;
				    }
				}
			    }
			else
			    {
			    for (j = 0; j < tmp_width; j++)
				{
				k = j + skewi;
				if ( k >= 0 && k < new_width)
				    {
				    img_new->data[k + new_width * i] = 
					img_h2.data[j + tmp_width * i];
				    }
				}
			    }
			}

		    img_free (&img_h2);
		    img_free (&img_h1);
		    break;

		case pbm:
		    if (antialias)
			{
			img_pgm (img_old, &img, 255, &stat);
			if (!odd (stat)) break;

			img_rotate (&img, img_new, angle, antialias, &stat);
			if (!odd (stat)) break;

			img_free (&img);
			}
		    else
			{
			img_create (&img_h1, pbm, tmp_width, img_old->height,
			    &stat);
			if (!odd (stat)) break;

			for (i = 0; i < img_old->height; i++)
			    {
			    skew = i * x_shear;
			    skewi = (int)skew;

                            n = skewi + img_old->width;
			    for (j = skewi; j < n; j++)
				{
				k = j + tmp_width * i;
				l = j - skewi + img_old->width * i;
				if (img_old->data[l>>03] & (01 << (7 - l % 8)))
				    img_h1.data[k>>03] |= (01 << (7 - k % 8));
				}
			    }

			img_create (&img_h2, pbm, tmp_width, new_height, &stat);
			if (!odd (stat)) 
                            {
                            img_free (&img_h1);
                            break;
                            }

			for (j = 0; j < tmp_width; j++)
			    {
			    skew = (tmp_width - j) * y_shear;
			    skewi = (int)skew;
			    skewf = skew - skewi;
			    skewi = skewi - y_shear_junk;

			    for (i = 0; i < img_old->height; i++)
				{
				k = i + skewi;
				if (k >= 0 && k < new_height)
				    {
				    l = j + tmp_width * k;
				    m = j + tmp_width * i;
				    if (img_h1.data[m>>03] & (01 << (7 - m % 8)))
					img_h2.data[l>>03] |= (01 << (7 - l % 8));
				    }
				}
			    }

			img_create (img_new, pbm, new_width, new_height, &stat);
			if (!odd (stat)) 
                            {
                            img_free (&img_h2);
                            img_free (&img_h1);
                            break;
                            }

			for (i = 0; i < new_height; i++)
			    {
			    skew = i * x_shear;
			    skewi = (int)skew;
			    skewf = skew - skewi;
			    skewi = skewi - x_shear_junk;

			    for (j = 0; j < tmp_width; j++)
				{
				k = j + skewi;
				if ( k >= 0 && k < new_width)
				    {
				    l = k + new_width * i;
				    m = j + tmp_width * i;
				    if (img_h2.data[m>>03] & (01 << (7 - m % 8)))
					img_new->data[l>>03] |= 
					    (01 << (7 - l % 8));
				    }
				}
			    }

			img_free (&img_h2);
			img_free (&img_h1);
			}
		    break;

		default:
		    stat = img__invimgtyp;
		    break;
		}
	    }
	}

    if (status != NIL)
	*status = stat;
    else
	if (!odd (stat))
	    raise_exception (stat, 0, NULL, NULL);
}



static
void buildgamma (BYTE *table, int maxval, double gamma)

/*
 * buildgamma - build a gamnma table for 256 intensities
 */

{
    register int i, j;
    register double gammainv, x;

    if (gamma != 0) gammainv = 1. / gamma;

    for (i = 0; i <= maxval; i++)
	{
	x = (double) i / (double)maxval;

        if (gamma != 0)
    	    j = round ((double)maxval * pow (x, gammainv));
        else
            j = 0;

	if (j > maxval)
	    j = maxval;

	table[i] = j;
	} 
}



void img_pgmgamma (image_dscr *img_old, image_dscr *img_new, double gamma,
    int *status)

/*
 *  img_pgmgamma - do a gamma correction of a pgm image
 */

{
    register int i, n;
    int stat;
    BYTE table[256];

    stat = img__normal;

    if (gamma < 0.0)
	stat = img__invgamma;
    else
	{
	buildgamma (table, img_old->maxgray, gamma);

	img_create (img_new, pgm, img_old->width, img_old->height, &stat);
	if (odd (stat))
	    {
	    img_new->maxgray = img_old->maxgray;

            n = img_old->width * img_old->height;
	    for (i = 0; i < n; i++)
		img_new->data[i] = table[img_old->data[i]];
	    }
	}

    if (status != NIL)
	*status = stat;
    else
	if (!odd (stat))
	    raise_exception (stat, 0, NULL, NULL);
}


    
void img_rgbgamma (image_dscr *img_old, image_dscr *img_new, double rgamma,
    double ggamma, double bgamma, int *status)

/*
 *  img_ppmgamma - do a gamma correction of a ppm image
 */

{
    register int i, n;
    int stat;
    BYTE rtable[256], gtable[256], btable[256];

    stat = img__normal;

    if (rgamma < 0.0 || ggamma < 0.0 || bgamma < 0.0)
	stat = img__invgamma;
    else
	{
        switch (img_old->type) {

            case ppm:
	        buildgamma (rtable, 255, rgamma);
	        buildgamma (gtable, 255, ggamma);
	        buildgamma (btable, 255, bgamma);

	        img_create (img_new, ppm, img_old->width, img_old->height, 
                    &stat);
	        if (odd (stat))
	            {
                    n = img_old->width * img_old->height;
	            for (i = 0; i < n; i++)
		        {
		        img_new->data[3*i]  = rtable[img_old->data[3*i]];
		        img_new->data[3*i+1] = gtable[img_old->data[3*i+1]];
		        img_new->data[3*i+2] = btable[img_old->data[3*i+2]];
		        }
	            }
                break;

            case pcm:
	        buildgamma (rtable, 255, rgamma);
	        buildgamma (gtable, 255, ggamma);
	        buildgamma (btable, 255, bgamma);

	        img_copy (img_old, img_new, &stat);
	        if (odd (stat))
	            {
	            for (i = 0; i < img_old->color_t->ncolors; i++)
		        {
		        img_new->color_t->rgb[i].r = 
                            rtable[img_old->color_t->rgb[i].r];
		        img_new->color_t->rgb[i].g = 
                            gtable[img_old->color_t->rgb[i].g];
		        img_new->color_t->rgb[i].b = 
                            btable[img_old->color_t->rgb[i].b];
		        }
	            }
                break;
        
            default:
                stat = img__invimgtyp;
                break;
            }
	}

    if (status != NIL)
	*status = stat;
    else
	if (!odd (stat))
	    raise_exception (stat, 0, NULL, NULL);
}



void img_gamma (image_dscr *img_old, image_dscr *img_new, double gamma,
    int *status)

/*
 *  img_gamma - do a gamma correction of an image
 */

{
    int stat;
    image_dscr img;

    stat = img__normal;

    switch (img_old->type) {

	case ppm:
	case pcm:
	    img_rgbgamma (img_old, img_new, gamma, gamma, gamma, &stat);
	    break;

	case pgm:
	    img_pgmgamma (img_old, img_new, gamma, &stat);
	    break;

	case pbm:
	    img_pgm (img_old, &img, 255, &stat);
	    if (odd (stat))
		{
		img_pgmgamma (&img, img_new, gamma, &stat);
		img_free (&img);
		}
	    break;

	default:
	    stat = img__invimgtyp;
	    break;
	}

    if (status != NIL)
	*status = stat;
    else
	if (!odd (stat))
	    raise_exception (stat, 0, NULL, NULL);
}



void img_invert (image_dscr *img_old, image_dscr *img_new, int *status)

/*
 *  img_invert - invert an image
 */

{
    register int i, j, n;
    int  stat;

    stat = img__normal;

    switch (img_old->type) {

	case ppm:
	    img_create (img_new, ppm, img_old->width, img_old->height, &stat);
	    if (!odd (stat)) break;

            n = 3 * img_old->width * img_old->height;
	    for (i = 0; i < n; i++)
		img_new->data[i] = 255 - img_old->data[i];
	    break;

	case pcm:
	    img_create (img_new, pcm, img_old->width, img_old->height, &stat);
	    if (!odd (stat)) break;

	    img_new->color_t->ncolors = img_old->color_t->ncolors;

	    for (i = 0; i < img_old->color_t->ncolors; i++)
		{
		img_new->color_t->rgb[i].r = 255 - img_old->color_t->rgb[i].r;
		img_new->color_t->rgb[i].g = 255 - img_old->color_t->rgb[i].g;
		img_new->color_t->rgb[i].b = 255 - img_old->color_t->rgb[i].b;
		}

            n = img_old->width * img_old->height;
	    for (i = 0; i < n; i++)
		img_new->data[i] = img_old->data[i];
	    break;

	case pgm:
	    img_create (img_new, pgm, img_old->width, img_old->height, &stat);
	    if (!odd (stat)) break;

	    img_new->maxgray = img_old->maxgray;

            n = img_old->width * img_old->height;
	    for (i = 0; i < n; i++)
		img_new->data[i] = img_new->maxgray - img_old->data[i];
	    break;

	case pbm:
	    img_create (img_new, pbm, img_old->width, img_old->height, &stat);
	    if (!odd (stat)) break;

	    j = (int)(img_old->width * img_old->height * 0.125);
	    if (j * 8 != img_old->width * img_old->height)
		j++;

	    for (i = 0; i < j; i++)
		img_new->data[i] = ~img_old->data[i];
	    break;

	default:
	    stat = img__invimgtyp;
	    break;
	}

    if (status != NIL)
	*status = stat;
    else
	if (!odd (stat))
	    raise_exception (stat, 0, NULL, NULL);
}



void img_autocrop (image_dscr *img_old, image_dscr *img_new, int *status)

/*
 *  img_autocrop - autocrop an image
 */ 

{
    register int i, j, k, l, x1, x2, y1, y2, dx, dy, found=0;
    BYTE bg[3], color[MAX_COLORS][3];
    int colorcount[MAX_COLORS], colornr, cmax, cindex;

    int stat;

    stat = img__normal;

    x1 = 1;
    x2 = img_old->width;
    y1 = 1;
    y2 = img_old->height;

   
    switch (img_old->type) {

	case ppm:

    	    color[0][0] = img_old->data[0];
    	    color[0][1] = img_old->data[1];
    	    color[0][2] = img_old->data[2];
    	    colornr = 1;
    	    colorcount[0] = 1;

	    for (i = 0; i < img_old->width; i++)
    	    	{
    	    	k = 3 * i;
    	    	for (j = 0; j < colornr; j++)
    	    	    {
    	    	    if (img_old->data[k] == color[j][0] 
    	    	    	&& img_old->data[k+1] == color[j][1]
                    	&& img_old->data[k+2] == color[j][2])
    	    	    	{
    	    	    	colorcount[j]++;
    	    	    	break;
    	    	    	}
    	    	    else
    	    	    	{	
    	    	    	if (colornr < MAX_COLORS)
    	    	    	    {
        	    	    colornr++;
    	    	    	    colorcount[colornr-1] = 1;
    	    	    	    color[colornr-1][0] = img_old->data[k];
    	    	    	    color[colornr-1][1] = img_old->data[k+1];
    	    	    	    color[colornr-1][2] = img_old->data[k+2];
    	    	    	    }
    	    	    	}
    	    	    }
    	    	}


	    for (i = 0; i < img_old->width; i++)
    	    	{
    	    	k = 3 * (img_old->width * (img_old->height - 1) + i);
    	    	for (j = 0; j < colornr; j++)
    	    	    {
    	    	    if (img_old->data[k] == color[j][0] 
    	    	    	&& img_old->data[k+1] == color[j][1]
                    	&& img_old->data[k+2] == color[j][2])
    	    	    	{
    	    	    	colorcount[j]++;
    	    	    	break;
    	    	    	}
    	    	    else
    	    	    	{	
    	    	    	if (colornr < MAX_COLORS)
    	    	    	    {
        	    	    colornr++;
    	    	    	    colorcount[colornr-1] = 1;
    	    	    	    color[colornr-1][0] = img_old->data[k];
    	    	    	    color[colornr-1][1] = img_old->data[k+1];
    	    	    	    color[colornr-1][2] = img_old->data[k+2];
    	    	    	    }
    	    	    	}
    	    	    }
    	    	}


	    for (i = 0; i < img_old->width; i++)
    	    	{
    	    	k = 3 * (img_old->width * i);
    	    	for (j = 0; j < colornr; j++)
    	    	    {
    	    	    if (img_old->data[k] == color[j][0] 
    	    	    	&& img_old->data[k+1] == color[j][1]
                    	&& img_old->data[k+2] == color[j][2])
    	    	    	{
    	    	    	colorcount[j]++;
    	    	    	break;
    	    	    	}
    	    	    else
    	    	    	{	
    	    	    	if (colornr < MAX_COLORS)
    	    	    	    {
        	    	    colornr++;
    	    	    	    colorcount[colornr-1] = 1;
    	    	    	    color[colornr-1][0] = img_old->data[k];
    	    	    	    color[colornr-1][1] = img_old->data[k+1];
    	    	    	    color[colornr-1][2] = img_old->data[k+2];
    	    	    	    }
    	    	    	}
    	    	    }
    	    	}


	    for (i = 0; i < img_old->width; i++)
    	    	{
    	    	k = 3 * (img_old->width * i + img_old->width - 1);
    	    	for (j = 0; j < colornr; j++)
    	    	    {
    	    	    if (img_old->data[k] == color[j][0] 
    	    	    	&& img_old->data[k+1] == color[j][1]
                    	&& img_old->data[k+2] == color[j][2])
    	    	    	{
    	    	    	colorcount[j]++;
    	    	    	break;
    	    	    	}
    	    	    else
    	    	    	{	
    	    	    	if (colornr < MAX_COLORS)
    	    	    	    {
        	    	    colornr++;
    	    	    	    colorcount[colornr-1] = 1;
    	    	    	    color[colornr-1][0] = img_old->data[k];
    	    	    	    color[colornr-1][1] = img_old->data[k+1];
    	    	    	    color[colornr-1][2] = img_old->data[k+2];
    	    	    	    }
    	    	    	}
    	    	    }
    	    	}
    	    
    	    /* set bgcolor */
    	    cmax = colorcount[0];
    	    cindex=0;
    	    for (j = 1; j < colornr; j++)
    	    	{
    	    	if (colorcount[j] > cmax)
    	    	    {
    	    	    cmax = colorcount[j];
    	    	    cindex = j;
    	    	    }
    	    	}


    	    bg[0] = color[cindex][0];
    	    bg[1] = color[cindex][1];
    	    bg[2] = color[cindex][2];
            

    	    /* find upper border */
    	    for (i = 0; i < img_old->height; i++)
    	    	{
    	    	for (j = 0; j < img_old->width; j++)
    	    	    {
    	    	    k = 3 * (img_old->width * i + j);
   	    	    if (img_old->data[k] != bg[0] || 
    	    	    	img_old->data[k+1] != bg[1] ||
    	    	    	img_old->data[k+2] != bg[2])
    	    	    	{
    	    	    	y1 = i;
    	    	    	found = 1;
    	    	    	break;
    	    	    	}
    	    	    }
    	    	if (found)
                   break;
     	    	}


            /* found lower border */
    	    found = 0;
    	    for (i = img_old->height - 1; i > 0; i--)
    	    	{
    	    	for (j = 0; j < img_old->width; j++)
    	    	    {
    	    	    k = 3 * (img_old->width * i + j);
   	    	    if (img_old->data[k] != bg[0] || 
    	    	    	img_old->data[k+1] != bg[1] ||
    	    	    	img_old->data[k+2] != bg[2])
    	    	    	{
    	    	    	y2 = i;
    	    	    	found = 1;
    	    	    	break;
    	    	    	}
    	    	    }
    	    	if (found)
                   break;
     	    	}


    	    /* find left border */
    	    found = 0;
    	    for (i = 0; i < img_old->width; i++)
    	    	{
    	    	for (j = 0; j < img_old->height; j++)
    	    	    {
    	    	    k = 3 * (img_old->width * j + i);
   	    	    if (img_old->data[k] != bg[0] || 
    	    	    	img_old->data[k+1] != bg[1] ||
    	    	    	img_old->data[k+2] != bg[2])
    	    	    	{
    	    	    	x1 = i;
    	    	    	found = 1;
    	    	    	break;
    	    	    	}
    	    	    }
    	    	if (found)
                   break;
     	    	}


    	    /* find right border */
    	    found=0;
    	    for (i = img_old->width - 1; i > 0; i--)
    	    	{
    	    	for (j = 0; j<img_old->height; j++)
    	    	    {
    	    	    k = 3 * (img_old->width * j + i);
   	    	    if (img_old->data[k] != bg[0] || 
    	    	    	img_old->data[k+1] != bg[1] ||
    	    	    	img_old->data[k+2] != bg[2])
    	    	    	{
    	    	    	x2 = i;
    	    	    	found = 1;
    	    	    	break;
    	    	    	}
    	    	    }
    	    	if (found)
                   break;
     	    	}

    	    dx = x2 - x1 +1;
    	    dy = y2 - y1 +1;

	    img_create (img_new, ppm, dx, dy, &stat);
	    if (!odd (stat)) break;

	    for (i = 0; i < dy; i++)
		for (j = 0; j < dx; j++)
		    {
		    k = 3 * (dx * i + j);
		    l = 3 * (img_old->width * (i + y1) + j + x1);
		    img_new->data[k] = img_old->data[l];
		    img_new->data[k+1] = img_old->data[l+1];
		    img_new->data[k+2] = img_old->data[l+2];
		    }

	    break;


    	case pcm:

    	    color[0][0] = img_old->data[0];
    	    colornr = 1;
    	    colorcount[0] = 1;

	    for (i = 0; i < img_old->width; i++)
    	    	{
    	    	for (j = 0; j < colornr; j++)
    	    	    {
    	    	    if (img_old->data[i] == color[j][0])
    	    	    	{
    	    	    	colorcount[j]++;
    	    	    	break;
    	    	    	}
    	    	    else
    	    	    	{	
    	    	    	if (colornr < MAX_COLORS)
    	    	    	    {
        	    	    colornr++;
    	    	    	    colorcount[colornr-1] = 1;
    	    	    	    color[colornr-1][0] = img_old->data[i];
    	    	    	    }
    	    	    	}
    	    	    }
    	    	}


	    for (i = 0; i < img_old->width; i++)
    	    	{
    	    	k = img_old->width * (img_old->height - 1) + i;
    	    	for (j = 0; j < colornr; j++)
    	    	    {
    	    	    if (img_old->data[i] == color[j][0]) 
    	    	    	{
    	    	    	colorcount[j]++;
    	    	    	break;
    	    	    	}
    	    	    else
    	    	    	{	
    	    	    	if (colornr < MAX_COLORS)
    	    	    	    {
        	    	    colornr++;
    	    	    	    colorcount[colornr-1] = 1;
    	    	    	    color[colornr-1][0] = img_old->data[i];
    	    	    	    }
    	    	    	}
    	    	    }
    	    	}


	    for (i = 0; i < img_old->width; i++)
    	    	{
    	    	k = img_old->width * i;
    	    	for (j = 0; j < colornr; j++)
    	    	    {
    	    	    if (img_old->data[k] == color[j][0])
    	    	    	{
    	    	    	colorcount[j]++;
    	    	    	break;
    	    	    	}
    	    	    else
    	    	    	{	
    	    	    	if (colornr < MAX_COLORS)
    	    	    	    {
        	    	    colornr++;
    	    	    	    colorcount[colornr-1] = 1;
    	    	    	    color[colornr-1][0] = img_old->data[i];
    	    	    	    }
    	    	    	}
    	    	    }
    	    	}


	    for (i = 0; i < img_old->width; i++)
    	    	{
    	    	k = img_old->width * i + img_old->width - 1;
    	    	for (j = 0; j < colornr; j++)
    	    	    {
    	    	    if (img_old->data[i] == color[j][0])
    	    	    	{
    	    	    	colorcount[j]++;
    	    	    	break;
    	    	    	}
    	    	    else
    	    	    	{	
    	    	    	if (colornr < MAX_COLORS)
    	    	    	    {
        	    	    colornr++;
    	    	    	    colorcount[colornr-1] = 1;
    	    	    	    color[colornr-1][0] = img_old->data[k];
    	    	    	    }
    	    	    	}
    	    	    }
    	    	}
    	    
    	    /* set bgcolor */
    	    cmax = colorcount[0];
    	    cindex=0;
    	    for (j = 1; j < colornr; j++)
    	    	{
    	    	if (colorcount[j] > cmax)
    	    	    {
    	    	    cmax = colorcount[j];
    	    	    cindex = j;
    	    	    }
    	    	}


    	    bg[0] = color[cindex][0];


    	    /* find upper border */
    	    found = 0;
    	    for (i = 0; i < img_old->height; i++)
    	    	{
    	    	for (j = 0; j < img_old->width; j++)
    	    	    {
    	    	    k = i * img_old->width + j;
   	    	    if (img_old->data[k] != bg[0])
    	    	    	{
    	    	    	y1 = i;
    	    	    	found = 1;
    	    	    	break;
    	    	    	}
    	    	    }
    	    	if (found)
                   break;
     	    	}


    	    /* find lower border */
    	    found = 0;
    	    for (i = img_old->height - 1; i >= 0; i--)
    	    	{
    	    	for (j = 0; j < img_old->width; j++)
    	    	    {
    	    	    k = i * img_old->width + j;
   	    	    if (img_old->data[k] != bg[0])
    	    	    	{
    	    	    	y2 = i;
    	    	    	found = 1;
    	    	    	break;
    	    	    	}
    	    	    }
    	    	if (found)
                   break;
     	    	}


    	    /* find left border */
    	    found = 0;
    	    for (i = 0; i < img_old->width; i++)
    	    	{
    	    	for (j = 0; j < img_old->height; j++)
    	    	    {
    	    	    k = j * img_old->width + i;
   	    	    if (img_old->data[k] != bg[0])
    	    	    	{
    	    	    	x1 = i;
    	    	    	found = 1;
    	    	    	break;
    	    	    	}
    	    	    }
    	    	if (found)
                   break;
     	    	}


    	    /* find right border */
    	    found = 0;
    	    for (i = img_old->width - 1; i >= 0; i--)
    	    	{
    	    	for (j = 0; j < img_old->height; j++)
    	    	    {
    	    	    k = j * img_old->width + i;
   	    	    if (img_old->data[k] != bg[0]) 
    	    	    	{
    	    	    	x2 = i;
    	    	    	found = 1;
    	    	    	break;
    	    	    	}
    	    	    }
    	    	if (found)
                   break;
     	    	}

    	    
    	    dx = x2 - x1 + 1;
    	    dy = y2 - y1 + 1;

    	    img_create (img_new, pcm, dx, dy, &stat);
    	    if (!odd (stat)) break;

    	    img_new->color_t->ncolors = img_old->color_t->ncolors;
    	    for (i = 0; i < img_old->color_t->ncolors; i++)
    	    	img_new->color_t->rgb[i] = img_old->color_t->rgb[i];

    	    for (i = 0; i < dy; i++)
    	    	for (j = 0; j < dx; j++)
    	    	    {
    	    	    k = i * dx + j;
    	    	    l = img_old->width * (i + y1) + (j + x1);
    	    	    img_new->data[k] = img_old->data[l];
    	    	    }
    	    break;


    	case pgm:

    	    color[0][0] = img_old->data[0];
    	    colornr = 1;
    	    colorcount[0] = 1;

	    for (i = 0; i < img_old->width; i++)
    	    	{
    	    	for (j = 0; j < colornr; j++)
    	    	    {
    	    	    if (img_old->data[i] == color[j][0])
    	    	    	{
    	    	    	colorcount[j]++;
    	    	    	break;
    	    	    	}
    	    	    else
    	    	    	{	
    	    	    	if (colornr < MAX_COLORS)
    	    	    	    {
        	    	    colornr++;
    	    	    	    colorcount[colornr-1] = 1;
    	    	    	    color[colornr-1][0] = img_old->data[i];
    	    	    	    }
    	    	    	}
    	    	    }
    	    	}


	    for (i = 0; i < img_old->width; i++)
    	    	{
    	    	k = img_old->width * (img_old->height - 1) + i;
    	    	for (j = 0; j < colornr; j++)
    	    	    {
    	    	    if (img_old->data[i] == color[j][0]) 
    	    	    	{
    	    	    	colorcount[j]++;
    	    	    	break;
    	    	    	}
    	    	    else
    	    	    	{	
    	    	    	if (colornr < MAX_COLORS)
    	    	    	    {
        	    	    colornr++;
    	    	    	    colorcount[colornr-1] = 1;
    	    	    	    color[colornr-1][0] = img_old->data[i];
    	    	    	    }
    	    	    	}
    	    	    }
    	    	}


	    for (i = 0; i < img_old->width; i++)
    	    	{
    	    	k = img_old->width * i;
    	    	for (j = 0; j < colornr; j++)
    	    	    {
    	    	    if (img_old->data[k] == color[j][0])
    	    	    	{
    	    	    	colorcount[j]++;
    	    	    	break;
    	    	    	}
    	    	    else
    	    	    	{	
    	    	    	if (colornr < MAX_COLORS)
    	    	    	    {
        	    	    colornr++;
    	    	    	    colorcount[colornr-1] = 1;
    	    	    	    color[colornr-1][0] = img_old->data[i];
    	    	    	    }
    	    	    	}
    	    	    }
    	    	}


	    for (i = 0; i < img_old->width; i++)
    	    	{
    	    	k = img_old->width * i + img_old->width - 1;
    	    	for (j = 0; j < colornr; j++)
    	    	    {
    	    	    if (img_old->data[i] == color[j][0])
    	    	    	{
    	    	    	colorcount[j]++;
    	    	    	break;
    	    	    	}
    	    	    else
    	    	    	{	
    	    	    	if (colornr < MAX_COLORS)
    	    	    	    {
        	    	    colornr++;
    	    	    	    colorcount[colornr-1] = 1;
    	    	    	    color[colornr-1][0] = img_old->data[k];
    	    	    	    }
    	    	    	}
    	    	    }
    	    	}
    	    
    	    /* set bgcolor */
    	    cmax = colorcount[0];
    	    cindex=0;
    	    for (j = 1; j < colornr; j++)
    	    	{
    	    	if (colorcount[j] > cmax)
    	    	    {
    	    	    cmax = colorcount[j];
    	    	    cindex = j;
    	    	    }
    	    	}


    	    bg[0] = color[cindex][0];


    	    /* find upper border */
    	    found = 0;
    	    for (i = 0; i < img_old->height; i++)
    	    	{
    	    	for (j = 0; j < img_old->width; j++)
    	    	    {
    	    	    k = i * img_old->width + j;
   	    	    if (img_old->data[k] != bg[0])
    	    	    	{
    	    	    	y1 = i;
    	    	    	found = 1;
    	    	    	break;
    	    	    	}
    	    	    }
    	    	if (found)
                   break;
     	    	}

    	    /* find lower border */
    	    found = 0;
    	    for (i = img_old->height - 1; i > 0; i--)
    	    	{
    	    	for (j = 0; j < img_old->width; j++)
    	    	    {
    	    	    k = i * img_old->width + j;
   	    	    if (img_old->data[k] != bg[0])
    	    	    	{
    	    	    	y2 = i;
    	    	    	found = 1;
    	    	    	break;
    	    	    	}
    	    	    }
    	    	if (found)
                   break;
     	    	}


    	    /* find left border */
    	    found = 0;
    	    for (i = 0; i < img_old->width; i++)
    	    	{
    	    	for (j = 0; j < img_old->height; j++)
    	    	    {
    	    	    k = j * img_old->width + i;
   	    	    if (img_old->data[k] != bg[0])
    	    	    	{
    	    	    	x1 = i;
    	    	    	found = 1;
    	    	    	break;
    	    	    	}
    	    	    }
    	    	if (found)
                   break;
     	    	}


    	    /* find right border */
    	    found = 0;
    	    for (i = img_old->width - 1; i > 0; i--)
    	    	{
    	    	for (j = 0; j < img_old->height; j++)
    	    	    {
    	    	    k = j * img_old->width + i;
   	    	    if (img_old->data[k] != bg[0])
    	    	    	{
    	    	    	x2 = i;
    	    	    	found = 1;
    	    	    	break;
    	    	    	}
    	    	    }
    	    	if (found)
                   break;
     	    	}


    	    dx = x2 - x1 + 1;
            dy = y2 - y1 + 1;

    	    img_create(img_new, pgm, dx, dy, &stat);
    	    if (!odd(stat)) break;
    	    
    	    img_new->maxgray = img_old->maxgray;
    	    
    	    for (i = 0; i < dy; i++)
    	    	for (j = 0; j < dx; j++)
    	    	    {
    	    	    k = dx * i + j;
    	    	    l = img_old->width * (i + y1) + j + x1;
    	    	    img_new->data[k] = img_old->data[l];
    	    	    }
    	    break;


    	case pbm:

	    for (i = 0; i < img_old->width; i++)
    	    	{
    	    	for (l = 0; l < 8; l++)
    	    	    {
   	    	    if ((img_old->data[i>>03] & 01 << l) == 0)
    	    	    	colorcount[0]++;
    	    	    else
    	    	    	colorcount[1]++;
    	    	    }
     	    	}


	    for (i = 0; i < img_old->width; i++)
    	    	{
    	    	k = img_old->width * (img_old->height - 1) + i;
    	    	for (l = 0; l < 8; l++)
    	    	    {
   	    	    if ((img_old->data[k>>03] & 01 << l) == 0)
    	    	    	colorcount[0]++;
    	    	    else
    	    	    	colorcount[1]++;
    	    	    }
     	    	}


	    for (i = 0; i < img_old->width; i++)
    	    	{
    	    	k = img_old->width * i;
    	    	for (l = 0; l < 8; l++)
    	    	    {
   	    	    if ((img_old->data[i>>03] & 01 << l) == 0)
    	    	    	colorcount[0]++;
    	    	    else
    	    	    	colorcount[1]++;
    	    	    }
     	    	}


	    for (i = 0; i < img_old->width; i++)
    	    	{
    	    	k = img_old->width * i + img_old->width - 1;
    	    	for (l = 0; l < 8; l++)
    	    	    {
   	    	    if ((img_old->data[i>>03] & 01 << l) == 0)
    	    	    	colorcount[0]++;
    	    	    else
    	    	    	colorcount[1]++;
    	    	    }
     	    	}


    	    if (colorcount[0] > colorcount[1])
    	    	bg[0] = 0;
    	    else
    	    	bg[0] = 1;


    	    /* find upper border */
    	    found = 0;
    	    for (i = 0; i < img_old->height; i++)
    	    	{
    	    	for (j = 0; j < img_old->width; j++)
    	    	    {
    	    	    k = i * img_old->width + j;
   	    	    if ((img_old->data[k>>03] & 01 << (7 - k % 8)) != bg[0])
    	    	    	{
    	    	    	y1 = i;
    	    	    	found = 1;
    	    	    	break;
    	    	    	}
    	    	    }
    	    	if (found)
                    break;
     	    	}

    	    /* find lower border */
    	    found = 0;
    	    for (i = img_old->height - 1; i > 0; i--)
    	    	{
    	    	for (j = 0; j < img_old->width; j++)
    	    	    {
    	    	    k = i * img_old->width + j;
   	    	    if ((img_old->data[k>>03] & 01 << (7 - k % 8)) != bg[0])
    	    	    	{
    	    	    	y2 = i;
    	    	    	found = 1;
    	    	    	break;
    	    	    	}
    	    	    }
    	    	if (found)
                   break;
     	    	}


    	    /* find left border */
    	    found = 0;
    	    for (i = 0; i < img_old->width; i++)
    	    	{
    	    	for (j = 0; j < img_old->height; j++)
    	    	    {
    	    	    k = j * img_old->width + i;
   	    	    if ((img_old->data[k>>03] & 01 << (7 - k % 8)) != bg[0])
    	    	    	{
    	    	    	x1 = i;
    	    	    	found = 1;
    	    	    	break;
    	    	    	}
    	    	    }
    	    	if (found)
                   break;
     	    	}


    	    /* find right border */
    	    found = 0;
    	    for (i = img_old->width - 1; i > 0; i--)
    	    	{
    	    	for (j = 0; j < img_old->height; j++)
    	    	    {
    	    	    k = j * img_old->width + i;
   	    	    if ((img_old->data[k>>03] & 01 << (7 - k % 8)) != bg[0]) 
    	    	    	{
    	    	    	x2 = i;
    	    	    	found = 1;
    	    	    	break;
    	    	    	}
    	    	    }
    	    	if (found)
                   break;
     	    	}


    	    dx = x2 - x1 + 1;
            dy = y2 - y1 + 1;

    	    img_create(img_new, pbm, dx, dy, &stat);
    	    if (!odd(stat)) break;
    	    
    	    for (i = 0; i < dy; i++)
    	    	for (j = 0; j < dx; j++)
    	    	    {
    	    	    k = dx * i + j;
    	    	    l = img_old->width * (i + y1) + j + x1;
    	    	    if ( img_old->data[l>>03] & (01 << (7 - l % 8)))
    	    	    	img_new->data[k>>03] |= (01 << (7 - k % 8));
    	    	    }
    	    break;

    	default:
    	    stat = img__invimgtyp;
    	    break;
    	}

    	if (status != NIL)
    	    *status = stat;
    	else
    	    if (!odd (stat))
    	    	raise_exception (stat, 0, NULL, NULL);
}



void img_scale (image_dscr *img_old, image_dscr *img_new, int new_width,
    int new_height, int *status)

/*
 *  img_scale - scale an image to the specified size
 */

{
    register int i, j, k, l, m, n;
    int stat; 

    stat = img__normal;

    img_create (img_new, img_old->type, new_width, new_height, &stat);
    if (odd (stat))
	{
	switch (img_old->type) {

	    case ppm:
		for (i = 0; i < new_height; i++)
		    {
		    k = (img_old->height * i) / new_height;
		    for (j = 0; j < new_width; j++)
			{
			l = (img_old->width * j) / new_width;
			m = 3 * (i * new_width + j);
			n = 3 * (k * img_old->width + l);

			img_new->data[m] = img_old->data[n];
			img_new->data[m+1] = img_old->data[n+1];
			img_new->data[m+2] = img_old->data[n+2];
			}
		    }
		break; 

	    case pcm:
		img_new->color_t->ncolors = img_old->color_t->ncolors;
		for (i = 0; i < img_new->color_t->ncolors; i++)
		    img_new->color_t->rgb[i] = img_old->color_t->rgb[i];

		for (i = 0; i < new_height; i++)
		    {
		    k = (img_old->height * i) / new_height;
		    for (j = 0; j < new_width; j++)
			{
			l = (img_old->width * j) / new_width;
			m = i * new_width + j;
			n = k * img_old->width + l;

			img_new->data[m] = img_old->data[n];
			}
		    }
		break;

	    case pgm:
		img_new->maxgray = img_old->maxgray;

		for (i = 0; i < new_height; i++)
		    {
		    k = (img_old->height * i) / new_height;
		    for (j = 0; j < new_width; j++)
			{
			l = (img_old->width * j) / new_width;
			m = i * new_width + j;
			n = k * img_old->width + l;

			img_new->data[m] = img_old->data[n];
			}
		    }
		break;

	    case pbm:
		for (i = 0; i < new_height; i++)
		    {
		    k = (img_old->height * i) / new_height;
		    for (j = 0; j < new_width; j++)
			{
			l = (img_old->width * j) / new_width;
			m = i * new_width + j;
			n = k * img_old->width + l;

			if (img_old->data[n>>03] & (01 << (7 - n % 8)))
			    img_new->data[m>>03] |= (01 << (7 - m % 8));
			}
		    }
		break;
	    }
	}

    if (status != NIL)
	*status = stat;
    else
	if (!odd (stat))
	    raise_exception (stat, 0, NULL, NULL);
}



void img_filter (image_dscr *img_old, image_dscr *img_new, BYTE (*filter)(BYTE),
    int *status)

/*
 *  img_filter - perform the filter function on the index of an image
 */

{
    register int i, n;
    int stat;

    stat = img__normal;

    img_create (img_new, img_old->type, img_old->width, img_old->height, &stat);
    if (odd (stat))
	{
	switch (img_old->type) {

	    case ppm:
		stat = img__invimgtyp;
		break; 

	    case pcm:
		img_new->color_t->ncolors = img_old->color_t->ncolors;
		for (i = 0; i < img_new->color_t->ncolors; i++)
		    img_new->color_t->rgb[i] = img_old->color_t->rgb[i];

                n = img_old->width * img_old->height;
		for (i = 0; i < n; i++)
		    img_new->data[i] = filter (img_old->data[i]);
		break;

	    case pgm:
		img_new->maxgray = img_old->maxgray;

                n = img_old->width * img_old->height;
		for (i = 0; i < n; i++)
		    img_new->data[i] = filter (img_old->data[i]);
		break;

	    case pbm:
		stat = img__invimgtyp;
		break;

	    default:
		stat = img__invimgtyp;
		break;
	    }
	}

    if (status != NIL)
	*status = stat;
    else
	if (!odd (stat))
	    raise_exception (stat, 0, NULL, NULL);
}



void img_rgbfilter (image_dscr *img_old, image_dscr *img_new,
    rgb_color (*filter)(rgb_color *), int *status)

/*
 *  img_rgbfilter - perform the filter function on the rgb values of an image
 */

{
    rgb_color color;
    register int i, n;
    int stat;

    stat = img__normal;

    img_create (img_new, img_old->type, img_old->width, img_old->height, &stat);
    if (odd (stat))
	{
	switch (img_old->type) {

	    case ppm:
                n = img_old->width * img_old->height;
		for (i = 0; i < n; i++)
		    {
		    color =  filter ((rgb_color *)&img_old->data[3*i]);
		    img_new->data[3*i] = color.r;
		    img_new->data[3*i+1] = color.g;
		    img_new->data[3*i+2] = color.b;
		    }
		break; 

	    case pcm:
		img_new->color_t->ncolors = img_old->color_t->ncolors;
		for (i = 0; i < img_new->color_t->ncolors; i++)
		    {
		    img_new->color_t->rgb[i] = filter (
			&img_old->color_t->rgb[i]);
		    }

                n = img_old->width * img_old->height;
		for (i = 0; i < n; i++)
		    img_new->data[i] = img_old->data[i];
		break;

	    case pgm:
		stat = img__invimgtyp;
		break;

	    case pbm:
		stat = img__invimgtyp;
		break;

	    default:
		stat = img__invimgtyp;
		break;
	    }
	}

if (status != NIL)
	*status = stat;
    else
	if (!odd (stat))
	    raise_exception (stat, 0, NULL, NULL);
}



static
void rgb_to_hls (double r, double g, double b, double *h, double *l, double *s)
{
    register double v;
    register double m;
    register double delta;
    register double l1, h1;

    v = max (r, g);
    v = max (v, b);
    m = min (r, g);
    m = min (m, b);

    if ((l1 = (m + v) / 2.0) <= 0.0) 
        {
        *l = 0.0;
        *h = 0.0;
        *s = 0.0;
        return;
        }
    if ((delta = v - m) > 0.0) 
        {
        if (l1 <= 0.5)
            *s = delta / (v + m);
        else
            *s = delta / (2.0 - v - m);
        *l = l1;
    
        if (r == v)
            h1 = (g - b) / delta;
        else if (g == v)
            h1 = 2.0 + (b - r) / delta;
        else if (b == v)
            h1 = 4.0 + (r - g) / delta;

        h1 = h1 * 60.0;

        if (h1 < 0.0)
            h1 = h1 + 360;
        *h = h1;
        }
    else
        {
        *h = 0.0;
        *s = 0.0;
        *l = l1;
        return;
        }
}



static
double value (double n1, double n2, double hue)
{
    register double val;

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



static
void hls_to_rgb (double h, double l, double s, double *r, double *g, double *b)
{
    register double m1,m2;

    m2 = (l<0.5) ? (l)*(1+s) : l + s - (l)*(s);
    m1 = 2*(l) - m2;

    if (s == 0)
	{ (*r)=(*g)=(*b)=(l); }
    else
	{
	*r = value (m1, m2, h+120.);
	*g = value (m1, m2, h+000.);
	*b = value (m1, m2, h-120.);
	}
    return;
}



void img_hlsfilter (image_dscr *img_old, image_dscr *img_new,
    hls_color (*filter)(hls_color *), int *status)

/*
 *  img_hlsfilter - perform the hls filter function on the rgb values of an 
 *                  image
 */

{
    hls_color color, ncolor;
    double r, g, b, h, l, s;
    register int i, n;
    int stat;

    stat = img__normal;

    img_create (img_new, img_old->type, img_old->width, img_old->height, &stat);
    if (odd (stat))
	{
	switch (img_old->type) {

	    case ppm:
                n = img_old->width * img_old->height;
		for (i = 0; i < n; i++)
		    {
                    r = img_old->data[3*i] / 255.0;
                    g = img_old->data[3*i+1] / 255.0;
                    b = img_old->data[3*i+2] / 255.0;
                   
                    rgb_to_hls (r, g, b, &h, &l, &s);
                    h = h / 360.0;

                    color.h = round (h * 255);
                    color.l = round (l * 255);
                    color.s = round (s * 255);
        
		    ncolor = filter (&color);

                    h = ncolor.h * 360.0 / 255.0;
                    l = ncolor.l / 255.0;
                    s = ncolor.s / 255.0;

                    hls_to_rgb (h, l, s, &r, &g, &b);

		    img_new->data[3*i] = round (r * 255);
		    img_new->data[3*i+1] = round (g * 255);
		    img_new->data[3*i+2] = round (b * 255);
		    }
		break; 

	    case pcm:
		img_new->color_t->ncolors = img_old->color_t->ncolors;
		for (i = 0; i < img_new->color_t->ncolors; i++)
		    {
                    r = img_old->color_t->rgb[i].r / 255.0;
                    g = img_old->color_t->rgb[i].g / 255.0;
                    b = img_old->color_t->rgb[i].b / 255.0;
                   
                    rgb_to_hls (r, g, b, &h, &l, &s);
                    h = h / 360.0;

                    color.h = round (h * 255);
                    color.l = round (l * 255);
                    color.s = round (s * 255);
                    
		    ncolor = filter (&color);

                    h = ncolor.h * 360.0 / 255.0;
                    l = ncolor.l / 255.0;
                    s = ncolor.s / 255.0;

                    hls_to_rgb (h, l, s, &r, &g, &b);

		    img_new->color_t->rgb[i].r = round (r * 255);
		    img_new->color_t->rgb[i].g = round (g * 255);
		    img_new->color_t->rgb[i].b = round (b * 255);
		    }

                n = img_old->width * img_old->height;
		for (i = 0; i < n; i++)
		    img_new->data[i] = img_old->data[i];
		break;

	    case pgm:
		stat = img__invimgtyp;
		break;

	    case pbm:
		stat = img__invimgtyp;
		break;

	    default:
		stat = img__invimgtyp;
		break;
	    }
	}

if (status != NIL)
	*status = stat;
    else
	if (!odd (stat))
	    raise_exception (stat, 0, NULL, NULL);
}



void img_set_color (image_dscr *img, color_table *colort, int *status)

/*
 * img_set_color - set the colortable of a pcm image
 */

{
    int i, stat;

    stat = img__normal;

    if (img->type == pcm)
	{
	for (i = 0; i < colort->ncolors; i++)
	    img->color_t->rgb[i] = colort->rgb[i];

	img->color_t->ncolors = colort->ncolors;
	}
    else
	{
	stat = img__invimgtyp;
	}

    if (status != NIL)
	*status = stat;
    else
	if (!odd (stat))
	    raise_exception (stat, 0, NULL, NULL);
}



void img_get_color (image_dscr *img, color_table *colort, int *status)

/*
 *  img_get_color - get the colortable of a pcm image
 */

{
    int i, stat;

    stat = img__normal;

    if (img->type == pcm)
	{
	for (i = 0; i < img->color_t->ncolors; i++)
	    colort->rgb[i] = img->color_t->rgb[i];

	colort->ncolors = img->color_t->ncolors;
	}
    else
	{
	stat = img__invimgtyp;
	}

    if (status != NIL)
	*status = stat;
    else
	if (!odd (stat))
	    raise_exception (stat, 0, NULL, NULL);
}



void img_get_stdcolor (color_table *colort, color_tables name, int ncol,
    int *status)

/*
 *  img_get_stdcolor - get a predefined colortable
 */

{
    int i, stat;
    double x; 
    double r, g, b;
    double h, l, s;

    stat = img__normal;
    colort->ncolors = ncol;

    for (i = 0; i < ncol; i++)
	{
	x = i / (float)(ncol - 1);

	switch (name)
	    {
	    case uniform:
		h = x * 360.0 - 120;
		l = 0.5;
		s = 0.75;

		hls_to_rgb (h, l, s, &r, &g, &b);
		break;

	    case temperature:
		h = 270 - x * 300.0;
		l = 0.5;
		s = 0.75;

		hls_to_rgb (h, l, s, &r, &g, &b);
		break;

	    case grayscale:
		r = x;
		g = x;
		b = x;
		break;

	    case glowing:
		r = pow (x, 1./4.);
		g = x;
		b = pow (x, 4.);
		break;

	    case rainbow:
	    case flame:
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

		if (name == flame)
                    {
                    r = 1.0 - r; g = 1.0 - g; b = 1.0 - b;
                    }
                break;

	    case geologic:
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

	    case greenscale:
		r = x;
		g = pow (x, 1./4.);
		b = pow (x, 4.);
		break;

	    case cyanscale:
		r = pow (x, 4.);
		g = pow (x, 1./4.);
		b = x;
		break;

	    case bluescale:
		r = pow (x, 4.);
		g = x;
		b = pow (x, 1./4.);
		break;

	    case magentascale:
		r = x;
		g = pow (x, 4.);
		b = pow (x, 1./4.);
		break;

	    case redscale:
		r = pow (x, 1./4.);
		g = pow (x, 4.);
		b = x;

	    default:
		stat = img__invcolor;
		break;
	    }
	
	if (odd (stat))
	    {
	    colort->rgb[i].r = round (255 * r);
	    colort->rgb[i].g = round (255 * g);
	    colort->rgb[i].b = round (255 * b);
	    }
	}

    if (status != NIL)
	*status = stat;
    else
	if (!odd (stat))
	    raise_exception (stat, 0, NULL, NULL);
}



void img_flip (image_dscr *img_old, image_dscr *img_new, flip_direct direction,
    int *status)

/*
 *  img_flip - flips an image by the specified direction
 */

{
    register int i, j, k, l;
    int stat;

    stat = img__normal;

    switch (direction) {
	
	case horizontal:
	    switch (img_old->type) {

		case ppm:
		    img_create (img_new, ppm, img_old->width, img_old->height, 
			&stat);
		    if (!odd (stat)) break;

		    for (i = 0; i < img_old->height; i++)
			for (j = 0; j < img_old->width; j++)
			    {
			    k = 3 * (img_old->width * i + j);
			    l = 3 * (img_old->width * (img_old->height - 1 - i)
				+ j);
			    img_new->data[k] = img_old->data[l];
			    img_new->data[k+1] = img_old->data[l+1];
			    img_new->data[k+2] = img_old->data[l+2];
			    }
		    break;

		case pcm:
		    img_create (img_new, pcm, img_old->width, img_old->height, 
			&stat);
		    if (!odd (stat)) break;

		    img_new->color_t->ncolors = img_old->color_t->ncolors;
		    for (i = 0; i < img_new->color_t->ncolors; i++)
			img_new->color_t->rgb[i] = img_old->color_t->rgb[i];

		    for (i = 0; i < img_old->height; i++)
			for (j = 0; j < img_old->width; j++)
			    {
			    k = img_old->width * i + j;
			    l = img_old->width * (img_old->height - 1 - i) + j;
			    img_new->data[k] = img_old->data[l];
			    }
		    break;

		case pgm:
		    img_create (img_new, pgm, img_old->width, img_old->height, 
			&stat);
		    if (!odd (stat)) break;

		    img_new->maxgray = img_old->maxgray;

		    for (i = 0; i < img_old->height; i++)
			for (j = 0; j < img_old->width; j++)
			    {
			    k = img_old->width * i + j;
			    l = img_old->width * (img_old->height - 1 - i) +j;
			    img_new->data[k] = img_old->data[l];
			    }
		    break;

		case pbm:
		    img_create (img_new, pbm, img_old->width, img_old->height, 
			&stat);
		    if (!odd (stat)) break;

		    for (i = 0; i < img_old->height; i++)
			for (j = 0; j < img_old->width; j++)
			    {
			    k = img_old->width * i + j;
			    l = img_old->width * (img_old->height - 1 - i) + j;
			    
			    if (img_old->data[l>>03] & (01 << (7 - l % 8)))
				img_new->data[k>>03] |= (01 << (7 - k % 8));
			    }
		    break;

		default:
		    stat = img__invimgtyp;
		    break;
		}
	    break;

	case vertical:
	    switch (img_old->type) {

		case ppm:
		    img_create (img_new, ppm, img_old->width, img_old->height, 
			&stat);
		    if (!odd (stat)) break;

		    for (i = 0; i < img_old->height; i++)
			for (j = 0; j < img_old->width; j++)
			    {
			    k = 3 * (img_old->width * i + j);
			    l = 3 * (img_old->width * i + img_old->width - 1 -
				j);
			    img_new->data[k] = img_old->data[l];
			    img_new->data[k+1] = img_old->data[l+1];
			    img_new->data[k+2] = img_old->data[l+2];
			    }
		    break;

		case pcm:
		    img_create (img_new, pcm, img_old->width, img_old->height, 
			&stat);
		    if (!odd (stat)) break;

		    img_new->color_t->ncolors = img_old->color_t->ncolors;
		    for (i = 0; i < img_new->color_t->ncolors; i++)
			img_new->color_t->rgb[i] = img_old->color_t->rgb[i];

		    for (i = 0; i < img_old->height; i++)
			for (j = 0; j < img_old->width; j++)
			    {
			    k = img_old->width * i + j;
			    l = img_old->width * i + img_old->width - 1 - j;
			    img_new->data[k] = img_old->data[l];
			    }
		    break;

		case pgm:
		    img_create (img_new, pgm, img_old->width, img_old->height, 
			&stat);
		    if (!odd (stat)) break;

		    img_new->maxgray = img_old->maxgray;

		    for (i = 0; i < img_old->height; i++)
			for (j = 0; j < img_old->width; j++)
			    {
			    k = img_old->width * i + j;
			    l = img_old->width * i + img_old->width - 1 - j;
			    img_new->data[k] = img_old->data[l];
			    }
		    break;

		case pbm:
		    img_create (img_new, pbm, img_old->width, img_old->height, 
			&stat);
		    if (!odd (stat)) break;

		    for (i = 0; i < img_old->height; i++)
			for (j = 0; j < img_old->width; j++)
			    {
			    k = img_old->width * i + j;
			    l = img_old->width * img_old->width - 1 - j;
			    if (img_old->data[l>>03] & (01 << (7 - l % 8)))
				img_new->data[k>>03] |= (01 << (7 - k % 8));
			    }
		    break;

		default:
		    stat = img__invimgtyp;
		    break;
		}
	    break;

	default:
	    stat = img__invdirect;
	    break;
	}

    if (status != NIL)
        *status = stat;
    else
        if (!odd (stat))
            raise_exception (stat, 0, NULL, NULL);
}



void img_cut (image_dscr *img_old, image_dscr *img_new, int x1, int x2, int y1,
    int y2, int *status)

/*
 *  img_cut - cut a part of an image into a new one without changing the old
 *	      one
 */

{
    register int i, j, k, l;
    register int h, dx, dy;
    int stat;

    stat = img__normal;

    if (x1 > x2) { h = x1; x1 = x2; x2 = h; }
    if (y1 > y2) { h = y1; y1 = y2; y2 = h; }

    if (x2 > (img_old->width-1) || x1 < 0 || y2 > (img_old->height-1) || y1 < 0)
	stat = img__invdim;
    else
	{
	dx = x2 - x1 + 1;
	dy = y2 - y1 + 1;

	switch (img_old->type) {

	    case ppm:
		img_create (img_new, ppm, dx, dy, &stat);
		if (!odd (stat)) break;

		for (i = 0; i < dy; i++)
		    for (j = 0; j < dx; j++)
			{
			k = 3 * (dx * i + j);
			l = 3 * (img_old->width * (i + y1) + j + x1);
			img_new->data[k] = img_old->data[l];
			img_new->data[k+1] = img_old->data[l+1];
			img_new->data[k+2] = img_old->data[l+2];
			}
		break;

	    case pcm:
		img_create (img_new, pcm, dx, dy, &stat);
		if (!odd (stat)) break;

		img_new->color_t->ncolors = img_old->color_t->ncolors;
		for (i = 0; i < img_new->color_t->ncolors; i++)
		    img_new->color_t->rgb[i] = img_old->color_t->rgb[i];

		for (i = 0; i < dy; i++)
		    for (j = 0; j < dx; j++)
			{
			k = dx * i + j;
			l = img_old->width * (i + y1) + j + x1;
			img_new->data[k] = img_old->data[l];
			}
		break;

	    case pgm:
		img_create (img_new, pgm, dx, dy, &stat);
		if (!odd (stat)) break;

		img_new->maxgray = img_old->maxgray;

		for (i = 0; i < dy; i++)
		    for (j = 0; j < dx; j++)
			{
			k = dx * i + j;
			l = img_old->width * (i + y1) + j + x1;
			img_new->data[k] = img_old->data[l];
			}
		break;

	    case pbm:
		img_create (img_new, pbm, dx, dy, &stat);
		if (!odd (stat)) break;

		for (i = 0; i < dy; i++)
		    for (j = 0; j < dx; j++)
			{
			k = dx * i + j;
			l = img_old->width * (i + y1) + j + x1;
			if (img_old->data[l>>03] & (01 << (7 - l % 8)))
			    img_new->data[k>>03] |= (01 << (7 - k % 8));
			}
		break;

	    default:
		stat = img__invimgtyp;
		break;
	    }
	}

    if (status != NIL)
        *status = stat;
    else
        if (!odd (stat))
            raise_exception (stat, 0, NULL, NULL);
}



void img_edge (image_dscr *img_old, image_dscr *img_new, int *status)

/*
 *  img_edge - dectect edges in a ppm image
 */

{
    register int i, j, val;
    register double a,b,c, d,e, f,g,h;
    register double sum1, sum2, sum;
    int stat;

    stat = img__normal;

    switch (img_old->type) {

	case pgm:
	    img_create (img_new, pgm, img_old->width, img_old->height, &stat);
	    if (!odd (stat)) break;

	    img_new->maxgray = img_old->maxgray;

	    for (i = 1; i < img_old->height - 1; i++)
		{
		img_new->data[i * img_old->width] = 0;
		img_new->data[i * img_old->width + img_old->width - 1] = 0;

		for (j = 1; j < img_old->width - 1; j++)
		    {
		    a = (double)img_old->data[img_old->width * (i - 1) + j - 1];
		    b = (double)img_old->data[img_old->width * (i - 1) + j];
		    c = (double)img_old->data[img_old->width * (i - 1) + j + 1];

		    d = (double)img_old->data[img_old->width * i + j - 1];
		    e = (double)img_old->data[img_old->width * i + j + 1];

		    f = (double)img_old->data[img_old->width * (i + 1) + j - 1];
		    g = (double)img_old->data[img_old->width * (i + 1) + j];
		    h = (double)img_old->data[img_old->width * (i + 1) + j + 1];
		    
		    sum1 = c - a + 2.0 * (e - d) + h - f;
		    sum2 = (f + 2.0 * g + h) - (a + 2.0 * b + c);
		    sum = sqrt (sum1 * sum1 + sum2 * sum2) * 0.555555555;
		    val = round (sum);
		    if (val > img_old->maxgray) val = img_old->maxgray;

		    img_new->data[img_old->width * i + j] = val;
		    }
		}

	    for (i = 0; i < img_old->width; i++)
		{
		img_new->data[i] = 0;
		img_new->data[i + img_old->width * (img_old->height - 1)] = 0;
		}
	    break;

	default:
	    stat = img__invimgtyp;
	    break;
	}

    if (status != NIL)
        *status = stat;
    else
        if (!odd (stat))
            raise_exception (stat, 0, NULL, NULL);
}



void img_median (image_dscr *img_old, image_dscr *img_new, int *status)

/*
 *  img_median - perform a median filter on the image
 */

{
    register int i, j, val, ind, cind;
    register double a,b,c, d,e, f,g,h;
    int stat;
    image_dscr img_tmp;

    stat = img__normal;

    switch (img_old->type) {

	case ppm:
	    img_create (img_new, ppm, img_old->width, img_old->height, &stat);
	    if (!odd (stat)) break;

	    for (i = 1; i < img_old->height - 1; i++)
		{
		for (cind = 0; cind < 3; cind++)
		    {
		    j = 0;

		    ind = 3 * (img_old->width * (i - 1) + j) + cind;
		    b = (double)img_old->data[ind];

		    ind = 3 * (img_old->width * (i - 1) + j + 1) + cind;
		    c = (double)img_old->data[ind];

		    ind = 3 * (img_old->width * i + j + 1) + cind;
		    e = (double)img_old->data[ind];

		    ind = 3 * (img_old->width * (i + 1) + j) + cind;
		    g = (double)img_old->data[ind];

		    ind = 3 * (img_old->width * (i + 1) + j + 1) + cind;
		    h = (double)img_old->data[ind];

		    val = round ((b + c + e + g + h) * 0.2);

		    ind = 3 * (img_old->width * i + j) + cind;
		    img_new->data[ind] = val;

		    j = img_old->width - 1;

		    ind = 3 * (img_old->width * (i - 1) + j - 1) + cind;
		    a = (double)img_old->data[ind];

		    ind = 3 * (img_old->width * (i - 1) + j) + cind;
		    b = (double)img_old->data[ind];

		    ind = 3 * (img_old->width * i + j - 1) + cind;
		    d = (double)img_old->data[ind];

		    ind = 3 * (img_old->width * (i + 1) + j - 1) + cind;
		    f = (double)img_old->data[ind];

		    ind = 3 * (img_old->width * (i + 1) + j) + cind;
		    g = (double)img_old->data[ind];

		    val = round ((a + b + d + f + g) * 0.2);

		    ind = 3 * (img_old->width * i + j) + cind;
		    img_new->data[ind] = val;
		    }

		for (j = 1; j < img_old->width - 1; j++)
		    {
		    for (cind = 0; cind < 3; cind++)
			{
			ind = 3 * (img_old->width * (i - 1) + j - 1) + cind;
			a = (double)img_old->data[ind];
			ind = 3 * (img_old->width * (i - 1) + j) + cind;
			b = (double)img_old->data[ind];
			ind = 3 * (img_old->width * (i - 1) + j + 1) + cind;
			c = (double)img_old->data[ind];

			ind = 3 * (img_old->width * i + j - 1) + cind;
			d = (double)img_old->data[ind];
			ind = 3 * (img_old->width * i + j + 1) + cind;
			e = (double)img_old->data[ind];

			ind = 3 * (img_old->width * (i + 1) + j - 1) + cind;
			f = (double)img_old->data[ind];
			ind = 3 * (img_old->width * (i + 1) + j) + cind;
			g = (double)img_old->data[ind];
			ind = 3 * (img_old->width * (i + 1) + j + 1) + cind;
			h = (double)img_old->data[ind];
			
			val = round ((a + b + c + d + e + f + g + h) * 0.125);

			ind = 3 * (img_old->width * i + j) + cind;
			img_new->data[ind] = val;
			}
		    }
		}

	    for (j = 1; j < img_old->width - 1; j++)
		{
		for (cind = 0; cind < 3; cind++)
		    {
		    i = 0;		

		    ind = 3 * (img_old->width * i + j - 1) + cind;
		    d = (double)img_old->data[ind];

		    ind = 3 * (img_old->width * i + j + 1) + cind;
		    e = (double)img_old->data[ind];

		    ind = 3 * (img_old->width * (i + 1) + j - 1) + cind;
		    f = (double)img_old->data[ind];

		    ind = 3 * (img_old->width * (i + 1) + j) + cind;
		    g = (double)img_old->data[ind];

		    ind = 3 * (img_old->width * (i + 1) + j + 1) + cind;
		    h = (double)img_old->data[ind];

		    val = round ((d + e + f + g + h) * 0.2);

		    ind = 3 * (img_old->width * i + j) + cind;
		    img_new->data[ind] = val;

		    i = img_old->height - 1;

		    ind = 3 * (img_old->width * (i - 1) + j - 1) + cind;
		    a = (double)img_old->data[ind];

		    ind = 3 * (img_old->width * (i - 1) + j) + cind;
		    b = (double)img_old->data[ind];

		    ind = 3 * (img_old->width * (i - 1) + j + 1) + cind;
		    c = (double)img_old->data[ind];

		    ind = 3 * (img_old->width * i + j - 1) + cind;
		    d = (double)img_old->data[ind];

		    ind = 3 * (img_old->width * i + j + 1) + cind;
		    e = (double)img_old->data[ind];

		    val = round ((a + b + c + d + e) * 0.2);

		    ind = 3 * (img_old->width * i + j) + cind;
		    img_new->data[ind] = val;
		    }
		}

	    for (cind = 0; cind < 3; cind++)
		{
		i = 0;
		j = 0;

		ind = 3 * (img_old->width * i + j + 1) + cind;
		e = (double)img_old->data[ind];

		ind = 3 * (img_old->width * (i + 1) + j) + cind;
		g = (double)img_old->data[ind];

		ind = 3 * (img_old->width * (i + 1) + j + 1) + cind;
		h = (double)img_old->data[ind];

		val = round ((e + g + h) * 0.33333333);

		ind = 3 * (img_old->width * i + j) + cind;
		img_new->data[ind] = val;

		i = 0;
		j = img_old->width - 1;

		ind = 3 * (img_old->width * i + j - 1) + cind;
		d = (double)img_old->data[ind];

		ind = 3 * (img_old->width * (i + 1) + j - 1) + cind;
		f = (double)img_old->data[ind];

		ind = 3 * (img_old->width * (i + 1) + j) + cind;
		g = (double)img_old->data[ind];
		
		val = round ((e + g + h) * 0.33333333);

		ind = 3 * (img_old->width * i + j) + cind;
		img_new->data[ind] = val;

		i = img_old->height - 1;
		j = 0;

		ind = 3 * (img_old->width * (i - 1) + j) + cind;
		b = (double)img_old->data[ind];

		ind = 3 * (img_old->width * (i - 1) + j + 1) + cind;
		c = (double)img_old->data[ind];

		ind = 3 * (img_old->width * i + j + 1) + cind;
		e = (double)img_old->data[ind];

		val = round ((e + g + h) * 0.33333333);

		ind = 3 * (img_old->width * i + j) + cind;
		img_new->data[ind] = val;
	       
		i = img_old->height - 1;
		j = img_old->width - 1;

		ind = 3 * (img_old->width * (i - 1) + j - 1) + cind;
		a = (double)img_old->data[ind];

		ind = 3 * (img_old->width * (i - 1) + j) + cind;
		b = (double)img_old->data[ind];

		ind = 3 * (img_old->width * i + j - 1) + cind;
		d = (double)img_old->data[ind];
		
		val = round ((e + g + h) * 0.33333333);

		ind = 3 * (img_old->width * i + j) + cind;
		img_new->data[ind] = val;
		}
	    break;

	case pcm:
	    img_ppm (img_old, &img_tmp, &stat);
	    if (odd (stat))
		img_median (&img_tmp, img_new, &stat);
	    img_free (&img_tmp);
	    break;

	case pgm:
	    img_create (img_new, pgm, img_old->width, img_old->height, &stat);
	    if (!odd (stat)) break;

	    img_new->maxgray = img_old->maxgray;

	    for (i = 1; i < img_old->height - 1; i++)
		{
		j = 0;

		ind = img_old->width * (i - 1) + j;
		b = (double)img_old->data[ind];

		ind = img_old->width * (i - 1) + j + 1;
		c = (double)img_old->data[ind];

		ind = img_old->width * i + j + 1;
		e = (double)img_old->data[ind];

		ind = img_old->width * (i + 1) + j;
		g = (double)img_old->data[ind];

		ind = img_old->width * (i + 1) + j + 1;
		h = (double)img_old->data[ind];

		val = round ((b + c + e + g + h) * 0.2);

		ind = img_old->width * i + j;
		img_new->data[ind] = val;

		j = img_old->width - 1;

		ind = img_old->width * (i - 1) + j - 1;
		a = (double)img_old->data[ind];

		ind = img_old->width * (i - 1) + j;
		b = (double)img_old->data[ind];

		ind = img_old->width * i + j - 1;
		d = (double)img_old->data[ind];

		ind = img_old->width * (i + 1) + j - 1;
		f = (double)img_old->data[ind];

		ind = img_old->width * (i + 1) + j;
		g = (double)img_old->data[ind];

		val = round ((a + b + d + f + g) * 0.2);

		ind = img_old->width * i + j;
		img_new->data[ind] = val;

		for (j = 1; j < img_old->width - 1; j++)
		    {
		    ind = img_old->width * (i - 1) + j - 1;
		    a = (double)img_old->data[ind];
		    ind = img_old->width * (i - 1) + j;
		    b = (double)img_old->data[ind];
		    ind = img_old->width * (i - 1) + j + 1;
		    c = (double)img_old->data[ind];

		    ind = img_old->width * i + j - 1;
		    d = (double)img_old->data[ind];
		    ind = img_old->width * i + j + 1;
		    e = (double)img_old->data[ind];

		    ind = img_old->width * (i + 1) + j - 1;
		    f = (double)img_old->data[ind];
		    ind = img_old->width * (i + 1) + j;
		    g = (double)img_old->data[ind];
		    ind = img_old->width * (i + 1) + j + 1;
		    h = (double)img_old->data[ind];
		    
		    val = round ((a + b + c + d + e + f + g + h) * 0.125);

		    ind = img_old->width * i + j;
		    img_new->data[ind] = val;
		    }
		}

	    for (j = 1; j < img_old->width - 1; j++)
		{
		i = 0;		

		ind = img_old->width * i + j - 1;
		d = (double)img_old->data[ind];

		ind = img_old->width * i + j + 1;
		e = (double)img_old->data[ind];

		ind = img_old->width * (i + 1) + j - 1;
		f = (double)img_old->data[ind];

		ind = img_old->width * (i + 1) + j;
		g = (double)img_old->data[ind];

		ind = img_old->width * (i + 1) + j + 1;
		h = (double)img_old->data[ind];

		val = round ((d + e + f + g + h) * 0.2);

		ind = img_old->width * i + j;
		img_new->data[ind] = val;

		i = img_old->height - 1;

		ind = img_old->width * (i - 1) + j - 1;
		a = (double)img_old->data[ind];

		ind = img_old->width * (i - 1) + j;
		b = (double)img_old->data[ind];

		ind = img_old->width * (i - 1) + j + 1;
		c = (double)img_old->data[ind];

		ind = img_old->width * i + j - 1;
		d = (double)img_old->data[ind];

		ind = img_old->width * i + j + 1;
		e = (double)img_old->data[ind];

		val = round ((a + b + c + d + e) * 0.2);

		ind = img_old->width * i + j;
		img_new->data[ind] = val;
		}

	    i = 0;
	    j = 0;

	    ind = img_old->width * i + j + 1;
	    e = (double)img_old->data[ind];

	    ind = img_old->width * (i + 1) + j;
	    g = (double)img_old->data[ind];

	    ind = img_old->width * (i + 1) + j + 1;
	    h = (double)img_old->data[ind];

	    val = round ((e + g + h) * 0.33333333);

	    ind = img_old->width * i + j;
	    img_new->data[ind] = val;

	    i = 0;
	    j = img_old->width - 1;

	    ind = img_old->width * i + j - 1;
	    d = (double)img_old->data[ind];

	    ind = img_old->width * (i + 1) + j - 1;
	    f = (double)img_old->data[ind];

	    ind = img_old->width * (i + 1) + j;
	    g = (double)img_old->data[ind];
            
	    val = round ((e + g + h) * 0.33333333);

	    ind = img_old->width * i + j;
	    img_new->data[ind] = val;

	    i = img_old->height - 1;
	    j = 0;

	    ind = img_old->width * (i - 1) + j;
	    b = (double)img_old->data[ind];

	    ind = img_old->width * (i - 1) + j + 1;
	    c = (double)img_old->data[ind];

	    ind = img_old->width * i + j + 1;
	    e = (double)img_old->data[ind];

 	    val = round ((e + g + h) * 0.33333333);

	    ind = img_old->width * i + j;
	    img_new->data[ind] = val;
           
            i = img_old->height - 1;
            j = img_old->width - 1;

	    ind = img_old->width * (i - 1) + j - 1;
	    a = (double)img_old->data[ind];

	    ind = img_old->width * (i - 1) + j;
	    b = (double)img_old->data[ind];

	    ind = img_old->width * i + j - 1;
	    d = (double)img_old->data[ind];
            
	    val = round ((e + g + h) * 0.33333333);

	    ind = img_old->width * i + j;
	    img_new->data[ind] = val;
	    break;

	case pbm:
	    img_pgm (img_old, &img_tmp, 255, &stat);
	    if (odd (stat))
		img_median (&img_tmp, img_new, &stat);
	    img_free (&img_tmp);
	    break;

 	default:
	    stat = img__invimgtyp;
	    break;
	}

    if (status != NIL)
        *status = stat;
    else
        if (!odd (stat))
            raise_exception (stat, 0, NULL, NULL);
}



void img_enhance (image_dscr *img_old, image_dscr *img_new, double level,
    int *status)

/*
 *  img_enhance - enhance the edges of an image
 */

{
    register int j, k, val, cind, ind;
    register double a,b,c, d,e,f, g,h,i;
    register double sum;
    image_dscr img_tmp;
    int stat;

    stat = img__normal;

    switch (img_old->type) {

	case pcm:
	    img_ppm (img_old, &img_tmp, &stat);
	    if (odd (stat))
		img_enhance (&img_tmp, img_new, level, &stat);
	    img_free (&img_tmp);
	    break;

	case pgm:
	    if (level < 0 || level > 1.0)
		stat = img__invenhance;
	    else
		img_create (img_new, pgm, img_old->width, img_old->height, 
		    &stat);

	    if (!odd (stat)) break;

	    img_new->maxgray = img_old->maxgray;

	    for (j = 1; j < img_old->height - 1; j++)
		{
		img_new->data[j * img_old->width] = 
		    img_old->data[j * img_old->width];
		img_new->data[j * img_old->width + img_old->width - 1] =
		    img_old->data[j * img_old->width + img_old->width - 1];

		for (k = 1; k < img_old->width - 1; k++)
		    {
		    a = (double)img_old->data[img_old->width * (j - 1) + k - 1];
		    b = (double)img_old->data[img_old->width * (j - 1) + k];
		    c = (double)img_old->data[img_old->width * (j - 1) + k + 1];

		    d = (double)img_old->data[img_old->width * j + k - 1];
		    e = (double)img_old->data[img_old->width * j + k];
		    f = (double)img_old->data[img_old->width * j + k + 1];

		    g = (double)img_old->data[img_old->width * (j + 1) + k - 1];
		    h = (double)img_old->data[img_old->width * (j + 1) + k];
		    i = (double)img_old->data[img_old->width * (j + 1) + k + 1];
		    
		    sum = a + b + c + d + e + f + g + h + i;

		    if (1.0-level != 0)
                    {
 		        val = round ((e - level * sum * 0.1) / (1.0 - level));
		        if (val > img_old->maxgray) val = img_old->maxgray;
		        if (val < 0) val = 0;
                    }
                    else 
                        if (e - level*sum*0.1 > 0)
			    val = img_old->maxgray; 
			else
			    val = 0;

		    img_new->data[img_old->width * j + k] = val;
		    }
		}

	    for (j = 0; j < img_old->width; j++)
		{
		img_new->data[j] = img_old->data[j];
		img_new->data[j + img_old->width * (img_old->height - 1)] = 
		    img_old->data[j + img_old->width * (img_old->height - 1)];
		}
	    break;

	case ppm:
	    if (level < 0 || level > 1.0)
		stat = img__invenhance;
            else
	        img_create (img_new, ppm, img_old->width, img_old->height, 
                    &stat);
	    if (!odd (stat)) break;

	    for (j = 1; j < img_old->height - 1; j++)
		{
		for (cind = 0; cind < 3; cind++)
                    {
		    img_new->data[j * img_old->width + cind] = 
		        img_old->data[j * img_old->width + cind];
		    img_new->data[j * img_old->width + img_old->width - 1 + cind] =
		        img_old->data[j * img_old->width + img_old->width - 1 +cind];
                    }
		for (k = 1; k < img_old->width - 1; k++)
		    {
		    for (cind = 0; cind < 3; cind++)
			{
			ind = 3 * (img_old->width * (j - 1) + k - 1) + cind;
			a = (double)img_old->data[ind];
			ind = 3 * (img_old->width * (j - 1) + k) + cind;
			b = (double)img_old->data[ind];
			ind = 3 * (img_old->width * (j - 1) + k + 1) + cind;
			c = (double)img_old->data[ind];

			ind = 3 * (img_old->width * j + k - 1) + cind;
			d = (double)img_old->data[ind];
			ind = 3 * (img_old->width * j + k) + cind;
			e = (double)img_old->data[ind];
			ind = 3 * (img_old->width * j + k + 1) + cind;
			f = (double)img_old->data[ind];

			ind = 3 * (img_old->width * (j + 1) + k - 1) + cind;
			g = (double)img_old->data[ind];
			ind = 3 * (img_old->width * (j + 1) + k) + cind;
			h = (double)img_old->data[ind];
			ind = 3 * (img_old->width * (j + 1) + k + 1) + cind;
			i = (double)img_old->data[ind];

		    sum = a + b + c + d + e + f + g + h + i;
		    if (1.0-level != 0.)
                    {
 		        val = round ((e - level * sum * 0.1) / (1.0 - level));
		        if (val > 255) val = 255;
		        if (val < 0) val = 0;
                    }
                    else 
                        if (e - level * sum * 0.1 > 0)
			    val = 255; 
			else
			    val = 0;

			ind = 3 * (img_old->width * j + k) + cind;
			img_new->data[ind] = val;
			}
		    }
		}

	    for (j = 0; j < img_old->width; j++)
                {
		for (cind = 0; cind < 3; cind++)
		    img_new->data[j + cind] = img_old->data[j + cind];
		img_new->data[j+cind + img_old->width*(img_old->height-1)] = 
		    img_old->data[j+cind + img_old->width*(img_old->height-1)];
                }
	    break;

	default:
	    stat = img__invimgtyp;
	    break;
	}

    if (status != NIL)
        *status = stat;
    else
        if (!odd (stat))
            raise_exception (stat, 0, NULL, NULL);
}



static
BYTE filter_fun (BYTE glevel)
{
    return (filter_tab[(int) glevel]);
}



static
hls_color hls_filter_fun (hls_color *color)
{
    static hls_color ncolor;
    
    ncolor.h = hls_filter_tab[(int) color->h].h;
    ncolor.l = hls_filter_tab[(int) color->l].l;
    ncolor.s = hls_filter_tab[(int) color->s].s;

    return (ncolor);
}



void img_contrast (image_dscr *img_old, image_dscr *img_new, double level,
    int *status)

/*
 *  img_contrast - modify the contrast an image
 */

{
    int stat;
    register int i, j;
    register double m, b;

    stat = img__normal;

    switch (img_old->type) {

	case pgm:
	    if (level < -1. || level > 1.)
		stat = img__invcontr;
	    else
		{
                m = tan ((1. + level) * PI * 0.25);
		b = 0.5 * (1. - m);
		
		for (i = 0; i < 256; i++)
		    {
		    j = round (m * i + b);

		    if (j < 0)
			j = 0;
		    if (j > 255)
			j = 255;

		    filter_tab[i] = (BYTE)j;
		    }

		img_filter (img_old, img_new, filter_fun, &stat);
		}
	    break;

	case ppm:
	case pcm:
	    if (level < -1. || level > 1.)
		stat = img__invcontr;
	    else
		{
                m = tan ((1. + level) * PI * 0.25);
		b = 0.5 * (1. - m);
		
		for (i = 0; i < 256; i++)
		    {
		    j = round (m * i + b);
		    if (j < 0)
			j = 0;
		    if (j > 255)
			j = 255;
                    hls_filter_tab[i].h = i;
		    hls_filter_tab[i].l = (BYTE)j;
                    hls_filter_tab[i].s = i;
		    }

		img_hlsfilter (img_old, img_new, hls_filter_fun, &stat);
		}
	    break;
    
	default:
	    stat = img__invimgtyp;
	    break;
	}

    if (status != NIL)
        *status = stat;
    else
        if (!odd (stat))
            raise_exception (stat, 0, NULL, NULL);
}



void img_gradx (image_dscr *img_old, image_dscr *img_new, int *status)

/*
 *  img_gradx - Gradient filter for pgm images in x direction
 */

{
    register int j, k, val;
    register double a,c, d,f, g,i;
    register double sum;
    int stat;

    stat = img__normal;

    switch (img_old->type) {

	case pgm:
            img_create (img_new, pgm, img_old->width, img_old->height, &stat);

	    if (!odd (stat)) break;

	    img_new->maxgray = img_old->maxgray;

	    for (j = 1; j < img_old->height - 1; j++)
		{
		img_new->data[j * img_old->width] =
		    (BYTE)(img_old->maxgray * 0.5);
		img_new->data[j * img_old->width + img_old->width - 1] =
		    (BYTE)(img_old->maxgray * 0.5);

		for (k = 1; k < img_old->width - 1; k++)
		    {
		    a = (double)img_old->data[img_old->width * (j - 1) + k - 1];
		    c = (double)img_old->data[img_old->width * (j - 1) + k + 1];

		    d = (double)img_old->data[img_old->width * j + k - 1];
		    f = (double)img_old->data[img_old->width * j + k + 1];

		    g = (double)img_old->data[img_old->width * (j + 1) + k - 1];
		    i = (double)img_old->data[img_old->width * (j + 1) + k + 1];
		    
                    sum = -a + c - 2 * d + 2 * f - g + i;
		    val = round (sum + img_old->maxgray * 0.5);
		    if (val > img_old->maxgray) val = img_old->maxgray;
		    if (val < 0) val = 0;

		    img_new->data[img_old->width * j + k] = val;
		    }
		}

	    for (j = 0; j < img_old->width; j++)
		{
		img_new->data[j] = (BYTE)(img_old->maxgray * 0.5);
		img_new->data[j + img_old->width * (img_old->height - 1)] = 
		    (BYTE)(img_old->maxgray * 0.5);
		}
	    break;

	default:
	    stat = img__invimgtyp;
	    break;
	}

    if (status != NIL)
        *status = stat;
    else
        if (!odd (stat))
            raise_exception (stat, 0, NULL, NULL);
}



void img_grady (image_dscr *img_old, image_dscr *img_new, int *status)

/*
 *  img_grady - Gradient filter for pgm images in y direction
 */

{
    register int j, k, val;
    register double a,b,c, g,h,i;
    register double sum;
    int stat;

    stat = img__normal;

    switch (img_old->type) {

	case pgm:
            img_create (img_new, pgm, img_old->width, img_old->height, &stat);

	    if (!odd (stat)) break;

	    img_new->maxgray = img_old->maxgray;

	    for (j = 1; j < img_old->height - 1; j++)
		{
		img_new->data[j * img_old->width] =
		    (BYTE)(img_old->maxgray * 0.5);
		img_new->data[j * img_old->width + img_old->width - 1] =
		    (BYTE)(img_old->maxgray * 0.5);

		for (k = 1; k < img_old->width - 1; k++)
		    {
		    a = (double)img_old->data[img_old->width * (j - 1) + k - 1];
		    b = (double)img_old->data[img_old->width * (j - 1) + k];
		    c = (double)img_old->data[img_old->width * (j - 1) + k + 1];

		    g = (double)img_old->data[img_old->width * (j + 1) + k - 1];
		    h = (double)img_old->data[img_old->width * (j + 1) + k];
		    i = (double)img_old->data[img_old->width * (j + 1) + k + 1];
		    
                    sum = a + 2 * b + c - g - 2 * h - i; 
		    val = round (sum + img_old->maxgray * 0.5);
		    if (val > img_old->maxgray) val = img_old->maxgray;
		    if (val < 0) val = 0;

		    img_new->data[img_old->width * j + k] = val;
		    }
		}

	    for (j = 0; j < img_old->width; j++)
		{
		img_new->data[j] = (BYTE)(img_old->maxgray * 0.5);
		img_new->data[j + img_old->width * (img_old->height - 1)] = 
		    (BYTE)(img_old->maxgray * 0.5);
		}
	    break;

	default:
	    stat = img__invimgtyp;
	    break;
	}

    if (status != NIL)
        *status = stat;
    else
        if (!odd (stat))
            raise_exception (stat, 0, NULL, NULL);
}



void img_norm (image_dscr *img_old, image_dscr *img_new, int *status)

/*
 *  img_norm - normalize a pgm image so that all values will range from 0 to
 *             maximum gray value
 */

{
    register int i, j;
    register int gmin, gmax, lmin, lmax, il;
    register double fac;
    double r, g, b, h, l, s;
    int stat;

    stat = img__normal;

    switch (img_old->type) {

	case pgm:
            img_create (img_new, pgm, img_old->width, img_old->height, &stat);

	    if (!odd (stat)) break;

            gmin = img_old->maxgray;
            gmax = 0;
            j = img_old->width * img_old->height;

            for (i = 0; i < j; i++)
                {
                gmin = min (img_old->data[i], gmin);
                gmax = max (img_old->data[i], gmax);
                }

	    img_new->maxgray = gmax - gmin;
            for (i = 0; i < j; i++)
                img_new->data[i] = img_old->data[i] - gmin;
	    break;

	case ppm:
            lmin = 255;
            lmax = 0;
            j = img_old->width * img_old->height;

            for (i = 0; i < j; i++)
                {
                r = img_old->data[3*i] / 255.0;
                g = img_old->data[3*i+1] / 255.0;
                b = img_old->data[3*i+2] / 255.0;
                rgb_to_hls (r, g, b, &h, &l, &s);
                il = round (l * 255.0);
                lmin = min (lmin, il);
                lmax = max (lmax, il);
                }

            fac = 255.0 / (lmax - lmin);
            for (i = 0; i < 256; i++)
                {
                hls_filter_tab[i].h = i;
                hls_filter_tab[i].l = round ((i - lmin) * fac);
                hls_filter_tab[i].s = i;
                }

            img_hlsfilter (img_old, img_new, hls_filter_fun, &stat);
	    break;

	case pcm:
            lmin = 255;
            lmax = 0;
            j = img_old->width * img_old->height;

            for (i = 0; i < j; i++)
                {
                r = img_old->color_t->rgb[img_old->data[i]].r / 255.0;
                g = img_old->color_t->rgb[img_old->data[i]].g / 255.0;
                b = img_old->color_t->rgb[img_old->data[i]].b / 255.0;
                rgb_to_hls (r, g, b, &h, &l, &s);
                il = round (l * 255.0);
                lmin = min (lmin, il);
                lmax = max (lmax, il);
                }

            fac = 255.0 / (lmax - lmin);
            for (i = 0; i < 256; i++)
                {
                hls_filter_tab[i].h = i;
                hls_filter_tab[i].l = round ((i - lmin) * fac);
                hls_filter_tab[i].s = i;
                }

            img_hlsfilter (img_old, img_new, hls_filter_fun, &stat);
	    break;

	default:
	    stat = img__invimgtyp;
	    break;
	}

    if (status != NIL)
        *status = stat;
    else
        if (!odd (stat))
            raise_exception (stat, 0, NULL, NULL);
}



static
void build_hist (BYTE *data, int ndata, int nclass, int *hist)
{
    int i;

    for (i = 0; i < nclass; i++)
        hist[i] = 0;
    for (i = 0; i < ndata; i++)
        hist[data[i]]++;
}



void img_histnorm (image_dscr *img_old, image_dscr *img_new, int *status)

/*
 *  img_histnorm - normalize the intensity histogram of an image
 */

{
    register int i;
    register int ndata;
    int hist[256];
    BYTE *data;
    double r, g, b, h, l, s;
    int stat;

    stat = img__normal;

    switch (img_old->type) {

	case pgm:
            ndata = img_old->width * img_old->height;

            build_hist (img_old->data, ndata, img_old->maxgray + 1, hist);

            for (i = 1; i <= img_old->maxgray; i++)
                hist[i] = hist[i-1] + hist[i];

            for (i = 0; i <= img_old->maxgray; i++)
                filter_tab[i] = round ((float)hist[i] / (float)ndata * 
                    img_old->maxgray);

            img_filter (img_old, img_new, filter_fun, &stat);
	    break;

	case ppm:
            ndata = img_old->width * img_old->height;
            data = (BYTE *) malloc (ndata);
            if (data == NULL)
                {
                stat = img__nomem;
                break;
                }

            for (i = 0; i < ndata; i++)
                {
                r = img_old->data[3*i] / 255.0;
                g = img_old->data[3*i+1] / 255.0;
                b = img_old->data[3*i+2] / 255.0;
                rgb_to_hls (r, g, b, &h, &l, &s);
                data[i] = round (l * 255.0);
                }            
    
            build_hist (data, ndata, 256, hist);

            for (i = 1; i < 256; i++)
                hist[i] = hist[i-1] + hist[i];

            for (i = 0; i < 256; i++)
                {
                hls_filter_tab[i].h = i;
		hls_filter_tab[i].l = round ((float)hist[i] / (float)ndata *
                    255.0);
                hls_filter_tab[i].s = i;
                }

            img_hlsfilter (img_old, img_new, hls_filter_fun, &stat);
            free (data);
	    break;

	case pcm:
            ndata = img_old->width * img_old->height;
            data = (BYTE *) malloc (ndata);
            if (data == NULL)
                {
                stat = img__nomem;
                break;
                }

            for (i = 0; i <= ndata; i++)
                {
                r = img_old->color_t->rgb[img_old->data[i]].r / 255.0;
                g = img_old->color_t->rgb[img_old->data[i]].g / 255.0;
                b = img_old->color_t->rgb[img_old->data[i]].b / 255.0;
                rgb_to_hls (r, g, b, &h, &l, &s);
                data[i] = round (l * 255.0);
                }            
    
            build_hist (data, ndata, 256, hist);

            for (i = 1; i < 256; i++)
                hist[i] = hist[i-1] + hist[i];

            for (i = 0; i < 256; i++)
                {
                hls_filter_tab[i].h = i;
		hls_filter_tab[i].l = round ((float)hist[i] / (float)ndata *
                    255.0);
                hls_filter_tab[i].s = i;
                }

            img_hlsfilter (img_old, img_new, hls_filter_fun, &stat);
            free (data);
	    break;

	default:
	    stat = img__invimgtyp;
	    break;
	}

    if (status != NIL)
        *status = stat;
    else
        if (!odd (stat))
            raise_exception (stat, 0, NULL, NULL);
}



void img_fft (image_dscr *img_old, image_dscr *img_new, int sign, int *status)

/*
 *  img_fft - perform a fourier-transformation on an image
 */

{
    int stat;
    register int i, j, dimx, dimy, width, height, d;
    register BYTE *data;
    register float *coeff;
    
    stat = img__normal;

    if (img_old->type == pgm)
	{
	img_copy (img_old, img_new, &stat);

	if (odd (stat))
	    {
	    if (sign == 1)		/* forward transform */
	        {
	        width = img_old->width;
	        for (dimx = 2; dimx < width; dimx *= 2)
		    ;
	        height = img_old->height;
	        for (dimy = 2; dimy < height; dimy *= 2)
		    ;

	        if (img_new->coeff != NULL)
		    {
		    free (img_new->coeff);
		    img_new->coeff = NULL;
		    }

	        img_new->dimx = dimx;
	        img_new->dimy = dimy;
	        img_new->coeff = (float *) malloc (dimx * dimy * 2 *
		    sizeof(float));

	        if (img_new->coeff != NULL)
		    {
		    data = img_old->data;
		    coeff = img_new->coeff;
		    for (j = 0; j < height; j++)
		        {
		        for (i = 0; i < width; i++)
			    {
			    *coeff++ = (float) *data++;
			    *coeff++ = 0.0;
			    }
		        for (; i < dimx; i++)
			    {
			    *coeff++ = 0.0;
			    *coeff++ = 0.0;
			    }
		        }
		    for (; j < dimy; j++)
		        for (i = 0; i < dimx; i++)
			    {
			    *coeff++ = 0.0;
			    *coeff++ = 0.0;
			    }

		    mth_fft2 (img_new->coeff, dimx, dimy, 1);
		    }
	        else
		    stat = img__nomem;
	        }
	    else			/* backward transform */
	        {
	        if (img_old->coeff != NULL)
		    {
		    width = img_old->width;
		    height = img_old->height;

		    dimx = img_new->dimx;
		    dimy = img_new->dimy;
		    mth_fft2 (img_new->coeff, dimx, dimy, -1);

		    coeff = img_new->coeff;
		    data = img_new->data;

		    for (j = 0; j < height; j++)
		        {
		        for (i = 0; i < width; i++)
			    {
			    d = (int) (*coeff + 0.5);
			    *data++ = (d < 0) ? 0 : (d > 255) ? 255 : (BYTE) d;
			    coeff += 2;
			    }
		        for (; i < dimx; i++)
			    coeff += 2;
		        }

		    free (img_new->coeff);
		    img_new->coeff = NULL;
		    }
	        else
		    stat = img__invimgtyp;
	        }
	    }
	else
	    stat = img__nomem;
	}
    else
	stat = img__invimgtyp;

    if (status != NIL)
	*status = stat;
    else
	if (!odd (stat))
	    raise_exception (stat, 0, NULL, NULL);
}



void img_apply_filter (image_dscr *img_old, image_dscr *img_new, int low,
    int high, int level0, int level1, int *status)
{
    register int i, j;
    register float *coeff;
    register int centerx, centery, dx, dy, d;
    int stat;

    centerx = img_old->dimx / 2;
    centery = img_old->dimy / 2;

    if (img_old->type == pgm)
	{
	img_copy (img_old, img_new, &stat);

	if (odd(stat))
	    {
	    coeff = img_new->coeff;
	    for (i = 0; i < img_new->dimy; i++)
		{
		dy = centery - i;
		for (j = 0; j < img_new->dimx; j++)
		    {
		    dx = centerx - j;
                    d = (int) (sqrt((double) (dx*dx + dy*dy)) + 0.5);

		    if (d >= low && d <= high)
			{
                        *coeff++ *= level1;
                        *coeff++ *= level1;
			}
		    else
			{
                        *coeff++ *= level0;
                        *coeff++ *= level0;
			}
		    }
		}
	    }
	}
    else
	stat = img__invimgtyp;

    if (status != NIL)
	*status = stat;
    else
	if (!odd (stat))
	    raise_exception (stat, 0, NULL, NULL);
}
