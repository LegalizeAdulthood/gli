/*
 *
 * FACILITY:
 *
 *	VAX C Run-Time System
 *
 * ABSTRACT:
 *
 *	This module contains dynamic variable allocation routines.
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


#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>


#include "strlib.h"
#include "function.h"
#include "variable.h"
#include "system.h"


#define BOOL int
#define TRUE 1
#define FALSE 0
#define NIL 0

#define stack_size 128

#define odd(status) ((status) & 01)


typedef char string[255];

typedef struct {
    unsigned short len;
    char body[65535];
} varying_string;


/* Global data structures */

static int var_key = 1;
static int var_stack_pointer = 0;
static float stack[stack_size];

static var_variable_descr *first_variable = NIL, *variable = NIL;



void var_pushf (float value, int *status)

/*
 * pushf - put a real number on the stack
 */

{
    int stat;

    if (var_stack_pointer < stack_size)
	{
	stack[var_stack_pointer++] = value;
	stat = var__normal;
	}
    else
	stat = var__stkovflo; /* stack overflow */

    if (status != NIL)
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);
}



void var_popf (float *value, int *status)

/*
 * popf - get a real number from the stack
 */

{
    int stat;

    if (var_stack_pointer > 0)
	{
	*value = stack[--var_stack_pointer];
	stat = var__normal;
	}
    else
	stat = var__stkundflo; /* stack underflow */

    if (status != NIL)
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);
}



static
void find_variable (char *name, int *status)

/*
 * find_variable - lookup variable in table
 */

{
    string str;
    char *cp;
    int stat;
    BOOL match;
    var_variable_descr *prev_variable;

    strcpy (str, name);
    str_cap (str);

    cp = str;
    if (isalpha(*cp))
	{
	cp++;
	while (isalnum(*cp) || *cp == '_' || *cp == '$')
	    cp++;
	if (*cp == '\0')
	    stat = var__normal;
	else
	    stat = var__invid; /* Syntax: invalid identifier */
	}
    else
	/* Syntax: identifier must begin with alphabetic character */
	stat = var__nonalpha;

    if (odd(stat))
	{
	if (strlen(str) <= var_ident_length)
	    {
	    prev_variable = NIL;
	    variable = first_variable;
	    match = FALSE;
	    while (variable != NIL && (!match))
		{
		prev_variable = variable;
		match = (strcmp(variable->ident, str) == 0);
		if (!match)
		    variable = variable->next;
		}
	    if (!match)
		{
		variable = prev_variable;
		stat = var__undefid; /* undefined identifier */
		}
	    else
		stat = var__existid; /* existing identifier */
	    }
	else
	    stat = var__illidlen; /* identifier exceeds 31 characters */
	}

    if (status != NIL)
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);
}



static
void find_key (int index, int *status)

/*
 * find_key - search specified key
 */

{
    if (index == 0)
	index = variable->allocn;
    if (index >= 0)
	{
	if (index <= variable->allocn)
	    {
	    variable->key = index;
	    *status = var__normal;
	    }
	else
	    *status = var__undefind; /* undefined index */
	}
    else
	*status = var__invind; /* invalid index */
}



void var_define (char *name, int index, int length, float *buffer, int segment,
    int *status)

/*
 * define - define a variable
 */

{
    int stat, count;
    fun_function_descr *addr;
    char str[var_ident_length];

    var_variable_descr *prev_variable;

    strcpy (str, name);
    str_cap (str);

    find_variable (str, &stat);

    if (stat == var__undefid) /* undefined identifier */
	{
	addr = fun_address(str, &stat);

	if (addr == NIL && stat == fun__undefun) /* undefined function */
	    {
	    if (!fun_reserved(str))
		{
		if (index == 0 || index == 1)
		    {
		    prev_variable = variable;
		    variable = (var_variable_descr *)
			malloc (sizeof (var_variable_descr));

		    variable->prev = prev_variable;
		    variable->next = NIL;
		    strcpy (variable->ident, str);
		    variable->data = NIL;
		    variable->segment = NIL;
		    variable->key = 0;
		    variable->allocn = 0;
		    variable->segm = 0;

		    if (prev_variable != NIL)
			prev_variable->next = variable;
		    else
			first_variable = variable;
		    stat = var__existid;
		    }
		else
		    stat = var__invind; /* invalid index */
		}
	    else
		stat = var__reserved; /* reserved name */
	    }
	else
	    stat = var__dupldcl; /* duplicate declaration */
	}
    if (stat == var__existid)
	{
	if (index == variable->allocn + 1)
	    index = 0;

	find_key (index, &stat);

	if (odd(stat))
	    {
	    if (variable->allocn == 0)
		{
		variable->data = (float *) malloc (sizeof (float) * length);
		variable->segment = (int *) malloc (sizeof (int) * length);
		}
	    else
		{
		variable->data = (float *) realloc (variable->data,
		    sizeof (float) * (variable->key + length));
		variable->segment = (int *) realloc (variable->segment,
		    sizeof (int) * (variable->key + length));
		}

	    for (count = 0; count < length; count++)
		{
		variable->data[variable->key + count] = buffer[count];
		variable->segment[variable->key + count] = FALSE;
		}	

	    variable->allocn = variable->key + length;
	    variable->segm = variable->segm + (segment & 1);

	    variable->segment[variable->allocn - 1] = segment;

	    if (index == 0)
		variable->key = variable->allocn;
	    else
		variable->key = index;
	    }
	}

    if (status != NIL)
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);

    var_stack_pointer = 0;
}



void var_define_c (varying_string *nameP, int index, int length, float *buffer,
    int segment, int *status)

{
    char name[255];

    strncpy (name, nameP->body, nameP->len);
    name[nameP->len] = '\0';

    var_define (name, index, length, buffer, segment, status);
}



void var_redefine (char *name, int index, int length, float *buffer,
    int segment, int *status)

/*
 * redefine - redefine a variable
 */

{
    int stat, count;

    find_variable (name, &stat);

    if (odd(stat) || stat == var__existid)
	{
	find_key (index, &stat);

	if (odd(stat))
	    {
	    count = 0;

	    while (count < length && index + count <= variable->allocn)
		{
		variable->data[index + count - 1] = buffer[count];

		if (variable->segment[index + count - 1])
		    variable->segm -= 1;
		if (count == length - 1)
		    {
		    variable->segment[index + count - 1] = segment;
		    if (segment)
			variable->segm += 1;
		    }
		else
		    variable->segment[index + count - 1] = FALSE;

		count++;
		}
	    }
	else
	    count = 0;

	if (count < length)
	    var_define (name, index+count, length-count, buffer+count,
		segment, &stat);
	}
    else
	if (stat == var__undefid)
	    var_define (name, 0, length, buffer, segment, &stat);

    if (status != NIL)
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);

    var_stack_pointer = 0;
}



void var_delete (char *name, int *status)

/*
 * delete - delete a variable
 */

{
    int stat;
    var_variable_descr *prev_variable, *next_variable;

    find_variable (name, &stat);

    if (stat == var__existid)
	{
	free (variable->segment);
	free (variable->data);

	prev_variable = variable->prev;
	next_variable = variable->next;

	free (variable);
	if (next_variable != NIL)
	    next_variable->prev = prev_variable;
	if (prev_variable != NIL)
	    prev_variable->next = next_variable;
	else
	    first_variable = next_variable;

	stat = var__normal;
	}

    if (status != NIL)
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);
}



void var_truncate (char *name, int index, int *status)

/*
 * truncate - truncate a variable
 */

{
    int stat;
    var_variable_descr *prev_variable, *next_variable;

    find_variable (name, &stat);

    if (stat == var__existid)
	{
	if (index >= 1 && index <= variable->allocn)
	    find_key (index, &stat);
	else
	    stat = var__undefind;

	if (odd(stat))
	    {
	    while (index <= variable->allocn)
		{
		if (variable->segment[variable->allocn - 1])
		    variable->segm -= 1;
		variable->allocn -= 1;
		}

	    variable->data = (float *) realloc (variable->data,
		sizeof (float) * variable->allocn);
	    variable->segment = (int *) realloc (variable->segment,
		sizeof (int) * variable->allocn);

	    if (variable->allocn == 0)
		{
		prev_variable = variable->prev;
		next_variable = variable->next;

		free (variable);
		if (next_variable != NIL)
		    next_variable->prev = prev_variable;
		if (prev_variable != NIL)
		    prev_variable->next = next_variable;
		else
		    first_variable = next_variable;
		}
	    }
	}

    if (status != NIL)
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);
}



void var_find (int index, int *status)

/*
 * find - find specified index
 */

{
    int stat;

    if (index > 0)
	{
	variable = first_variable;
	while (variable != NIL)
	    {
	    if (index <= variable->allocn)
		variable->key = index;
	    variable = variable->next;
	    }
	var_key = index;
	stat = var__normal;
	}
    else
	stat = var__invind; /* invalid index */

    if (status != NIL)
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);
}



var_variable_descr *var_address (char *name, int *status)

/*
 * address - return address of variable
 */

{
    int  stat;
    var_variable_descr *addr;

    find_variable (name, &stat);

    if (stat == var__existid)
	{
	addr = variable;
	stat = var__normal;
	}
    else
	addr = NIL;

   if (status != NIL)
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);

    return (addr);
}



void var_deposit (var_variable_descr *context, float value, int *status)

/*
 * deposit_variable - deposit a variable
 */

{
    int stat;
    var_variable_descr *variable;

    variable = context;
    if (var_key <= variable->allocn)
	{
	variable->data[variable->key - 1] = value;
	if (variable->key == var_key)
	    {
	    if (!variable->segment[variable->key - 1])
		stat = var__normal;
	    else
		stat = var__eos; /* end of segment detected */
	    }
	else
	    stat = var__indnotdef; /* index not explicitly defined */
	}
    else
	stat = var__undefind; /* undefined index */

    if (status != NIL)
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);
}



void var_examine (var_variable_descr *context, float *value, int *status)

/*
 * examine - examine a variable
 */

{
    int stat;
    var_variable_descr *variable;

    variable = context;
    if (var_key <= variable->allocn)
	{
	*value = variable->data[variable->key - 1];
	if (variable->key == var_key)
	    {
	    if (!variable->segment[variable->key - 1])
		stat = var__normal;
	    else
		stat = var__eos; /* end of segment detected */
	    }
	else
	    stat = var__indnotdef; /* index not explicitly defined */
	}
    else
	if (variable->allocn == 1)
	    {
	    *value = variable->data[0];
	    stat = var__constant; /* constant expression */
	    }
	else
	    stat = var__undefind; /* undefined index */

    if (status != NIL)
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);
}



void var_examine_entry (var_variable_descr *context, int index, float *value,
    int *status)

/*
 * examine entry - examine a variable entry
 */

{
    int stat;

    variable = context;
    if (index >= 1 && index <= variable->allocn)
	{
	*value = variable->data[index - 1];
        stat = var__normal;
	}
    else
	stat = var__undefind; /* undefined index */

    if (status != NIL)
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);
}



void var_read_variable (var_variable_descr *context, int size, float *buffer,
    int *status)

/*
 * read_variable - read variable into buffer
 */

{
    int index, stat;

    variable = context;
    for (index = 0; index < size; index++)
        buffer[index] = variable->data[index];

    stat = var__normal;

    if (status != NIL)
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);
}



void var_min (char *name, float *result, int *status)

/*
 * min - calculate minimum value
 */

{
    int stat, index;

    find_variable (name, &stat);

    if (stat == var__existid)
	{
        *result = variable->data[0];
	for (index = 0; index < variable->allocn; index++)
	    {
	    if (variable->data[index] < *result)
		*result = variable->data[index];
	    }

	stat = var__normal;
	}

    if (status != NIL)
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);
}



void var_max (char *name, float *result, int *status)

/*
 * max - calculate maximum value
 */

{
    int stat, index;

    find_variable (name, &stat);

    if (stat == var__existid)
	{
        *result = variable->data[0];
	for (index = 0; index < variable->allocn; index++)
	    {
	    if (variable->data[index] > *result)
		*result = variable->data[index];
	    }

	stat = var__normal;
	}

    if (status != NIL)
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);
}



void var_mean (char *name, float *result, int *status)

/*
 * mean - calculate mean value
 */

{
    int stat, index;

    find_variable (name, &stat);

    if (stat == var__existid)
	{
        *result = 0;
	for (index = 0; index < variable->allocn; index++)
	    *result += variable->data[index];
	*result /= variable->allocn;

	stat = var__normal;
	}

    if (status != NIL)
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);
}



void var_stddev (char *name, float *result, int *status)

/*
 * stddev - calculate standard deviation
 */

{
    int stat, n, index;
    float x, sx, sxq;

    find_variable (name, &stat);

    if (stat == var__existid)
	{
        n = variable->allocn;
	sx  = 0;
	sxq = 0;
	for (index = 0; index < n; index++)
	    {
	    x    = variable->data[index];
	    sx  += x;
	    sxq += x*x;
	    }
	if (n > 1)
	    *result = sqrt((sxq-sx*sx/n)/(n-1));
	else
	    *result = -1;

	stat = var__normal;
	}

    if (status != NIL)
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);
}



void var_size (char *name, int *result, int *status)

/*
 * size - return size of variable
 */

{
    int stat;

    find_variable (name, &stat);

    if (stat == var__existid)
	{
        *result = variable->allocn;
	stat = var__normal;
	}
    else
	*result = -1;

    if (status != NIL)
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);
}



void var_total (char *name, float *result, int *status)

/*
 * total - calculate total sum
 */

{
    int stat, index;

    find_variable (name, &stat);

    if (stat == var__existid)
	{
        *result = 0;
	for (index = 0; index < variable->allocn; index++)
	    *result += variable->data[index];

	stat = var__normal;
	}

    if (status != NIL)
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);
}



var_variable_descr *var_inquire_variable (var_variable_descr *context,
    char *name, int *allocation, int *segments, int *size, int *status)

/*
 * inquire_variable - inquire variable information
 */

{
    int	stat;
    var_variable_descr *variable;

    variable = context;
    if (variable == NIL)
	variable = first_variable;

    if (variable != NIL)
	{
	strcpy (name, variable->ident);
	*allocation = variable->allocn;

	if (variable->segm > 0)
	    *segments = variable->segm + 1;
	else
	    *segments = 0;

	*size = sizeof(var_variable_descr) + 
	    variable->allocn * (sizeof(float) + sizeof(int));

	variable = variable->next;
	context = variable;

	if (variable != NIL)
	    stat = var__normal;
	else
	    stat = var__nmv; /* no more variable */
	}
    else
	stat = var__novariable; /* no variables found */

    if (status != NIL)
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);

    return (context);
}

