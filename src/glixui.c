/*
 *
 * FACILITY:
 *
 *	GLI XUI (X11 User Interface)
 *
 * ABSTRACT:
 *
 *	This module contains some easy-to-use X11 User Interface
 *	routines based MIT's X11 Windowing System (Release 3).
 *
 * AUTHOR:
 *
 *	Josef Heinen
 *
 *  CREATION DATE:
 *
 *      12-MAR-1990
 *
 *  VERSION:
 *
 *      V1.0 (X11 Release 3 or higher)
 *
 *  CHANGE LOG:
 *
 */


#if !defined(MSDOS) && !defined(NO_X11)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>

#include "terminal.h"

#define app_name "gli"
#define menu_font "-adobe-helvetica-bold-o-normal-*-14-*-*-*-*-*-*-*"

#define TRUE 1
#define FALSE 0
#define NIL 0


static int xinit = FALSE;
static Display *dpy = NIL;
static Window win;
static GC gc;
static int fg, bg;
static Screen *screen;
static XFontStruct *font;
static int SunServer = FALSE;



int xui_present (void)

/*
 * xui_present - try to open X display
 */

  {
  if (!xinit)
    dpy = XOpenDisplay (NULL);
  
  return (dpy != 0);
  }



static
void get_color (char *resource, int *pixel, char *fallback)

  {
  char *colname;
  XColor dcolor, color;

  colname = XGetDefault (dpy, app_name, resource);
  if (colname == NULL)
    colname = fallback;

  if (colname)
    if (XLookupColor (dpy, XDefaultColormap (dpy, DefaultScreen(dpy)), colname,
      &dcolor, &color))
      if (XAllocColor (dpy, XDefaultColormap (dpy, DefaultScreen(dpy)), &color))
	*pixel = color.pixel;    
  }


void xui_open (void)

/*
 * xui_open - open the XUI environment
 */

  {
  XSetWindowAttributes xswa;
  XGCValues xgcv;
  Cursor cursor;

  if (!dpy)
    dpy = XOpenDisplay (NULL);
  
  if (!dpy) {
    tt_fprintf (stderr, "XUI: can't open display\n");
    exit (-1);
    }

  screen = XDefaultScreenOfDisplay (dpy);
  fg = XBlackPixel (dpy, XDefaultScreen(dpy));
  bg = XWhitePixel (dpy, XDefaultScreen(dpy));

  if (XDisplayCells (dpy, XDefaultScreen(dpy)) > 2) {
    get_color ("foreground", &fg, "navy");
    get_color ("background", &bg, "wheat");
    }

/*  Create a window whose parent is the root window  */

  xswa.background_pixel = bg;
  xswa.event_mask = StructureNotifyMask | ExposureMask;

  win = XCreateWindow (dpy, XRootWindowOfScreen(screen),
    0, 0, 100, 100, 2, XDefaultDepthOfScreen (screen), InputOutput,
    XDefaultVisualOfScreen (screen), CWBackPixel | CWEventMask, &xswa);

/*  Create graphics context  */

  xgcv.foreground = fg;
  gc = XCreateGC (dpy, win, GCForeground, &xgcv);   

  font = XLoadQueryFont (dpy, menu_font);
  if (!font)
    font = XLoadQueryFont (dpy, "variable");
  if (!font)
    font = XLoadQueryFont (dpy, "fixed");
  if (!font) {
    tt_fprintf (stderr, "XUI: unable to load font\n");
    exit (-1);
    }

  XSetFont (dpy, gc, font->fid);

  cursor = XCreateFontCursor (dpy, XC_hand1);
  XDefineCursor (dpy, win, cursor);

  SunServer = strcmp(ServerVendor(dpy),"X11/NeWS - Sun Microsystems Inc.") == 0;
  
  xinit = TRUE;
  }



void xui_close (void)

/*
 * xui_close - close the XUI environment
 */

  {
  XUnmapWindow (dpy, win);
  XDestroyWindow (dpy, win);
  XCloseDisplay (dpy);

  xinit = FALSE;
  dpy = NIL;
  }



static
char *extract (int number, char *list, char *element)

/*
 * extract - extract an element from a string in which the
 * elements are separated by a blank (or a vertical bar)
 */

  {
  char *str, delimiter;

  if (*list == '|')
    delimiter = *list++;
  else
    delimiter = ' ';

  str = element;
  while (*list && number)
    {
    while (*list && *list != delimiter) list++;
    while (*list == delimiter) list++;    
    number--;
    }

  while (*list && *list != delimiter && *list != '*')
    *str++ = *list++;
  *str = '\0';

  return (element);
  }


    
static
void get_suitable_origin (int *x, int *y, unsigned int width,
    unsigned int height)

/*
 * get_suitable_origin - query current pointer position and calculate a
 * suitable window origin
 */

  {
  Window root_win, child_win;
  int win_x, win_y;
  int screen_width, screen_height;
  unsigned int status;

  XQueryPointer (dpy, XRootWindowOfScreen (screen), &root_win, &child_win,
    x, y, &win_x, &win_y, &status);

  screen_width = XWidthOfScreen (screen);
  screen_height = XHeightOfScreen (screen);

  *x = *x - width/2;
  if (*x < 4) *x = 4;
  if (*x+width+4 > screen_width) *x = screen_width-width-4;

  *y = *y - height/2;
  if (*y < 16) *y = 16;
  if (*y+height+8 > screen_height) *y = screen_height-height-8;
  }



int xui_get_choice (char *prompt, char *choice_list, char *choice, Bool *more)

/*
 * xui_get_choice - get choice input
 */

  {
  unsigned int max_width, width, height;
  int xorg, yorg, descent, text_height;
  XEvent event;
  Window focus;
  int revert;
  int x, y, count;
  int nchoices, current_choice, new_choice;

  if (!xinit)
    xui_open ();

  max_width = XTextWidth (font, prompt, strlen(prompt)) + 75;
  text_height = font->ascent + font->descent + 2;

  nchoices = 0;
  do
    {
    choice = extract (nchoices, choice_list, choice);
    if (*choice) {
      nchoices++;
      width = XTextWidth (font, choice, strlen(choice)) + 50;
      if (width > max_width)
	max_width = width;
      }
    }
  while (*choice);

  if (nchoices > 0) {
    width = max_width;
    height = text_height*nchoices + 4;
    descent = font->descent;
    }

  get_suitable_origin (&xorg, &yorg, width, height);

  XStoreName (dpy, win, prompt);

  XMoveResizeWindow (dpy, win, xorg, yorg, width, height+descent);
  XMapRaised (dpy, win);

  XGetInputFocus (dpy, &focus, &revert);
  XSelectInput (dpy, win, PointerMotionMask | ButtonPressMask | 
    StructureNotifyMask | ExposureMask);
  XSync (dpy, False);

  current_choice = -1;
  do
      {
      XWindowEvent (dpy, win, PointerMotionMask | ButtonPressMask |
	StructureNotifyMask | ExposureMask, &event);

      switch (event.type)
	{
	case MotionNotify :

	  y = event.xmotion.y;
	  new_choice = (int)((float)(y-descent)/height*nchoices);

	  if (new_choice != current_choice) {
	    if (current_choice >= 0) {
	      y = current_choice*text_height;
	      XSetForeground (dpy, gc, bg);
	      XDrawRectangle (dpy, win, gc, 8,y+4, width-16, text_height-1);
	      }
	    if (new_choice >= 0) {
	      y = new_choice*text_height;
	      XSetForeground (dpy, gc, fg);
	      XDrawRectangle (dpy, win, gc, 8,y+4, width-16, text_height-1);
	      }
	    current_choice = new_choice;
	    }
	  
	  break;

	case ButtonPress :

	  *more = event.xbutton.button == Button3;

	  if (event.xbutton.button != Button2) {
	    y = event.xbutton.y;
	    current_choice = (int)((float)(y-descent)/height*nchoices);
	    }
	  else
	    current_choice = -1;

	  break;

	case MapNotify :
	case Expose :

	  XClearWindow (dpy, win);
	  y = 0;
	  for (count = 0; count < nchoices; count++)
	    {
	    choice = extract (count, choice_list, choice);
	    x = width/2 - XTextWidth (font, choice, strlen(choice))/2;
	    y = y + text_height;
	    XSetForeground (dpy, gc, fg);
	    XDrawString (dpy, win, gc, x, y, choice, strlen(choice));
	    }
	  
	  XSetInputFocus (dpy, win, revert, CurrentTime);
	  break;
	}
      }

  while (event.type != ButtonPress);

  if (SunServer)
    XMoveResizeWindow (dpy, win, xorg, yorg, width, 1);
  else
    XUnmapWindow (dpy, win);
  
  XSetInputFocus (dpy, focus, revert, CurrentTime);
  XSync (dpy, False);

  if (current_choice >= 0)
    strcpy (choice, extract (current_choice, choice_list, choice));

  return (current_choice);
  }



static
void draw_text_box (char *text)

  {
  XSetForeground (dpy, gc, fg);
  XDrawRectangle (dpy, win, gc, 25, 25, 150, 25);
  if (*text)
    XDrawString (dpy, win, gc, 30, 45, text, strlen(text));
  }



static
int dispatch_character (XKeyEvent *event, char *text)

  {
  KeySym keysym;
  char str[10];
  static XComposeStatus status;

  XSetForeground (dpy, gc, bg);
  XDrawString (dpy, win, gc, 30, 45, text, strlen(text));

  XLookupString (event, str, 9, &keysym, &status);

  if ((keysym >= XK_space && keysym <= XK_asciitilde) ||
      (keysym >= XK_nobreakspace && keysym <= XK_ydiaeresis)) {
    str[0] = (char) (keysym & 0xff);
    strncat (text, str, 1);
    }

  else if (keysym == XK_Delete)
    if (strlen(text) > 0)
      text[strlen(text)-1] = '\0';

  XSetForeground (dpy, gc, fg);
  XDrawString (dpy, win, gc, 30, 45, text, strlen(text));

  return (keysym);
  }



int xui_get_text (char *prompt, char *control, char *text, Bool *more)

/*
 * xui_get_text - get text input
 */

  {
  unsigned int width, height;
  int xorg, yorg, done;
  XEvent event;
  Window focus;
  int revert;
  char str[255];

  if (!xinit)
    xui_open ();

  width = 200;
  height = 75;
  get_suitable_origin (&xorg, &yorg, width, height);

  XStoreName (dpy, win, prompt);

  XMoveResizeWindow (dpy, win, xorg, yorg, width, height);
  XMapRaised (dpy, win);

  XGetInputFocus (dpy, &focus, &revert);
  XSelectInput (dpy, win, KeyPressMask | ButtonPressMask |
    StructureNotifyMask | ExposureMask);
  XSync (dpy, False);

  strcpy (str, "");
  done = FALSE;

  do
      {
      XWindowEvent (dpy, win, KeyPressMask | ButtonPressMask | 
	StructureNotifyMask | ExposureMask, &event);

      switch (event.type)
	{
	case KeyPress :
	  if (dispatch_character ((XKeyEvent *)&event, str) == XK_Return) {
	    strcpy (text, str);
	    done = TRUE;
	    }
	  break;

	case ButtonPress :
	  *more = event.xbutton.button == Button3;
	  if (event.xbutton.button != Button2) {
	    strcpy (text, str);
	    done = TRUE;
	    }
	  break;

	case MapNotify :
	case Expose :
	  XClearWindow (dpy, win);
	  draw_text_box (str);
	  
	  XSetInputFocus (dpy, win, revert, CurrentTime);
	  break;
	}
      }

  while (!done && event.type != ButtonPress);

  if (SunServer)
    XMoveResizeWindow (dpy, win, xorg, yorg, width, 1);
  else
    XUnmapWindow (dpy, win);
  
  XSetInputFocus (dpy, focus, revert, CurrentTime);
  XSync (dpy, False);
  
  if (done)
    return (strlen (str));
  else
    return (-1);
  }



static
int pvalue (int y)

  {
  y = 130-y;
  if (y > 100)
    return (100);
  else if (y < 0)
    return (0);
  else
    return (y);
  }


static
void draw_scaler (int center)

  {
  XSetForeground (dpy, gc, fg);
  XDrawRectangle (dpy, win, gc, center-16, 25, 32, 110);

  XDrawString (dpy, win, gc, center+20, 35, "100%", 4);
  XDrawString (dpy, win, gc, center+20, 135, "0%", 2);
  }


static
int draw_bar (int center, int current_value, int new_value)

  {
  int x, y;
  char value[6];

  if (current_value >= 0) {
    y = 130-current_value;
    XSetForeground (dpy, gc, bg);
    XDrawRectangle (dpy, win, gc, center-15, y-4, 30, 8);
    }
  if (new_value >= 0) {
    y = 130-new_value;
    XSetForeground (dpy, gc, fg);
    XDrawRectangle (dpy, win, gc, center-15, y-4, 30, 8);
    }
  if (current_value >= 0) {
    sprintf (value, " %3d%c ", current_value, '%');
    x = center - XTextWidth (font, value, strlen(value))/2;
    XSetBackground (dpy, gc, bg);
    XSetForeground (dpy, gc, fg);
    XDrawImageString (dpy, win, gc, x, 16, value, strlen(value));
    }

  return (new_value);
  }


int xui_get_value (char *prompt, int *value)

/*
 * xui_get_value - get valuator input
 */

  {
  unsigned int width, height;
  int xorg, yorg;
  XEvent event;
  Window focus;
  int revert;
  int x, y;
  int current_value;

  if (!xinit)
    xui_open ();

  width = XTextWidth (font, prompt, strlen(prompt)) + 75;
  if (width < 125) width = 125;
  x = width/2;
  height = 150;
  get_suitable_origin (&xorg, &yorg, width, height);

  XStoreName (dpy, win, prompt);

  XMoveResizeWindow (dpy, win, xorg, yorg, width, height);
  XMapRaised (dpy, win);

  XGetInputFocus (dpy, &focus, &revert);
  XSelectInput (dpy, win, PointerMotionMask | ButtonPressMask |
    StructureNotifyMask | ExposureMask);
  XSync (dpy, False);

  current_value = -1;
  do
      {
      XWindowEvent (dpy, win, PointerMotionMask | ButtonPressMask |
	StructureNotifyMask | ExposureMask, &event);

      switch (event.type)
	{
	case MotionNotify :
	  y = event.xmotion.y;
	  current_value = draw_bar (x, current_value, pvalue (y));
	  break;

	case ButtonPress :
	  if (event.xbutton.button != Button2) {
	    y = event.xbutton.y;
	    current_value = pvalue (y);
	    }
	  else
	    current_value = -1;
	  break;

	case MapNotify :
	case Expose :
	  XClearWindow (dpy, win);
	  draw_scaler (x);
	  
	  XSetInputFocus (dpy, win, revert, CurrentTime);
	  break;
	}
      }

  while (event.type != ButtonPress);

  if (SunServer)
    XMoveResizeWindow (dpy, win, xorg, yorg, width, 1);
  else
    XUnmapWindow (dpy, win);
  
  XSetInputFocus (dpy, focus, revert, CurrentTime);
  XSync (dpy, False);
  
  if (current_value >= 0)
    *value = current_value;

  return (current_value);
  }

#else

xui_present() {}
xui_open() {}
xui_close() {}
xui_get_choice() {}
xui_get_text() {}
xui_get_value() {}

#endif
