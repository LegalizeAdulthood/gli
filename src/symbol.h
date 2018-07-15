/*
 *
 * FACILITY:
 *
 *	VAX C Run-Time System
 *
 * ABSTRACT:
 *
 *	'Symbol' Run-Time Library definitions.
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

#define symbol__facility 0x00000852

#define sym__normal	0x08528801 /* normal successful completion */
#define sym__nms	0x08528809 /* no more symbol */
#define sym__supersed	0x08528811 /* previous value has been superseded */
#define sym__nonalpha	0x0852A004 /* Syntax: identifier must begin with alphabetic character */
#define sym__invsymb	0x0852A00C /* Syntax: invalid symbol */
#define sym__illsymlen	0x0852A014 /* symbol exceeds 31 characters */
#define sym__undsymb	0x0852A01C /* undefined symbol */
#define sym__existsym	0x0852A024 /* existing symbol */
#define sym__stringovr	0x0852A02C /* resulting string overflow */
#define sym__illstrlen	0x0852A034 /* string exceeds 255 characters */
#define sym__empty	0x0852A03C /* empty string */
#define sym__nosymbol	0x0852A044 /* no symbols found */


/* Constant definitions */

#define sym_string_length 255
#define sym_ident_length 31


/* Type definitions */

typedef struct sym_symbol_descr_struct {
    struct sym_symbol_descr_struct *prev;
    struct sym_symbol_descr_struct *next;
    char ident[sym_ident_length];
    char equ[sym_string_length];
    } sym_symbol_descr;


/* Entry point definitions */

void sym_define(char *, char *, int *);
void sym_delete(char *, int *);
void sym_translate(char *, char *, int *);
void sym_getenv(char *, char *, int *);
sym_symbol_descr *sym_inquire_symbol(sym_symbol_descr *, char *, char *, int *);
