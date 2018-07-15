/*
 *
 * FACILITY:
 *
 *      VAX C Run-Time System
 *
 * ABSTRACT:
 *
 *      This module contains a command language interpreter.
 *
 * AUTHOR:
 *
 *      Josef Heinen
 *
 * VERSION:
 *
 *      V1.0-00
 *
 */

#if defined(RPC) || (defined(_WIN32) && !defined(__GNUC__))
#define HAVE_SOCKETS
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <signal.h>

#ifdef VMS
#include <descrip.h>
#endif

#ifdef cray
#include <fortran.h>
#endif

#define __cli 1

#define BOOL int
#define TRUE 1
#define FALSE 0
#define NIL 0
#define MAXARGS 16

#ifndef caddr_t
#define caddr_t char *
#endif


/* Global variables */

int (*cli_a_input_routine)(char *, char *, char *, int *) = NIL;
int cli_b_abort = FALSE;
int cli_b_tcl_interp = FALSE;
int cli_b_tcl_got_partial = FALSE;


#include "system.h"
#include "strlib.h"
#include "symbol.h"
#include "variable.h"
#include "function.h"
#include "terminal.h"
#include "command.h"


#define null '\0'
#define tab '\t'
#define delimiter '!'
#define underscore '_'
#define colon ':'
#define apostrophe '\''
#define at_sign '@'
#define dollar_sign '$'
#define blank ' '
#define slash '/'
#define question_mark '?'
#define comma ','
#define semicolon ';'
#define left_bracket '('
#define right_bracket ')'
#define equal_sign '='
#define quotation_mark '\"'
#define asterisk '*'

#define ht "\11"


#define line_width 80
#define string_length 255

#define odd(status) ((status) & 01)
#define round(x) (((x)<0) ? (int)((x)-0.5) : (int)((x)+0.5)) 


enum match_result {unrecognized, normal, ambiguous};


typedef struct
{
    union
    {
        int *iaddr;
        float *faddr;
        caddr_t caddr;
#ifdef VMS
	struct dsc$descriptor_s *descr;
#else
#ifdef cray
        _fcd descr;
#else
	int len;
#endif /* cray */
#endif /* VMS */
    } u;
} command_arg;

typedef struct
{
    union
    {
        int inum;
        float fnum;
	char str[string_length];
    } u;
#ifdef VMS
    struct dsc$descriptor_s text;
#endif
#ifdef cray
    _fcd text;
#endif
} arg_stack;

static command_arg argv[MAXARGS];
static arg_stack sp[MAXARGS];
static int len[MAXARGS];


static cli_string cmd_line;
static int cmd_length;
static int read_index, parse_index;

static BOOL more, outstanding_help = FALSE;
static cli_procedure current_procedure = NIL;


static tt_cid *conid = NIL;


char *app_name;
int max_points;
float *px, *py, *pz, *pe1, *pe2, z_min;



static
char read_character (void)

/*
 * read_character - get next character from command line
 */

{
    if (read_index < cmd_length)
	return cmd_line[read_index++];
    else
	return null;
}



static
char next_character (void)

/*
 * next_character - return next character from command line
 */

{
    if (read_index < cmd_length)
	return cmd_line[read_index];
    else
	return null;
}



static
BOOL end_of_command (void)

/*
 * end_of_command - test for end of command stream
 */

{
    while (cmd_line[read_index] == blank || cmd_line[read_index] == tab)
	read_index++;
	
    return (cmd_line[read_index] == delimiter || cmd_line[read_index] == null ||
	read_index >= cmd_length);
}



static
void get_label (char *command_label)

/*
 * get_label - get command label
 */

{
    char ch, *cp;

    cp = command_label;
    ch = read_character();

    while (ch == blank || ch == tab)
    {
	*cp++ = ch;
	ch = read_character();
    }

    parse_index = read_index;

    while (isalnum(ch) || ch == '_' || ch == '$')
    {
	*cp++ = ch;
	ch = read_character();
    }

    while (ch == blank || ch == tab)
    {
	*cp++ = ch;
	ch = read_character();
    }

    if ((ch == colon && next_character() != equal_sign) && *command_label)
    {
	*cp++ = ch;

	while (next_character() == blank || next_character() == tab)
	{
	    *cp++ = ch;
	    ch = read_character();
	}
    }
    else
    {
	read_index = 0;
	cp = command_label;
    }

    *cp = '\0';
}



static
void get_item (char *item, cli_delim_set delimiters, BOOL then_keyword,
    char *delim)

/*
 * get_item - get item from command line
 */

{
    static char white_space[] = "! \t\0";

    cli_delim_set set;
    char ch, *cp;
    cli_string object;
    int then_placement;
    BOOL quoted_string;

    strcpy (set, delimiters);

    ch = read_character();
    while (ch == blank || ch == tab)
	ch = read_character();

    parse_index = read_index;

    if (!then_keyword)
    {
	strcat (set, "!");

	if (strchr (set, blank) != 0)
	    strcat (set, ht);

	cp = item;
	quoted_string = FALSE;

	while (ch && (quoted_string || (strchr (set, ch) == 0)))
	{
	    if (ch == quotation_mark && next_character() == quotation_mark)
	    {
		*cp++ = ch;
		ch = read_character();
	    }
	    else if (ch == quotation_mark)
		quoted_string = !quoted_string;

	    *cp++ = ch;
	    ch = read_character();
	}

	*cp = '\0';

	if (delim != NIL)
	    *delim = ch;

	if (strchr (set, blank) != 0)
	{
	    while (next_character() == blank || next_character() == tab)
		ch = read_character();

	    if (strchr (set, next_character()) != 0 &&
		strchr (white_space, next_character()) == 0)
		ch = read_character();

	    if (ch == colon && next_character() == equal_sign)
		ch = read_character();
	}
    }
    else
    {
	cp = object;

	while (ch && ch != delimiter)
	{
	    *cp++ = ch;
	    ch = read_character();
	}

	*cp = '\0';

	str_cap (object);

	then_placement = str_index (object, "THEN");

	read_index = parse_index + then_placement - 1;

	if (then_placement >= 0)
	    read_index = read_index + 4;
	else
	    then_placement = 1;

	*item = '\0';
	strncat (item, object, then_placement-1);
    }
}



static
float get_real_constant (char *data_spec, char *ch, int *read_index, int *stat)

/*
 * get_real_constant - get real constant
 */

{
    static char num[] = "0123456789Ee+-.";
    cli_string chars;
    char *cp;

    while (*ch == blank || *ch == tab)
	*ch = data_spec[(*read_index)++];

    cp = chars;

    while (*ch && strchr (num, *ch) != 0 &&
	strncmp (&data_spec[*read_index-1], "..", 2) != 0)
    {
	*cp++ = *ch;
	*ch = data_spec[(*read_index)++];
    }

    *cp = '\0';

    while (*ch == blank || *ch == tab)
	*ch = data_spec[(*read_index)++];

    return (str_real (chars, stat));
}



static
float get_real (char *data_spec, char *ch, int *read_index, int *stat)

/*
 * get_real - get real value
 */

{
    cli_string name;
    char *cp;
    var_variable_descr *variable;
    float result = 0;

    while (*ch == blank || *ch == tab)
	*ch = data_spec[(*read_index)++];

    cp = name;

    if (isalpha(*ch))
	{
	while (isalnum(*ch) || *ch == '_' || *ch == '$')
	    {
	    *cp++ = *ch;
	    *ch = data_spec[(*read_index)++];
	    }

	*cp = '\0';

	while (*ch == blank || *ch == tab)
	    *ch = data_spec[(*read_index)++];

	variable = var_address (name, stat);
	if (odd(*stat))
	    var_examine (variable, &result, stat);
	}
    else
	result = get_real_constant (data_spec, ch, read_index, stat);

    return result;
}



static
int get_integer_constant (char *data_spec, char *ch, int *read_index, int *stat)

/*
 * get_integer_constant - get integer constant
 */

{
    static char num[] = "0123456789+-";
    static char digits[] = "0123456789ABCDEFabcdef";
    static char base[] = "bBoOxX";
    cli_string chars;
    char *cp;

    while (*ch == blank || *ch == tab)
	*ch = data_spec[(*read_index)++];

    cp = chars;

    if (*ch && strchr (num, *ch) != 0)
    {
	*cp++ = *ch;
	*ch = data_spec[(*read_index)++];

	if (*ch && strchr (base, *ch) != 0)
	{
	    *cp++ = *ch;
	    *ch = data_spec[(*read_index)++];
	}

	while (*ch && strchr (digits, *ch) != 0)
	{
	    *cp++ = *ch;
	    *ch = data_spec[(*read_index)++];
	}
    }

    *cp = '\0';

    while (*ch == blank || *ch == tab)
	*ch = data_spec[(*read_index)++];

    return (str_integer (chars, stat));
}



static
int get_integer (char *data_spec, char *ch, int *read_index, int *stat)

/*
 * get_integer - get integer value
 */

{
    cli_string name;
    char *cp;
    float component;
    var_variable_descr *variable;
    int result = 0;

    while (*ch == blank || *ch == tab)
	*ch = data_spec[(*read_index)++];

    cp = name;

    if (isalpha(*ch))
	{
	while (isalnum(*ch) || *ch == '_' || *ch == '$')
	    {
	    *cp++ = *ch;
	    *ch = data_spec[(*read_index)++];
	    }

	*cp = '\0';

	while (*ch == blank || *ch == tab)
	    *ch = data_spec[(*read_index)++];

	variable = var_address (name, stat);
	if (odd(*stat))
	    {
	    var_examine (variable, &component, stat);
	    result = (int)component;
	    }
	}
    else
	result = get_integer_constant (data_spec, ch, read_index, stat);

    return result;
}



static
cli_data_descriptor *data_descriptor (cli_data_descriptor *data_dsc,
    char *data_spec, int *stat)

/*
 *  data_descriptor - return data descriptor
 */

{
    int read_index;
    char ch, *cp;
    cli_string symbol, equ_symbol;
    float component, value, min, max, step;
    int n, total;

    data_dsc->key = 1;
    var_find (data_dsc->key, NIL);

    *stat = cli__normal;

    read_index = 0;
    ch = data_spec[read_index++];

    if (ch == dollar_sign)
    {
	*equ_symbol = '\0';

	cp = symbol;
	ch = data_spec[read_index++];

	if (isalpha(ch))
	{
	    while (isalnum(ch) || ch == '_' || ch == '$')
	    {
		*cp++ = ch;
		ch = data_spec[read_index++];
	    }
	    *cp = '\0';

	    while (ch == blank || ch == tab)
		ch = data_spec[read_index++];
	}
	else
	    *stat = cli__synillexpr; /* Syntax: invalid expression */
    }
    else if (ch == quotation_mark)
    {
	*symbol = '\0';

	cp = equ_symbol;
	ch = data_spec[read_index++];

	while (ch && ch != quotation_mark)
	{
	    *cp++ = ch;
	    ch = data_spec[read_index++];
	}
	*cp = '\0';

	if (ch == quotation_mark)
	{
	    ch = data_spec[read_index++];

	    while (ch == blank || ch == tab)
		ch = data_spec[read_index++];
	}
	else
	    *stat = cli__synillexpr; /* Syntax: invalid expression */
    }
    else
	*stat = cli__synillexpr; /* Syntax: invalid expression */

    if (odd(*stat))
    {
	data_dsc->data_type = type_symbol;

	strcpy (data_dsc->type.symbol.ident, symbol);
	strcpy (data_dsc->type.symbol.equ, equ_symbol);
    }
    else
    {
	data_dsc->type.constant = str_real (data_spec, stat);
	if (!odd(*stat))
	    data_dsc->type.constant = str_integer (data_spec, stat);

	if (odd(*stat)) 
	    data_dsc->data_type = type_constant;
	else
	{
	    data_spec = str_cap (data_spec);

	    read_index = 0;
	    ch = data_spec[read_index++];

	    n = get_integer_constant (data_spec, &ch, &read_index, stat);
	    value = 0;

	    if (odd(*stat) && n > 0 &&
		strncmp(&data_spec[read_index-1], "OF", 2) == 0)
	    {
		ch = data_spec[read_index++];
		ch = data_spec[read_index++];

		value = get_real_constant (data_spec, &ch, &read_index, stat);

		if (ch && ch != delimiter)
		    *stat = cli__synillexpr; /* Syntax: invalid expression */
	    }
	    else
		*stat = cli__synillexpr; /* Syntax: invalid expression */

	    data_dsc->type.repetition.n = n;
	    data_dsc->type.repetition.value = value;

	    if (odd(*stat)) {
		data_dsc->data_type = type_repetition;
	    }
	    else
	    {
		read_index = 0;
		ch = data_spec[read_index++];

		min = (float) get_integer (data_spec, &ch, &read_index, stat);

		if (odd(*stat) &&
		    strncmp(&data_spec[read_index-1], "..", 2) == 0)
		{
		    ch = data_spec[read_index++];
		    ch = data_spec[read_index++];

		    step = 1.0;
		    max = (float) get_integer (data_spec, &ch, &read_index,
			stat);

		    if ((ch && ch != delimiter) || max < min)
			*stat = cli__synillexpr;
		    else
			total = (int)(max-min)+1;
		}
		else
		    *stat = cli__synillexpr; /* Syntax: invalid expression */

		data_dsc->type.range.min = min;
		data_dsc->type.range.max = max;
		data_dsc->type.range.step = step;
		data_dsc->type.range.total = total;

		if (odd(*stat)) 
		    data_dsc->data_type = type_subrange;
		else
		{
		    read_index = 0;
		    ch = data_spec[read_index++];

		    min = get_real (data_spec, &ch, &read_index, stat);

		    if (odd(*stat) && ch == left_bracket)
		    {
			ch = data_spec[read_index++];

			step = get_real (data_spec, &ch, &read_index, stat);

			if (odd(*stat) && step != 0 && ch == right_bracket)
			{
			    ch = data_spec[read_index++];

			    max = get_real (data_spec, &ch, &read_index, stat);

			    if ((ch && ch != delimiter) || 
				step*(max-min) <= 0)
				*stat = cli__synillexpr;
			    else
				total = (int)((max-min)/step+0.5)+1;
			}
			else
			    *stat = cli__synillexpr;
		    }
		    else
			*stat = cli__synillexpr;

		    data_dsc->type.range.min = min;
		    data_dsc->type.range.max = max;
		    data_dsc->type.range.step = step;
		    data_dsc->type.range.total = total;
	
		    if (odd(*stat))
			data_dsc->data_type = type_range;
		    else
		    {
			data_dsc->type.variable = var_address (data_spec, stat);
	
			if (odd(*stat))
			    data_dsc->data_type = type_variable;
			else
			{
			    data_dsc->type.function = fun_address (data_spec,
				stat);

			    if (odd(*stat))
				data_dsc->data_type = type_function;
			    else
			    {
				fun_parse (data_spec, &component, FALSE, stat);
	
				if (*stat == fun__constant)
				{
				    data_dsc->type.constant = component;
				    data_dsc->data_type = type_constant;
				}
	
				else if (odd(*stat))
				{
				    strcpy (data_dsc->type.expression,
					data_spec);
				    data_dsc->data_type = type_expression;
				}
			    }
			}
		    }
		}
	    }
	}
    }

    return (data_dsc);
}



static
void parse_command (int *status)

/*
 * parse_command - parse command line
 */

{
    char ch, *cp;
    int saved_read_index;
    cli_string ident,data_spec;
    cli_data_descriptor data;
    BOOL assignment;

    *status = cli__normal;
    saved_read_index = read_index;

    ch = read_character();

    while (ch == blank || ch == tab)
	ch = read_character();

    if (isalpha(ch) || ch == '_' || ch == '$')
    {
	cp = ident;

	while (isalnum(ch) || ch == '_' || ch == '$' || ch == '*')
	{
	    *cp++ = ch;
	    ch = read_character();
	}

	*cp = '\0';

	while (ch == blank || ch == tab)
	    ch = read_character();

	assignment = (ch == colon);
	if (assignment)
	    ch = read_character();

	if (ch == equal_sign)
	{
	    get_item (data_spec, " ,;", FALSE, 0);

	    data_descriptor (&data, data_spec, status);

	    if (odd(*status))

		switch (data.data_type)
		{
		    case type_symbol :
		      *status = cli__symbol;
		      break;

		    case type_repetition :
		    case type_subrange :
		    case type_range :
		      *status = cli__variable;
		      break;

		    case type_constant :
		    case type_variable :
		    case type_function :
		    case type_expression :
		      if (assignment)
			  *status = cli__variable;
		      else
			  *status = cli__function;
		      break;
		}
	}
    }

    read_index = saved_read_index;
}



static
void translate_command (int *status)

/*
 * translate_command - translate command line
 */

{
    char ch, *cp;
    int saved_read_index, symbol_pos;
    cli_verb symbol;
    cli_string equ_symbol, str;
    BOOL quoted_string, double_apostrophe;

    *status = cli__normal;
    saved_read_index = read_index;

    ch = read_character();
    quoted_string = FALSE;

    while (odd(*status) && ch && ch != delimiter)
    {
	while (ch == blank || ch == tab)
	    ch = read_character();

	if (ch == quotation_mark && next_character() == quotation_mark)
	    ch = read_character();

	else if (ch == quotation_mark)
	    quoted_string = !quoted_string;

	double_apostrophe = (quoted_string && 
            ch == apostrophe && next_character() == apostrophe);

	if (ch == '~' && !quoted_string)
	{
	    char *env;

	    *str = '\0';
	    strncat (str, cmd_line, read_index-1);
	    if ((env = (char *) getenv ("HOME")) != NULL)
		strcat (str, env);
	    strncat (str, &cmd_line[read_index], cmd_length-read_index);

	    strcpy (cmd_line, str);
	    cmd_length = strlen(cmd_line);

	    ch = read_character();
	}

	else if ((ch == apostrophe && !quoted_string) || double_apostrophe)
	{
	    symbol_pos = read_index-1;

            if (double_apostrophe)
                ch = read_character();

	    cp = symbol;
	    ch = read_character();

	    while (isalnum(ch) || ch == '_' || ch == '$')
	    {
		*cp++ = ch;
		ch = read_character();
	    }

	    *cp = '\0';

	    if (ch == apostrophe)
	    {
		sym_getenv (symbol, equ_symbol, status);

		if (odd(*status))
		{
		    ch = read_character();

		    if (ch && read_index <= cmd_length)
		    {
			*str = '\0';
			strncat (str, cmd_line, symbol_pos);
			strcat (str, equ_symbol);
			strncat (str, &cmd_line[read_index-1],
			    cmd_length-read_index+1);
			strcpy (cmd_line, str);
		    }
		    else
		    {
			*str = '\0';
			strncat (str, cmd_line, symbol_pos);
			strcat (str, equ_symbol);
			strcpy (cmd_line, str);
		    }

		    cmd_length = strlen(cmd_line);
		    read_index = symbol_pos+strlen(equ_symbol)+1;
		}
	    }
	    else
	    {
		if (*symbol)
		    *status = cli__synapos; /* Syntax: "'" expected */
		else
		    *status = cli__nosymbol; /* missing symbol */
	    }
	}
	else
	    ch = read_character();
    }

    if (!odd(*status))
	parse_index = read_index;
    else
	read_index = saved_read_index;
}



static
int verb_index (cli_verb_list verb_list, cli_verb verb,
    enum match_result *result)

/*
 * verb_index - search verb in list
 */

{
    int return_value;
    register char *p, *cp;
    cli_verb list_element;
    int list_index;
    BOOL exact;

    list_index = 0;
    *result = unrecognized;

    p = verb_list;
    while (*p == blank || *p == tab)
	p++;

    while (*p && *p != delimiter)
    {
	cp = list_element;

	while (*p && *p != blank && *p != delimiter)
	    *cp++ = *p++;

	*cp = '\0';

	while (*p == blank || *p == tab)
	    p++;

	if (str_match (verb, list_element, TRUE))
	{
	    if (*result == unrecognized)
	    {
		return_value = list_index;
		exact = strlen(verb) == strlen(list_element);

		*result = normal;
	    }
	    else
		if (!exact)
		    *result = ambiguous;
	}

	list_index = list_index+1;
    }

    return (return_value);
}



static
void disp_verb_list (tt_cid *cid, cli_verb_list verb_list)

/*
 * disp_verb_list - display verb list
 */
{
    char ch, *cp;
    int read_index;
    cli_verb verb;
    cli_string line;

    strcpy (line, "  ");

    read_index = 0;
    ch = verb_list[read_index++];

    while (ch && ch != delimiter)
    {
	cp = verb;

	while (ch && ch != blank && ch != tab && ch != delimiter)
	{
	    *cp++ = ch;
	    ch = verb_list[read_index++];
	}

	*cp = '\0';

	if (verb[strlen(verb)-1] == asterisk)
	    verb[strlen(verb)-1] = '\0';

	str_pad (verb, blank, 11*((int)(strlen(verb)/11)+1));

	if (strlen(verb)+strlen(line) >= line_width)
	{
	    tt_put_line (cid, str_remove (line, blank, FALSE), PREFIX, POSTFIX,
		NIL);
	    strcpy (line, "  ");
	}

	strcat (line, str_cap (verb));

	while (ch == blank || ch == tab)
	    ch = verb_list[read_index++];
    }

    tt_put_line (cid, line, PREFIX, POSTFIX, NIL);

    more = outstanding_help = FALSE;
}



tt_cid *cli_connect (char *lognam,
    int (*input_routine)(char *, char *, char *, int *), int *status)

/*
 * connect - connect specified terminal
 */

{
    int stat;

    cli_b_abort = FALSE;

    if (conid == NIL)
    {
	conid = tt_connect (lognam, &stat);

	if (odd(stat))
	{
	    if (input_routine != NIL)
		cli_a_input_routine = input_routine;

	    more = outstanding_help = FALSE;
	    stat = cli__normal;
	}
	else
	    stat = cli__confai; /* connection failure */
    }
    else
	stat = cli__alreadycon; /* connection already done */

    if (status != NIL)
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);

    return (conid);
}



void cli_disconnect (int *status)

/*
 * disconnect - disconnect terminal
 */

{
    int stat;

    if (conid != NIL)
    {
	tt_disconnect (conid, &stat);

	if (odd(stat))
	{
	    stat = cli__normal;
	    conid = NIL;
	}
	else
	    stat = cli__disconfai; /* disconnect failed */
    }
    else
	stat = cli__noconnect; /* no connection done */

    if (status != NIL)
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);
}



void cli_get_keyword (char *prompt, cli_verb_list keyword_list, int *index,
    int *status)

/*
 * get_keyword - get command keyword
 */

{
    int stat;
    cli_verb keyword;
    enum match_result result;
    cli_string cont, line;

    if (end_of_command())
    {
	if (conid != NIL)
	{
	    if (outstanding_help)
	    {
		disp_verb_list (conid, keyword_list);
		outstanding_help = FALSE;
	    }

	    do
	    {
		if (cli_a_input_routine != NIL)
		{
		    if ((*cli_a_input_routine)
			(prompt, keyword_list, line, &more) >= 0)
		    {
			tt_write_buffer (conid, line, TRUE, NIL);
			stat = cli__normal;
		    }
		    else
			stat = cli__eof;
		}
		else
		{
		    strcpy (cont, "_");
		    strcat (cont, prompt);
		    strcat (cont, ": ");

		    tt_get_line (conid, cont, line, 0, TRUE, TRUE, NIL, &stat);
		}

		if (stat == tt__help)
		{
		    if (!*line)
			disp_verb_list (conid, keyword_list);
		    else
			outstanding_help = TRUE;
		}
		else
		    if (stat == tt__eof)
			stat = cli__eof;
		    else
			if (stat == tt__controlc)
			    stat = cli__controlc;
	    }
	    while (odd(stat) && !*line);

	    if (odd(stat))
	    {
		strcat (cmd_line, " ");
		strcat (cmd_line, line);
		cmd_length = strlen(cmd_line);
	    }
	    else
		more = outstanding_help = FALSE;
	}
	else
	    stat = cli__insfprm; /* missing command parameters */
    }
    else
	stat = cli__normal;

    if (odd(stat))
    {
	get_item (keyword, " ", FALSE, 0);

	*index = verb_index (keyword_list, keyword, &result);

	switch (result)
	{
	    case unrecognized :
	      stat = cli__ivkeyw; /* unrecognized command keyword */
	      break;

	    case normal :
	      stat = cli__normal;
	      break;

	    case ambiguous :
	      stat = cli__abkeyw; /* ambiguous command keyword */
	      break;
	}
    }

    if (status != NIL)
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);
}



static
void string_to_text (char *chars)
{
    char *str, *text;

    if (*chars == quotation_mark)
    {
	str = text = chars;

	while (*str)
	{
	    if (*str == quotation_mark && *(str+1) == quotation_mark)
		*text++ = *str++;
	    else if (*str == quotation_mark)
		str++;
	    else
		*text++ = *str++;           
	}

	*text = '\0';
    }
}



void cli_get_parameter (char *prompt, char *parameter, cli_delim_set delimiters,
    BOOL then_keyword, BOOL punctuation, int *status)

/*
 * get_parameter - get command parameter
 */

{
    int stat;
    cli_string cont, line;

    if (end_of_command())
    {
	if (conid != NIL)
	{
	    outstanding_help = FALSE;
	    do
	    {
		if (cli_a_input_routine != NIL)
		{
		    if ((*cli_a_input_routine)(prompt, "", line, &more) >= 0)
		    {
			tt_write_buffer (conid, line, TRUE, NIL);
			stat = cli__normal;
		    }
		    else
			stat = cli__eof;
		}
		else
		{
		    if (punctuation)
		    {
			strcpy (cont, "_");
			strcat (cont, prompt);
			strcat (cont, ": ");
		    }
		    else
			strcpy (cont, prompt);

		    tt_get_line (conid, cont, line, 0, TRUE, TRUE, NIL, &stat);
		}

		if (stat == tt__help)
		    outstanding_help = TRUE;
		else
		    if (stat == tt__eof)
			stat = cli__eof;
		    else
			if (stat == tt__controlc)
			    stat = cli__controlc;
	    }
	    while (odd(stat) && !*line && punctuation);

	    if (odd(stat))
	    {
		if (*line)
		{
		    strcat (cmd_line, " ");
		    strcat (cmd_line, line);
		    cmd_length = strlen(cmd_line);
		}
	    }
	    else
		more = outstanding_help = FALSE;
	}
	else
	    stat = cli__insfprm; /* missing command parameters */
    }
    else
	stat = cli__normal;

    if (odd(stat))
    {
	get_item (parameter, delimiters, then_keyword, 0);

	if (then_keyword && !*parameter)
	    stat = cli__nothen; /* IF or ON statement syntax error */
	else
	    string_to_text (parameter);
    }

    if (status != NIL)
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);
}



var_variable_descr *cli_get_variable (char *prompt, char *ident, int *size,
    int *status)

/*
 * get_variable - get a variable specification
 */

{
    int stat;
    var_variable_descr *variable;

    cli_get_parameter (prompt, ident, ", ", FALSE, TRUE, &stat);
    if (odd(stat))
    {
	variable = var_address (ident, &stat);

	if (odd(stat))
	    var_size (ident, size, &stat);
    }

    if (status != NIL)
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);

    return (variable);
}



void cli_await_key (char *prompt, char key, int *status)

/*
 * await_key - await specified key
 */

{
    int stat;
    char ch;

    if (conid != NIL)
    {
	tt_put_line (conid, prompt, PREFIX, POSTFIX, &stat);

	if (odd(stat))
	{
	    do
		tt_get_key (conid, &ch, 0, &stat);
	    while (ch != key && odd(stat));

	    if (stat == tt__eof)
		stat = cli__eof;
	    else
		if (stat == tt__controlc)
		    stat = cli__controlc;
	}
    }
    else
	stat = cli__normal;

    if (status != NIL)
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);
}



void cli_get_data (char *prompt, cli_data_descriptor *data, int *status)

/*
 * get_data - get data
 */

{
    int stat;
    cli_data_descriptor *descr;
    cli_string cont, line, data_spec;
    char delim;

    descr = data;

    if (prompt != NIL)
    {
	if (end_of_command())
	{
	    if (conid != NIL)
	    {
		outstanding_help = FALSE;
		do
		{
		    if (cli_a_input_routine != NIL)
		    {
			if ((*cli_a_input_routine)
			    (prompt, "", line, &more) >= 0)
			{
			    tt_write_buffer (conid, line, TRUE, NIL);
			    stat = cli__normal;
			}
			else
			    stat = cli__eof;
		    }
		    else
		    {
			strcpy (cont, "_");
			strcat (cont, prompt);
			strcat (cont, ": ");

			tt_get_line (conid, cont, line, 0, TRUE, TRUE, NIL,
			    &stat);
		    }
		}
		while (odd(stat) && !*line);

		if (stat == tt__eof)
		    stat = cli__eof;

		else if (stat == tt__controlc)
		    stat = cli__controlc;

		if (odd(stat))
		{
		    strcat (cmd_line, " ");
		    strcat (cmd_line, line);
		    cmd_length = strlen(cmd_line);
		}
		else
		    more = outstanding_help = FALSE;
	    }
	    else
		stat = cli__insfprm; /* missing command parameters */
	}
	else
	    stat = cli__normal;

	if (odd(stat))
	{
	    get_item (data_spec, " ,;", FALSE, &delim);

	    descr = data_descriptor (descr, data_spec, &stat);  
	    descr->eos = (delim == semicolon);
	}
    }

    if (status != NIL)
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);
}



void cli_get_data_buffer (cli_data_descriptor *data, int size, float *buffer,
    int *return_length, int *status)

/*
 * get_data_buffer - get data buffer
 */

{
    int stat;
    cli_data_descriptor *descr;
    int count;
    float component;
    BOOL last_component;

    descr = data;
    last_component = TRUE;

    stat = cli__normal;
    count = 0;

    while (odd(stat) && stat != cli__eos && count < size)
    {
	var_find (descr->key, NIL);

	switch (descr->data_type)
	{
	    case type_symbol :
		last_component = TRUE;

		if (*descr->type.symbol.ident)
		    sym_getenv (descr->type.symbol.ident,
			descr->type.symbol.equ, &stat);

		if (odd(stat))
		{
		    if (descr->key == 1)
			component = strlen (descr->type.symbol.equ);
		    else
			stat = cli__nmd; /* no more data */
		}
		break;

	    case type_constant :
		last_component = TRUE;

		if (descr->key == 1)
		    component = descr->type.constant;
		else
		    stat = cli__nmd; /* no more data */
		break;

	    case type_repetition :
		last_component = (descr->key == descr->type.repetition.n);

		if (descr->key <= descr->type.repetition.n)
		    component = descr->type.repetition.value;
		else
		    stat = cli__nmd; /* no more data */
		break;

	    case type_subrange:
	    case type_range :
		last_component = (descr->key == descr->type.range.total);

		if (descr->key <= descr->type.range.total)
		    component = (descr->key-1)*descr->type.range.step +
			descr->type.range.min;
		else
		    stat = cli__nmd; /* no more data */
		break;

	    case type_variable :
		var_examine (descr->type.variable, &component, &stat);

		if (stat == var__undefind || stat == var__constant)
		    stat = cli__nmd; /* no more data */

		else if (stat == var__eos)
		    stat = cli__eos;  /* end of segment detected */
		break;

	    case type_function :
		fun_evaluate (descr->type.function, &component, &stat);

		if ((stat == fun__constant && descr->key > 1) ||
		     stat == var__undefind)
		    stat = cli__nmd; /* no more data */

		else if (stat == fun__eos || stat == var__eos)
		    stat = cli__eos; /* end of segment detected */
		break;

	    case type_expression :
		fun_parse (descr->type.expression, &component, FALSE,
		    &stat);

		if ((stat == fun__constant && descr->key > 1) ||
		     stat == var__undefind)
		    stat = cli__nmd; /* no more data */

		else if (stat == fun__eos || stat == var__eos)
		    stat = cli__eos; /* end of segment detected */
		break;
	}

	if (odd(stat))
	{
	    descr->key++;
	    *(buffer+count) = component;
	    count++;
	}
    }

    if (stat == cli__nmd && count > 0)
	stat = cli__normal;

    if (odd(stat) && descr->eos && last_component)
	stat = cli__eos; /* end of segment detected */

    if (return_length != NIL)
	*return_length = count;

    if (status != NIL)
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);
}



void cli_data (char *chars, int size, float *buffer, int *return_length,
    int *status)

/*
 * data - get data buffer (easy-to-use version)
 */

{
    cli_data_descriptor data, *descr;
    int stat;

    descr = data_descriptor (&data, chars, &stat);  
    descr->eos = FALSE;

    if (odd(stat))
	cli_get_data_buffer (descr, size, buffer, return_length, &stat);

    if (status != NIL)
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);
}



int cli_integer (char *chars, int *status)

/*
 * integer - convert string to integer
 */

{
    int stat;
    cli_data_descriptor data;
    float component;

    data_descriptor (&data, chars, &stat);
    component = 0;

    if (odd(stat))
    {
	var_find (data.key, NIL);

	switch (data.data_type)
	{
	    case type_symbol :
	    case type_subrange :
	    case type_range :
	    case type_repetition :
	      stat = cli__synillexpr; /* Syntax: ill-formed expression */
	      break;

	    case type_constant :
	      component = data.type.constant;
	      break;

	    case type_variable :
	      var_examine (data.type.variable, &component, &stat);
	      break;

	    case type_function :
	      fun_evaluate (data.type.function, &component, &stat);
	      break;

	    case type_expression :
	      fun_parse (data.type.expression, &component, FALSE, &stat);
	      break;
	}

	if (odd(stat))
	    if (component != (float)round(component))
		stat = cli__ordexprreq; /* ordinal expression required */
    }

    if (status != NIL)
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);

    return (round(component));
}



float cli_real (char *chars, int *status)

/*
 * real - convert string to real
 */

{
    int stat;
    cli_data_descriptor data;
    float component;

    data_descriptor (&data, chars, &stat);
    component = 0;

    if (odd(stat))
    {
	var_find (data.key, NIL);

	switch (data.data_type)
	{
	    case type_symbol :
	    case type_subrange :
	    case type_range :
	    case type_repetition :
	      stat = cli__synillexpr; /* Syntax: ill-formed expression */
	      break;

	    case type_constant :
	      component = data.type.constant;
	      break;

	    case type_variable :
	      var_examine (data.type.variable, &component, &stat);
	      break;

	    case type_function :
	      fun_evaluate (data.type.function, &component, &stat);
	      break;

	    case type_expression :
	      fun_parse (data.type.expression, &component, FALSE, &stat);
	      break;
	}
    }

    if (status != NIL)
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);

    return (component);
}



BOOL cli_end_of_command (void)

/*
 * end_of_command - test for end of command stream
 */

{
    return (end_of_command() && !outstanding_help && !more);
}



void cli_set_command (char *command, int *status)

/*
 * set_command - set command string
 */

{
    int stat;

    read_index = 0;
    more = outstanding_help = FALSE;

    stat = cli__normal;
    strcpy (cmd_line, command);
    cmd_length = strlen(cmd_line);

    if (status != NIL)
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);
}



void cli_inquire_command (char *command, int *index, int *status)

/*
 * inquire_command - inquire command string
 */

{
    int stat;

    stat = cli__normal;
    strcpy (command, cmd_line);

    if (index != NIL)
	*index = parse_index;

    if (status != NIL)
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);
}



cli_procedure cli_open (char *file_spec, int *status)

/*
 * open - open command procedure
 */

{
    cli_procedure command_procedure;
    cli_string line;
    int stat;
    cli_command prev_command;
    FILE *command_file;

    command_procedure = (cli_procedure) malloc (sizeof(cli_procedure_descr));

    command_procedure->saved_procedure = current_procedure;
    command_procedure->first_command = NIL;
    command_procedure->command = NIL;

    command_file = fopen (file_spec, "r");

    while (fgets (line, string_length, command_file))
    {
	prev_command = command_procedure->command;
	command_procedure->command = (cli_command_descr *)
	    malloc (sizeof(cli_command_descr));

	str_translate (line, '\n', '\0');

	strcpy (command_procedure->command->line, line);
	command_procedure->command->next = NIL;

	if (prev_command != NIL)
	    prev_command->next = command_procedure->command;
	else
	    command_procedure->first_command = command_procedure->command;
    }

    command_procedure->command = command_procedure->first_command;
    command_procedure->frame_pointer = NIL;

    fclose (command_file);
    stat = cli__normal;

    current_procedure = command_procedure;

    if (status != NIL)
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);

    return (command_procedure);
}



cli_procedure cli_open_pipe (int *status)

/*
 * open - open command pipe
 */

{
    cli_procedure command_procedure;
    cli_string line;
    int stat;
    cli_command prev_command;

    command_procedure = (cli_procedure) malloc (sizeof(cli_procedure_descr));

    command_procedure->saved_procedure = current_procedure;
    command_procedure->first_command = NIL;
    command_procedure->command = NIL;

    while (fgets (line, string_length, stdin))
    {
	prev_command = command_procedure->command;
	command_procedure->command = (cli_command_descr *)
	    malloc (sizeof(cli_command_descr));

	str_translate (line, '\n', '\0');

	strcpy (command_procedure->command->line, line);
	command_procedure->command->next = NIL;

	if (prev_command != NIL)
	    prev_command->next = command_procedure->command;
	else
	    command_procedure->first_command = command_procedure->command;
    }

    command_procedure->command = command_procedure->first_command;
    command_procedure->frame_pointer = NIL;

    stat = cli__normal;

    current_procedure = command_procedure;

    if (status != NIL)
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);

    return (command_procedure);
}



void cli_close (cli_procedure command_procedure, int *status)

/*
 * close - close command file
 */

{
    int stat;
    cli_command next_command;
    cli_frame prev_frame;

    if (command_procedure != NIL)
    {
	command_procedure->command = command_procedure->first_command;

	while (command_procedure->command != NIL)
	{
	    next_command = command_procedure->command->next;
	    free (command_procedure->command);
	    command_procedure->command = next_command;
	}

	while (command_procedure->frame_pointer != NIL)
	{
	    prev_frame = command_procedure->frame_pointer->saved_fp;
	    free (command_procedure->frame_pointer);
	    command_procedure->frame_pointer = prev_frame;
	}

	current_procedure = command_procedure->saved_procedure;
	free (command_procedure);

	stat = cli__normal;
    }
    else
	stat = cli__invargpas; /* invalid argument passed to CLI routine */

    if (status != NIL)
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);
}



BOOL cli_eof (cli_procedure command_procedure, int *status)

/*
 * eof - return end-of-file status
 */

{
    int stat;
    BOOL return_value;

    if (command_procedure != NIL)
    {
	return_value = (command_procedure->command == NIL);
	stat = cli__normal;
    }
    else
	stat = cli__invargpas; /* invalid argument passed to CLI routine */

    if (status != NIL)
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);

    return (return_value);
}



void cli_readln (cli_procedure command_procedure, char *line, int *status)

/*
 * readln - read next command line from command procedure
 */

{
    int stat;

    if (command_procedure != NIL)
    {
	if (command_procedure->command != NIL)
	{
	    strcpy (line, command_procedure->command->line);
	    command_procedure->command = command_procedure->command->next;
#ifdef HAVE_SOCKETS
	    if (conid != NIL)
		sys_receive();
#endif
	    stat = cli__normal;
	}
	else
	    stat = cli__eof;   /* file is at end-of-file */
    }
    else
	stat = cli__invargpas; /* invalid argument passed to CLI routine */

    if (status != NIL)
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);
}



static char *extract_label (char *command, char *label)

/*
 * extract_label - extract command label from command line
 */

{
    char *str;

    while (*command == blank || *command == tab)
	command++;

    str = label;
    while (isalnum(*command) || *command == '_' || *command == '$')
	*str++ = islower(*command) ? toupper (*command++) : *command++;

    while (*command == blank || *command == tab)
	command++;

    if (*command != colon) {
	str = label;
	*str = null;
    }
    else
	*str = null;

    return (label);
}



void cli_goto (cli_label command_label, int *status)

/*
 * goto - transfer control to specified label
 */

{
    int stat;
    cli_command saved_command;
    cli_label return_value;

    if (current_procedure != NIL)
    {
	str_cap (command_label);
	saved_command = current_procedure->command;

	stat = cli__usgoto;
	current_procedure->command = current_procedure->first_command;

	while (!odd(stat) && current_procedure->command != NIL)
	{
	    if (strcmp (command_label, extract_label (
		current_procedure->command->line, return_value)) == 0)
		stat = cli__normal;
	    else
		current_procedure->command = current_procedure->command->next;
	}

	if (!odd(stat))
	    current_procedure->command = saved_command;
    }
    else
	/* GOTO command not allowed on current command level */
	stat = cli__invgoto;

    if (status != NIL)
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);
}




void cli_gosub (cli_label command_label, int *status)

/*
 * gosub - transfer control to specified subroutine
 */

{
    int stat;
    cli_command saved_command;
    cli_frame saved_frame;
    cli_label return_value;

    if (current_procedure != NIL)
    {
	str_cap (command_label);
	saved_command = current_procedure->command;

	stat = cli__usgosub;
	current_procedure->command = current_procedure->first_command;

	while (!odd(stat) && current_procedure->command != NIL)
	{
	    if (strcmp (command_label, extract_label (
		current_procedure->command->line, return_value)) == 0)
		stat = cli__normal;
	    else
		current_procedure->command = current_procedure->command->next;
	}

	if (odd(stat))
	{
	    saved_frame = current_procedure->frame_pointer;
	    current_procedure->frame_pointer = (cli_frame_descr *)
		malloc (sizeof(cli_frame_descr));
	    current_procedure->frame_pointer->saved_cmd = saved_command;
	    current_procedure->frame_pointer->saved_fp = saved_frame;
	}
	else
	    current_procedure->command = saved_command;
    }
    else
	/* GOSUB command not allowed on current command level */
	stat = cli__invgosub;

    if (status != NIL)
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);
}



void cli_return (int *status)

/*
 * return - terminate a GOSUB subroutine
 */

{
    int stat;
    cli_frame saved_frame;

    if (current_procedure != NIL)
    {
	if (current_procedure->frame_pointer != NIL)
	{
	    current_procedure->command =
		current_procedure->frame_pointer->saved_cmd;

	    saved_frame = current_procedure->frame_pointer->saved_fp;
	    free (current_procedure->frame_pointer);
	    current_procedure->frame_pointer = saved_frame;

	    stat = cli__normal;
	}
	else
	    /* RETURN command not allowed on current command level */
	    stat = cli__invreturn;
    }
    else
	/* RETURN command not allowed on current command level */
	stat = cli__invreturn;

    if (status != NIL)
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);
}



void cli_get (char *line, int *status)

/*
 * get - read next command line from command procedure
 */

{
    int stat;

    if (current_procedure != NIL)
    {
	if (current_procedure->command != NIL)
	{
	    strcpy (line, current_procedure->command->line);
	    current_procedure->command = current_procedure->command->next;
	    stat = cli__normal;
	}
	else
	    stat = cli__eof; /* file is at end-of-file */
    }
    else
	/* GET command not allowed on current command level */
	stat = cli__invget;

    if (status != NIL)
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);
}



static
BOOL is_a_tcl_command (char *cmd)
{
    register char *cp;
    BOOL result;

    cp = cmd;
    while (isspace(*cp))
	cp++;

    if (!*cp)
	result = FALSE;

    else if (strstr(cp, "$ ") || *cp == '@' || *cp == '!')
	result = FALSE;

    else if (isupper(*cp))
	{
	cp++;
	while (isupper(*cp) || isdigit(*cp) || *cp == '_' || *cp == '$')
	    cp++;

	if (!*cp || isspace(*cp) || *cp == ':' || *cp == '=' || *cp == '!')
	    result = FALSE;
	}
    else
	result = TRUE;

    return result;
}



void cli_get_command (char *prompt, cli_verb_list command_list, int *index,
    int *status)

/*
 * get_command - get command input
 */

{
    int stat;
    cli_label command_label;
    cli_verb command;
    cli_string equ_cmd;
    enum match_result result;
    cli_string str;

    if (end_of_command())
    {
	if (conid != NIL)
	{
	    outstanding_help = FALSE;
	    read_index = 0;
	    do
	    {
		if (cli_a_input_routine != NIL)
		{
		    if ((*cli_a_input_routine)
			(prompt, command_list, cmd_line, &more) >= 0)
		    {
			tt_write_buffer (conid, cmd_line, FALSE, NIL);
			stat = cli__normal;
		    }
		    else
			stat = cli__eof;
		}
		else
		    tt_get_line (conid, prompt, cmd_line, 0, TRUE, FALSE, NIL,
			&stat);

		cmd_length = strlen(cmd_line);

		if (stat == tt__help)
		{
		    if (cmd_length == 0)
			disp_verb_list (conid, command_list);
		    else
			outstanding_help = TRUE;
		}
		else
		    if (stat == tt__eof)
			stat = cli__eof;
		    else
			if (stat == tt__controlc)
			    stat = cli__controlc;
	    }
	    while (odd(stat) && cmd_length == 0);

	    if (!odd(stat))
		more = outstanding_help = FALSE;
	}
	else
	    stat = cli__insfprm; /* missing command parameters */
    }
    else
	stat = cli__normal;

    if (odd(stat) && cli_b_tcl_interp)
    {
	if (cli_b_tcl_got_partial || is_a_tcl_command(cmd_line))
	{
	    translate_command (&stat);
	    stat = cli__tcl; /* Tcl command */
	}
    }

    if (odd(stat) && stat != cli__tcl)
    {
	get_label (command_label);

	if (current_procedure != NIL || !*command_label)
	{
	    if (!end_of_command())
	    {
		parse_command (&stat);

		if (stat == cli__normal)
		{
		    translate_command (&stat);

		    if (odd(stat))
		    {
			get_item (command, " ", FALSE, NIL);

			sym_translate (command, equ_cmd, &stat);

			if (odd(stat))
			{
			    if (!*equ_cmd)
				strcpy (equ_cmd, command);

			    if (read_index < cmd_length)
			    {
				strcpy (str, command_label);
				strcat (str, equ_cmd);
				strncat (str, &cmd_line[read_index-1],
				    cmd_length-read_index+1);
				strcpy (cmd_line,str);
			    }
			    else
			    {
				strcpy (cmd_line, command_label);
				strcat (cmd_line, equ_cmd);
			    }

			    cmd_length = strlen(cmd_line);
			    read_index = strlen(command_label);

			    get_item (command, " ", FALSE, NIL);
			}
			else
			    stat = cli__normal;

			if (odd(stat))
			{
			    *index = verb_index (command_list, command,
				&result);

			    switch (result)
			    {
				case unrecognized :
				  if (isalpha(command[0]))
				      stat = cli__ivverb;
				  else
				      stat = cli__nocomd;
				  break;

				case normal :
				  stat = cli__normal;
				  break;

				case ambiguous :
				  stat = cli__abverb;
				  break;
			    }
			}
		    }
		}
	    }
	    else
		stat = cli__empty; /* command line is empty */
	}
	else
	    stat = cli__nolbls; /* label ignored */
    }

    if (status != NIL)
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);
}



void cli_parse_command (char *syntax, int *status)

/*
 * parse_command - parse command line
 */

{
    char *str, type;
    cli_string name, parm;
    int stat, n, nx, ny, nz, ne1, ne2;
    int *ia, l, nl;

    stat = cli__normal;
    n = nl = 0;

    while (*syntax && odd(stat))
    {
        str = name;
        while (isalnum(*syntax))
            *str++ = *syntax++;
	*str = '\0';

        if (*syntax == ':')
	{
	    syntax++;
	    type = *syntax++;

	    if (*syntax == '=')
	    {
		syntax++;

		str = parm;
		while (*syntax && *syntax != ' ')
		    *str++ = *syntax++;
		*str = '\0';
	    }
	    else
		*parm = '\0';

	    if (!cli_end_of_command() || !*parm)
		cli_get_parameter (name, parm, " ,", FALSE, TRUE, &stat);

	    if (odd(stat))
	    {
		switch (type)
		{
		    case 'R' :
			sp[n].u.fnum = cli_real (parm, &stat);
			argv[n].u.faddr = &sp[n].u.fnum;
			break;

		    case 'I' :
			if (strcmp(parm, "#") == 0)
			    sp[n].u.inum = nx < ny ? nx : ny;
			else
			    sp[n].u.inum = cli_integer (parm, &stat);

			argv[n].u.iaddr = &sp[n].u.inum;
			break;

		    case 'L' :
			sp[n].u.inum = cli_integer (parm, &stat);
#ifdef cray
			sp[n].u.inum = _btol(sp[n].u.inum);
#endif
			argv[n].u.iaddr = &sp[n].u.inum;
			break;

		    case 'C' :
			strcpy (sp[n].u.str, parm);
#ifdef VMS
			sp[n].text.dsc$b_dtype = DSC$K_DTYPE_T;
			sp[n].text.dsc$b_class = DSC$K_CLASS_S;
			sp[n].text.dsc$w_length = strlen(sp[n].u.str);
			sp[n].text.dsc$a_pointer = sp[n].u.str;

			argv[n].u.descr = &sp[n].text;
#else
#ifdef cray
                        sp[n].text = _cptofcd(sp[n].u.str, strlen(sp[n].u.str));
                        argv[n].u.descr = sp[n].text;
#else
			argv[n].u.caddr = sp[n].u.str;
			len[nl++] = strlen(argv[n].u.caddr);
#endif /* cray */
#endif /* VMS */
			break;

		    case '1' :
			cli_data (parm, max_points, px, &nx, &stat);
			argv[n].u.faddr = px;
			break;

		    case '2' :
			cli_data (parm, max_points, py, &ny, &stat);
			argv[n].u.faddr = py;
			break;

		    case '3' :
			cli_data (parm, max_points, pz, &nz, &stat);
			argv[n].u.faddr = pz;
			break;

		    case '4' :
			cli_data (parm, max_points, pe1, &ne1, &stat);
			argv[n].u.faddr = pe1;
			break;

		    case '5' :
			cli_data (parm, max_points, pe2, &ne2, &stat);
			if (odd(stat))
                        {
                            ia = (int *) pe2;
                            for (l=0; l<ne2; l++)
                                ia[l] = (int) pe2[l];
                        }
                        argv[n].u.faddr = pe2;
			break;
		}
		n++;

		if (*syntax == ' ')
		    syntax++;
	    }
	}
    }

    if (odd(stat))
    {
        if (cli_end_of_command())
        {
#if !defined(VMS) && !defined(cray)
	    for (l=0; l<nl; l++)
	        argv[n++].u.len = len[l];
#endif
            while (n < MAXARGS)
                argv[n++].u.caddr = NULL;
        }
        else
	    stat = cli__maxparm; /* maximum parameter count exceeded */
    }

    if (status != NIL)
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);
}


void cli_call_proc (int (*routine)(char *, ...))
{
    routine(
        argv[ 0].u.caddr, argv[ 1].u.caddr, argv[ 2].u.caddr, argv[ 3].u.caddr,
	argv[ 4].u.caddr, argv[ 5].u.caddr, argv[ 6].u.caddr, argv[ 7].u.caddr,
	argv[ 8].u.caddr, argv[ 9].u.caddr, argv[10].u.caddr, argv[11].u.caddr,
	argv[12].u.caddr, argv[13].u.caddr, argv[14].u.caddr, argv[15].u.caddr);
}
    
