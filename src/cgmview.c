/*
 *
 * (C) Copyright 1991-1999  Josef Heinen
 * 
 * 
 * FACILITY:
 * 
 *     Graphics Language Interpreter (GLI)
 * 
 * ABSTRACT:
 * 
 *     This module contains an OSF/Motif based viewer for CGM files
 *     (Computer Graphics Metafile).
 * 
 * AUTHOR:
 * 
 *     Jens Kuenne  7-AUG-1991
 *     Josef Heinen
 * 
 * VERSION:
 * 
 *     V1.0
 * 
 */


#include <stdio.h>		      
#include <stdlib.h>		      
#include <math.h>		      
#include <string.h>
#include <signal.h>

#if !defined(VMS) && !defined(MSDOS) && !defined(_WIN32)
#include <unistd.h>
#endif

#include "terminal.h"

/*
 * MOTIF definitions
 */
#ifdef MOTIF

#include <Xm/Text.h>
#include <Xm/MessageB.h>
#include <Mrm/MrmAppl.h>	       /* Motif Toolkit and MRM */
#include <X11/X.h>

#include "cgmview.h"

/*
 * Global data
 */
static MrmType  class_id;	       /* Place to keep class ID */
static MrmType  dummy_class;	       /* and class variable. */
#ifdef VMS
static char    *default_path = "sys$sysdevice:[gli]";
#else
#ifdef _WIN32
static char    *default_path = "c:\\gli";
#else
static char    *default_path = "/usr/local/gli";
#endif
#endif
static char    *db_filename_vec[] =    /* Mrm.hierachy file list. */
 { "cgmview.uid                                                    " };
static int      db_filename_num =
(sizeof db_filename_vec / sizeof db_filename_vec[0]);
int             i;
#define hash_table_limit 500
struct HASH_TABLE_STRUCT
{
    char           *widget_name;
    Widget          id;
}               hash_table[hash_table_limit + 1];

/*
 * Names and addresses of callback routines to register with Mrm
 */
static void do_something(Widget, int *, XmAnyCallbackStruct *);
static MrmRegisterArg reglist[] = {
{"do_something", (caddr_t) do_something}};
static int      reglist_num = (sizeof reglist / sizeof reglist[0]);


/*
 * X Window globals
 */
static XWindowAttributes window_attributes;	/* use to get window  attr */
static XGCValues values_struct;
static Window   window_id;
static Display *display_ptr;
static Widget   draw_window;
static GC       gc_id;				/* graphical context */

#endif	/* MOTIF */

/*
 * User includes 
 */
#include "gksdefs.h"

void cgm_import (char *, int, int, int);


/* 
 * user defined globals 
 */
#define BINARY_FORMAT 1
#define CLEAR_FORMAT 2

static int      cgm_format = BINARY_FORMAT;	    /* Format of CGM-Input */
static int      picture_nr = 1;			    /* current picture-nr  */
#ifdef MOTIF
static int      file_open = 0;			    /* is file opend	   */
#endif
static int      ws_id = 1;			    /* GKS Workstation ID  */
static int      wstype = 211;			    /* GKS Workstation Type*/
static int	con_id = 0;			    /* GKS Connection ID   */
static char     filename[255];			    /* Input filename      */
static int	flags[13] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};


/*
 * get the GKS workstation type
 */
static void get_wstype(void)
{
    char *env;

    env = (char *) getenv("GLI_WSTYPE");
    if (env)
        wstype = atoi(env);
    else
        wstype = GWSDEF;
}


#ifdef MOTIF
/*
 * All errors are fatal.
 */
static void s_error(char *problem_string)
{
    Widget	id;
    Arg		al[2];
    

    if (MrmFetchWidget(s_MrmHierarchy, "information_widget", toplevel_widget,
	&id, &dummy_class)  == MrmSUCCESS)
	{
	XtSetArg(al[0], XmNmessageString, 
	    XmStringCreateLtoR (problem_string, XmSTRING_DEFAULT_CHARSET));
	XtSetArg(al[1], XmNokLabelString, 
	    XmStringCreateLtoR ("Acknowledged", XmSTRING_DEFAULT_CHARSET));
	XtSetValues(id, al, 2);

	if (!XtIsManaged(id))
	    {
	    /* Minimize Buttons */
	    XtUnmanageChild(XmMessageBoxGetChild(id,XmDIALOG_HELP_BUTTON));  
	    XtUnmanageChild(XmMessageBoxGetChild(id,XmDIALOG_CANCEL_BUTTON));  

	    XtManageChild(id);
	    }
	}
}



/*
 * initialize the GKS
 */
static void do_gks_init(void)
{
    int             errfil = 0;
    int             bufsize = -1;
    int		    wc = 1;
    int             *conid;

    /* emergency close GKS */
    GECLKS ();

    putenv ("GLI_GKS_CMAP_EXTENT=");

    /* Initialize GKS */
    GOPKS (&errfil, &bufsize);

    /* Set aspect source flags */
    GSASF (flags);

    display_ptr = XtDisplay(draw_window);
    window_id = XtWindow(draw_window);
    if (!XGetWindowAttributes(display_ptr, window_id, &window_attributes))
    {
	s_error("couldn't get attributes");
	return;
    }

    conid = (int *)draw_window;
    GOPWK (&ws_id, conid, &wstype);
    GACWK (&ws_id);

    /* get the default graphics context */
    if (gc_id != (GC) 0)
	XFreeGC(display_ptr, gc_id);

    gc_id = XCreateGC(display_ptr, window_id, 0, &values_struct);

    GSELNT (&wc);
}



/*
 * setup gks and call CGM viewer
 */
static int draw_picture(int page_shift)
{
    if (file_open == 1)
    {
	if (access(filename, 4))
	{
	    char s[255];
	    
	    sprintf(s, "Can't read file %s", filename);
	    s_error(s);
	} else
        {
	    /* initialize GKS */
	    do_gks_init();

	    GCLRWK (&ws_id, &GALWAY);

	    cgm_import(filename, cgm_format, page_shift, 0);
        }
    }
    else
    {
        s_error("No file opened");
    }
    return (1);
}



static int HashFunction(char *name, int max)

{
#define HashVecSize		20     /* plenty for 31 character names */
    typedef union
    {
	short int       intname[HashVecSize];	/* name as vector of
						 * ints */
	char            charname[2 * HashVecSize];	/* name as vector of
							 * chars */
    }               HashName;

    HashName        locname;	       /* aligned name */
    int             namelen;	       /* length of name */
    int             namelim;	       /* length limit (fullword size) */
    int             namextra;	       /* limit factor remainder */
    int             code = 0;	       /* hash code value */
    int             ndx;	       /* loop index */


    /*
     * Copy the name into the local aligned union. Process the name as
     * a vector of integers, with some remaining characters. The string
     * is copied into a local union in order to force correct alignment
     * for alignment-sensitive processors.
     */
    strcpy(locname.charname, name);
    namelen = strlen(locname.charname);
    namelim = namelen >> 1;	       /* divide by 2 */
    namextra = namelen & 1;	       /* remainder */

    /*
     * XOR each integer part of the name together, followed by the
     * trailing 0/1 character
     */
    for (ndx = 0; ndx < namelim; ndx++)
	code = code ^ ((locname.intname[ndx]) << ndx);
    if (namextra > 0)
	code = code ^ ((locname.intname[ndx]) & 0x00FF);

    return (code & 0x7FFF) % max;
}



static int HashLookup(char *name, Widget *id)
{
    int             ndx;

    for (ndx = HashFunction(name, hash_table_limit);
	 ((hash_table[ndx].widget_name != NULL) &&
	  (ndx <= hash_table_limit));
	 ndx++)
	if (strcmp(name, hash_table[ndx].widget_name) == 0)
	{
	    *id = hash_table[ndx].id;
	    return (1);
	}
    if (ndx > hash_table_limit)
	for (ndx = 0;
	     ((hash_table[ndx].widget_name != NULL) &&
	      (ndx <= hash_table_limit));
	     ndx++)
	{
	    if (strcmp(name, hash_table[ndx].widget_name) == 0)
	    {
		*id = hash_table[ndx].id;
		return (1);
	    }
	}

    return (0);
}


static int HashRegister(char *widget_name, Widget id)
{
    int             ndx;

    for (ndx = HashFunction(widget_name, hash_table_limit);
	 ((hash_table[ndx].widget_name != NULL) &&
	  (ndx < hash_table_limit));
	 ndx++);
    if (hash_table[ndx].widget_name != NULL)
	for (ndx = 0;
	     hash_table[ndx].widget_name != NULL;
	     ndx++);
    if (ndx > hash_table_limit)
	return (0);
    else
    {
	hash_table[ndx].widget_name = XtCalloc(1, strlen(widget_name) + 1);
	strcpy(hash_table[ndx].widget_name, widget_name);
	hash_table[ndx].id = id;
	return (1);
    }
}


static void VUIT_Manage(char *widget_name)
{
    Widget          id;
    Window          pop_window;
    XWindowChanges  values;

    if (HashLookup(widget_name, &id))
	if (XtIsManaged(id))
	{
	    pop_window = XtWindow(XtParent(id));
	    values.x = values.y = values.width = values.height =
		values.border_width = values.sibling = 0;
	    values.stack_mode = Above;
	    XConfigureWindow(display, pop_window, CWStackMode, &values);
	} else
	    XtManageChild(id);
    else
    {
	MrmFetchWidget(s_MrmHierarchy, widget_name, toplevel_widget, &id,
		       &class_id);
	XtManageChild(id);
	HashRegister(widget_name, id);
    }
}


static void VUIT_Unmanage(char *widget_name)
{
    Widget          id;

    if (HashLookup(widget_name, &id))
	XtUnmanageChild(id);
}


/* 
 * support routine to get normal string from XmString 
 */
static char *extract_normal_string(XmString cs)
{

    XmStringContext context;
    XmStringCharSet charset;
    XmStringDirection direction;
    Boolean         separator;
    static char    *primitive_string;

    XmStringInitContext(&context, cs);
    XmStringGetNextSegment(context, &primitive_string,
			   &charset, &direction, &separator);
    XmStringFreeContext(context);
    return ((char *) primitive_string);
}


/*
 * Event-Handler for ConfigureNotify - Event
 */
static void process_event(Widget widget, caddr_t client_data,
    XConfigureEvent *event, Boolean *continue_to_dispatch)
{
    int             errind;
    int             dcunit, lx, ly, w, h;
    float           rx, ry, x0, x1, y0, y1;
    int             *conid;

    if (widget && (event->type == ConfigureNotify) && (file_open == 1))
    {
	GQWKC (&ws_id, &errind, &con_id, &wstype);
	if (errind == 0)
	{
	    GDAWK (&ws_id);
	    GCLWK (&ws_id);
	}

	conid = (int *)draw_window;
	GOPWK (&ws_id, conid, &wstype);
	GACWK (&ws_id);

	/* Inquire the new window size */
	GQDSP (&wstype, &errind, &dcunit, &rx, &ry, &lx, &ly);

	if (dcunit == GMETRE)
	{
	    w = (event->width > k_draw_width) ? event->width :
		event->width - (k_main_width - k_draw_width);
	    h = (event->height > k_draw_height) ? event->height - 50 :
		event->height - (k_main_height - k_draw_height);

	    x0 = 0;
	    x1 = rx * w / lx;
	    y0 = 0;
	    y1 = ry * h / ly;

	    GSWKVP (&ws_id, &x0, &x1, &y0, &y1);
	}
	GUWK (&ws_id, &GPERFO);

	draw_picture(picture_nr);
    }
}



/*
 * get the new filename and draw the picture
 */
static void new_file(XmFileSelectionBoxCallbackStruct *call_data)
{
    char           *result;

    result = extract_normal_string(call_data->value);
    strcpy(filename, result);
    file_open = 1;
    picture_nr = 1;
    draw_picture(picture_nr);
}


/*
 * Callback for all buttons
 */
static void do_something(Widget w, int *tag, XmAnyCallbackStruct *cb_data)
{
    int             action;

    action = *tag;

    switch (action)
    {
    case k_draw_cb:		/* Drawing Area created */
	{
	    draw_window = w;

	    /* Add Event Handler */
	    XtAddEventHandler(toplevel_widget, StructureNotifyMask, False,
			      (XtEventHandler) process_event, NULL);
	    break;
	}
    case k_format_binary:	/* Change input-format to binary */
	{
	    cgm_format = BINARY_FORMAT;
	    break;
	}
    case k_format_clear_text:	/* Change input-format to clear text */
	{
	    cgm_format = CLEAR_FORMAT;
	    break;
	}
    case k_previous_picture:	/* draw the previous picture */
	{
	    if (picture_nr > 1)
	    {
		picture_nr--;
		draw_picture(picture_nr);
	    }
	    break;
	}
    case k_redraw_picture:	/* redraw the picture */
	{
	    draw_picture(picture_nr);
	    break;
	}
    case k_next_picture:	/* draw the next picture */
	{
	    picture_nr++;
	    draw_picture(picture_nr);
	    break;
	}
    case k_file_ok:		/* got input-file */
	{
	    VUIT_Unmanage("file_box_widget");
	    new_file((XmFileSelectionBoxCallbackStruct *)cb_data);
	    break;
	}
    case k_file_cancel:		/* caneld file selection */
	{
	    VUIT_Unmanage("file_box_widget");
	    file_open = 0;
	    break;
	}
    case k_open:		/* want file selection */
	{
	    VUIT_Manage("file_box_widget");
	    break;
	}
    case k_quit:		/* quit the CGMViewer */
	{
	    GECLKS();
	    VUIT_Unmanage("main_window_widget");
	    exit(0);
	    break;
	}

    case k_ok:			/* information aknowledged */
    case k_cancel:
	{
	    VUIT_Unmanage("information_widget");
	    break;
	}

    case k_help:		/* want help about CGMViewer */
	{
	    /* VUIT_Manage("dxm_help_widget"); */
	    break;
	}

    default:			/* do nothing */
	{
	    break;
	}
    }
}

#endif				       /* MOTIF */



static void usage (void)
{
    tt_fprintf(stderr, "\
Usage:\n\
cgmview [-c] [-h] [-m] [-p picture] [-t wstype] file\n\
         -c   Specifies the input file to be a clear text format CGM file,\n\
              default is binary format.\n\
         -h   Prints this information.\n\
         -m   Tells cgmview to maximize the output window.\n\
 -p picture   Specifies the picture number, picture.\n\
  -t wstype   Use GKS workstation type wstype.\n\
\n\
The present workstation types recognized by cgmview are:\n\
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

    exit(-1);
}


void exit_handler (void)
{
    GECLKS ();
}


void signal_handler (void)
{
    exit_handler ();
    exit (0);
}


int gli_do_command(void)
{
    return (1);
}


/*
 * OS transfer point.  The main routine does all the one-time setup and
 * then calls XtAppMainLoop.
 */
int main(int argc, char **argv)
{

#ifndef __linux__
#if defined(__ALPHA) || defined(__alpha)
#define machine "ALPHA"
#endif
#endif
#ifdef __sgi
#define machine "SGI"
#endif
#ifdef __linux__
#define machine "PCLINUX"
#endif
#ifdef __NetBSD__
#define machine "NetBSD"
#endif
#ifndef __sgi
#ifdef mips
#define machine "RISC"
#endif
#endif
#ifdef sun
#define machine "SUN"
#endif
#ifdef hpux
#define machine "HP-UX"
#endif
#ifdef cray
#define machine "CRAY"
#endif
#ifdef aix
#define machine "AIX"
#endif
#ifdef VAX
#define machine "VAX"
#endif
#ifdef __APPLE__
#define machine "Apple"
#endif
#if defined (MSDOS) || defined (_WIN32)
#define machine "PC"
#endif
#ifdef VMS
#define os "VMS"
#else
#ifdef MSDOS
#define os "MS-DOS"
#else
#ifdef _WIN32
#define os "WINDOWS"
#else
#ifdef __osf__
#define os "Digital UNIX"
#else
#ifdef __DARWIN__
#define os "Darwin"
#else
#define os "UNIX"
#endif
#endif
#endif
#endif
#endif

    if (isatty(1))
    {
        tt_printf("\n\tC G M V I E W\n");
        tt_printf("\t%s version 4.5 (%s)\n", machine, os);
        tt_printf("\tpatchlevel 4.5.30, 13 Apr 2012\n\n");
        tt_printf("\tCopyright @ 1991-1999, Josef Heinen, Jens Kuenne\n");
        tt_printf(
	    "\tCopyright @ 1989, Pittsburgh Supercomputing, Phil Andrews\n");
        tt_printf("\n\tSend bugs and comments to J.Heinen@FZ-Juelich.de\n\n");
    }

#if defined (sun) && !defined (__SVR4)
    on_exit ((void (*)(void))exit_handler, NULL);
#else
    atexit ((void (*)(void))exit_handler);
#endif

    signal (SIGTERM, (void (*)(int))signal_handler);
#ifdef SIGQUIT
    signal (SIGQUIT, (void (*)(int))signal_handler);
#endif
#ifdef SIGHUP
    signal (SIGHUP, (void (*)(int))signal_handler);
#endif

    if (argc <= 1)
    {
#ifdef MOTIF
	Arg             arglist[2];
	int             n;
	char	    *path;

	get_wstype();

	MrmInitialize();		   /* Initialize MRM before
					    * initializing 
					    * the X Toolkit. */

	/*
	 * If we had user-defined widgets, we would register them with
	 * Mrm.here.
	 */

	/*
	 * Initialize the X Toolkit. We get back a top level shell widget.
	 */
	XtToolkitInitialize();

	app_context = XtCreateApplicationContext();
	display = XtOpenDisplay(app_context, NULL, "CGMview", "cgmview",
				NULL, 0, &argc, argv);
	if (display == NULL)
	{
	    char s[255];

	    sprintf(s, "%s: can't open display", argv[0]);
	    s_error(s);
	    exit(1);
	}
	n = 0;
	XtSetArg(arglist[n], XmNallowShellResize, True);
	n++;
	toplevel_widget = XtAppCreateShell("CGMview", NULL,
			applicationShellWidgetClass, display, arglist, n);

	/*
	 * get path for cgmview.uid 
	 */
	path = (char *) getenv("GLI_HOME");
	if (!path)
	    path = default_path;
#ifdef VMS
	sprintf(db_filename_vec[0], "%scgmview.uid", path);
#else
#ifdef _WIN32
	sprintf(db_filename_vec[0], "%s\\cgmview.uid", path);
#else
	sprintf(db_filename_vec[0], "%s/cgmview.uid", path);
#endif
#endif

	/*
	 * Open the UID files (the output of the UIL compiler) in the
	 * hierarchy
	 */
	if (MrmOpenHierarchy(db_filename_num,	/* Number of files. */
			     db_filename_vec,	/* Array of file names.  */
			     NULL,	        /* Default OS extenstion. */
			     &s_MrmHierarchy)	/* Pointer to returned
						 * MRM ID */
	    != MrmSUCCESS)
	    s_error("can't open hierarchy");

	MrmRegisterNames(reglist, reglist_num);

	VUIT_Manage("main_window_widget");

	/*
	 * Realize the top level widget.  All managed children now become
	 * visible
	 */
	XtRealizeWidget(toplevel_widget);

	/*
	 * Sit around forever waiting to process X-events.  We never leave
	 * XtAppMainLoop. From here on, we only execute our callback
	 * routines.
	 */
	XtAppMainLoop(app_context);
#else
	usage ();
#endif
    } else
    {
	char           *option;
	float           x1, x2, y1, y2;
	int		errfil = 0, bufsiz = -1;
	int             wc = 1, maximize = 0;
        int             errind, dcunit, lx, ly;

	/* setup the defaults */
	con_id	    = 0;
	picture_nr  = 1;
	cgm_format  = BINARY_FORMAT;

        get_wstype();

	x1 = 0.0;
        x2 = 0.1905;
        y1 = 0.0;
        y2 = 0.1905;

	argv++;
	/* check the options */
	while (option = *argv++)
	{
	    if (!strcmp(option, "-c"))  	/* clear text format */
		cgm_format = CLEAR_FORMAT;

            else if (!strcmp(option, "-h"))	/* help */
		usage ();

	    else if (!strcmp(option, "-m"))	/* maximize */
		maximize = 1;

	    else if (!strcmp(option, "-p"))     /* picture number */
            {
		if (*argv)
		    picture_nr = atoi(*argv++);
                else
                    usage ();
            }
	    else if (!strcmp(option, "-t"))     /* GKS Workstation Type */
            {
		if (*argv)
                    wstype = atoi(*argv++);
                else
                    usage ();
            }
	    else if (*option == '-')
	    {
		tt_fprintf(stderr, "Invalid option: '%s'\n", option);
		usage ();
	    }
            else
                strcpy (filename, option);
	}

	if (access(filename, 4))
	{
            tt_fprintf(stderr, "Can't read file %s\n", filename);
            exit(1);
        }

	putenv ("GLI_GKS_CMAP_EXTENT=");

	/* initialize GKS */

	GOPKS (&errfil, &bufsiz);

	/* Set aspect source flags */
	GSASF (flags);

	GOPWK (&ws_id, &con_id, &wstype);
	GACWK (&ws_id);

	GSELNT (&wc);

	if (maximize)
	{
	    GQDSP (&wstype, &errind, &dcunit, &x2, &y2, &lx, &ly);
            x2 = x2*0.9;
            y2 = x2/sqrt(2.0);
	}
        GSWKVP (&ws_id, &x1, &x2, &y1, &y2);
	if (maximize)
        {
            y2 /= x2; x2 = 1.0;
            GSWKWN (&ws_id, &x1, &x2, &y1, &y2);
        }

	cgm_import(filename, cgm_format, picture_nr, 0);

	GECLKS ();
    }

    return 0;
}
