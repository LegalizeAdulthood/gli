/*
 *
 * FACILITY:
 *
 *	Graphical Kernel System (GKS)
 *
 * ABSTRACT:
 *
 *	This module converts binary input/output to or from a PLOT 10 Terminal
 *	Control System into corresponding GKS commands.
 *
 * AUTHOR(S):
 *
 *	Josef Heinen
 *	Jochen Werner (C Version)
 *
 * VERSION:
 *
 *	V1.0-01
 *
 * MODIFIED BY:
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef VMS
#include <descrip.h>
#endif

#ifdef cray
#include <fortran.h>
#endif

#include "gksdefs.h"
#include "gus.h"


#define IN &

/* Constant definitions */

#define TRUE	1
#define FALSE	0

/* Characters */

#define nul		'\000'	/* <NUL> */
#define eot		'\004'	/* <EOT> */
#define enq		'\005'	/* <ENQ> */
#define bel		'\007'	/* <BEL> */
#define bs		'\010'	/* Backspace */
#define ht		'\011'	/* Horizontal Tab */
#define lf		'\012'	/* Line Feed */
#define vt		'\013'	/* Vertical Tab */
#define ff		'\014'	/* Form Feed */
#define cr		'\015'  /* Carriage Return */
#define so		'\016'	/* <SO> */
#define si		'\017'	/* <SI> */
#define syn		'\026'	/* <SYN> */
#define etb		'\027'	/* <ETB> */
#define can		'\030'	/* <CAN> */
#define sub		'\032'	/* <SUB> */
#define esc		'\033'	/* <ESC> */
#define fs		'\034'	/* <FS> */
#define gs		'\035'	/* <GS> */
#define rs		'\036'	/* <RS> */
#define us		'\037'	/* <US> */
#define csi		'\233'	/* <CSI> 155 */
#define ss3		'\217'	/* <SS3> 143 */
#define ctrl_z		'\032'
#define space		'\040'
#define native_mode	cr
#define point_plot_mode	fs
#define vector_mode	gs
#define inc_plot_mode   rs
#define alpha_mode	us	/* Other */
#define buf_size	1024	/* Buffer size */


#define hiy	(01<<0)
#define xloy	(01<<1)
#define loy     (01<<2)
#define hix     (01<<3)
#define lox     (01<<4)

typedef unsigned int byte;

typedef byte data_bytes;

typedef char kade_array[buf_size+1];

typedef int data_byte_array[5];


static int errfil = 0;		    /* GKS error file */
static int bufsiz = -1;
static int wkid = 1;		    /* GKS workstation identifier */
static int csize = 1;
static int x_margin = 0, x_cursor = 0;
static int y_cursor = 3028;

static char operating_mode = native_mode;

static int flags[13] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

static int escape_start = FALSE, escape = FALSE, ansi_escape = FALSE;
static char esc_seq[buf_size];

static int inp_byte[5] = {0, 0, 0, 0, 0};
static int inp_byte_count = 0;

static int outp_byte[6];
static int outp_byte_count = 0;

static float px[buf_size], py[buf_size];
static int np = 0;

static char chars[buf_size];
static int char_len = 0;

static int opsta = 0;

static float wn[4], vp[4];
static int tnr, xformno, ltype, mtype, halign, valign;
static float chh;

static int data_byte[5] = {0, 0, 0, 0, 0};

static int state_table[10][3] = {{1, 9, 2}, {0, 1, 3}, {6, 7, 7}, {9, 3, 4}, 
    {5, 2, 0}, {0, 10, 0}, {0, 8, 0}, {8, 5, 0}, {0, 6, 0}, {0, 4, 0}};

static data_bytes required_bytes[11] = {0, hiy|lox, hiy|xloy|loy|lox, 
    hiy|loy|lox, hiy|loy|hix|lox, xloy|loy|lox, xloy|loy|hix|lox, loy|lox, 
    loy|hix|lox, lox, hiy|xloy|loy|hix|lox};

static int linetype[16] = {1, 3, 4, 2, -1, -2, -3, -4, 1, 3, 4, 2, -1, -2, -3, 
    -4};

static int height[4] = {88, 83, 53, 48};

static int width[4] = {56, 51, 34, 31};



static
void initialize_data_bytes(void)

/*
 * initialize_data_bytes - initialize graphic data bytes
 */

{
    np = 0;
    inp_byte_count = 0;
}



static
void decode_data_bytes (int nchar, data_byte_array kade, int *x, int *y)

/*
 * decode_data_bytes - decode graphic data bytes
 */

{
    int byte_number, code, state;
    int byte_name;

    state = 0;
    for (byte_number = 0; byte_number < nchar; byte_number++)
        {
        code = (int) kade[byte_number] / 32;
        state = state_table[state][code-1];
        }

    byte_number = 0;
    for (byte_name = 0; byte_name <= 4; byte_name++)
        if (01<<byte_name IN required_bytes[state])
            {
            byte_number++;
            data_byte[byte_name] = (int) kade[byte_number-1] % 32;
            }

    *x = data_byte[3] * 128 + data_byte[4] * 4 + data_byte[1] % 4;
    *y = data_byte[0] * 128 + data_byte[2] * 4 + data_byte[1] / 4;
}


void gus_plot10_initt()

/*
 * gus_plot10_initt - PLOT 10 initialization routine
 */

{
    int errind, conid, wtype, dcunit, lx, ly;
    char *wstype;
    float cheight, rx, ry, x0, x1, y0, y1;
    
    float xmin, xmax, ymin, ymax;

    /* inquire GKS operating state */

    GQOPS (&opsta);

    if (opsta == GGKCL)
        {
        GOPKS (&errfil, &bufsiz);
        opsta = GGKOP;
	GSASF (flags);
        }

    if (opsta == GGKOP)
        {
        wstype = getenv ("GLI_WSTYPE");
	if (wstype)
            wtype = atoi (wstype);
        else
            wtype = GWSDEF;

	GOPWK (&wkid, &GCONID, &wtype);
        opsta = GWSOP;
        }

    if (opsta == GWSOP)
        {
	GACWK (&wkid);
        opsta = GWSAC;
        }

    if (opsta >= GWSAC)
        {
        GQLN (&errind, &ltype);
        GQMK (&errind, &mtype);
        GQCHH (&errind, &chh);
        GQTXAL (&errind, &halign, &valign);

        GQCNTN (&errind, &tnr);
        xformno = tnr;
        if (xformno == 0)
            {
            xformno = 1;
            GSELNT (&xformno);
            }

	GQWKC (&wkid, &errind, &conid, &wtype);
	GQDSP (&wtype, &errind, &dcunit, &rx, &ry, &lx, &ly);
	if (dcunit == GMETRE)
	    {
	    x0 = 0; x1 = 0.256;
	    y0 = 0; y1 = x1*3120/4096;
	    GSWKVP (&wkid, &x0, &x1, &y0, &y1);

	    y1 = y1/x1; x1 = 1;
	    GSWKWN (&wkid, &x0, &x1, &y0, &y1);
	    }

        GQNT (&xformno, &errind, wn, vp);
	xmin = 0;
	xmax = 4096;
	ymin = 0;
	ymax = 3120;
        GSWN (&xformno, &xmin, &xmax, &ymin, &ymax);
	xmin = 0;
	xmax = 1;
	ymin = 0;
	ymax = 3120./4096.;
        GSVP (&xformno, &xmin, &xmax, &ymin, &ymax);

        GSLN (&GLSOLI);
        GSMK (&GPOINT);
	cheight = height[csize-1] * 2. / 3.; 
        GSCHH (&cheight);
        GSTXAL (&GALEFT, &GABASE);
        }
}



void gus_plot10_finitt()

/*
 * gus_plot10_finitt - PLOT 10 termination routine
 */

{
    if (opsta >= GWSAC)
        {
        GUWK (&wkid, &GPOSTP);
        GSVP (&xformno, &vp[0], &vp[1], &vp[2], &vp[3]);
        GSWN (&xformno, &wn[0], &wn[1], &wn[2], &wn[3]);
        GSELNT (&tnr);
        GSTXAL (&halign, &valign);
        GSCHH (&chh);
        GSMK (&mtype);
        GSLN (&ltype);
        }
}



static
void dump_graphic_info(void)

/*
 * dump_graphic_info - dump graphic information
 */

{
    float x, y;

    switch (operating_mode) {

        case point_plot_mode :
          if (np > 0)
              {
              GPM (&np, px, py);
              np = 1;
              }
          break;

        case vector_mode :
          if (np > 1)
              GPL (&np, px, py);
          if (np > 0)
              {
              px[0] = px[np-1];
              py[0] = py[np-1];
              np = 1;
              }

        case alpha_mode :
          if (char_len > 0)
              {
              x = x_cursor;
              y = y_cursor;
              gks_text_s (&x, &y, &char_len, chars);
              char_len = 0;
              }
	break;
	}
}



static
void change_mode_to_native (void)

/*
 * change_mode_to_native - change to native mode
 */

{
    if (operating_mode != native_mode)
        {
        dump_graphic_info ();
        GUWK (&wkid, &GPOSTP);
        initialize_data_bytes ();
        operating_mode = native_mode;
        }
}



static
void set_alpha_cursor (void)

/*
 * set_alpha_cursor - position alpha cursor
 */

{
    if (np > 0)
        {
        x_cursor = (int) (px[np-1]);
        y_cursor = (int) (py[np-1]);
        }
}



static
void line_feed (void)

/*
 * line_feed - move one line down
 */

{
    if (y_cursor < height[csize-1])
        {
        x_cursor = (x_cursor + 2048) % 4096;
        y_cursor = 3120 - height[csize-1];
        x_margin = (x_margin + 2048) % 4096;
        }
    else
        y_cursor = y_cursor - height[csize-1];
}



static
void vertical_tab (void)

/*
 * vertical_tab - move one line up
 */

{
    if (y_cursor > 3120 - height[csize-1])
        {
        x_cursor = (x_cursor + 2048) % 4096;
        y_cursor = 0;
        x_margin = (x_margin + 2048) % 4096;
        }
    else
        y_cursor = y_cursor + height[csize-1];
}



static
void backspace (void)

/*
 * backspace - move one space left
 */

{
    if (x_cursor < width[csize-1])
        {
        x_cursor = x_margin - width[csize-1];
        vertical_tab ();
        }
    else
        x_cursor = x_cursor - width[csize-1];
}



static
void horizontal_tab (void)

/*
 * horizontal_tab - move one space right
 */

{
    if (x_cursor > 3120 - width[csize-1])
        {
        x_cursor = x_margin;
        line_feed ();
        }
    else
        x_cursor = x_cursor + width[csize-1];
}



static
void carriage_return (void)

/*
 * carriage_return - move to left margin
 */

{
    x_cursor = x_margin;
    change_mode_to_native ();
}



static
void form_feed (void)

/*
 * form_feed - move to home position
 */

{
    x_margin = 0;
    x_cursor = 0;
    y_cursor = 3120 - height[csize-1];
    change_mode_to_native ();
}



static
void clear_open_ws (void)

/*
 * clear_open_ws - clear all open workstations
 */

{
    int state, i, n, errind, ol, wkid;

    GQOPS (&state);
    if (state == GSGOP)
        GCLSG ();

    if (state >= GWSOP)
        {
        n = 1;
        GQOPWK (&n, &errind, &ol, &wkid);
        for (i = ol; i >= 1; i--)
            {
            n = i;
            GQOPWK (&n, &errind, &ol, &wkid);
            GCLRWK (&wkid, &GALWAY);
            }
        }
}



static int write_index, esc_len = 0;
static char *kade, ch;

static
void put_character (void)

/*
 * put_character - put character into kade array
 */

{
    write_index++;
    kade[write_index-1] = ch;
}  



static
void put_escape (void)

/*
 * put_escape - put escape sequence into kade array
 */

{
    int i;

    for (i = 0; i < esc_len; i++)
        {
        write_index++;
        kade[write_index-1] = esc_seq[i];
        }
}



void gus_plot10_adeout (int *nchar, char *string, int *clear)

/*
 * gus_plot10_adeout - PLOT 10 ASCII decimal equivalent output routine
 */

{
    int read_index;
    float x, y;
    int ix, iy;
    int	status, tnr, ltype;
    unsigned int tag_bits;
    int lcdnr;
    float cheight, lwidth;

    kade = string;

    if (opsta == GGKCL)
	gus_plot10_initt ();

    write_index = 0;

    for (read_index = 0; read_index < *nchar; read_index++)
	{
	ch = kade[read_index];

	if (escape_start)
	    {

	    switch (ch) {

		case enq:
		case etb:
		    break; /* not yet supported */

		case ff:
		    if (*clear) clear_open_ws ();
		    form_feed ();
		    break;

		case sub:
		    lcdnr = 1;
		    GRQLC (&wkid, &lcdnr, &status, &tnr, &x, &y);

		    if (status == GOK)
			{
			ix = (int) (x * 1024);
			iy = (int) (y * 779);

			outp_byte_count = 6;
			outp_byte[0] = (int) space;
			outp_byte[1] = 32 + ix / 32;
			outp_byte[2] = 32 + ix % 32;
			outp_byte[3] = 32 + iy / 32;
			outp_byte[4] = 32 + iy % 32;
			outp_byte[5] = (int) cr;
			}
		    else
			{
			outp_byte_count = 1;
			outp_byte[0] = ctrl_z;
			}
		    break;

		case '8':
		case '9':
		case ':':
		case ';':
		    csize = (int) ch - (int) '8' + 1;
		    if (csize < 1)
			csize = 1;
		    else if (csize > 4)
			csize = 4;
		    cheight = height[csize-1] * 2. / 3.;
		    GSCHH (&cheight);

		    if (ch == '8') /* <ESC>8 is also a valid escape sequence */
			{
			esc_seq[0] = esc; 
			esc_seq[1] = ch;
			esc_len = 2;
			put_escape ();
			}
		    break;

		case '`':
		case 'a':
		case 'b':
		case 'c': 
		case 'd':
		case 'e':
		case 'f':
		case 'g':
		    lwidth = 1.;
		    GSLWSC (&lwidth);
		    ltype = linetype[(int) ch - (int) '`'];
		    GSLN (&ltype);
		    break;

		case 'h':
		case 'i':
		case 'j':
		case 'k': 
		case 'l':
		case 'm':
		case 'n':
		case 'o':
		    lwidth = 2.;
		    GSLWSC (&lwidth);
		    ltype = linetype[(int) ch - (int) '`'];
		    GSLN (&ltype);
		    break;

		case 'p':
		    lwidth = 1.;
		    ltype = 0;
		    GSLWSC (&lwidth);
		    GSLN (&ltype);
		    break;

		case '[':
		case 'O':
		    esc_seq[0] = esc; 
		    esc_seq[1] = ch;
		    esc_len = 2;
		    ansi_escape = TRUE;
		    break;

		default:
		    if (((int) ch >= 32) && ((int) ch <= 47))
			{
			esc_seq[0] = esc; 
			esc_seq[1] = ch;
			esc_len = 2;
			escape = TRUE;
			}

		    else if (((int) ch >= 48) && ((int) ch <= 126))
			{
			esc_seq[0] = esc; 
			esc_seq[1] = ch;
			esc_len = 2;
			put_escape ();
			change_mode_to_native ();
			}
		    break;
		}

	    escape_start = FALSE;	
	    }

	else if (escape)
	    {
	    esc_seq[esc_len++] = ch;

	    if (((int) ch >= 48) && ((int) ch <= 126))
		{
		put_escape ();
		escape = FALSE;
		change_mode_to_native ();
		}
	    }

	else if (ansi_escape)
	    {
	    esc_seq[esc_len++] = ch;

	    if (((int) ch >= 64) && ((int) ch <= 126))
		{
		put_escape ();
		ansi_escape = FALSE;
		change_mode_to_native ();
		}
	    }

	else if ((ch == bs) || (ch == ht) || (ch == lf) || (ch == vt) || 
	    (ch == cr))
	    {
	    if (operating_mode != native_mode)
		{
		dump_graphic_info ();

		switch (ch) {

		    case bs: backspace (); break;
		    case ht: horizontal_tab (); break;
		    case lf: line_feed (); break;
		    case vt: vertical_tab (); break;
		    case cr: carriage_return (); break;
		    }
		}

	    if (operating_mode == native_mode)
		put_character ();
	    }

	else 
	    switch (ch) {

		case bel:
		    put_character ();
		    break; 

		case eot:
		case so:
		case si: 
		    break; /* not yet supported */

		case nul:
		case syn:
		    break; /* ignored */

		case esc:
		    escape_start = TRUE;
		    break;

		case csi:
		case ss3:
		    ansi_escape = TRUE;
		    break;
		    
		case can:
		    change_mode_to_native ();
		    break;

		case fs:
		    dump_graphic_info ();
		    initialize_data_bytes ();
		    operating_mode = point_plot_mode;
		    break;

		case gs:
		    dump_graphic_info ();
		    initialize_data_bytes ();
		    operating_mode = vector_mode;
		    break;

		case us:
		    dump_graphic_info ();

		    if (operating_mode == vector_mode)
			set_alpha_cursor ();

		    initialize_data_bytes ();
		    operating_mode = alpha_mode;
		    break;

		default:

		    switch (operating_mode) {

			case vector_mode:
			case point_plot_mode:
			    tag_bits = (unsigned int) ch / 32;

			    if ((tag_bits >= 1) && (tag_bits <= 3) &&
				(inp_byte_count < 5))
				{
				inp_byte[inp_byte_count] = (int) ch;
				inp_byte_count++;

				if (tag_bits == 2)
				    {
				    decode_data_bytes (inp_byte_count, inp_byte,
					&ix, &iy);
				    inp_byte_count = 0;

				    if (np == buf_size)
					dump_graphic_info ();

				    np++;
				    px[np-1] = ix; 
				    py[np-1] = iy;
				    }
				}
			    else
				change_mode_to_native ();
			    break;

			case alpha_mode:
			    if (char_len < buf_size)
				chars[char_len++] = ch;
			    break;

			case native_mode:
			    put_character ();
			    break;
			}	
                    break;
		}
	}

    *nchar = write_index;
    }


void gus_plot10_adein (int *nchar, char *kade)

/*
 * gus_plot10_adein - PLOT 10 ASCII decimal equivalent input routine
 */

{
    int count;

    *nchar = outp_byte_count;
    outp_byte_count = 0;

    for (count = 0; count < *nchar; count++)
        kade[count] = (char) outp_byte[count];
}
