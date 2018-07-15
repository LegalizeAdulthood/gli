/*
 *
 * FACILITY:
 *
 *	VAX C Run-Time System
 *
 * ABSTRACT:
 *
 *	'String' Run-Time Library definitions.
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

#define string__facility 0x00000824

#define str__success	0x08248801 /* normal successful completion */
#define str__empty	0x08249803 /* empty string */
#define str__intudf	0x0824A004 /* integer underflow */
#define str__intovf	0x0824A00C /* integer overflow */
#define str__invsynint	0x0824A014 /* invalid syntax for an integer */
#define str__fltudf	0x0824A01C /* floating underflow */
#define str__fltovf	0x0824A024 /* floating overflow */
#define str__invsynrea	0x0824A02C /* invalid syntax for a real number */
#define str__invsynuic	0x0824A034 /* invalid syntax for a user identification
				      code */
#define str__invsynpro	0x0824A03C /* invalid syntax for a protection
				      specification */


#define FNode 1
#define	FDevice 2
#define FDirectory 4 
#define FName 8
#define FType 16
#define FVersion 32
#define FAll 63


/* Entry point definitions */

char *str_remove(char *, char, int);
char *str_translate(char *, char, char);
int str_locate(char *, char);
int str_index(char *, char *);
void str_reverse(char *);
char *str_pad(char *, char, int);
char *str_cap(char *);
char *str_element(char *, int, char, char *);
int str_integer(char *, int *);
float str_real(char *, int *);
char *str_dec(char *, int);
char *str_flt(char *, float);
char *str_ftoa(char *, float, float);
int str_match(char *, char *, int);
char *str_parse(char *, char *, int, char *);
float str_atof(char *, char **);
