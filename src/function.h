/*
 *
 * FACILITY:
 *
 *	VAX C Run-Time System
 *
 * ABSTRACT:
 *
 *	'Function' Run-Time Library definitions.
 *
 * AUTHOR:
 *
 *	J.Heinen
 *
 * VERSION:
 *
 *	V1.0-00
 *
 */


/* Status code definitions */

#define function__facility 0x00000851
#define fun__normal	0x08518801 /* normal successful completion */
#define fun__nmf	0x08518809 /* no more function */
#define fun__eos	0x08518811 /* end of segment detected */
#define fun__constant	0x08518819 /* constant expression */
#define fun__undefid	0x08519002 /* undefined identifier */
#define fun__invconst	0x0851900A /* invalid constant */
#define fun__constexp	0x08519012 /* Syntax: constant expected */
#define fun__lparexp	0x0851901A /* Syntax: "(" expected */
#define fun__rparexp	0x08519022 /* Syntax: ")" expected */
#define fun__lbracexp	0x0851902A /* Syntax: "[" expected */
#define fun__rbracexp	0x08519032 /* Syntax: "]" expected */
#define fun__expresexp	0x0851903A /* Syntax: expression expected */
#define fun__extrachar	0x08519042 /* extra characters following a valid
				    * expression */
#define fun__idexp	0x0851904A /* Syntax: identifier expected */
#define fun__existfun	0x0851A004 /* existing function */
#define fun__nonalpha	0x0851A00C /* Syntax: identifier must begin with
				    * alphabetic character */
#define fun__invfid	0x0851A014 /* Syntax: invalid function identifier */
#define fun__illfidlen	0x0851A01C /* function identifier exceeds 31 characters
				    */
#define fun__dupldcl	0x0851A024 /* duplicate declaration - name already
				    * declared as variable */
#define fun__reserved	0x0851A02C /* reserved name */
#define fun__undefun	0x0851A034 /* undefined function */
#define fun__empty	0x0851A03C /* empty string */
#define fun__illstrlen	0x0851A044 /* string exceeds 255 characters */
#define fun__nofunction	0x0851A04C /* no functions found */
#define fun__fltdiv	0x0851A054 /* floating zero divide */
#define fun__undexp	0x0851A05C /* undefined exponentiation */
#define fun__ovfexp	0x0851A064 /* floating overflow in math library EXP */
#define fun__logzerneg	0x0851A06C /* logarithm of zero or negative number */
#define fun__squrooneg	0x0851A074 /* square root of negative number */
#define fun__invargmat	0x0851A07C /* invalid argument to math library */
#define fun__ilfuncall	0x0851A084 /* illegal function call */
#define fun__undsymref	0x0851A08C /* undefined symbol reference */


/* Constant definitions */

#define fun_ident_length 31
#define fun_expr_length 255


/* Type definitions */

typedef enum {operator_add, operator_subtract, 
    operator_multiply, operator_divide, operator_raise} fun_arithmetic_operator;

typedef enum {operator_equal, operator_not_equal, 
    operator_less_than, operator_less_than_or_equal, operator_greater_than, 
    operator_greater_than_or_equal} fun_relational_operator;

typedef enum {operator_not, operator_and, operator_or} fun_logical_operator;

typedef enum {code_arithmetic, code_relation, code_logical, code_negate, 
    code_constant, code_variable, code_indexed_variable, code_function, 
    code_predefined_constant, code_intrinsic_function, code_vector_function}
    fun_instruction_code;

typedef enum {constant_pi, constant_e} fun_predefined_constant;

typedef enum {function_abs, function_arccos, 
    function_arcosh, function_arcsin, function_arctan, function_arsinh, 
    function_artanh, function_cos, function_cosh, 
    function_deg, function_erf, function_erfc, function_exp, 
    function_frac, function_gamma, function_int, function_ln, 
    function_log, function_rad, function_ran, function_rand, function_sign, 
    function_sin, function_sinh, function_sqr, function_sqrt, 
    function_tan, function_tanh, function_trunc} fun_intrinsic_function;

typedef enum {function_max, function_mean, function_min, function_size,
    function_stddev, function_total}
    fun_vector_function;


/* Instruction descriptor */

typedef struct fun_instruction_struct {
    struct fun_instruction_struct *next;	/* Pointer to next */
    fun_instruction_code code;			/* Instruction code */
    union {
	fun_arithmetic_operator arithmetic;	/* Arithmetic operation */
	fun_relational_operator relation;	/* Relational operation */
	fun_logical_operator logical;		/* Logical operation */
	float constant;				/* Constant value */
	float value;				/* Predefined constant */
	fun_intrinsic_function intrinsic;	/* Intrinsic function */
	fun_vector_function vector;		/* Vector function */
	unsigned *reference;			/* Ext. reference */
	} field;
    union {
	double seed;				/* Seed variable */
	char *ident;				/* Identifier */
	} op;
    } fun_instruction_descr;


/* Function descriptor */

typedef struct fun_function_struct {
    struct fun_function_struct		/* Previous, next function */
	*prev, *next;
    char ident[fun_ident_length];	/* Identifier */
    char expr[fun_expr_length];		/* Expression */
    fun_instruction_descr *instr;	/* Pointer to 1st instruction */
    int called;				/* Call flag */
    } fun_function_descr;


/* Entry point definitions */

int fun_reserved(char *);
void fun_define(char *, char *, int *);
void fun_delete(char *, int *);
fun_function_descr *fun_address(char *, int *);
int fun_parse_index(void);
fun_function_descr *fun_inquire_function(fun_function_descr *, char *, char *,
    int *);
void fun_parse(char *, float *, int, int *);
void fun_evaluate(fun_function_descr *, float *, int *);

