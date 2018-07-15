/*
 *
 * FACILITY:
 *
 *      VAX C Run-Time System
 *
 * ABSTRACT:
 *
 *      This module contains some generic interface routines for
 *      communication with ANSI terminal devices.
 *
 * AUTHOR:
 *
 *      J.Heinen
 *
 * VERSION:
 *
 *      V1.0-00
 *
 */


#ifdef VMS
#include <descrip.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>

#if !defined(VMS) && !defined(MSDOS) && !defined(_WIN32)
#include <unistd.h>
#endif

#ifdef TERMCAP
#include <term.h>
#endif

#if !defined(VMS) && !defined(MSDOS) && !defined(_WIN32)

#include <sys/ioctl.h>

#ifdef NO_TERMIO
#include <sgtty.h>
#else
#include <termios.h>
#endif

#endif

#define __tt 1

#include "terminal.h"
#include "strlib.h"
#include "system.h"


/* Global variables */

tt_cid *tt_a_cid = NULL;
unsigned char tt_b_disable_help_key = 0;


#define TCAPLEN 1024            /* length of termcap buffers */

#define TRUE 1
#define FALSE 0
#define BOOL int
#define NIL 0

#define odd(status) ((status) & 01)
#define mod(i, j) i >= 0 ? (i)%j : j+((i)%j)


/* Control characters */

#define nul '\0'                /* Null */
#define ctrl_a '\1'             /* <CTRL/A> */
#define ctrl_b '\2'             /* <CTRL/B> */
#define ctrl_c '\3'             /* <CTRL/C> */
#define ctrl_d '\4'             /* <CTRL/D> */
#define ctrl_e '\5'             /* <CTRL/E> */
#define ctrl_f '\6'             /* <CTRL/F> */
#define bs '\10'                /* Backspace */
#define ht '\11'                /* Tab */
#define lf '\12'                /* Line feed */
#define ctrl_l '\14'            /* <CTRL/L> */
#define cr '\15'                /* Carriage return */
#define ctrl_r '\22'            /* <CTRL/R> */
#define ctrl_u '\25'            /* <CTRL/U> */
#define ctrl_w '\27'            /* <CTRL/W> */
#define ctrl_z '\32'            /* <CTRL/Z> */
#define esc '\33'               /* Escape */
#define sp ' '                  /* Space bar */

#define space " "

#define question_mark '?'
#define tilde '~'
#define rubout '\177'           /* Rubout character */

#define xa0 '\240'

/* Control sequence introducers */

#define ss3 '\217'              /* <SS3>, same as <ESC>O */
#define csi '\233'              /* <CSI>, same as <ESC>[ */
#define none '\377'

/* Write function carriage control */

#define single_space_carriage_control 0x20
#define prompt_carriage_control 0x24


#ifdef TERMCAP
static char termbuf[TCAPLEN];       
#endif

/* Escape sequences */

static char *bl = "\7";             /* :bl=: audible signal (bell) */

static char *ks = "\33=";           /* :ks=: put terminal in keypad mode */
static char *ke = "\33>";           /* :ke=: end keypad mode */

static char *cs = "\33[%d;%dr";     /* :cs=: change scroll region */

static char *cm = "\33[%d;%dH";     /* :cm=: cursor motion */
static char *ce = "\33[K";          /* :ce=: clear to end of line */
static char *cl = "\33[H\33[J";     /* :cl=: clear screen and home cursor */

static char *le = "\10";            /* :le=: move cursor left one space */
static char *dc = "\33[P";          /* :dc=: delete character */
static char *im = "\33[4h";         /* :im=: insert mode start */
static char *ei = "\33[4l";         /* :ei=: end insert mode */

static char *mr = "\33[7m";         /* :mr=: reverse start */
static char *mb = "\33[5m";         /* :mb=: turn on blinking */
static char *md = "\33[1m";         /* :md=: turn on bold mode */
static char *me = "\33[m";          /* :me=: turn off all attributes */
static char *us = "\33[4m";         /* :us=: start underscore mode */

static char *sc = "\0337";          /* :sc=: save cursor */
static char *rc = "\0338";          /* :rc=: restore cursor */

#ifdef _WIN32
static int num_lines = 25;
#else
static int num_lines = 24;
#endif

static int line_count = 0;


char escape_table[256] =
{
    none, none, none, none, none, none, none, none, none, none,
    none, none, none, none, none, none, none, none, none, none,
    none, none, none, none, none, none, none, none, none, none,
    none, none, none, none, none, none, none, none, none, none,
    none, none, none, none, none, none, none, none, none, none,
    none, none, none, none, none, none, none, none, none, none,
    none, none, none, none, none,
    tt_k_ar_up, tt_k_ar_down, tt_k_ar_right, tt_k_ar_left,
    none, none, none, tt_k_home,
    none, none, none, none, tt_k_kp_ntr, none, none,
    tt_k_pf_1, tt_k_pf_2, tt_k_pf_3, tt_k_pf_4,
    none, none, none, none, none, none, none, none, none, none,
    none, none, none, none, none, none, none, none, none, none,
    none, none, none, none,
    tt_k_kp_com, tt_k_kp_hyp, tt_k_kp_per, none,
    tt_k_kp_0, tt_k_kp_1, tt_k_kp_2, tt_k_kp_3,
    tt_k_kp_4, tt_k_kp_5, tt_k_kp_6, tt_k_kp_7,
    tt_k_kp_8, tt_k_kp_9,
    none, none, none, none, none, none, none, none, none, none,
    none, none, none, none, none, none, none, none, none, none,
    none, none, none, none, none, none, none, none, none, none,
    none, none, none, none, none, none, none, none, none, none,
    none, none, none, none, none, none, none, none, none, none,
    none, none, none, none, none, none, none, none, none, none,
    none, none, none, none, none, none, none, none, none, none,
    none, none, none, none, none, none, none, none, none, none,
    none, none, none, none, none, none, none, none, none, none,
    none, none, none, none, none, none, none, none, none, none,
    none, none, none, none, none, none, none, none, none, none,
    none, none, none, none, none, none, none, none, none, none,
    none, none, none, none, none, none, none, none, none, none,
    none, none, none, none
};

char f_key[] = {
   tt_k_f1, tt_k_f2, tt_k_f3, tt_k_f4, tt_k_f5, none,
   tt_k_f6, tt_k_f7, tt_k_f8, tt_k_f9, tt_k_exit, none,
   tt_k_f11, tt_k_bol, tt_k_del_word, tt_k_ins_ovs, none,
   tt_k_help, tt_k_do, none,
   tt_k_f17, tt_k_f18, tt_k_f19, tt_k_f20
};

char *my_strncat(char *s1, const char *s2, int n)
{
  char *s = s1;
  int i = 0;

  while (*s1)
    s1++;
  while (i < n)
    {
      *s1++ = *s2++;
      i++;
    }
  *s1 = '\0';

  return s;
}

#ifdef TERMCAP

static
char *getstr (char *name, char *def)
{
    char *str;
    static char *cp = termbuf;

    str = (char *) tgetstr (name, &cp);
    if (str)
    {
        while (isdigit(*str))
            str++;
    }
    else
        str = def;

    return str;
}


static
int getnum (char *name, int def)
{
    int num;

    num = tgetnum (name);
    if (num == -1)
        num = def;

    return num;
}


static
void init_screen (void)
{
    char *term, termcap[TCAPLEN];

    if ((term = (char *) getenv("TERM")) != NULL)
    {
        if (tgetent (termcap, term) == 1)
        {
            bl = getstr ("bl", "\7");

            ks = getstr ("ks", "\33=");     ke = getstr ("ke", "\33>");
            cs = getstr ("cs", "\33[%d;%dr");

            cm = getstr ("cm", "\33[%d;%dH");
            ce = getstr ("ce", "\33[K");
            cl = getstr ("cl", "\33[H\33[J");

            le = getstr ("le", "\10");
            dc = getstr ("dc", "\33[P");
            im = getstr ("im", "\33[4h");   ei = getstr ("ei", "\33[4l");

            mr = getstr ("mr", "\33[7m");   me = getstr ("me", "\33[m");
            mb = getstr ("mb", "\33[5m");
            md = getstr ("md", "\33[1m");
            us = getstr ("us", "\33[4m");

            sc = getstr ("sc", "\0337");    rc = getstr ("rc", "\0338");

            num_lines = getnum ("li", 24);
        }
    }
}

#endif /* TERMCAP */


static
void get_char (int chan, char *ch, int time, int cancel)

/*
 *  get_char - get character from terminal
 */

{
    unsigned func;
    tt_r_quad iosb;

    func = IO__READVBLK | IO_M_NOFILTR | IO_M_NOECHO;
    if (time != 0)
        func = func | IO_M_TIMED;
    do
    {
        if (SYS_QIOW (0, chan, func, (short *)&iosb, 0, 0,
	    (unsigned char *)ch, 1, time, 0, 0, 0) != SS__NORMAL)
	    exit(-1);

        if (iosb.w0 == SS__TIMEOUT)
            *ch = tt_k_null;
        else if (iosb.w0 == SS__HANGUP)
            *ch = tt_k_hangup;
        else if (iosb.w0 == SS__CANCEL)
            *ch = tt_k_cancel;
        else if (iosb.w0 == SS__CONTROLC)
            *ch = tt_k_controlc;
    }
    while (!cancel && *ch == tt_k_cancel);
}


static
void get_key (int chan, char *ch, int time)

/*
 *  get_key - get key from terminal
 */

{
#ifndef _WIN32

    int param;

    do
    {
        get_char (chan, ch, time, TRUE);
        if (*ch == esc || *ch == ss3 || *ch == csi)
        {
            if (*ch == esc)
            {
                get_char (chan, ch, time, FALSE);
                if (*ch == '[' || *ch == 'O')
                    get_char (chan, ch, time, FALSE);
                else
                    /* allow <esc>1..4 to be recognized as PF1..PF4 */
                    if (*ch >= '1' && *ch <= '4') *ch = *ch-'0' + 'O';
            }
            else
                get_char (chan, ch, time, FALSE);

            if (*ch == '0')
                get_char (chan, ch, time, FALSE);

            if (*ch >= '0' && *ch <= '9')
            {
                param = *ch-'0';
                get_char (chan, ch, time, FALSE);
                if (*ch >= '0' && *ch <= '9')
                {
                    param = param*10 + *ch-'0';
                    get_char (chan, ch, time, FALSE);
                }
                if (*ch == 'q' || *ch == tilde || *ch == 'z')
                    {
                    if (param >= 1 && param <= 10)
                        *ch = f_key[param-1];
                    else if (param >= 11 && param <= 34)
                        *ch = f_key[param-11];
                    else if (param == 139)
                        *ch = tt_k_ins_ovs;
                    else if (param >= 224 && param <= 235)
                        *ch = f_key[param-224];
                    else
                        *ch = none;
                    }
                else
                    *ch = none;
            }
            else
                *ch = escape_table[(int)*ch];
        }
    }
    while (*ch == none);

#else

   get_char (chan, ch, time, TRUE);

#endif
}



static
void put_chars (int chan, char *chars)

/*
 *  put_chars - output characters to terminal
 */

{
    if (SYS_QIOW (0, chan, IO__WRITEVBLK | IO_M_NOFORMAT, 0, 0, 0,
        (unsigned char *)chars, strlen(chars), 0, 0, 0, 0) != SS__NORMAL)
	exit(-1);
}



static
void put_line (int chan, char *line)

/*
 *  put_line - output line to terminal
 */

{
    if (SYS_QIOW (0, chan, IO__WRITEVBLK, 0, 0, 0,
        (unsigned char *)line, strlen(line), 0, single_space_carriage_control,
	0, 0) != SS__NORMAL)
	exit(-1);
}



static
void put_prompt (int chan, char *prompt)

/*
 *  put_prompt - output prompt to terminal
 */

{
    if (SYS_QIOW (0, chan, IO__WRITEVBLK, 0, 0, 0,
        (unsigned char *)prompt, strlen(prompt), 0, prompt_carriage_control,
	0, 0) != SS__NORMAL)
	exit(-1);
}



static
void put_silo (tt_cid *cid, char *chars)

/*
 *  put_silo - put characters into output silo
 */

{
    char str[tt_k_cmd_length];

    if (strlen(cid->output_silo)+strlen(chars) >= tt_k_silo_size)
    {
        if (sc && rc)
        {
            strcpy (str, sc);
            strcat (str, cid->output_silo);
            strcat (str, rc);
        }
        else
            strcpy (str, cid->output_silo);

        put_chars (cid->chan, str);

        strcpy (cid->output_silo, chars);
    }
    else
        strcat (cid->output_silo, chars);
}



static
void dump_silo (tt_cid *cid)

/*
 *  dump_silo - dump output silo
 */

{
    char str[tt_k_cmd_length+3];

    if (strlen(cid->output_silo) > 0)
    {
        if (sc && rc)
        {
            strcpy (str, sc);
            strcat (str, cid->output_silo);
            strcat (str, rc);
        }
        else
            strcpy (str, cid->output_silo);

        if (me) strcat (str, me);

        put_chars (cid->chan, str);

        strcpy (cid->output_silo, "");
    }
}


#ifdef VMS
static struct dsc$descriptor dev =
{ 2, DSC$K_CLASS_S, DSC$K_DTYPE_T, "TT" };
#else
static char *dev = "tty";
#endif


tt_cid *tt_connect (char *lognam, int *status)

/*
 *  connect - connect terminal device
 */

{
    tt_cid *cid;
    int index, stat;

    cid = (tt_cid *) malloc (sizeof(tt_cid));
#ifndef VMS
    stat = SYS_ASSIGN (dev, &cid->chan, 0, 0);
#else
    stat = SYS_ASSIGN (&dev, &cid->chan, 0, 0);
#endif

    if (odd(stat))
    {
	tt_a_cid = cid;
	line_count = 0;

        strcpy (cid->output_silo, "");
        cid->read_index = 0;
        cid->write_index = 0;

        for (index = 0; index <= tt_k_cmd_buffer_size; index++)
            strcpy (cid->cmd_buffer[index], "");

#ifdef TERMCAP
        init_screen ();
#endif /* TERMCAP */

        strcpy (cid->mailbox_buffer, "");
        stat = tt__normal;
    }
    else
        stat = tt__confai; /* connection failure */

    if (!odd(stat))
    {
        free (cid);
        cid = NIL;
    }

    if (status != NIL)
        *status = stat;
    else
        if (!odd(stat))
            raise_exception (stat, 0, NULL, NULL);

    return (cid);
}



void tt_disconnect (tt_cid *cid, int *status)

/*
 *  disconnect - disconnect terminal device
 */

{
    int stat;

    if (cid != NIL)
        stat = SYS_DASSGN (cid->chan);
    else
        stat = tt__invconid; /* invalid connection identifier */

    if (odd(stat))
    {
	tt_a_cid = NULL;

        stat = tt__normal;
        free (cid);
    }
    else
        stat = tt__disconfai; /* disconnect failed */

    if (status != NIL)
        *status = stat;
    else
        if (!odd(stat))
            raise_exception (stat, 0, NULL, NULL);
}



void tt_put_chars (tt_cid *cid, char *chars, int line, int column,
    unsigned int attributes, int *status)

/*
 *  put_chars - output characters to terminal device
 */

{
    int stat;
    unsigned int attr;
    char str[tt_k_cmd_length];

    if (cid != NIL)
    {
#ifdef TERMCAP
        strcpy (str, tgoto (cm, column-1, line-1));
#else
        sprintf (str, cm, line, column);
#endif
        for (attr = 0; attr <= 3; attr++)
        {
            if (attributes & (1L << attr))
            {
                switch (attr) {
                    case 0: strcat (str, md); break;
                    case 1: strcat (str, us); break;
                    case 2: strcat (str, mb); break;
                    case 3: strcat (str, mr); break;
                }
            }
        }
        strcat (str, chars);
        strcat (str, me);
        stat = tt__normal;
    }
    else
        stat = tt__invconid; /* invalid connection identifier */

    if (odd(stat))
        put_silo (cid, str);

    if (status != NIL)
        *status = stat;
    else
        if (!odd(stat))
            raise_exception (stat, 0, NULL, NULL);
}



void tt_fflush (FILE *stream)
{
    if (stream != NULL)
	fflush (stream);
    else
	fflush (stdout);
}



void tt_flush (tt_cid *cid, int *status)

/*
 *  flush - flush output buffer
 */

{
    int stat;

    if (cid != NIL)
    {
	dump_silo (cid);
        stat = tt__normal;
    }
    else
        stat = tt__invconid; /* invalid connection identifier */

    if (status != NIL)
        *status = stat;
    else
        if (!odd(stat))
            raise_exception (stat, 0, NULL, NULL);
}



void tt_put_line (tt_cid *cid, char *line, char *prefix, char *postfix,
    int *status)

/*
 *  put_line - output buffer to terminal device
 */

{
    int stat;
    char str[tt_k_cmd_length];

    if (cid != NIL)
    {
        if (prefix != NIL || postfix != NIL)
        {
            strcpy (str, "");
            if (prefix != NIL) strcat (str, prefix);
            strcat (str, line);
            if (postfix != NIL) strcat (str, postfix);
            put_chars (cid->chan, str);
        }
        else
            put_line (cid->chan, line);

        stat = tt__normal;
    }
    else
        stat = tt__invconid; /* invalid connection identifier */

    if (status != NIL)
        *status = stat;
    else
        if (!odd(stat))
            raise_exception (stat, 0, NULL, NULL);
}



static
char *nltocr (char *s)
{
    register char *cp = s;

    while (*cp)
    {
	if (*cp == '\n')
	    *cp = '\r';
	++cp;
    }

    return s;
}



void tt_printf (char *format, ...)
{
    va_list args;
    char s[BUFSIZ], *cp;

    va_start (args, format);
    vsprintf (s, format, args);
    va_end (args);

    if (tt_a_cid != NULL)
    {
	if (strchr (s, '\n') != NULL)
	    put_line (tt_a_cid->chan, nltocr (s));
	else
	    put_chars (tt_a_cid->chan, s);
    }
    else
	printf (s);

    cp = s;
    while (*cp)
    {
	if (*cp++ == '\n')
	    line_count++;
    }
}



void tt_fprintf (FILE *stream, char *format, ...)
{
    va_list args;
    char s[BUFSIZ], *cp;

    va_start (args, format);
    vsprintf (s, format, args);
    va_end (args);

    if (stream == stdout || stream == stderr)
    {
	if (tt_a_cid != NULL)
	{
	    if (strchr (s, '\n') != NULL)
		put_line (tt_a_cid->chan, nltocr (s));
	    else
		put_chars (tt_a_cid->chan, s);
	}
	else
	    fprintf (stream, s);

	cp = s;
	while (*cp)
	{
	    if (*cp++ == '\n')
		line_count++;
	}
    }
    else
	fprintf (stream, s);
}



void tt_fputs (char *string, FILE *stream)
{
    if (stream == stdout || stream == stderr)
    {
	if (tt_a_cid != NULL)
	    put_chars (tt_a_cid->chan, string);
	else
	    fputs (string, stream);
    }
    else
	fputs (string, stream);
}



void tt_ring_bell (tt_cid *cid, int *status)

/*
 *  ring_bell - ring terminal bell
 */

{
    int stat;

    if (cid != NIL)
    {
        put_chars (cid->chan, bl);
        stat = tt__normal;
    }
    else
        stat = tt__invconid; /* invalid connection identifier */

    if (status != NIL)
        *status = stat;
    else
        if (!odd(stat))
            raise_exception (stat, 0, NULL, NULL);
}



void tt_get_key (tt_cid *cid, char *ch, int time, int *status)

/*
 *  get_key - get key from terminal
 */

{
    int stat;

    if (time < 0 || time > 25)
        stat = tt__badparam; /* bad parameter value */

    else if (cid != NIL)
    {
        get_key (cid->chan, ch, time);

        switch (*ch)
        {
#ifndef VMS
            case ctrl_d :
#endif
            case ctrl_z :
            case tt_k_exit :        stat = tt__eof; break;
            case tt_k_null :        stat = tt__timeout; break;
            case tt_k_hangup :      stat = tt__hangup; break;
            case tt_k_cancel :      stat = tt__cancel; break;
            case tt_k_controlc :    stat = tt__controlc; break;
            default :               stat = tt__normal;
        }
    }
    else
        stat = tt__invconid; /* invalid connection identifier */

    if (status != NIL)
        *status = stat;
    else
        if (!odd(stat))
            raise_exception (stat, 0, NULL, NULL);
}



int tt_getchar (void)
{
    char ch;

    if (tt_a_cid != NULL)
    {
	get_char (tt_a_cid->chan, &ch, 0, TRUE);
	return (int) ch;
    }
    else
	return getchar ();
}



void tt_cancel (tt_cid *cid, char *message, int *status)

/*
 *  cancel - cancel read request
 */

{
    int stat;

    if (cid != NIL)
    {
        strcpy (cid->mailbox_buffer, message);
        stat = SYS_CANCEL (cid->chan);

        if (odd(stat))
            stat = tt__normal;
        else
            stat = tt__canfai; /* cancel failed */
    }
    else
        stat = tt__invconid; /* invalid connection identifier */

    if (status != NIL)
        *status = stat;
    else
        if (!odd(stat))
            raise_exception (stat, 0, NULL, NULL);
}



static
void redisplay_line (int chan, int index, char *prompt, char *line)

/*
 *  redisplay_line - redisplay input line
 */

{
    char str[tt_k_cmd_length];

    strcpy (str, "\r");
    strcat (str, ce);
    strcat (str, prompt);
    strcat (str, line);

    if (index == 0)
        index = strlen(line);
    else
    {
        strcat (str, "\r");
        strcat (str, prompt);
        my_strncat (str, line, index);
    }

    put_chars (chan, str);
}




void tt_write_buffer (tt_cid *cid, char *line, int append, int *status)

/*
 *  write_buffer - put line into command buffer
 */

{
    int stat;

    if (cid != NIL)
    {
        str_remove (line, ' ', TRUE);

        if ((strcmp (line, cid->cmd_buffer[cid->write_index]) != 0) &&
            (strlen(line) != 0))
        {
            if (!append)
            {
              cid->write_index = mod(cid->write_index+1, tt_k_cmd_buffer_size);
              strcpy (cid->cmd_buffer[cid->write_index], line);
          }
            else
            {
              strcat (cid->cmd_buffer[cid->write_index], space);
              strcat (cid->cmd_buffer[cid->write_index], line);
          }
        }

        stat = tt__normal;
    }
    else
        stat = tt__invconid; /* invalid connection identifier */

    if (status != NIL)
        *status = stat;
    else
        if (!odd(stat))
            raise_exception (stat, 0, NULL, NULL);
}



void tt_get_line (tt_cid *cid, char *prompt, char *line, int time,
    int echo, int append, char *terminator, int *status)

/*
 *  get_line - get a line of input
 */

{
  int stat;
  BOOL delimiter, end_of_list, insert_mode;
  int recall_count, index;
  char ch;
  char chars[tt_k_cmd_length];
  int get_index;
  char str[tt_k_cmd_length];

  if (time < 0 || time > 25)
    stat = tt__badparam; /* bad parameter value */

  else if (cid != NIL)
  {
    stat = tt__normal;
    put_prompt (cid->chan, prompt);
    cid->read_index = cid->write_index;

    strcpy (chars, "");
    get_index = 0;
    delimiter = FALSE;
#ifdef _WIN32
    insert_mode = FALSE;
#else
    insert_mode = TRUE;
#endif
    end_of_list = FALSE;
    recall_count = 0;

    do
    {
      get_key (cid->chan, &ch, time);

      if (echo)

        switch (ch) {

          case ctrl_a :
          case tt_k_ins_ovs :
#ifndef _WIN32
            insert_mode = !insert_mode;
#endif
            break;

          case ctrl_b :
          case tt_k_ar_up :
            if (!end_of_list)
            {
              if (recall_count > 0)
              {
                cid->read_index = mod(cid->read_index-1, tt_k_cmd_buffer_size);
                end_of_list = (cid->read_index == cid->write_index);
              }
              if (!end_of_list)
              {
                strcpy (chars, cid->cmd_buffer[cid->read_index]);
                end_of_list = (strlen(chars) == 0);
              }
              else
                strcpy (chars, "");
              recall_count = recall_count+1;
            }
            else
              strcpy (chars, "");

            redisplay_line (cid->chan, 0, prompt, chars);
            get_index = strlen (chars);
            break;

          case tt_k_ar_down :
            if (recall_count > 0)
            {
              if (recall_count > 1)
              {
                cid->read_index = mod(cid->read_index+1, tt_k_cmd_buffer_size);
                strcpy (chars, cid->cmd_buffer[cid->read_index]);
              }
              else
                strcpy (chars, "");
              end_of_list = FALSE;
              recall_count = recall_count-1;
            }
            else
              strcpy (chars, "");

            redisplay_line (cid->chan, 0, prompt, chars);
            get_index = strlen (chars);
            break;

#ifdef VMS
          case ctrl_d :
#endif
          case tt_k_ar_left :
            if (get_index > 0)
            {
              get_index = get_index-1;
              if (chars[get_index] == ht)
                redisplay_line (cid->chan, get_index, prompt, chars);
              else
                put_chars (cid->chan, "\b");
            }
            break;

          case ctrl_f :
          case tt_k_ar_right :
            if (get_index < strlen(chars))
            {
              str[0] = chars[get_index];
              str[1] = nul;
              put_chars (cid->chan, str);
              get_index = get_index+1;
            }
            break;

#ifdef VMS
          case bs :
#endif
          case tt_k_bol :
            get_index = 0;
            strcpy (str, "\r");
            strcat (str, prompt);
            put_chars (cid->chan, str);
            break;

          case ctrl_e :
            if (get_index < strlen(chars))
            {
              index = get_index;
              get_index = strlen(chars);
              put_chars (cid->chan, &chars[index]);
            }
            break;

          case tt_k_del_word :
            /* not yet supported */
            break;

          case ctrl_l :
          case ctrl_w :
            put_chars (cid->chan, cl);
            redisplay_line (cid->chan, get_index, prompt, chars);
            break;

          case ctrl_u :
            if (get_index < strlen(chars))
              strcpy (chars, &chars[get_index]);
            else
              strcpy (chars, "");
            redisplay_line (cid->chan, 0, prompt, chars);
            break;

          case ctrl_r :
            redisplay_line (cid->chan, get_index, prompt, chars);
            break;

#ifndef VMS
          case bs :
#endif
          case rubout :
            if (get_index > 0)
            {
              index = get_index-1;
              ch = chars[index];

              if (index < strlen(chars))
              {
                chars[index] = nul;
                strcpy (str, chars);            
                strcat (str, &chars[get_index]);
                strcpy (chars, str);
              }
              else
                chars[index] = nul;

              get_index = index;
              if (ch == ht)
                redisplay_line (cid->chan, get_index, prompt, chars);
              else {
                strcpy (str, le);
#ifdef _WIN32
                strcat (str, chars + index);
                strcat (str, " \r");
                strcat (str, prompt);
                my_strncat (str, chars, index);
#else
                strcat (str, dc);
#endif
                put_chars (cid->chan, str);
              }
            }
            break;

          case tt_k_controlc :
          case tt_k_f6 :
            delimiter = TRUE;
            if (strlen(cid->mailbox_buffer) != 0)
            {
              strcpy (chars, cid->mailbox_buffer);
              strcpy (cid->mailbox_buffer, "");
              put_chars (cid->chan, chars);
              stat = tt__normal;
            }
            else
            {
#ifndef VMS
              sprintf (str, "\n%s Cancel %s\n%s", mr, me, POSTFIX);
              put_chars (cid->chan, str);
#endif
              stat = tt__controlc;
            }
            break;

          case tt_k_null :
            delimiter = TRUE;
            stat = tt__timeout;
            break;

          case tt_k_hangup :
            delimiter = TRUE;
            stat = tt__hangup;
            break;

          case tt_k_cancel :
            delimiter = TRUE;
            stat = tt__cancel;
            break;

#ifndef VMS
          case ctrl_d :
#endif
          case ctrl_z :
          case tt_k_exit :
            delimiter = TRUE;
            sprintf (str, "%s Exit %s%s", mr, me, POSTFIX);
            put_chars (cid->chan, str);
            stat = tt__eof;
            break;

          case tt_k_do :
            delimiter = TRUE;
            put_chars (cid->chan, cl);
            stat = tt__normal;
            break;

          case lf :
          case cr :
            delimiter = TRUE;
            put_chars (cid->chan, POSTFIX);
            stat = tt__normal;
            break;

          case tt_k_find :
          case tt_k_insert :
          case tt_k_remove :
          case tt_k_select :
          case tt_k_prev_scr :
          case tt_k_next_scr :
          case tt_k_f17 :
          case tt_k_f18 :
          case tt_k_f19 :
          case tt_k_f20 :
          case tt_k_pf_1 :
          case tt_k_pf_2 :
          case tt_k_pf_3 :
          case tt_k_pf_4 :
            strcpy (str, POSTFIX);
            strcat (str, prompt);
            strcat (str, chars);
            put_chars (cid->chan, str);
            get_index = strlen(chars);
            break;

          case question_mark :
          case tt_k_help :
            if (!tt_b_disable_help_key)
	    {
	      delimiter = TRUE;
	      put_chars (cid->chan, POSTFIX);
	      stat = tt__help;
	      break;
	    }

          default :
            if (ch > 0 && ch < sp && ch != ht)
            {
              delimiter = TRUE;
              stat = tt__normal;
            }
            else
            {
              if (get_index < strlen(chars))
              {
                if (insert_mode)
                {
                  strcpy (str, "");
                  my_strncat (str, chars, get_index);
                  my_strncat (str, &ch, 1);
                  strcat (str, &chars[get_index]);
                  strcpy (chars, str);
                }
                else
                  chars[get_index] = ch;
              }
              else
                my_strncat (chars, &ch, 1);

              if (insert_mode)
              {
                strcpy (str, im);
                my_strncat (str, &ch, 1);
                strcat (str, ei);
                put_chars (cid->chan, str);
              }
              else
              {
                str[0] = ch;
                str[1] = nul;
                put_chars (cid->chan, str);
              }
              get_index = get_index+1;
            }
            break;
      }

      else

        switch (ch) {

          case tt_k_null :
            delimiter = TRUE;
            stat = tt__timeout;
            break;

          case tt_k_hangup :
            delimiter = TRUE;
            stat = tt__hangup;
            break;

          case tt_k_cancel :
            delimiter = TRUE;
#ifndef VMS
            sprintf (str, "\n%s Cancel %s\n%s", mr, me, POSTFIX);
            put_chars (cid->chan, str);
#endif
            stat = tt__cancel;
            break;

#ifndef VMS
          case bs :
#endif
          case rubout :
            if (get_index > 0)
            {
              get_index = get_index-1;
              chars[get_index] = nul;
            }
            break;

          default :
            if (ch >= sp)
            {
              my_strncat (chars, &ch, 1);
              get_index = get_index+1;
            }
            else
            {
              delimiter = TRUE;
              put_chars (cid->chan, POSTFIX);
              stat = tt__normal;
            }
        }
    }
    while (!delimiter);

    if (echo)
      tt_write_buffer (cid, chars, append, 0);

    strcpy (line, chars);

    if (terminator != NIL)
      *terminator = ch;
  }
  else
    stat = tt__invconid; /* invalid connection identifier */

  if (status != NIL)
    *status = stat;
  else
    if (!odd(stat))
      raise_exception (stat, 0, NULL, NULL);
}



char *tt_fgets (char *string, int n, FILE *stream)
{
    int stat;

    if (stream == stdin)
    {
	if (tt_a_cid != NULL)
	{
	    tt_get_line (tt_a_cid, "", string, 0, TRUE, FALSE, NULL, &stat);
	    return odd(stat) ? string : NULL;
	}
	else
	    return fgets (string, n, stream);
    }
    else
	return fgets (string, n, stream);
}



void tt_clear_line (tt_cid *cid, int line, int *status)

/*
 *  clear_line - clear specified line
 */

{
    int stat;
    char str[tt_k_cmd_length];

    if (cid != NIL)
    {
#ifdef TERMCAP
        strcpy (str, tgoto (cm, 0, line-1));
#else
        sprintf (str, cm, line, 0);
#endif
        strcat (str, ce);
        put_silo (cid, str);
        stat = tt__normal;
    }
    else
        stat = tt__invconid; /* invalid connection identifier */

    if (status != NIL)
        *status = stat;
    else
        if (!odd(stat))
            raise_exception (stat, 0, NULL, NULL);
}



void tt_clear_disp (tt_cid *cid, int *status)

/*
 *  clear_disp - clear display
 */

{
    int stat;

    if (cid != NIL)
    {
        put_chars (cid->chan, cl);
	line_count = 0;

        stat = tt__normal;
    }
    else
        stat = tt__invconid; /* invalid connection identifier */

    if (status != NIL)
        *status = stat;
    else
        if (!odd(stat))
            raise_exception (stat, 0, NULL, NULL);
}



void tt_clear_home (tt_cid *cid, int *status)

/*
 *  clear_home - clear display and move cursor to home position
 */

{
    int stat;
    char str[tt_k_cmd_length];

    if (cid != NIL)
    {
        put_chars (cid->chan, cl);
	line_count = 0;
#ifdef TERMCAP
        strcpy (str, tgoto (cm, 0, num_lines - 1));
#else
        sprintf (str, cm, num_lines, 0);
#endif
        put_chars (cid->chan, str);

        stat = tt__normal;
    }
    else
        stat = tt__invconid; /* invalid connection identifier */

    if (status != NIL)
        *status = stat;
    else
        if (!odd(stat))
            raise_exception (stat, 0, NULL, NULL);
}



void tt_more (char *format, ...)
{
    va_list args;
    char s[BUFSIZ], *cp;
    char ch;

    va_start (args, format);
    vsprintf (s, format, args);
    va_end (args);

    if (line_count >= num_lines - 1)
    {
	for (;;)
	{
	    char cl[10];

	    sprintf(cl, "\r%s", ce);
	    if (tt_a_cid != NULL)
	    {
		put_chars (tt_a_cid->chan, "--More--");
		get_key (tt_a_cid->chan, &ch, 0);
		put_chars (tt_a_cid->chan, cl);
	    }
	    else
	    {
		printf ("--More--");
		fflush (stdout);
		getchar ();
		printf (cl);
	    }		
	    if (ch == '\r')
	    {
		line_count--;
		break;
	    }
	    else if (ch == ' ')
	    {
		line_count = 0;
		break;
	    }
	}
    }

    if (tt_a_cid != NULL)
        put_chars (tt_a_cid->chan, s);
    else
	printf (s);

    cp = s;
    while (*cp)
    {
	if (*cp++ == '\n')
	    line_count++;
    }
}



void tt_set_terminal (tt_cid *cid, unsigned int mode, int ansi,
    int first, int last, int *status)

/*
 *  set_terminal - set up terminal
 */

{
    int stat;
    char str[tt_k_cmd_length];

    if (cid != NIL)
    {
        switch (mode)
        {
            case mode_numeric :
              strcpy (str, ke);
              break;
            case mode_application :
              strcpy (str, ks);
              break;
            default :
              strcpy (str, "");
              break;
        }
        if (ansi)
            strcat (str, "\33<");

        put_chars (cid->chan, str);

        if (!first)
            first = 1;
        if (!last)
        {
#if defined (TIOCGWINSZ)
            struct winsize win;
            if (ioctl(fileno(stdout), TIOCGWINSZ, &win) != -1)
                last = win.ws_row;
            else
#endif
#ifdef _WIN32
		last = 25;
#else
		last = 24;
#endif
        }
        num_lines = last;

#ifdef TERMCAP
        strcpy (str, tgoto (cs, last-1, first-1));
#else
        sprintf (str, cs, first, last);
#endif
        put_chars (cid->chan, str);

#ifdef TERMCAP
        strcpy (str, tgoto (cm, 0, last-1));
#else
        sprintf (str, cm, last-1, 0);
#endif
        put_chars (cid->chan, str);

        stat = tt__normal;
    }
    else
        stat = tt__invconid; /* invalid connection identifier */

    if (status != NIL)
        *status = stat;
    else
        if (!odd(stat))
            raise_exception (stat, 0, NULL, NULL);
}
