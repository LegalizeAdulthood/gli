/*
 * Copyright @ 1995 - 1999   Josef Heinen
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
 *	GLI GKS V4.5
 *
 * ABSTRACT:
 *
 *	This module contains an interpreter for GKSM metafiles.
 *
 * AUTHOR:
 *
 *	Josef Heinen
 *
 * VERSION:
 *
 *	V1.0
 *
 */


#if defined(RPC) || (defined(_WIN32) && !defined(__GNUC__))
#define HAVE_SOCKETS
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#if !defined(VMS) && !defined(MSDOS) && !defined(_WIN32)
#include <unistd.h>
#endif

#if defined (cray) || defined (__SVR4) || defined (MSDOS) || defined(_WIN32)
#include <fcntl.h>
#else
#include <sys/types.h>
#include <sys/file.h>
#endif

#ifdef HAVE_SOCKETS
#ifndef _WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#else
#include <windows.h>
#endif
#endif

#ifndef MSDOS
#include <sys/stat.h>
#endif

#ifdef VMS
#include <descrip.h>
#endif

#ifdef cray
#include <fortran.h>
#endif


#include "terminal.h"
#include "gksdefs.h"


#define RESOLVE(arg, type, nbytes) arg = (type *)(s + sp); sp += nbytes


static
char *gksname[] = {
    NULL,     NULL,     NULL,     NULL,     NULL,
    NULL,     NULL,     NULL,     NULL,     NULL,
    NULL,     NULL,     "GPL",    "GPM",    "GTXS",
    "GFA",    "GCA",    NULL,     NULL,     "GSLN",
    "GSLWSC", "GSPLCI", NULL,     "GSMK",   "GSMKSC",
    "GSPMCI", NULL,     "GSTXFP", "GSCHXP", "GSCHSP",
    "GSTXCI", "GSCHH",  "GSCHUP", "GSTXP",  "GSTXAL",
    NULL,     "GSFAIS", "GSFASI", "GSFACI", NULL,
    NULL,     NULL,     NULL,     NULL,     NULL,
    NULL,     NULL,     NULL,     "GSCR",   "GSWN",
    "GSVP",   NULL,     "GSELNT", "GSCLIP"
 };

static char *path = NULL;
static int wkid = 1, conid = 0, wstype = 0;
static int run_as_daemon = 0;
static int orientation = 0;
static float factor = 1.0;
static int verbose = 0;
static int new_frame = 0;


#ifdef HAVE_SOCKETS

static
int open_socket(char *name, char flag)
{
    static int s = -1;
    struct hostent *hp;
    struct sockaddr_in sin;
    char *hostname, *port, tmp[BUFSIZ];
    int sd;

#if defined(_WIN32) && !defined(__GNUC__)
    WORD wVersionRequested = MAKEWORD(2, 0);
    WSADATA wsaData;

    if (WSAStartup(wVersionRequested, &wsaData) != 0)
    {
        tt_fprintf(stderr, "Can't find a usable WinSock DLL\n");
        exit(1);
    }
    else
	s = -1;
#endif
    if (s == -1 || flag == 'w')
    {
	strcpy(tmp, name);
	hostname = strtok(tmp, ":");
	port = strtok(NULL, ":");
	if (port == NULL) {
	    port = hostname;
	    hostname = "localhost";
	}

	if (flag == 'w') {
	    if ((hp = gethostbyname(hostname)) == 0) {
		perror("gethostbyname");
		exit(1);
	    }
	}

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = flag == 'w' ?
	    ((struct in_addr *)(hp->h_addr))->s_addr : INADDR_ANY;
	sin.sin_port = htons((unsigned short)atoi(port));
    }

    if (s == -1)
    {
	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s == -1) {
	    perror("socket");
	    exit(1);
	}

	if (flag == 'r') {
	    if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) == -1) {
		perror("bind");
		exit(1);
	    }
	    if (listen(s, 1) == -1) {
		perror("listen");
		exit(1);
	    }
	}
    }

    if (flag == 'r') {
	sd = accept(s, NULL, NULL);
	if (sd == -1) {
	    perror("accept");
	    exit(1);
	}
    }
    else {
	if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) == -1) {
	    perror("connect");
	    exit(1);
	}
	sd = s;
    }

    return sd;
}

#endif

static
char *readfile(char *path)
{
    int fd, cc;
#ifdef HAVE_SOCKETS
    int is_socket = 0;
#endif
#ifndef MSDOS
    struct stat buf;
#endif
    char *s = NULL;

#ifdef HAVE_SOCKETS
    if (strchr(path, ':') == NULL)
#endif
	fd = open(path,
#ifdef _WIN32
	    O_RDONLY | O_BINARY, 0);
#else
	    O_RDONLY, 0);
#endif
#ifdef HAVE_SOCKETS
    else if ((fd = open_socket(path, 'r')) != -1)
	is_socket = 1;
#endif
    if (fd != -1)
    {
        int size, offset = 0;

#ifndef MSDOS
	fstat(fd, &buf);
        size = (buf.st_size > 0) ? buf.st_size : 1000000;
#else
	size = 32767;
#endif
        s = (char *) malloc(size + 1);
	while ((cc =
#ifdef HAVE_SOCKETS
	    is_socket ? recv(fd, s + offset, BUFSIZ, 0) :
#endif
	    read(fd, s + offset, BUFSIZ)) > 0)
	{
	    offset += cc;
	    if (offset > size)
	    {
		tt_fprintf(stderr, "Buffer overflow\n");
		break;
	    }
	}
	s[offset] = '\0';

#if defined (_WIN32) && !defined (__GNUC__)
        if (is_socket)
	    closesocket(fd);
	else
	    close(fd);

	WSACleanup();
#else
	close(fd);
#endif
    }
    else
        tt_fprintf(stderr, "Can't open: %s (No such file or directory)\n",
	    path);

    return s;
}

static
void awaitkey(void)
{
    if (run_as_daemon)
    {
	int lc_dev_nr = 1, inp_dev_stat, inp_tnr;
	float px, py;

	GRQLC (&wkid, &lc_dev_nr, &inp_dev_stat, &inp_tnr, &px, &py);
	if (inp_dev_stat == 0)
	    exit(0);
    }
    else if (isatty(0) && isatty(1))
    {
	GUWK(&wkid, &GPERFO);

	tt_printf("Press RETURN to continue..."); tt_fflush(stdout);
	getchar();
    }
}

static
void gksinit(gks_state_list *gksl)
{
    int i, tnr, state;
    float *wn, *vp;
    int segm = 1;
    float fixed_point_x = 0.5, fixed_point_y = 0.5, translation_x = 0,
	translation_y = 0, rotation = 0, mat[3][2];

    if (new_frame)
    {
	awaitkey();
	GQOPS(&state);
	if (state == GSGOP)
	    GCLSG();
	GCLRWK(&wkid, &GALWAY);
    }
    else if (!run_as_daemon)
	new_frame = 1;

    GSPLI(&gksl->lindex);
    GSLN(&gksl->ltype);
    GSLWSC(&gksl->lwidth);
    GSPLCI(&gksl->plcoli);

    GSPMI(&gksl->mindex);
    GSMK(&gksl->mtype);
    GSMKSC(&gksl->mszsc);
    GSPMCI(&gksl->pmcoli);

    GSTXI(&gksl->tindex);
    GSTXFP(&gksl->txfont, &gksl->txprec);
    GSCHXP(&gksl->chxp);
    GSCHSP(&gksl->chsp);
    GSTXCI(&gksl->txcoli);
    GSCHH(&gksl->chh);
    GSCHUP(&gksl->chup[0], &gksl->chup[1]);
    GSTXP(&gksl->txp);
    GSTXAL(&gksl->txal[0], &gksl->txal[1]);

    GSFAI(&gksl->findex);
    GSFAIS(&gksl->ints);
    GSFASI(&gksl->styli);
    GSFACI(&gksl->facoli);

    for (i = 1; i <= 8; i++)
    {
	tnr = i;
	wn = gksl->window[tnr];
	vp = gksl->viewport[tnr];
	GSWN(&tnr, &wn[0], &wn[1], &wn[2], &wn[3]);
	GSVP(&tnr, &vp[0], &vp[1], &vp[2], &vp[3]);
    }

    GSELNT(&gksl->cntnr);
    GSCLIP(&gksl->clip);

    if (factor != 1.0)
    {
	if (factor < 1)
	{
	    translation_x = -0.5 * (factor - 1);
	    translation_y = -translation_x;
	}
	GCRSG(&segm);
        GEVTM (&fixed_point_x, &fixed_point_y, &translation_x,
            &translation_y, &rotation, &factor, &factor, &GNDC, mat);
	GSSGT(&segm, mat);
    }
    else
	GSSGT(&gksl->opsg, gksl->mat);

    GSASF(gksl->asf);
}

static
void interp(char *s)
{    
    gks_state_list *gksl;
    int sp = 0, *len, *f, *ia, *dx, *dy, *dimx, sx = 1, sy = 1, *lc;
    float *r1, *r2;
    char *c;
    register int i;
#ifdef VMS
    struct dsc$descriptor_s text;
#endif
#ifdef cray
    _fcd text;
#endif

    while (s[sp])
    {
	RESOLVE(len, int, sizeof(int));
	if (*len < 0)
	{
	    tt_fprintf(stderr, "Metafile is corrupted\n");
	    exit(1);
	}
	RESOLVE(f, int, sizeof(int));

	switch (*f)
	{
	    case  2 :

		RESOLVE(gksl, gks_state_list, sizeof(gks_state_list));
		break;

	    case 12 :		/* polyline */
	    case 13 :		/* polymarker */
	    case 15 :		/* fill area */

		RESOLVE(ia, int, sizeof(int));
		RESOLVE(r1, float, *ia * sizeof(float));
		RESOLVE(r2, float, *ia * sizeof(float));

		if (verbose)
		{
		    tt_printf("%6s %d", gksname[*f], *ia); 
		    for (i = 0; i < *ia; i++)
			tt_printf(" %g %g", r1[i], r2[i]); 
		    tt_printf("\n");
		}
		break;

	    case 14 :                       /* text */

	        RESOLVE(r1, float, sizeof(float));
	        RESOLVE(r2, float, sizeof(float));
	        RESOLVE(lc, int, sizeof(int));
	        RESOLVE(c, char, 132);

		if (verbose)
		{
		    tt_printf("%6s %g %g %d %s\n", gksname[*f], *r1, *r2,
			*lc, c); 
		}
	        break;

	    case 16 :               /* cell array */

	        RESOLVE(r1, float, 2 * sizeof(float));
	        RESOLVE(r2, float, 2 * sizeof(float));
	        RESOLVE(dx, int, sizeof(int));
	        RESOLVE(dy, int, sizeof(int));
	        RESOLVE(dimx, int, sizeof(int));
	        RESOLVE(ia, int, *dimx * *dy * sizeof(int));

		if (verbose)
		{
		    tt_printf("%6s %g %g %g %g %d %d %d %d %d %d",
			gksname[*f], r1[0], r2[0], r1[1], r2[1],
			*dx, *dy, sx, sy, *dimx, *dy);
		    for (i = 0; i < *dimx * *dy; i++)
			tt_printf(" %d", ia[i]); 
		    tt_printf("\n"); 
		}
	        break;

	    case 19 :		/* set linetype */
	    case 21 :		/* set polyline color index */
	    case 23 :		/* set markertype */
	    case 25 :           /* set polymarker color index */
	    case 30 :           /* set text color index */
	    case 33 :           /* set text path */
	    case 36 :           /* set fillarea interior style */
	    case 37 :           /* set fillarea style index */
	    case 38 :           /* set fillarea color index */
	    case 52 :           /* select normalization transformation */
	    case 53 :           /* set clipping indicator */

		RESOLVE(ia, int, sizeof(int));

		if (verbose)
		{
		    tt_printf("%6s %d\n", gksname[*f], *ia);
		}
	        break;

	    case 27 :		/* set text font and precision */
	    case 34 :		/* set text alignment */

		RESOLVE(ia, int, 2 * sizeof(int));

		if (verbose)
		{
		    tt_printf("%6s %d %d\n", gksname[*f], ia[0], ia[1]);
		}
	        break;

	    case 20 :           /* set linewidth scale factor */
	    case 24 :           /* set marker size scale factor */
	    case 28 :           /* set character expansion factor */
	    case 29 :           /* set character spacing */
	    case 31 :           /* set character height */

		RESOLVE(r1, float, sizeof(float));

		if (verbose)
		{
		    tt_printf("%6s %g\n", gksname[*f], *r1);
		}
	        break;

	    case 32 :           /* set character up vector */

		RESOLVE(r1, float, sizeof(float));
		RESOLVE(r2, float, sizeof(float));

		if (verbose)
		{
		    tt_printf("%6s %g %g\n", gksname[*f], *r1, *r2);
		}
	        break;

	    case 48 :           /* set color representation */

	        RESOLVE(ia, int, sizeof(int));
	        RESOLVE(r1, float, 3 * sizeof(float));

		if (verbose)
		{
		    tt_printf("%6s %d %g %g %g\n",
			gksname[*f], *ia, r1[0], r1[1], r1[2]);
		}
		break;

	    case 49 :           /* set window */
	    case 50 :           /* set viewport */

	        RESOLVE(ia, int, sizeof(int));
	        RESOLVE(r1, float, 2 * sizeof(float));
	        RESOLVE(r2, float, 2 * sizeof(float));

		if (verbose)
		{
		    tt_printf("%6s %d %g %g %g %g\n",
			gksname[*f], *ia, r1[0], r1[1], r2[0], r2[1]);
		}
	        break;

	    default:
		tt_fprintf(stderr, "Metafile is corrupted\n");
		exit(1);
	}

        switch (*f)
	{
	    case  2 : gksinit(gksl); break;

	    case 12 : GPL(ia, r1, r2); break;
	    case 13 : GPM(ia, r1, r2); break;

	    case 14 :
#ifdef VMS
		text.dsc$b_dtype = DSC$K_DTYPE_T;
		text.dsc$b_class = DSC$K_CLASS_S;
		text.dsc$w_length = *lc;
		text.dsc$a_pointer = c;

		GTXS(r1, r2, lc, &text);
#else
#ifdef cray
		text = _cptofcd(c, *lc);

		GTXS(r1, r2, lc, text);
#else
#if defined(_WIN32) && !defined(__GNUC__)
		{
		    unsigned short chars_len = *lc;

		    GTXS(r1, r2, lc, c, chars_len);
		}
#else
		GTXS(r1, r2, lc, c, *lc);
#endif /* _WIN32 */
#endif /* cray */
#endif /* VMS */
	        break;

	    case 15 : GFA(ia, r1, r2); break;
	    case 16 : GCA(&r1[0], &r2[0], &r1[1], &r2[1], dx, dy, &sx, &sy,
			  dimx, dy, ia); break;

	    case 19 : GSLN(ia); break;
	    case 20 : GSLWSC(r1); break;
	    case 21 : GSPLCI(ia); break;
	    case 23 : GSMK(ia); break;
	    case 24 : GSMKSC(r1); break;
	    case 25 : GSPMCI(ia); break;
	    case 27 : GSTXFP(&ia[0], &ia[1]); break;
	    case 28 : GSCHXP(r1); break;
	    case 29 : GSCHSP(r1); break;
	    case 30 : GSTXCI(ia); break;
	    case 31 : GSCHH(r1); break;
	    case 32 : GSCHUP(r1, r2); break;
	    case 33 : GSTXP(ia); break;
	    case 34 : GSTXAL(&ia[0], &ia[1]); break;
	    case 36 : GSFAIS(ia); break;
	    case 37 : GSFASI(ia); break;
	    case 38 : GSFACI(ia); break;

	    case 48 : GSCR(&wkid, ia, &r1[0], &r1[1], &r1[2]); break;

	    case 49 : GSWN(ia, &r1[0], &r1[1], &r2[0], &r2[1]); break;
	    case 50 : GSVP(ia, &r1[0], &r1[1], &r2[0], &r2[1]); break;
	    case 52 : GSELNT(ia); break;
	    case 53 : GSCLIP(ia); break;
	}
    }
}

static
void opengks(void)
{
    int errfil = 0, bufsiz = -1;
    static int asf[] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
    float xmin = 0.0, xmax, ymin = 0.0, ymax;

    GOPKS(&errfil, &bufsiz);
    GSASF(asf);
    GOPWK(&wkid, &conid, &wstype);
    GACWK(&wkid);

    switch (orientation)
    {
      case 0 :
	break;

      case 1 :
	xmax = 0.297; ymax = 0.21;
	GSWKVP(&wkid, &xmin, &xmax, &ymin, &ymax);

	xmax = 1.0; ymax = 0.21/0.297;
	GSWKWN(&wkid, &xmin, &xmax, &ymin, &ymax);
	break;

      case 2 :
	xmax = 0.21; ymax = 0.297;
	GSWKVP(&wkid, &xmin, &xmax, &ymin, &ymax);

	xmax = 0.21/0.297; ymax = 1.0;
	GSWKWN(&wkid, &xmin, &xmax, &ymin, &ymax);
	break;
    }
    new_frame = 0;
}

static
void closegks(void)
{
    awaitkey();
    GECLKS();
}

static
void usage(void)
{
    tt_fprintf(stderr, "Usage:\n\
gligksm [-Oorientation] [-d] [-factor f] [-h] [-t wstype] [-v] file\n\
         -Oorientation   Select the orientation (landscape, portrait).\n\
                    -d   Run as daemon.\n\
             -factor f   Specifies the magnification factor.\n\
                    -h   Prints this information.\n\
             -t wstype   Use GKS workstation type wstype.\n\
\n\
The present workstation types recognized by gligksm are:\n\
     16:       VT330               7:    CGM Binary\n\
     17:       VT340               8:    CGM Clear Text\n\
     72:       TEK 401x           38:    DEC LN03+\n\
     82:       TEK 42xx       51, 53:    HP-GL Graphics Plotter\n\
    201:       TAB 132/15-G   61, 62:    PostScript (b/w, color)\n\
    204:       Monterey       63, 64:    CompuServe GIF dump (b/w, color)\n\
    207:       IBM/PC             92:    DEC LJ250 Companion Color Printer\n\
    210, 211:  X display    103, 104:    Portable BitMap (72, 75 dpi)\n\
    214:       X display w\\ Sun rasterfile dump\n\
    215, 218:  X display w\\ CompuServe GIF dump (87a, 89a)\n\
    217:       X display w\\ frame grabber\n\n");

    exit(1);
}

static
void parse(int argc, char **argv)
{
    char *option;

    argv++;
    while ((option = *argv++) != NULL)
    {
        if (!strcmp(option, "-Olandscape"))
	{
	    orientation = 1;
	}
	else if (!strcmp(option, "-Oportrait"))
	{
	    orientation = 2;
	}
	else if (!strcmp(option, "-d"))
	{
	    run_as_daemon = 1;
	}
	else if (!strcmp(option, "-factor"))
	{
	    if (*argv)
		factor = atof(*argv++);
	    else
		usage();
	}
	else if (!strcmp(option, "-h"))
	{
	    usage();
	}
	else if (!strcmp(option, "-t"))
	{
	    if (*argv)
		wstype = atoi(*argv++);
	    else
		usage();
	}
	else if (!strcmp(option, "-v"))
	{
	    verbose = 1;
	}
        else if (*option == '-')
	{
	    tt_fprintf(stderr, "Invalid option: '%s'\n", option);
	    usage();
	}
	else
	    path = option;
    }
}

#if !defined(VMS) && !defined(MSDOS) && !defined(_WIN32)

static
void forkdaemon(void)
{
    int cpid, devnull;

    if ((cpid = fork()) == -1)
    {
	tt_fprintf(stderr, "Can't fork daemon");
	exit(-1);
    }
    if (cpid)
	exit(0);

    setsid();
    chdir("/");

    if ((devnull = open("/dev/null", O_RDWR, 0)) != -1)
    {
	dup2(devnull, 0);
	dup2(devnull, 1);
	dup2(devnull, 2);
	if (devnull > 2)
	    close(devnull);
    }
}

#endif

int main(int argc, char **argv)
{
    char *s;

    parse(argc, argv);

    if (path != NULL)
    {
#if !defined(VMS) && !defined(MSDOS) && !defined(_WIN32)
	if (run_as_daemon)
	    forkdaemon();
#endif
	do
	{
	    if ((s = readfile(path)) != NULL)
	    {
		opengks();
		interp(s);
		closegks();
		free(s);
	    }
	}
	while (run_as_daemon);
    }
    else
	usage();

    return 0;
}
