/*
 *
 * FACILITY:
 *
 *	VAX C Run-Time System
 *
 * ABSTRACT:
 *
 *	'Variable' Run-Time Library definitions.
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

#define	variable__facility 0x00000850

#define	var__indnotdef	0x08508000  /* index not explicitly defined */
#define	var__constant	0x08508008  /* constant expression */
#define	var__normal	0x08508801  /* normal successful completion */
#define	var__nmv	0x08508809  /* no more variable */
#define	var__eos	0x08508811  /* end of segment detected */
#define	var__stkovflo	0x0850A004  /* stack overflow */
#define	var__stkundflo	0x0850A00C  /* stack underflow */
#define	var__nonalpha	0x0850A014  /* Syntax: identifier must begin with
				     * alphabetic character */
#define	var__invid	0x0850A01C  /* Syntax: invalid identifier */
#define	var__illidlen	0x0850A024  /* identifier exceeds 31 characters */
#define	var__dupldcl	0x0850A02C  /* duplicate declaration - name already
				     * declared as function */
#define	var__reserved	0x0850A034  /* reserved name */
#define	var__invind	0x0850A03C  /* invalid index */
#define	var__undefind	0x0850A044  /* undefined index */
#define	var__undefid	0x0850A04C  /* undefined identifier */
#define	var__existid	0x0850A054  /* existing identifier */
#define	var__novariable	0x0850A05C  /* no variables found */
#define	var__illvardsc	0x0850A064  /* illegal variable descriptor */
#define	var__nme	0x0850A06C  /* no more entries */


/* Constant definitions */

#define var_ident_length 31


/* Type definitions */

typedef struct var_variable_struct {
    struct var_variable_struct *prev, *next;
    char ident[var_ident_length];
    float *data;
    int *segment;
    int key, allocn, segm;
    } var_variable_descr;


/* Entry point definitions */

void var_pushf(float, int *);
void var_popf(float *, int *);
void var_define(char *, int, int, float *, int, int *);
void var_redefine(char *, int, int, float *, int, int *);
void var_delete(char *, int *);
void var_truncate(char *, int, int *);
void var_find(int, int *);
var_variable_descr *var_address(char *, int *);
void var_deposit(var_variable_descr *, float, int *);
void var_examine(var_variable_descr *, float *, int *);
void var_examine_entry(var_variable_descr *, int, float *, int *);
void var_read_variable(var_variable_descr *, int, float *, int *);
void var_min(char *, float *, int *);
void var_max(char *, float *, int *);
void var_mean(char *, float *, int *);
void var_stddev(char *, float *, int *);
void var_size(char *, int *, int *);
void var_total(char *, float *, int *);
var_variable_descr *var_inquire_variable(var_variable_descr *, char *, int *,
    int *, int *, int *);
