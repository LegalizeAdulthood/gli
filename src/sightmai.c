#ifdef MOTIF

/*
 *
 * FACILITY:
 *
 *	Graphics Language Interpreter (GLI)
 *
 * ABSTRACT:
 *
 *	This module contains the main procedure body for the Simple
 *	Interactive Graphics Handling Tool (Sight).
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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#if !defined(VMS) && !defined(MSDOS) && !defined(_WIN32)
#include <unistd.h>
#endif

#include <sys/types.h>

#include <X11/keysym.h>
#include <X11/cursorfont.h>

#include <Mrm/MrmAppl.h>
#include <Xm/Command.h>

#include "image.h"
#include "sight.h"
#include "terminal.h"
#include "system.h"

void gli_do_command(char *);

#define min(x1,x2)      (((x1)<(x2)) ? (x1) : (x2))
#define max(x1,x2)      (((x1)>(x2)) ? (x1) : (x2))
#define sqr(x)		((double)(x)*(x))

#define Width		686
#define Height		485

#define MAX_STRING	256
#define MAX_POINTS	512

#ifndef PI
#define PI              3.14159265358979323846
#endif

/*
 * These numbers are matched with corresponding numbers in the Sight
 * UIL module.
 */

#define k_drawing		1
#define k_cgm			2
#define k_data			3
#define k_quit			4
#define k_exit			5
#define k_tool_box              6
#define k_attribute_box         7
#define k_digitize_area		8
#define k_digitize_points	9
#define k_digitize_vbars	10
#define k_digitize_hbars	11
#define k_enter			12
#define k_help			13
#define k_warning		14
#define k_drawing_area		15
#define k_image			16

#define k_max_widget		16

#define MAX_WIDGETS (k_max_widget + 1)


/*
 * Global data
 */

static XtAppContext app_context;	/* Application context */
static Display *display;		/* Display variable */
static Window main_window,              /* Main window ID */
  window;			        /* Command window ID */

static Widget toplevel_widget = NULL,   /* Root widget ID of our */
                                        /* application. */
  main_window_widget,                   /* Root widget ID of main */
                                        /* Mrm fetch */
  graphic_window_widget,		/* Graphic window widget ID */
  command_window_widget,		/* Command window widget ID */
  widget_array[MAX_WIDGETS];            /* Place to keep all other */
                                        /* widget IDs */

static MrmHierarchy s_MrmHierarchy;     /* Mrm database hierarchy ID */
static MrmType dummy_class;             /* and class variable. */

static String resources[] = {
"*Sight*fontList: -*-helvetica-bold-o-*-*-14-*-*-*-*-*-*-*",
"*Sight*XmTextField.*fontList: -*-helvetica-bold-r-*-*-14-*-*-*-*-*-*-*",
"*Sight*XmList*fontList: -*-helvetica-bold-r-*-*-14-*-*-*-*-*-*-*",
"*Sight*background: wheat",
"*Sight*foreground: navy",
NULL
};

static Dimension main_width, main_height, border_width;
static Dimension graphic_width = Width, graphic_height = Height;
static unsigned int watch_cursor;

extern int sight_save_needed;
extern int sight_dpi;
extern int sight_gui_open;

#ifdef VMS
static char *default_path = "sys$sysdevice:[gli]";
#else
#ifdef _WIN32
static char *default_path = "c:\\gli";
#else
static char *default_path = "/usr/local/gli";
#endif
#endif
static char *default_lang = "ae_AE";

static String db_filename_vec[2];
static int db_filename_num = 2;

static char command[MAX_STRING + 1];
static Boolean done;

static Boolean area_ok = False;
static double xorg, xscale, yorg, yscale;
static double sin_f, cos_f;

static FILE *tablet = NULL;


static
void init_application (void)
{
    int k;
    Arg args[3];

    /* Initialize the application data structures. */
    for (k = 0; k < MAX_WIDGETS; k++)
      widget_array[k] = NULL;

    k = 0;
    XtSetArg (args[k], XmNwidth, (XtArgVal) &main_width); k++;
    XtSetArg (args[k], XmNheight, (XtArgVal) &main_height); k++;
    XtSetArg (args[k], XmNborderWidth, (XtArgVal) &border_width); k++;
    XtGetValues (main_window_widget, args, k);

    k = 0;
    XtSetArg (args[k], XmNwidth, (XtArgVal) &graphic_width); k++;
    XtSetArg (args[k], XmNheight, (XtArgVal) &graphic_height); k++;
    XtGetValues (graphic_window_widget, args, k);
}


/*
 * All errors are fatal.
 */

static
void s_error (char *problem_string)
{
    tt_fprintf (stderr, "Sight: %s\n", problem_string);
    exit (0);
}


static
void fetch_widget (int widget_num, char * widget_id)
{
    if (widget_array[widget_num] == NULL)
      if (MrmFetchWidget (s_MrmHierarchy, widget_id, toplevel_widget,
	&widget_array[widget_num], &dummy_class) != MrmSUCCESS)
	  s_error ("can't fetch widget");
}


static
void bring_up (int widget_num)
{
    if (widget_array[widget_num] != NULL)
      /* Put up the widget */
      XtManageChild (widget_array[widget_num]);
}


static
void bring_down (int widget_num)
{
    if (widget_array[widget_num] != NULL)
      /* Bring down the widget. */
      XtUnmanageChild (widget_array[widget_num]);
}


static
void activate_widget (
    Widget widget, int *tag, XmAnyCallbackStruct *callback_data)
{
    int widget_num = *tag;

    switch (widget_num)
      {
      case k_drawing:	fetch_widget (k_drawing, "drawing_selection_box");
			break;
      case k_cgm:	fetch_widget (k_cgm, "cgm_selection_box"); break;
      case k_data:	fetch_widget (k_data, "data_selection_box"); break;
      case k_image:	fetch_widget (k_image, "image_selection_box"); break;
      case k_quit:	if (sight_save_needed)
                            fetch_widget (k_quit, "quit_warning_box");
                        else
                            done = True;
                        break;
      case k_tool_box:	fetch_widget (k_tool_box, "tool_box"); break;
      case k_attribute_box:
                	fetch_widget (k_attribute_box, "attribute_box"); break;
      case k_help:	break;
      }

    bring_up (widget_num);
}


static
void dismiss_widget (
    Widget widget, int *tag, XmAnyCallbackStruct *callback_data)
{
    int widget_num = *tag;

    bring_down (widget_num);
}


static
void send_button (unsigned int button, int state)
{
    int status;
    XEvent event;

    event.xbutton.type		= ButtonPress;
    event.xbutton.display	= display;
    event.xbutton.window	= window;
    event.xbutton.root		= RootWindow (display, DefaultScreen(display));
    event.xbutton.button	= button;
    event.xbutton.state		= state;
    event.xbutton.time		= CurrentTime;
    event.xbutton.same_screen	= True;
    event.xbutton.x		= 0;
    event.xbutton.y		= 0;
    event.xbutton.x_root	= 0;
    event.xbutton.y_root	= 0;
    event.xbutton.subwindow	= (Window) None;

    status = XSendEvent (display, window, False, ButtonPressMask, &event);
    if (status != 0) {
	event.xbutton.type	= ButtonRelease;
	event.xbutton.time	= CurrentTime;

	status = XSendEvent (display, window, False, ButtonReleaseMask, &event);
	}
}


static
void send_keycode (KeyCode keycode, int state)
{
    int status;
    XEvent event;

    event.xkey.type		= KeyPress;
    event.xkey.display		= display;
    event.xkey.window		= window;
    event.xkey.root		= RootWindow (display, DefaultScreen(display));
    event.xkey.keycode		= keycode;
    event.xkey.state		= state;
    event.xkey.time		= CurrentTime;
    event.xkey.same_screen	= True;
    event.xkey.x		= 0;
    event.xkey.y		= 0;
    event.xkey.x_root		= 0;
    event.xkey.y_root		= 0;
    event.xkey.subwindow	= (Window) None;

    status = XSendEvent (display, window, False, KeyPressMask, &event);
    if (status != 0) {
	event.xkey.type		= KeyRelease;
	event.xkey.time		= CurrentTime;

	status = XSendEvent (display, window, False, KeyReleaseMask, &event);
	}
}


static
void do_command (void)
{
    XmString str;

    send_button (Button1, 0x00);

    str = XmStringCreateLtoR (command, XmSTRING_DEFAULT_CHARSET);
    XmCommandSetValue (command_window_widget, str);
    XmStringFree (str);

    send_keycode (XKeysymToKeycode (display, XK_Return), 0x00);
}


static
void set_command (Widget widget, char *tag, XmAnyCallbackStruct *callback_data)
{
    char *more;

    if (*tag == '\r') {
	strcpy (command, "");
	tag++;
	}
    else if (*tag && *tag != ' ')
        strcpy (command, "sight ");

    if (more = strchr (tag, '\\'))
        strncat (command, tag, more-tag);
    else {
        strcat (command, tag);
	more = strchr (command, '%');
	}

    if (!more)
        do_command ();
}


static
char *extract_normal_string (XmString cs)
{
    XmStringContext context;
    XmStringCharSet charset;
    XmStringDirection direction;
    Boolean separator;
    static char *primitive_string;

    XmStringInitContext (&context, cs);
    XmStringGetNextSegment (context, &primitive_string, &charset, &direction,
	&separator);
    XmStringFreeContext (context);

    return (primitive_string);
}


static
void file_selection_box_cb (XmFileSelectionBoxCallbackStruct *callback_data)
{
    char *filename, str[MAX_STRING];

    filename = extract_normal_string (callback_data->value);

    sprintf (str, command, filename);
    strcpy (command, str);

    do_command ();
}


static
void command_entered_cb (XmCommandCallbackStruct *callback_data)
{
    XmString str;
    char information[MAX_STRING];

    XDefineCursor (display, main_window, watch_cursor);
    XFlush (display);

    strcpy (command, extract_normal_string (callback_data->value));
    gli_do_command (command);
    strcpy (command, "");

    *information = '?';
    SightInquireObject (information);

    str = XmStringCreateLtoR (information, XmSTRING_DEFAULT_CHARSET);
    XmCommandError (command_window_widget, str);
    XmStringFree (str);

    XUndefineCursor (display, main_window);
    XFlush (display);
}


static
int get_time (void)
{
    time_t time_val;
    struct tm *date;
    int timestamp;

    time (&time_val);
    date = localtime (&time_val);
    timestamp = date->tm_sec + 60 * (date->tm_min + 60 * (date->tm_hour + 24 *
        (date->tm_mday + 31 * (date->tm_mon + 12 * (date->tm_year - 96)))));

    return (timestamp);
}


static
int get_tablet_pointer (int *x, int *y)
{
    char buf[BUFSIZ];
    int current_time, timestamp, button, status;
    
    current_time = get_time ();
    do {
        do {
            if (read (fileno(tablet), buf, BUFSIZ) <= 0)
                s_error ("can't read from tablet port");
        }
        while (sscanf (buf, "%d %d %d %d", &button, x, y, &timestamp) != 4);
    }
    while (timestamp < current_time);

    if (button == Button1)
        status = True;
    else
        status = False;

    return (status);
}


static 
int get_mouse_pointer (int *x, int *y)
{
    Screen *screen;
    XEvent event;
    int status;

    screen = XDefaultScreenOfDisplay(display);

    XAllowEvents (display, SyncPointer, CurrentTime);
    XWindowEvent (display, RootWindow (display, DefaultScreen(display)),
        ButtonPressMask, &event);

    *x = event.xbutton.x * XHeightOfScreen(screen)/XWidthOfScreen(screen);
    *y = event.xbutton.y;

    if (event.xbutton.button == Button1)
        status = True;
    else
        status = False;

    return (status);
}


static
int get_pointer (unsigned int which_cursor, int *x, int *y)
{
    Cursor cursor;
    int status;

    cursor = XCreateFontCursor (display, which_cursor);

    status = XGrabPointer (display, 
        RootWindow (display, DefaultScreen(display)), False, ButtonPressMask,
        GrabModeSync, GrabModeAsync, None, cursor, CurrentTime);

    if (status == GrabSuccess) {
        if (tablet)
            status = get_tablet_pointer (x, y);
        else
            status = get_mouse_pointer (x, y);

        XUngrabPointer (display, CurrentTime);
        XSync (display, False);
        }
    else
        status = False;

    XFreeCursor (display, cursor);

    return (status);
}


static
void digitize_area (void)
{
    int xtl, ytl, xbl, ybl, xbr, ybr;
    double alpha, angle;

    area_ok = False;
    if (!get_pointer (XC_top_left_corner, &xtl, &ytl)) return;
    if (!get_pointer (XC_bottom_left_corner, &xbl, &ybl)) return;
    if (!get_pointer (XC_bottom_right_corner, &xbr, &ybr)) return;

    if ((xbr == xbl && ybr == ybl) ||
	(xtl == xbl && ytl == ybl)) {
	XBell (display, 0);
	return;
	};

    alpha = atan2((double)(xtl-xbl),(double)(ybl-ytl));
    sin_f = sin(alpha);
    cos_f = cos(alpha);

    xorg = xbl; xscale = 1.0/sqrt(sqr(xbr-xbl)+sqr(ybr-ybl));
    yorg = ybl; yscale = 1.0/sqrt(sqr(xtl-xbl)+sqr(ytl-ybl));

    area_ok = True;

    angle = (atan2((double)(ybl-ytl),(double)(xtl-xbl)) +
        atan2((double)(ybr-ybl),(double)(xbr-xbl))) * 180/PI;

    if (fabs(angle-90) >= 0.1) {
        tt_fprintf (stderr, "Sight: digitize area is not right-angled (%g)\n",
            angle);
	XBell (display, 0);
        }
}


static
void apply_inverse_xform (int x, int y, float *resx, float *resy)
{
    double xn, yn;

    xn = x-xorg;
    yn = yorg-y;

    *resx = xscale*(cos_f*xn-sin_f*yn);
    *resy = yscale*(sin_f*xn+cos_f*yn);
}


static
void digitize_points (void)
{
    int n, x, y;
    float xw[MAX_POINTS], yw[MAX_POINTS];

    if (!area_ok)
	digitize_area ();

    n = 0;
    while (get_pointer (XC_dotbox, &x, &y) && (n < MAX_POINTS)) {
	apply_inverse_xform (x, y, &xw[n], &yw[n]);
	n++;
  	}

    if (n > 0)
	SightDefineXY (n, xw, yw);
}


static
void digitize_vbars (void)
{
    int n, x, y, xe;
    float xw[MAX_POINTS], yw[MAX_POINTS], e1[MAX_POINTS], e2[MAX_POINTS];

    if (!area_ok)
	digitize_area ();

    n = 0;
    while (get_pointer (XC_dotbox, &x, &y) && (n < MAX_POINTS))
	{
	apply_inverse_xform (x, y, &xw[n], &yw[n]);

	get_pointer (XC_bottom_tee, &xe, &y);
	apply_inverse_xform (x, y, &xw[n], &e1[n]);

	get_pointer (XC_top_tee, &xe, &y);
	apply_inverse_xform (x, y, &xw[n], &e2[n]);
	n++;
  	}

    if (n > 0) {
	SightDefineXY (n, xw, yw);
	SightDefineY ("E1", n, e1);
	SightDefineY ("E2", n, e2);
	}
}


static
void digitize_hbars (void)
{
    int n, x, y, ye;
    float xw[MAX_POINTS], yw[MAX_POINTS], e1[MAX_POINTS], e2[MAX_POINTS];

    if (!area_ok)
	digitize_area ();

    n = 0;
    while (get_pointer (XC_dotbox, &x, &y) && (n < MAX_POINTS))
	{
	apply_inverse_xform (x, y, &xw[n], &yw[n]);

	get_pointer (XC_left_tee, &x, &ye);
	apply_inverse_xform (x, y, &e1[n], &yw[n]);

	get_pointer (XC_right_tee, &x, &ye);
	apply_inverse_xform (x, y, &e2[n], &yw[n]);
	n++;
  	}

    if (n > 0) {
	SightDefineXY (n, xw, yw);
	SightDefineX ("E1", n, e1);
	SightDefineX ("E2", n, e2);
	}
}


static
void set_something (Widget widget, char *resource, XmString value)
{
    Arg a1[1];

    XtSetArg (a1[0], resource, value);
    XtSetValues (widget, a1, 1);
}


static void process_button_event (
    Widget widget, XtPointer closure, XButtonEvent *event,
    Boolean *continue_to_dispatch)
{
    float x, y, scale;

    if (graphic_width > graphic_height)
	scale = graphic_width;
    else
	scale = graphic_height;

    x = (float)(event->x)/scale;
    y = (float)(graphic_height-event->y)/scale;

    switch (event->button)
      {
      case Button1: sprintf (command, "sight select object %.3f %.3f", x, y);
	break;
      case Button2: sprintf (command, "sight cut %.3f %.3f", x, y); break;
      case Button3: sprintf (command, "sight paste %.3f %.3f", x, y); break;
      }

    do_command ();
}


static
void drawing_area_cb (Widget widget)
{
    graphic_window_widget = widget;
}


static
void do_something (Widget widget, int *tag, XmAnyCallbackStruct *callback_data)
{
    int widget_num = *tag;

    switch (widget_num)
      {
      case k_drawing:
      case k_cgm:
      case k_data:
      case k_image:
	file_selection_box_cb (
	  (XmFileSelectionBoxCallbackStruct *)callback_data);
	break;
      case k_exit:
	gli_do_command ("sight close_drawing");
      case k_quit:
	done = True; break;
      case k_digitize_area:
	digitize_area ();
	break;
      case k_digitize_points:
	digitize_points ();
	break;
      case k_digitize_vbars:
	digitize_vbars ();
	break;
      case k_digitize_hbars:
	digitize_hbars ();
	break;
      case k_enter:
	command_entered_cb ((XmCommandCallbackStruct *)callback_data);
	break;
      case k_drawing_area:
	drawing_area_cb (widget);
	break;
      }

    bring_down (widget_num);
}


void SightConfigure (int width, int height)
{
    width = (int)(width * sight_dpi / 75.0);
    height = (int)(height * sight_dpi / 75.0);

    if (toplevel_widget != NULL)
    {
	main_width -= graphic_width - width;
	main_height -= graphic_height - height;

	XtResizeWidget (toplevel_widget, main_width, main_height, border_width);
    }

    graphic_width = width;
    graphic_height = height;
}	


static
void configure_cb (
    Widget widget, XtPointer closure, XConfigureEvent *event,
    Boolean *continue_to_dispatch)
{
    graphic_width += event->width - main_width;
    graphic_height += event->height - main_height;

    SightResize (graphic_width, graphic_height);
    SightRedraw (NULL);

    main_width = event->width;
    main_height = event->height;
}	


static
void window_setup (void)
{
    Widget command_widget;

    command_window_widget = XtNameToWidget (main_window_widget,
	"command_window_widget");
    command_widget = (Widget) XmCommandGetChild (command_window_widget,
	XmDIALOG_COMMAND_TEXT);

    main_window = XtWindow (main_window_widget);
    window = XtWindow (command_widget);
}


static
void add_event_handlers (void)
{
    XtAddEventHandler (main_window_widget, StructureNotifyMask, False,
	(XtEventHandler) configure_cb, NULL);
    XtAddEventHandler (graphic_window_widget, ButtonPressMask, False,
	(XtEventHandler) process_button_event, NULL);
}


static
void remove_event_handlers (void)
{
    XtRemoveEventHandler (graphic_window_widget, ButtonPressMask, False,
	(XtEventHandler) process_button_event, NULL);
    XtRemoveEventHandler (main_window_widget, StructureNotifyMask, False,
	(XtEventHandler) configure_cb, NULL);
}


/* The names and addresses of things that Mrm has to bind. The names do
 * not have to be in alphabetical order. */

static MrmRegisterArg reglist[] = {
    {"activate_widget", (caddr_t) activate_widget},
    {"dismiss_widget", (caddr_t) dismiss_widget},
    {"do_something", (caddr_t) do_something},
    {"set_command", (caddr_t) set_command}
};

static int reglist_num = (sizeof reglist / sizeof reglist [0]);


static
void error_handler (char *line)
{
    XmString str;

    fetch_widget (k_warning, "warning_box");

    str = XmStringCreateLtoR (line, XmSTRING_DEFAULT_CHARSET);
    set_something (widget_array[k_warning], XmNmessageString, str);
    XmStringFree (str);

    bring_up (k_warning);
}


static
void synchronize (void)
{
    XEvent event;

    while (XtAppPending (app_context)) {
	XtAppNextEvent (app_context, &event);
	XtDispatchEvent (&event);
        }
}


/*
 * Sight main loop
 */

void SightMainLoop (void)
{
    Arg arglist[2];
    XEvent event;
    int n, argc = 0;
    char *name, *path, *lang, lang_prefix[MAX_STRING];
    static Bool app = False;

#ifdef ultrix
    if (getenv ("GLI_TABLET_PORT")) {
        if ((tablet = popen ("tablet", "r")) == NULL)
            s_error ("can't open pipe");
        }
    else
        tablet = NULL;
#endif

    if (!app) {
        MrmInitialize ();               /* Initialize Mrm before initializing
                                         * the X Toolkit. */
        XtToolkitInitialize ();
        app_context = XtCreateApplicationContext ();

	/* Establish a default set of resource values */

        XtAppSetFallbackResources (app_context, resources);

        name = (char *) getenv ("GLI_CONID");
        if (!name) name = (char *) getenv ("DISPLAY");

        if ((display = XtOpenDisplay (app_context, name, "Sight", "sight",
	    NULL, 0, &argc, NULL)) == NULL)
	    s_error ("can't open display");

        n = 0;
        toplevel_widget = XtAppCreateShell ("Sight", NULL,
	    applicationShellWidgetClass, display, arglist, n);

        /* Open the UID files (the output of the UIL compiler) */

        path = (char *) getenv ("GLI_HOME");
        if (!path)
	    path = default_path;

        lang = (char *) getenv ("GLI_LANG");
        if (!lang)
	    lang = (char *) getenv ("LANG");
        if (!lang)
	    lang = default_lang;
        if (strcmp(lang, "ae_AE") && strcmp(lang, "de_DE"))
            lang = default_lang;

	strcpy (lang_prefix, lang);
	strtok (lang_prefix, "_");

        db_filename_vec[0] = (String) malloc (MAX_STRING);
        db_filename_vec[1] = (String) malloc (MAX_STRING);
#ifdef VMS
	sprintf (db_filename_vec[0], "%s%sliter.uid", path, lang_prefix);
        sprintf (db_filename_vec[1], "%ssight.uid", path);
#else
#ifdef _WIN32
        sprintf (db_filename_vec[0], "%s\\%sliter.uid", path, lang_prefix);
        sprintf (db_filename_vec[1], "%s\\sight.uid", path);
#else
        sprintf (db_filename_vec[0], "%s/%sliter.uid", path, lang_prefix);
        sprintf (db_filename_vec[1], "%s/sight.uid", path);
#endif
#endif

#ifdef VMS
        if (MrmOpenHierarchy (
#else
        if (MrmOpenHierarchyPerDisplay (
	  display,                             /* Connection to the X server */
#endif
	  db_filename_num,                     /* Number of files. */
          db_filename_vec,                     /* Array of file names. */
          NULL,                                /* Default OS extension. */
          &s_MrmHierarchy)                     /* Pointer to returned Mrm ID */
          !=MrmSUCCESS)
            s_error ("can't open hierarchy");

        /* Register the items Mrm needs to bind for us. */

        MrmRegisterNames (reglist, reglist_num);

        /* Go get the main part of the application. */

        if (MrmFetchWidget (s_MrmHierarchy, "main_window_widget",
          toplevel_widget, &main_window_widget, &dummy_class) != MrmSUCCESS)
            s_error ("can't fetch main window");

        /* Manage the main part and realize everything. The interface comes up
         * on the display now. */

        XtManageChild (main_window_widget);

        app = True;
        }

    XtRealizeWidget (toplevel_widget);
    synchronize ();

    init_application ();
    window_setup ();

    watch_cursor = XCreateFontCursor (display, XC_watch);

    SightOpen ((int *)graphic_window_widget);
    SightConfigure (Width, Height);
    SightResize (graphic_width, graphic_height);
    SightRedraw (NULL);
    synchronize ();

    sight_gui_open = 1;

    add_event_handlers ();
    establish ((int (*)())error_handler);

    /* Sight main loop */

    done = False;
    while (!done) {
	XtAppNextEvent (app_context, &event);
	XtDispatchEvent (&event);
	}

    establish (NULL);
    remove_event_handlers ();

    sight_gui_open = 0;

    SightClose ((int *)graphic_window_widget);

    XFreeCursor (display, watch_cursor);

    XtUnrealizeWidget (toplevel_widget);
    synchronize ();

#ifdef ultrix
    if (tablet)
        pclose (tablet);
#endif
}

#else

#include <stdio.h>

#include "terminal.h"

void SightConfigure (int width, int height)
{
}	

void SightMainLoop (void)
{
    tt_fprintf (stderr, "Sight: can't access OSF/Motif libraries\n");
}

#endif /* MOTIF */
