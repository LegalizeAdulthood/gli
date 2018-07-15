/*
 *
 * FACILITY:
 *
 *	VAX C Run-Time System
 *
 * ABSTRACT:
 *
 *	This module contains symbol table service routines.
 *
 * AUTHOR:
 *
 *	Josef Heinen
 *
 * VERSION:
 *
 *	V1.0-00
 *
 */


#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "variable.h"
#include "function.h"
#include "symbol.h"
#include "strlib.h"
#include "system.h"


#define BOOL int
#define TRUE 1
#define FALSE 0
#define NIL 0

#define terminator '\0'

#define odd(status) ((status) & 01)


/* Global data structures */

char *(*sym_a_tcl_getvar)(char *) = NULL;

static sym_symbol_descr
    *first_symbol = NIL, *symbol = NIL;



static
void find_symbol (char *name, int *status)

/*
 * find_symbol - lookup symbol in table
 */

{
    int stat;
    char *cp;
    BOOL match;
    sym_symbol_descr *prev_symbol;

    cp = name;
    if (isalpha(*cp))
	{
	cp++;
	while (isalnum(*cp) || *cp == '_' || *cp == '$')
	    cp++;
	if (*cp == '|')
	    {
	    cp++;
            while (isalnum(*cp) || *cp == '_' || *cp == '$')
                cp++;
	    }
	if (*cp == terminator)
	    stat = sym__normal;
	else
	    stat = sym__invsymb; /* Syntax: invalid symbol */
	}
    else
	/* Syntax: identifier must begin with alphabetic character */
	stat = sym__nonalpha;

    if (odd(stat))
	{
	if (strlen(name) <= sym_ident_length)
	    {
	    prev_symbol = NIL;
	    symbol = first_symbol;
	    match = FALSE;
	    cp = strchr (name, '|');
	    if (cp != 0)
		*cp = '\0';
	    while (symbol != NIL && !match)
		{
		prev_symbol = symbol;
		match = str_match (name, symbol->ident, FALSE);
		if (!match)
		    symbol = symbol->next;
		}
	    if (cp != 0)
		*cp = '|';
	    if (!match)
		{
		symbol = prev_symbol;
		stat = sym__undsymb; /* undefined symbol */
		}
	    else
		stat = sym__existsym; /* existing symbol */
	    }
	else
	    stat = sym__illsymlen; /* symbol exceeds 31 characters */
	}

    if (status != NIL)
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);
}



void sym_define (char *name, char *str, int *status)

/*
 * define - define a symbol
 */

{
    int i, len;
    char id[sym_string_length], equ[sym_string_length];	
    int stat;
    sym_symbol_descr *prev_symbol;

    strcpy (id, name);
    strcpy (equ, str);

    len = strlen(id);
    for (i = 0; i < len; i++)
	if (id[i] == '*')
	    id[i] = '|';
	else
	    if (islower(id[i])) id[i] = toupper(id[i]);

    find_symbol (id, &stat);
    if (stat == sym__undsymb || stat == sym__existsym)
	{
	str_remove (equ, ' ', TRUE);
	if (strlen(equ) > 0)
	    {
	    if (strlen(equ) < sym_string_length) {
		if (stat == sym__undsymb)
		    stat = sym__normal;
		}
	    else
		stat = sym__illstrlen; /* string exceeds 255 characters */
	    }
	else
	    stat = sym__empty; /* empty string */

	if (stat == sym__normal)
	    {
	    prev_symbol = symbol;

	    symbol = (sym_symbol_descr *) malloc (sizeof (sym_symbol_descr));

	    symbol->prev = prev_symbol;
	    symbol->next = NIL;
	    strcpy (symbol->ident, id);
	    strcpy (symbol->equ, equ);

	    if (prev_symbol != NIL)
		prev_symbol->next = symbol;
	    else
		first_symbol = symbol;
	    }
	else
	    if (stat == sym__existsym)
		{
		strcpy (symbol->equ, equ);
		stat = sym__supersed; /* previous value has been superseded */
		}
	}

    if (status != NIL)
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);
}



void sym_delete (char *name, int *status)

/*
 * delete - delete a symbol
 */

{
    int i, len;
    char id[sym_string_length];
    int stat;
    sym_symbol_descr *prev_symbol;
    sym_symbol_descr *next_symbol;

    strcpy (id, name);
    len = strlen(id);

    for (i = 0; i < len; i++)
	if (id[i] == '*')
	    id[i] = '|';
	else
	    if (islower(id[i])) id[i] = toupper(id[i]);

    find_symbol (id, &stat);
    if (stat == sym__existsym)
	{
	prev_symbol = symbol->prev;
	next_symbol = symbol->next;

	free (symbol);
	if (next_symbol != NIL)
	    next_symbol->prev = prev_symbol;
	if (prev_symbol != NIL)
	    prev_symbol->next = next_symbol;
	else
	    first_symbol = next_symbol;
	stat = sym__normal;
	}

    if (status != NIL)
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);
}



void sym_translate (char *name, char *str, int *status)

/*
 * translate - translate symbol
 */

{
    int i, len;
    char id[sym_string_length];
    int stat;

    strcpy (id, name);
    len = strlen(id);

    for (i = 0; i < len; i++)
	if (id[i] == '*')
	    id[i] = '|';
	else
	    if (islower(id[i])) id[i] = toupper(id[i]);

    find_symbol (id, &stat);
    if (stat == sym__existsym) {
	strcpy (str, symbol->equ);
	if (strlen(symbol->equ) <= sym_string_length)
	    stat = sym__normal;
	else
	    stat = sym__stringovr;
	}
    else
	strcpy (str, id);

    if (status != NIL)
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);
}



void sym_getenv (char *name, char *str, int *status)

/*
 * getenv - get environment variable
 */

{
    int stat;
    char *env;
    float value;

    sym_translate (name, str, &stat);

    if (stat == sym__undsymb)
	if (env = (char *) getenv (name)) {
	    strcpy (str, env);
	    stat = sym__normal;
	    }

    if (stat == sym__undsymb) {
	var_find (1, NIL);
	fun_parse (name, &value, FALSE, &stat);

	if (odd(stat)) {
	    str_flt (str, value);
	    stat = sym__normal;
	    }
	else
	    stat = sym__undsymb;
	}

    if (stat == sym__undsymb && sym_a_tcl_getvar != NULL)
	if (env = sym_a_tcl_getvar (name)) {
	    strcpy (str, env);
	    stat = sym__normal;
	    }

    if (status != NIL)
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);
}



sym_symbol_descr *sym_inquire_symbol (sym_symbol_descr *context, char *name,
    char *str, int *status)

/*
 * inquire_symbol - inquire symbol information
 */

{
    int i, len;
    int stat;
    sym_symbol_descr *symbol;

    symbol = context;
    if (symbol == NIL)
	symbol = first_symbol;
    if (symbol != NIL)
	{
	strcpy (name, symbol->ident);
	strcpy (str, symbol->equ);

        len = strlen(name);
	for (i = 0; i < len; i++)
	    if (name[i] == '|')
		name[i] = '*';
	    else
		if (islower(name[i])) name[i] = toupper(name[i]);

	symbol = symbol->next;
	context = symbol;
	if (symbol != NIL)
	    stat = sym__normal;
	else
	    stat = sym__nms; /* no more symbol */
	}
    else
	stat = sym__nosymbol; /* no symbols found */

    if (status != NIL)
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);

    return context;
}
