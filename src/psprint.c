/*
 * Copyright @ 1994 - 1998   Josef Heinen
 *
 * Permission to use, copy, and distribute this software and its
 * documentation for any purpose with or without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.
 *
 * Permission to modify the software is granted, but not the right to
 * distribute the modified code.  Modifications are to be distributed
 * as patches to released version.
 *
 * This software is provided "as is" without express or implied warranty.
 *
 * Send your comments or suggestions to
 *  J.Heinen@kfa-juelich.de.
 *
 *
 * FACILITY:
 *
 *      GLI GKS V4.5
 *
 * ABSTRACT:
 *
 *      This module contains a logical device driver for Display PostScript.
 *
 * AUTHOR:
 *
 *      Josef Heinen
 *
 * VERSION:
 *
 *      V1.0
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#if !defined(VMS) && !defined(MSDOS) && !defined(_WIN32)
#include <unistd.h>
#endif

#include <sys/types.h>

#if defined (cray) || defined (__SVR4) || defined(MSDOS) || defined(_WIN32)
#include <fcntl.h>
#else
#include <sys/file.h>
#endif

#include <X11/Xlib.h>

#include <DPS/dpsXclient.h>

#include "gksdefs.h"

#define max(a,b) (((a) > (b)) ? (a) : (b))

#define usage "Usage:  %s [-width w] [-height h] [-sixel] [-gif]\n"

#define DCS "\220"			/* Device Control String */
#define ST  "\234"			/* String Terminator */

#define SIXEL 0
#define GIF   1

#define MAX_BYTES 10240

typedef unsigned char byte;

static Display *dpy;
static Pixmap pixmap;
static int screen;
static int Width = 500, Height = 500;
static int format = SIXEL;
static int fd;

static
Display *InitializeDisplay(int argc, char **argv)
{
    Display *dpy;
    int i;

    for (i = 1; i < argc; i++)
    {
	if (strncmp(argv[i], "-width", strlen(argv[i])) == 0)
	    Width = atoi(argv[++i]);
	else if (strncmp(argv[i], "-height", strlen(argv[i])) == 0)
	    Height = atoi(argv[++i]);
	else if (strncmp(argv[i], "-sixel", strlen(argv[i])) == 0)
	    format = SIXEL;
	else if (strncmp(argv[i], "-gif", strlen(argv[i])) == 0)
	    format = GIF;
	else
	{
	    fprintf(stderr, usage, argv[0]);
	    exit(-1);
	}
    }
    dpy = XOpenDisplay(NULL);
    if (dpy == NULL) {
	fprintf(stderr, "%s: can't open display\n", argv[0]);
	exit(-1);
    }
    return dpy;
}

static
void TextOut(DPSContext ctx, char *buffer, unsigned count)
{
    fwrite(buffer, 1, count, stdout);
    fflush(stdout);
}

static
void HandleStatus(DPSContext ctx, int status)
{
    char *status_event;

    switch (status)
    {
	case PSRUNNING:		status_event = "running"; break;
	case PSNEEDSINPUT:	status_event = "needs input"; break;
	case PSZOMBIE:		status_event = "zombie"; break;
	case PSFROZEN:		status_event = "frozen"; break;
	default:		status_event = "unknown status"; break;
    }
    fprintf(stderr, "[Status event - %s]\n", status_event);
}

static
DPSContext InitializePostScript(Display *dpy, Pixmap pixmap)
{
    DPSContext ctx;

    ctx = XDPSCreateSimpleContext(dpy, pixmap, DefaultGC(dpy, screen),
	0, Height, (DPSTextProc)TextOut, DPSDefaultErrorProc, NULL);
    if (ctx == NULL) {
        printf("GKS: X server does not have PostScript extension");
        exit(-1);
    }
    XDPSRegisterStatusProc(ctx, HandleStatus);
    DPSPrintf(ctx, "resyncstart\n");

    return ctx;
}

static
void WriteGIFWord(int fd, int w)
{
    byte c;

    c = (w & 0xff);
    write(fd, &c, 1);
    c = ((w >> 8) & 0xff);
    write(fd, &c, 1);
}

static
void PixmapToGIF(int page)
{
    char *env, path[256], *cp;
    int ncolors, size, besize;
    byte c, r, g, b, *pix, *ppix, *beimage;
    register int i, j, n;
    XImage *image;
    XColor ctab[256];
    int BitsPerPixel, ColorMapSize, InitCodeSize;
    unsigned long pixel;

    if (page > 0)
    {
	if ((env = (char *) getenv("GLI_GIF")) == NULL)
	    sprintf(path, "gli%2d.gif", page);
	else
	    sprintf(path, env, page);

	for (cp = path; *cp; cp++)
	    if (*cp == ' ')
		*cp = '0';

	fd = open(path, O_CREAT | O_TRUNC | O_WRONLY, 0644);
	if (fd < 0)
	{
	    fprintf(stderr, "GKS: can't open GIF file\n");
	    exit(-1);
	}
    }
    else
	fd = 1;

    image = XGetImage(dpy, pixmap, 0, 0, Width, Height, AllPlanes, ZPixmap);

    ncolors = 0;
    for (j = 0; j < Height; j++) {
	for (i = 0; i < Width; i++) {
	    pixel = XGetPixel(image, i, j);
	    for (n = 0; n < ncolors; n++) {
		if (ctab[n].pixel == pixel)
		    break;
	    }
	    if (n == ncolors)
		ctab[ncolors++].pixel = pixel;
	}
    }
    XQueryColors(dpy, DefaultColormap(dpy, screen), ctab, ncolors);

    for (BitsPerPixel = 1; BitsPerPixel < 8; BitsPerPixel++)
        if ((1 << BitsPerPixel) > ncolors) break;

    /* write the GIF header */

    write(fd, "GIF87a", 6);

    WriteGIFWord(fd, Width);	/* screen descriptor */
    WriteGIFWord(fd, Height);

    c = 0x80;                   /* yes, there is a color map */
    c |= (8 - 1) << 4;          /* OR in the color resolution (hardwired 8) */
    c |= (BitsPerPixel - 1);    /* OR in the # of bits per pixel */
    write(fd, &c, 1);

    c = 0x0;
    write(fd, &c, 1);		/* background color */
    write(fd, &c, 1);		/* future expansion byte */

    /* write colormap */

    ColorMapSize = 1 << BitsPerPixel;

    for (i = 0; i < ncolors; i++) {
        r = (byte) (ctab[i].red   >> 8);
        g = (byte) (ctab[i].green >> 8);
        b = (byte) (ctab[i].blue  >> 8);
        write(fd, &r, 1);
        write(fd, &g, 1);
        write(fd, &b, 1);
    }
    for ( ; i < ColorMapSize; i++) {
        r = b = g = 0x0;
        write(fd, &r, 1);
        write(fd, &g, 1);
        write(fd, &b, 1);
    }
    
    write(fd, ",", 1);		/* image separator */

    WriteGIFWord(fd, 0);	/* image header */
    WriteGIFWord(fd, 0);
    WriteGIFWord(fd, Width);
    WriteGIFWord(fd, Height);

    write(fd, &c, 1);

    size = Width * Height;
    pix = ppix = (byte *) malloc(sizeof(byte) * size);
    beimage = (byte *) malloc(sizeof(byte) * size * 3 / 2);	/* worst case */

    if (pix != NULL && beimage != NULL)
    {
        for (j = 0; j < Height; j++) {
            for (i = 0; i < Width; i++) {
                pixel = XGetPixel(image, i, j);
		for (n = 0; n < ncolors; n++) {
		    if (ctab[n].pixel == pixel)
			break;
		}
                *ppix++ = n;
	    }
	}

        InitCodeSize = max(BitsPerPixel, 2);
        gkscompress(InitCodeSize + 1, pix, size, beimage, &besize);

        c = InitCodeSize;
        write(fd, &c, 1);
        if (write(fd, beimage, besize) != besize) {
            fprintf(stderr, "GKS: can't write GIF file\n");
            perror("write");
	}

        free(beimage);
        free(pix);
    }
    else
        fprintf(stderr, "GKS: can't allocate temporary storage\n");

    write(fd, "\0", 1);		/* write out a zero-length packet (EOF) */
    write(fd, ";", 1);		/* terminator */

    XDestroyImage(image);

    close(fd);
}

static
void WriteSixelData(int fd, byte *pix)
{
    register int i, j;
    register int b, repeat, thiscolor, nextcolor;
    char s[30];
    char *sixel = "@ACGO_";

    for (j = 0; j < Height; j++)
    {
	b = j % 6;
	repeat = 1;

        for (i = 0; i < Width; i++)
	{
            thiscolor = *pix++;
	    if (i == Width - 1)		/* last pixel in row */
	    {
		if (repeat == 1)
		    sprintf(s, "#%d%c", thiscolor, sixel[b]);
		else
		    sprintf(s, "#%d!%d%c", thiscolor, repeat, sixel[b]);
		write(fd, s, strlen(s));
	    }
	    else			/* not last pixel in row */
	    {
		nextcolor =  *(pix + 1);
		if (thiscolor == nextcolor)
		    ++repeat;
		else {
		    if (repeat == 1)
			sprintf(s, "#%d%c", thiscolor, sixel[b]);
		    else {
			sprintf(s, "#%d!%d%c", thiscolor, repeat, sixel[b]);
			repeat = 1;
		    }
		    write(fd, s, strlen(s));
		}
	    }
	}

	write(fd, "$\n", 2);		/* Carriage Return */
	if (b == 5)
	    write(fd, "-\n", 2);	/* Line Feed (one sixel height) */
    }
}

static
void PixmapToSixel(int page)
{
    char path[12], *cp, s[30];
    int ncolors, size;
    byte r, g, b, *pix, *ppix;
    register int i, j, n;
    XImage *image;
    XColor ctab[256];
    unsigned long pixel;

    if (page > 0)
    {
	sprintf(path, "gli%2d.six", page);
	for (cp = path; *cp; cp++)
	    if (*cp == ' ')
		*cp = '0';

	fd = open(path, O_CREAT | O_TRUNC | O_WRONLY, 0644);
	if (fd < 0)
	{
	    fprintf(stderr, "GKS: can't open Sixel file\n");
	    exit(-1);
	}
    }
    else
	fd = 1;

    image = XGetImage(dpy, pixmap, 0, 0, Width, Height, AllPlanes, ZPixmap);

    ncolors = 0;
    for (j = 0; j < Height; j++) {
	for (i = 0; i < Width; i++) {
	    pixel = XGetPixel(image, i, j);
	    for (n = 0; n < ncolors; n++) {
		if (ctab[n].pixel == pixel)
		    break;
	    }
	    if (n == ncolors)
		ctab[ncolors++].pixel = pixel;
	}
    }
    XQueryColors(dpy, DefaultColormap(dpy, screen), ctab, ncolors);

    write(fd, DCS, 1);		/* start with Device Control String */
    write(fd, "0;0;1q", 6);	/* horizontal grid size */
    write(fd, "\"1;1\n", 7);	/* set aspect ratio 1:1 */

    /* write colormap */

    for (i = 0; i < ncolors; i++) {
        r = (byte) ((float) ctab[i].red   / 65535 * 100);
        g = (byte) ((float) ctab[i].green / 65535 * 100);
        b = (byte) ((float) ctab[i].blue  / 65535 * 100);

	sprintf(s, "#%d;2;%d;%d;%d", i, (int) r, (int) g, (int) b);
	write(fd, s, strlen(s));
    }
    
    size = Width * Height;
    pix = ppix = (byte *) malloc(sizeof(byte) * size);

    if (pix != NULL)
    {
        for (j = 0; j < Height; j++) {
            for (i = 0; i < Width; i++) {
                pixel = XGetPixel(image, i, j);
		for (n = 0; n < ncolors; n++) {
		    if (ctab[n].pixel == pixel)
			break;
		}
                *ppix++ = n;
	    }
	}
        WriteSixelData(fd, pix);

        free(pix);
    }
    else
        fprintf(stderr, "GKS: can't allocate temporary storage\n");

    write(fd, ST, 1);

    XDestroyImage(image);

    close(fd);
}

void main(int argc, char **argv)
{
    char *env;
    DPSContext ctx;    
    GC gc;
    XGCValues xgcv;
    char buf[MAX_BYTES];
    int page = 1;

    if (!isatty(1))
	page = 0;
    else if ((env = (char *) getenv("GLI_GKS_PAGE")) != NULL)
	page = atol(env);

    dpy = InitializeDisplay(argc, argv);
    screen = DefaultScreen(dpy);

    pixmap = XCreatePixmap(dpy, DefaultRootWindow(dpy), Width, Height,
	DefaultDepth(dpy, screen));

    xgcv.foreground = WhitePixel(dpy, screen);
    xgcv.background = BlackPixel(dpy, screen);
    gc = XCreateGC(dpy, pixmap, GCForeground | GCBackground, &xgcv);
    XFillRectangle(dpy, pixmap, gc, 0, 0, Width, Height);
    XFreeGC(dpy, gc);

    ctx = InitializePostScript(dpy, pixmap);

    if (!isatty(0))
    {
	while (fgets(buf, MAX_BYTES, stdin))
	{
	    DPSWritePostScript(ctx, buf, strlen(buf));
	    if (!strncmp(buf, "%%EndPage:", 10))
	    {
		DPSFlushContext(ctx);
		if (format == SIXEL)
		    PixmapToSixel(page++);
		else
		    PixmapToGIF(page++);
	    }
	}
	if (page == 0)
	{
	    DPSFlushContext(ctx);
	    if (format == SIXEL)
		PixmapToSixel(0);
	    else
		PixmapToGIF(0);
	}
    }

    DPSDestroySpace(DPSSpaceFromContext(ctx));

    XFreePixmap(dpy, pixmap);
    XCloseDisplay(dpy);
}
