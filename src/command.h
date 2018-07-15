/*
 *
 * FACILITY:
 *
 *	VAX C Run-Time System
 *
 * ABSTRACT:
 *
 *	'Command' Run-Time Library definitions.
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


/* Status code definitions */

#define cli__facility 0x00000803 

#define cli__normal	0x08038001 /* normal successful completion */
#define cli__empty	0x08038009 /* command line is empty */
#define cli__return	0x08038011 /* return to caller */
#define cli__eos	0x08038019 /* end of segment detected */
#define cli__symbol	0x08038021 /* symbol assignment */
#define cli__variable   0x08038029 /* variable assignment */
#define cli__function	0x08038031 /* function assignment */
#define cli__break	0x08038039 /* break */
#define cli__tcl	0x08038041 /* Tcl command */
#define cli__nocomd	0x08038802 /* no command on line - reenter with alphabetic first character */
#define cli__ivverb	0x0803880A /* unrecognized command verb - check validity and spelling */
#define cli__ivkeyw	0x08038812 /* unrecognized command keyword - check validity and spelling */
#define cli__ivqual	0x0803881A /* unrecognized command qualifier - check validity and spelling */
#define cli__abverb	0x08038822 /* ambiguous command verb - supply more characters */
#define cli__abkeyw	0x0803882A /* ambiguous command keyword - supply more characters */
#define cli__nokeyw	0x08038832 /* qualifier name is missing - append the name to the slash */
#define cli__expsyn	0x0803883A /* invalid expression syntax - check operators and operands */
#define cli__synillexpr	0x08038842 /* Syntax: ill-formed expression */
#define cli__insfprm	0x0803884A /* missing command parameters - supply all required parameters */
#define cli__nolbls	0x08038852 /* label ignored - use only within command procedures */
#define cli__usgoto	0x0803885A /* target of GOTO not found - check spelling and presence of label */
#define cli__usgosub	0x08038862 /* target of GOSUB not found - check spelling and presence of label */
#define cli__nothen	0x0803886A /* IF or ON statement syntax error - check placement of THEN keyword */
#define cli__defovf	0x08038872 /* too many command procedure parameters - limit to eight */
#define cli__nosymbol	0x0803887A /* missing symbol */
#define cli__synapos	0x08038882 /* Syntax: "'" expected */
#define cli__ordexprreq	0x0803888A /* ordinal expression required */
#define cli__maxparm	0x08039000 /* too many parameters - reenter command with fewer parameters */
#define cli__nmd	0x08039008 /* no more data */
#define cli__invgoto	0x08039010 /* GOTO command not allowed on current command level */
#define cli__invgosub	0x08039018 /* GOSUB command not allowed on current command level */
#define cli__invreturn	0x08039020 /* RETURN command not allowed on current command level */
#define cli__invget	0x08039028 /* GET command not allowed on current command level */
#define cli__skptxt	0x08039030 /* textual information (non-numeric records) ignored */
#define cli__eof	0x08039038 /* file is at end-of-file */
#define cli__controlc	0x08039040 /* input has been canceled by keyboard action */
#define cli__errdurget	0x08039048 /* error during GET */
#define cli__invargpas	0x08039050 /* invalid argument passed to CLI routine */
#define cli__invcomd	0x08039058 /* command not allowed on current command level */
#define cli__alreadycon	0x08039804 /* connection already done */
#define cli__confai	0x0803980C /* connection failure */
#define cli__noconnect	0x08039814 /* no connection done */
#define cli__disconfai	0x0803981C /* disconnect failed */
#define cli__nospace	0x08039824 /* not enough space for requested operation */
#define cli__comdnyi	0x0803982C /* command not yet implemented */
#define cli__range	0x08039834 /* parameter out of range */
#define cli__notcl	0x0803983C /* no Tcl interpreter */
#define cli__rpcerror	0x08039844 /* RPC command error */


/* Type definitions */

typedef char cli_string[255];

typedef char cli_verb[31];
typedef char *cli_verb_list;
typedef char cli_delim_set[31];
typedef char cli_label[31];

typedef enum _cli_data_type { 
    type_symbol, type_constant, type_repetition, type_subrange,
    type_range, type_variable, type_function, type_expression
    } cli_data_type;

typedef struct cli_command_struct {
    cli_string line;
    struct cli_command_struct *next;
    } cli_command_descr;

typedef cli_command_descr *cli_command;

typedef	struct cli_frame_struct {
    cli_command_descr *saved_cmd;
    struct cli_frame_struct *saved_fp;
    } cli_frame_descr;

typedef	cli_frame_descr *cli_frame;

typedef struct cli_procedure_struct {
    cli_command_descr *first_command, *command;
    cli_frame_descr *frame_pointer;
    struct cli_procedure_struct *saved_procedure;
    } cli_procedure_descr;

typedef cli_procedure_descr *cli_procedure;

typedef struct {
    int key;
    int eos;
    cli_data_type data_type;
    union {
	struct {
	    cli_verb ident;
	    cli_string equ;
	    } symbol;
	float constant;
	struct {
	    int n;  
	    float value;
	    } repetition;
	struct {
	    float min, max, step;
	    int total;
	    } range;
	struct var_variable_struct *variable;
	struct fun_function_struct *function;
	cli_string expression;
	} type;
    } cli_data_descriptor;


/* Global variables */

#ifndef __cli

extern int (*cli_a_input_routine)(char *, char *, char *, int *);
extern int cli_b_abort;
extern int cli_b_tcl_interp;
extern int cli_b_tcl_got_partial;

#endif /* __cli */


/* Entry point definitions */

tt_cid *cli_connect (char *lognam,
    int (*input_routine)(char *, char *, char *, int *), int *status);
var_variable_descr *cli_get_variable (char *prompt, char *ident, int *size,
    int *status);
int cli_integer (char *chars, int *status);
float cli_real (char *chars, int *status);
int cli_end_of_command ();
cli_procedure cli_open (char *file_spec, int *status);
cli_procedure cli_open_pipe (int *status);
int cli_eof (cli_procedure command_procedure, int *status);

void cli_disconnect (int *status);
void cli_get_keyword (char *prompt, cli_verb_list keyword_list, int *index,
    int *status);
void cli_get_parameter (char *prompt, char *parameter, cli_delim_set delimiters,
    int then_keyword, int punctuation, int *status);
void cli_readln (cli_procedure command_procedure, char *line, int *status);
void cli_goto (cli_label command_label, int *status);
void cli_gosub (cli_label command_label, int *status);
void cli_return (int *status);
void cli_get (char *line, int *status);
void cli_get_command (char *prompt, cli_verb_list command_list, int *index,
    int *status);
void cli_parse_command (char *syntax, int *status);
void cli_call_proc (int (*routine)(char *, ...));
void cli_await_key (char *prompt, char key, int *status);
void cli_get_data (char *prompt, cli_data_descriptor *data, int *status);
void cli_get_data_buffer (cli_data_descriptor *data, int size, float *buffer,
    int *return_length, int *status);
void cli_data (char *chars, int size, float *buffer, int *return_length,
    int *status);
void cli_set_command (char *command, int *status);
void cli_inquire_command (char *command, int *index, int *status);
void cli_close (cli_procedure command_procedure, int *status);
