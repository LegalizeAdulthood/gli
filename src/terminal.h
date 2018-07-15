/*
 *
 * FACILITY:
 *
 *	VAX C Run-Time System
 *
 * ABSTRACT:
 *
 *	'Terminal' Run-Time Library definitions.
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


#define tt_k_silo_size 128	/* Output silo size */
#define tt_k_cmd_length 200	/* Maximum command length */
#define tt_k_cmd_buffer_size 40	/* Command recall buffer size */


#define tt_k_null '\0'		/* <NUL> */
#define tt_k_controlc '\3'	/* <CTRL/C> */
#define tt_k_shift_out '\16'	/* <SO> */
#define tt_k_shift_in '\17'	/* <SI> */
#define tt_k_cancel '\30'	/* <CAN> */
#define tt_k_hangup '\35'	/* <CTRL/]> */

/* Keypad keys */

#define tt_k_kp_0 '\200'	/* Keypad 0	*/
#define tt_k_kp_1 '\201'	/* Keypad 1	*/
#define tt_k_kp_2 '\202'	/* Keypad 2	*/
#define tt_k_kp_3 '\203'	/* Keypad 3	*/
#define tt_k_kp_4 '\204'	/* Keypad 4	*/
#define tt_k_kp_5 '\205'	/* Keypad 5	*/
#define tt_k_kp_6 '\206'	/* Keypad 6	*/
#define tt_k_kp_7 '\207'	/* Keypad 7	*/
#define tt_k_kp_8 '\210'	/* Keypad 8	*/
#define tt_k_kp_9 '\211'	/* Keypad 9	*/
#define tt_k_kp_hyp '\217'	/* Keypad -	*/
#define tt_k_kp_com '\220'	/* Keypad ,	*/
#define tt_k_kp_ntr '\221'	/* Keypad ENTER	*/
#define tt_k_kp_per '\222'	/* Keypad .	*/

/* Function keys */

#define tt_k_pf_1 '\212'	/* PF1		*/
#define tt_k_pf_2 '\213'	/* PF2		*/
#define tt_k_pf_3 '\214'	/* PF3		*/
#define tt_k_pf_4 '\215'	/* PF4		*/

/* Arrow keys */

#define tt_k_ar_up '\244'	/* UPARROW	*/
#define tt_k_ar_down '\245'	/* DOWNARROW	*/
#define tt_k_ar_right '\246'	/* RIGHTARROW	*/
#define tt_k_ar_left '\247'	/* LEFTARROW	*/
#define tt_k_home '\250'	/* HOME		*/

/* Special VT2xx keys */

#define tt_k_find '\231'	/* Find */
#define tt_k_insert '\232'	/* Insert here */
#define tt_k_remove '\233'	/* Remove */
#define tt_k_select '\234'	/* Select */
#define tt_k_prev_scr '\235'	/* Previous screen */
#define tt_k_next_scr '\236'	/* Next screen */

#define tt_k_f1 '\253'		/* F1 */
#define tt_k_f2 '\254'		/* F2 */
#define tt_k_f3 '\255'		/* F3 */
#define tt_k_f4 '\256'		/* F4 */
#define tt_k_f5 '\257'		/* F5 */

#define tt_k_f6 '\261'		/* F6 (Cancel) */
#define tt_k_f7 '\262'		/* F7 */
#define tt_k_f8 '\263'		/* F8 */
#define tt_k_f9 '\264'		/* F9 */
#define tt_k_exit '\265'	/* F10 (Exit) */

#define tt_k_f11 '\267'		/* F11 */
#define tt_k_bol '\270'		/* F12 (Beginning of line) */
#define tt_k_del_word '\271'	/* F13 (Delete word) */
#define tt_k_ins_ovs '\272'	/* F14 (Insert/Overstrike) */

#define tt_k_help '\274'	/* Help */
#define tt_k_do '\275'		/* Do */

#define tt_k_f17 '\277'		/* F17 */
#define tt_k_f18 '\300'		/* F18 */
#define tt_k_f19 '\301'		/* F19 */
#define tt_k_f20 '\302'		/* F20 */


/* Terminal mode */

#define mode_numeric 1
#define mode_application 2

/* Video attributes */

#define attribute_bold 1
#define attribute_underline 2
#define attribute_blinking 4
#define attribute_reverse 8


/* Status code definitions */

#define terminal__facility 0x00000853

#define tt__eof		0x08538000 /* end of file detected */
#define tt__cancel	0x08538008 /* I/O operation canceled */
#define tt__timeout	0x08538010 /* timeout period expired */
#define tt__hangup	0x08538018 /* data set hang-up */
#define tt__badparam	0x08538020 /* bad parameter value */
#define tt__normal	0x08538801 /* normal successful completion */
#define tt__help	0x08538809 /* help key pressed */
#define tt__controlc	0x08538811 /* input has been canceled by keyboard action */
#define tt__confai	0x0853A004 /* connection failure */
#define tt__invconid	0x0853A00C /* invalid connection identifier */
#define tt__disconfai	0x0853A014 /* disconnect failed */
#define tt__canfai	0x0853A01C /* cancel failed */


/* Terminal control block */

typedef struct tt_r_tcb
    {
    unsigned int chan;
    char output_silo[tt_k_silo_size];
    int read_index;
    int write_index;
    char cmd_buffer[tt_k_cmd_buffer_size+1][tt_k_cmd_length];
    char mailbox_buffer[tt_k_cmd_length];
    } tt_cid;

typedef struct
    {
    unsigned short w0, w1, w2, w3;
    } tt_r_quad;

typedef int tt_attributes;


/* Global variables */

#ifndef __tt

extern unsigned char tt_b_disable_help_key;

#endif /* __tt */


/* Entry point definitions */

tt_cid *tt_connect(char *, int *);
void tt_disconnect(tt_cid *, int *);
void tt_put_chars(tt_cid *, char *, int, int, unsigned int, int *);
void tt_fflush(FILE *);
void tt_flush(tt_cid *, int *);
void tt_put_line(tt_cid *, char *, char *, char *, int *);
void tt_printf(char *, ...);
void tt_fprintf(FILE *, char *, ...);
void tt_fputs(char *, FILE *);
void tt_ring_bell(tt_cid *, int *);
void tt_get_key(tt_cid *, char *, int, int *);
int tt_getchar(void);
void tt_cancel(tt_cid *, char *, int *);
void tt_write_buffer(tt_cid *, char *, int, int *);
void tt_get_line(tt_cid *, char *, char *, int, int, int, char *, int *);
char *tt_fgets(char *, int, FILE*);
void tt_clear_line(tt_cid *, int, int *);
void tt_clear_disp(tt_cid *, int *);
void tt_clear_home(tt_cid *, int *);
void tt_more(char *, ...);
void tt_set_terminal(tt_cid *, unsigned int, int, int, int, int *);
