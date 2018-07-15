/*
 *
 * (C) Copyright 1991-1999  Josef Heinen
 *
 *
 * FACILITY:
 *
 *	Graphics Language Interpreter (GLI)
 *
 * ABSTRACT:
 *
 *	This module contains a graphics command language interpreter.
 *
 * AUTHOR:
 *
 *	Josef Heinen
 *
 * VERSION:
 *
 *	V4.5
 *
 */

#if defined(RPC) || (defined(_WIN32) && !defined(__GNUC__))
#define HAVE_SOCKETS
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <setjmp.h>
#include <signal.h>

#if !defined(VMS) && !defined(MSDOS) && !defined(_WIN32)
#include <unistd.h>
#endif

#if defined(cray) || defined(__SVR4) || defined(MSDOS) || defined(_WIN32)
#include <fcntl.h>
#else
#include <sys/file.h>
#endif

#ifndef MSDOS
#include <sys/stat.h>
#include <sys/types.h>
#endif

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
#endif

#ifdef TCL

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winbase.h>
#include <direct.h>
#undef WIN32_LEAN_AND_MEAN
#endif

#if defined(_WIN32) && !defined(__GNUC__)
#define WIN32_LEAN_AND_MEAN
#include <mmsystem.h>
#undef WIN32_LEAN_AND_MEAN
#else
#include <sys/time.h>
#endif

#ifndef NO_X11
#include <X11/Intrinsic.h>
#endif

#include <tcl.h>
#include <tk.h>

#endif /* TCL */

#ifdef VMS
#include <ssdef.h>
#include <mthdef.h>
#include <rmsdef.h>
#endif


#include "system.h"
#include "strlib.h"
#include "mathlib.h"
#include "symbol.h"
#include "variable.h"
#include "function.h"
#include "terminal.h"
#include "command.h"
#include "formdrv.h"
#include "gksdefs.h"
#include "gus.h"
#include "image.h"
#include "sight.h"
#include "frtl.h"

int xui_present(void);
void xui_open(void);
void xui_close(void);
int xui_get_choice(char *, char *, char *, int *);
int xui_get_text(char *, char *, char *, int *);
int xui_get_value (char *, int *);

void lib_help (tt_cid *cid, char *topic, char *helpdb);

void gli_callrpc (char *command, int argc, int *size, float **data);
void cgm_import (char *, int, int, int);

void do_gks_command(int *);
void do_grsoft_command(int *);
void do_gus_command(int *);
void do_image_command(int *);
void do_sight_command(int *);
void simpleplot(int *);

#define BOOL int
#define TRUE 1
#define FALSE 0
#define NIL 0

#if defined(_WIN32) && !defined(__GNUC__)
#define MSECONDS 20
#else
#define USECONDS 20000
#endif

#define odd(status) ((status) & 01)
#define even(status) ((!(status)) & 01)


#ifdef VMS
#define default_prompt "GLI> "
#else
#define default_prompt "gli> "
#endif

#define comment_delim '!'

#define max_parm_count 8
#define max_outp_parm_count 19
#define max_inp_parm_count 40
#define max_rpc_parm_count 32
#define max_choice_count 32
#define read_buffer_size 80
#define record_size 800
#define string_length 255
#define max_event 32

#define tab '\11'
#define blank ' '
#define dollar '$'
#define underscore '_'
#define at_sign '@'
#define quotation_mark '\"'
#define semicolon ';'


#ifdef VMS
#define tty "tt"
#else
#define tty "tty"
#endif


typedef char gli_string[string_length];
typedef char gli_prompt_string[31];
typedef char gli_identifier[31];

typedef char file_specification[256];
typedef gli_string parameter_list[max_parm_count];

typedef enum {
    recovery_message, recovery_continue} error_recovery;


/* GLI commands */

typedef enum {
    command_dcl, command_execute_procedure, command_append,
    command_calculate, command_case, command_com, command_define,
    command_delete, command_display, command_do, command_exit,
    command_gks, command_gosub, command_goto, command_gridit, command_grsoft,
    command_gus, command_help, command_if, command_image, command_import,
    command_initialize, command_inquire, command_learn, command_load,
    command_message, command_on, command_pipe, command_print, command_quit,
    command_read, command_recover, command_redim, command_return, command_rpc,
    command_save, command_set, command_show, command_sight, command_simpleplot,
    command_sleep, command_smooth, command_split, command_write, command_xui
    } gli_command;

/* command options */

typedef enum {
    option_symbol, option_variable, option_function, option_logical
    } what_option;

typedef enum {
    option_default, option_verify, option_noverify,
    option_log, option_nolog, option_trace, option_notrace, option_prompt,
    option_ansi, option_tek, option_tcl, option_notcl, option_help,
    option_nohelp
    } set_option;

typedef enum {
    status_normal, status_error, status_do, status_save,
    status_unsave, status_return, status_rewind, status_quit,
    status_exit, status_retain
    } return_status;

typedef enum {
    format_tek, format_cgm_binary, format_cgm_clear_text
    } import_format;

typedef enum {
    x_choice, x_valuator
    } x_option;


/* define command keyword tables */

static cli_verb_list gli_command_table =

"$* @* append calculate case com define delete display do exit\
 gks gosub goto gridit grsoft gus help if image import initialize inquire\
 learn load message on pipe print quit read recover redim return rpc save set\
 show sight simpleplot sleep smooth split write xui";

static cli_verb_list conditions =

"help update init refresh f4 f3 f2 f1 ctrl_z\
 e1 e2 e3 e4 e5 e6 e7 e8 e9 e10 e11 e12 e13 e14 e15 e16\
 e17 e18 e19 e20 e21 e22 e23 e24 e25 e26 e27 e28 e29 e30 e31 e32";

static cli_verb_list then_keyword =
    "then";

static cli_verb_list what_options =
    "symbol variable function logical";

static cli_verb_list set_options =
"default verify noverify log nolog trace notrace prompt ansi tek tcl\
 notcl help nohelp";

static cli_verb_list status_codes =
    "normal error do save unsave return rewind quit exit retain";

static cli_verb_list x_options =
    "choice valuator";

static cli_verb_list import_formats =
    "tek cgm_binary cgm_clear_text";

static cli_verb_list interfaces =
    "gks sight";

static char *pl_s[] = {
    "", "s"
    };

static char *file_mode[] = {
    "written", "appended"
    };

static char *parm[] = {
    "P1", "P2", "P3", "P4", "P5", "P6", "P7", "P8"
    };

static char *ansi_escape[] = {
    "\033[?38l", "\033[?38h"
    };

static char *redim_prompt[] = {
    "Start X", "Dimension X", "Start Y", "Dimension Y", "X1", "Xn", "Y1", "Yn"
    };

extern char *com(char *);

extern char *app_name;
extern int max_points;
extern float *px, *py, *pz, *pe1, *pe2, z_min;

static int menu_stat;
static int uar_stat;

static gli_string uar_command;
static gli_prompt_string prompt;
static gli_string on_conditions [max_event-fdv_c_help+1] = {
    "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    };

static char **environment;
static BOOL interactive, verify, logging, trace, journal, traceback;
static BOOL buffered_io, recovery_complete, sleeping;

static tt_cid *cid;

#ifdef HAVE_SOCKETS
static BOOL listen;
static char *users[2];
static int nusers;
#endif

static int procedure_depth, menu_depth, eof_count;
static FILE *journal_file;

static jmp_buf recover;

static gli_string conid = "terminal";
static gli_string wstype = "";

static char *file = NULL;
#ifdef TCL
static char *progname;
#endif


static void evaluate_command (char *, error_recovery, int *);


#ifdef TCL

static Tcl_Interp *interp = NULL;
Tcl_Interp *gli_tcl_interp = NULL;
static Tcl_DString command;
static BOOL idle = TRUE;

static char *cmd[] = {
    "@", "APPEND", "CALCULATE", "CASE", "COM", "DEFINE", "DELETE", "DO", "EXIT",
    "GKS", "GOSUB", "GOTO", "GRIDIT", "GRSOFT", "GUS", "IF", "IMAGE", "IMPORT",
    "MESSAGE", "PIPE", "PRINT", "QUIT", "READ", "REDIM", "RETURN", "RPC",
    "SAVE", "SET", "SHOW", "SLEEP", "SMOOTH", "SPLIT", "WRITE", ":"
    };
static int n_cmd = sizeof(cmd)/sizeof(cmd[0]);

extern char (*sym_a_tcl_getvar)();

static const char *tcl_getvar (char *name)
{
    if (interp)
	return (Tcl_GetVar (interp, name, TCL_GLOBAL_ONLY));
    else
	return NULL;
}

static int tcl_do_command (ClientData *clientData, Tcl_Interp *interp, int argc,
    char **argv)
{
    int i, result, stat = cli__normal;
    int saved_tcl_interp;
    gli_string message;
    char buffer[1000];

    if (argc >= 1)
	{
	saved_tcl_interp = cli_b_tcl_interp;

	if (strcmp(argv[0], ":") == 0)
	    {
	    strcpy (buffer, "");
	    cli_b_tcl_interp = FALSE;
	    }
	else
	    strcpy (buffer, argv[0]);

	for (i = 1; i < argc; i++) {
	    strcat (buffer, " ");
	    strcat (buffer, argv[i]);
	    }

	*message = '\0';
#ifndef _WIN32
	gks_a_error_info = message;
#endif
	evaluate_command (buffer, recovery_continue, &stat);
	if (!odd(stat) || *message)
	    {
	    if (*message == '\0')
		get_status_text (stat, STS_M_MSG_TEXT, message);

	    Tcl_AppendResult (interp, message, (char *) NULL);
	    result = TCL_ERROR;
	    }
	else
	    result = TCL_OK;

#ifndef _WIN32
	gks_a_error_info = NULL;
#endif
	cli_b_tcl_interp = saved_tcl_interp;
	}
    else
	{
	interp->result = "wrong # of args: should be : ?arg ...?";
	result = TCL_ERROR;
	}

    return result;
}

#if defined(_WIN32) && !defined(__GNUC__)

static
MMRESULT timer_id;

static
void CALLBACK timeout_rtn(
    UINT nTimerID, UINT wMsg, DWORD dwUser, DWORD dw1, DWORD dw2)
{
    while (idle)
	if (Tk_DoOneEvent(TK_DONT_WAIT) == 0)
	    break;
}

static
void start_timer(void)
{
    DWORD dw;

    timer_id = timeSetEvent(MSECONDS, MSECONDS, timeout_rtn,
	(DWORD)(LONG far *)&dw, TIME_PERIODIC);
}

static
void cancel_timer(void)
{
    timeKillEvent(timer_id);
}

#else

static void timeout_rtn (int id)
{
    struct itimerval value;

    while (idle)
	if (Tk_DoOneEvent (TK_DONT_WAIT) == 0)
	    break;

    signal (SIGALRM, (void (*)(int))timeout_rtn);

#if defined(cray)
    alarm (1);
#else
    value.it_value.tv_sec     = 0;
    value.it_value.tv_usec    = USECONDS;
    value.it_interval.tv_sec  = 0;
    value.it_interval.tv_usec = 0;
    setitimer (ITIMER_REAL, &value, (struct itimerval *) 0);
#endif
}

static void start_timer()
{
    struct itimerval value;

    signal (SIGALRM, (void (*)(int))timeout_rtn);

#if defined(cray)
    alarm (1);
#else
    value.it_value.tv_sec     = 0;
    value.it_value.tv_usec    = USECONDS;
    value.it_interval.tv_sec  = 0;
    value.it_interval.tv_usec = 0;
    setitimer (ITIMER_REAL, &value, (struct itimerval *) 0);
#endif
}

static void cancel_timer()
{
    struct itimerval value;

#if defined(cray)
    alarm (0);
#else
    value.it_value.tv_sec     = 0;
    value.it_value.tv_usec    = 0;
    value.it_interval.tv_sec  = 0;
    value.it_interval.tv_usec = 0;
    setitimer (ITIMER_REAL, &value, (struct itimerval *) 0);

    signal (SIGALRM, SIG_DFL);
#endif
}

#endif /* _WIN32 */

static void create_tcl_interp (void)
{
    int i;

    if (interp == NULL)
	{
#if TK_MAJOR_VERSION>=8
	Tcl_FindExecutable(progname);
#endif
	gli_tcl_interp = interp = Tcl_CreateInterp ();
	sym_a_tcl_getvar = (char (*)()) tcl_getvar;

	Tcl_SetVar (interp, "argv", "gli", TCL_GLOBAL_ONLY);
	Tcl_SetVar (interp, "argc", "1", TCL_GLOBAL_ONLY);
	Tcl_SetVar (interp, "argv0", "gli", TCL_GLOBAL_ONLY);
	Tcl_SetVar (interp, "tcl_interactive", interactive ? "1" : "0",
	    TCL_GLOBAL_ONLY);

	if (Tcl_Init (interp) == TCL_ERROR)
	    tt_fprintf (stderr, "Tcl_Init failed: %s\n", interp->result);
	if (Tk_Init (interp) == TCL_ERROR)
	    tt_fprintf (stderr, "Tk_Init failed: %s\n", interp->result);

#if TK_MAJOR_VERSION>4
	Tcl_StaticPackage (interp, "Tk", Tk_Init, Tk_SafeInit);
#endif
	for (i = 0; i < n_cmd; i++)
	    Tcl_CreateCommand (interp, cmd[i], (Tcl_CmdProc *) tcl_do_command,
		(ClientData) 0, (Tcl_CmdDeleteProc *) NULL);

	Tcl_ResetResult (interp);

	cli_b_tcl_got_partial = FALSE;
	Tcl_DStringInit (&command);
	}

    cli_b_tcl_interp = TRUE;

    if (interactive)
	start_timer ();
}

static void delete_tcl_interp (void)
{
    if (interactive)
	cancel_timer ();

    if (interp != NULL)
	{
	Tcl_DeleteInterp (interp);
	gli_tcl_interp = interp = NULL;

	sym_a_tcl_getvar = NULL;
	}

    cli_b_tcl_interp = FALSE;
}

static void evaluate_tcl_command (char *buffer, int *stat)
{
    char *cmd;

    if (interp != NULL)
	{
	idle = FALSE;

	strcat (buffer, "\n");
	cmd = Tcl_DStringAppend (&command, buffer, -1);
	if (*buffer && !Tcl_CommandComplete (cmd))
	    cli_b_tcl_got_partial = TRUE;
	else 
	    {
	    cli_b_tcl_got_partial = FALSE;
	    if (Tcl_Eval (interp, cmd) != TCL_OK)
		tt_fprintf (stderr, "%s\n", interp->result);

	    else if (interactive && *interp->result != '\0' &&
		procedure_depth == 0)
		tt_printf ("%s\n", interp->result);

	    Tcl_DStringFree (&command);
	    }

	idle = TRUE;
	}
    else
	*stat = cli__notcl; /* no Tcl interpreter */
}

#endif /* TCL */



static void show_error_message (int status)
{
    gli_string line;
    int count, parse_index;
    
    cli_inquire_command (line, &parse_index, NIL);

    if (STATUS_SEVERITY(status) == STS_K_ERROR || procedure_depth > 0)
	{
	if (STATUS_FAC_NO(status) == function__facility)
	    parse_index = parse_index + fun_parse_index() - 1;

	tt_fprintf (stderr, "  %s\n", line);
	for (count = 0; count < parse_index; count++)
	    if (line[count] != tab)
		line[count] = blank;
	line[parse_index-1] = '\0';

	tt_fprintf (stderr, "  %s^\n", line);
	}

    status = status & ~STS_M_SEVERITY | STS_K_WARNING;
    raise_exception (status, 0, NULL, NULL);
}



int gli_input_routine (char *prompt, char *choice, char *str, BOOL *more)
{
    if (*choice)
	return (xui_get_choice (prompt, choice, str, more));
    else
	return (xui_get_text (prompt, choice, str, more));
}



static void local_startup (int *stat)

/*
 * local_startup - local startup procedure
 */

{
    gli_string str, startup;
    char *path;
    int len;

    path = (char *) getenv ("HOME");
    if (path)
	{
	strcpy (startup, path);
#ifndef VMS
	if (len = strlen(startup))
#ifdef _WIN32
	    if (startup[--len] != '\\')
		strcat (startup, "\\");
#else
	    if (startup[--len] != '/')
		strcat (startup, "/");
#endif
#endif
	}
    else
	strcpy (startup, "");

    strcat (startup, "gliini.gli");
    if (!access(startup, 0))
	{
	strcpy (str, "@");
	strcat (str, startup);
	evaluate_command (str, recovery_message, stat);
	}
}


static void reset_journal_file (void)

{
    fclose (journal_file);
    journal_file = fopen ("gli.jou", "r");
    if (!journal_file)
	journal_file = fopen ("gli.jou", "w");

    if (!journal_file)
	{
	tt_fprintf (stderr, "  Can't reset journal file.\n");
	exit (-1);
	}
}


static void rewrite_journal_file (void)

{
    fclose (journal_file);
#ifdef VMS
    remove ("gli.jou;0");
#endif
    journal_file = fopen ("gli.jou", "w");
    if (!journal_file)
	{
	tt_fprintf (stderr, "  Can't rewrite journal file.\n");
	exit (-1);
	}
}


#ifdef _WIN32

#define DIRDELIM '\\'

static
char *dirname(char *path)
{
    char *newpath;
    char *slash;
    int length;

    slash = strrchr(path, DIRDELIM);
    if (slash == 0)
    {
	/* File is in the current directory.  */
	path = ".";
	length = 1;
    }
    else
    {
	/* Remove any trailing slashes from the result.	 */
	while (slash > path && *slash == DIRDELIM)
	    --slash;

	length = slash - path + 1;
    }

    newpath = malloc(length + 1);
    if (newpath == 0)
	return 0;

    strncpy(newpath, path, length);
    newpath[length] = 0;

    return newpath;
}

#endif


static void setup_env (void)
{
    char *path, *env;
    gli_string home, str;

    path = (char *) getenv ("GLI_HOME");
    if (path)
	{
	strcpy (home, path);
#ifdef VMS
	strtok (home, "]");
#endif
	}
    else
	{
#ifdef VMS
	strcpy (home, "sys$sysdevice:[gli.");
#else
#ifdef _WIN32
	char filename[255];

#ifndef __GNUC__
	GetModuleFileName(NULL, filename, sizeof(filename));
#else
	strcpy (home, "/gli");
#endif
	strcpy (home, dirname(filename));
#else
	strcpy (home, "/usr/local/gli");
#endif
#endif
	sprintf (str, "GLI_HOME=%s", home);

	env = (char *) malloc (strlen(str) + 1);
	strcpy (env, str);
	putenv (env);
	}

    sprintf (str, "GLI_DEMO=%s", home);
#ifdef VMS
    strcat (str, ".demo]");
#else
#ifdef _WIN32
    strcat (str, "\\demo\\");
#else
    strcat (str, "/demo/");
#endif
#endif
    env = (char *) malloc (strlen(str) + 1);
    strcpy (env, str);
    putenv (env);

    sprintf (str, "GLI_IMAGES=%s", home);
#ifdef VMS
    strcat (str, ".demo.images]");
#else
#ifdef _WIN32
    strcat (str, "\\demo\\images\\");
#else
    strcat (str, "/demo/images/");
#endif
#endif
    env = (char *) malloc (strlen(str) + 1);
    strcpy (env, str);
    putenv (env);

    env = (char *) malloc (15);
    strcpy (env, "GLI_GKS=GLIGKS");
    putenv (env);
}


static void allocate_buffers (void)
{
    char *env;

    if (env = (char *) getenv ("GLI_POINTS"))
    {
	max_points = atoi(env);
	if (max_points < 1024)
	    max_points = 1024;
	else
	    if (max_points > 512*512)
		max_points = 512*512;
    }
    else
	max_points = 32768;
	
    px	= (float *) malloc (max_points * 5 * sizeof(float));
    py	= px  + max_points;
    pz	= py  + max_points;
    pe1 = pz  + max_points;
    pe2 = pe1 + max_points;
}


static void ctrl_c_ast_rtn (int sig)
{
    static int count = 0;
    int ignore, value = 0;

    if (cli_b_abort) {
	if (++count >= 2)
	    exit (-1);
	    }
	else
	    count = 0;
	    
    cli_b_abort = TRUE;
    
#ifdef VMS
    signal (SIGINT, (void (*)(int))ctrl_c_ast_rtn);
#endif
    
    if (sleeping) {
	if (menu_depth != 0)
	    fdv_exit (&ignore);
	
	longjmp (recover, value);
	}
}


#if defined(_WIN32) && defined(__CYGWIN__)

static HKEY hKey = HKEY_CURRENT_USER;

static void registry(int create)
{
  int ret_code;
  DWORD Disp;
  char CurDir[MAX_PATH];
  DWORD len = MAX_PATH;

  if (create)
  {
    if ((ret_code = RegCreateKeyEx(
      hKey, "Software\\GLI\\Directory", 0, "", REG_OPTION_NON_VOLATILE,
      KEY_SET_VALUE | KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS,
      NULL, &hKey, &Disp)) != ERROR_SUCCESS)
    {
      tt_fprintf(stderr, "Can't access registry.\n");
      exit(-1);
    }
    RegQueryValueEx(hKey, NULL, 0, NULL, CurDir, &len);
    if (*CurDir)
      chdir(CurDir);
  }
  else
  {
    getcwd(CurDir, MAX_PATH);
    len = strlen(CurDir);
    RegSetValueEx(hKey, NULL, 0, REG_SZ, CurDir, len);
    RegCloseKey(hKey);
  }
}

#endif


static void startup (int *stat)

/*
 * startup - GLI startup procedure
 */

{
    char *path;
    gli_string cmd_line;

    journal = FALSE;
    logging = FALSE;
    
    traceback = FALSE;
    cli_b_abort = FALSE;
    sleeping = FALSE;

    path = (char *) getenv ("GLI_HOME");
    if (path)
	{
	strcpy (cmd_line, "@ ");
	strcat (cmd_line, path);
#ifndef VMS
#ifdef _WIN32
	strcat (cmd_line, "\\");
#else
	strcat (cmd_line, "/");
#endif
#endif
	strcat (cmd_line, "glistart.gli");
	}
    else
#ifdef VMS
	strcpy (cmd_line, "@ sys$sysdevice:[gli]glistart.gli");
#else
#ifdef _WIN32
	strcpy (cmd_line, "@ c:\\gli\\glistart.gli");
#else
	strcpy (cmd_line, "@ /usr/local/gli/glistart.gli");
#endif
#endif

    evaluate_command (cmd_line, recovery_message, stat);

    sprintf (cmd_line, "gks open_ws %s %s", conid, wstype);
    evaluate_command (cmd_line, recovery_message, stat);

    local_startup (stat);

    cid = cli_connect (tty, NIL, stat);
#ifdef HAVE_SOCKETS
    if (listen)
	{
	char *env;

	nusers = 0;
	users[nusers++] = "jheinen";
	if ((env = (char *) getenv ("USER")) != NULL)
	    users[nusers++] = env;

	sys_authorize (nusers, users);
	sys_listen (0x4500);
	}
#endif

    signal (SIGINT, (void (*)(int))ctrl_c_ast_rtn);

    reset_journal_file ();

    while (fgets (cmd_line, string_length, journal_file) && !cli_b_abort)
	{
	str_translate (cmd_line, '\n', '\0');

	if (*cmd_line)
	    tt_write_buffer (cid, cmd_line, FALSE, NIL);
	else
	    break;
	}

    recovery_complete = TRUE;
    journal = TRUE;
    logging = TRUE;
}



static void shut_down (BOOL complete, int *stat)

/*
 * shut_down - GLI shutdown procedure
 */

{
    journal = FALSE;
    logging = FALSE;

#ifdef TCL
    delete_tcl_interp ();
#endif

    if (complete)
	{
	evaluate_command ("delete symbol *", recovery_continue, stat);
	evaluate_command ("delete variable *", recovery_continue, stat);
	evaluate_command ("delete function *", recovery_continue, stat);
	img_delete_all (stat);
	}

    evaluate_command ("gks emergency_close", recovery_continue, stat);

    cli_disconnect (stat);
#ifdef HAVE_SOCKETS
    sys_shutdown();
#endif
}



static void batch_processor (void)

/*
 * batch - GLI batch processor
 */

{
    char *path;
    gli_string cmd_line;
    cli_procedure command_procedure;
    int stat;
    char *cp;

    path = (char *) getenv ("GLI_HOME");
    if (path)
	{
	strcpy (cmd_line, "@ ");
	strcat (cmd_line, path);
#ifndef VMS
#ifdef _WIN32
	strcat (cmd_line, "\\");
#else
	strcat (cmd_line, "/");
#endif
#endif
	strcat (cmd_line, "glistart.gli");
	}
    else
#ifdef VMS
	strcpy (cmd_line, "@ sys$sysdevice:[gli]glistart.gli");
#else
#ifdef _WIN32
	strcpy (cmd_line, "@ c:\\gli\\glistart.gli");
#else
	strcpy (cmd_line, "@ /usr/local/gli/glistart.gli");
#endif
#endif

    evaluate_command (cmd_line, recovery_message, &stat);

    sprintf (cmd_line, "gks open_ws %s %s", conid, wstype);
    evaluate_command (cmd_line, recovery_message, &stat);

    local_startup (&stat);

#ifdef TCL
    if (file)
	{
/*
	create_tcl_interp ();
	if (Tcl_EvalFile (interp, file) != TCL_OK)
	    tt_fprintf (stderr, "%s\n", interp->result);
 */
	struct stat buf;
	int fd, cc;
	char *s;

	fd = open (file, O_RDONLY, 0);
	if (fd != -1)
	    {
	    fstat (fd, &buf);
	    s = (char *) malloc (buf.st_size + 1);
	    if ((cc = read (fd, s, buf.st_size)) != -1)
		{
		s[cc] = '\0';

		create_tcl_interp ();
		if (Tcl_Eval (interp, s) != TCL_OK)
		    tt_fprintf (stderr, "%s\n", interp->result);
		}
	    free (s);

	    close(fd);
	    }
	}
    else
#endif
    if (buffered_io)
	{
	command_procedure = cli_open_pipe (&stat);

	if (odd(stat))
	    {
	    while (odd(stat) && stat != cli__return && stat != cli__break &&
		!cli_eof(command_procedure, NIL))
		{
		cli_readln (command_procedure, cmd_line, NIL);
	    
		cp = cmd_line;
		while (*cp == blank || *cp == tab)
		    cp++;
		
		if (*cp != comment_delim && *cp)
		    evaluate_command (cmd_line, recovery_message, &stat);

		tt_printf ("%s\n", cmd_line);
		}

	    if (odd(stat) && cli_eof(command_procedure, NIL))
		tt_printf ("[EOF]\n");
	    else
		tt_printf ("\n");

	    cli_close (command_procedure, NIL);
	    }
	}
    else
	{
	setbuf (stdin, NULL);
	setbuf (stdout, NULL);
	setbuf (stderr, NULL);

	while (fgets(cmd_line, string_length, stdin))
	    {	 
	    str_translate (cmd_line, '\n', '\0');

	    cp = cmd_line;
	    while (*cp == blank || *cp == tab)
		cp++;
		
	    if (*cp != comment_delim && *cp)
		evaluate_command (cmd_line, recovery_message, &stat);
	    }
	}

#ifdef TCL
    if (interp)
	Tk_MainLoop ();
#endif

    shut_down (FALSE, &stat);

#ifdef TCL
    if (interp)
	Tcl_Eval (interp, "exit");
#endif

    exit (even(stat));
}



static void get_file_specification (char *file_spec, char *default_spec,
    int *stat)

/*
 * get_file_specification - get a file specification
 */

{
    char *result_spec;

    cli_get_parameter ("File", file_spec, " ,", FALSE, TRUE, stat);

    if (odd(*stat))
	{
	result_spec = (char *) getenv(file_spec);
	
	if (result_spec != NIL)
	    strcpy (file_spec, result_spec);

	if (*file_spec != '|')
	    {
	    str_parse (file_spec, default_spec, FAll, file_spec);

	    if (access (file_spec, 0))
		*stat = RMS__FNF;
	    }
	}
}



static void write_command_file (int *stat)

/*
 * write_command_file - create and write a new command file
 */

{
    FILE *command_file;
    file_specification file_spec;
    gli_string cmd_line;

    get_file_specification (file_spec, ".gli", stat);

    if (cli_end_of_command())
	{
	if (*stat == RMS__FNF)
	    *stat = RMS__NORMAL;

	if (odd(*stat))
	    {
	    command_file = fopen (file_spec, "w");
	    if (!command_file)
		{
		*stat = RMS__ACC; /* ACP file access failed */
		return;
		}

	    reset_journal_file ();

	    while (fgets (cmd_line, string_length, journal_file) &&
		   !cli_b_abort)
		{
		str_translate (cmd_line, '\n', '\0');

		if (*cmd_line)
		    fprintf (command_file, "%s\n", cmd_line);
		else
		    break;
		}

	    rewrite_journal_file ();
	    fclose (command_file);
	    }
	}
    else
	*stat = cli__maxparm; /* maximum parameter count exceeded */
}



static void dcl_command (int *stat)

/*
 * dcl_command - do dcl command
 */

{
    gli_string cmd_line;
    int pos;

    cli_inquire_command (cmd_line, NIL, NIL);

    pos = str_locate(cmd_line, dollar)+1;
    strcpy (cmd_line, &cmd_line[pos]);

    sys_system (cmd_line);
}



static void save_parm_list (parameter_list parameter)

/*
 * save_parm_list - save parameter list
 */

{
    int parm_count, status;

    for (parm_count = 0; parm_count < max_parm_count; parm_count++)
	{
	sym_translate (parm[parm_count], parameter[parm_count], &status);
	if (!odd(status))
	    strcpy (parameter[parm_count], "");
	}
}



static void restore_parm_list (parameter_list parameter)

/*
 * restore_parm_list - restore parameter list
 */

{
    int parm_count, ignore;

    for (parm_count = 0; parm_count < max_parm_count; parm_count++)
	if (*parameter[parm_count])
	    sym_define (parm[parm_count], parameter[parm_count], &ignore);
}



static
void find_file (file_specification file_spec, int *stat)
{
    char *env, *name;
    char path[256], filename[256];

    if ((env = getenv ("GLI_PATH")) != NULL)
	{
	strcpy (path, env);
	name = strtok (path, ":");
	while (name != NULL)
	    {
	    strcpy (filename, name);
#ifdef _WIN32
	    strcat (filename, "\\");
#else
	    strcat (filename, "/");
#endif
	    strcat (filename, file_spec);
	    if (access (filename, 0) == 0)
		{
		strcpy (file_spec, filename);
		*stat = RMS__NORMAL;
		break;
		}
	    name = strtok (NULL, ":");
	    }
      }
}



static void execute_procedure (int *stat)

/*
 * execute_procedure - execute command procedure
 */

{
    file_specification file_spec;
    cli_procedure command_procedure;
    gli_string saved_parm[max_parm_count];
    gli_string parameter, cmd_line, line;
    int ignore, p, n;
    char *cp;
    int saved_tcl_interp;

    cli_inquire_command (cmd_line, NIL, NIL);
    strcpy (line, cmd_line);

    cp = cmd_line;
    while (*cp && *cp != at_sign)
	cp++;
    if (*cp)
	cp++;
    cli_set_command (cp, NIL);

    get_file_specification (file_spec, ".gli", stat);

    if (*stat == RMS__FNF)
	{
#ifdef _WIN32
	if (strchr (file_spec, '\\') == NULL)
#else
	if (strchr (file_spec, '/') == NULL)
#endif
	    find_file (file_spec, stat);
	}

    if (odd(*stat))
	{
	p = -1;
	save_parm_list (saved_parm);

	saved_tcl_interp = cli_b_tcl_interp;
	cli_b_tcl_interp = FALSE;

	while (odd(*stat) && !cli_end_of_command() && p < max_parm_count)
	    {
	    cli_get_parameter ("Parameter", parameter, " ,", FALSE, TRUE, stat);
	    if (odd(*stat))
		{
		p = p+1;
		sym_define (parm[p], parameter, &ignore);
		}
	    }

	cli_inquire_command (line + (int)(cp - cmd_line), NIL, NIL);
	
	if (odd(*stat) && cli_end_of_command())
	    {
	    n = 0;
	    procedure_depth++;
	    command_procedure = cli_open (file_spec, stat);

	    while (odd(*stat) && !cli_b_abort && *stat != cli__return &&
		*stat != cli__break && !cli_eof(command_procedure, NIL))
		{
		n++;
		cli_readln (command_procedure, cmd_line, NIL);

		cp = cmd_line;
		while (*cp == blank || *cp == tab)
		    cp++;
		
		if (*cp != comment_delim && *cp)
		    evaluate_command (cmd_line, recovery_continue, stat);
		else
		    if (verify)
			tt_printf ("%s\n", cmd_line);
		
		if (!odd(*stat))
		    {
		    if (!traceback)
			{
			tt_fprintf (stderr, "Error in file %s, line %d\n",
			    file_spec, n);
			show_error_message (*stat);
			
			traceback = TRUE;
			}
		    else
			tt_fprintf (stderr, "  (%d)  %s, line %d\n",
			    procedure_depth, file_spec, n);
		    }
		}

	    if (verify)
		{
		if (odd(*stat) && cli_eof(command_procedure, NIL))
		    tt_printf ("@[EOF] %s\n", file_spec);
		}

	    cli_close (command_procedure, NIL);
	    procedure_depth--;
	    }
	else
	    *stat = cli__maxparm; /* maximum parameter count exceeded */

	cli_b_tcl_interp = saved_tcl_interp;

	restore_parm_list (saved_parm);
	}
    
    cli_set_command (line, NIL);
}



static void calculate_command (int *stat)

/*
 * calculate_command - calculate expression
 */

{
    gli_string expression;
    float value;

    cli_get_parameter ("Expression", expression, "", FALSE, TRUE, stat);

    if (odd(*stat))
	{
	var_find (1, NIL);
	fun_parse (expression, &value, FALSE, stat);

	if (odd(*stat))
	    tt_printf ("  %g\n", value);
	}
}



static void case_command (int *stat)

/*
 * case_command - evaluate case command
 */

{
    gli_identifier symbol;
    gli_string equ_symbol, choice;
    gli_string command;

    cli_get_parameter ("Symbol", symbol, " :=", FALSE, TRUE, stat);

    if (odd(*stat))
	{
	sym_translate (symbol, equ_symbol, stat);

	if (odd(*stat))
	    {
	    str_cap (equ_symbol);
	    cli_get_parameter ("Choice", choice, " ,", TRUE, TRUE, stat);

	    if (odd(*stat))
		{
		str_cap (choice);
		if (!cli_end_of_command())
		    {
		    cli_get_parameter ("Command", command, "", FALSE, TRUE,
			stat);

		    if (odd(*stat) && strcmp (equ_symbol, choice) == 0)
			evaluate_command (command, recovery_message, stat);
		    }
		else
		    *stat = cli__insfprm; /* missing command parameters */
		}
	    }
	}
}



static void com_command (int *stat)

/*
 * com_command - do com command
 */

{
    gli_string cmd_line;
    char *result;
    int ignore;

    cli_get_parameter ("Command", cmd_line, "", FALSE, TRUE, stat);

    if (odd(*stat))
	{
	result = com(cmd_line);

	if (*result != '?')
	    {
	    sym_define ("COM_RESULT", result, &ignore);
	    if (*result)
		tt_printf ("%s\n", result);
	    }
	else
	    tt_fprintf (stderr, "%s\n", result);
	}
}



static void define_symbol (int *stat)

/*
 * define_symbol - define symbol
 */

{
    gli_identifier symbol;
    gli_string equ_symbol;

    cli_get_parameter ("Symbol", symbol, " :=", FALSE, TRUE, stat);

    if (odd(*stat))
	{
	sym_translate (symbol, equ_symbol, stat);

	if (odd(*stat) || *stat == sym__undsymb)
	    {
	    cli_get_parameter ("Equ_Smbol", equ_symbol, "", FALSE, TRUE, stat);

	    if (odd(*stat))
		{
		str_remove (equ_symbol, quotation_mark, TRUE);
		sym_define (symbol, equ_symbol, stat);
		}
	    }
	}
}



static void define_variable (int *stat)

/*
 * define_variable - define variable
 */

{
    gli_identifier variable;
    var_variable_descr *addr;
    cli_data_descriptor data;
    BOOL segment;
    int index, retlen, ignore;

    cli_get_parameter ("Variable", variable, " :=", FALSE, TRUE, stat);

    if (odd(*stat))
	{
	addr = var_address(variable, stat);

	if (odd(*stat) || *stat == var__undefid || addr == NULL)
	    {
	    index = 1;
	    do
		{
		cli_get_data ("Data", &data, stat);

		while (odd(*stat) && !cli_b_abort)
		    {
		    cli_get_data_buffer (&data, max_points, px, &retlen, stat);

		    if (odd(*stat))
			{
			segment = (*stat == cli__eos) ? TRUE : FALSE;
			var_redefine (variable, index, retlen, px, segment,
			    stat);
			index += retlen;
			}
		    }

		if (*stat == cli__nmd)
		    *stat = cli__normal;
		}
	    while (odd(*stat) && !cli_end_of_command());

	    if (odd(*stat))
		var_truncate (variable, index, &ignore);
	    }
	}
}



static void define_function (int *stat)

/*
 * define_function - define function
 */

{
    gli_identifier function_id;
    fun_function_descr *addr;
    gli_string expression;
    BOOL existing_function;

    cli_get_parameter ("Function", function_id, " :=", FALSE, TRUE, stat);

   if (odd(*stat))
	{
	addr = fun_address(function_id, stat);
	existing_function = odd(*stat);

	if (existing_function || *stat == fun__undefun || addr == NULL)
	    {
	    cli_get_parameter ("Expression", expression, "", FALSE, TRUE, stat);

	    if (odd(*stat))
		{
		if (existing_function)
		    fun_delete (function_id, NIL);
		fun_define (function_id, expression, stat);
		}
	    }
	}
}



static void define_logical (int *stat)

/*
 * define_logical - define logical name
 */

{
    gli_identifier log_name;
    gli_string equ_name;
    char *env;

    cli_get_parameter ("Log name", log_name, " :=", FALSE, TRUE, stat);

    if (odd(*stat))
	{
	cli_get_parameter ("Equ name", equ_name, "", FALSE, TRUE, stat);

	if (odd(*stat))
	    {
	    env = (char *) malloc (strlen(log_name) + 1 + strlen(equ_name) + 1);

	    strcpy (env, log_name);
	    strcat (env, "=");
	    strcat (env, equ_name);

	    putenv (env);
	    }
	}
}



static void define_command (int *stat)

/*
 * define_command - evaluate define command
 */

{
    int option;

    cli_get_keyword ("What", what_options, &option, stat);

    if (odd(*stat))

	switch ((what_option)option)
	    {
	    case option_symbol :
	      define_symbol (stat);
	      break;

	    case option_variable :
	      define_variable (stat);
	      break;

	    case option_function :
	      define_function (stat);
	      break;

	    case option_logical :
	      define_logical (stat);
	      break;
	    }
}



static void delete_symbol (int *stat)

/*
 * delete_symbol - delete symbol
 */

{
    gli_identifier symbol_spec, symbol;
    sym_symbol_descr *context;
    gli_string equ_symbol;
    int n;

    do
	{
	cli_get_parameter ("Symbol(s)", symbol_spec, " ,", FALSE, TRUE, stat);

	if (odd(*stat))
	    {
	    context = NIL;
	    n = 0;
	    do
		{
		context = sym_inquire_symbol (context, symbol, equ_symbol,
		    stat);

		if (odd(*stat))
		    if (str_match (symbol, symbol_spec, FALSE))
			{
			n = n+1;
			sym_delete (symbol, stat);
			}
		}
	    while (context != NIL);

	    if (logging && (n > 0))
		tt_printf ("  %d symbol%s deleted\n", n, pl_s[n>1]);

	    if (odd(*stat) && (n == 0))
		*stat = sym__undsymb; /* undefined symbol */
	    }
	}
    while (odd(*stat) && !cli_end_of_command());
}



static void delete_variable (int *stat)

/*
 * delete_variable - delete variable
 */

{
    gli_identifier variable_spec, variable;
    var_variable_descr *context;
    int allocation, segments, size;
    int n, total_values;

    do
	{
	cli_get_parameter ("Variable(s)", variable_spec, " ,", FALSE, TRUE,
	    stat);

	if (odd(*stat))
	    {
	    context = NIL;
	    n = 0;
	    total_values = 0;

	    do
		{
		context = var_inquire_variable (context, variable, &allocation,
		    &segments, &size, stat);

		if (odd(*stat))
		    if (str_match (variable, variable_spec, FALSE))
			{
			n = n+1;
			total_values = total_values+allocation;
			var_delete (variable, stat);
			}
		}
	    while (context != NIL);

	    if (logging && (n > 0))
		tt_printf ("  %d variable%s deleted (%d value%s)\n", n,
		    pl_s[n>1], total_values, pl_s[total_values>1]);

	    if (odd(*stat) && (n == 0))
		*stat = var__undefid; /* undefined identifier */
	    }
	}
    while (odd(*stat) && !cli_end_of_command());
}



static void delete_function (int *stat)

/*
 * delete_function - delete function
 */

{
    gli_identifier function_spec, function_id;
    fun_function_descr *context;
    gli_string expression;
    int n;

    do
	{
	cli_get_parameter ("Function(s)", function_spec, " ,", FALSE, TRUE,
	    stat);

	if (odd(*stat))
	    {
	    context = NIL;
	    n = 0;
	    do
		{
		context = fun_inquire_function (context, function_id,
		    expression, stat);

		if (odd(*stat))
		    if (str_match (function_id, function_spec, FALSE))
			{
			n = n+1;
			fun_delete (function_id, stat);
			}
		}
	    while (context != NIL);

	    if (logging && (n > 0))
		tt_printf ("  %d function%s deleted\n", n, pl_s[n>1]);

	    if (odd(*stat) && (n == 0))
		*stat = fun__undefun; /* undefined function */
	    }
	}
    while (odd(*stat) && !cli_end_of_command());
}



static void delete_logical (int *stat)

/*
 * delete_logical - delete logical name
 */

{
    gli_identifier log_name;

    cli_get_parameter ("Log name", log_name, " ,", FALSE, TRUE, stat);

    if (odd(*stat))
	{
	if (cli_end_of_command())
	    if (logging)
		tt_fprintf (stderr,
		    "DELETE LOGICAL command not yet supported.\n");
	else
	    *stat = cli__maxparm; /* maximum parameter count exceeded */
	}
}



static void delete_command (int *stat)

/*
 * delete_command - evaluate delete command
 */

{
    int option;

    cli_get_keyword ("What", what_options, &option, stat);

    if (odd(*stat))

	switch ((what_option)option)
	    {
	    case option_symbol :
	      delete_symbol (stat);
	      break;

	    case option_variable :
	      delete_variable (stat);
	      break;

	    case option_function :
	      delete_function (stat);
	      break;

	    case option_logical :
	      delete_logical (stat);
	      break;
	    }
}



static
void fdv_do_command (void)

/*
 * fdv_do_command - evaluate a GLI command
 */

{
    evaluate_command (uar_command, recovery_message, &uar_stat);
}



static
void uar (int *efn, int *status)

/*
 * uar - Form Driver user-action routine
 */

{
    gli_string saved_parm[max_parm_count];
    int saved_tcl_interp;

    save_parm_list (saved_parm);

    saved_tcl_interp = cli_b_tcl_interp;
    cli_b_tcl_interp = FALSE;

    fdv_define (NIL);

    if (fdv_c_help <= *efn && *efn <= max_event)
	{
	strcpy (uar_command, on_conditions[*efn-fdv_c_help]);

	if (*efn == fdv_c_ctrl_z)
	    {
	    if (menu_depth == 1)
		*status = fdv__exit;
	    else
		*status = fdv__return;
	    }
	}
    else
	strcpy (uar_command, "");

    if (odd(*status) && *uar_command)
	{
	if (*efn > 0)
	    fdv_call ((void (*)(void)) fdv_do_command, NIL);
	else
	    fdv_do_command();

	*status = uar_stat;

	if (cli_b_abort)
	    *status = fdv__quit;
	else
	    if (odd(*status))
		*status = menu_stat;
	    else
		{
		*status = *status & ~STS_M_SEVERITY | STS_K_WARNING;
		raise_exception (*status, 0, NULL, NULL);
		}
	}

    cli_b_tcl_interp = saved_tcl_interp;

    restore_parm_list (saved_parm);
}



static int generation = 0, key = 0;
static void display (void);


static
void default_uar (int *efn, int *status)

/*
 * uar - default Form Driver user-action routine
 */

{
    int saved_key;

    switch (*efn)
	{
	case fdv_c_init :
	    fdv_text("\
   F1 = Back	  F2 = Forward	 F3 = Left	F4 = Right     Ctrl/Z = Quit",
		23, 1, 0, NIL);
	    break;

	case fdv_c_ctrl_z :
	    *status = fdv__quit;
	    break;

	case fdv_c_f1 :
	    *status = fdv__rewind;
	    break;

	case fdv_c_f2 :
	    generation++;
	    saved_key = key;
	    key = 1;

	    display ();

	    key = saved_key;
	    generation--;

	    if (*status == fdv__rewind)
		*status = fdv__normal;
	    break;

	case fdv_c_f3 :
	    if (key > 1) {
		key--;
		*status = fdv__return;
		}
	    break;

	case fdv_c_f4 :
	    key++;
	    *status = fdv__return;
	    break;

	default :
	    *status = fdv__normal;
	}
}



static void display (void)
{
    int status = fdv__normal;

    do
	{
	fdv_disp (key, default_uar, NIL, mode_conversational, 0, &status);

	if (key > 1 && status == fdv__undefkey) {
	    key--;
	    status = fdv__normal;
	    }

	if (generation == 0)
	    key = 0;
	}
    while (status == fdv__normal);
}



static void display_command (int *stat)

/*
 * display_command - evaluate display command
 */

{
    gli_string menu_key;
    int ignore;

    if (!cli_end_of_command())
	{
	cli_get_parameter ("Menu key", menu_key, " ,", FALSE, TRUE, stat);

	if (odd(*stat))
	    key = cli_integer(menu_key, stat);
	}
    else
	key = 0;

    if (odd(*stat))
	{
	if (cli_end_of_command())
	    {
	    menu_stat = fdv__normal;
	    menu_depth++;

	    if (key == -1) {
		cli_disconnect (&ignore);

		key = 0;
		display ();

		cid = cli_connect (tty, NIL, &ignore);
		}
	    else
		fdv_disp (key, uar, NIL, mode_conversational, 0, &menu_stat);

	    menu_depth--;
	    }
	else
	    *stat = cli__maxparm; /* maximum parameter count exceeded */
	}
}



static void do_command (int *stat)

/*
 * do_command - evaluate do command
 */

{
    gli_string cmd_line, command;
    int count;
    BOOL done = FALSE;

    count = 0;
    cli_get_parameter ("Command(s)", cmd_line, "", FALSE, TRUE, stat);

    while (odd(*stat) && !cli_b_abort && !done)
	{
	count = count+1;
	str_element(command, count, semicolon, cmd_line);

	if (*command)
	    evaluate_command (command, recovery_message, stat);
	else
	    done = TRUE;
	}
}



static void exit_command (int *stat)

/*
 * exit_command - evaluate exit command
 */

{
    if (procedure_depth == 0)
	{
	if (!cli_end_of_command())
	    write_command_file (stat);

	if (odd(*stat))
	    {
	    shut_down (FALSE, stat);
	    exit (even(*stat));
	    }
	}
    else
	/* return to caller */
	*stat = cli__break;
}



static void gosub_command (int *stat)

/*
 * gosub_command - evaluate gosub command
 */

{
    cli_label command_label;
    gli_string cmd_line;
    int ignore;
    char *cp;

    cli_get_parameter ("Label", command_label, " ,", FALSE, TRUE, stat);

    if (odd(*stat))
	{
	if (cli_end_of_command())
	    {
	    cli_gosub (command_label, stat);

	    while (odd(*stat) && !cli_b_abort && *stat != cli__return &&
		*stat != cli__break)
		{
		cli_get (cmd_line, stat);
		
		cp = cmd_line;
		while (*cp == blank || *cp == tab)
		    cp++;
		
		if (*cp != comment_delim && *cp)
		    {
		    if (odd(*stat))
			evaluate_command (cmd_line, recovery_message, stat);
		    }
		else
		    if (verify)
			tt_printf ("\n");
		}

	    if (*stat != cli__return)
		{
		cli_return (&ignore);
		if (*stat == cli__eof)
		    *stat = cli__normal;
		}
	    else
		*stat = cli__normal;
	    }
	else
	    *stat = cli__maxparm; /* maximum parameter count exceeded */
	}
}



static void goto_command (int *stat)

/*
 * goto_command - evaluate goto command
 */

{
    cli_label command_label;

    cli_get_parameter ("Label", command_label, " ,", FALSE, TRUE, stat);

    if (odd(*stat))
	{
	if (cli_end_of_command())
	    cli_goto (command_label, stat);
	else
	    *stat = cli__maxparm; /* maximum parameter count exceeded */
	}
}



static void gridit_command (int *stat)

/*
 * gridit_command - evaluate gridit command
 */

{
    gli_identifier x, y, z;
    var_variable_descr *xv, *yv, *zv;
    gli_string resolution;
    int nx, ny, nz, ignore;

    float *xd, *yd, *zd, *wk;
    int nd, i, nxi, nyi, *iwk;
    float *xi, *yi, *zi;

    xv = (var_variable_descr *) cli_get_variable ("X", x, &nx, stat);

    if (odd(*stat))
	{
	yv = (var_variable_descr *) cli_get_variable ("Y", y, &ny, stat);

	if (odd(*stat))
	    {
	    zv = (var_variable_descr *) cli_get_variable ("Z", z, &nz, stat);

	    if (odd(*stat))
		{
		if (!cli_end_of_command())
		    {
		    cli_get_parameter ("Resolution X", resolution, " ,", FALSE,
			TRUE, stat);

		    if (odd(*stat))
			nxi = cli_integer(resolution, stat);

		    if (odd(*stat))
			{
			if (!cli_end_of_command())
			    {
			    cli_get_parameter ("Resolution Y", resolution, " ,",
				FALSE, TRUE, stat);

			    if (odd(*stat))
				nyi = cli_integer(resolution, stat);
			    }
			else
			    nyi = nxi;
			}
		    }
		else
		    nxi = nyi = 40;
		}
	    }
	}

    if (odd(*stat))
	{
	if (cli_end_of_command())
	    {
	    nd = nx;
	    if (ny < nd ) nd = ny;
	    if (nz < nd ) nd = nz;

	    xd = (float *) malloc (nd * sizeof(float));
	    yd = (float *) malloc (nd * sizeof(float));
	    zd = (float *) malloc (nd * sizeof(float));
	    xi = (float *) malloc (nxi * sizeof(float));
	    yi = (float *) malloc (nyi * sizeof(float));
	    zi = (float *) malloc (nxi*nyi * sizeof(float));
	    iwk = (int *) malloc ((31*nd+nxi*nyi) * sizeof(int));
	    wk = (float *) malloc (8*nd * sizeof(float));

	    var_read_variable (xv, nd, xd, stat);
	    var_read_variable (yv, nd, yd, stat);
	    var_read_variable (zv, nd, zd, stat);

	    for (i = 0; i < nxi*nyi; i++)
		zi[i] = z_min;

	    GRIDIT (&nd, xd, yd, zd, &nxi, &nyi, xi, yi, zi, iwk, wk);

	    var_redefine (x, 1, nxi, xi, FALSE, NIL);
	    var_truncate (x, nxi+1, &ignore);
	    var_redefine (y, 1, nyi, yi, FALSE, NIL);
	    var_truncate (y, nyi+1, &ignore);
	    var_redefine (z, 1, nxi*nyi, zi, FALSE, NIL);
	    var_truncate (z, nxi*nyi+1, &ignore);

	    free (wk);
	    free (iwk);
	    free (zi);
	    free (yi);
	    free (xi);
	    free (zd);
	    free (yd);
	    free (xd);
	    }
	else
	    *stat = cli__maxparm; /* maximum parameter count exceeded */
	}
}



static void help_command (int *stat)

/*
 * help_command - evaluate help command
 */

{
    gli_string topic;
    char *path, helpdb[80];

    if (!cli_end_of_command())
	cli_get_parameter ("Topic", topic, "", FALSE, TRUE, stat);
    else
	strcpy (topic, "");

    if (odd(*stat))
	{

    path = (char *) getenv ("GLI_HOME");
    if (path != NULL)
	{
	strcpy (helpdb, path);
#ifndef VMS
#ifdef _WIN32
	strcat (helpdb, "\\");
#else
	strcat (helpdb, "/");
#endif
#endif
	strcat (helpdb, "gli.hlp");
	}
    else
#ifdef VMS
	strcpy (helpdb, "sys$sysdevice:[gli]gli.hlp");
#else
#ifdef _WIN32
	strcpy (helpdb, "c:\\gli\\gli.hlp");
#else
	strcpy (helpdb, "/usr/local/gli/gli.hlp");
#endif
#endif
	lib_help (cid, topic, helpdb);
	}
}



static void if_command (int *stat)

/*
 * if_command - evaluate if command
 */

{
    gli_string condition;
    float result;
    gli_string command;

    cli_get_parameter ("Condition", condition, " ,", TRUE, TRUE, stat);

    if (odd(*stat))
	{
	var_find (1, NIL);
	fun_parse (condition, &result, TRUE, stat);

	if (odd(*stat))
	    {
	    if (!cli_end_of_command())
		{
		cli_get_parameter ("Command", command, "", FALSE, TRUE, stat);

		if (odd(*stat) && (result != 0))
		    evaluate_command (command, recovery_message, stat);
		}
	    else
		*stat = cli__insfprm; /* missing command parameters */
	    }
	}
}



static void initialize_command (int *stat)

/*
 * initialize_command - evaluate initialize command
 */

{
    if (cli_end_of_command())
	{
	shut_down (TRUE, stat);
	startup (stat);
	}
    else
	*stat = cli__maxparm; /* maximum parameter count exceeded */
}



static void inquire_command (int *stat)

/*
 * inquire_command - evaluate inquire command
 */

{
    gli_identifier symbol;
    gli_string equ_symbol, prompt;

    cli_get_parameter ("Symbol", symbol, " ,", FALSE, TRUE, stat);

    if (odd(*stat))
	{
	sym_translate (symbol, equ_symbol, stat);
	if (*stat == sym__undsymb)
	    *stat = sym__normal;

	if (odd(*stat))
	    {
	    if (!cli_end_of_command())
		{
		cli_get_parameter ("Prompt", prompt, "", FALSE, TRUE, stat);
		str_remove (prompt, quotation_mark, TRUE);
		}
	    else
		{
		strcpy (prompt, str_cap (symbol));
		strcat (prompt, ": ");
		}

	    if (odd(*stat))
		{
		cli_get_parameter (prompt, equ_symbol, "", FALSE, FALSE, stat);

		if (odd(*stat) && *equ_symbol)
		    {
		    str_remove (equ_symbol, quotation_mark, TRUE);
		    sym_define (symbol, equ_symbol, stat);
		    }
		if (*stat == cli__eof)
		    *stat = cli__normal;
		}
	    }
	}
}



static void learn_command (int *stat)

/*
 * learn_command - evaluate learn command
 */

{
    if (cli_end_of_command())
	rewrite_journal_file ();
    else
	*stat = cli__maxparm; /* maximum parameter count exceeded */
}



static void load_command (int *stat)

/*
 * load_command - load menu description file
 */

{
    file_specification file_spec, display;

    get_file_specification (file_spec, ".fdv", stat);

    if (odd(*stat))
	{
	if (!cli_end_of_command())
	    cli_get_parameter ("Display", display, " ,", FALSE, TRUE, stat);
	else
	    strcpy (display, tty);

	if (odd(*stat))
	    {
	    if (cli_end_of_command())
		fdv_load (file_spec, display, NIL, "", stat);
	    else
		*stat = cli__maxparm; /* maximum parameter count exceeded */
	    }
	}
}



static void message_command (int *stat)

/*
 * message_command - display message
 */

{
    gli_string message;

    if (!cli_end_of_command())
	cli_get_parameter ("Message", message, "", FALSE, TRUE, stat);
    else
	strcpy (message, "");

    if (odd(*stat))
	{
	if (menu_depth != 0)
	    fdv_message (message, stat);
	else
	    tt_printf ("%s\n", message);
	}
}



static void on_command (int *stat)

/*
 * on_command - evaluate on command
 */

{
    int condition, keyword;

    cli_get_keyword ("Condition", conditions, &condition, stat);

    if (odd(*stat))
	{
	if (!cli_end_of_command())
	    {
	    cli_get_keyword ("Then", then_keyword, &keyword, stat);

	    if (odd(*stat))
		{
		if (!cli_end_of_command())
		    cli_get_parameter ("Command",
			on_conditions[condition], "", FALSE, TRUE,
			stat);
		else
		    *stat = cli__insfprm; /* missing command parameters */
		}
	    }
	else
	    *stat = cli__nothen; /* IF or ON statement syntax error */
	}
}



static void pipe_command (int *stat)

/*
 * pipe_command - evaluate pipe command
 */

{
#if !defined(VMS) && !defined(MSDOS) && !defined(_WIN32)
    gli_string command;
    FILE *stream;

    cli_get_parameter ("Command", command, "", FALSE, TRUE, stat);

    if (odd(*stat))
	{
	if (stream = popen (command, "r"))
	    {
	    while (odd(*stat) && fgets (command, sizeof(command), stream))
		{
		str_translate (command, '\n', '\0');

		if (*command != comment_delim && *command)
		    evaluate_command (command, recovery_continue, stat);
		}

	    pclose (stream);
	    }
	else
	    tt_fprintf (stderr, "  Can't open pipe\n");
	}
#else
    tt_fprintf (stderr,
	"  PIPE command not supported for this type of machine");
#endif
}



static void print_command (int *stat)

/*
 * print_command - evaluate print command
 */

{
    cli_data_descriptor data[max_outp_parm_count];
    int count, parm_count;
    float value;
    int n;
    BOOL segment;
    gli_string item, result;

    count = 0;
    do
	if (count < max_outp_parm_count)
	    {
	    cli_get_data ("Data", &data[count], stat);
	    count = count+1;
	    }
	else
	    *stat = cli__maxparm; /* maximum parameter count exceeded */

    while (odd(*stat) && !cli_end_of_command());

    if (odd(*stat))
	{
	parm_count = count;

	n = 0;
	count = 0;
	segment = FALSE;

	while (odd(*stat) && !cli_b_abort)
	    {
	    *result = '\0';

	    while (odd(*stat) && count < parm_count)
		{
		cli_get_data_buffer (&data[count], 1, &value, NIL, stat);

		if (*stat == cli__eos)
		    segment = TRUE;

		if (odd(*stat))
		    {
		    if (data[count].data_type == type_symbol)
			sprintf (item, "%s", data[count].type.symbol.equ);
		    else
			sprintf (item, "%g", value);

		    if (count)
			strcat (result, " ");
		    strcat (result, item);

		    count++;
		    }
		}

	    if (segment || count == parm_count) {
		n++;
		count = 0;
		}

	    if (segment) {
		n++;
		count = 0;
		segment = FALSE;
		}
#ifdef TCL
	    if (interp) {
		if (*result)
		    Tcl_AppendResult (interp, result, (char *) NULL);
		}
	    else
#endif
		tt_printf ("%s\n", result);
	    }

	if (*stat == cli__nmd)
	    *stat = cli__normal;

	if (count) {
	    n++;
#ifdef TCL
	    if (!interp)
#endif
		tt_printf ("\n");
	    }
	}
}



static void quit_command (int *stat)

/*
 * quit_command - evaluate quit command
 */

{
    if (cli_end_of_command())
	{
	shut_down (FALSE, stat);
	exit (even(*stat));
	}
    else
	*stat = cli__maxparm; /* maximum parameter count exceeded */
}



static void read_command (int *stat)

/*
 * read_command - evaluate read command
 */

{
    var_variable_descr *addr;
    FILE *data_file;
    file_specification file_spec;
    gli_identifier ident[max_inp_parm_count];
    int skipped, ignored, count, parm_count, ignore, n, index;
    register char *str, *cp, **endptr;
    char *buf, *line;
#if !defined(MSDOS) && !defined(_WIN32)
    struct stat stat_buffer;
#endif
    float value, buffer[max_inp_parm_count][read_buffer_size];
    int length[max_inp_parm_count];
    gli_string parm;
    BOOL segment, empty_string, data_found;

    get_file_specification (file_spec, ".dat", stat);

    if (odd(*stat))
	{
	count = 0;
	do
	    if (count < max_inp_parm_count)
		{
		cli_get_parameter ("Variable(s)", ident[count], " ,",
				    FALSE, TRUE, stat);
		addr = var_address(ident[count++], stat);
		if (*stat == var__undefid || addr == NULL)
		    *stat = var__normal;
		}
	    else
		*stat = cli__maxparm; /* maximum parameter count exceeded */

	while (odd(*stat) && !cli_end_of_command());

	if (odd(*stat))
	    {
#if !defined(VMS) && !defined(MSDOS) && !defined(_WIN32)
	    if (*file_spec == '|')
		{
#ifdef TCL
		if (cli_b_tcl_interp)
		    cancel_timer ();
#endif
		data_file = popen (file_spec+1, "r");
		}
	    else
#endif
		data_file = fopen (file_spec, "r");

	    if (!data_file)
		{
		*stat = RMS__ACC; /* ACP file access failed */
		return;
		}

#if !defined(VMS) && !defined(MSDOS) && !defined(_WIN32)
	    if (*file_spec != '|')
		{
		fstat (fileno(data_file), &stat_buffer);
		n = stat_buffer.st_size;
		}
	    else
#endif
		n = max_points * read_buffer_size;

	    buf = (char *) malloc (n + 1);
	    if ((n = fread (buf, 1, n, data_file)) > 0)
		buf[n] = '\0';
	    else
		*buf = '\0';

#if !defined(VMS) && !defined(MSDOS) && !defined(_WIN32)
	    if (*file_spec == '|')
		{
		pclose (data_file);
#ifdef TCL
		if (cli_b_tcl_interp)
		    start_timer ();
#endif
		}
	    else
#endif
		fclose (data_file);

	    if (logging && (n > 0))
		tt_printf ("  %d byte%s read from %s %s\n", n, pl_s[n>1],
		    *file_spec == '|' ? "pipe" : "file", file_spec + 
		    (*file_spec == '|' ? 1 : 0));

	    parm_count = count;
	    for (count = 0; count < parm_count; count++)
		{
		var_delete (ident[count], &ignore);
		length[count] = 0;
		}

	    skipped = ignored = 0;
	    n = 0;
	    count = 0;
	    index = 0;
	    segment = FALSE;
	    empty_string = TRUE;
	    data_found = FALSE;

	    endptr = (char **) malloc (sizeof(char *));
	    *endptr = buf;

	    cp = buf;
	    while (*cp && odd(*stat) && !cli_b_abort)
		{
		line = str = cp;
		while (*cp && *cp != '\n')
		    cp++;

		if (*cp == '\n') {
		    *cp++ = '\0';
		    n++;
		    }

		do {
		    while (isspace(*str))
			str++;

		    if (*str)
			{
			empty_string = FALSE;

			if (*str == '~' || *str == ',')
			    {
			    value = 0;
			    *endptr = str + 1;
			    }
			else
			    value = str_atof (str, endptr);

			if (*endptr != str)
			    {
			    if (**endptr == ',' && *str != ',')
				(*endptr)++;

			    data_found = TRUE;

			    if (count == parm_count)
				{
				if (++index == read_buffer_size || segment)
				    {
				    for (count = 0; count < parm_count; count++)
					{
					var_define (ident[count], 0,
					    length[count], buffer[count],
					    segment, stat);

					length[count] = 0;
					}

				    index = 0; 
				    segment = FALSE;  
				    }

				count = 0;
				}

			    buffer[count][index] = value;
			    if (*str != '~')
				length[count] = index + 1;

			    count++;
    
			    str = *endptr;
			    }
			else
			    {
			    if ((int) strlen(line) < string_length)
				{
				skipped++;
				sprintf (parm, "p%d", skipped);

				if (count)
				    {
				    str = line;
				    while (isspace(*str) && *str)
					str++;

				    sym_define (parm, str, &ignore);

				    if (count == parm_count)
					{
					for (count = 0; count < parm_count;
					    count++)
					    {
					    var_define (ident[count], 0,
						length[count], buffer[count],
						FALSE, stat);

					    length[count] = 0;
					    }

					index = 0;
					}

				    data_found = FALSE;
				    count = 0;
				    }
				else
				    sym_define (parm, line, &ignore);
				}
			    else
				ignored++;

			    *str = '\0';
			    }
			}
		    else
			if (empty_string && data_found)
			    segment = TRUE;
		    }	
		while (*str && (count < parm_count || parm_count == 1));

		empty_string = TRUE;
		}

	    free (endptr);
	    free (buf);

	    if (count)
		{
		for (count = 0; count < parm_count; count++)
		    var_define (ident[count], 0, length[count], buffer[count],
			FALSE, stat);
		}

	    if (logging && (n > 0))
		{
		tt_printf ("  Processed %d record%s\n", n, pl_s[n>1]);

		if (odd(*stat))
		    {
		    if (skipped > 0)
			tt_printf (
			    "  %d record%s contained textual information\n",
			    skipped, pl_s[skipped>1]);
		    if (ignored > 0)
			tt_printf ("  %d record%s were unreadable\n",
			    ignored, pl_s[ignored>1]);
		    }
		}
	    }
	}
}



static void recover_command (int *stat)

/*
 * recover_command - evaluate recover command
 */

{
    gli_string cmd_line;
    BOOL journal_flag;

    if (cli_end_of_command())
	{
	procedure_depth++;

	journal_flag = journal;
	journal = FALSE;
	recovery_complete = FALSE;

	reset_journal_file ();

	tt_printf ("Recovery started\n");

	while (fgets (cmd_line, string_length, journal_file) && odd(*stat) &&
	       !cli_b_abort)
	    {
	    str_translate (cmd_line, '\n', '\0');

	    if (*cmd_line)
		evaluate_command (cmd_line, recovery_message, stat);
	    else
		break;
	    }

	if (odd(*stat) && feof(journal_file))
	    tt_printf ("Recovery complete\n");

	journal = journal_flag;
	recovery_complete = TRUE;
	procedure_depth--;
	}
    else
	*stat = cli__maxparm; /* maximum parameter count exceeded */
}



static int get_integer (char *parm, int min_value, int max_value, int *stat)
{
    int value;

    value = cli_integer(parm, stat);
    if (odd(*stat))
	{
	if (value < min_value || value > max_value)
	    *stat = cli__range; /* parameter out of range */
	}

    return value;
}


static void redim_command (int *stat)

/*
 * redim_command - evaluate redim command
 */

{
    gli_identifier z;
    var_variable_descr *zv;
    gli_string parm;
    int count, sx, nx, sy, ny, nz, nzi, x1, xn, y1, yn, ignore;
    register float *zd, *zi;
    register int i, j, k;

    zv = (var_variable_descr *) cli_get_variable ("Array", z, &nz, stat);

    count = 0;

    while (odd(*stat) && (count < 8))
	{
	cli_get_parameter (redim_prompt[count++], parm, " ,", FALSE,
	     TRUE, stat);

	if (odd(*stat))
	    {
	    switch (count)
		{
		case 1 : sx = get_integer(parm, 1, max_points, stat); break;
		case 2 : nx = get_integer(parm, sx, max_points, stat); break;
		case 3 : sy = get_integer(parm, 1, max_points, stat); break;
		case 4 : ny = get_integer(parm, sy, max_points, stat); break;
		case 5 : x1 = get_integer(parm, 1, max_points, stat); break;
		case 6 : xn = get_integer(parm, x1, max_points, stat); break;
		case 7 : y1 = get_integer(parm, 1, max_points, stat); break;
		case 8 : yn = get_integer(parm, y1, max_points, stat); break;
		}
	    }
	}

    if (odd(*stat))
	{
	if (cli_end_of_command())
	    {
	    nz = (nx - sx + 1) * (ny - sy + 1);
	    zd = (float *) malloc (nz * sizeof(float));
	    var_read_variable (zv, nz, zd, stat);

	    nzi = (xn - x1 + 1) * (yn - y1 + 1);
	    zi = (float *) malloc (nzi * sizeof(float));

	    for (i = y1, k = 0; i <= yn; i++)
		for (j = x1; j <= xn; j++)
		    if (i >= sy && i <= ny && j >= sx && j <= nx)
		      zi[k++] = zd[(i - sy) * (nx - sx + 1) + j - sx];
		    else
		      zi[k++] = 0;

	    var_redefine (z, 1, nzi, zi, FALSE, NIL);
	    var_truncate (z, nzi+1, &ignore);

	    free (zi);
	    free (zd);
	    }
	else
	    *stat = cli__maxparm; /* maximum parameter count exceeded */
	}
}



static void return_command (int *stat)

/*
 * return_command - evaluate return command
 */

{
    int status, ignore;

    *stat = cli__normal;

    if (!cli_end_of_command())
	{
	cli_get_keyword ("Status", status_codes, &status, stat);

	if (odd(*stat))

	    switch ((return_status)status)
		{
		case status_normal :
		  menu_stat = fdv__normal;
		  break;

		case status_error :
		  menu_stat = fdv__error;
		  break;

		case status_do :
		  menu_stat = fdv__do;
		  break;

		case status_save :
		  menu_stat = fdv__save;
		  break;

		case status_unsave :
		  menu_stat = fdv__unsave;
		  break;

		case status_return :
		  menu_stat = fdv__return;
		  break;

		case status_rewind :
		  menu_stat = fdv__rewind;
		  break;

		case status_quit :
		  menu_stat = fdv__quit;
		  break;

		case status_exit :
		  menu_stat = fdv__exit;
		  break;

		case status_retain :
		  menu_stat = fdv__retain;
		  break;
		}
	}
    else
	menu_stat = fdv__retain;

    if (odd(*stat))
	{
	if (cli_end_of_command())
	    {
	    cli_return (&ignore);
	    *stat = cli__return;
	    }
	else
	    /* maximum parameter count exceeded */
	    *stat = cli__maxparm;
	}
}



static void save_command (int *stat)

/*
 * save_command - evaluate save command
 */

{
    if (menu_depth != 0)
	{
	if (cli_end_of_command())
	    fdv_save (logging, stat);
	else
	    /* maximum parameter count exceeded */
	    *stat = cli__maxparm;
	}
    else
	write_command_file (stat);
}



static void set_command (int *stat)

/*
 * set_command - evaluate set command
 */

{
    int option;
    gli_string new_dir;
    gli_prompt_string new_prompt;

    cli_get_keyword ("What", set_options, &option, stat);

    if (odd(*stat))

	switch ((set_option)option)
	    {
	    case option_default :

	      cli_get_parameter ("Directory", new_dir, "", FALSE, TRUE, stat);
	      if (odd(*stat))
		  {
		  if (!chdir (new_dir))
		      *stat = RMS__NORMAL;
		  else
		      *stat = RMS__FND; /* file or directory lookup failed */
		  }
	      break;

	    case option_verify :
	    case option_noverify :

	      if (cli_end_of_command())
		  verify = (option == (int)(option_verify));
	      else
		  /* maximum parameter count exceeded */
		  *stat = cli__maxparm;
	      break;

	    case option_log :
	    case option_nolog :

	      if (cli_end_of_command())
		  logging = (option == (int)(option_log));
	      else
		  /* maximum parameter count exceeded */
		  *stat = cli__maxparm;
	      break;

	    case option_trace :
	    case option_notrace :

	      if (cli_end_of_command())
		  trace = (option == (int)(option_trace));
	      else
		  /* maximum parameter count exceeded */
		  *stat = cli__maxparm;
	      break;

	    case option_prompt :

	      if (!cli_end_of_command())
		  {
		  cli_get_parameter ("Prompt", new_prompt, "", FALSE, TRUE,
		    stat);

		  if (odd(*stat))
		      strcpy (prompt, str_remove (new_prompt, quotation_mark,
			  TRUE));
		  }
	      else
		  strcpy (prompt, default_prompt);
	      break;

	    case option_ansi :
	    case option_tek :

	      if (cli_end_of_command())
		  {
		  switch ((set_option)option)
		    {
		    case option_ansi:
		      tt_printf ("%s\n", ansi_escape[0]);
		      break;

		    case option_tek:
		      tt_printf ("%s\n", ansi_escape[1]);
		      break;
		    }
		  }
			
	      else
		  *stat = cli__maxparm; /* maximum parameter count exceeded */
	      break;

	    case option_tcl :
	    case option_notcl :

	      if (cli_end_of_command())
		  {
#ifdef TCL
		  switch ((set_option)option)
		    {
		    case option_tcl:
		      if (interp == NULL)
			create_tcl_interp ();

		      cli_b_tcl_interp = TRUE;
		      break;

		    case option_notcl:
		      cli_b_tcl_interp = FALSE;
		      break;
		    }
#else
		  tt_fprintf (stderr,
		      "Can't access Tcl/Tk software libraries.\n");
#endif
		  }
			
	      else
		  *stat = cli__maxparm; /* maximum parameter count exceeded */
	      break;

	    case option_help :
	    case option_nohelp :

	      if (cli_end_of_command())
		  tt_b_disable_help_key = (option == (int)(option_nohelp));
	      else
		  /* maximum parameter count exceeded */
		  *stat = cli__maxparm;
	      break;
	    }
}



static void rpc_command (int *stat)

/*
 * rpc_command - evaluate RPC command
 */

{
    gli_string command;
    gli_identifier variable[max_rpc_parm_count];
    var_variable_descr *addr;

    int argc, size[max_rpc_parm_count];
    float *data[max_rpc_parm_count];
    int ignore;

    cli_get_parameter ("Command", command, " ,", FALSE, TRUE, stat);

    argc = 0;
    while (!cli_end_of_command() && argc < max_rpc_parm_count)
	{
	cli_get_parameter ("Variable", variable[argc], " ,", FALSE, TRUE, stat);
	if (odd(*stat))
	    {
	    addr = var_address(variable[argc], stat);
	    if (odd(*stat))
		{
		size[argc] = addr->allocn;
		data[argc] = addr->data;
		++argc;
		}
	    }
	}

    if (odd(*stat))
	{
	if (cli_end_of_command())
	    {	 
	    gli_callrpc (command, argc, size, data);

	    sym_define ("RPC_RESULT", command, &ignore);

	    if (*command != '?') {
		while (argc--)
		    var_truncate (variable[argc], size[argc]+1, &ignore);

		if (*command && logging)
		    tt_printf ("  %s\n", command);
		}
	     else {
		tt_fprintf (stderr, "  %s\n", command + 1);
		*stat = cli__rpcerror; /* RPC command error */
		}
	    }
	else
	    *stat = cli__maxparm; /* maximum parameter count exceeded */
	}
}



static void show_symbol (int *stat)

/*
 * show_symbol - show symbol
 */

{
    gli_identifier candidate, symbol, pattern;
    sym_symbol_descr *context;
    gli_string equ_symbol;
    int n;

    do
	{
	if (cli_end_of_command())
	    {
	    strcpy (candidate, "*");
	    *stat = cli__normal;
	    }
	else
	    cli_get_parameter ("Symbol(s)", candidate, " ,", FALSE, TRUE, stat);

	if (odd(*stat))
	    {
	    context = NIL;
	    n = 0;
	    do
		{
		context = sym_inquire_symbol (context, symbol, equ_symbol,
		    stat);

		if (odd(*stat))
		    {
		    strcpy (pattern, symbol);
		    str_translate (pattern, '*', '|');

		    if (str_match (candidate, pattern, FALSE))
			{
			n = n+1;
			tt_printf ("  %s = \"%s\"\n", symbol, equ_symbol);
			}
		    }
		}
	    while (context != NIL);

	    if (n > 0)
		{
		tt_printf ("\n");
		tt_printf ("  Total of %d symbol%s.\n", n, pl_s[n>1]);
		}

	    if (odd(*stat) && (n == 0))
		*stat = sym__undsymb; /* undefined symbol */
	    }
	}
    while (odd(*stat) && !cli_end_of_command());
}



static void show_variable (int *stat)

/*
 * show_variable - show variable
 */

{
    gli_identifier variable_spec, variable;
    var_variable_descr *context;
    int allocation, segments, size;
    int n, total_values, total_size;

    do
	{
	if (cli_end_of_command())
	    {
	    strcpy (variable_spec, "*");
	    *stat = cli__normal;
	    }
	else
	    cli_get_parameter ("Variable(s)", variable_spec, " ,", FALSE, TRUE,
		stat);

	if (odd(*stat))
	    {
	    context = NIL;
	    n = 0;
	    total_values = 0;
	    total_size = 0;
	    do
		{
		context = var_inquire_variable (context, variable, &allocation,
		    &segments, &size, stat);

		if (odd(*stat))
		    if (str_match (variable, variable_spec, FALSE))
			{
			n = n+1;
			total_values += allocation;
			total_size += size;
			if (segments > 0)
			    tt_printf (
				"  %s (%d value%s in %d segment%s, %d bytes)\n",
				variable, allocation, pl_s[allocation>1],
				segments, pl_s[segments>1], size);
			else
			    tt_printf (
				"  %s (%d value%s, %d bytes)\n", variable,
				allocation, pl_s[allocation>1], size);
			}
		}
	    while (context != NIL);

	    if (n > 0)
		{
		tt_printf ("\n");
		tt_printf (
		    "  Total of %d variable%s, %d value%s, %d bytes.\n", n,
		    pl_s[n>1], total_values, pl_s[total_values>1], total_size);
		}

	    if (odd(*stat) && (n == 0))
		*stat = var__undefid; /* undefined identifier */
	    }
	}
    while (odd(*stat) && !cli_end_of_command());
}



static void show_function (int *stat)

/*
 * show_function - show function
 */

{
    gli_identifier function_spec, function_id;
    fun_function_descr *context;
    gli_string expression;
    int n;

    do
	{
	if (cli_end_of_command())
	    {
	    strcpy (function_spec, "*");
	    *stat = cli__normal;
	    }
	else
	    cli_get_parameter ("Function(s)", function_spec, " ,", FALSE, TRUE,
		stat);

	if (odd(*stat))
	    {
	    context = NIL;
	    n = 0;
	    do
		{
		context = fun_inquire_function (context, function_id,
		    expression, stat);

		if (odd(*stat))
		    if (str_match (function_id, function_spec, FALSE))
			{
			n = n+1;
			tt_printf ("  %s = %s\n", function_id, expression);
			}
		}
	    while (context != NIL);

	    if (n > 0)
		{
		tt_printf ("\n");
		tt_printf ("  Total of %d function%s.\n", n, pl_s[n>1]);
		}

	    if (odd(*stat) && (n == 0))
		*stat = fun__undefun; /* undefined function */
	    }
	}
    while (odd(*stat) && !cli_end_of_command());
}



static void show_logical (int *stat)

/*
 * show_logical - show logical command
 */

{
    char **envp;

    envp = environment;

    if (cli_end_of_command ())
	while (*envp)
	    tt_printf ("  %s\n", *envp++);
    else
	*stat = cli__maxparm; /* maximum parameter count exceeded */
}



static void show_command (int *stat)

/*
 * show_command - evaluate show command
 */

{
    int option;

    cli_get_keyword ("What", what_options, &option, stat);

    if (odd(*stat))

	switch ((what_option)option)
	    {
	    case option_symbol :
	      show_symbol (stat);
	      break;

	    case option_variable :
	      show_variable (stat);
	      break;

	    case option_function :
	      show_function (stat);
	      break;

	    case option_logical :
	      show_logical (stat);
	      break;
	    }
}



static void sleep_command (int *stat)
    
/*
 * sleep - evaluate sleep command
 */

{
    gli_string seconds;
    unsigned int sec;

    cli_get_parameter ("Suspension time", seconds, " ,", FALSE, TRUE, stat);

    if (odd(*stat))
	{
	if (cli_end_of_command())
	    {
	    sec = (unsigned int) (abs (cli_integer(seconds, stat)));
	    if (odd(*stat))
		{
		sleeping = TRUE;
		sleep (sec);
		sleeping = FALSE;
		}
	    }
	else
	    *stat = cli__maxparm;
	}
}



static void smooth_command (int *stat)

/*
 * smooth_command - evaluate smooth command
 */

{
    gli_identifier variable;
    var_variable_descr *addr;
    gli_string sm_level, seg_size;
    float *y, *smy;
    int i, key, n, level, size;

    cli_get_parameter ("Variable", variable, " ,", FALSE, TRUE, stat);

    if (odd(*stat))
	{
	n = 0;
	addr = var_address(variable, stat);

	if (odd(*stat))
	    {
	    level = 2;
	    size = max_points;

	    if (!cli_end_of_command())
		{
		cli_get_parameter ("Smoothing level", sm_level, " ,",
		    FALSE, TRUE, stat);

		if (odd(*stat))
		    {
		    level = cli_integer(sm_level, stat);

		    if (odd(*stat))
			{
			if (!cli_end_of_command())
			    {
			    cli_get_parameter ("Segment size", seg_size,
				" ,", FALSE, TRUE, stat);

			    if (odd(*stat))
				{
				size = cli_integer(seg_size, stat);

				if (odd(*stat))
				    if (!cli_end_of_command())
					*stat = cli__maxparm;
				}
			    }
			}
		    }
		}
	    }

	if (odd(*stat))
	    {
	    n = (size < addr->allocn) ? size : addr->allocn;
	    y = addr->data;
	    smy = (float *) malloc (sizeof(float) * n);

	    key = 0;
	    while (key < addr->allocn && !cli_b_abort)
		{
		if (key + n > addr->allocn)
		    n = addr->allocn - key;

		mth_smooth (n, y, smy, level);
		for (i=0; i<n; i++)
		    y[i] = smy[i];

		y += n;
		key += n;
		}

	    free (smy);
	    }
	}
}



static void split_command (int *stat)

/*
 * split_command - evaluate split command
 */

{
    gli_identifier source;
    var_variable_descr *addr, *sv;
    gli_string dest[max_inp_parm_count];
    int count, ns, ignore, length;
    float *buffer[max_inp_parm_count], *bufferP[max_inp_parm_count], *svP;
    register int i;

    sv = (var_variable_descr *) cli_get_variable ("Source Array", source, &ns,
	stat);

    count = 0;

    while (odd(*stat) && (count < max_inp_parm_count) &&
	   (!count || !cli_end_of_command()))
	{
	cli_get_parameter ("Destination Array(s)", dest[count], " ,", FALSE,
	    TRUE, stat);

	if (odd(*stat))
	    {
	    addr = var_address (dest[count], &ignore);
	    if (addr != NIL)
		var_delete (dest[count], stat);		      

	    count++;
	    }
	}

    if (odd(*stat))
	{
	if (!cli_end_of_command())
	    *stat = cli__maxparm; /* maximum parameter count exceeded */
	}

    if (odd(*stat))
	{
	for (i = 0; (i < count) && odd(*stat); i++)
	    {
	    length = ns / count + (i < (ns % count));
	    if (length)
		{
		buffer[i] = (float *) malloc (length * sizeof(float));
		bufferP[i] = buffer[i];
		}
	    }	     
	}

    if (odd(*stat))
	{
	svP = sv->data;
	for (i = 0; i < ns; i++)
	    {
	    *bufferP[i % count]++ = *svP++;
	    }
	}

    if (odd(*stat))
	{
	for (i = 0; (i < count) && odd(*stat); i++)
	    {
	    length = ns / count + (i < (ns % count));
	    if (length)
		{
		var_define (dest[i], 0, length, buffer[i], FALSE, stat);
		free (buffer[i]);
		}
	    }
	}
}



static void tek_import (char *file_spec)

/*
 * tek_import - import TEKtronix file
 */

{
    FILE *graphics_file;
    char input_line[record_size];
    int n, clear, skipped;

    clear = FALSE;
    skipped = 0;

    if (graphics_file = fopen (file_spec, "r"))
	{
	gus_plot10_initt ();

	memset((void *)input_line, 0, record_size);

	while (fgets (input_line, record_size, graphics_file) && !cli_b_abort)
	    {
	    n = record_size - 1;
	    while (n && input_line[n] == '\0')
		n--;

	    gus_plot10_adeout (&n, input_line, &clear);

	    if (logging && (n > 0))
		{
		input_line[n] = '\0';
		tt_printf ("%s", input_line);
		}

	    memset((void *)input_line, 0, record_size);
	    skipped += n;
	    }

	fclose (graphics_file);

	gus_plot10_finitt ();
	}

    if (logging && (skipped > 0))
	tt_printf ("  Textual information has been ignored (%d byte%s)\n",
	    skipped, pl_s[skipped>1]);
}



static void import_command (int *stat)

/*
 * import_command - evaluate import command
 */

{
    file_specification file_spec;
    int format, interface, picture = 0;

    get_file_specification (file_spec, "", stat);

    if (odd(*stat))
	{
	cli_get_keyword ("Format", import_formats, &format, stat);

	if (odd(*stat))
	    {
	    cli_get_keyword ("Interface", interfaces, &interface, stat);

	    if (odd(*stat))
		{	    
		if (cli_end_of_command())

		    switch ((import_format)format)
			{
			case format_tek :
			  tek_import (file_spec);
			  break;

			case format_cgm_binary :
			case format_cgm_clear_text :
			  cgm_import (file_spec, format, picture, interface);
			  break;
			}

		else
		    *stat = cli__maxparm; /* maximum parameter count exceeded */
		}
	    }
	}
}



static void write_command (int *stat, BOOL append)

/*
 * write_command - evaluate write command
 */

{
    FILE *data_file;
    file_specification file_spec;
    cli_data_descriptor data[max_outp_parm_count];
    int count, parm_count;
    float value;
    int n;
    BOOL segment;
    char *mode;

    get_file_specification (file_spec, ".dat", stat);
    if (*stat == RMS__FNF)
	*stat = RMS__NORMAL;

    if (odd(*stat))
	{
	count = 0;
	do
	    if (count < max_outp_parm_count)
		{
		cli_get_data ("Data", &data[count], stat);
		count = count+1;
		}
	    else
		*stat = cli__maxparm; /* maximum parameter count exceeded */

	while (odd(*stat) && !cli_end_of_command());

	if (odd(*stat))
	    {
	    mode = append ? "a" : "w";
	    data_file = fopen (file_spec, mode
#ifdef VAXC
		, "rop=wbh", "mbc=16");
#else
		);
#endif
	    if (!data_file)
		{
		*stat = RMS__ACC; /* ACP file access failed */
		return;
		}

	    if (append)
		fprintf (data_file, "\n");

	    parm_count = count;

	    n = 0;
	    count = 0;
	    segment = FALSE;

	    while (odd(*stat) && !cli_b_abort)
		{
		while (odd(*stat) && count < parm_count)
		    {
		    cli_get_data_buffer (&data[count], 1, &value, NIL, stat);

		    if (*stat == cli__eos)
			segment = TRUE;

		    if (odd(*stat))
			{
			if (data[count].data_type == type_symbol) {
			    if (count)
				fprintf (data_file, " ");
			    fprintf (data_file, "%s",
				data[count].type.symbol.equ);
			    }
			else
			    fprintf (data_file, " %g", value);

			count++;
			}
		    }

		if (segment || count == parm_count) {
		    n++;
		    fprintf (data_file, "\n");
		    count = 0;
		    }

		if (segment) {
		    n++;
		    fprintf (data_file, "\n");
		    count = 0;
		    segment = FALSE;
		    }
		}

	    if (*stat == cli__nmd)
		*stat = cli__normal;

	    if (count) {
		n++;
		fprintf (data_file, "\n");
		}

	    fclose (data_file);

	    if (logging && n > 0)
		tt_printf ("  %d line%s %s to file %s\n", n, pl_s[n>1],
		    file_mode[append], file_spec);
	    }
	}
}



static void redirect_input (void)

{
if (xui_present())
    {
    if (cli_a_input_routine == NIL) {
	cli_a_input_routine = (int (*)(char *, char *, char *, int *))
	    gli_input_routine;
	xui_open ();
	}
    else {
	xui_close ();
	cli_a_input_routine = NIL;
	}
    }
else
    tt_fprintf (stderr, "  Can't open display.\n");
}



static void get_choice (int *stat)

/*
 * get_choice - evaluate get choice command
 */

{
    gli_identifier symbol;
    gli_string choice_list, choice;
    BOOL more;

    cli_get_parameter ("Symbol", symbol, " ,", FALSE, TRUE, stat);

    if (odd(*stat))
	{
	sym_translate (symbol, choice, stat);
	if (*stat == sym__undsymb)
	    *stat = sym__normal;

	if (odd(*stat))
	    {
	    cli_get_parameter ("Choice list", choice_list, "",
		FALSE, TRUE, stat);

	    if (odd(*stat))
		{
		if (xui_get_choice (symbol, choice_list, choice, &more) >= 0)
		    sym_define (symbol, choice, stat);
		else
		    *stat = cli__eof;
		}
	    }
	}
}



static void get_value (int *stat)

/*
 * get_value - evaluate get value command
 */

{
    gli_identifier variable;
    var_variable_descr *addr;
    int value;
    float rvalue;

    cli_get_parameter ("Variable", variable, " ,", FALSE, TRUE, stat);

    if (odd(*stat))
	{
	addr = var_address(variable, stat);

	if (odd(*stat) || *stat == var__undefid || addr == NULL)
	    {
	    if (odd(*stat))
		var_delete (variable, NIL);

	    if (cli_end_of_command())
		{
		if (xui_get_value (variable, &value) >= 0)
		    {
		    rvalue = value;
		    var_define (variable, 0, 1, &rvalue, 0, stat);
		    }
		else
		    *stat = cli__eof;
		}
	    else
		*stat = cli__maxparm; /* maximum parameter count exceeded */
	    }
	}
}



static void xui_command (int *stat)

/*
 * xui_command - evaluate an XUI command
 */

{
    int option;

    if (!cli_end_of_command())
	{
	cli_get_keyword ("Function", x_options, &option, stat);

	if (odd(*stat))
	    {
	    if (xui_present())
		{
		xui_open ();

		switch ((x_option)option)
		    {
		    case x_choice :
			get_choice (stat);
			break;

		    case x_valuator :
			get_value (stat);
			break;
		    }
		xui_close ();
		}
	    else
		tt_fprintf (stderr, "  Can't open display.\n");
	    }
	}
    else
	redirect_input ();
}



static void evaluate_command (char *cmd_line, error_recovery error, int *status)

/*
 * evaluate_command - evaluate a GLI command
 */

{
    int index;
    gli_command command;
    gli_string line;

    cli_set_command (cmd_line, NIL);
    cli_get_command (prompt, gli_command_table, &index, status);

    if (odd(*status) && (*status != cli__empty))
	{
	cli_inquire_command (line, NIL, NIL);

	if (verify && (procedure_depth != 0))
	    tt_printf ("%s\n", line);

	command = command_define;

#ifdef TCL
	if (*status == cli__tcl) /* Tcl command */
	    evaluate_tcl_command (line, status);
	else
#endif
	if (*status == cli__symbol) /* symbol assignment */
	    define_symbol (status);

	else if (*status == cli__variable) /* variable assignment */
	    define_variable (status);

	else if (*status == cli__function) /* function assignment */
	    define_function (status);

	else
	    {
	    command = (gli_command)index;

	    switch (command)
		{
		case command_sight :
		    if (!interactive)
			if (cli_end_of_command())
			    /* command not allowed on current command level */
			    *status = cli__invcomd;
		    break;

		case command_help :
		case command_learn :
		case command_recover :
		    if (procedure_depth > 0)
			/* command not allowed on current command level */
			*status = cli__invcomd;

		case command_display :
		case command_inquire :
		case command_initialize :
		case command_load :
		case command_on :
		case command_simpleplot :
		case command_xui :
		    if (!interactive)
			/* command not allowed on current command level */
			*status = cli__invcomd;
		    break;

		default :
		    break;
		}

	    if (odd(*status))

		switch (command)
		    {
		    case command_dcl :
		      dcl_command (status);
		      break;

		    case command_execute_procedure :
		      execute_procedure (status);
		      break;

		    case command_append :
		      write_command (status, TRUE);
		      break;

		    case command_calculate :
		      calculate_command (status);
		      break;

		    case command_case :
		      case_command (status);
		      break;

		    case command_com :
		      com_command (status);
		      break;

		    case command_define :
		      define_command (status);
		      break;

		    case command_delete :
		      delete_command (status);
		      break;

		    case command_display :
		      display_command (status);
		      break;

		    case command_do :
		      do_command (status);
		      break;

		    case command_exit :
		      exit_command (status);
		      break;

		    case command_gks :
		      do_gks_command (status);
		      break;

		    case command_gosub :
		      gosub_command (status);
		      break;

		    case command_goto :
		      goto_command (status);
		      break;

		    case command_gridit :
		      gridit_command (status);
		      break;

		    case command_grsoft :
		      do_grsoft_command (status);
		      break;

		    case command_gus :
		      do_gus_command (status);
		      break;

		    case command_help :
		      help_command (status);
		      break;

		    case command_if :
		      if_command (status);
		      break;

		    case command_image :
		      do_image_command (status);
		      break;

		    case command_initialize :
		      initialize_command (status);
		      break;

		    case command_inquire :
		      inquire_command (status);
		      break;

		    case command_learn :
		      learn_command (status);
		      break;

		    case command_load :
		      load_command (status);
		      break;

		    case command_message :
		      message_command (status);
		      break;

		    case command_on :
		      on_command (status);
		      break;

		    case command_pipe :
		      pipe_command (status);
		      break;

		    case command_print :
		      print_command (status);
		      break;

		    case command_quit :
		      quit_command (status);
		      break;

		    case command_read :
		      read_command (status);
		      break;

		    case command_recover :
		      recover_command (status);
		      break;

		    case command_redim :
		      redim_command (status);
		      break;

		    case command_return :
		      return_command (status);
		      break;

		    case command_rpc :
		      rpc_command (status);
		      break;

		    case command_save :
		      save_command (status);
		      break;

		    case command_set :
		      set_command (status);
		      break;

		    case command_show :
		      show_command (status);
		      break;

		    case command_sight :
		      do_sight_command (status);
		      break;

		    case command_simpleplot :
		      simpleplot (status);
		      break;

		    case command_sleep :
		      sleep_command (status);
		      break;

		    case command_smooth :
		      smooth_command (status);
		      break;

		    case command_split :
		      split_command (status);
		      break;

		    case command_import :
		      import_command (status);
		      break;

		    case command_write :
		      write_command (status, FALSE);
		      break;

		    case command_xui :
		      xui_command (status);
		      break;
		    }
	    }

	if (journal && odd(*status) && procedure_depth == 0)
	    {
	    if (recovery_complete)
		{
		rewrite_journal_file ();
		recovery_complete = FALSE;
		}

	    switch (command)
		{
		case command_exit :
		case command_initialize :
		case command_learn :
		case command_quit :
		case command_recover :
		    break;

		default :
		    cli_inquire_command (line, NIL, NIL);
		    fprintf (journal_file, "%s\n", line);
		}
	    }
	}

    if (!odd(*status) && !cli_b_abort && !traceback &&
	*status != cli__eof && *status != cli__controlc &&
	error == recovery_message)
	show_error_message (*status);
}



int gli_do_command (char *command)

/*
 * gli_do_command - do a GLI command
 */

{
    int status;

    evaluate_command (command, recovery_message, &status);

    return (status);
}



#ifdef VAXC

static int exception_handler (
    CHF_R_SIGNAL_ARGS *sigargs, CHF_R_MECH_ARGS *mechargs)
{
    int ignore, value;

    if (LIB$MATCH_COND (&sigargs->name, &SS$_FLTDIV_F, &SS$_FLTOVF_F,
	&SS$_FLTDIV, &SS$_FLTOVF, &SS$_ROPRAND, &MTH$_SQUROONEG,
	&MTH$_FLOOVEMAT, &MTH$_FLOUNDMAT) || STATUS_CUST_DEF(sigargs->name) ||
	STATUS_FAC_NO(sigargs->name) == RMS$_FACILITY)
	{
	if (menu_depth != 0)
	    fdv_exit (&ignore);

	if (trace && !STATUS_CUST_DEF(sigargs->name))
	    {
	    LIB$SIG_TO_STOP (sigargs, mechargs);
	    return (SS$_RESIGNAL);
	    }

	if (STATUS_CUST_DEF(sigargs->name))
	    sigargs->name = sigargs->name & ~STS_M_SEVERITY | STS_K_WARNING;

	sigargs->arg_count -= 2;
	SYS$PUTMSG (sigargs, NULL, NULL, NULL);
	sigargs->arg_count += 2;

	if (!STATUS_CUST_DEF(sigargs->name))
	    {
	    value = 0;
	    longjmp (recover, value);
	    }

	return (SS$_CONTINUE);
	}
    else
	return (SS$_RESIGNAL);
}

#endif



static void usage (void)
{
    tt_fprintf (stderr, "Usage:	 \
gli [-c conid] [-d[isplay] display] [-t wstype] [-trace] [-pipe]\n%s%s\n"
#ifdef HAVE_SOCKETS
	, " [-listen]"
#else
	, ""
#endif
#ifdef TCL
	, " [-f[file] file]"
#else
	, ""
#endif
	);
    exit (-1);
}


static void parse_args (int argc, char **argv)
{
    char *option, *env;

    app_name = *argv++;

    while (option = *argv++)
	{
	if (!strcmp (option, "-trace"))
	    trace = TRUE;

	else if (!strcmp (option, "-pipe"))
	    buffered_io = FALSE;

#ifdef HAVE_SOCKETS
	else if (!strcmp (option, "-listen"))
	    listen = TRUE;
#endif
	else if (*argv)
	    {
	    if (!strcmp (option, "-c"))
		strcpy (conid, *argv++);

	    else if (!strcmp (option, "-d") || !strcmp (option, "-display")) {
		env = (char *) malloc (8 + strlen(*argv) + 1);

		strcpy (env, "DISPLAY=");
		strcat (env, *argv++);

		putenv (env);
		}
#ifdef TCL
	    else if (!strcmp (option, "-f") || !strcmp (option, "-file"))
		file = *argv++;
#endif
	    else if (!strcmp (option, "-t"))
		strcpy (wstype, *argv++);
	    else
		usage ();
	    }
	else
	    usage ();
	}
}




void exit_handler (void)
{
    static BOOL exiting = FALSE;
    int ignore;

    if (!exiting) {
	exiting = TRUE;

	SightAutoSave ();

	GECLKS ();

	fdv_exit (&ignore);
	cli_disconnect (&ignore);
#ifdef HAVE_SOCKETS
	sys_shutdown();
#endif

#if defined(_WIN32) && defined(__CYGWIN__)
	registry(0);
#endif
	}
}



static void message (char *signal, char *reason)
{
    tt_fprintf (stderr, "  GLI exiting due to a `%s' signal: %s.\n",
	signal, reason);
}


void signal_handler (int sig)
{
    switch (sig) {
#ifdef SIGABRT
	case SIGABRT : message ("ABRT", "abort"); break;
#endif
#ifdef SIGBUS
	case  SIGBUS : message ("BUS", "bus error"); break;
#endif
#ifdef SIGHUP
	case  SIGHUP : message ("HUP", "hangup"); break;
#endif
#ifdef SIGQUIT
	case SIGQUIT : break;
#endif
	case  SIGFPE : message ("FPE", "floating point exception"); break;
	case  SIGILL : message ("ILL", "illegal instruction"); break;
	case SIGSEGV : message ("SEGV", "segment violation"); break;
	case SIGTERM : break;
    }

    if (!trace)
    {
	if (sig != SIGSEGV)
	    exit_handler ();

	exit (-1);
    }
}


int main (int argc, char **argv, char **envp)

/*
 * gli - main program
 */

{
    int stat;

    environment = envp;

    strcpy (prompt, default_prompt);

    interactive = TRUE;
    verify = FALSE;
    procedure_depth = 0;
    menu_depth = 0;
    trace = FALSE;
#ifdef HAVE_SOCKETS
    listen = FALSE;
#endif
    buffered_io = TRUE;

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
#if defined(MSDOS) || defined(_WIN32)
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
#define os "UNIX"
#endif
#endif
#endif
#endif

#ifdef TCL
    progname = argv[0];
#endif

    parse_args (argc, argv);

    tt_printf ("\n\tG L I\n");
    tt_printf ("\t%s version 4.5 (%s)\n", machine, os);
    tt_printf ("\tpatchlevel 4.5.30, 13 Apr 2012\n\n");
    tt_printf ("\tCopyright @ 1986-1999, Josef Heinen, Jochen Werner,\
 Gunnar Grimm\n");
#ifdef GRSOFT
    tt_printf ("\tCopyright @ 1998, ZAM, Forschungszentrum Juelich GmbH\
 (GR-Software)\n");
#endif
#ifdef TCL
    tt_printf ("\n\tCopyright @ 1991-1994, The Regents of the University of\
 California.\n\tCopyright @ 1994-1998, Sun Microsystems, Inc. (Tcl/Tk)\n");
#endif
#ifdef ZLIB
    tt_printf ("\tCopyright @ 1995-1998, Jean-loup Gailly and Mark Adler\n");
#endif
    tt_printf ("\n\tSend bugs and comments to J.Heinen@FZ-Juelich.de\n\n");

#if defined(sun) && !defined(__SVR4)
    on_exit ((void (*)(void))exit_handler, NULL);
#else
    atexit ((void (*)(void))exit_handler);
#endif

    if (!trace) {
#ifdef SIGABRT
	signal (SIGABRT, (void (*)(int))signal_handler);
#endif
#ifdef SIGBUS
	signal (SIGBUS, (void (*)(int))signal_handler);
#endif
#ifdef SIGHUP
	signal (SIGHUP, (void (*)(int))signal_handler);
#endif
#ifdef SIGQUIT
	signal (SIGQUIT, (void (*)(int))signal_handler);
#endif
	signal (SIGFPE, (void (*)(int))signal_handler);
	signal (SIGILL, (void (*)(int))signal_handler);
	signal (SIGSEGV, (void (*)(int))signal_handler);
	signal (SIGTERM, (void (*)(int))signal_handler);
	}

#if defined(_WIN32) && defined(__CYGWIN__)
    registry(1);
#endif
    setup_env ();

    allocate_buffers ();
    if (!isatty(0) || file)
	{
	interactive = FALSE;
	batch_processor ();
	}

    journal_file = fopen ("gli.jou", "a+");
    if (!journal_file)
	{
	tt_fprintf (stderr, "  Can't open journal file.\n");
	exit (-1);
	}

    startup (&stat);

#ifdef VAXC
    VAXC$ESTABLISH (exception_handler);
#endif

    eof_count = 0;

    do
	{
	setjmp (recover);

	stat = cli__normal;
	
	procedure_depth = 0;
	menu_depth = 0;

	traceback = FALSE;
	cli_b_abort = FALSE;
	sleeping = FALSE;

	evaluate_command ("", recovery_message, &stat);

	if (stat == cli__eof)
	    eof_count = eof_count+1;
	else
	    eof_count = 0;
	}
    while (eof_count != 3);

    return 0;
}
