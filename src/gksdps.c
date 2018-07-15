/*
 * Copyright @ 1997 - 2000  Josef Heinen
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
 *      This module contains a logical device driver for Adobe's
 *      PostScript format
 *
 * AUTHOR(S):
 *
 *      Josef Heinen
 *      Elmar Westphal
 *
 * VERSION:
 *
 *      V2.0
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#ifdef VMS
#include <descrip.h>
#endif
 
#ifdef cray
#include <fortran.h>
#endif

#if !defined(VMS) && !defined(MSDOS) && !defined(_WIN32)
#include <unistd.h>
#endif

#ifdef hpux
#include <sys/utsname.h>
#endif

#if defined (cray) || defined (__SVR4) || defined(MSDOS) || defined(_WIN32)
#include <fcntl.h>
#else
#include <sys/file.h>
#endif

#if defined(_WIN32) && !defined(__GNUC__)
#include <windows.h>
#endif

#ifdef DPS
#include <X11/Xlib.h>
#endif /* DPS */

#include "gksdefs.h"

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif
#define NINT(a) (int)((a) + 0.5)

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define FEPS 1.0E-06

#if defined(_WIN32) && !defined(__GNUC__)
#define STDCALL __stdcall
#else
#define STDCALL
#endif

#ifdef VMS
#define CHARARG(a) struct dsc$descriptor *a
#else
#ifdef cray
#define CHARARG(a) _fcd a
#else
#if defined(_WIN32) && !defined(__GNUC__)
#define CHARARG(a) char *(a), unsigned short a##_len
#else
#define CHARARG(a) char *a
#endif /* _WIN32 */
#endif /* cray */
#endif /* VMS */

#define MAX_TNR 9

#define WC_to_NDC(xw, yw, tnr, xn, yn) \
  xn = a[tnr] * (xw) + b[tnr]; \
  yn = c[tnr] * (yw) + d[tnr]

#define WC_to_NDC_rel(xw, yw, tnr, xn, yn) \
  xn = a[tnr] * (xw); \
  yn = c[tnr] * (yw)

#define NDC_to_DC(xn, yn, xd, yd) \
  xd = p->a * (xn) + p->b; \
  yd = p->c * (yn) + p->d

#if defined(hpux) && !defined(NAGware)
static int (*line_routine_a)();
static int (*fill_routine_a)();
#endif

#ifdef DPS
static char *options[] = { "-sixel", "-gif" };
#endif

static
gks_state_list *gksl;

static
float a[MAX_TNR], b[MAX_TNR], c[MAX_TNR], d[MAX_TNR];

static int X11 = 0;
static char *show[] = {"lj", "lj", "ct", "rj"};
static float yfac[] = {0., -1.2, -1.0, -0.5, 0., 0.2};

static int predef_font[] = {1, 1, 1, -2, -3, -4};
static int predef_prec[] = {0, 1, 2, 2, 2, 2};
static int predef_ints[] = {0, 1, 3, 3, 3};
static int predef_styli[] = {1, 1, 1, 2, 3};

static int map[] =
{
  22, 9, 5, 14, 18, 26, 13, 1,
  24, 11, 7, 16, 20, 28, 13, 3,
  23, 10, 6, 15, 19, 27, 13, 2,
  25, 12, 8, 17, 21, 29, 13, 4
};

static float caps[] =
{
  0.662, 0.653, 0.676, 0.669,
  0.718, 0.718, 0.718, 0.718,
  0.562, 0.562, 0.562, 0.562,
  0.667,
  0.714, 0.714, 0.714, 0.714,
  0.722, 0.722, 0.722, 0.722,
  0.740, 0.740, 0.740, 0.740,
  0.732, 0.733, 0.732, 0.732,
  0.718, 0.718, 0.718, 0.718,
  0.681, 0.681, 0.681, 0.681,
  0.692, 0.692, 0.681, 0.681,
  0.587, 0.587
};

static char *fonts[] =
{
  "Times-Roman", "Times-Italic", "Times-Bold", "Times-BoldItalic",
  "Helvetica", "Helvetica-Oblique", "Helvetica-Bold",
  "Helvetica-BoldOblique", "Courier", "Courier-Oblique",
  "Courier-Bold", "Courier-BoldOblique", "Symbol",
  "LubalinGraph-Book", "LubalinGraph-BookOblique",
  "LubalinGraph-Demi", "LubalinGraph-DemiOblique",
  "NewCenturySchlbk-Roman", "NewCenturySchlbk-Italic",
  "NewCenturySchlbk-Bold", "NewCenturySchlbk-BoldItalic",
  "AvantGarde-Book", "AvantGarde-BookOblique", "AvantGarde-Demi",
  "AvantGarde-DemiOblique", "Souvenir-Light",
  "Souvenir-LightItalic", "Souvenir-Demi", "Souvenir-DemiItalic",
  "Helvetica-Narrow", "Helvetica-Narrow-Oblique",
  "Helvetica-Narrow-Bold", "Helvetica-Narrow-BoldOblique",
  "Bookman-Light", "Bookman-LightItalic", "Bookman-Demi",
  "Bookman-DemiItalic", "Palatino-Roman", "Palatino-Italic",
  "Palatino-Bold", "Palatino-BoldItalic",
  "ZapfChancery-MediumItalic", "ZapfDingbats"
};

static char *afonts[] =
{
  "AGOldFace-BoldOutline", "AGOldFace-Outline",
  "AmericanTypewriter-Bold", "AmericanTypewriter-BoldA",
  "AmericanTypewriter-BoldCond", "AmericanTypewriter-BoldCondA",
  "AmericanTypewriter-Cond", "AmericanTypewriter-CondA",
  "AmericanTypewriter-Light", "AmericanTypewriter-LightA",
  "AmericanTypewriter-LightCond", "AmericanTypewriter-LightCondA",
  "AmericanTypewriter-Medium", "AmericanTypewriter-MediumA",
  "AvantGarde-Book", "AvantGarde-BookOblique", "AvantGarde-Demi",
  "AvantGarde-DemiOblique", "Bauhaus-Bold", "Bauhaus-Demi",
  "Bauhaus-Heavy", "Bauhaus-Light", "Bauhaus-Medium", "Bellevue",
  "Benguiat-Bold", "Benguiat-Book", "BenguiatGothic-Bold",
  "BenguiatGothic-BoldOblique", "BenguiatGothic-Book",
  "BenguiatGothic-BookOblique", "BenguiatGothic-Heavy",
  "BenguiatGothic-HeavyOblique", "BenguiatGothic-Medium",
  "BenguiatGothic-MediumOblique", "Bookman-Bold",
  "Bookman-BoldItalic", "Bookman-Demi", "Bookman-DemiItalic",
  "Bookman-Light", "Bookman-LightItalic", "Bookman-Medium",
  "Bookman-MediumItalic", "CaslonTwoTwentyFour-Black",
  "CaslonTwoTwentyFour-BlackIt", "CaslonTwoTwentyFour-Bold",
  "CaslonTwoTwentyFour-BoldIt", "CaslonTwoTwentyFour-Book",
  "CaslonTwoTwentyFour-BookIt", "CaslonTwoTwentyFour-Medium",
  "CaslonTwoTwentyFour-MediumIt", "CastellarMT", "Cheltenham-Bold",
  "Cheltenham-BoldItalic", "Cheltenham-Book",
  "Cheltenham-BookItalic", "Cheltenham-Light",
  "Cheltenham-LightItalic", "Cheltenham-Ultra",
  "Cheltenham-UltraItalic", "City-Bold", "City-BoldItalic",
  "City-Medium", "City-MediumItalic", "Courier", "Courier-Bold",
  "Courier-BoldOblique", "Courier-Oblique", "Cushing-Bold",
  "Cushing-BoldItalic", "Cushing-Book", "Cushing-BookItalic",
  "Cushing-Heavy", "Cushing-HeavyItalic", "Cushing-Medium",
  "Cushing-MediumItalic", "DorchesterScriptMT", "Esprit-Black",
  "Esprit-BlackItalic", "Esprit-Bold", "Esprit-BoldItalic",
  "Esprit-Book", "Esprit-BookItalic", "Esprit-Medium",
  "Esprit-MediumItalic", "Fenice-Bold", "Fenice-BoldOblique",
  "Fenice-Light", "Fenice-LightOblique", "Fenice-Regular",
  "Fenice-RegularOblique", "Fenice-Ultra", "Fenice-UltraOblique",
  "FrizQuadrata", "FrizQuadrata-Bold",
  "Galliard-Black", "Galliard-BlackItalic", "Galliard-Bold",
  "Galliard-BoldItalic", "Galliard-Italic", "Galliard-Roman",
  "Galliard-Ultra", "Galliard-UltraItalic", "Garamond-Bold",
  "Garamond-BoldCondensed", "Garamond-BoldCondensedItalic",
  "Garamond-BoldItalic", "Garamond-Book", "Garamond-BookCondensed",
  "Garamond-BookCondensedItalic", "Garamond-BookItalic",
  "Garamond-Light", "Garamond-LightCondensed",
  "Garamond-LightCondensedItalic", "Garamond-LightItalic",
  "Garamond-Ultra", "Garamond-UltraCondensed",
  "Garamond-UltraCondensedItalic", "Garamond-UltraItalic",
  "GillSans", "GillSans-Bold", "GillSans-BoldItalic",
  "GillSans-ExtraBold", "GillSans-Italic", "Giovanni-Black",
  "Giovanni-BlackItalic", "Giovanni-Bold", "Giovanni-BoldItalic",
  "Giovanni-Book", "Giovanni-BookItalic", "GoudyTextMT",
  "Helvetica", "Helvetica-Bold", "Helvetica-BoldOblique",
  "Helvetica-Oblique", "Isadora-Bold", "Isadora-Regular",
  "ItcEras-Bold", "ItcEras-Book", "ItcEras-Demi", "ItcEras-Light",
  "ItcEras-Medium", "ItcEras-Ultra", "ItcKabel-Bold",
  "ItcKabel-Book", "ItcKabel-Demi", "ItcKabel-Medium",
  "ItcKabel-Ultra", "Korinna-Bold",
  "Korinna-KursivBold", "Korinna-KursivRegular", "Korinna-Regular",
  "Leawood-Black", "Leawood-BlackItalic", "Leawood-Bold",
  "Leawood-BoldItalic", "Leawood-Book", "Leawood-BookItalic",
  "Leawood-Medium", "Leawood-MediumItalic", "Machine",
  "Machine-Bold", "Madrone", "NewBaskerville-Bold",
  "NewBaskerville-BoldItalic", "NewBaskerville-Italic",
  "NewBaskerville-Roman", "NuptialScript", "OfficinaSans-Bold",
  "OfficinaSans-BoldItalic", "OfficinaSans-Book",
  "OfficinaSans-BookItalic", "OfficinaSerif-Bold",
  "OfficinaSerif-BoldItalic", "OfficinaSerif-Book",
  "OfficinaSerif-BookItalic", "PepitaMT", "Ponderosa", "Poplar",
  "Rosewood-Fill", "Rosewood-Regular", "RussellSquare",
  "RussellSquare-Oblique",
  "Slimbach-Black", "Slimbach-BlackItalic", "Slimbach-Bold",
  "Slimbach-BoldItalic", "Slimbach-Book", "Slimbach-BookItalic",
  "Slimbach-Medium", "Slimbach-MediumItalic",
  "Souvenir-Demi", "Souvenir-DemiItalic", "Souvenir-Light",
  "Souvenir-LightItalic", "Stencil", "Tiepolo-Black",
  "Tiepolo-BlackItalic", "Tiepolo-Bold", "Tiepolo-BoldItalic",
  "Tiepolo-Book", "Tiepolo-BookItalic", "Times-Bold",
  "Times-BoldItalic", "Times-Italic", "Times-Roman",
  "Usherwood-Black", "Usherwood-BlackItalic", "Usherwood-Bold",
  "Usherwood-BoldItalic", "Usherwood-Book", "Usherwood-BookItalic",
  "Usherwood-Medium", "Usherwood-MediumItalic", "Veljovic-Black",
  "Veljovic-BlackItalic", "Veljovic-Bold", "Veljovic-BoldItalic",
  "Veljovic-Book", "Veljovic-BookItalic", "Veljovic-Medium",
  "Veljovic-MediumItalic", "Willow", "ZapfChancery-MediumItalic"
};

static int acaps[] =
{
  706, 706, 686, 686, 683, 683, 688, 688, 683, 683,
  683, 683, 686, 686, 740, 740, 740, 740, 700, 700,
  700, 700, 700, 706, 658, 658, 691, 691, 691, 691,
  691, 691, 691, 691, 681, 681, 681, 681, 681, 681,
  681, 681, 680, 680, 680, 680, 680, 680, 680, 680,
  715, 703, 703, 703, 703, 703, 703, 703, 703, 706,
  706, 706, 706, 562, 562, 562, 562, 714, 714, 714,
  714, 714, 714, 714, 714, 589, 666, 666, 666, 666,
  666, 666, 666, 666, 692, 692, 692, 692, 692, 692,
  692, 692, 658, 658, 682, 682, 680, 680, 680, 680,
  682, 682, 623, 630, 630, 623, 623, 630, 630, 627,
  623, 630, 630, 624, 622, 630, 630, 623, 682, 682,
  682, 682, 682, 660, 660, 660, 660, 660, 660, 739,
  718, 718, 718, 718, 695, 700, 667, 667, 667, 667,
  667, 667, 702, 702, 702, 702, 702, 714, 714, 714,
  714, 709, 709, 709, 709, 709, 709, 709, 709, 717,
  717, 750, 660, 660, 660, 660, 604, 685, 685, 685,
  685, 685, 685, 685, 685, 567, 850, 750, 630, 648,
  720, 720, 670, 670, 670, 670, 670, 670, 670, 670,
  732, 732, 732, 732, 748, 614, 614, 614, 614, 614,
  614, 676, 669, 653, 662, 627, 627, 627, 627, 627,
  627, 627, 627, 626, 626, 626, 626, 626, 626, 626,
  626, 750, 708
};

static int aenc[] =
{
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0
};

static char *dc[3][3] = {
  {"F", "E", "D"},
  {"G", "I", "C"},
  {"H", "A", "B"}
};

typedef struct ws_state_list_t
  {
    int conid, wtype, state;
#ifdef DPS
    FILE *stream;
    int fd;
#endif
    int empty, init, pages;

    int ix, iy;
    float a, b, c, d, e, f, g, h, mw, mh;
    int ytrans;
    float magstep;
    int stroke, limit, np;

    float red[980], green[980], blue[980];
    int color, fcol;

    float ysize;

    int len, column;
    char buffer[501];

    unsigned char ascii85_buffer[10];
    char a85line[76];
    int a85offset;

    float window[4], viewpt[4];

    int ltype;
    float cwidth, csize, cangle, cheight;
    int font, height;
  }
ws_state_list;

static
ws_state_list *p;

static
void fatal(char *text)
{
  fprintf(stderr, "%s\n", text);
  exit(1);
}

static
void set_norm_xform(int tnr, float *wn, float *vp)
{
  a[tnr] = (vp[1] - vp[0]) / (wn[1] - wn[0]);
  b[tnr] = vp[0] - wn[0] * a[tnr];
  c[tnr] = (vp[3] - vp[2]) / (wn[3] - wn[2]);
  d[tnr] = vp[2] - wn[2] * c[tnr];
}

static
void init_norm_xform(void)
{
  int tnr;

  for (tnr = 0; tnr < MAX_TNR; tnr++)
    set_norm_xform(tnr, gksl->window[tnr], gksl->viewport[tnr]);
}

static
void seg_xform(float *x, float *y)
{
  float xx;

  xx = *x * gksl->mat[0][0] + *y * gksl->mat[0][1] + gksl->mat[2][0];
  *y = *x * gksl->mat[1][0] + *y * gksl->mat[1][1] + gksl->mat[2][1];
  *x = xx;
}

static
void seg_xform_rel(float *x, float *y)
{
  float xx;

  xx = *x * gksl->mat[0][0] + *y * gksl->mat[0][1];
  *y = *x * gksl->mat[1][0] + *y * gksl->mat[1][1];
  *x = xx;
}

static
void dpsopen(float *sizex, float *sizey, int *format)
{
#ifdef DPS
  char *path, *delim, command[255];
  Display *dpy;
  Screen *screen;
  int dpi, width, height;

  if ((dpy = XOpenDisplay(NULL)) != NULL)
    {
      screen = XDefaultScreenOfDisplay(dpy);
      dpi = (int) (XWidthOfScreen(screen) / (XWidthMMOfScreen(screen) / 25.4));
      XCloseDisplay(dpy);
    }
  else
    {
      gks_fprintf(stderr, "GKS: can't open display\n");
      exit(-1);
    }

  path = (char *) getenv("GLI_HOME");
  if (path == NULL)
#ifdef VMS
    path = "sys$sysdevice:[gli]";
#else
#ifdef _WIN32
    path = "c:\\gli";
#else
    path = "/usr/local/gli";
#endif
#endif

  width = (int) (*sizex * dpi / 600);
  height = (int) (*sizey * dpi / 600);
#ifdef VMS
  delim = "";
#else
#ifdef _WIN32
  delim = "\\";
#else
  delim = "/";
#endif
#endif
  sprintf(command, "%s%spsprint -width %d -height %d %s",
	  path, delim, width, height, options[*format]);

  if ((p->stream = popen(command, "w")) == NULL)
    {
      gks_fprintf(stderr, "GKS: can't initiate pipe to DPS system\n");
      exit(-1);
    }
  else
    p->fd = fileno(p->stream);
#else
  gks_fprintf(stderr,
	      "GKS: X client does not have Display PostScript system\n");
#endif /* DPS */
}

static
void dpswrite(int nchars, char *chars)
{
#ifdef DPS
  if (nchars > 0)
    {
      if (write(p->fd, chars, nchars) != nchars)
	{
	  gks_fprintf(stderr, "GKS: can't write to DPS pipe\n");
	  exit(-1);
	}
    }
#endif /* DPS */
}

static
void dpsflush(void)
{
#ifdef DPS
  if (p->stream)
    fflush(p->stream);
#endif /* DPS */
}

static
void dpsclose(void)
{
#ifdef DPS
  if (p->stream)
    {
      fflush(p->stream);
      pclose(p->stream);
      p->stream = NULL;
    }
#endif /* DPS */
}

static
void packb(char *buff)
{
  int len, i;

  len = strlen(buff);

  if (buff[0] == '%')
    {
      if (p->column != 0)
	{
	  p->buffer[p->len++] = '\n';
	  p->column = 0;
	}
    }
  else if (len > 78 - p->column)
    {
      if (p->len != 0)
	{
	  p->buffer[p->len++] = '\n';
	  p->column = 0;
	}
    }

  if ((len + 2) > (500 - p->len))
    {
      if (p->wtype >= 63)
	dpswrite(p->len, p->buffer);
      else
	BUFOUT(&p->conid, &p->len, p->buffer, (unsigned short) p->len);
      p->len = 0;
    }

  if (p->column != 0)
    {
      p->buffer[p->len++] = ' ';
      p->column++;
    }

  for (i = 0; i < len; i++)
    {
      p->buffer[p->len++] = buff[i];
      p->column++;
    }

  if (buff[0] == '%')
    {
      p->buffer[p->len++] = '\n';
      p->column = 0;
    }
}

static
char *Ascii85Tuple(unsigned char *data)
{
  static char tuple[6];
  register unsigned int word, x;
  register unsigned short y;

  word = (((data[0] << 8) | data[1]) << 16) | (data[2] << 8) | data[3];
  if (word == 0L)
    {
      tuple[0] = 'z';
      tuple[1] = '\0';
      return (tuple);
    }
  x = (unsigned int) (word / (85L * 85 * 85 * 85));
  tuple[0] = x + '!';
  word -= x * (85L * 85 * 85 * 85);
  x = (unsigned int) (word / (85L * 85 * 85));
  tuple[1] = x + '!';
  word -= x * (85L * 85 * 85);
  x = (unsigned int) (word / (85 * 85));
  tuple[2] = x + '!';
  y = (unsigned short) (word - x * (85L * 85));
  tuple[3] = (y / 85) + '!';
  tuple[4] = (y % 85) + '!';
  tuple[5] = '\0';

  return (tuple);
}

static
void Ascii85Initialize(void)
{
  p->a85offset = 0;
  p->a85line[0] = '\0';
}

static
void Ascii85Flush(void)
{
  register char *tuple;

  packb(p->a85line);
  if (p->a85offset > 0)
    {
      p->ascii85_buffer[p->a85offset] = '\0';
      p->ascii85_buffer[p->a85offset + 1] = '\0';
      p->ascii85_buffer[p->a85offset + 2] = '\0';
      tuple = Ascii85Tuple(p->ascii85_buffer);
      packb(*tuple == 'z' ? "!!!!" : tuple);
    }
  packb("~>");
}

static
void Ascii85Encode(unsigned int code)
{
  int n, i = 0;
  register char *q;
  register unsigned char *c;
  char b[100];

  p->ascii85_buffer[p->a85offset] = code;
  p->a85offset++;
  if (p->a85offset < 4)
    return;
  c = p->ascii85_buffer;
  for (n = p->a85offset; n >= 4; n -= 4)
    {
      for (q = Ascii85Tuple(c); *q; q++)
	b[i++] = *q;
      c += 8;
    }
  p->a85offset = n;
  c -= 4;
  b[i] = '\0';
  strcat(p->a85line, b);
  if (strlen(p->a85line) == 75)
    {
      packb(p->a85line);
      p->a85line[0] = '\0';
    }
  for (n = 0; n < 4; n++)
    p->ascii85_buffer[n] = (*c++);
}

static
unsigned int LZWEncodeImage(unsigned int number_pixels, unsigned char *pixels)
{
#define LZWClr  256		/* Clear Table Marker */
#define LZWEod  257		/* End of Data marker */
#define OutputCode(code) \
{ \
    accumulator += ((long) code) << (32-code_width-number_bits); \
    number_bits += code_width; \
    while (number_bits >= 8) \
    { \
        Ascii85Encode((unsigned int) (accumulator >> 24)); \
        accumulator = accumulator << 8; \
        number_bits -= 8; \
    } \
}

  typedef struct _TableType
  {
    short
     prefix, suffix, next;
  }
  TableType;

  int index;
  register int i;
  short number_bits, code_width, last_code, next_index;
  TableType *table;
  unsigned long accumulator;

  /*
   * Allocate string table.
   */

  table = (TableType *) malloc((1 << 12) * sizeof(TableType));
  if (table == (TableType *) NULL)
    return (0);

  /*
   * Initialize variables.
   */
  accumulator = 0;
  code_width = 9;
  number_bits = 0;
  last_code = 0;
  Ascii85Initialize();
  OutputCode(LZWClr);
  for (index = 0; index < 256; index++)
    {
      table[index].prefix = (-1);
      table[index].suffix = index;
      table[index].next = (-1);
    }
  next_index = LZWEod + 1;
  code_width = 9;
  last_code = pixels[0];
  for (i = 1; i < (int) number_pixels; i++)
    {
      /*
       * Find string.
       */
      index = last_code;
      while (index != -1)
	if ((table[index].prefix != last_code) ||
	    (table[index].suffix != pixels[i]))
	  index = table[index].next;
	else
	  {
	    last_code = index;
	    break;
	  }
      if (last_code != index)
	{
	  /*
	   * Add string.
	   */
	  OutputCode(last_code);
	  table[next_index].prefix = last_code;
	  table[next_index].suffix = pixels[i];
	  table[next_index].next = table[last_code].next;
	  table[last_code].next = next_index;
	  next_index++;
	  /*
	   * Did we just move up to next bit width?
	   */
	  if ((next_index >> code_width) != 0)
	    {
	      code_width++;
	      if (code_width > 12)
		{
		  /*
		   * Did we overflow the max bit width?
		   */
		  code_width--;
		  OutputCode(LZWClr);
		  for (index = 0; index < 256; index++)
		    {
		      table[index].prefix = (-1);
		      table[index].suffix = index;
		      table[index].next = (-1);
		    }
		  next_index = LZWEod + 1;
		  code_width = 9;
		}
	    }
	  last_code = pixels[i];
	}
    }
  /*
   * Flush tables.
   */
  OutputCode(last_code);
  OutputCode(LZWEod);
  if (number_bits != 0)
    Ascii85Encode(accumulator >> 24);

  Ascii85Flush();
  free(table);

  return (0);
}

static
void set_xform(float *wn, float *vp, int *height, int wtype)
{
  p->e = (vp[1] - vp[0]) / (wn[1] - wn[0]);
  if (wtype == 92)
    {
      p->f = (1980 - 1) / 0.2794 * 600 / 180;
      p->h = (1440 - 1) / 0.2032 * 600 / 180;
    }
  else
    {
      p->f = (6750 - 1) / 0.28575;
      p->h = (4650 - 1) / 0.19685;
    }
  p->g = (vp[3] - vp[2]) / (wn[3] - wn[2]);

  p->a = p->e * p->f;
  p->b = p->f * (vp[0] - wn[0] * p->e);
  p->c = p->g * p->h;
  p->d = p->h * (vp[2] - wn[2] * p->g);

  p->mw = p->a * (wn[1] - wn[0]);
  p->mh = p->c * (wn[3] - wn[2]);

  *height = (int) p->c;

  p->stroke = 0;
}

static
void apply_magstep(int landscape, float magstep, float *sizex, float *sizey)
{
  float magnification;

  if (fabs(magstep) > FEPS)
    magnification = pow(1.2, magstep);
  else
    magnification = 1.0;

  *sizex = (landscape ? p->mh : p->mw) * magnification;
  *sizey = (landscape ? p->mw : p->mh) * magnification;
}

static
void bounding_box(int wtype, int landscape, float magstep)
{
  char buffer[50];
  int ix1, ix2, iy1, iy2;
  float magn;

  if (fabs(magstep) > FEPS)
    magn = pow(1.2, magstep);
  else
    magn = 1.0;

  if (wtype < 63)
    ix1 = 21 - (wtype % 2) * 3;
  else
    ix1 = 0;
  iy1 = wtype < 63 ? 15 : 0;
  ix2 = ix1 + NINT((landscape ? p->mh : p->mw) * 72 / 600 * magn);
  iy2 = iy1 + NINT((landscape ? p->mw : p->mh) * 72 / 600 * magn);
  p->ytrans = landscape ? iy2 : iy1;

  sprintf(buffer, "%%%%BoundingBox: %d %d %d %d", ix1, iy1, ix2, iy2);
  packb(buffer);
}

static
void move(float *x, float *y)
{
  char buffer[50];

  p->ix = NINT(p->a * (*x) + p->b);
  p->iy = NINT(p->c * (*y) + p->d);

  if (p->stroke)
    {
      packb("sk");
      p->stroke = 0;
    }
  sprintf(buffer, "np %d %d m", p->ix, p->iy);
  packb(buffer);
  p->np = 1;
}

static
void draw(float *x, float *y)
{
  char buffer[50];
  int jx, jy, rx, ry;

  jx = p->ix;
  jy = p->iy;
  p->ix = NINT(p->a * (*x) + p->b);
  p->iy = NINT(p->c * (*y) + p->d);

  if (p->np == 1 || p->ix != jx || p->iy != jy)
    {
      rx = p->ix - jx;
      ry = p->iy - jy;
      if (abs(rx) > 1 || abs(ry) > 1)
	{
	  sprintf(buffer, "%d %d rl", rx, ry);
	  packb(buffer);
	}
      else
	packb(dc[rx + 1][ry + 1]);
      p->np++;

      if (p->np == p->limit)
	{
	  packb("sk");
	  p->stroke = 0;
	  sprintf(buffer, "%d %d m", p->ix, p->iy);
	  packb(buffer);
	  p->np = 1;
	}
      else
	p->stroke = 1;
    }
}

static
void moveto(float x, float y)
{
  char buffer[20];

  p->ix = NINT(x);
  p->iy = NINT(y);

  sprintf(buffer, "%d %d m", p->ix, p->iy);
  packb(buffer);
}

static
void amoveto(int angle, float x, float y)
{
  char buffer[30];

  p->ix = NINT(x);
  p->iy = NINT(y);

  sprintf(buffer, "%d %d %d am", angle, p->ix, p->iy);
  packb(buffer);
}

static
void set_linetype(int ltype)
{
  char buffer[50];
  static char *pattern[] =
  {
    "[20 15 20 15 20 15 20 30]", "[20 15 20 15 20 30]",
    "[20 15 20 30]", "[15 10 15 10 15 10 15 30]",
    "[15 10 15 10 15 30]", "[15 10 15 30]",
    "[15 10 15 10 15 10 15 20]", "[15 10 15 10 15 20]",
    "[15 10 15 20]", "[0 10]", "[0 20]", "[0 40]", "[0 50]",
    "[0 15 0 15 0 30]", "[0 15 0 30]", "[30 15 0 15 0 15 0 15]",
    "[30 15 0 15 0 15]", "[30 15 0 15]", "[60 20 30 20]",
    "[60 20]", "[30 30]", "[30 20]",
    "[0 30 0 30 0 60]", "[0 30 0 60]", "[0 60]", "[60 60]",
    "[120 40 60 40]", "[120 40]", "[60 30 0 30 0 30 0 30]",
    "[60 30 0 30 0 30]", "[]",
    "[]", "[60 40]", "[0 30]", "[60 30 0 30]"};

  if (ltype != p->ltype)
    {
      p->ltype = ltype;
      sprintf(buffer, "%s 0 setdash", pattern[ltype + 30]);
      packb(buffer);

      if (ltype == 1)
	packb("0 setlinecap");
      else
	packb("1 setlinecap");
    }
}

static
void set_linewidth(float width)
{
  char buffer[20];

  if (fabs(width - p->cwidth) > FEPS)
    {
      p->cwidth = fabs(width);
      sprintf(buffer, "%.4g lw", 4 * p->cwidth);
      packb(buffer);
    }
}

static
void set_markersize(float size)
{
  char buffer[20];

  if (fabs(size - p->csize) > FEPS)
    {
      p->csize = fabs(size);
      sprintf(buffer, "%.4g ms", p->csize);
      packb(buffer);
    }
}

static
void set_markerangle(float angle)
{
  char buffer[20];

  if (fabs(angle - p->cangle) > FEPS)
    {
      p->cangle = fabs(angle);
      sprintf(buffer, "%.4g ma", p->cangle);
      packb(buffer);
    }
}

static
void gkinfo(int *nchars, char *chars)
{
  char *date, *user, host[100];
  time_t elapsed_time;
#ifdef hpux
  struct utsname utsname;
#endif
#ifdef _WIN32
  char lpBuffer[100];
  DWORD nSize = 100;
#endif

  time(&elapsed_time);
  date = ctime(&elapsed_time);

#ifndef _WIN32
  user = (char *) getenv("USER");
#else
  if (GetUserName(lpBuffer, &nSize) != 0)
    {
      user = lpBuffer;
      lpBuffer[nSize] = '\0';
    }
  else
    user = NULL;
#endif
  if (user == NULL)
    user = "(?)";

#ifdef VMS
  strcpy(host, (char *) getenv("SYS$NODE"));
#else
#ifdef hpux
  uname(&utsname);
  strcpy(host, utsname.nodename);
#else
#if defined(OS2) || (defined(_WIN32) && !defined(__GNUC__))
  strcpy(host, "(unknown)");	/* FIXME */
#else
  gethostname(host, 100);
#endif /* _WIN32 */
#endif /* hpux */
#endif /* VMS */

  strtok(date, "\n");
  strtok(host, ".");

  sprintf(chars, "%s  by user  %s @ %s", date, user, host);
  *nchars = strlen(chars);
}

static
void ps_header(void)
{
  int nchars;
  char info[150], buffer[150];

  gkinfo(&nchars, info);
  packb("%!PS-Adobe-2.0");
  if (nchars > 0)
    {
      sprintf(buffer, "\
%%%%Creator: %s, GLI GKS PostScript Device Handler, V4.5", info + 35);
      packb(buffer);
      info[24] = '\0';
      sprintf(buffer, "%%%%+CreationDate: %s", info);
      packb(buffer);
    }
  else
    packb("%%Creator: GLI GKS PostScript Device Handler, V4.5");
  packb("%%+Copyright @ 1993-1995, J.Heinen");
  packb("%%Pages: (at end)");
}

static
void set_color(int color, int wtype)
{
  char buffer[50];
  float grey;
  int index, pixel;

  if (color < 980)
    {
      if (color != p->color)
	{
	  index = abs(color);
	  if (wtype > 63 && X11)
	    {
	      GQPIX(&index, &pixel);
	      sprintf(buffer, "%d sp", pixel);
	      packb(buffer);
	    }
	  else
	    {
	      if (wtype % 2)
		{
		  grey = 0.3 * p->red[index] + 0.59 * p->green[index] +
			 0.11 * p->blue[index];
		  sprintf(buffer, "%.4g sg", grey);
		  packb(buffer);
		}
	      else
		{
		  sprintf(buffer, "%.4g %.4g %.4g sc", p->red[index],
			  p->green[index], p->blue[index]);
		  packb(buffer);
		}
	    }
	  p->color = index;
	}
    }
}

static
void set_foreground(int color, int wtype)
{
  char buffer[50];
  int index, pixel;
  float grey;

  if (color < 980)
    {
      if (color != p->fcol)
	{
	  index = abs(color);
	  if (wtype >= 63 && X11)
	    {
	      GQPIX(&index, &pixel);
	      sprintf(buffer, "/fg {%d sp} def", pixel);
	      packb(buffer);
	    }
	  else
	    {
	      if (wtype % 2)
		{
		  grey = 0.3 * p->red[index] + 0.59 * p->green[index] +
			 0.11 * p->blue[index];
		  sprintf(buffer, "/fg {%.4g sg} def", grey);
		  packb(buffer);
		}
	      else
		{
		  sprintf(buffer, "/fg {%.4g %.4g %.4g sc} def",
			  p->red[index], p->green[index], p->blue[index]);
		  packb(buffer);
		}
	    }
	  p->fcol = index;
	}
      if (color != p->color)
	{
	  index = abs(color);
	  packb("fg");
	  p->color = index;
	}
    }
}

static
void set_background(int wtype)
{
  char buffer[50];
  int index = 0, pixel;
  float grey;

  if (wtype >= 63 && X11)
    {
      GQPIX(&index, &pixel);
      sprintf(buffer, "/bg {%d sp} def", pixel);
      packb(buffer);
    }
  else
    {
      if (wtype % 2)
	{
	  grey = 0.3 * p->red[0] + 0.59 * p->green[0] + 0.11 * p->blue[0];
	  sprintf(buffer, "/bg {%.4g sg} def", grey);
	  packb(buffer);
	}
      else
	{
	  sprintf(buffer, "/bg {%.4g %.4g %.4g sc} def",
		  p->red[0], p->green[0], p->blue[0]);
	  packb(buffer);
	}
    }
}

static
void update(void)
{
  char lf = '\n';
  int nbytes = 1;

  if (p->column != 0)
    {
      p->buffer[p->len++] = '\n';
      p->column = 0;
    }
  if (p->wtype >= 63)
    {
      dpswrite(p->len, p->buffer);
      dpswrite(nbytes, &lf);
      dpsflush();
    }
  else
    BUFOUT(&p->conid, &p->len, p->buffer, (unsigned short) p->len);
  p->len = 0;
}

static
void set_clipping(float *clrt)
{
  int i, j;
  int ix1, ix2, iy1, iy2;
  float cx1, cy1, cx2, cy2;
  char buffer[100];

  i = clrt[0] < clrt[1] ? 0 : 1;
  j = clrt[2] < clrt[3] ? 2 : 3;

  NDC_to_DC(clrt[i], clrt[j], cx1, cy1);
  NDC_to_DC(clrt[1 - i], clrt[5 - j], cx2, cy2);

  ix1 = ((int) cx1) - 2;
  iy1 = ((int) cy1) - 2;
  ix2 = NINT(cx2) + 2;
  iy2 = NINT(cy2) + 2;

  sprintf(buffer, "np %d %d m %d %d l %d %d l %d %d l cp clip",
	  ix1, iy1, ix1, iy2, ix2, iy2, ix2, iy1);
  packb(buffer);
}

static
void set_font(int font, int height)
{

  float scale, w, h, ux, uy, chh, acap;
  char buffer[200];
  int family, size;

  scale = sqrt(gksl->chup[0]*gksl->chup[0] + gksl->chup[1]*gksl->chup[1]);
  ux = gksl->chup[0] / scale * gksl->chh;
  uy = gksl->chup[1] / scale * gksl->chh;
  WC_to_NDC_rel(ux, uy, gksl->cntnr, ux, uy);

  w = 0;
  h = sqrt(ux*ux + uy*uy);
  seg_xform_rel(&w, &h);

  chh = sqrt(w*w + h*h);

  if ((font != p->font) || (fabs(chh - p->cheight) > FEPS))
    {
      p->font = abs(font);
      p->cheight = fabs(chh);

      if (p->font > 10000 && p->font <= 10223)
	{
	  family = p->font - 10001;

	  p->ysize = p->cheight * height;
	  acap = ((float) acaps[family]) / 1000.0;
	  size = MIN(MAX(NINT(p->ysize / acap), 1), 7200);

	  if (aenc[family] == 0)
	    {
	      sprintf(buffer, "gsave /%s_ ISOLatin1Encoding", afonts[family]);
	      packb(buffer);
	      sprintf(buffer, "/%s encodefont pop grestore", afonts[family]);
	      packb(buffer);
	      sprintf(buffer, "/%s_ findfont %d scalefont setfont",
		      afonts[family], size);
	      packb(buffer);
	    }
	  else
	    {
	      sprintf(buffer, "/%s findfont %d scalefont setfont",
		      afonts[family], size);
	      packb(buffer);
	    }
	}
      else
	{
	  if (p->font >= 101 && p->font <= 143)
	    family = p->font - 101;
	  else if (p->font >= 1 && p->font <= 32)
	    family = map[p->font - 1] - 1;
	  else
	    family = 8;

	  p->ysize = p->cheight * height;
	  size = MIN(MAX((int) (p->ysize / caps[family]), 1), 7200);

	  if (family != 12 && family != 41 && family != 42)
	    {
	      sprintf(buffer, "gsave /%s_ ISOLatin1Encoding", fonts[family]);
	      packb(buffer);
	      sprintf(buffer, "/%s encodefont pop grestore", fonts[family]);
	      packb(buffer);
	      sprintf(buffer, "/%s_ findfont %d scalefont setfont",
		      fonts[family], size);
	      packb(buffer);
	    }
	  else
	    {
	      sprintf(buffer, "/%s findfont %d scalefont setfont",
		      fonts[family], size);
	      packb(buffer);
	    }
	}
    }
}

static
void get_magstep(float *magstep, int *dpi)
{
  char *env;
#ifdef DPS
  Display *dpy;
  Screen *screen;
#endif /* DPS */

  if ((env = (char *) getenv("GLI_GKS_MAGSTEP")) != NULL)
    *magstep = atof(env);
  else
    *magstep = 0;

  *dpi = 75;
#ifdef DPS
  if ((dpy = XOpenDisplay(NULL)) != NULL)
    {
      screen = XDefaultScreenOfDisplay(dpy);
      *dpi = (int) (XWidthOfScreen(screen) /
		    (XWidthMMOfScreen(screen) / 25.4));
      XCloseDisplay(dpy);
    }
#endif /* DPS */
}

static
void ps_init(int *pages)
{
  int dpi, form;
  int landscape;
  float sizex, sizey;
  char buffer[30];

  landscape = (p->viewpt[1] - p->viewpt[0]) -
    (p->wtype == 92 ? 0.2032 : 0.19685) > FEPS;
  if (*pages == 0)
    {
      get_magstep(&p->magstep, &dpi);
      if (p->wtype == 92)
	p->magstep = log(180.0 / dpi) / log(1.2);

      if (p->wtype >= 63)
	{
	  apply_magstep(landscape, p->magstep, &sizex, &sizey);
	  form = (p->wtype == 92 ? 0 : 1);
	  dpsopen(&sizex, &sizey, &form);
	}

      ps_header();
      bounding_box(p->wtype, landscape, p->magstep);
      packb("%%EndComments");
      packb("%%BeginProcSet GLI 4.5");

      packb("save /GLI_GKS_save exch def");
      packb("/GLI_GKS_dict 150 dict def GLI_GKS_dict begin");

      packb("/in {72 mul} def");
      packb("/np {newpath} def");
      packb("/cp {closepath} def");
      packb("/m {moveto} def");
      packb("/l {lineto} def");
      packb("/A {1 0 rlineto} def");
      packb("/B {1 1 rlineto} def");
      packb("/C {0 1 rlineto} def");
      packb("/D {-1 1 rlineto} def");
      packb("/E {-1 0 rlineto} def");
      packb("/F {-1 -1 rlineto} def");
      packb("/G {0 -1 rlineto} def");
      packb("/H {1 -1 rlineto} def");
      packb("/I {1 0 rlineto -1 0 rlineto} def");
      packb("/am {np gsave translate rotate 0 0 m} def");
      packb("/gr {grestore} def");
      packb("/rm {rmoveto} def");
      packb("/srm {rxy rx s mul ry s mul rm} def");
      packb("/rl {rlineto} def");
      packb("/srl {rxy rx s mul ry s mul rl} def");
      packb("/sk {stroke} def");
      packb("/csk {closepath stroke} def");
      packb("/fi {closepath eofill} def");
      packb("/sg {setgray} def");
      packb("/sc {setrgbcolor} def");

      if (p->wtype >= 63 && X11)
	packb("/sp {dup [/DevicePixel 8] setcolorspace pop setcolor} def");

      packb("/fg {0 sg} def");

      set_background(p->wtype);

      packb("/lw {setlinewidth} def");
      packb("/ms {/s exch def} def");
      packb("/ma {/a exch def} def");
      packb("/ct {dup stringwidth pop 2 div neg 0 rmoveto show} def");
      packb("/rj {dup stringwidth pop neg 0 rmoveto show} def");
      packb("/lj {show} def");
      packb("/xy {/y exch def /x exch def} def");
      packb("/rxy {/ry exch def /rx exch def} def");
      packb("/sxy {gsave xy x y translate a rotate x neg y neg translate} def");
      packb("/dt {xy np fg x y s 0 360 arc fi} def");
      packb("/pl {sxy np x y m fg -24 0 srl 48 0 srl\
 -24 0 srl 0 24 srl 0 -48 srl sk gr} def");
      packb("/as {np x y m 0 24 srm 14 -43.4 srl\
 -36.8 26.8 srl 45.6 0 srl -36.8 -26.8 srl");
      packb("14 43.4 srl 14 -43.4 srl} def");
      packb("/fas {sxy fg as fill fg as csk gr} def");
      packb("/dc {sxy np x y m fg -24 24 srl 48 -48 srl\
 -24 24 srl -24 -24 srl 48 48 srl");
      packb("sk gr} def");
      packb("/sq {np x y m 0 24 srm 24 0 srl 0 -48 srl\
 -48 0 srl 0 48 srl 24 0 srl} def");
      packb("/nsq {sxy bg sq fi fg sq csk gr} def");
      packb("/fsq {sxy fg sq fi fg sq csk gr} def");
      packb("/ci {np x y 24 s mul 0 360 arc} def");
      packb("/nci {xy bg ci fi fg ci sk} def");
      packb("/fci {xy fg ci fi fg ci sk} def");
      packb("/tu {np x y m 0 28 srm -24 -42 srl 48 0 srl -24 42 srl} def");
      packb("/ntu {sxy bg tu fi fg tu csk gr} def");
      packb("/ftu {sxy fg tu fi fg tu csk gr} def");
      packb("/td {np x y m 0 -28 srm -24 42 srl 48 0 srl -24 -42 srl} def");
      packb("/ntd {sxy bg td fi fg td csk gr} def");
      packb("/ftd {sxy fg td fi fg td csk gr} def");
      packb("/dm {np x y m 0 24 srm -24 -24 srl\
 24 -24 srl 24 24 srl -24 24 srl} def");
      packb("/ndm {sxy bg dm fi fg dm csk gr} def");
      packb("/fdm {sxy fg dm fi fg dm csk gr} def");
      packb("/bt {np x y m -30 24 srl 0 -48 srl\
 60 48 srl 0 -48 srl -30 24 srl} def");
      packb("/nbt {sxy bg bt fi fg bt csk gr} def");
      packb("/fbt {sxy fg bt fi fg bt csk gr} def");
      packb("/hg {np x y m -24 30 srl 48 0 srl\
 -48 -60 srl 48 0 srl -24 30 srl} def");
      packb("/nhg {sxy bg hg fi fg hg csk gr} def");
      packb("/fhg {sxy fg hg fi fg hg csk gr} def");
      packb("/st {sxy bg as fi fg as csk gr} def");
      packb("/fst {fas} def");
      packb("/tud {sxy bg tu fi bg td fi fg tu csk fg td csk gr} def");
      packb("/tl {np x y m -14 0 srm 42 -24 srl 0 48 srl -42 -24 srl} def");
      packb("/ftl {sxy fg tl fi fg tl csk gr} def");
      packb("/tr {np x y m 28 0 srm -42 -24 srl 0 48 srl 42 -24 srl} def");
      packb("/ftr {sxy fg tr fi fg tr csk gr} def");
      packb("/hpl {np x y m 0 24 srm 8 0 srl\
 0 -16 srl 16 0 srl 0 -16 srl -16 0 srl");
      packb("0 -16 srl -16 0 srl 0 16 srl -16 0 srl\
 0 16 srl 16 0 srl 0 16 srl 8 0 srl} def");
      packb("/npl {sxy bg hpl fi fg hpl csk gr} def");
      packb("/om {np x y m 0 24 srm 16 0 srl\
 8 -8 srl 0 -32 srl -8 -8 srl -32 0 srl");
      packb("-8 8 srl 0 32 srl 8 8 srl 16 0 srl} def");
      packb("/nom {sxy bg om fi fg om csk gr} def");
      packb("/pat1 {/px exch def /pa 16 array def 0 1 15\
 {/py exch def /pw 2 string def");
      packb("pw 0 px py 2 mul 2 getinterval putinterval\
 pa py pw put} for} def");
      packb("/pat2 {/pi exch def /cflag exch def save\
 cflag 1 eq {eoclip} {clip}");
      packb("ifelse newpath {clippath pathbbox} stopped\
 not {/ph exch def /pw exch def");
      packb("/py exch def /px exch def /px px 256 div floor 256 mul def");
      packb("/py py 256 div floor 256 mul def px py translate\
 /pw pw px sub 256 div");
      packb("floor 1 add cvi def /ph ph py sub 256 div floor 1 add cvi def");
      packb("pw 256 mul ph 256 mul scale /pw pw 32 mul def /ph ph 32 mul def");
      packb("/px 0 def /py 0 def pw ph pi\
 [pw neg 0 0 ph neg pw ph] {pa py get");
      packb("/px px 16 add def px pw ge {/px 0 def\
 /py py 1 add 16 mod def} if} pi type");
      packb("/booleantype eq {imagemask} {image} ifelse} if restore} def");
      packb("/bp {closepath gsave} def");
      packb("/ep {pat1 1 1 pat2 grestore} def");

      packb("/OF /findfont load def");
      packb("/findfont {dup GLI_GKS_dict exch known");
      packb("{GLI_GKS_dict exch get}");
      packb("if GLI_GKS_dict /OF get exec} def");
      packb("mark");
      packb("/ISOLatin1Encoding 8#000 1 8#001 {StandardEncoding exch get} for");
      packb("/emdash /endash 8#004 1 8#025 {StandardEncoding exch get} for");
      packb("/quotedblleft /quotedblright 8#030 1 8#054\
 {StandardEncoding exch get} for");
      packb("/minus 8#056 1 8#217 {StandardEncoding exch get} for");
      packb("/dotlessi 8#301 1 8#317 {StandardEncoding exch get} for");
      packb("/space/exclamdown/cent/sterling/currency/yen/brokenbar/section");
      packb("/dieresis/copyright/ordfeminine/guillemotleft\
/logicalnot/hyphen/registered");
      packb("/macron/degree/plusminus/twosuperior\
/threesuperior/acute/mu/paragraph");
      packb("/periodcentered/cedilla/onesuperior/ordmasculine\
/guillemotright/onequarter");
      packb("/onehalf/threequarters/questiondown/Agrave\
/Aacute/Acircumflex/Atilde");
      packb("/Adieresis/Aring/AE/Ccedilla/Egrave/Eacute\
/Ecircumflex/Edieresis/Igrave");
      packb("/Iacute/Icircumflex/Idieresis/Eth/Ntilde/Ograve\
/Oacute/Ocircumflex/Otilde");
      packb("/Odieresis/multiply/Oslash/Ugrave/Uacute\
/Ucircumflex/Udieresis/Yacute/Thorn");
      packb("/germandbls/agrave/aacute/acircumflex/atilde\
/adieresis/aring/ae/ccedilla");
      packb("/egrave/eacute/ecircumflex/edieresis/igrave\
/iacute/icircumflex/idieresis");
      packb("/eth/ntilde/ograve/oacute/ocircumflex/otilde\
/odieresis/divide/oslash/ugrave");
      packb("/uacute/ucircumflex/udieresis/yacute/thorn\
/ydieresis");
      packb("256 array astore def cleartomark");
      packb("/encodefont {findfont dup maxlength dict begin");
      packb("{1 index /FID ne {def} {pop pop} ifelse} forall");
      packb("/Encoding exch def dup");
      packb("/FontName exch def currentdict");
      packb("definefont end} def");
      packb("end");

      packb("%%EndProcSet");
      packb("%%EndProlog");
    }

  (*pages)++;
  sprintf(buffer, "%%%%Page: %d %d", *pages, *pages);
  packb(buffer);

  packb("%%BeginPageSetup");
  packb("GLI_GKS_dict begin save /psl exch def");

  if (p->wtype >= 63)
    {
      packb("initgraphics");
      packb("1 setgray clippath fill");
    }

  if (p->wtype < 63)
    {
      if (landscape)
	{
	  sprintf(buffer, "%d %d  translate -90 rotate",
		  p->wtype % 2 ? 18 : 21, p->ytrans);
	  packb(buffer);
	}
      else
	{
	  sprintf(buffer, "%d 15 translate", p->wtype % 2 ? 18 : 21);
	  packb(buffer);
	}
    }
  else if (landscape)
    {
      sprintf(buffer, "0 %d translate -90 rotate", p->ytrans);
      packb(buffer);
    }
  if (fabs(p->magstep) > FEPS)
    {
      sprintf(buffer, "%.4g 1 in 600 div mul dup scale", pow(1.2, p->magstep));
      packb(buffer);
    }
  else
    packb("1 in 600 div dup scale");

  set_color(-1, p->wtype);
  set_foreground(-1, p->wtype);
  packb("0 setlinecap 1 setlinejoin");
  set_linewidth(-1.0);
  set_markersize(-1.0);
  packb("0 ma");
  set_font(-1, p->height);
  set_clipping(p->window);
  packb("%%EndPageSetup");
  update();
}

static
void end_page(int pages)
{
  char buffer[30];

  sprintf(buffer, "%%%%EndPage: %d %d", pages, pages);
  packb(buffer);
}


static
void set_colortable(void)
{
  int i, j;

  for (i = 0; i < 980; i++)
    {
      j = i;
      GQRGB(&j, p->red + j, p->green + j, p->blue + j);
    }
  p->color = -1;
}

static
void set_color_rep(int color, float red, float green, float blue)
{
  if (color >= 0 && color < 980)
    {
      p->red[color] = red;
      p->green[color] = green;
      p->blue[color] = blue;
    }
}

static
void query_color(int index, unsigned char **buf, int wtype)
{
  float grey;

  index %= 980;

  if (wtype % 2)
    {
      grey = 0.3 * p->red[index] + 0.59 * p->green[index] +
	     0.11 * p->blue[index];
      **buf = (char) NINT(grey * 255);
      (*buf)++;
    }
  else
    {
      **buf = (char) NINT(p->red[index] * 255);
      (*buf)++;
      **buf = (char) NINT(p->green[index] * 255);
      (*buf)++;
      **buf = (char) NINT(p->blue[index] * 255);
      (*buf)++;
    }
}

static
void set_connection(int conid, int wtype)
{
  p = (ws_state_list *) calloc(1, sizeof(struct ws_state_list_t));
  p->conid = conid;
  p->wtype = wtype;
#ifdef DPS
  p->stream = NULL;
  p->fd = 0;
#endif

  p->window[0] = 0;
  p->window[1] = 1;
  p->window[2] = 0;
  p->window[3] = 1;

  p->viewpt[0] = 0;
  if (p->wtype == 92)
    p->viewpt[1] = 0.2032;
  else
    p->viewpt[1] = 0.19685;
  p->viewpt[2] = 0;
  p->viewpt[3] = p->viewpt[1];

  set_xform(p->window, p->viewpt, &p->height, p->wtype);

  p->pages = 0;
  p->init = 0;
  p->empty = 1;
  p->color = 1;
  p->len = p->column = 0;
  p->font = 0;
  p->ltype = GLSOLI;
  p->cwidth = p->csize = p->cangle = p->cheight = 0.0;
}

static
void marker_routine(float *x, float *y, int *marker)
{
  float dx, dy;
  char buffer[50];
  static char *macro[] =
  {
    "nom", "npl", "ftr", "ftl", "tud", "fst", " st", "fdm", "ndm", "fhg",
    "nhg", "fbt", "nbt", "fsq", "nsq", "ftd", "ntd", "ftu", "ntu", "fci",
    " dt", " dt", " pl", "fas", "nci", " dc"
  };

  NDC_to_DC(*x, *y, dx, dy);

  p->ix = NINT(dx);
  p->iy = NINT(dy);
  sprintf(buffer, "%d %d %s", p->ix, p->iy, macro[*marker + 20]);
  packb(buffer);
}

static
void cell_array(
  float xmin, float xmax, float ymin, float ymax,
  int dx, int dy, int dimx, int *colia, int wtype)
{
  char buffer[100];
  unsigned char *buf, *bufP;
  int clsw;
  float clrt[4], x1, x2, y1, y2;
  int w, h, x, y;

  int i, j, ii, jj, ci, len, swap = 0;
  int tnr;

  tnr = gksl->cntnr;

  WC_to_NDC(xmin, ymax, tnr, x1, y1);
  seg_xform(&x1, &y1);
  NDC_to_DC(x1, y1, x1, y1);

  WC_to_NDC(xmax, ymin, tnr, x2, y2);
  seg_xform(&x2, &y2);
  NDC_to_DC(x2, y2, x2, y2);

  w = NINT(fabs(x2 - x1));
  h = NINT(fabs(y2 - y1));
  x = NINT(MIN(x1, x2));
  y = NINT(MIN(y1, y2));

  packb("gsave");

  clsw = gksl->clip;
  for (i = 0; i < 4; i++)
    clrt[i] = gksl->viewport[clsw == GCLIP ? tnr : 0][i];

  set_clipping(clrt);

  packb("/RawData currentfile /ASCII85Decode filter def");
  packb("/Data RawData << >> /LZWDecode filter def");

  sprintf(buffer, "%d %d translate", x, y);
  packb(buffer);

  sprintf(buffer, "%d %d scale", w, h);
  packb(buffer);

  sprintf(buffer, "/Device%s setcolorspace", wtype % 2 == 0 ? "RGB" : "Gray");
  packb(buffer);

  if (x1 > x2)
    swap = 1;
  if (y1 > y2)
    swap += 2;

  packb("{ << /ImageType 1");

  sprintf(buffer, "/Width %d /Height %d", dx, dy);
  packb(buffer);
  if (swap == 0)
    sprintf(buffer, "/ImageMatrix [%d 0 0 -%d 0 %d]", dx, dy, dy);
  else if (swap == 1)
    sprintf(buffer, "/ImageMatrix [-%d 0 0 -%d %d %d]", dx, dy, dx, dy);
  else if (swap == 2)
    sprintf(buffer, "/ImageMatrix [%d 0 0 %d 0 0]", dx, dy);
  else
    sprintf(buffer, "/ImageMatrix [-%d 0 %d %d 0 0]", dx, dx, dy);
  packb(buffer);

  sprintf(buffer, "/DataSource Data /BitsPerComponent 8 /Decode [0 1%s]",
    wtype % 2 == 0 ? " 0 1 0 1" : "");
  packb(buffer);

  packb(">> image Data closefile RawData flushfile } exec"); 

  len = dx * dy;
  if (wtype % 2 == 0) len = len * 3;

  buf = (unsigned char *) malloc(len);
  bufP = buf;
  ii = jj = 0;
  for (j = 1; j <= dy; j++)
    {
      for (i = 1; i <= dx; i++)
	{
	  ci = colia[jj + i - 1];
	  if (ci >= 588)
	    ci = 80 + (ci - 588) / 56 * 12 + NINT((ci - 588) % 56) * 11.0 / 56;
	  else if (ci >= 257)
	    ci = 8 + NINT((ci - 257) / 330.0 * (72 - 1));
	  else if (ci < 0)
	    ci = 0;
	  query_color(ci, &bufP, wtype);
	}
      jj += dimx;
    }
  LZWEncodeImage(len, buf);
  free(buf);

  packb("grestore");
}

static
void text_routine(float *x, float *y, int *nchars,
#ifdef VMS
  struct dsc$descriptor *text)
#else
#ifdef cray
  _fcd text)
#else
  char *text)
#endif /* cray */
#endif /* VMS */
{
  char *chars;
  int i, j;
  float ux, uy, yrel, phi;
  float xorg, yorg;
  int alh, alv, ic;
  char str[500], buffer[510];
  int prec, angle;

#ifdef VMS
  chars = text->dsc$a_pointer;
#else
#ifdef cray
  chars = _fcdtocp(text);
#else
  chars = text;
#endif /* cray */
#endif /* VMS */

  NDC_to_DC(*x, *y, xorg, yorg);

  prec = gksl->asf[6] ? gksl->txprec : predef_prec[gksl->tindex - 1];
  alh = gksl->txal[0];
  alv = gksl->txal[1];

  WC_to_NDC_rel(gksl->chup[0], gksl->chup[1], gksl->cntnr, ux, uy);
  seg_xform_rel(&ux, &uy);
  angle = NINT(-atan2(ux, uy) * 180.0 / M_PI);

  if (prec == GSTRP)
    {
      phi = angle / 180.0 * M_PI;
      yrel = p->ysize * yfac[alv - GAVNOR];
      xorg -= yrel * sin(phi);
      yorg += yrel * cos(phi);
    }

  if (angle == 0)
    moveto(xorg, yorg);
  else
    amoveto(angle, xorg, yorg);

  for (i = 0, j = 0; i < *nchars; i++)
    {
      ic = chars[i];
      if (ic < 0)
	ic += 256;
      if (ic < 127)
	{
	  if (strchr("()\\", ic) != NULL)
	    str[j++] = '\\';
	  str[j++] = chars[i];
	}
      else
	{
	  sprintf(str+j, "\\%03o", ic);
	  j += 4;
	}
      str[j] = '\0';
    }
  sprintf(buffer, "(%s) %s", str, show[alh + GAHNOR]);
  packb(buffer);
  if (angle != 0)
    packb("gr");
}

static
void fillpattern_routine(int *n, float *px, float *py, int *tnr, int *pattern)
{
  int pa[33], i, j, k, ltype = 0;
  char str[65], buffer[100];
  char hex[16] = "0123456789ABCDEF";

  p->limit = 0;
  GPOLIN(n, px, py, &ltype, tnr, move, draw);
  GKQPA(pattern, pa);
  k = pa[0];
  if (k == 4 || k == 8)
    j = 2;
  else if (k == 32)
    j = 1;
  else
    fatal("GKS: invalid pattern");
  k = 1;
  for (i = 1; i <= 32; i++)
    {
      str[2 * (i - 1)] = hex[pa[k] / 16];
      str[2 * (i - 1) + 1] = hex[pa[k] % 16];
      if ((i % j) == 0)
	k++;
      if (k > pa[0])
	k = 1;
    }
  str[64] = '\0';
  sprintf(buffer, "bp <%s> ep", str);
  packb(buffer);
}

static
void fill_routine(int *n, float *px, float *py, int *tnr)
{
  int ltype = 0;

  p->limit = 0;
  GPOLIN(n, px, py, &ltype, tnr, move, draw);
  packb("fi");
}

static
void line_routine(int *n, float *px, float *py, int *ltype, int *tnr)
{
  p->limit = 1000;
  GPOLIN(n, px, py, ltype, tnr, move, draw);
  if (p->stroke)
    {
      packb("sk");
      p->stroke = 0;
    }
}

void STDCALL GKDPS(
  int *fctid, int *dx, int *dy, int *dimx, int *ia,
  int *lr1, float *r1, int *lr2, float *r2, int *lc, CHARARG(chars),
  ws_state_list **ptr)
{
  int style, color, pattern, ltype;
  float yres, width, size, factor, x, y, angle;
  char buffer[100];
  int font, tnr, prec;
  int nchars;

  p = *ptr;

  switch (*fctid)
    {
/* open workstation */
    case 2:
      gksl = (gks_state_list *) (ia + 4);

      init_norm_xform();
      set_connection(ia[1], ia[2]);
      set_colortable();
      if (sizeof(char *) > sizeof(int))
	{
	  long *la = (long *) ia;
	  *la = (long) p;
	}
      else
	*ia = (long) p;

#if defined(hpux) && !defined(NAGware)
      line_routine_a = (int (*)())line_routine;
      fill_routine_a = (int (*)())fill_routine;
#endif
      break;

/* close workstation */
    case 3:
      if (p->init)
	{
	  if (!p->empty && (p->wtype < 63))
	    packb("showpage");
	  packb("psl restore end % GLI_GKS_dict");
	  end_page(p->pages);
	  packb("%%Trailer");
	  packb("GLI_GKS_save restore");
	}
      if (p->pages > 0)
	{
	  sprintf(buffer, "%%%%Pages: %d", p->pages);
	  packb(buffer);
	}
      else if (p->wtype < 63)
	{
	  ps_header();
	  packb("%%Trailer");
	  packb("%%Pages: (none)");
	}
      update();
      if (p->wtype >= 63)
	dpsclose();
      free(p);
      break;

/* activate workstation */
    case 4:
      p->state = GACTIV;
      break;

/* deactivate workstation */
    case 5:
      p->state = GINACT;
      break;

/* clear workstation */
    case 6:
      if (p->init)
	{
	  if (!p->empty)
	    {
	      if (p->wtype < 63)
		packb("showpage");
	      p->empty = 1;
	    }
	  packb("psl restore end % GLI_GKS_dict");
	  end_page(p->pages);
	  p->init = 0;
	}
      break;

/* update workstation */
    case 8:
      update();
      break;

/* polyline */
    case 12:
      if (p->state == GACTIV)
	{
	  if (!p->init)
	    {
	      ps_init(&p->pages);
	      p->init = 1;
	    }
	  tnr = gksl->cntnr;
	  GSDT(p->window, p->viewpt);
	  ltype = gksl->asf[0] ? gksl->ltype : gksl->lindex;
	  if (ltype != GLSOLI)
	    set_linetype(ltype);
	  width = gksl->asf[1] ? gksl->lwidth : 1;
	  set_linewidth(width);
	  color = gksl->asf[2] ? gksl->plcoli : 1;
	  set_color(color, p->wtype);
	  line_routine(ia, r1, r2, &ltype, &tnr);
	  if (ltype != GLSOLI)
	    set_linetype(GLSOLI);
	  p->empty = 0;
	}
      break;

/* polymarker */
    case 13:
      if (p->state == GACTIV)
	{
	  if (!p->init)
	    {
	      ps_init(&p->pages);
	      p->init = 1;
	    }
	  GSDT(p->window, p->viewpt);
	  size = gksl->asf[4] ? gksl->mszsc : 1;
	  set_markersize(size);
	  x = 0.0;
	  y = 1.0;
	  seg_xform_rel(&x, &y);
	  angle = -atan2(x, y) * 180.0 / M_PI;
	  set_markerangle(angle);
	  factor = size * 1.5;
	  set_linewidth(factor);
	  color = gksl->asf[5] ? gksl->pmcoli : 1;
	  set_foreground(color, p->wtype);
	  GSIMPM(ia, r1, r2, marker_routine);
	  p->empty = 0;
	}
      break;

/* text */
    case 14:
      if (p->state == GACTIV)
	{
	  if (!p->init)
	    {
	      ps_init(&p->pages);
	      p->init = 1;
	    }
	  tnr = gksl->cntnr;
	  GSDT(p->window, p->viewpt);
	  font = gksl->asf[6] ? gksl->txfont : predef_font[gksl->tindex - 1];
	  prec = gksl->asf[6] ? gksl->txprec : predef_prec[gksl->tindex - 1];
	  if (prec != GSTRKP)
	    set_font(font, p->height);
	  else
	    set_linewidth(1.0);
	  color = gksl->asf[9] ? gksl->txcoli : 1;
	  set_color(color, p->wtype);
#ifdef VMS
	  nchars = chars->dsc$w_length;
#else
#ifdef cray
	  nchars = _fcdlen(chars);
#else
	  nchars = strlen(chars);
#endif /* cray */
#endif /* VMS */
	  if (prec == GSTRP)
	    {
	      float px, py;
	      WC_to_NDC(*r1, *r2, tnr, px, py);
	      seg_xform(&px, &py);
	      text_routine(&px, &py, &nchars, chars);
	    }
	  else
	    {
	      unsigned short chars_len = nchars;
#if defined(hpux) && !defined(NAGware)
	      GSIMTX(r1, r2, &nchars, chars,
#if defined(__hp9000s700) || defined(__hp9000s300)
	        line_routine_a, fill_routine_a, chars_len);
#else
	        &line_routine_a, &fill_routine_a, chars_len);
#endif /* __hp9000s700 || __hp9000s300 */
#else
#if defined(_WIN32) && !defined(__GNUC__)
	      GSIMTX(r1, r2, &nchars, chars, chars_len,
	        line_routine, fill_routine);
#else
	      GSIMTX(r1, r2, &nchars, chars,
	        line_routine, fill_routine, chars_len);
#endif
#endif
	    }
	  p->empty = 0;
	}
      break;

/* fill area */
    case 15:
      if (p->state == GACTIV)
	{
	  if (!p->init)
	    {
	      ps_init(&p->pages);
	      p->init = 1;
	    }
	  tnr = gksl->cntnr;
	  GSDT(p->window, p->viewpt);
	  style = gksl->asf[10] ? gksl->ints   : predef_ints[gksl->findex - 1];
	  color = gksl->asf[12] ? gksl->facoli : 1;
	  set_color(color, p->wtype);
	  set_linewidth(1.0);
	  if (style == GSOLID)
	    fill_routine(ia, r1, r2, &tnr);
	  else if (style == GPATTR)
	    {
	      pattern = gksl->asf[11] ? gksl->styli :
		predef_styli[gksl->findex - 1];
	      fillpattern_routine(ia, r1, r2, &tnr, &pattern);
	    }
	  else
	    {
	      yres = 1.0 / 4650.0;
	      GFILLA(ia, r1, r2, &tnr, line_routine, &yres);
	    }
	  p->empty = 0;
	}
      break;

/* cell array */
    case 16:
      if (p->state == GACTIV)
	{
	  if (!p->init)
	    {
	      ps_init(&p->pages);
	      p->init = 1;
	    }
	  GSDT(p->window, p->viewpt);
	  cell_array(r1[0], r1[1], r2[0], r2[1], *dx, *dy, *dimx, ia, p->wtype);
	  p->empty = 0;
	}
      break;

/* set color representation */
    case 48:
      set_color_rep(ia[1], r1[0], r1[1], r1[2]);
      break;

    case 49:
/* set window */
      set_norm_xform(*ia, gksl->window[*ia], gksl->viewport[*ia]);
      break;

    case 50:
/* set viewport */
      set_norm_xform(*ia, gksl->window[*ia], gksl->viewport[*ia]);
      break;

/* set workstation window */
    case 54:
      p->window[0] = r1[0];
      p->window[1] = r1[1];
      p->window[2] = r2[0];
      p->window[3] = r2[1];
      set_xform(p->window, p->viewpt, &p->height, p->wtype);
      init_norm_xform();
      if (p->init)
	set_clipping(p->window);
      break;

/* set workstation viewport */
    case 55:
      p->viewpt[0] = r1[0];
      p->viewpt[1] = r1[1];
      p->viewpt[2] = r2[0];
      p->viewpt[3] = r2[1];
      set_xform(p->window, p->viewpt, &p->height, p->wtype);
      init_norm_xform();
      break;

    default:
      ;
    }
}
