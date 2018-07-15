/*
 *
 * FACILITY:
 *
 *	VAX C Run-Time System
 *
 * ABSTRACT:
 *
 *	This module contains a Form Driver.
 *
 * AUTHOR:
 *
 *	Josef Heinen
 *	Matthias Steinert (C Version)
 *
 * VERSION:
 *
 *	V1.0-00
 *
 * MODIFIED BY:
 *
 *	Gunnar Grimm (Tcl/Tk extension)
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#if !defined(VMS) && !defined(MSDOS) && !defined(_WIN32)
#include <unistd.h>
#endif

#ifdef TCL
#include <tcl.h>
#include <tk.h>
#endif

#if defined(__WORDSIZE)
#if __WORDSIZE == 64
#define u_addr unsigned long
#else
#define u_addr unsigned int
#endif
#else
#if defined(__ALPHA) || defined(__alpha) || defined(MSDOS)
#define u_addr unsigned long
#else
#define u_addr unsigned int
#endif
#endif

#include "system.h"
#include "symbol.h"
#include "terminal.h"
#include "strlib.h"
#include "variable.h"
#include "formdrv.h"

#define u_char char

/* Characters */

#define ctrl_a	    '\1'
#define ctrl_c	    '\2'
#define ctrl_d	    '\4'
#define ctrl_e	    '\5'
#define backspace   '\10'
#define tab	    '\11'
#define linefeed    '\12'
#define ctrl_l	    '\14'
#define formfeed    ctrl_l
#define cr	    '\15'

#define ctrl_r	    '\22'
#define ctrl_w	    '\27'
#define ctrl_z	    '\32'

#define control_flag   '!'
#define space	    ' '
#define plus	    '+'
#define minus	    '-'
#define period	    '.'
#define terminator  '\0'

#define semicolon   ';'
#define cont_char   '-'
#define dollar	    '$'
#define backslash   '\134'

#define help	    '?'
#define rubout	    '\177'

#ifdef DECterm
#define diamond     '`'
#else
#define diamond     '\\'
#endif

#define null	    tt_k_null
#define cancel	    tt_k_cancel
#define hangup	    tt_k_hangup

#define f1	    tt_k_f1
#define f2	    tt_k_f2
#define f3	    tt_k_f3
#define f4	    tt_k_f4

#define pf1	    tt_k_pf_1
#define pf2	    tt_k_pf_2
#define pf3	    tt_k_pf_3
#define pf4	    tt_k_pf_4

#define del_chr     tt_k_kp_com
#define del_field   tt_k_kp_hyp
#define enter	    tt_k_kp_ntr

#define uparrow     tt_k_ar_up
#define downarrow   tt_k_ar_down
#define rightarrow  tt_k_ar_right
#define leftarrow   tt_k_ar_left

#define home_cursor tt_k_home
#define upwards     tt_k_kp_8
#define downwards   tt_k_kp_2
#define east	    tt_k_kp_6
#define north_east  tt_k_kp_9
#define south_east  tt_k_kp_3
#define west	    tt_k_kp_4
#define north_west  tt_k_kp_7
#define south_west  tt_k_kp_1

#define top	    tt_k_kp_5
#ifndef MSDOS
#define open_line   tt_k_kp_0
#endif

#define f_10	    tt_k_exit
#define f_12	    tt_k_bol
#define f_13	    tt_k_del_word
#define f_14	    tt_k_ins_ovs

#define help_key    tt_k_help
#define do_key	    tt_k_do 


/* Other */

#ifdef _WIN32
#define max_row 	25
#else
#define max_row 	24
#endif
#define max_column	80

#define max_line	20
#define status_line	24
#define legend_length	(max_column/4)-2

#define id_length	31
#define max_name_length 64
#define filspec_length	255

#define buffer_size	255

#define min_integer	-2147483647
#define max_integer	2147483647

#define min_real	-0.17E39
#define max_real	0.17E39


#if defined (VMS)

#define char_align	1
#define int_align	1
#define float_align	1
#define enum_align	1

#else

#if defined (cray)

#define char_align	1
#define int_align	8
#define float_align	8
#define enum_align	8

#else

#define char_align	1
#define int_align	4
#define float_align	4
#define enum_align	4

#endif /* cray */

#endif

#define align(i, j) i%j ? i+j-i%j : i

#define int16_align	2
#define int32_align	4


#define BOOL int
#define TRUE 1
#define FALSE 0
#define NIL 0
#define EMPTY_SET 0

/* 
 * defines for events -
 * 1-4	-> legend
 */
#define LOADFILE 5
#define SAVEFILE 6


#define odd(status) ((status) & 01)
#define present(object) ((object) != NIL)


typedef struct {
    unsigned short len;
    char body[65535];
} varying_string;

typedef enum { integer, real, ascii, list, none } type_descr;
typedef enum { mapped, overlaid } list_format;
typedef enum { label, fieldname, entry, menubutton, button } widget_format;


typedef struct text_descr_struct {
    struct text_descr_struct *next;
    char		     str[max_column+1];
    int 		     line, column;
    } text_descr;


typedef text_descr legend_descr[5];


typedef struct item_descr_struct {
    struct item_descr_struct *prev, *next;
    char		     name[max_name_length];
    int 		     order;
    int 		     efn;
    } item_descr;


typedef struct {		
    int     i_value;
    int     i_range;
    int     i_min, i_max;
    } int_type;


typedef struct {
    float   r_value;
    int     r_range;
    float   r_min, r_max;
    } real_type;


typedef struct {
    list_format format;
    struct item_descr_struct *first_item, *item;
    } list_type;


typedef struct field_descr_struct {
    struct field_descr_struct *prev, *next;
    struct text_descr_struct  *name;
    struct text_descr_struct  data;
    int 		      width, offset, efn;
    int 		      monitor;
    type_descr		      type;
    union {
	int_type    i;
	real_type   r;
	list_type   l;
	} field_type;
    } field_descr;


typedef struct node_descr_struct {
    struct node_descr_struct *next;
    int 		     key;
    char		     name[max_name_length];
    struct menu_descr_struct *sub_menu;
    } node_descr;


typedef struct menu_descr_struct {
    struct menu_descr_struct *prev;
    struct node_descr_struct *first_node, *node;
    int 		     first_line, last_line;
    struct field_descr_struct *first_field, *field;
    struct text_descr_struct *first_field_name, *field_name, *first_text, *text;
    legend_descr	     *legend;
    u_addr		     *db;
    int			     max_button_width;
    } menu_descr;


typedef tt_attributes fdv_attributes;

typedef enum { up, down, back, forward, left, right, home } cursor_movement;
typedef enum { load, disp, call } operating_mode_def;
typedef enum { tty, gui } display_mode_def;
typedef enum { mixed, fixed } font_mode_def;

typedef struct context_descr {

    u_addr	*data_base;
    BOOL	insert_mode;
    BOOL	ignore_status;
    char	chr;
    char	old_string[max_column+1];
    field_descr *old_field, *next_field;
    field_descr saved_field;

    struct context_descr    *last;
    } context;

/* macros for TCL-implementation */

typedef struct {
    char name[30];
    char value;
    } convert_key;

#ifndef _WIN32
#define FIXED_FONT "{courier 14}"
#define FIXED_FONT_SMALL "{courier 12}"
#define FONT "{times 12}"
#else
#define FIXED_FONT "{courier 12}"
#define FIXED_FONT_SMALL "{courier 10}"
#define FONT "{times 12}"
#endif

#define FG "black"
#define FG_BOLD "darkred"
#define FG_REVERSE "black"
#define BG_REVERSE "grey80"
#define FG_BOLD_REVERSE "blue"
#define BG_BOLD_REVERSE "grey60"

/* size of standard window */
#define WIDTH "82"
#define HEIGHT "21"
#define PADY "4"

/* events */
#define NONE 0
#define KEYPRESSED 1
#define BUTTONPRESSED 2
#define SIGNALEVENT 3


/* global variables */

static display_mode_def display_mode = tty;

#ifdef TCL
static font_mode_def font_mode = fixed;
static BOOL menu_built = FALSE;
static char command[buffer_size];
#endif

static context *saved_context = NIL;

static u_addr *data_base;
static BOOL insert_mode;
static BOOL ignore_status;
static u_char chr;
static char old_string[max_column+1];
static field_descr *old_field, *next_field;
static field_descr saved_field;
static BOOL called_from_pascal = FALSE;

static tt_cid *console;
static int ret_stat;

static char name[filspec_length];
static char terminal[filspec_length];

static int record_number, line_number, column_number;

static char buffer[buffer_size];

static BOOL disp_status[max_row] = { 
	TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, 
	TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, 
#ifdef _WIN32
	TRUE, TRUE, TRUE, TRUE, TRUE };
#else
	TRUE, TRUE, TRUE, TRUE };
#endif

static BOOL debug_mode = FALSE;

static char *field_type[] = { "integer", "real", "ascii", "list", "none" };

static char *input_mode[] = { "overstrike", "insert ch" };

static operating_mode_def operating_mode = load;

static menu_descr *first_menu = NIL, *menu = NIL;
static BOOL return_from_submenu;

static int cur_line, cur_column;

static menu_descr  *prev_menu;


static char	    ch;
static int	    read_index;
static FILE	    *menu_file, *data_file;
static int	    repeat_count;
static int	    field_line, field_column, 
		    field_width, field_offset;

static u_addr	    *menu_dest;
static int	    cursor_pos;


#ifdef TCL

/*
 * functions for TCL implementation
 */

static Tcl_Interp *interp;	/* Interpreter for this application. */

static
void init_tk(void)
{
#if TK_MAJOR_VERSION>=8
    Tcl_FindExecutable("csh");	/* TODO */
#endif
    interp = Tcl_CreateInterp();

    if (Tcl_Init(interp) == TCL_ERROR || Tk_Init(interp) == TCL_ERROR)	{
	tt_fprintf(stderr, "formdrv: Tcl/Tk initialization failed\n");
    }
}


#define DOTCL1(cmd, arg1) \
		{ \
		    sprintf(command, cmd, arg1); \
		    DOTCL(command); \
		}
#define DOTCL2(cmd, arg1, arg2) \
		{ \
		    sprintf(command, cmd, arg1, arg2); \
		    DOTCL(command); \
		}
#define DOTCL3(cmd, arg1, arg2, arg3) \
		{ \
		    sprintf(command, cmd, arg1, arg2, arg3); \
		    DOTCL(command); \
		}

static
char *DOTCL(char *cmd)

/*
 * execute tcl command	
 */
{
    if (Tcl_Eval(interp, cmd) == TCL_ERROR)
    {
      tt_fprintf(stderr, "formdrv: Tcl error in command \"%s\" (%s)\n", cmd,
	Tcl_GetVar(interp, "errorInfo", TCL_GLOBAL_ONLY));
    }
    return(interp->result);
}


static
void exit_tk(void)
{
#if TK_MAJOR_VERSION>=8
    Tcl_Finalize();
#endif
    Tcl_DeleteInterp(interp);
    menu_built = FALSE;
}


static
void display_error(char *errormsg)

/*
 * open window to display error-message
 */

{
    DOTCL1("tk_messageBox -icon error -message {%s} -title Error "
		     "-type ok -parent $w", errormsg);
}

#endif


static
void connect (char *terminal)

/*
 * connect terminal
 */

{
#ifdef TCL
    if (display_mode == gui)
	{
	if (!menu_built)
	    {
	    init_tk();
	    DOTCL("set w .");
	    DOTCL("wm title $w Formdriver");
/* set standard options */
	    if (font_mode == mixed)
		{
		DOTCL("option add *font " FONT);
		DOTCL("option add *Entry*font " FIXED_FONT);
		DOTCL("set testfont " FIXED_FONT);
		}
	    else
		{
		DOTCL("option add *font " FIXED_FONT_SMALL);
		DOTCL("option add *Entry*font " FIXED_FONT_SMALL);
		DOTCL("set testfont " FIXED_FONT_SMALL);
		}
	    DOTCL("option add *fg " FG);
	    DOTCL("option add *Button*padX 1");
	    DOTCL("option add *Button*padY 1");
	    DOTCL("option add *Menubutton*padX 1");
	    DOTCL("option add *Menubutton*padY 1");
/* get real fontsize */
	    DOTCL("label .a -text X -font $testfont");
	    DOTCL("set xfactor [winfo reqwidth .a]");
	    DOTCL("set yfactor [winfo reqheight .a]");
	    DOTCL("destroy .a");
	    DOTCL("label .a -text XX -font $testfont");
	    DOTCL("set xfactor [expr [winfo reqwidth .a]-$xfactor]");
	    DOTCL("destroy .a");
	    DOTCL("incr yfactor " PADY);
/* init window size */
	    DOTCL("set winwidth 0");
	    DOTCL("set winheight 0");
/* proc for debugoutputs */
	    DOTCL("proc msg {txt} {tk_messageBox -type ok -message $txt}");
/* proc for widgetname calculation */
	    DOTCL("proc wname {x {y \"\"} } {"
		  "  global w;"
	  	  "  if {$y != \"\"} {"
	  	  "    set body \"${x}_$y\""
		  "  } else {"
		  "    set body $x"
		  "  };"
		  "  if {$w == \".\"} {"
		  "    return \".$body\""
		  "  } else {"
		  "    return \"$w.$body\""
		  "  }"
		  "}");
/* default bindings */
	    DOTCL("bind all <Key-F10> {}");
	    DOTCL1("bind $w <Any-Key> {set key %%K; set event %d}", KEYPRESSED);
	    DOTCL("bind Entry <Delete> {"
		  "  if {[%W select present]} {"
		  "    %W delete sel.first sel.last"
		  "  } else {"
		  "    set index [%W index insert];"
		  "    if {$index != 0} {"
		  "	  %W delete [expr $index - 1]"
		  "    }"
		  "  }"
		  "}");
	    DOTCL("bind Entry <Return> {tkTabToWindow [tk_focusNext %W]}");
	    DOTCL("bind Button <Return> {tkTabToWindow [tk_focusNext %W]}");
	    DOTCL("bind Entry <KP_Enter> {tkTabToWindow [tk_focusNext %W]}");
	    DOTCL("bind Button <KP_Enter> {tkTabToWindow [tk_focusNext %W]}");
/* init some vars */
	    DOTCL("set setfocus \"\"");
	    }
	}
    else
	{
#endif
	console = tt_connect(terminal, NIL);
#ifdef TCL
	}
#endif
}



static
void disconnect (void)

/*
 * disconnect terminal
 */

{
#ifdef TCL
    if (display_mode == tty)
	{
#endif
	tt_disconnect (console, NIL);
#ifdef TCL
	}
#endif
}



static
void clear_display (void)

/*
 * clear display
 */

{
    if (display_mode == tty)
	tt_clear_home (console, NIL);
}



static
void ring_bell (void)

/*
 * ring terminal bell
 */

{
    if (display_mode == tty)
	tt_ring_bell (console, NIL);
}



static
void flush_buffer (void)

/*
 * flush output buffer
 */

{
    if (display_mode == tty)
	tt_flush (console, NIL);
}


#ifdef TCL

static
char *string_trim(char *dest, char *source)

/*
 * crop spaces (& tabs) and copy string
 */
{
    int pos1, pos2;
    char *cropchars = " 	";

    if ((pos1=strspn(source, cropchars)) < (pos2=strlen(source)))
    {
	while (strchr(cropchars, source[--pos2]));
	strncpy(dest, source+pos1, pos2-pos1+1);
	dest[pos2-pos1+1] = '\0';
    }
    else
	*dest = '\0';

    return(dest);
}

#endif


static
void update_field (void)

/*
 * update field in communication region
 */

{
    void *dest;

    if (data_base != NIL)
	{
	dest = (void *)((u_addr)data_base + menu->field->offset);

	switch (menu->field->type) {

	    case integer :
		{ int_type *pi = &(menu->field->field_type.i);
		if ((pi->i_min <= pi->i_value) && (pi->i_value <= pi->i_max))
		    *(int *)dest = pi->i_value;
		break;
		}

	    case real :
		{ real_type *pr = &(menu->field->field_type.r);
		if ((pr->r_min <= pr->r_value) && (pr->r_value <= pr->r_max))
		    *(float *)dest = pr->r_value;
		break;
		}

	    case ascii :
		if (called_from_pascal)
		    { varying_string *str = (varying_string *)dest;

		    str->len = strlen(menu->field->data.str);
		    strncpy (str->body, menu->field->data.str, str->len);
		    }
		else
		    strcpy ((char *)dest, menu->field->data.str);
		break;

	    case list :
		*(int *)dest = menu->field->field_type.l.item->order;
		break;

	    case none :
		break;
	    }
	} 
}


#ifdef TCL

static
void copy_to_tcl (int *status)

/*
 * copy data from internal data structure to tcl
 */

{
    old_field = menu->field;
    menu->field = menu->first_field;

    while (menu->field != NIL && odd(*status))
	{
	DOTCL2("set widget [wname %d %d]", menu->field->data.column,
					   menu->field->data.line);
	switch (menu->field->type) {
	    case integer : 
		if (menu->field->field_type.i.i_value)
		{
		    DOTCL1("set textvar($widget) %d",
			    menu->field->field_type.i.i_value);
		}
		break;
	    case real : 
		if (menu->field->field_type.r.r_value != 0)
		{
		    DOTCL1("set textvar($widget) %g",
			    menu->field->field_type.r.r_value);
		}
		break;
	    case ascii :
		DOTCL1("set textvar($widget) {%s}", menu->field->data.str);
		break;
	    case list :
		break;
	    case none :
		break;
	    }

	menu->field = menu->field->next;
	}

    menu->field = old_field;
}



static
void copy_from_tcl (int *status)

/*
 * copy data from tcl to internal data structure
 */

{
    char buffer2[buffer_size];

    old_field = menu->field;
    menu->field = menu->first_field;

    while (menu->field != NIL && odd(*status))
	{
        if (menu->field->type != none)
	    {
	    DOTCL2("set widget [wname %d %d]", menu->field->data.column,
					   menu->field->data.line);
	    strcpy(buffer, DOTCL("string trim $textvar($widget)"));
	    if (strlen(buffer) > 0)
		switch (menu->field->type) {
		    case integer : 
			menu->field->field_type.i.i_value = 
			    str_integer(buffer, status);
		        if (!odd(*status))
			    {
			    sprintf(buffer2, "Content of Field \"%s\" isn't a "
					"valid integer!\n",
					 menu->field->name->str);
			    display_error(buffer2);
			    DOTCL("tkTabToWindow $widget");
			    }
		        break;
		    case real : 
			menu->field->field_type.r.r_value = 
			    str_real(buffer, status);
			if (!odd(*status))
			    {
			    sprintf(buffer2, "Content of Field \"%s\" isn't a "
					 "valid floating point expression!\n",
					 menu->field->name->str);
			    display_error(buffer2);
			    DOTCL("tkTabToWindow $widget");
			    }
			break;
		    case ascii :
			break;
		    case list :
			menu->field->field_type.l.item = 
				menu->field->field_type.l.first_item;
		    
		    /* Lookup - lookup an identifier of an 
		     * enumerated type */
			{ item_descr *item = menu->field->field_type.l.item;
			*status = fdv__invsynenu;
			while ((item != NIL) && (!odd(*status)))
			    {
			    string_trim(buffer2, item->name);
			    if (strcmp(buffer2, buffer) == 0)
				{
				strcpy(buffer, item->name);
				*status = fdv__normal;
				}
			    else
				item = item->next;
			    }
			menu->field->field_type.l.item = item;
			}

			if (!odd(*status))
			    {
			    menu->field->field_type.l.item =	
				menu->field->field_type.l.first_item;
			    }
			break;
		    }
	    }

	if (odd(*status))
	    {
	    strcpy (menu->field->data.str, buffer);
	    update_field();
	    }

	menu->field = menu->field->next;
	}

    menu->field = old_field;
}

#endif


static
void get_key (u_char *ch, int time)

/*
 * get key from terminal
 */

{
    int status = fdv__normal;
#ifdef TCL
    char key_name[30];
    convert_key key_table[] = {
	{"Tab", 	tab},
	{"Return",	do_key},
	{"KP_F1",	pf1},
	{"KP_F2",	pf2},
	{"KP_F3",	pf3},
	{"KP_F4",	pf4},
	{"F1",		f1},
	{"F2",		f2},
	{"F3",		f3},
	{"F4",		f4},
	{"F10", 	f_10},
	{"Up",		uparrow},
	{"Down",	downarrow},
	{"Right",	rightarrow},
	{"Left",	leftarrow},
	{"\0",		'\0'}
    };
    int i, loop, event, button, posx, posy;
    char buttonevents[] = {f1, f2, f3, f4, do_key};
#endif

    if (display_mode == tty)
	{
	flush_buffer ();
	tt_get_key (console, ch, time, &status);
	}
#ifdef TCL
    else
	{
	copy_to_tcl(&status);
	loop = 1;
	while (loop)
	    {
	    DOTCL1("set event %d", NONE);
	    DOTCL("tkwait variable event");
	    loop = 1;
	    sscanf(Tcl_GetVar(interp, "event", TCL_GLOBAL_ONLY), "%d", &event);
	    switch (event) {
		case BUTTONPRESSED:
		    {
		    sscanf(Tcl_GetVar(interp, "button", TCL_GLOBAL_ONLY), "%d",
				      &button);
		    *ch = buttonevents[button-1];
		    if (*ch == do_key)
			{
			sscanf(Tcl_GetVar(interp, "posx", TCL_GLOBAL_ONLY),
					"%d", &posx);
			sscanf(Tcl_GetVar(interp, "posy", TCL_GLOBAL_ONLY),
					"%d", &posy);
			menu->field = menu->first_field;
			while (menu->field != NIL &&
				(posy != menu->field->name->line ||
				 posx != menu->field->name->column))
			    {
			    menu->field = menu->field->next;
			    }
			}
		    if (menu->field == NIL)
			{
			tt_fprintf(stderr,
			    "formdrv: can't find selected field\n");
			menu->field = menu->first_field;
			}
		    loop = 0;
		    break;
		    }
		case KEYPRESSED:
		    {
		    strcpy(key_name, Tcl_GetVar(interp, "key",
						TCL_GLOBAL_ONLY));
		    for (i = 0;
			 *key_table[i].name != '\0' &&
				       strcmp(key_table[i].name, key_name);
			 i++);
		    loop = ((*ch = key_table[i].value) == '\0');
		    break;
		    }
		case SIGNALEVENT:
		    {
		    break;
		    }
		}
	    if (!loop)
		{
		copy_from_tcl(&status);
		if (!odd(status))
		    {
		    status = fdv__normal;
		    loop = 1;
		    }
		}
	    }
	}
#endif
}



static
void cancel_read (void)

/*
 * cancel read request
 */

{
    if (display_mode == tty)
	tt_cancel (console, "", NIL);
}



static
void set_up_terminal (void)

/*
 * set up terminal
 */

{
    if (display_mode == tty)
	tt_set_terminal (console, mode_application, TRUE, 21, 23, NIL);
}



static
void reset_terminal (void)

/*
 * reset terminal
 */

{
    if (display_mode == tty)
	tt_set_terminal (console, mode_numeric, FALSE, 0, 0, NIL);
}



static
char *symbol (char *str, char ch)

/*
 * return graphic symbol
 */

{
#ifdef DECterm
    sprintf (str, "%c%c%c", tt_k_shift_out, ch, tt_k_shift_in);
#else
    sprintf (str, "%c", ch);
#endif
    return (str);
}



static
void clear (int line)

/*
 * clear specified line
 */

{
    if (display_mode == tty && disp_status[line-1])
	{
	tt_clear_line (console, line, NIL);
	disp_status[line-1] = FALSE;
	}
}



static
void clear_menu (void)

/*
 * clear menu
 */

{
    int line;

#ifdef TCL
    if (display_mode == gui)
	{
	if (menu_built)
	    {
	    DOTCL("foreach widget [winfo children .] {destroy $widget}");
	    DOTCL("catch {unset textvar}");
	    DOTCL("set setfocus \"\"");
	    }
	}
    else
#endif
	for (line = menu->first_line; line <= menu->last_line; line++)
	    {
	    clear (line);
	    disp_status[line-1] = FALSE;
	    }
}



#ifdef TCL

static
void build_widget (widget_format format, char *text, int line_nr, int column_nr,
    tt_attributes attributes, int buttonevent)

/*
 * define and place tcl-widget
 */

{
    char parms[256] = "\0";
    char buttontext[30], buttoncommand[256];
    int textlen, nobutton;
#ifdef DECterm
    char tmpbuf[4];
#endif

/* remove diamond/shift-out characters */

#ifdef DECterm
    sprintf (tmpbuf, "%c%c%c", tt_k_shift_out, diamond, tt_k_shift_in);
    if (!strcmp(text+strlen(text)-3, tmpbuf)) text[strlen(text)-3] = '\0';
#else
    if (text[strlen(text)-1] == diamond) text[strlen(text)-1] = '\0';
#endif

/* set widget name */
    DOTCL2("set widget [wname %d %d]", column_nr, line_nr);
    if (strcmp(DOTCL("winfo exists $widget"), "1"))
	{
/* widget does not exist */

/* set colors */
	if (attributes & attribute_bold)
	    if (attributes & attribute_reverse)
		{
		strcpy(parms, "-fg \"" FG_BOLD_REVERSE "\"");
		sprintf(parms, "%s -bg \"" BG_BOLD_REVERSE "\"", parms);
		}
	    else
		strcpy(parms, "-fg \"" FG_BOLD "\"");
	else if (attributes & attribute_reverse)
	    {
	    strcpy(parms, "-fg \"" FG_REVERSE "\"");
	    sprintf(parms, "%s -bg \"" BG_REVERSE "\"", parms);
	    }

	textlen = strlen(text);

	DOTCL("set yoffset 0");
	switch(format) {
	    case button:
		{
		textlen++;
		/* TODO */
		if (0 && menu->max_button_width > 0 && 
			(buttonevent < 1 || buttonevent > 4))
		    textlen = menu->max_button_width;
		DOTCL1("set buttontext [string trim {%s}]", text);
		sprintf(command,
		    "button $widget -text $buttontext %s"
	    	    " -command {set button %d; set event %d;"
		    "           set posx %d; set posy %d}",
			parms, buttonevent, BUTTONPRESSED, column_nr, line_nr);
		DOTCL(command);
		break;
		}	    
	    case menubutton:
		{ item_descr *pitem = menu->field->field_type.l.first_item;
		textlen++;
		DOTCL("menubutton $widget -menu $widget.menu"
			     " -relief ridge -textvariable textvar($widget)"
			     " -anchor w");
		DOTCL("menu $widget.menu -tearoff no");
		DOTCL1("set textvar($widget) {%s}", text);
		while (pitem != NIL)
		    {
		    DOTCL2("$widget.menu add command -label {%s}"
			     " -command \"set textvar($widget) {%s}\"",
				 pitem->name, pitem->name);
		    pitem = pitem->next;
		    }
		break;
		}
	    case fieldname:
		{
		if (font_mode == mixed)
		    textlen = menu->field->data.column -
						menu->field->name->column;
		}
	    case label:
		{
		if (strspn(text, "-") != strlen(text))
		    {
		    DOTCL2("label $widget -text {%s} %s", text, parms);
		    }
		else
		    {
		    DOTCL("frame $widget -height 3 -bd 2 -relief sunken");
		    DOTCL("set yoffset [expr $yfactor/2]");
		    }
		break;
		}	    
	    case entry:
		{
		DOTCL1("set textvar($widget) [string trim {%s}]", text);
		if (buttonevent)
		    {
		    nobutton = 0;
		    switch (buttonevent) {
			case LOADFILE:
			    {
			    strcpy(buttontext, "SEARCH");
			    strcpy(buttoncommand,
				"set textvar($widget) \\[tk_getOpenFile\\]");
			    break;
			    }
			case SAVEFILE:
			    {
			    strcpy(buttontext, "SEARCH");
			    strcpy(buttoncommand,
				"set textvar($widget) \\[tk_getSaveFile\\]");
			    break;
			    }
			default:
			    {
			    nobutton = 1;
			    }
			}
		    if (!nobutton)
			{
			textlen -= strlen(buttontext)+2;
			DOTCL2("button ${widget}_button -text {%s}"
				" -command \"%s\"", buttontext, buttoncommand);
			}
			DOTCL3("place ${widget}_button"
				" -x [set xpos [expr int(%f*$xfactor)]]"
				" -y [set ypos [expr %d*$yfactor-1]]"
				" -width [expr %d*$xfactor]",
			column_nr+textlen+1.5, line_nr, strlen(buttontext));
		    }
		DOTCL1("entry $widget -textvariable textvar($widget) -width %d",
								      textlen);
		DOTCL("if {$setfocus == \"\"} {set setfocus $widget}");
		break;
		}	    
	    }

	DOTCL3("place $widget -x [set xpos [expr %d*$xfactor]]"
			 " -y [set ypos [expr %d*$yfactor+$yoffset]]"
			 " -width [expr %d*$xfactor]",
			 column_nr, line_nr, textlen);
	DOTCL("if {[incr xpos [winfo reqwidth $widget]] > $winwidth} {set winwidth $xpos}");
	DOTCL("if {[incr ypos [winfo reqheight $widget]] > $winheight} {set winheight $ypos}");
	}
    else /* widget already exists */
	{
	switch (format) {
	    case entry:
	    case menubutton:
		DOTCL1("set textvariable($widget) \"%s\"", text);
		break;
	    case label:
	    case button:
		if (attributes & attribute_bold)
		    {
		    DOTCL("focus $widget");
		    break;
		    }
		else
                    {
		    if (debug_mode)
			tt_fprintf(stderr,
			    "Button \"%s\" (%d_%d) already exists!\n", text,
			    column_nr, line_nr);
                    }
		break;
	    }
	}
}

#endif


static
void display_buffer (tt_attributes attributes, widget_format format,
    int buttonevent)

/*
 * display a buffer using the specified character attributes
 */

{
    if (strlen(buffer) > 0)
	{
	if (display_mode == tty)
	    tt_put_chars (console, buffer, line_number, column_number,
			  attributes, NIL);
#ifdef TCL
	else
	    {
	    build_widget(format, buffer, line_number, column_number,
			 attributes, buttonevent);
	    }
#endif
	disp_status[line_number-1] = TRUE;
	}
}



static
void read_data (menu_descr *menu, int *field_offset, void *com_reg)

/*
 * read data of specified menu
 */

{
    void *dest;
    int structure_alignment = 0;

    menu->field = menu->first_field;

    while (menu->field != NIL)
	{
	switch (menu->field->type) {

	    case integer :
		if (int_align > structure_alignment)
		    structure_alignment = int_align;
		break;

	    case real :
		if (float_align > structure_alignment)
		    structure_alignment = float_align;
		break;

	    case list :
		if (enum_align > structure_alignment)
		    structure_alignment = enum_align;
		break;

	    case ascii :
		if (called_from_pascal)
		    {
		    if (int16_align > structure_alignment)
			structure_alignment = int16_align;
		    }
		break;
	    }

	menu->field = menu->field->next;
	}

    if (structure_alignment != 0)
	*field_offset = align(*field_offset, structure_alignment);

    menu->db = (u_addr *)((u_addr)com_reg + *field_offset);
    menu->field = menu->first_field;

    while (menu->field != NIL)
	{
	switch (menu->field->type) {

	    case integer :
		*field_offset = align(*field_offset, int_align);
		break;

	    case real :
		*field_offset = align(*field_offset, float_align);
		break;

	    case list :
		*field_offset = align(*field_offset, enum_align);
		break;

	    case ascii :
		if (called_from_pascal)
		    *field_offset = align(*field_offset, int16_align);
		break;
	    }

	dest = (void *)((u_addr)com_reg + *field_offset);

	switch (menu->field->type) {

	    case integer :
		{ int_type *pi = &(menu->field->field_type.i);

		if ((pi->i_min <= pi->i_value) && (pi->i_value <= pi->i_max))
		    *(int *)dest = pi->i_value;

		*field_offset = *field_offset + sizeof(int);
		break;
		}

	    case real :
		{ real_type *pr = &(menu->field->field_type.r);

		if ((pr->r_min <= pr->r_value) && (pr->r_value <= pr->r_max))
		    *(float *)dest = pr->r_value;

		*field_offset = *field_offset + sizeof(float);
		break;
		}

	    case ascii :
		if (called_from_pascal)
		    { varying_string *str = (varying_string *)dest;

		    str->len = strlen(menu->field->data.str);
		    strncpy (str->body, menu->field->data.str, str->len);

		    *field_offset = *field_offset + sizeof(unsigned short) +
			menu->field->width;
		    }
		else
		    {
		    strcpy ((char *)dest, menu->field->data.str);
		    *field_offset = *field_offset + menu->field->width + 1;
		    }
		break;

	    case list :
		*(int *)dest = menu->field->field_type.l.item->order;
		*field_offset = *field_offset + sizeof(int);
		break;
	    }

	menu->field = menu->field->next;
	}

    menu->node = menu->first_node;

    while (menu->node != NIL)
	{
	read_data (menu->node->sub_menu, field_offset, com_reg);
	menu->node = menu->node->next;
	}
}



static
void read_menu (void *com_reg)

/*
 * read menu into communication region(s)
 */

{
    menu_descr *menu;
    int field_offset = 0;

    if (com_reg != NIL)
	{
	menu = first_menu;
	read_data (menu, &field_offset, com_reg);
	}
}



static
void initialize (menu_descr *menu)

/*
 * initialize the internal data structure
 */

{
    field_descr *next_field;
    item_descr	*next_item;
    text_descr	*next_field_name;
    text_descr	*next_text;

    if (menu != NIL)
	{
	menu->node = menu->first_node;

	while (menu->node != NIL)
	    {
	    initialize (menu->node->sub_menu);
	    menu->node = menu->node->next;
	    }

	menu->node = NIL;
	menu->first_node = NIL;

	menu->field = menu->first_field;

	while (menu->field != NIL)
	    {
	    if (menu->field->type == list)
		{ list_type *pl = &(menu->field->field_type.l);

		pl->item = pl->first_item;

		while (pl->item != NIL)
		    {
		    next_item = pl->item->next;
		    free (pl->item);
		    pl->item = next_item;
		    }

		pl->first_item = NIL;
		pl->item = NIL;
		}

	    next_field = menu->field->next;
	    free (menu->field);
	    menu->field = next_field;
	    }

	menu->field = NIL;
	menu->first_field = NIL;

	menu->field_name = menu->first_field_name;

	while (menu->field_name != NIL)
	    {
	    next_field_name = menu->field_name->next;
	    free (menu->field_name);
	    menu->field_name = next_field_name;
	    }

	menu->field_name = NIL;
	menu->first_field_name = NIL;

	menu->text = menu->first_text;

	while (menu->text != NIL)
	    {
	    next_text = menu->text->next;
	    free (menu->text);
	    menu->text = next_text;
	    }

	menu->text = NIL;
	menu->first_text = NIL;

	if (menu->legend != NIL)
	    free (menu->legend);

	free (menu);
	menu = NIL;
	}
}



static
void new_menu (void)

/*
 * allocate new menu
 */

{
    menu = (menu_descr *)malloc (sizeof (menu_descr));

    menu->prev = prev_menu;
    menu->first_node = NIL;
    menu->node = NIL;

    menu->first_line = 0;
    menu->last_line = 1;

    menu->first_field = NIL;
    menu->field = NIL;

    menu->first_field_name = NIL;
    menu->field_name = NIL;

    menu->first_text = NIL;
    menu->text = NIL;

    menu->legend = NIL;

    menu->db = NIL;

    menu->max_button_width = 0;
}



static
void readc (void)

/*
 * get next character from buffer
 */

{

    if (read_index < strlen(buffer))
	{
	ch = buffer[read_index];
	read_index++;

	if (ch == tab)
	    column_number = column_number + 8 - (column_number % 8);
	else
	    column_number++;

	if ((read_index == strlen(buffer)) && (ch == cont_char))
	    {
	    if (!feof (menu_file))
		{
		fgets (buffer, buffer_size, menu_file);
		if (*buffer) buffer[strlen(buffer)-1] = '\0';
		
		record_number++;
		column_number = 0;

		read_index = 0;
		readc ();

		while (ch == space || ch == tab)
		    readc ();
		}
	    else
		ch = terminator;
	    }
	}
    else
	ch = terminator;
}



static
int get_int(void)

/*
 * extract an integer
 */

{
    int value;

    value = 0;

    while (isdigit(ch))
	{
	value = value*10 + (int)(ch) - (int)'0';
	readc ();
	}

    return (value);
}



static
int get_efn(void)

/*
 * get event flag number
 */

{
    if (ch == '^') 
	{
	readc ();
	return (get_int());
	}
    else
	return (0);
}



static
void new_field_descr(void)

/*
 * allocate a new field descriptor
 */

{
    field_descr *prev_field;

    prev_field = menu->field;
    menu->field = (field_descr *)malloc(sizeof(field_descr));

    if (prev_field == NIL)
	menu->first_field = menu->field;
    else
	prev_field->next = menu->field;

    menu->field->prev = prev_field;
    menu->field->next = NIL;

    menu->field->name = menu->field_name;

    strcpy (menu->field->data.str, "");
    menu->field->data.line = field_line;

    menu->field->data.column = field_column;

    menu->field->width = 0;
    menu->field->offset = 0;
    menu->field->efn = 0;
    menu->field->monitor = FALSE;
}



static
void input_field (int *status)

/*
 * process an input field descriptor
 */

{
    type_descr field_type;
    BOOL field_range;
    int field_efn;
    BOOL monitor_field;
    int count;
    int min_i, max_i;
    float min_r, max_r;

    switch (ch) {
	case 'I' :
	case 'i' :
	    field_type = integer;
	    break;
	case 'F' :
	case 'f' :
	    field_type = real;
	    break;
	case 'A' :
	case 'a' :
	    field_type = ascii;
	    break;
	}

    readc ();
    field_width = get_int();

    if (field_type == integer || field_type == real)
	{
 
	/* Range_spec - process range specification */

	char chars[max_column+1];

	if (ch == '[')
	    {

	    /* Extract_number - extract a number */

	    strcpy (chars, "");
	    readc ();

	    while ((ch == plus) || (ch == minus) || 
		    isdigit(ch) || (ch == period) || 
		  (ch == 'E'))
		{
		strncat(chars, &ch, 1);
		readc ();
		}

	    if (ch == ',')
		{
		switch (field_type) {
		    case integer :
			min_i = str_integer(chars, status);
			break;
		    case real :
			min_r = str_real(chars, status);
			break;
		    }

		if (odd(*status))
		    {

		    /* Extract_number - extract a number */

		    strcpy (chars, "");
		    readc ();

		    while ((ch == plus) || (ch == minus) || 
			isdigit(ch) || (ch == period) || 
			(ch == 'E'))
			{
			strncat (chars, &ch, 1);
			readc ();
			}

		    if (ch == ']')
			{
			switch (field_type) {
			    case integer :
				max_i = str_integer(chars, status);
				break;
			    case real :
				max_r = str_real(chars, status);
				break;
			    }

			if (odd(*status))
			    {
			    field_range = TRUE;
			    *status = fdv__normal;
			    }

			readc ();
			}
		    else
			/* invalid menu field descriptor */
			*status = fdv__invmenu; 

		    }
		}
	    else
		*status = fdv__invmenu; /* invalid menu field descriptor */

	    }
	else
	    {
	    field_range = FALSE;

	    switch (field_type) {
		case integer :
		    min_i = min_integer;
		    max_i = max_integer;
		    break;
		case real :
		    min_r = min_real;
		    max_r = max_real;
		    break;
		}
	    }
	}

    if (odd(*status))
	{
	field_efn = get_efn();

	monitor_field = (ch == dollar);
	if (monitor_field)
	    readc ();

	for (count = 1; count <= repeat_count; count++)
	    {
	    new_field_descr ();

	    menu->field->width = field_width;
	    menu->field->efn = field_efn;
	    menu->field->monitor = monitor_field;

	    menu->field->type = field_type;

	    switch (field_type) {
		case integer :
		    { int_type *pi = &(menu->field->field_type.i);

		    field_offset = align(field_offset, int_align);
		    menu->field->offset = field_offset;

		    field_offset = field_offset + sizeof(int);

		    if (min_i > 0)
			pi->i_value = min_i;
		    else if (max_i < 0)
			pi->i_value = max_i;
		    else
			pi->i_value = 0;

		    pi->i_range = field_range;
		    pi->i_min = min_i;
		    pi->i_max = max_i;
		    break;
		    }

		case real :
		    { real_type *pr = &(menu->field->field_type.r);

		    field_offset = align(field_offset, float_align);
		    menu->field->offset = field_offset;

		    field_offset = field_offset + sizeof(float);

		    if (min_r > 0)
			pr->r_value = min_r;
		    else if (max_r < 0)
			pr->r_value = max_r;
		    else
			pr->r_value = 0;

		    pr->r_range = field_range;
		    pr->r_min = min_r;
		    pr->r_max = max_r;
		    break;
		    }

		case ascii :

		    if (called_from_pascal)
			field_offset = align(field_offset, int16_align);
		    else
			field_offset = align(field_offset, char_align);

		    menu->field->offset = field_offset;

		    if (called_from_pascal)
			field_offset = field_offset + sizeof(unsigned short) +
			    menu->field->width;
		    else
			field_offset = field_offset + menu->field->width + 1;
		    break;
		}

	    if (field_efn > 0)
		field_efn++;

	    if (count < repeat_count)
		field_column = field_column + field_width + 2;
	    }
	}
}



static
void enumeration_field (int *status)

/*
 * process an enumeration field
 */

{
    char	item_name[max_name_length];
    int 	item_order;
    item_descr	*prev_item;

    new_field_descr ();

    field_width = 0;
    menu->field->type = list;

    { list_type *pl = &(menu->field->field_type.l);

    if (ch == '[')
	pl->format = mapped;
    else
	pl->format = overlaid;

    pl->item = NIL;
    pl->first_item = NIL;

    item_order = 0;

    while (!((ch == ']') || (ch == ')') || 
	    (ch == terminator)))
	{
	readc ();

	/* Get_item - extract an item */

	strcpy (item_name, "");

	while (!((ch == '^') || (ch == ',') || 
		 (ch == ']') || (ch == ')') ||
		 (ch == terminator)))
	    {
	    strncat (item_name, &ch, 1);
	    readc ();
	    }

	if (strlen(item_name) > field_width)
	    field_width = strlen(item_name);

	/* New_item - allocate new item */

	prev_item = pl->item;
	pl->item = (item_descr *)malloc(sizeof(item_descr));

	if (prev_item != NIL)
	    prev_item->next = pl->item;
	else
	    pl->first_item = pl->item;

	pl->item->prev = prev_item;
	pl->item->next = NIL;
	strcpy (pl->item->name, item_name);
	pl->item->order = item_order;
	pl->item->efn = get_efn();
	item_order++;
	}

    if ((ch == ')') || (ch == ']'))
	{
	menu->field->width = field_width;

	field_offset = align(field_offset, enum_align);
	menu->field->offset = field_offset;

	field_offset = field_offset + sizeof(int);

	readc ();
	menu->field->efn = get_efn();

	menu->field->monitor = (ch == dollar);
	if (menu->field->monitor)
	    readc ();

	pl->item = pl->first_item;
	strcpy (menu->field->data.str, pl->item->name);
	}
    else
	*status = fdv__invmenu; /* invalid menu field descriptor */
    }
}



static
void control_field (void)

/*
 * process a control field descriptor
 */

{
    int len;

    new_field_descr();

    menu->field->type = none;
    menu->field->efn = get_efn();

    len = strlen(menu->field->name->str);
    if (len > menu->max_button_width)
 	menu->max_button_width = len;
}



static
char *center (char *result, char *chars, int size)

/*
 * center string
 */

{
    int prefix_length;
    char str[255];

    if (strlen(chars) > size) {
	chars[size-1] = '\0';
	strcat (chars, symbol(str, diamond));

	strcpy (result, chars);
	}
    else
	{
	prefix_length = (size - (int)strlen(chars)) / 2;
	strcpy (str, "");

	str_pad (str, space, prefix_length);
	strcat (str, chars);

	str_pad (str, space, size);
	strcpy (result, str);
	}

    return (result);
}



static
void legend_specification (int *status)

/*
 * parse a legend specification
 */

{
    char chars[max_column+1];
    int count;
    char str[255];

    menu->legend = (legend_descr *)malloc(5 * sizeof(legend_descr));

    /* Get_item - get an item */

    strcpy (chars, "");

    if (ch != ')')
	{
	readc ();

	while (!((ch == ',') || (ch == ')') ||
		 (ch == terminator)))
	    {
	    strncat (chars, &ch, 1);
	    readc ();
	    }
	}

    menu->legend[0]->line = 1;
    menu->legend[0]->column = 1;
    strcpy (menu->legend[0]->str, center(str, chars, max_column));

    for (count = 1; count <= 4; count++)
	{

	/* Get_item - get an item */

	strcpy (chars, "");

	if (ch != ')')
	    {
	    readc ();
	    while (!((ch == ',') || (ch == ')') ||
		    (ch == terminator)))
		{
		strncat (chars, &ch, 1);
		readc ();
		}
	    }

	menu->legend[count]->line = max_line;
	menu->legend[count]->column = 2+(count-1)*(legend_length+2);
	strcpy (menu->legend[count]->str, center(str, chars, legend_length));
	}

    if (ch == ')')
	readc ();
    else
	*status = fdv__invmenu; /* invalid menu field descriptor */
}



static
void new_node (int *node_generation, int *status)

/*
 * allocate new node
 */

{
    int 	generation;
    int 	node_key;
    node_descr	*prev_node;

    readc ();
    generation = get_int();

    while (generation < *node_generation)
	{
	menu = menu->prev;
       (*node_generation)--;
	}

    if (generation == *node_generation)
	prev_menu = menu->prev;

    else if (generation == *node_generation+1)
	{
	prev_menu = menu;
	*node_generation = generation;
	}
    else
	*status = fdv__invmenu; /* invalid menu field descriptor */

    if (prev_menu != NIL)
	{
	new_menu ();

	prev_node = prev_menu->node;
	prev_menu->node = (node_descr *)malloc(sizeof(node_descr));

	if (prev_node == NIL)
	    {
	    node_key = 1;
	    prev_menu->first_node = prev_menu->node;
	    }
	else 
	    {
	    prev_node->next = prev_menu->node;
	    node_key = prev_node->key + 1;
	    }

	prev_menu->node->next = NIL;
	prev_menu->node->key = node_key;
	prev_menu->node->sub_menu = menu;
	}

    if (ch == '(')
	legend_specification (status);

    line_number = 1;
    field_offset = 0;
}



static
void new_text (void)

/*
 * allocate text
 */

{
    text_descr	*prev_text;

    prev_text = menu->text;

    menu->text = (text_descr *)malloc(sizeof(text_descr));
    if (prev_text == NIL)
	menu->first_text = menu->text;
    else
	prev_text->next = menu->text;

    menu->text->next = NIL;
}



static
void new_field_name (void)

/*
 * allocate field name
 */

{
    text_descr	*prev_field_name;

    prev_field_name = menu->field_name;

    menu->field_name = (text_descr *)malloc(sizeof(text_descr));
    if (prev_field_name == NIL)
	menu->first_field_name = menu->field_name;
    else
	prev_field_name->next = menu->field_name;

    menu->field_name->next = NIL;
}



static
void check_window (int *status)

/*
 * check window limits
 */

{
    if (line_number <= max_line)
	{
	if (menu->first_line == 0)
	    menu->first_line = line_number;

	menu->last_line = line_number;
	}
    else
	*status = fdv__illmenlen; /* menu exceeds 20 lines */
} 



static
void expression (int *status)

/*
 * parse an expression
 */

{
    int count;
    char chars[max_column+1];

    strcpy (chars, "");
    readc ();

    while (!((ch == '>') || (ch == terminator)))
	{
	strncat (chars, &ch, 1);
	readc ();
	}

    if (ch == '>')
	{
	readc ();

	if (ch == terminator)
	    {

	    if (repeat_count * strlen(chars) <= buffer_size)
		{
		strcpy (buffer, "");

		for (count = 1; count <= repeat_count; count++)
		    strcat (buffer, chars);

		read_index = 0;
		readc ();
		column_number = field_column;

		*status = fdv__normal;
		}
	    else
		*status = fdv__strtoolon; /* string is too long */
	    }
	else
	    *status = fdv__invmenu; /* invalid menu field descriptor */
	}
    else
	*status = fdv__invmenu; /* invalid menu field descriptor */
}



static
void parse (int *node_generation, int *status)

/*
 * parse a descriptor record
 */

{
    BOOL repeat_count_defined;
    char text_string[max_column+1];
    int text_line, text_column;

    while (ch == space || ch == tab)
	readc ();

    field_line = line_number;
    field_column = column_number;

    repeat_count = 1;

    if (ch == control_flag)
	{
	readc ();
	repeat_count_defined = isdigit(ch);

	if (repeat_count_defined)
	    repeat_count = get_int();

	switch (ch) {

	    case 'A' :
	    case 'a' :
	    case 'F' :
	    case 'f' :
	    case 'I' :
	    case 'i' :
		check_window (status);
		if (odd(*status))
		    input_field (status);
		break;

	    case '[' :
	    case '(' :
		if (!repeat_count_defined)
		    {
		    check_window (status);
		    if (odd(*status))
			enumeration_field (status);
		    }
		else
		    *status = fdv__invmenu; /* invalid menu field descriptor */
		break;

	    case '^' :
		if (!repeat_count_defined)
		    {
		    check_window (status);
		    if (odd(*status))
			control_field ();
		    }
		else
		    *status = fdv__invmenu; /* invalid menu field descriptor */
		break;

	    case 'N' :
	    case 'n' :
		if (!repeat_count_defined)
		    new_node (node_generation, status);
		else
		    *status = fdv__invmenu; /* invalid menu field descriptor */
		break;

	    case '<' :
		if (repeat_count_defined)
		    expression (status);
		else
		    *status = fdv__invmenu; /* invalid menu field descriptor */
		break;

	    case space :
	    case terminator :
		break;

	    default :
		*status = fdv__invmenu; /* invalid menu field descriptor */
	    }

	if (odd(*status))

	    if (ch == semicolon)
		{
		readc ();
		column_number = field_column + field_width;
		}
	}
    else
	{
	check_window (status);

	if (odd(*status))
	    {
	    strcpy (text_string, "");
	    text_line = line_number;
	    text_column = column_number;

	    while (!((ch == control_flag) || (ch == terminator) ||
		    (ch == backslash)))
		{
		strncat (text_string, &ch, 1);
		readc ();
		}

	    str_remove (text_string, ' ', FALSE);

	    if (strlen(text_string) > 0)

		switch (ch) {

		    case terminator :
		    case backslash :
			new_text ();

			strcpy (menu->text->str, text_string);
			menu->text->line = text_line;
			menu->text->column = text_column;
			break;

		    case control_flag :
			new_field_name ();

			strcpy (menu->field_name->str, text_string);
			menu->field_name->line = text_line;
			menu->field_name->column = text_column;
			break;
		    }

	    if (ch == backslash)
		readc ();
	    }
	}
}



static
void unsave_read_data (menu_descr *menu, int *status)

/*
 * read data from save file
 */

{
    menu_descr saved_menu;

    saved_menu = *menu;

    menu->field = menu->first_field;
    record_number = 0;

    while ((menu->field != NIL) && odd(*status))
	{
	if (menu->field->type != none)
	    {
	    if (!feof (data_file))
		{
		fgets (buffer, buffer_size, data_file);
		if (*buffer) buffer[strlen(buffer)-1] = '\0';
		
		record_number++;

		if (strlen(buffer) > 0)
		    switch (menu->field->type) {
			case integer : 
			    menu->field->field_type.i.i_value = 
				str_integer(buffer, status);
			    break;
			case real : 
			    menu->field->field_type.r.r_value = 
				str_real(buffer, status);
			    break;
			case ascii :
			    break;
			case list :
			    menu->field->field_type.l.item = 
				menu->field->field_type.l.first_item;

			    /* Lookup - lookup an identifier of an 
			     * enumerated type */

			    { item_descr *item = menu->field->field_type.l.item;

			    *status = fdv__invsynenu;

			    while ((item != NIL) && (!odd(*status)))
				{

				if (strcmp(item->name, buffer) == 0)
				    *status = fdv__normal;
				else
				    item = item->next;
				}
			    menu->field->field_type.l.item = item;
			    }

			    if (!odd(*status))
				{
				menu->field->field_type.l.item =	
				    menu->field->field_type.l.first_item;
				}
			    break;
			}

		if (odd(*status))
		    strcpy (menu->field->data.str, buffer);
		}
	    else
		*status = fdv__error;
	    }

	menu->field = menu->field->next;
	}

    menu->node = menu->first_node;

    while ((menu->node != NIL) && odd(*status))
	{
	unsave_read_data (menu->node->sub_menu, status);
	menu->node = menu->node->next;
	}

    if (odd(*status))
	*status = fdv__normal;

    *menu = saved_menu;
}



static
void unsave_menu (int *status, BOOL log_flag)

/*
 * load save-file into internal data structure
 */

{
    char str[buffer_size];

    if (log_flag && display_mode == tty) 
	{
	strcpy (str, "Reading ");
	strcat (str, name);
	fdv_message (str, NIL);
	}

    if (!access(name, 0))
	{
	data_file = fopen(name, "r");

	if (data_file)
	    {
	    rewind (data_file);

	    if (!feof(data_file))
		{
		*status = fdv__normal;

		unsave_read_data (first_menu, status);

		if (!odd(*status))
		    *status = fdv__illformat; /* save file has illegal format */
		}
	    else
		*status = fdv__initfai; /* initialization failed */

	    fclose (data_file);
	    }
	else
	    *status = fdv__openfai; /* save file open failure */
	}

    if (log_flag)
	{
	if (*status == fdv__initfai)
	    fdv_error ("Save-file is empty", NIL);
	else
	    {
	    clear (status_line);
	    flush_buffer ();
	    }
	}
}



void fdv_load (char *menu_name, char *display, void *com_reg, char *save_file,
    int *status)

/*
 * load specified menu into an internal data structure
 */

{
    int stat;
    int node_generation;
    char *env;

    if (operating_mode == load)
	{
	env = (char *)getenv("GLI_GUI");
	if (env != NULL)
	    {
	    display_mode = gui;
#ifdef TCL
	    if (strstr(env, "mixed") != NULL)
		font_mode = mixed;
#endif
	    }

	strcpy (name, menu_name);

	str_parse (menu_name, ".fdv" , FAll, name);

	if (!access(name, 0))
	    {
	    menu_file = fopen(name, "r");

	    if (menu_file) 
		{
		if (strlen(save_file) > 0)
		    str_parse (save_file, ".sav", FAll, name);
		else
		    {
		    str_parse (menu_name, "", FNode | FName, name);
		    strcat (name, ".sav");
		    }

		menu = first_menu;
		if (menu != NIL)
		    initialize (menu);

		prev_menu = NIL;
		new_menu ();
		first_menu = menu;

		record_number = 0;
		line_number = 0;

		node_generation = 0;
		field_offset = 0;

		stat = fdv__normal;

		while ((stat == fdv__normal) && (!feof(menu_file)))
		    {
		    fgets (buffer, buffer_size, menu_file);
		    if (*buffer) buffer[strlen(buffer)-1] = '\0';

		    record_number++;
		    line_number++;
		    column_number = 0;

		    read_index = 0;
		    readc ();

		    while ((ch != terminator) && odd(stat))
			parse (&node_generation, &stat);
		    }

		fclose (menu_file);

		if (odd(stat))
		    {
		    unsave_menu (&stat, FALSE);

		    read_menu (com_reg);
		    menu = first_menu;

		    strcpy (terminal, display);
		    }
		}
	    else
		stat = fdv__filnotfnd; /* menu description file not found */

	    if (!odd(stat))
		{
		menu = first_menu;
		initialize (menu);
		first_menu = NIL;
		}
	    }
	}
    else
	stat = fdv__illcall; /* recursive call */

    if (present(status))
	*status = stat;
    else
	if (!odd(stat))
	    {
	    if ((stat == fdv__invmenu) || (stat == fdv__strtoolon) || 
		(stat == fdv__illmenlen) || (stat == fdv__illformat))
		raise_exception (fdv__parse, 2, record_number, column_number);

	    raise_exception (stat, 0, NULL, NULL);
	    }
}



void fdv_load_c (varying_string *menu_nameP, varying_string *displayP,
    void *com_reg, varying_string *save_fileP, int *status)
{
    char menu_name[255], display[255], save_file[255];

    strncpy (menu_name, menu_nameP->body, menu_nameP->len);
    menu_name[menu_nameP->len] = '\0';

    if (displayP != NULL)
	{
	strncpy (display, displayP->body, displayP->len);
	display[displayP->len] = '\0';
	}
    else
	*display = '\0';

    if (save_fileP != NULL)
	{
	strncpy (save_file, save_fileP->body, save_fileP->len);
	save_file[save_fileP->len] = '\0';
	}
    else
	*save_file = '\0';

    called_from_pascal = TRUE;

    fdv_load (menu_name, display, com_reg, save_file, status);
}



static
void write_data (menu_descr *menu)

/*
 * write data to save file
 */

{
    menu_descr saved_menu;

    saved_menu = *menu;

    menu->field = menu->first_field;

    while (menu->field != NIL)
	{
	if (menu->field->type != none)
	    fprintf (data_file, "%s\n", menu->field->data.str);

	menu->field = menu->field->next;
	}

    menu->node = menu->first_node;

    while (menu->node != NIL)
	{
	write_data (menu->node->sub_menu);
	menu->node = menu->node->next;
	}

    *menu = saved_menu;
}



static
void save_menu (BOOL log_flag)

/*
 * copy internal data structure to the save-file
 */

{
    char str[buffer_size];

    if (log_flag && display_mode == tty) 
	{
	strcpy (str, "Writing ");
	strcat (str, name);
	fdv_message (str, NIL);
	}

    data_file = fopen (name, "w");

    write_data (first_menu);

    fclose (data_file);

    if (log_flag)
	{
	clear (status_line);
	flush_buffer ();
	}
}



static
void display_data (char *old_string)

/*
 * display data
 */

{
    int pos, len;
    BOOL match;
    char str[255];

    switch (menu->field->type) {

	case integer :
	case real :
	case ascii :
	    if (display_mode == tty)
	    {
		pos = 1;
		match = TRUE;
		len = strlen(menu->field->data.str);
		if (strlen(old_string) < len)
		    len = strlen(old_string);

		while (match && (pos < len))
		    {
		    match = (menu->field->data.str[pos-1] == old_string[pos-1]);
		    if (match)
			pos++;
		    }

		len = strlen(old_string) - pos + 1;
		if (strlen(menu->field->data.str) > len)
		    len = strlen(menu->field->data.str);

		strcpy (str, menu->field->data.str);
		str_pad (str, space, menu->field->width);
		strcpy (buffer, "");
		strncat (buffer, &str[pos-1], len);
		line_number = menu->field->data.line;
		column_number = menu->field->data.column+pos-1;

		display_buffer (attribute_reverse, label, 0);
		}
#ifdef TCL
	    else /* display_mode == gui */
		{
		line_number = menu->field->data.line;
		column_number = menu->field->data.column;
		strcpy (buffer, old_string);

		display_buffer (attribute_reverse, entry, 0);
		}
#endif
	    break;

	case list :
	case none :
	    break;
	}
}



static
void display_item (fdv_attributes attributes)

/*
 * display an item using the specified video attributes
 */

{
    line_number = menu->field->data.line;

    { list_type *pl = &(menu->field->field_type.l);
    switch (pl->format) {
	case mapped :
	    strcpy (buffer, pl->item->name);
	    column_number = menu->field->data.column + pl->item->order *
		(menu->field->width + 1);
	    break;
	case overlaid :
	    strcpy (buffer, pl->item->name);
	    str_pad (buffer, space, menu->field->width);
	    column_number = menu->field->data.column;
	    break;
	}
    }

    if (display_mode == tty)
	{
	display_buffer (attributes, label, 0);
	}
}



static
void render (text_descr *field_name)

/*
 * render a field name
 */

{
    if (field_name != NIL)
	{
	strcpy (buffer, field_name->str);
	line_number = field_name->line;
	column_number = field_name->column;

	display_buffer (attribute_bold, fieldname, 0);
	}
}



static
void fade_out (text_descr *field_name)

/*
 * fade out a field name
 */

{
    if (field_name != NIL)
	{
	strcpy (buffer, field_name->str);
	line_number = field_name->line;
	column_number = field_name->column;

	display_buffer (EMPTY_SET, fieldname, 0);
	}
}



static
BOOL monitor (field_descr *field)
{
    if (field != NIL)
	return (field->monitor && !debug_mode);
    else
	return (FALSE);
}



static
void jump (cursor_movement movement)

/*
 * jump to another field
 */

{
    field_descr *old_field, *closest_field;
    int field_line, closest_line, old_line, old_column, dist, minimum;

    old_field = menu->field;

    switch (movement) {
	case up :
	case down :
	    closest_field = NIL;

	    do {

		old_line = menu->field->data.line;
		old_column = menu->field->data.column;

		do {

		    switch (movement) {
			case up :
			    menu->field = menu->field->prev;
			    break;
			case down :
			    menu->field = menu->field->next;
			    break;
			}

		    if (menu->field != NIL)
			field_line = menu->field->data.line;
		  
		} while ((field_line == old_line || monitor(menu->field)) && 
		   (menu->field != NIL));

		if (menu->field != NIL)
		    {
		    closest_line = field_line;
		    closest_field = menu->field;

		    minimum = abs(menu->field->data.column - old_column);

		    do {
			switch (movement) {
			    case up :
				menu->field = menu->field->prev;
				break;
			    case down :
				menu->field = menu->field->next;
				break;
			    }

			if (menu->field != NIL)
			    {
			    field_line = menu->field->data.line;

			    if ((field_line == closest_line) && 
				!monitor(menu->field))
				{
				dist = abs(menu->field->data.column - 
				    old_column);

				if (dist < minimum)
				    {
				    minimum = dist;
				    closest_field = menu->field;
				    }
				}
			    }
		    } while ((menu->field != NIL) && 
			(field_line == closest_line));

		    }

	    } while ((menu->field != NIL) && (closest_field == NIL));

	    menu->field = closest_field;
	    break;

	case back :
	case forward :
	    do
		switch (movement) {
		    case back :
			menu->field = menu->field->prev;
			break;
		    case forward :
			menu->field = menu->field->next;
			break;
		    }
	    while ((menu->field != NIL) && monitor(menu->field));
	    break;

	case left :
	case right :
	    old_line = menu->field->data.line;

	    do {

		switch (movement) {
		    case left :
			menu->field = menu->field->prev;
			break;
		    case right :
			menu->field = menu->field->next;
			break;
		    }

		if (menu->field != NIL)
		    if (menu->field->data.line != old_line)
			menu->field = NIL;
	      
	    } while ((menu->field != NIL) && monitor(menu->field));

	    break;

	case home :
	    menu->field = menu->first_field;

	    while ((menu->field != NIL) && monitor(menu->field))
		menu->field = menu->field->next;
	    break;
	}

    if (menu->field == NIL)
	menu->field = old_field;
}



static
void display_field (void)

/*
 * display current field
 */

{
    item_descr *cur_item;

    { list_type *pl = &(menu->field->field_type.l);
    if ((menu->field->type == list) && (pl->format == mapped))
	{
	cur_item = pl->item;
	pl->item = pl->first_item;

	while (pl->item != NIL)
	    {
	    strcpy (buffer, pl->item->name);
	    line_number = menu->field->data.line;
	    column_number = menu->field->data.column + pl->item->order *
		(menu->field->width + 1);

	    if (pl->item == cur_item) 
		display_buffer (attribute_bold, menubutton, 0);
	    else 
		display_buffer (EMPTY_SET, menubutton, 0);

	    pl->item = pl->item->next;
	    }

	pl->item = cur_item;
	}
    else
	{
	strcpy (buffer, menu->field->data.str);
	str_pad (buffer, space, menu->field->width);

	line_number = menu->field->data.line;
	column_number = menu->field->data.column;

	if (menu->field->type == list) 
	    display_buffer (attribute_bold, menubutton, 0);
	else 
	    {
/* TODO */
	    if (0)
		display_buffer (attribute_reverse, entry, LOADFILE);
	    else
		display_buffer (attribute_reverse, entry, 0);
	    }
	}
    }
}



static
void display_text (void)

/*
 * display text
 */

{
    strcpy (buffer, menu->text->str);

    line_number = menu->text->line;
    column_number = menu->text->column;

    display_buffer (EMPTY_SET, label, 0);
}



static
void display_legend (void)

/*
 * display legend text
 */

{
    int count;

    if (menu->legend != NIL)
	for (count = 0; count <= 4; count++)
	    {
	    strcpy (buffer, menu->legend[count]->str);
	    line_number = menu->legend[count]->line;

	    column_number = menu->legend[count]->column;

	    if (count == 0)
		display_buffer (attribute_bold | attribute_reverse, label, 0);
	    else
		display_buffer (attribute_reverse, button, count);
	    }
}


static
void display_menu (void)

/*
 * display menu on the screen
 */

{
    field_descr *old_field;
    text_descr *old_name;

    set_up_terminal ();
    clear_menu ();


    menu->text = menu->first_text;
#ifdef TCL
    if (display_mode == gui)
    {
	DOTCL("set winwidth [expr $xfactor*" WIDTH "]");
	DOTCL("set winheight [expr $yfactor*" HEIGHT "]");
    }
#endif
    while (menu->text != NIL)
	{
	display_text ();
	menu->text = menu->text->next;
	}

    display_legend ();

    old_field = menu->field;
    menu->field = menu->first_field;

    old_name = NIL;

    while (menu->field != NIL)
	{
	if (display_mode == tty)
	    {
	    if (menu->field == old_field)
		render (menu->field->name);
	    else
		if (menu->field->name != old_name)
		    fade_out (menu->field->name);
	    }
	else
	    {
	    if (menu->field->type == none)
		{
		strcpy (buffer, menu->field->name->str);
		line_number = menu->field->name->line;
		column_number = menu->field->name->column;

		display_buffer (attribute_bold, button, 5);		
		}
	    else if (menu->field->name != old_name)
		{
		fade_out (menu->field->name);
		}
	    }
		

	old_name = menu->field->name;

	display_field ();

	menu->field = menu->field->next;
	}

    menu->field = old_field;
    
#ifdef TCL
    if (display_mode == gui)
    {
	DOTCL("set status \"\"");
	DOTCL("set widget [wname status]");
	DOTCL("label $widget -textvariable status");
	DOTCL("place $widget -x [expr $xfactor*2] -y $winheight");
	if (!menu_built)
	    {
	    DOTCL("wm geometry $w "
	      "[expr $winwidth+$xfactor]x[expr $winheight+$yfactor]");
	    DOTCL("update idletasks");
	    }
	DOTCL("if {$setfocus != \"\"} {focus $setfocus}");
    }
    menu_built = TRUE;
#endif

    flush_buffer ();
} 



static
BOOL exit_status (void)

/*
 * return status information
 */

{
    return ((ret_stat == fdv__quit) || (ret_stat == fdv__exit) || 
	    (ret_stat == fdv__rewind));
}



static
int search (int key)

/*
 * search specified key
 */

{
    if (key < 0)
	ret_stat = fdv__invkey; /* invalid key */
    else

	switch (operating_mode) {
	    case disp :
		ret_stat = fdv__undefkey; /* undefined key */

		if (menu != NIL)
		    {
		    menu->node = menu->first_node;

		    while ((menu->node != NIL) && (ret_stat != fdv__normal))
			if (menu->node->key == key)
			    ret_stat = fdv__normal;
			else
			    menu->node = menu->node->next;
		    }

		if (ret_stat == fdv__normal)
		    menu = menu->node->sub_menu;
		break;

	    case load :
		if (key == 0)
		    if (menu == NIL)
			ret_stat = fdv__nomenu;
		    else
			ret_stat = fdv__normal;
		else
		    ret_stat = fdv__undefkey; /* undefined key */
		break;

	    case call :
		ret_stat = fdv__nodisp; /* no menu displayed */
		break;
	    }

    return (ret_stat);
}



static
void test_field (BOOL display_flag)

/*
 * compare field with communication region entry
 */

{
    int i;
    float r;
    char s[max_column+1], old_s[max_column+1];
    int b;
    int status;

    menu_dest = (u_addr *)((u_addr)data_base + menu->field->offset);

    switch (menu->field->type) {
	case integer :
	    { int_type *pi = &(menu->field->field_type.i);

	    i = *(int *)menu_dest;

	    if (i != pi->i_value)
		{
		str_dec (s, i);

		if ((pi->i_min <= i) && (i <= pi->i_max))
		    pi->i_value = i;
		else
		    raise_exception (fdv__range, 0, NULL, NULL);

		if (strlen(s) > menu->field->width)
		    s[menu->field->width] = '\0';

		strcpy (old_s, menu->field->data.str);
		strcpy (menu->field->data.str, s);

		if (display_flag)
		    display_data (old_s);
		}
	    break;
	    }

	case real :
	    { real_type *pr = &(menu->field->field_type.r);

	    r = *(float *)menu_dest;

	    if (r != pr->r_value)
		{
		str_flt (s, r);

		if ((pr->r_min <= r) && (r <= pr->r_max))
		    pr->r_value = r;
		else
		    raise_exception (fdv__range, 0, NULL, NULL);

		if (strlen(s) > menu->field->width)
		    s[menu->field->width] = '\0';

		strcpy (old_s, menu->field->data.str);
		strcpy (menu->field->data.str, s);

		if (display_flag)
		    display_data (old_s);
		}
	    break;
	    }

	case ascii :
	    if (called_from_pascal)
		{ varying_string *str = (varying_string *)menu_dest;

		strncpy (s, str->body, str->len);
		s[str->len] = '\0';
		}
	    else
		strcpy (s, (char *)menu_dest);

	    if (strcmp(s, menu->field->data.str) != 0)
		{
		if (strlen(s) > menu->field->width)
		    s[menu->field->width] = '\0';

		strcpy (old_s, menu->field->data.str);
		strcpy (menu->field->data.str, s);

		if (display_flag)
		    display_data (old_s);
		}
	    break;

	case list : 
	    { list_type *pl = &(menu->field->field_type.l);
	    b = *(int *)menu_dest;

	    if (b != pl->item->order)
		{
		if (pl->format == mapped)
		    display_item (EMPTY_SET);
		
		pl->item = pl->first_item;

		/* Lookup - lookup an identifier of an enumerated type */

		status = fdv__invsynenu;

		while ((pl->item != NIL) && (!odd(status))) 
		    {
		    if (pl->item->order == b)
			status = fdv__normal;
		    else
			pl->item = pl->item->next;
		    }

		if (!odd(status))
		    {
		    pl->item = pl->first_item;
		    }
		strcpy (menu->field->data.str, pl->item->name);

		if (display_flag)
		    display_item (attribute_bold);
		}
	    break;
	    }

	case none : 
	    break;
    }
}



static
void update_menu (BOOL display_flag)

/*
 * update menu
 */

{
    menu_descr saved_menu;

    if (data_base != NIL)
	{
	saved_menu = *menu;

	menu->field = menu->first_field;

	while (menu->field != NIL)
	    {
	    test_field (display_flag);
	    menu->field = menu->field->next;
	    }

	*menu = saved_menu;
	}
}



static
void initialize_field (void)

/*
 * initialize current field
 */

{
    switch (menu->field->type) {

	case integer :
	    { int_type *pi = &(menu->field->field_type.i);
	    if (pi->i_min > 0)
		pi->i_value = pi->i_min;
	    else
		if (pi->i_max < 0)
		    pi->i_value = pi->i_max;
		else
		    pi->i_value = 0;
	    break;
	    }

	case real :
	    { real_type *pr = &(menu->field->field_type.r);
	    if (pr->r_min > 0)
		pr->r_value = pr->r_min;
	    else
		if (pr->r_max < 0)
		    pr->r_value = pr->r_max;
		else
		    pr->r_value = 0;
	    break;
	    }

	case ascii :
	    break;
	}
}



static
void pas_call (void (*user_action_routine)(int *, int *), int *event_flag,
    int *status)
{
    user_action_routine (event_flag, status);
}



static
void signal_event (int event_flag, void (*user_action_routine)(int *, int *))

/*
 * signal an event
 */

{
    int ignore, e_flag;

    e_flag = event_flag;

    if (present(user_action_routine))
	{
	user_action_routine (&e_flag, &ret_stat);

	if (odd(ret_stat) && !debug_mode)
	    if (ret_stat != fdv__retain)
		clear (status_line);

	if (return_from_submenu)
	    {
	    return_from_submenu = FALSE;
	    update_menu (FALSE);

	    if (!exit_status())
		{
		display_menu ();
		signal_event (fdv_c_init, user_action_routine);
		}
	    }
	else
	    update_menu (TRUE);

	if (ret_stat == fdv__save)
	    save_menu (TRUE);
	else
	    if (ret_stat == fdv__unsave)
		unsave_menu (&ignore, TRUE);
	}

    if (ret_stat == fdv__do)
	if (menu->field != NIL)
	    if (menu->field->efn != 0)
		signal_event (menu->field->efn, user_action_routine);
}



static
void display_debug_information (void)

/*
 * display debug information
 */

{
    char line[3], column[3];
    char field_range[13], field_event[13];
    char str[255];
    int event_flag;
    fdv_attributes attributes;

    int_type *pi = &(menu->field->field_type.i);
    real_type *pr = &(menu->field->field_type.r);
    list_type *pl = &(menu->field->field_type.l);
    

    if (odd(ret_stat) && (menu->field != NIL))
	event_flag = menu->field->efn;

	if (menu->field->type == list)
	    if (pl->item->efn != 0)
		event_flag = pl->item->efn;

	fdv_clear (status_line, NIL);

	fdv_text ("line=", status_line, 1, 0, NIL);
	str_dec (line, cur_line);
	fdv_text (line, status_line, 6, attribute_reverse, NIL);

	fdv_text ("column=", status_line, 9, 0, NIL);
	str_dec (column, cur_column);
	fdv_text (column, status_line, 16, attribute_reverse, NIL);

	fdv_text (field_type[(int)(menu->field->type)], status_line, 20, 
	    attribute_bold, NIL);

	if (menu->field->type == integer)
	    {
	    if (pi->i_range) 
		{
		strcpy (field_range, str_dec(str, pi->i_min));
		strcat (field_range, "..");
		strcat (field_range, str_dec(str, pi->i_max));
		}
	    else
		strcpy (field_range, "no range");
	    }
	else
	    if (menu->field->type == real)
		{
		if (pr->r_range) 
		    {
		    strcpy (field_range, str_flt(str, pr->r_min));
		    strcat (field_range, "..");
		    strcat (field_range, str_flt(str, pr->r_max));
		    }
		else
		    strcpy (field_range, "no range");
		}
	    else
		strcpy (field_range, "");

	fdv_text (field_range, status_line, 30, attribute_bold, NIL);

	if (event_flag == 0)
	    {
	    strcpy (field_event, "no event");
	    attributes = 0;
	    }
	else
	    {
	    strcpy (field_event, "event ");
	    strcat (field_event, str_dec(str, event_flag));
	    attributes = attribute_blinking;
	    }

	fdv_text (field_event, status_line, 50, attributes, NIL);

	if (menu->field->monitor)
	    fdv_text ("no input  ", status_line, 70, 0, NIL);
	else
	    fdv_text (input_mode[insert_mode], status_line, 70, 0, NIL);
}



static
void read_item_render (int new_column)

/*
 * render cursor
 */

{
    char str[255];

    line_number = cur_line;
    column_number = new_column;

    cursor_pos = column_number - menu->field->data.column + 1;

    if (cursor_pos <= menu->field->width)
	{
	if (display_mode == tty)
	    {
	    if (cursor_pos <= strlen(menu->field->data.str)) 
		{
		buffer[0] = menu->field->data.str[cursor_pos - 1];
		buffer[1] = '\0';
		}
	    else
		{
#ifdef DECterm
		buffer[0] = space;
#else
		buffer[0] = '_';
#endif
		buffer[1] = '\0';
		}
	    display_buffer (attribute_bold | attribute_blinking | 
			    attribute_reverse, label, 0);
	    }
	}
    else
	{
	column_number = menu->field->data.column + menu->field->width - 1;
	strcpy (buffer, symbol(str, diamond));
	display_buffer (attribute_bold | attribute_blinking | 
	    attribute_reverse, label, 0);
	}
}



static
void read_item_fade_out (int old_column)

/*
 * fade out cursor
 */

{
    line_number = cur_line;
    column_number = old_column;

    cursor_pos = column_number - menu->field->data.column + 1;

    if (cursor_pos <= menu->field->width)
	{
	if (cursor_pos <= strlen(menu->field->data.str)) 
	    {
	    buffer[0] = menu->field->data.str[cursor_pos - 1];
	    buffer[1] = '\0';
	    }
	else 
	    {
	    buffer[0] = space;
	    buffer[1] = '\0';
	    }
	if (display_mode == tty)
	    display_buffer (attribute_reverse, label, 0);
	}
    else
	{
	column_number = menu->field->data.column + menu->field->width - 1;
	buffer[0] = menu->field->data.str[menu->field->width];
	buffer[1] = '\0';
	if (display_mode == tty)
	    display_buffer (attribute_reverse, label, 0);
	}
}



static
void input_check (void)

/*
 * perform syntax check
 */

{
    int i;
    float r;
    unsigned char ch;

    if (strlen(buffer) > 0)

	switch (menu->field->type) {

	    case integer :
		i = str_integer (buffer, &ret_stat);

		if (odd(ret_stat))
		    menu->field->field_type.i.i_value = i;
		break;

	    case real :
		r = str_real (buffer, &ret_stat);

		if (odd(ret_stat))
		    menu->field->field_type.r.r_value = r;
		break;

	    case ascii :
		ch = (unsigned char) buffer[cursor_pos - 1];

		if (ch >= space)
		    ret_stat = fdv__normal;
		else
		    ret_stat = fdv__error;
		break;
	    }
    else
	{
	initialize_field ();
	ret_stat = fdv__normal;
	}
}



static
void range_check (void)

/*
 * perform range check
 */

{
    BOOL out_of_range;
    char str[255];

    out_of_range = FALSE;

    switch (menu->field->type) {

	case integer :
	    { int_type *pi = &(menu->field->field_type.i);
	    if (pi->i_range)
		{
		if ((pi->i_min > pi->i_value) || (pi->i_value > pi->i_max))
		    {
		    if (!ignore_status)
			{
			strcpy (buffer, "?Value must be in range ");
			strcat (buffer, str_dec(str, pi->i_min));
			strcat (buffer, "..");
			strcat (buffer, str_dec(str, pi->i_max));
			fdv_message (buffer, NIL);
			}
		    out_of_range = TRUE;
		    }
		}
	    break;
	    }

	case real :
	    { real_type *pr = &(menu->field->field_type.r);
	    if (pr->r_range)
		{
		if ((pr->r_min > pr->r_value) || (pr->r_value > pr->r_max))
		    {
		    if (!ignore_status)
			{
			strcpy (buffer, "?Value must be in range ");
			strcat (buffer, str_flt(str, pr->r_min));
			strcat (buffer, "..");
			strcat (buffer, str_flt(str, pr->r_max));
			fdv_message (buffer, NIL);
			}
		    out_of_range = TRUE;
		    }
		}
	    break;
	    }
	}

    if (out_of_range)
	ret_stat = fdv__error;
    else
	if (!exit_status())
	    ret_stat = fdv__normal;
}



static
void del_char (void)

/*
 * delete character
 */

{
    char str[buffer_size];

    cursor_pos = cur_column - menu->field->data.column;

    if (chr == del_chr)
	cursor_pos++;

    strcpy (buffer, "");
    strncat (buffer, menu->field->data.str, cursor_pos-1);
    if (cursor_pos < strlen(menu->field->data.str))
	strncat (buffer, &menu->field->data.str[cursor_pos], 
	    strlen(menu->field->data.str)-cursor_pos);

    if ((menu->field->type == integer) || (menu->field->type == real))
	input_check ();
    else
	ret_stat = fdv__normal;

    if (odd(ret_stat))
	{
	strcpy (menu->field->data.str, buffer);
	if (chr == rubout)
	    cur_column--;
#ifndef VMS
	if (chr == backspace)
	    cur_column--;
#endif

	line_number = cur_line;
	column_number = cur_column;
	strcpy (str, "");
	strncat (str, &buffer[cursor_pos-1], strlen(buffer) - 
	    cursor_pos + 1);
	strcat (str, " ");
	strcpy (buffer, str);

	display_buffer (attribute_reverse, label, 0);

	if (chr == del_chr)
	    read_item_render (cur_column);

	update_field ();
	}
    else
	ring_bell ();
}



static
BOOL end_of_form (void)

/*
 * return end of form status
 */

{
    BOOL result;

    switch (chr) {

#ifdef VMS
	case backspace : 
	    jump (back);
	    break;
#endif
	case west : 
	    jump (left);
	    break;

	case north_west : 
	    jump (up);
	    jump (left);
	    break;

	case south_west :
	    jump (down);
	    jump (left);
	    break;

	case tab :
	case cr :
#ifndef MSDOS
	case open_line :
#endif
	    jump (forward);
	    break;

	case east :
	    jump (right);
	    break;

	case north_east :
	    jump (up);
	    jump (right);
	    break;

	case south_east :
	    jump (down);
	    jump (right);
	    break;

	case downarrow :
	case downwards :
	    jump (down);
	    break;

	case uparrow :
	case upwards :
	    jump (up);
	    break;

	case home_cursor :
	case top :
	    jump (home);
	    break;
	}

    result = (menu->field == old_field);
    next_field = menu->field;
    menu->field = old_field;

    return (result);
}



static
void read_item (int interval, void (*user_action_routine)(int *, int *))

/*
 * process input for current field
 */

{
    BOOL new_field;
    char prev_string[max_column+1];
    int old_column;
    char str[255];

    new_field = FALSE;

    strcpy (old_string, menu->field->data.str);
    cur_line = menu->field->data.line;
    cur_column = menu->field->data.column + strlen (menu->field->data.str);

    if (display_mode == tty)
	read_item_render (cur_column);
    old_column = cur_column;

    do {
	if (debug_mode)
	    display_debug_information ();

	if (display_mode == tty && old_column != cur_column) 
	    {
	    read_item_fade_out (old_column);
	    read_item_render (cur_column);
	    old_column = cur_column;
	    }

	get_key (&chr, interval);

	switch (chr) {

	    case hangup : /* hangup */
		signal_event (fdv_c_hangup, user_action_routine);
		break;

	    case null :   /* timed out */
	    case cancel : /* canceled */
		strcpy (prev_string, menu->field->data.str);

		signal_event (fdv_c_update, user_action_routine);

		if (strcmp(menu->field->data.str, prev_string) != 0) 
		    {
		    if (cur_column > menu->field->data.column + 
			strlen(menu->field->data.str))
			cur_column = menu->field->data.column + 
			    strlen(menu->field->data.str);

		    if (display_mode == tty)
			read_item_render (cur_column);
		    }
		break;

	    case leftarrow : 
		if (cur_column > menu->field->data.column)
		    cur_column--;
		break;

	    case rightarrow :
		if (cur_column < menu->field->data.column + 
		    strlen(menu->field->data.str))
		    cur_column++;
		break;

	    case ctrl_a :
	    case f_14 :
		insert_mode = !insert_mode;
		break;

	    case f_12 :
		cur_column = menu->field->data.column;
		break;

	    case ctrl_e :
		if (cur_column < menu->field->data.column + 
		    strlen(menu->field->data.str))
		    cur_column = menu->field->data.column + 
			strlen(menu->field->data.str);
		break;

#ifndef VMS
	    case backspace :
#endif
	    case rubout :
		if (cur_column > menu->field->data.column)
		    del_char ();
		break;

	    case del_chr :
		if (cur_column < menu->field->data.column + 
		    strlen(menu->field->data.str))
		    del_char ();
		break;

	    case linefeed :
	    case del_field :
	    case f_13 :
		if (strlen(menu->field->data.str) > 0) 
		    {
		    strcpy (buffer, "");
		    str_pad (buffer, space, menu->field->width);

		    strcpy (menu->field->data.str, "");
		    line_number = menu->field->data.line;
		    column_number = menu->field->data.column;

		    display_buffer (attribute_reverse, entry, 0);

		    if (display_mode == tty)
			{
			cur_line = line_number;
			cur_column = column_number;
			read_item_render (cur_column);

			old_column = cur_column;
			}

		    initialize_field ();
		    update_field ();
		    }
		break;

#ifndef MSDOS
	    case open_line :
#endif
#ifdef VMS
	    case backspace : 
#endif
	    case west :
	    case north_west :
	    case south_west :
	    case tab :
	    case east :
	    case north_east :
	    case south_east :
	    case cr :
	    case downarrow :  
	    case downwards :
	    case uparrow :
	    case upwards :
	    case home_cursor :
	    case top :
		if (!end_of_form())
		    new_field = TRUE;
		break;

	    case enter :
	    case do_key :
		if (menu->field->efn != 0)
		    new_field = TRUE;
		break;

	    case help :
	    case help_key :
	    case f1 :
	    case f2 :
	    case f3 :
	    case f4 :
	    case pf1 :
	    case pf2 :
	    case pf3 :
	    case pf4 :
	    case ctrl_c :
	    case ctrl_d :
	    case formfeed :
	    case ctrl_r :
	    case ctrl_w :
	    case ctrl_z :
	    case f_10 :
		new_field = TRUE;
		break;

	    default :
		if (insert_mode && (strlen(menu->field->data.str) >= 
		    menu->field->width))
		    ring_bell ();
		else 
		    {
		    cursor_pos = cur_column - menu->field->data.column + 1;

		    if (cursor_pos <= menu->field->width) 
			{
			strcpy (buffer, menu->field->data.str);

			if (cursor_pos > strlen(buffer))
			    strncat (buffer, &chr, 1);
			else
			    if (insert_mode) 
				{
				strcpy (str, "");
				strncat (str, &buffer[cursor_pos-1], 
				    strlen(buffer) - cursor_pos + 1);

				buffer[cursor_pos - 1] = '\0';

				strncat (buffer, &chr, 1);
				strcat (buffer, str);
				}
			    else
				buffer[cursor_pos-1] = chr;

			input_check ();

			if (odd(ret_stat)) 
			    {
			    strcpy (menu->field->data.str, buffer);

			    if (insert_mode)
				{
				strcpy (str, "");
				strncat (str, &buffer[cursor_pos-1], 
				    strlen(buffer) - cursor_pos + 1);

				strcpy (buffer, str);
				}
			    else
				{
				buffer[0] = chr;
				buffer[1] = '\0';
				}

			    line_number = cur_line;
			    column_number = cur_column;

			    display_buffer (attribute_reverse, entry, 0);

			    cur_column++;

			    update_field ();
			    }
			else
			    ring_bell ();
			}
		    else
			ring_bell ();
		    } 
		} 
	    }
    while (!new_field && !exit_status());

    read_item_fade_out (cur_column);

    if (display_mode == tty)
	if ((menu->field->type == integer) || (menu->field->type == real))
	    if (strlen(menu->field->data.str) > 0)
		range_check ();

    if (odd(ret_stat) && (menu->field->efn != 0))
	if (ignore_status || display_mode == gui ||
	   (strcmp(old_string, menu->field->data.str) != 0))
	    signal_event (menu->field->efn, user_action_routine);
}



static
void toggle (int interval, void (*user_action_routine)(int *, int *))

/*
 * process toggle field
 */

{
    BOOL	new_field;
    item_descr	*prev_item, *old_item;

    line_number = menu->field->data.line;
    column_number = menu->field->data.column;
    strcpy (old_string, menu->field->data.str);

    new_field = FALSE;
    old_item = menu->field->field_type.l.item;
    prev_item = NIL;

    do {
	if (menu->field->field_type.l.item != prev_item)
	    {
	    display_item (attribute_bold | attribute_blinking | 
		attribute_reverse);

	    prev_item = menu->field->field_type.l.item;
	    cur_line = line_number;
	    cur_column = column_number;
	    }

	if (debug_mode)
	    display_debug_information ();

	get_key (&chr, interval);

	switch (chr) {

	    case hangup : /* hangup */
		signal_event (fdv_c_hangup, user_action_routine);
		break;

	    case null :   /* timed out */
	    case cancel : /* canceled */
		signal_event (fdv_c_update, user_action_routine);
		break;

	    case rightarrow :
		{ list_type *pl = &(menu->field->field_type.l);
		if (pl->item->next != NIL)
		    {
		    if (pl->format == mapped)
			display_item (EMPTY_SET);

		    pl->item = pl->item->next;
		    strcpy (menu->field->data.str, pl->item->name);

		    update_field ();

		    if (pl->item->efn != 0)
			{
			signal_event (pl->item->efn, user_action_routine);
			new_field = TRUE;
			}
		    }
		break;
		}

	    case leftarrow :
		{ list_type *pl = &(menu->field->field_type.l);
		if (pl->item->prev != NIL)
		    {
		    if (pl->format == mapped)
			display_item (EMPTY_SET);

		    pl->item = pl->item->prev;
		    strcpy (menu->field->data.str, pl->item->name);
		    update_field ();

		    if (pl->item->efn != 0)
			{
			signal_event (pl->item->efn, user_action_routine);
			new_field = TRUE;
			}
		    }
		break;
		}

#ifndef MSDOS
	    case open_line :
#endif
#ifdef VMS
	    case backspace : 
#endif
	    case west :
	    case north_west :
	    case south_west :
	    case tab :
	    case east :
	    case north_east :
	    case south_east :
	    case cr:
	    case downarrow :
	    case downwards :
	    case uparrow :
	    case upwards :
	    case home_cursor :
	    case top :
		if (!end_of_form())
		    new_field = TRUE;
		break;

	    case enter :
	    case do_key :
		if (menu->field->efn != 0)
		    new_field = TRUE;
		break;

	    case help :
	    case help_key :
	    case f1 :
	    case f2 :
	    case f3 :
	    case f4 :
	    case pf1 :
	    case pf2 :
	    case pf3 :
	    case pf4 :
	    case ctrl_c :
	    case ctrl_d :
	    case formfeed :
	    case ctrl_r :
	    case ctrl_w :
	    case ctrl_z :
	    case f_10 :
		new_field = TRUE;
		break;

	    default :
		ring_bell ();
	    }
    } while (!(new_field || exit_status()));

    
    display_item (attribute_bold);

    if (menu->field->efn != 0)
	if (ignore_status || (menu->field->field_type.l.item != old_item))
	    signal_event (menu->field->efn, user_action_routine);
}



static
void test_key (int interval, void (*user_action_routine)(int *, int *))

/*
 * test for enter/do key
 */

{
    BOOL new_field;

    new_field = FALSE;
    cur_line = menu->field->data.line;
    cur_column = menu->field->data.column;

    do {

	if (debug_mode)
	    display_debug_information ();

	get_key (&chr, interval);

	switch (chr) {

	    case hangup : /* hangup */
		signal_event (fdv_c_hangup, user_action_routine);
		break;

	    case null :   /* timed out */ 
	    case cancel : /* canceled */
		signal_event (fdv_c_update, user_action_routine);
		break;

	    case rightarrow :
	    case leftarrow : /* ignored */
		break;

#ifndef MSDOS
	    case open_line :
#endif
#ifdef VMS
	    case backspace : 
#endif
	    case west :
	    case north_west :
	    case south_west :
	    case tab :
	    case east :
	    case north_east :
	    case south_east :
	    case cr :
	    case downarrow :
	    case downwards :
	    case uparrow :
	    case upwards :
	    case home_cursor :
	    case top :
		if (!end_of_form())
		    new_field = TRUE;
		break;

	    case enter :
	    case do_key :
		signal_event (menu->field->efn, user_action_routine);
		break;

	    case help :
	    case help_key :
	    case f1 :
	    case f2 :
	    case f3 :
	    case f4 :
	    case pf1 :
	    case pf2 :
	    case pf3 :
	    case pf4 :
	    case ctrl_c :
	    case ctrl_d :
	    case formfeed :
	    case ctrl_r :
	    case ctrl_w :
	    case ctrl_z :
	    case f_10 :
		new_field = TRUE;
		break;

	    default :
		ring_bell ();
	    }
    } while (!((ret_stat == fdv__return) || exit_status() || new_field));
}



static
void update_com_reg (void)

/*
 * update communication region
 */

{
    field_descr *old_field;

    if (data_base != NIL)
	{
	old_field = menu->field;
	menu->field = menu->first_field;

	while (menu->field != NIL)
	    {
	    update_field ();
	    menu->field = menu->field->next;
	    }
	menu->field = old_field;
	} 
}



static
void restore_field (void)

/*
 * restore old field
 */

{
    ring_bell ();

    switch (menu->field->type) {
	case integer :
	case real :
	case ascii :
	    *menu->field = saved_field;
	    display_data (old_string);

	    update_field ();
	    break;

	case list :
	    display_item (EMPTY_SET);

	    *menu->field = saved_field;
	    display_item (attribute_bold);

	    update_field ();
	    break;

	case none :
	    break;
	}

    clear (status_line);
    ret_stat = fdv__normal;
}



static
void save_context (void)
{
    context *p;

    p = (context *)malloc(sizeof(context));

    p->last = saved_context;
    p->data_base = data_base;
    p->insert_mode = insert_mode;
    p->ignore_status = ignore_status;
    p->chr = chr;
    strcpy (p->old_string, old_string);
    p->old_field = old_field;
    p->next_field = next_field;
    p->saved_field = saved_field;

    saved_context = p;
}



static
void restore_context (void)
{
    context *p;

    p = saved_context;

    data_base = p->data_base;
    insert_mode = p->insert_mode;
    ignore_status = p->ignore_status;
    chr = p->chr;
    strcpy (old_string, p->old_string);
    old_field = p->old_field;
    next_field = p->next_field;
    saved_field = p->saved_field;

    saved_context = p->last;
    free (p);
}



void fdv_disp (int key, void (*user_action_routine)(int *, int *),
    void *com_reg, fdv_mode mode, int interval, int *status)

/*
 * display specified menu
 */

{
    save_context ();

    insert_mode = FALSE;
    ignore_status = FALSE;

    ret_stat = search(key);

    if (ret_stat == fdv__normal)
	{
	if (present(com_reg))
	    {
	    data_base = (u_addr *)com_reg;

	    if (menu->db != NIL)
		if (data_base != menu->db)
		    {
		    /* invalid communication region address */
		    raise_exception (fdv__invaddr, 0, NULL, NULL);

		    if (debug_mode)
			tt_fprintf (stderr, "fdv_disp: va=%p, should be %p\n",
			    com_reg, menu->db);
		    }
	    }
	else
	    data_base = menu->db;

	if (menu->prev == NIL)
	    {
	    connect (terminal);
	    clear_display ();
	    operating_mode = disp;
	    }

	menu->field = menu->first_field;
	jump (home);

	old_field = menu->field;

	return_from_submenu = FALSE;

	update_com_reg ();

	display_menu ();
	signal_event (fdv_c_init, user_action_routine);

	if (!exit_status())
	    {
	    if ((menu->field != NIL) && (mode == mode_conversational))

		do {

		    if (menu->field->name != old_field->name)
			{
			if (display_mode == tty)
			    fade_out (old_field->name);
			render (menu->field->name);
			}

		    line_number = menu->field->data.line;
		    column_number = menu->field->data.column;

		    ret_stat = fdv__normal;
		    ignore_status = TRUE;

		    old_field = menu->field;

		    saved_field = *menu->field;

		    do {

			ignore_status = !ignore_status;

			switch (menu->field->type) {
			    case integer :
			    case real :
			    case ascii : 
				read_item (interval, user_action_routine);
				break;

			    case list :
				toggle (interval, user_action_routine);
				break;

			    case none :
				test_key (interval, user_action_routine);
				break;
			    }

		    } while (!(odd(ret_stat) || ignore_status));

		    if (!odd(ret_stat))
			{
			if (ignore_status)
			    restore_field ();
			}
		    else
			if ((ret_stat != fdv__retain) && !debug_mode)
			    clear (status_line);

		    switch (chr) {

			case hangup : /* hangup */
			case null :   /* timed out */
			case cancel : /* canceled */
			    break;

#ifndef MSDOS
			case open_line :
#endif
#ifdef VMS
			case backspace :
#endif
			case west :
			case north_west :
			case south_west :
			case tab :
			case east :
			case north_east :
			case south_east :
			case cr :
			case downarrow :
			case downwards :
			case uparrow :
			case upwards :
			case home_cursor :
			case top :
			    menu->field = next_field;
			    break;
			    
			case rightarrow :
			case leftarrow :
			case enter :
			case do_key : /* ignored */
			    break;

			case help :
			case help_key :
			    signal_event (fdv_c_help, user_action_routine);
			    break;

			case f1 :
			case pf1 :
			    signal_event (fdv_c_f1, user_action_routine);
			    break;

			case f2 :
			case pf2 :
			    signal_event (fdv_c_f2, user_action_routine);
			    break;
			    
			case f3 :
			case pf3 :
			    signal_event (fdv_c_f3, user_action_routine);
			    break;
			    
			case f4 :
			case pf4 :
			    signal_event (fdv_c_f4, user_action_routine);
			    break;
			    
			case ctrl_c :
			    ret_stat = fdv__quit;
			    break;
			    
			case ctrl_d :
			    debug_mode = !debug_mode;
			    if (!debug_mode)
				clear (status_line);
			    break;
			   
			case formfeed :
			    clear_display ();
			case ctrl_r :
			case ctrl_w :
			    display_menu ();
			    signal_event (fdv_c_refresh, user_action_routine);
			    break;
			   
			case ctrl_z :
			case f_10 :
			    ret_stat = fdv__quit;
			    signal_event (fdv_c_ctrl_z, user_action_routine);
			    break;
			    
			default :
			    ring_bell ();
			}
		    
		} while (!((ret_stat == fdv__return) || exit_status()));
	    else

		do {

		    get_key (&chr, interval);

		    switch (chr) {
			case hangup : /* hangup */
			    signal_event (fdv_c_hangup, user_action_routine);
			    break;

			case null :   /* timed out */
			case cancel : /* canceled */
			    signal_event (fdv_c_update, user_action_routine);
			    break;

			case f1 :
			case pf1 :
			    signal_event (fdv_c_f1, user_action_routine);
			    break;

			case f2 :
			case pf2 :
			    signal_event (fdv_c_f2, user_action_routine);
			    break;

			case f3 :
			case pf3 :
			    signal_event (fdv_c_f3, user_action_routine);
			    break;
			    
			case f4 :
			case pf4 :
			    signal_event (fdv_c_f4, user_action_routine);
			    break;
			   
			case ctrl_c :
			    ret_stat = fdv__quit;
			    break;
			   
			case ctrl_d :
			    debug_mode = !debug_mode;
			    if (!debug_mode)
				clear (status_line);
			    break;
			   
			case formfeed :
			    clear_display ();
			case ctrl_r :
			case ctrl_w :
			    display_menu ();
			    signal_event (fdv_c_refresh, user_action_routine);
			    break;
			   
			case ctrl_z :
			case f_10 :
			    ret_stat = fdv__quit;
			    signal_event (fdv_c_ctrl_z, user_action_routine);
			    break;
			   
			default :
			    ring_bell ();
			}
		    
		} while (!((ret_stat == fdv__return) || exit_status()));
	    }

	clear_menu ();
	clear (status_line);

	return_from_submenu = (menu->prev != NIL);

	if (!return_from_submenu)
	    {
	    if (ret_stat == fdv__exit)
		save_menu (FALSE);

	    reset_terminal ();
	    clear_display ();
#ifdef TCL
	    if (display_mode == tty)
		{
#endif
	    disconnect ();
#ifdef TCL
		}
	    else
		{
		exit_tk();
		}
#endif
	    operating_mode = load;
	    }

	jump (home);

	if (ret_stat == fdv__return)
	    ret_stat = fdv__normal;

	if (return_from_submenu)
	    {
	    if ((ret_stat == fdv__rewind) && (menu == first_menu))
		ret_stat = fdv__normal;
	    else
		menu = menu->prev;
	    }
	}

    if (present(status))
	*status = ret_stat;
    else
	if (!odd(ret_stat))
	    raise_exception (ret_stat, 0, NULL, NULL);

    restore_context ();
}



int fdv_present (var_variable_descr *variable, int *status)

/*
 * return presence of specified variable
 */

{
    int ret_stat;
    field_descr *dest;
    var_variable_descr *addr;
    u_addr src_addr, dest_addr;
    BOOL result;

    result = FALSE;

    if (operating_mode == disp)
	{
	addr = variable;
	src_addr = (u_addr)addr;

	if (menu->db != NIL)
	    {
	    dest = menu->first_field;
	    ret_stat = fdv__invaddr;

	    while ((dest != NIL) && (ret_stat == fdv__invaddr))
		{
		dest_addr = (u_addr)menu->db + dest->offset;
		if ((dest_addr == src_addr) && (dest->type != none)) 
		    {
		    ret_stat = fdv__normal;
		    result = (strlen(str_remove(dest->data.str, 
			' ', FALSE)) > 0);
		    }
		else
		    dest = dest->next;
		}
	    }
	else
	    ret_stat = fdv__nocomreg; /* no communication region */
	}
    else
	ret_stat = fdv__nodisp; /* no menu displayed */

    if (present(status))
	*status = ret_stat;
    else
	if (!odd(ret_stat))
	    raise_exception (ret_stat, 0, NULL, NULL);

    return (result);
}



void fdv_update (int *status)

/*
 * signal menu update
 */

{
    int stat;

    if (operating_mode == disp)
	{
	cancel_read ();
	stat = fdv__normal;
	}
    else
	stat = fdv__nodisp; /* no menu displayed */

    if (present(status))
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);
}

 


void fdv_message (char *chars, int *status)

/*
 * display a message on the screen
 */

{
    int stat;
    char old_buffer[buffer_size];
    int old_line, old_column;

    if (display_mode == tty)
	{
	if (operating_mode == disp)
	    {
	    old_line = line_number;
	    old_column = column_number;
	    strcpy (old_buffer, buffer);

	    line_number = status_line;
	    column_number = 1;
	    center (buffer, chars, max_column);
    
	    if (strlen(chars) > 0)
		display_buffer (attribute_reverse, entry, 0);
	    else
		clear (status_line);

	    flush_buffer ();

	    line_number = old_line;
	    column_number = old_column;
	    strcpy (buffer, old_buffer);

	    if (strlen(chars) <= max_column)
		stat = fdv__normal;
	    else
		stat = fdv__strtoolon; /* string is too long */
	    }
	else
	    stat = fdv__nodisp; /* no menu displayed */

	if (present(status))
	    *status = stat;
	else
	    if (!odd(stat))
	raise_exception (stat, 0, NULL, NULL);

	} else { /* display_mode == gui */
#ifdef TCL
	    if (*chars == '?')
		fdv_error (chars + 1, &stat);
	    else
		{
		DOTCL("if {[info exists afterid]} {after cancel $afterid}");
		DOTCL1("set status \"%s...\"", chars);
		DOTCL("set afterid ["
			"after 2000 {"
			  "set status \"\";"
			  "catch {unset afterid}"
			 "}"
		      "]");
		}
#endif
	}
}



void fdv_message_c (varying_string *charsP, int *status)

{
    char chars[255];

    strncpy (chars, charsP->body, charsP->len);
    chars[charsP->len] = '\0';

    fdv_message (chars, status);
}



void fdv_error (char *chars, int *status)

/*
 * display an error-message on the screen
 */

{
    if (display_mode == tty)
	fdv_message(chars, status);
#ifdef TCL
    else
	{
	display_error(chars);
	*status = fdv__normal;
	}
#endif
}



void fdv_text (char *chars, int line, int column, int attributes, int *status)

/*
 * display text
 */

{
    int stat;
    char old_buffer[buffer_size];
    int old_line, old_column;

    if (operating_mode == disp)
	{
	if (strlen(chars) <= max_column)
	    {
	    old_line = line_number;
	    old_column = column_number;
	    strcpy (old_buffer, buffer);

	    line_number = line;
	    column_number = column;
	    strcpy (buffer, chars);

	    display_buffer (attributes, label, 0);
	    flush_buffer ();

	    line_number = old_line;
	    column_number = old_column;
	    strcpy (buffer, old_buffer);

	    stat = fdv__normal;
	    }
	else
	    stat = fdv__strtoolon; /* string is too long */
	}
    else
	stat = fdv__nodisp; /* no menu displayed */

    if (present(status))
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);
}



void fdv_text_c (varying_string *charsP, int line, int column, int attributes,
    int *status)
{
    char chars[255];

    strncpy (chars, charsP->body, charsP->len);
    chars[charsP->len] = '\0';

    fdv_text (chars, line, column, attributes, status);
}



void fdv_clear (int line, int *status)

/*
 * clear line
 */

{
    int stat;

    if (operating_mode == disp)
	{
	clear (line);
	flush_buffer ();

	stat = fdv__normal;
	}
    else
	stat = fdv__nodisp; /* no menu displayed */

    if (present(status))
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);
}



void fdv_call (void (*user_action_routine)(void), int *status)

/*
 * call user action routine
 */

{
    int stat;

    if (operating_mode == disp)
	{
	reset_terminal ();
	clear_display ();

	operating_mode = call;
	user_action_routine ();
	operating_mode = disp;

	if (!exit_status())
	    {
	    clear_display ();
	    display_menu ();
	    }

	stat = fdv__normal;
	}
    else
	stat = fdv__nodisp; /* no menu displayed */

    if (present(status))
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);
}



void fdv_define (int *status)

/*
 * define symbol table
 */

{
    int stat, ignore;
    menu_descr saved_menu;
    char symbol[id_length];
    int parm_count;
    char str[255];

    if (operating_mode == disp)
	{
	saved_menu = *menu;

	parm_count = 0;
	menu->field = menu->first_field;

	while (menu->field != NIL)
	    {
	    if (menu->field->type != none)
		{
		parm_count++;
		strcpy (symbol, "P");
		str_dec (str, parm_count);
		strcat (symbol, str);

		sym_define (symbol, menu->field->data.str, &ignore);
		}
	    menu->field = menu->field->next;
	    }
	*menu = saved_menu;

	stat = fdv__normal;
	}
    else
	stat = fdv__nodisp; /* no menu displayed */

    if (present(status))
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);
}



char *fdv_get (char *name, int *status)

/*
 * get specified item
 */

{
    static char result[max_column+1];
    int stat, n;
    menu_descr saved_menu;

    if (operating_mode == disp)
	{
	strcpy(result, "");
	n = strlen(name);

	saved_menu = *menu;
	menu->field = menu->first_field;

	stat = fdv__noname; /* no such menu field name */

	while (stat == fdv__noname && menu->field != NIL)
	    {
	    if (menu->field->type != none)
		{
		if (strncmp(name, menu->field->name->str, n) == 0)
		    {
		    strcpy(result, menu->field->data.str);
		    stat = fdv__normal;
		    }
		}
	    menu->field = menu->field->next;
	    }
	*menu = saved_menu;
	}
    else
	stat = fdv__nodisp; /* no menu displayed */

    if (present(status))
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);

    return (result);
}



void fdv_exit (int *status)

/*
 * Form Driver exit routine
 */

{
    int stat;

    if (operating_mode != load)
	{
	reset_terminal ();
	clear_display ();

#ifdef TCL
	if (display_mode == tty)
	    {
#endif
	    disconnect ();
#ifdef TCL
	    }
	else
	    {
	    exit_tk();
	    }
#endif

	operating_mode = load;

	stat = fdv__normal;
	}
    else
	stat = fdv__nodisp; /* no menu displayed */

    if (present(status))
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);
}



void fdv_save (int log, int *status)

/*
 * copy internal data structure to the save-file
 */

{
    int stat;

    if (operating_mode == disp)
	{
	save_menu (log);
	stat = fdv__normal;
	}
    else
	stat = fdv__nodisp; /* no menu displayed */

    if (present(status))
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);
}
