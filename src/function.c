/*
 *
 * FACILITY:
 *
 *	VAX C Run-Time System
 *
 * ABSTRACT:
 *
 *	This module contains symbolic function definition and evaluation
 *	routines.
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


#include <math.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>


#include "strlib.h"
#include "symbol.h"
#include "variable.h"
#include "function.h"
#include "mathlib.h"
#include "system.h"


#define BOOL int
#define TRUE 1
#define FALSE 0
#define NIL 0

#define odd(status) ((status) & 01)


/* Arithmetic operators */

#define plus '+'
#define minus '-'
#define asterisk '*'
#define slash '/'
#define double_ast "**"
#define equal '='
#define greater_than '>'
#define less_than '<'
#define logical_and "AND"
#define logical_or "OR"

#define blank ' '
#define tab '\t'
#define left_par '('
#define right_par ')'
#define left_brace '['
#define right_brace ']'
#define period '.'
#define exponent_sign 'E'
#define terminator '\0'

#define const_length 15
#define max_exponent 88

#define min_gamma 5.8775E-39
#define max_gamma 34.844
#define pi 3.141592654
#define e 2.718281828


/* Global data structures */

static int parse_index;			/* Parse index */
static BOOL variable_reference;		/* Symbolic reference flag */

static fun_function_descr *first_func = NIL, *func = NIL;


static char str[fun_expr_length];
static char character;
static int read_index;
static fun_instruction_descr *instr, *first_instr;


/* Lookup tables */

static char *constant_table[] = {"PI", "E"};
static float constant_value[] = {pi, e};

static char *function_table[] = {
    "ABS", "ARCCOS", "ARCOSH", "ARCSIN", "ARCTAN", "ARSINH", "ARTANH", "COS", 
    "COSH", "DEG", "ERF", "ERFC", "EXP", "FRAC", "GAMMA", "INT", "LN", "LOG",
    "RAD", "RAN", "RAND", "SIGN", "SIN", "SINH", "SQR", "SQRT", "TAN", "TANH",
    "TRUNC"};

static char *vector_function_table[] = {
    "MAX", "MEAN", "MIN", "SIZE", "STDDEV", "TOTAL"};



static
int sign (float x)

/*
 * sign - return sign x
 */

{
    if (x == 0)
	return (0);
    else
	if (x > 0)
	    return (1);
	else
	    return (-1);
}



static
float fract (float x)

/*
 * fract - return fract x
 */

{
    return (x-(int)(x));
}



int fun_reserved (char *name)

/*
 * reserved - scan reserved name lists
 */

{
    BOOL match;
    int index;

    match = FALSE;
    str_cap (name);

    for (index = (int)constant_pi; index <= (int)constant_e; index = index+1)
	if (strcmp (name, constant_table[index]) == 0) {
	    match = TRUE;
	    goto exit;
	    }

    for (index = (int)function_abs; index <= (int)function_trunc;
	index = index+1)
	if (strcmp (name, function_table[index]) == 0) {
	    match = TRUE;
	    goto exit;
	    }
    
    for (index = (int)function_max; index <= (int)function_total;
	index = index+1)
	if (strcmp (name, vector_function_table[index]) == 0) {
	    match = TRUE;
	    goto exit;
	    }
    exit:
    return (match);
}



static
void find_function (char *name, int *status)

/*
 * find_function - lookup function in table
 */

{
    int stat;
    char *cp;
    BOOL match;
    fun_function_descr *prev_func;

    str_cap(name);
    cp = name;

    if (isalpha(*cp))
	{
	cp++;
	while (isalnum(*cp) || *cp == '_' || *cp == '$')
	    cp++;
	if (*cp == '\0')
	    stat = fun__normal;
	else
	    stat = fun__invfid; /* Syntax: invalid function identifier */
	}
    else
	/* Syntax: identifier must begin with an alphabetic character */
	stat = fun__nonalpha;

    if (odd(stat))
	{
	if (strlen(name) <= fun_ident_length)
	    {
	    prev_func = NIL;
	    func = first_func;
	    match = FALSE;
	    while (func != NIL && (!match))
		{
		prev_func = func;
		match = (strcmp(str_cap(func->ident), name) == 0);
		if (!match)
		    func = func->next;
		}
	    if (!match)
		{
		func = prev_func;
		stat = fun__undefun; /* undefined function */
		}
	    else
		stat = fun__existfun; /* existing function */
	    }
	else
	    /* function identifier exceeds 31 characters */
	    stat = fun__illfidlen;
	}

    if (status != NIL)
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);
}



static
void skip_white_space (void)
{
    while (character == blank || character == tab)
	character = str[read_index++];
}



static
void new_instr (void)

/*
 * new_instr - allocate a new instruction
 */

{
    fun_instruction_descr *prev_instr;

    prev_instr = instr;
    instr = (fun_instruction_descr *) malloc (sizeof (fun_instruction_descr));
    instr->next = NIL;
    if (prev_instr != NIL)
	prev_instr->next = instr;
    else
	first_instr = instr;	
}



static
void parse_constant (int *status)

/*
 * parse_constant - parse numeric constant
 */

{
    char number[const_length];
    int pos;

    strcpy (number, "");
    pos = 0;

    while (isdigit(character))
	{
	number[pos++] = character;
	character = str[read_index++];
	}

    if (character == period)
	{
	number[pos++] = period;
	character = str[read_index++];
	while (isdigit(character))
	    {
	    number[pos++] = character;
	    character = str[read_index++];
	    }
	}

    if (character == exponent_sign)
	{
	number[pos++] = exponent_sign;
	character = str[read_index++];
	if (character == plus || character == minus)
	    {
	    number[pos++] = character;
	    character = str[read_index++];
	    }
	if (isdigit (character))
	    while (isdigit (character))
		{
		number[pos++] = character;
		character = str[read_index++];
		}
	else
	    *status = fun__constexp; /* Syntax: constant expected */
	}
    
    number[pos] = terminator;

    if (odd(*status))
	{
	new_instr ();
	instr->code = code_constant;
	instr->field.constant = str_real(number, status);
	if (!odd(*status))
	    *status = fun__invconst; /* invalid constant */
	}
}



static
void parse_identifier (char *identifier)

/*
 * parse_identifier - parse an identifier
 */

{
    int pos;

    pos = 0;
    do  {
	identifier[pos++] = character;
	character = str[read_index++];
	}
    while (isalnum(character) || character == '_' || character == '$');
    identifier[pos] = terminator;
}



static 
void lookup_predefined_constant (char *identifier, int *match)

/*
 * lookup_predefined_constant - lookup predefined constant
 */

{
    int index;

    for (index = (int)constant_pi; index <= (int)constant_e; index = index+1)
	if (strcmp(identifier, constant_table[index]) == 0)
	    {
	    *match = TRUE;
	    new_instr ();
	    instr->code = code_predefined_constant;
	    instr->field.value = constant_value[index];
	    goto done;
	    }
    done:;
}



static
void factor (int *status);

static
void lookup_intrinsic_function (char *identifier, int *match, int *status)

/*
 * lookup_intrinsic_function - lookup intrinsic function
 */

{
    int index;

    for (index = (int)function_abs; index <= (int)function_trunc;
	index = index+1)
	if (strcmp(identifier, function_table[index]) == 0)
	    {
	    *match = TRUE;
            skip_white_space ();
	    if (character == left_par)
		{
		factor (status);
		if (odd(*status))
		    {
		    new_instr ();
		    instr->code = code_intrinsic_function;
		    instr->field.intrinsic = (fun_intrinsic_function) index;
		    instr->op.seed = 0;
		    }
		}
	    else
		*status = fun__lparexp; /* Syntax: "(" expected */
	    goto done;
	    }
    done:;
}



static
void simple_expression (int *status);

static
void parse_variable_index (int *status)
{
    character = str[read_index++];
    simple_expression(status);
    if (odd(*status))
	if (character == right_brace)
	    character = str[read_index++];
	else
	    *status = fun__rbracexp; /* Syntax: "]" expected */
}



static
void parse_variable (int index, int *status)
{
    char identifier[fun_ident_length];
    var_variable_descr *variable_address;

    character = str[read_index++];
    skip_white_space ();
    if (isalpha(character))
	{
	parse_identifier (identifier);
	variable_address = var_address(identifier, status);
	if (variable_address == NIL || !odd(*status))
	    {
	    parse_index = read_index-strlen(identifier);
	    *status = fun__undefid; /* undefined identifier */
            }
	}
    else
	*status = fun__idexp; /* identifier expected */

    if (odd(*status))
	{
	skip_white_space ();
	if (character == right_par)
	    {
	    character = str[read_index++];
	    new_instr ();
	    instr->code = code_vector_function;
	    instr->field.vector = (fun_vector_function) index;
	    instr->op.ident = (char *) malloc (var_ident_length);
	    strcpy (instr->op.ident, identifier);
	    }
	else
	    *status = fun__rparexp; /* Syntax: ")" expected */
	}
}



static
void lookup_vector_function (char *identifier, int *match, int *status)

/*
 * lookup_vector_function - lookup vector function
 */

{
    int index;

    for (index = (int)function_max; index <= (int)function_total;
	index = index+1)
	if (strcmp(identifier, vector_function_table[index]) == 0)
	    {
	    *match = TRUE;
            skip_white_space ();
	    if (character == left_par)
		parse_variable (index, status);
	    else
		*status = fun__lparexp; /* Syntax: "(" expected */
	    goto done;
	    }
    done:;
}



static
void lookup_variable (char *identifier, int *match, int *status)

/*
 * lookup_variable - lookup variable
 */

{
    int stat;
    var_variable_descr *variable_address;

    variable_address = var_address(identifier, &stat);
    if (odd(stat))
	{
	*match = TRUE;
        skip_white_space ();
	if (character == left_brace)
	    {
	    parse_variable_index (status);
	    new_instr ();
	    instr->code = code_indexed_variable;
	    instr->op.ident = (char *) malloc (var_ident_length);
	    instr->field.reference = (unsigned *) variable_address;
	    strcpy (instr->op.ident, identifier);
	    }
	else
	    {
	    new_instr ();
	    instr->code = code_variable;
	    instr->op.ident = (char *) malloc (var_ident_length);
	    instr->field.reference = (unsigned *) variable_address;
	    strcpy (instr->op.ident, identifier);
	    }
	}
}



static
void lookup_function (char *identifier, int *match, int *status)

/*
 * lookup_function - lookup function
 */

{
    int stat;
    fun_function_descr *function_address;

    function_address = fun_address(identifier, &stat);
    if (odd(stat))
	{
	*match = TRUE;
	new_instr ();
	instr->code = code_function;
	instr->op.ident = (char *) malloc (fun_ident_length);
	instr->field.reference = (unsigned *) function_address;
	strcpy (instr->op.ident, identifier);
	}
}



static
void parse_expression (int *status)

/*
 * parse_expression - parse an expression
 */

{
    char identifier[fun_ident_length];
    BOOL match;

    parse_identifier (identifier);

    match = FALSE;
    lookup_predefined_constant (identifier, &match);
    if (!match)
	{
	lookup_intrinsic_function (identifier, &match, status);
	if (!match)
	    {
	    lookup_vector_function (identifier, &match, status);
	    if (!match)
		{
		lookup_variable (identifier, &match, status);
		if (!match)
		    {
		    lookup_function (identifier, &match, status);
		    if (!match)
			{
			parse_index = read_index-strlen(identifier);
			*status = fun__undefid; /* undefined identifier */
		        }
		    }
		}
	    }
	}
}



static
void term (int *status);

static
void factor (int *status)
{
    skip_white_space ();
    if (isalpha(character))
	parse_expression(status);
    else
	if (character == left_par)
	    {
	    character = str[read_index++];
	    simple_expression(status);
	    if (odd(*status))
		if (character == right_par)
		    character = str[read_index++];
		else
		    *status = fun__rparexp; /* Syntax: ")" expected */
	    }
	else
	    if (character == minus)
		{
		character = str[read_index++];
		if (character != minus)
		    term(status);
		else
		    *status = fun__expresexp; /* Syntax: expression expected */
		new_instr ();
		instr->code = code_negate;
		}
	    else
		if (isdigit(character))
		    parse_constant(status);
		else
		    *status = fun__expresexp; /* Syntax: expression expected */

    if (odd(*status))
       skip_white_space ();
}



static
void primary (int *status)
{
    factor(status);
    while (strncmp(&str[read_index-1], double_ast, 2) == 0 && odd(*status))
	{
	character = str[read_index+1];
	read_index = read_index+2;
	factor(status);

	new_instr ();
	instr->code = code_arithmetic;
	instr->field.arithmetic = operator_raise;
	}
}



static
void term (int *status)
{
    char mul_op;

    primary(status);
    while ((character == asterisk || character == slash) && odd(*status))
	{
	mul_op = character;
	character = str[read_index++];
	primary(status);

	new_instr ();
	instr->code = code_arithmetic;
	if (mul_op == asterisk)
	    instr->field.arithmetic = operator_multiply;
	else
	    instr->field.arithmetic = operator_divide;
	}
}



static
void simple_expression (int *status)
{
    char add_op;

    term(status);
    while ((character == plus || character == minus) && odd(*status))
	{
	add_op = character;
	character = str[read_index++];
	term(status);

	new_instr ();
	instr->code = code_arithmetic;
	if (add_op == plus)
	    instr->field.arithmetic = operator_add;
	else
	    instr->field.arithmetic = operator_subtract;
	}
}



static
void expression (int *status)
{
    fun_relational_operator rel_op;

    simple_expression(status);
    if ((character == equal || character == less_than ||
	 character == greater_than) && odd(*status))
	{
	switch (character)
	    {
	    case equal :
	      rel_op = operator_equal;
	      break;

	    case less_than :
	      if (str[read_index] == greater_than)
		  {
		  character = str[read_index++];
		  rel_op = operator_not_equal;
		  }
	      else
		  if (str[read_index] == equal)
		      {
		      character = str[read_index++];
		      rel_op = operator_less_than_or_equal;
		      }
		  else
		      rel_op = operator_less_than;
	      break;

	    case greater_than :
	      if (str[read_index] == equal) {
		  character = str[read_index++];
		  rel_op = operator_greater_than_or_equal;
		  }
	      else
		  rel_op = operator_greater_than;
	      break;
	    }

	character = str[read_index++];
	simple_expression (status);

	new_instr ();
	instr->code = code_relation;
	instr->field.relation = rel_op;
	}
}



static
void boolean_term (int *status)
{
    expression(status);
    while (strncmp(&str[read_index-1], logical_and, 3) == 0 && odd(*status))
	{
	character = str[read_index+2];
	read_index = read_index+3;
	expression(status);

	new_instr ();
	instr->code = code_logical;
	instr->field.logical = operator_and;
	}
}



static
void boolean_expression (int *status)
{
    boolean_term(status);
    while (strncmp(&str[read_index-1], logical_or, 2) == 0 && odd(*status))
	{
	character = str[read_index+1];
	read_index = read_index+2;
	boolean_term(status);

	new_instr ();
	instr->code = code_logical;
	instr->field.logical = operator_or;
	}
}



static
void parse (char *expression, int logical, int *status)

/*
 * parse - parse a function expression
 */

{
    fun_instruction_descr *next_instr;

    instr = NIL;
    first_instr = NIL;

    strcpy (str, expression);
    str_cap(str);

    *status = fun__normal;
    character = str[0];
    read_index = 1;

    if (logical)
	boolean_expression(status);
    else
	simple_expression(status);

    if (odd(*status))
	{
	if (character != terminator)
	    /* extra characters following a valid expression */
	    *status = fun__extrachar;
	}

    if (!odd(*status))
	{
	instr = first_instr;
	while (instr != NIL)
	    {
	    next_instr = instr->next;
	    if (instr->code == code_variable ||
		instr->code == code_indexed_variable ||
		instr->code == code_function ||
		instr->code == code_vector_function)
		free (instr->op.ident);
	    free (instr);
	    instr = next_instr;
	    }
	first_instr = NIL;
	}

    if (*status != fun__undefid)
	parse_index = read_index;
}



void fun_define (char *name, char *expression, int *status)

/*
 * define - define a function
 */

{
    int stat;
    var_variable_descr *addr;
    fun_function_descr *prev_func;

    find_function (name, &stat);

    if (stat == fun__undefun) /* undefined function */
	{
	addr = var_address(name, &stat);

	if (addr == NIL && stat == var__undefid) /* undefined identifier */
	    {
	    if (!fun_reserved(name))
		{
		if (strlen(expression) <= fun_expr_length)
		    {
		    prev_func = func;
		    parse (expression, FALSE, &stat);

		    if (odd(stat))
			{
			func = (fun_function_descr *)
			    malloc (sizeof (fun_function_descr));
			func->prev = prev_func;
			func->next = NIL;
			strcpy (func->ident, str_cap(name));
			strcpy (func->expr, expression);
			func->instr = first_instr;
			func->called = FALSE;

			if (prev_func != NIL)
			    prev_func->next = func;
			else
			    first_func = func;
			stat = fun__normal;
			}
		    }
		else
		    stat = fun__illstrlen; /* string exceeds 255 characters */
		}
	    else
		stat = fun__reserved; /* reserved name */
	    }
	else
	    stat = fun__dupldcl; /* duplicate declaration */
	}

    if (status != NIL)
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);
}



void fun_delete (char *name, int *status)

/*
 * delete - delete a function
 */

{
    int stat;
    fun_instruction_descr *next_instr;
    fun_function_descr *prev_func, *next_func;

    find_function (name, &stat);
    if (stat == fun__existfun)
	{
	prev_func = func->prev;
	next_func = func->next;
	while (func->instr != NIL)
	    {
	    next_instr = func->instr->next;
	    if (func->instr->code == code_variable ||
		func->instr->code == code_indexed_variable ||
		func->instr->code == code_function ||
		func->instr->code == code_vector_function)
		free (func->instr->op.ident);

	    free (func->instr);
	    func->instr = next_instr;
	    }

	free (func);
	if (next_func != NIL)
	    next_func->prev = prev_func;
	if (prev_func != NIL)
	    prev_func->next = next_func;
	else
	    first_func = next_func;
	stat = fun__normal;
	}

    if (status != NIL)
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);
}



fun_function_descr *fun_address (char *name, int *status)

/*
 * address - return address of function
 */

{
    int stat;
    fun_function_descr *this_func;
    fun_instruction_descr *instruction;

    find_function (name, &stat);
    if (stat == fun__existfun)
	{
	this_func = func;
	stat = fun__normal;
	instruction = this_func->instr;

	while (odd(stat) && (instruction != NIL))
	    {
	    if (instruction->code == code_variable ||
		instruction->code == code_indexed_variable)
		instruction->field.reference = (unsigned *) var_address (
		    instruction->op.ident, &stat);

	    else if (instruction->code == code_function)
		instruction->field.reference = (unsigned *) fun_address (
		    instruction->op.ident, &stat);

	    instruction = instruction->next;
	    }

	if (odd(stat))
	    stat = fun__normal;
	else
	    if (stat == var__undefid) /* undefined identifier */
		stat = fun__undsymref; /* undefined symbol reference */
	}
    else
	this_func = NIL;

    if (status != NIL)
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);

    return (this_func);
}



int fun_parse_index (void)

/*
 * parse_index - return parse index
 */

{
    return (parse_index);
}



fun_function_descr *fun_inquire_function (fun_function_descr *context,
    char *name, char *expression, int *status)

/*
 * inquire_function - inquire function information
 */

{
    int stat;
    fun_function_descr *func;

    func = context;
    if (func == NIL)
	func = first_func;
    if (func != NIL)
	{
	strcpy(name, func->ident);
	strcpy(expression, func->expr);
	func = func->next;
	if (func != NIL)
	    stat = fun__normal;
	else
	    stat = fun__nmf; /* no more function */
	}
    else
	stat = fun__nofunction; /* no functions found */

    if (status != NIL)
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);
    return (func);
}



static 
void evaluate_arithmetic_operation (fun_arithmetic_operator arithmetic,
    int *status)

/*
 * evaluate_arithmetic_operation - evaluate an arithmetic operation
 */

{
    float source_1, source_2, destination = 0;

    var_popf (&source_2, NIL);
    var_popf (&source_1, NIL);

    switch (arithmetic)
	{
	case operator_add :
	  destination = source_1+source_2;
	  break;

	case operator_subtract :
	  destination = source_1-source_2;
	  break;

	case operator_multiply :
	  destination = source_1*source_2;
	  break;

	case operator_divide :
	  if (source_2 != 0)
	      destination = source_1/source_2;
	  else
	      *status = fun__fltdiv; /* floating zero divide */
	  break;

	case operator_raise :
	  if (source_1 >= 0)
	      destination = pow((double)source_1, (double)source_2);
	  else
	      if (fract(source_2) == 0)
		  {
		  destination = pow(fabs((double)source_1), (double)source_2);
		  if (odd((int)(source_2)))
		      destination = -destination;
		  }
	      else
		  *status = fun__undexp; /* undefined exponentiation */
	  break;
	}

    var_pushf (destination, NIL);
}



static
void evaluate_relational_operation (fun_relational_operator relation,
    int *status)

/*
 * evaluate_relational_operation - evaluate an relational operation
 */

{
    float source_1, source_2, result;
    BOOL destination = FALSE;

    var_popf (&source_2, NIL);
    var_popf (&source_1, NIL);

    switch (relation)
	{
	case operator_equal :
	  destination = (source_1 == source_2);
	  break;

	case operator_not_equal :
	  destination = (source_1 != source_2);
	  break;

	case operator_less_than :
	  destination = (source_1 < source_2);
	  break;

	case operator_less_than_or_equal :
	  destination = (source_1 <= source_2);
	  break;

	case operator_greater_than :
	  destination = (source_1 > source_2);
	  break;

	case operator_greater_than_or_equal :
	  destination = (source_1 >= source_2);
	  break;
	}

    result = (float) destination;
   
    var_pushf (result, NIL);
}



static
void evaluate_logical_operation (fun_logical_operator logical, int *status)

/*
 * evaluate_logical_operation - evaluate an logical operation
 */

{
    float source_1, source_2, result;
    BOOL destination = FALSE;

    var_popf (&source_2, NIL);
    var_popf (&source_1, NIL);

    switch (logical)
	{
	case operator_and :
	  destination = (source_1 != 0) && (source_2 != 0);
	  break;

	case operator_or :
	  destination = (source_1 != 0) || (source_2 != 0);
	  break;
	}

    result = (float) destination;
    
    var_pushf (result, NIL);
}



static
void evaluate_intrinsic_function (fun_intrinsic_function index, double *seed,
    int *status)

/*
 * evaluate_intrinsic_function - evaluate an intrinsic function
 */

{
    float source, destination = 0;

    var_popf (&source, NIL);

    switch (index)
      {
      case function_abs :
	destination = fabs((double)source);
	break;

      case function_arccos :
	if (source > -1.0 && source < 1.0)
	    destination = pi/2.0-atan(source/sqrt(1.0-source*source));
	else
	    if (source == 1.0)
		destination = 0.0;
	    else if (source == -1.0)
		destination = pi;
	    else
		*status = fun__invargmat; /* invalid argument to math library */
	break;

      case function_arcosh :
	if (source >= 1.0)
	    destination = log(source+sqrt(source*source-1.0));
	else
	    *status = fun__invargmat; /* invalid argument to math library */
	break;

      case function_arcsin :
	if (source > -1.0 && source < 1.0)
	    destination = atan(source/sqrt(1.0-source*source));
	else
	    if (source == 1.0)
		destination = pi/2.0;
	    else if (source == -1.0)
		destination = -pi/2.0;
	    else
		*status = fun__invargmat; /* invalid argument to math library */
	break;

      case function_arctan :
	destination = atan((double)source);
	break;

      case function_arsinh :
	destination = log(source+sqrt(source*source+1.0));
	break;

      case function_artanh :
	if (source > -1.0 && source < 1.0)
	    destination = 0.5*log((1.0+source)/(1.0-source));
	else
	    *status = fun__invargmat; /* invalid argument to math library */
	break;

      case function_cos :
	destination = cos(source);
	break;

      case function_cosh :
	if (source <= (float)max_exponent)
	    destination = 0.5*(exp(source)+exp(-source));
	else
	    *status = fun__invargmat; /* invalid argument to math library */
	break;

      case function_deg :
	destination = 180.0/pi * source;
	break;

      case function_erf :
	destination = mth_erf(source);
	break;

      case function_erfc :
	destination = mth_erfc(source);
	break;

      case function_exp :
	if (source <= (float)max_exponent)
	    destination = exp(source);
	else
	    /* floating overflow in math library EXP */
	    *status = fun__ovfexp;
	break;

      case function_frac :
	destination = source-(int)(source);
	break;

      case function_gamma :
	if (min_gamma < fabs(source) && fabs(source) < max_gamma &&
	     !(source < 0.0 && fract(source) == 0.0))
	    destination = mth_gamma (source);
	else
	    *status = fun__invargmat; /* invalid argument to math library */
	break;

      case function_int :
      case function_trunc :
	destination = (int)(source);
	break;

      case function_ln :
	if (source > 0.0)
	    destination = log(source);
	else
	    *status = fun__logzerneg; /* logarithm of zero or negative value */
	break;

      case function_log :
	if (source > 0.0)
	    destination = log10(source);
	else
	    *status = fun__logzerneg; /* logarithm of zero or negative value */
	break;

      case function_rad :
	destination = pi/180.0 * source;
	break;

      case function_ran :
	if (*seed == 0.0)
	    *seed = source;
	if (*seed != 0)
	    destination = mth_ran(seed);
	else
	    *status = fun__invargmat; /* invalid argument to math library */
	break;
      
      case function_rand :
	if (*seed == 0.0)
	    *seed = source;
	if (*seed != 0)
	    destination = mth_rand(seed);
	else
	    *status = fun__invargmat; /* invalid argument to math library */
	break;

      case function_sign :
	destination = (float)sign(source);
	break;

      case function_sin :
	destination = sin(source);
	break;

      case function_sinh :
	if (source <= (float)max_exponent)
	    destination = 0.5*(exp(source)-exp(-source));
	else
	    *status = fun__invargmat; /* invalid argument to math library */
	break;

      case function_sqr :
	destination = source*source;
	break;

      case function_sqrt :
	if (source >= 0.0)
	    destination = sqrt(source);
	else
	    *status = fun__squrooneg; /* square root of negative number */
	break;

      case function_tan :
	if (cos(source) != 0.0)
	    destination = sin(source)/cos(source);
	else
	    *status = fun__invargmat; /* invalid argument to math library */
	break;

      case function_tanh :
	if (source <= (float)max_exponent)
	    destination = (exp(source)-exp(-source))/(exp(source)+exp(-source));
	else
	    *status = fun__invargmat; /* invalid argument to math library */
	break;
      }

    var_pushf (destination, NIL);
}



static
void evaluate_vector_function (char *variable, fun_vector_function index,
    int *status)

/*
 * evaluate_vector_function - evaluate a vector function
 */

{
    float destination = 0;

    switch (index)
      {
      case function_max :
	var_max (variable, &destination, status);
	break;
      
      case function_mean :
	var_mean (variable, &destination, status);
	break;
      
      case function_min :
	var_min (variable, &destination, status);
	break;
      
      case function_size :
	{
	int size;
	var_size (variable, &size, status);
	destination = size;
	break;
	}

      case function_stddev :
	var_stddev (variable, &destination, status);
	break;

      case function_total :
	var_total (variable, &destination, status);
	break;
      }

    var_pushf (destination, NIL);
}



static
void evaluate_function (fun_instruction_descr *instr, int *status);

static
void function_evaluate (fun_function_descr *func, float *value, int *status)

/*
 * function_evaluate - evaluate function
 */

{
    if (!func->called)
	{
	func->called = TRUE;
	evaluate_function(func->instr, status);
	func->called = FALSE;
	if (odd(*status))
	    {
	    if (*status != var__eos)
		*status = fun__normal;
	    else
		*status = fun__eos; /* end of segment detected */
	    }
	}
    else
	*status = fun__ilfuncall; /* illegal function call */

    var_popf (value, NIL);
}



static
void evaluate_function (fun_instruction_descr *instr, int *status)

/*
 * evaluate_function - evaluate function code
 */

{
    fun_instruction_descr *this_instr;
    fun_function_descr *this_func;
    var_variable_descr *this_var;
    int index;
    float source = 0;

    this_instr = instr;
    *status = fun__normal;
    while (odd(*status) && (this_instr != NIL))
	{
	switch (this_instr->code)
	    {
	    case code_arithmetic :
	      evaluate_arithmetic_operation (
		this_instr->field.arithmetic, status);
	      break;

	    case code_relation :
	      evaluate_relational_operation (
	        this_instr->field.relation, status);
	      break;

	    case code_logical :
	      evaluate_logical_operation (this_instr->field.logical, status);
	      break;

	    case code_negate :
	      var_popf (&source, NIL);
	      var_pushf (-source, NIL);
	      break;

	    case code_constant :
	      var_pushf (this_instr->field.constant, NIL);
	      break;

	    case code_variable :
	      this_var = (var_variable_descr *) this_instr->field.reference;
	      var_examine (this_var, &source, status);
	      var_pushf (source, NIL);
	      if (*status == var__constant)
		*status = var__normal;
	      else
		variable_reference = TRUE;
	      break;

	    case code_indexed_variable :
	      this_var = (var_variable_descr *) this_instr->field.reference;
	      var_popf (&source, NIL);
	      index = (int)(source+0.5);
	      var_examine_entry (this_var, index, &source, status);
	      var_pushf (source, NIL);
	      break;

	    case code_function :
	      this_func = (fun_function_descr *) this_instr->field.reference;
	      function_evaluate (this_func, &source, status);
	      var_pushf (source, NIL);
	      break;

	    case code_predefined_constant :
	      var_pushf (this_instr->field.value, NIL);
	      break;

	    case code_intrinsic_function :
	      evaluate_intrinsic_function (this_instr->field.intrinsic, 
		&this_instr->op.seed, status);
	      break;
	    
	    case code_vector_function :
	      evaluate_vector_function (this_instr->op.ident,
		this_instr->field.vector, status);
	      break;
	    }

	this_instr = this_instr->next;
	}
}



void fun_parse (char *expression, float *value, int logical, int *status)

/*
 * parse - parse expression
 */

{
    int stat;
    fun_instruction_descr *next_instr;

    parse (expression, logical, &stat);

    if (odd(stat) && (value != NIL))
	{
	variable_reference = FALSE;
	evaluate_function (first_instr, &stat);
	var_popf (value, NIL);
	if (odd(stat) && !variable_reference) 
	    stat = fun__constant; /* constant expression */
	}
    instr = first_instr;
    while (instr != NIL)
	{
	next_instr = instr->next;
	if (instr->code == code_variable ||
	    instr->code == code_indexed_variable ||
	    instr->code == code_function ||
	    instr->code == code_vector_function)
	    free (instr->op.ident);

	free (instr);
	instr = next_instr;
	}

    if (status != NIL)
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);
}



void fun_evaluate (fun_function_descr *func, float *value, int *status)

/*
 * evaluate - evaluate function
 */

{
    int stat;

    variable_reference = FALSE;
    function_evaluate (func, value, &stat);
    if (odd(stat) && !variable_reference)
	stat = fun__constant; /* constant expression */

    if (status != NIL)
	*status = stat;
    else
	if (odd(!stat))
	    raise_exception (stat, 0, NULL, NULL);
}
