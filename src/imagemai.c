/*
 *
 * FACILITY:
 *
 *	Graphics Language Interpreter (GLI)
 *
 * ABSTRACT:
 *
 *	This module contains the main procedure body for the Image Tool.
 *
 * AUTHOR:
 *
 *	Josef Heinen, Jochen Werner
 *
 * VERSION:
 *
 *	V2.0
 *
 */


#include <stdio.h>

#ifdef MOTIF
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <X11/keysym.h>
#include <X11/cursorfont.h>
#include <Xm/Text.h>
#include <Xm/Command.h>
#include <Mrm/MrmPublic.h>

#include "system.h"
#include "terminal.h"
#include "image.h"

void gli_do_command(char *);

#define Width		512
#define Height		512
#define BellVolume	0

#define MAX_STRING	256

#define MAX_IMAGES      100

#define NIL             0


/*
 * These numbers are matched with corresponding numbers in the Image
 * UIL module.
 */

#define k_read_image            1
#define k_write_image           2
#define k_quit                  3
#define k_help                  4
#define k_enter                 5
#define k_drawing_area          6
#define k_warning               7
#define k_read_image_name       8
#define k_image_list            9
#define k_image_selected        10
#define k_read_parameter        11
#define k_set_names             12
#define k_scale_box             13
#define k_scale_width           14
#define k_scale_height          15
#define k_import_box            16
#define k_import_image          17
#define k_import_variable       18
#define k_import_width          19

#define k_max_widget		19
#define MAX_WIDGETS (k_max_widget + 1)

typedef char string[MAX_STRING];

/*
 * Global data
 */

static XtAppContext app_context;	/* Application context */
static Display *display;		/* Display variable */
static Window 
    main_window,
    image_list_window,
    window,			        /* Command window ID */
    graphic_window;			/* Graphic window ID */

static GC gc, clear;
static XImage *ximage = NULL;
static XFontStruct *font;

static unsigned int
    graphic_width = Width, graphic_height = Height;

static Widget 
    toplevel_widget,                    /* Root widget ID of our application. */
    main_window_widget,                 /* Root widget ID of main Mrm fetch */
    graphic_window_widget,		/* Graphic window widget ID */
    command_window_widget,		/* Command window widget ID */
    list_box_widget,
    image_list_widget,
    scale_box_widget,
    scale_width_widget,
    scale_height_widget,
    import_box_widget,
    import_image_widget,
    import_variable_widget,
    import_width_widget,
    widget_array[MAX_WIDGETS];          /* Place to keep all other */
                                        /* widget IDs */

static MrmHierarchy s_MrmHierarchy;     /* Mrm database hierarchy ID */
static MrmType dummy_class;             /* and class variable. */

static String resources[] = {
"*Image*fontList: -*-helvetica-bold-o-*-*-14-*-*-*-*-*-*-*",
"*Image*XmTextField.*fontList: -*-helvetica-bold-r-*-*-14-*-*-*-*-*-*-*",
"*Image*XmList*fontList: -*-helvetica-bold-r-*-*-14-*-*-*-*-*-*-*",
"*Image*background: wheat",
"*Image*foreground: navy",
NULL
};

static unsigned int watch_cursor, cross_cursor;

#ifdef VMS
static char *default_path = "sys$sysdevice:[gli]";
#else
#ifdef _WIN32
static char *default_path = "c:\\gli";
#else
static char *default_path = "/usr/local/gli";
#endif
#endif

static String db_filename_vec[1];
static int db_filename_num = 1;

static char command[MAX_STRING + 1];
static Boolean update = False;
static Boolean done;

static string source_name;
static string dest_name;
static string read_name;
static string write_name;

static Dimension main_width;
static Dimension main_height;

static XmString image_list[100];




/*
 * All errors are fatal.
 */

static
void s_error (char *problem_string)
{
    tt_fprintf (stderr, "Image: %s\n", problem_string);
    exit (0);
}



static
void init_application (void)
{
    int k;
    Arg args[3];

    /* Initialize the application data structures. */
    for (k = 0; k < MAX_WIDGETS; k++)
        widget_array[k] = NULL;

    widget_array[k_scale_box] = scale_box_widget;
    widget_array[k_import_box] = import_box_widget;

    for (k = 0; k < MAX_IMAGES; k++)
        image_list[k] = (XmString) XmStringCreateLtoR ("", 
            XmSTRING_DEFAULT_CHARSET);

    source_name[0] = '\0';
    dest_name[0] = '\0';
    read_name[0] = '\0';
    write_name[0] = '\0';

    k = 0;
    XtSetArg (args[k], XmNwidth, (XtArgVal) &main_width); k++;
    XtSetArg (args[k], XmNheight, (XtArgVal) &main_height); k++;
    XtGetValues (main_window_widget, args, k);
}



static
void fetch_widget (int widget_num, char *widget_id)
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
void read_parameter (Widget widget, char *tag,
    XmAnyCallbackStruct *callback_data)
{
    Arg args[3];
    int n;
    XmString str_1, str_2;

    fetch_widget (k_read_parameter, "read_parameter_box"); 

    str_1 = XmStringCreateLtoR (tag, XmSTRING_DEFAULT_CHARSET);
    str_2 = XmStringCreateLtoR ("", XmSTRING_DEFAULT_CHARSET);

    n = 0;
    XtSetArg (args[n], XmNselectionLabelString, (XtArgVal) str_1); n++;
    XtSetArg (args[n], XmNtextString, (XtArgVal) str_2); n++;
    XtSetValues (widget_array[k_read_parameter], args, n);

    XmStringFree (str_1);
    XmStringFree (str_2);

    bring_up (k_read_parameter);
}



static
void set_read_text_string (void)

{
    XmString str;
    Arg args[2];
    int n, i;
    string name;

#ifdef VMS
#define dir_sep ']'
#else
#define dir_sep '/'
#endif

    n = strlen (read_name) - 1;
    while (read_name[n] != dir_sep && n > 0)
        {
        n--;
        }

    if (read_name[n] == dir_sep)
        n++;
    i = 0;
    while (read_name[n] != '.' && read_name[n] != '\0')
        {
        name[i] = read_name[n];
        n++;
        i++;
        }
    name[i] = '\0';
        
    str = XmStringCreateLtoR (name, XmSTRING_DEFAULT_CHARSET);

    n = 0;
    XtSetArg (args[n], XmNtextString, (XtArgVal) str); n++;
    XtSetValues (widget_array[k_read_image_name], args, n);
    XmStringFree (str);
}



static
void activate_widget (Widget widget, int *tag,
    XmAnyCallbackStruct *callback_data)
{
    int widget_num = *tag;

    switch (widget_num) {
        case k_read_image:
            fetch_widget (k_read_image, "read_box"); 
            break;

        case k_read_image_name:  
            fetch_widget (k_read_image_name, "read_name_box");
            set_read_text_string ();
            break;

        case k_write_image:   
            fetch_widget (k_write_image, "write_box");
            break;

        case k_quit:
            fetch_widget (k_quit, "quit_warning_box"); 
            break;

        case k_scale_box:
            fetch_widget (k_scale_box, "scale_dialog_box"); 
            break;

        case k_import_box:
            fetch_widget (k_import_box, "import_dialog_box"); 
            break;

        case k_help:
            return;

        default:
            return;
        }

    bring_up (widget_num);
}



static
void dismiss_widget (Widget widget, int *tag,
    XmAnyCallbackStruct *callback_data)
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
    if (status != 0) 
        {
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
    if (status != 0) 
        {
	event.xkey.type		= KeyRelease;
	event.xkey.time		= CurrentTime;

	status = XSendEvent (display, window, False, KeyReleaseMask, &event);
	}
}



static
void do_command (void)
{
    XmString str;

    if (strchr(command, '!') != NULL)
	{
	strtok(command, "!");
	update = True;
	}

    send_button (Button1, 0x00);

    str = XmStringCreateLtoR (command, XmSTRING_DEFAULT_CHARSET);
    XmCommandSetValue (command_window_widget, str);
    XmStringFree (str);

    send_keycode (XKeysymToKeycode (display, XK_Return), 0x00);
}



static
void set_dest_name (void)
{
    strcpy (dest_name, source_name);
}



static
void exec_command (Widget widget, char *tag, XmAnyCallbackStruct *callback_data)
{
    string str;

    set_dest_name ();
    
    strcpy (command, "image ");
    sprintf (str, tag, source_name, dest_name);
    strcat (command, str);
    
    do_command ();
}



static
void set_command (Widget widget, char *tag, XmAnyCallbackStruct *callback_data)
{
    char *more, *s;
    string str;

    s = str;
    strcpy(s, tag);

    if (strchr(s, '!') != NULL)
	{
	strtok(s, "!");
	update = True;
	}

    if (*s == '\r') 
        {
	strcpy (command, "");
	s++;
	}
    else if (*s && *s != ' ')
        strcpy (command, "image ");

    if (more = strchr (s, '\\'))
        strncat (command, s, more-s);
    else 
        {
        strcat (command, s);
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
void get_image (void)
{
    Window root;
    int x, y;
    unsigned int border_width, depth;

    if (ximage != NULL)
	{
	XDestroyImage (ximage);
	ximage = NULL;
	}

    XGetGeometry (display, graphic_window, &root, &x, &y,
	&graphic_width, &graphic_height, &border_width, &depth);
    if (graphic_width < main_width && graphic_height < main_height)
	{
	ximage = XGetImage (display, graphic_window, x, y,
	    graphic_width, graphic_height, AllPlanes, ZPixmap);
	}
}



static
void update_image (void)
{
    string command;

    if (*source_name)
	{
	sprintf (command, "image display %s pixel", source_name);
	gli_do_command (command);
	}
    update = False;

    get_image ();
}



static
void update_image_list (void)
{
    string name, list_entry;
    image_dscr img;
    img_image_descr *img_dscr;
    int i, j, n, stat;
    Arg args[3];

    img_dscr = NIL;
    i = 0;
    do
        {
        img_dscr = img_inquire_image (img_dscr, name, &img, &stat);

        if (stat != img__noimage)
            {
            j = strlen (name);
            for (n = j; n < img_ident_length; n++)
                name[n] = ' ';
            name[n] = '\0';
            strcat (name, "%s   %dx%d");

            switch (img.type) {
                case ppm:
                    sprintf (list_entry, name, " PPM", img.width, img.height);
                    break;

                case pcm:
                    sprintf (list_entry, name, " PCM", img.width, img.height);
                    break;

                case pgm:
                    sprintf (list_entry, name, " PGM", img.width, img.height);
                    break;

                case pbm:
                    sprintf (list_entry, name, " PBM", img.width, img.height);
                    break;
                }

            XmStringFree (image_list[i]);
            image_list[i] = XmStringCreateLtoR (list_entry, 
                XmSTRING_DEFAULT_CHARSET);
            i++;
            }
        }
    while (stat != img__noimage && stat != img__nmi);

    n = 0;
    XtSetArg (args[n], XmNitems, (XtArgVal) image_list); n++;
    XtSetArg (args[n], XmNitemCount, (XtArgVal) i); n++;
    XtSetValues (image_list_widget, args, n);
}



static
void set_something (Widget widget, char *resource, char *value)
{
    Arg a1[1];

    XtSetArg (a1[0], resource, value);
    XtSetValues (widget, a1, 1);
}



static
void set_source_image_cb (XmListCallbackStruct *callback_data)
{
    static string name;
    int n;

    strcpy (name, extract_normal_string (callback_data->item));

    n = img_ident_length;
    do  
        {
        n--;
        }
    while (name[n] == ' ');

    name[n+1] = '\0';
    strcpy (source_name, name);
}
    


static
void read_file_selection_box_cb (
    XmFileSelectionBoxCallbackStruct *callback_data)
{
    char *filename, str[MAX_STRING];

    filename = extract_normal_string (callback_data->value);

    strcpy (read_name, filename);
    sprintf (str, command, filename);
    strcpy (command, str);
}



static
void write_file_selection_box_cb (
    XmFileSelectionBoxCallbackStruct *callback_data)
{
    char *filename, str[MAX_STRING];

    filename = extract_normal_string (callback_data->value);

    strcpy (write_name, filename);
    sprintf (str, command, filename);
    strcpy (command, str);
}



static
void read_image_name_cb (XmSelectionBoxCallbackStruct *callback_data)
{
    char *name, str[MAX_STRING];

    name = extract_normal_string (callback_data->value);
    
    sprintf (str, " %s", name);
    strcat (command, str);

    strcpy(source_name, name);
}



static
void read_parameter_cb (XmSelectionBoxCallbackStruct *callback_data)
{
    string str;
    char *parameter, *c;
    int nparameter;

    c = command;
    nparameter = 0;
    while (c = strchr(c, '%'))
        {
        nparameter++;
        c++;
        }

    parameter = extract_normal_string (callback_data->value);
    set_dest_name ();
    
    if (nparameter == 3)
        sprintf (str, command, source_name, dest_name, parameter);
    else
        sprintf (str, command, source_name, parameter);

    strcpy (command, str);

    do_command ();
}



static
void command_entered_cb (XmCommandCallbackStruct *callback_data)
{
    XDefineCursor (display, main_window, watch_cursor);
    XDefineCursor (display, image_list_window, watch_cursor);
    XFlush (display);

    strcpy (command, extract_normal_string (callback_data->value));
    gli_do_command (command);

    strcpy (command, "");

    if (update)
	update_image ();

    update_image_list ();

    XUndefineCursor (display, image_list_window);
    XUndefineCursor (display, main_window);
    XFlush (display);
}



static
void drawing_area_cb (Widget widget)
{
    graphic_window_widget = widget;
}



static
void image_list_widget_cb (Widget widget)
{
    image_list_widget = widget;
}



static
void scale_width_cb (Widget widget)
{
    scale_width_widget = widget;
}



static
void scale_height_cb (Widget widget)
{
    scale_height_widget = widget;
}



static
void import_image_cb (Widget widget)
{
    import_image_widget = widget;
}



static
void import_variable_cb (Widget widget)
{
    import_variable_widget = widget;
}



static
void import_width_cb (Widget widget)
{
    import_width_widget = widget;
}



static
void scale_box_cb (void)

{
    char *width, *height;

    width = XmTextGetString (scale_width_widget);
    height = XmTextGetString (scale_height_widget);

    set_dest_name ();

    sprintf (command, "image scale %s %s %s %s", source_name, dest_name,
        width, height);
}



static
void import_box_cb (void)

{
    char *image, *variable, *width;

    image = XmTextGetString (import_image_widget);
    variable = XmTextGetString (import_variable_widget);
    width = XmTextGetString (import_width_widget);

    sprintf (command, "image import %s %s %s", image, variable, width);
}



static
void do_something (Widget widget, int *tag, XmAnyCallbackStruct *callback_data)
{
    int widget_num = *tag;
    string str;

    switch (widget_num) 
        {
        case k_read_image:	
            read_file_selection_box_cb (
		(XmFileSelectionBoxCallbackStruct *)callback_data);
            break;

        case k_read_image_name:	
            read_image_name_cb (
		(XmSelectionBoxCallbackStruct *)callback_data);
            do_command ();
            break;

        case k_write_image:       
            write_file_selection_box_cb (
		(XmFileSelectionBoxCallbackStruct *)callback_data);
            strcat (command, source_name); 
            do_command ();
            break;

        case k_quit:		
            done = True; 
            break;

        case k_enter:		
            command_entered_cb ((XmCommandCallbackStruct *)callback_data); 
            break;

        case k_drawing_area:	
            drawing_area_cb (widget); 
            break;

        case k_image_list:
            image_list_widget_cb (widget);
            break;

        case k_scale_width:
            scale_width_cb (widget);
            break;

        case k_scale_height:
            scale_height_cb (widget);
            break;

        case k_import_image:
            import_image_cb (widget);
            break;

        case k_import_variable:
            import_variable_cb (widget);
            break;

        case k_import_width:
            import_width_cb (widget);
            break;

        case k_image_selected:
            set_source_image_cb ((XmListCallbackStruct *)callback_data);
            break;

        case k_read_parameter:
            read_parameter_cb ((XmSelectionBoxCallbackStruct *)callback_data);
            break;

        case k_set_names:
            sprintf (str, command, source_name, dest_name);
            strcpy (command, str);
            break;

        case k_scale_box:
            scale_box_cb ();
            do_command ();
            break;

        case k_import_box:
            import_box_cb ();
            do_command ();
            break;
        }

    bring_down (widget_num);
}



static
void configure_cb (Widget widget, XtPointer closure, XConfigureEvent *event,
    Boolean *continue_to_dispatch)
{
    graphic_width += event->width - main_width;
    graphic_height += event->height - main_height;

    ImageResize (graphic_width, graphic_height);

    main_width = event->width;
    main_height = event->height;

    update_image ();
}	



static
void info (int x, int y, int button)
{
    char s[80];
    unsigned long pixel;
    XColor color;

    XFillRectangle (display, graphic_window, clear, 10, 10, 240, 20);
    XDrawRectangle (display, graphic_window, gc, 10, 10, 240, 20);

    if (button > 1)
	{
	if (ximage == NULL)
	    get_image ();

	if (ximage != NULL)
	    {
	    pixel = XGetPixel (ximage, x, y);
	    color.pixel = pixel;

	    XQueryColor (display,
		XDefaultColormapOfScreen (XDefaultScreenOfDisplay (display)),
		&color);
	    }
	else
	    button = 1;
	}

    switch (button)
	{
	case 1:
	    sprintf (s, "+%d+%d", x, y);
	    break;
	case 2:
	    sprintf (s, "+%d+%d (%3u,%3u,%3u) %d",
		x, y, color.red >> 8, color.green >> 8, color.blue >> 8, pixel);
	    break;
	}

    XDrawString (display, graphic_window, gc,
	130 - XTextWidth (font, s, strlen(s)) / 2, 24, s, strlen(s));
}


static
void process_motion_event (Widget widget, XtPointer closure,
    XMotionEvent *event, Boolean *continue_to_dispatch)
{
    if (event->x >= 0 && event->y >= 0 &&
	event->x < graphic_width && event->y < graphic_height)
	{
	if (event->state & Button1MotionMask)
	    info (event->x, event->y, 1);
	else if (event->state & Button2MotionMask)
	    info (event->x, event->y, 2);
	}
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
    image_list_window = XtWindow (image_list_widget);
    window = XtWindow (command_widget);

    graphic_window = XtWindow (graphic_window_widget);
    XDefineCursor (display, graphic_window, cross_cursor);
}



static
void create_GC (void)
{
    Screen *screen;
    XGCValues xgcv;

    screen = XDefaultScreenOfDisplay (display);

    xgcv.foreground = XBlackPixelOfScreen (screen);
    xgcv.background = XWhitePixelOfScreen (screen);
    gc = XCreateGC (display, graphic_window, GCForeground | GCBackground,
	&xgcv);

    xgcv.foreground = xgcv.background;
    clear = XCreateGC (display, graphic_window, GCForeground | GCBackground,
	&xgcv);
}



static
void load_font (void)
{
    font = XLoadQueryFont (display, "variable");
    if (font == NULL)
	font = XLoadQueryFont (display, "fixed");

    XSetFont (display, gc, font->fid);
}



static
void free_GC (void)
{
    XFreeGC (display, clear);
    XFreeGC (display, gc);
}



static
void add_event_handlers (void)
{
    XtAddEventHandler (main_window_widget, StructureNotifyMask, False,
	(XtEventHandler) configure_cb, NULL);
    XtAddEventHandler (graphic_window_widget, PointerMotionMask, False,
        (XtEventHandler) process_motion_event, NULL);
}


static
void remove_event_handlers (void)
{
    XtRemoveEventHandler (graphic_window_widget, PointerMotionMask, False,
        (XtEventHandler) process_motion_event, NULL);
    XtRemoveEventHandler (main_window_widget, StructureNotifyMask, False,
	(XtEventHandler) configure_cb, NULL);
}



/* The names and addresses of things that Mrm has to bind. The names do
 * not have to be in alphabetical order. */

static MrmRegisterArg reglist[] = {
    {"activate_widget", (caddr_t) activate_widget},
    {"dismiss_widget", (caddr_t) dismiss_widget},
    {"do_something", (caddr_t) do_something},
    {"set_command", (caddr_t) set_command},
    {"read_parameter", (caddr_t) read_parameter},
    {"exec_command", (caddr_t) exec_command}
};

static int reglist_num = (sizeof reglist / sizeof reglist [0]);



static
void error_handler (char *line)
{
    XmString str;

    fetch_widget (k_warning, "warning_box");

    str = XmStringCreateLtoR (line, XmSTRING_DEFAULT_CHARSET);
    set_something (widget_array[k_warning], XmNmessageString, (char *)str);
    XmStringFree (str);

    bring_up (k_warning);

    update = False;
}



static
void synchronize (void)

{
    XEvent event;

    while (XtAppPending (app_context)) 
        {
	XtAppNextEvent (app_context, &event);
	XtDispatchEvent (&event);
        }
}



/*
 * Image main loop
 */

void ImageMainLoop (void)

{
    Arg arglist[2];
    XEvent event;
    int n, argc = 0;
    char *name, *path;
    static Bool app = False;

    if (!app) 
        {
        MrmInitialize ();               /* Initialize Mrm before initializing
                                         * the X Toolkit. */
        XtToolkitInitialize ();
        app_context = XtCreateApplicationContext ();

        /* Establish a default set of resource values */

        XtAppSetFallbackResources (app_context, resources);

        name = (char *) getenv ("GLI_CONID");
        if (!name) name = (char *) getenv ("DISPLAY");

        if ((display = XtOpenDisplay (app_context, name, "Image", "image",
	    NULL, 0, &argc, NULL)) == NULL)
	    s_error ("can't open display");

        n = 0;

        toplevel_widget = XtAppCreateShell ("Image", NULL,
	    applicationShellWidgetClass, display, arglist, n);

        /* Open the UID files (the output of the UIL compiler) */

        path = (char *) getenv ("GLI_HOME");
        if (!path) path = default_path;

	db_filename_vec[0] = (String) malloc (MAX_STRING);
#ifdef VMS
        sprintf (db_filename_vec[0], "%simage.uid", path);
#else
#ifdef _WIN32
        sprintf (db_filename_vec[0], "%s\\image.uid", path);
#else
        sprintf (db_filename_vec[0], "%s/image.uid", path);
#endif
#endif

#ifdef VMS
        if (MrmOpenHierarchy (
#else
        if (MrmOpenHierarchyPerDisplay (
	    display,				/* Connection to the X server */
#endif
	    db_filename_num,			/* Number of files. */
            db_filename_vec,                    /* Array of file names. */
            NULL,                               /* Default OS extension. */
            &s_MrmHierarchy)                    /* Pointer to returned Mrm ID */
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

        if (MrmFetchWidget (s_MrmHierarchy, "image_list_box",
            toplevel_widget, &list_box_widget, &dummy_class) != MrmSUCCESS)
            s_error ("can't fetch list box");

        if (MrmFetchWidget (s_MrmHierarchy, "scale_dialog_box",
            toplevel_widget, &scale_box_widget, &dummy_class) != 
            MrmSUCCESS)
            s_error ("can't fetch scale box");

        if (MrmFetchWidget (s_MrmHierarchy, "import_dialog_box",
            toplevel_widget, &import_box_widget, &dummy_class) != 
            MrmSUCCESS)
            s_error ("can't fetch import box");

        app = True;
        }

    XtManageChild (list_box_widget);
    XtRealizeWidget (toplevel_widget);
    synchronize ();

    init_application ();
    update_image_list ();

    watch_cursor = XCreateFontCursor (display, XC_watch);
    cross_cursor = XCreateFontCursor (display, XC_cross);

    window_setup ();

    ImageOpen ((int *)graphic_window_widget);
    synchronize ();

    create_GC ();
    load_font ();
    add_event_handlers ();

    establish ((int (*)())error_handler);

    /* Image main loop */

    done = False;
    while (!done) 
        {
	XtAppNextEvent (app_context, &event);
	XtDispatchEvent (&event);
	}

    establish (NULL);

    ImageClose ((int *)graphic_window_widget);

    remove_event_handlers ();
    free_GC ();

    XFreeCursor (display, cross_cursor);
    XFreeCursor (display, watch_cursor);

    XtUnmanageChild (list_box_widget);

    XtUnrealizeWidget (toplevel_widget);
    synchronize ();
}



#else

#include "terminal.h"

void ImageMainLoop (void)

{
    tt_fprintf (stderr, "can't access OSF/Motif libraries\n");
}

#endif
