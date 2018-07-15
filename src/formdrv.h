/*
 *
 * FACILITY:
 *
 *	VAX C Run-Time System
 *
 * ABSTRACT:
 *
 *	'Form Driver' Run-Time Library definitions.
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

/* Predeclared event numbers */

#define fdv_c_hangup	(-9)	/* CTRL/] */
#define fdv_c_help	(-8)	/* ? */
#define fdv_c_update	(-7)	/* update cycle */
#define fdv_c_init	(-6)	/* initialization phase */
#define fdv_c_refresh	(-5)	/* CTRL/R, CTRL/W */
#define fdv_c_f4	(-4)	/* F4 */
#define fdv_c_f3	(-3)	/* F3 */
#define fdv_c_f2	(-2)	/* F2 */
#define fdv_c_f1	(-1)	/* F1 */
#define fdv_c_ctrl_z	0	/* CTRL/Z */

typedef enum {
    mode_conversational, mode_monitor
    } fdv_mode;

/* Status code definitions */

#define fdv__facility 0x00000829

#define fdv__normal	0x08298801 /* normal successful completion */
#define fdv__do		0x08298809 /* signal event */
#define fdv__return	0x08298811 /* return from current menu */
#define fdv__rewind	0x08298819 /* rewind tree */
#define fdv__quit	0x08298821 /* quit menu */
#define fdv__exit	0x08298829 /* exit menu */
#define fdv__save	0x08298831 /* save menu */
#define fdv__unsave	0x08298839 /* unsave menu */
#define fdv__retain	0x08298841 /* retain message */
#define fdv__hangup	0x08298849 /* hangup menu */
#define fdv__error	0x08299002 /* error status */
#define fdv__initfai	0x08298003 /* initialization failed */
#define fdv__illcall	0x0829A004 /* recursive menu operation */
#define fdv__filnotfnd	0x0829A00C /* menu description file not found */
#define fdv__invmenu	0x0829A014 /* invalid menu field descriptor */
#define fdv__strtoolon	0x0829A01C /* string is too long */
#define fdv__illmenlen	0x0829A024 /* menu exceeds 20 lines */
#define fdv__openfai	0x0829A02C /* save file open failure */
#define fdv__invsynenu	0x0829A034 /* invalid syntax for an enumeration type */
#define fdv__illformat	0x0829A03C /* save file has illegal format */
#define fdv__nomenu	0x0829A044 /* no menu loaded */
#define fdv__invkey	0x0829A04C /* invalid key */
#define fdv__undefkey	0x0829A054 /* undefined key */
#define fdv__nodisp	0x0829A05C /* no menu displayed */
#define fdv__nocomreg	0x0829A064 /* no communication region */
#define fdv__parse	0x0829C000 /* error parsing line !SL, column !SL */
#define fdv__range	0x0829C008 /* internal inconsistency, value out of
				      range */
#define fdv__invaddr	0x0829C010 /* invalid communication region address */
#define fdv__noname	0x0829C018 /* no such menu field name */


/* Entry point definitions */

void fdv_load (char *, char *, void *, char *, int *);
void fdv_disp (int, void (*)(int *, int *), void *, fdv_mode, int, int *);
int fdv_present (var_variable_descr *, int *);
void fdv_update (int *);
void fdv_message (char *, int *);
void fdv_text (char *, int, int, int, int *);
void fdv_clear (int, int *);
void fdv_call (void (*)(void), int *);
void fdv_status (int *);
char *fdv_get (char *, int *);
void fdv_exit (int *);
void fdv_save (int, int *);
void fdv_error (char *, int *);
void fdv_define (int *);
