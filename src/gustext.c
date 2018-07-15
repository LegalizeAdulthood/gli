/*
 *
 * FACILITY:
 *
 *	Graphic Utility System (GUS)
 *
 * ABSTRACT:
 *
 *	This module contains a powerful text formatting routine based on
 *	an implementation of the Graphical Kernel System (GKS) Version 7.4
 *
 * AUTHOR:
 *
 *	Jochen Werner	2-Mai-1991
 *
 * VERSION:
 *
 *	V1.4
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <string.h>

#if !defined(VMS) && !defined(MSDOS) && !defined(_WIN32)
#include <unistd.h>
#endif

#if defined (cray) || defined (__SVR4) || defined (MSDOS) || defined(_WIN32)
#include <fcntl.h>
#else
#include <sys/types.h>
#include <sys/file.h>
#endif

#include "terminal.h"
#include "gksdefs.h"
#include "gus.h"


/* Constant definitions */

#define blank	' '
#define NIL	0
#define TRUE	1
#define FALSE	0
#ifndef PI
#define PI      3.141592654
#endif

#define ch_xp               1.0         /* nominal character expansion factor */
#define fact_r_degree	    0.28	/* factor for the height of the degree 
					   of a root */
#define fact_index	    0.75	/* factor for the index */
#define fact_sum	    0.3		/* factor for sub and superscript of
					   a mathematical sum */
#define fact_spec	    0.5		/* factor for sub and superscript of
					   a special expression */
#define fact_exp	    0.75        /* factor for the exponent */
#define delta		    0.1		/* delta between charaters (fraction) */
#define fact_math           1.1175      /* factor for special math chars */
#define root_size           4.142857143 /* factor for roots */
#define math_f_size	    4.554	/* height of math-char with full 
					   height */
#define math_n_size	    1.52381	/* height of math-char with normal
					   height */
#define width_fact          60          /* factor for the linewidth */

#define f_left		    0		/* index for the font - buffer */
#define f_rght		    1		
#define f_size		    2		
#define f_bot 		    3
#define f_base		    4
#define f_cap 		    5
#define f_top 		    6
#define f_len		    7


/* Macro definitions */

#define odd(status)	    ((status) &01)
#define present(status)     ((status) != NIL)
#ifndef max
#define max(a, b)	    ((a) > (b) ? (a) : (b))
#endif
#define s_size(height)	    ((height) * math_n_size)
#define char_hgt(height)    ((height) / math_n_size)
#define c_size(act_chr_hgt) (character_size * (act_chr_hgt))

#if defined (__sgi) || defined (__GNUC__)
#define u_char char
#else
#define u_char unsigned char
#endif

/* Type definitions */
 
typedef enum {normal, greek, math} font_type;

typedef char buffer_rec[256];

typedef enum {fract, power, indx, s_expr, par, root, sum, operand, string} 
    node_types;	

typedef struct node_type {
    node_types		 type;		    /* type of the node */
    struct node_type	 *next;		    /* pointer to next node */
    union {

	/* fraction */

	struct {
	    float	         numer_hgt; /* height of the numerator */
	    float	         numer_lgt; /* length of the numerator */
	    float	         numer_dy;  /* dy of the numerator */
	    struct node_type	 *numer;    /* pointer to the numerator */
	    float	         denom_hgt; /* height of the denominator */
	    float	         denom_lgt; /* length of the denominator */ 
	    float	         denom_dy;  /* dy of the denominartor */ 
	    struct node_type	 *denom;    /* pointer the denominator */
        } fract;

	/* power */

	struct {
	    float	         fact_hgt;  /* height of the factor */
	    float	         fact_lgt;  /* length of the factor */
	    struct node_type	 *fact;	    /* pointer to the factor */
	    float	         exp_hgt;   /* height of the exponent */
	    float	         exp_lgt;   /* length of the exponent */
	    float	         exp_dy;    /* dy of the exponent */
	    struct node_type	 *exp;      /* pointer to the exponent */
        } power;

	/* index */

	struct {
	    float	         arg_hgt;   /* height of the argument */
	    float	         arg_lgt;   /* length of the argument */
	    struct node_type	 *arg;	    /* pointer to the argument */
	    float	         ind_hgt;   /* height of the index */
	    float	         ind_lgt;   /* length of the index */
	    float	         ind_dy;    /* dy of the index */
	    struct node_type	 *ind;      /* pointer to the index */
        } index;

	/* special expression */

	struct {
	    float	         index_hgt; /* height of the index */
	    float	         index_lgt; /* length of the index */
	    float	         index_dy;  /* dy of the index */
	    struct node_type	 *index;    /* pointer to the index */ 
	    float	         exp_hgt;   /* height of the exponent */
	    float	         exp_lgt;   /* length of the exponent */
	    float	         exp_dy;    /* dy of the exponent */
	    struct node_type	 *exp;      /* pointer to the exponent */
	    float	         super_hgt; /* height of the superscript */
	    float	         super_lgt; /* length of the superscript */
	    float	         super_dy;  /* dy of the superscript */
	    struct node_type	 *super;    /* pointer to the superscript */
	    float	         sub_hgt;   /* height of the subscript */
	    float	         sub_lgt;   /* length of the subscript */
	    float	         sub_dy;    /* dy of the subscript */
	    struct node_type	 *sub;	    /* pointer to the subscript */
	    float	         arg_hgt;   /* height of the argument */
	    float	         arg_lgt;   /* length of the argument */
	    float	         arg_dy;    /* dy of the argument */
	    struct node_type	 *arg;	    /* pointer to the argument */
        } s_expr;

	/* mathematical sum */

	struct {
	    float	         super_hgt; /* height of the superscript */
	    float	         super_lgt; /* length of the superscript */
	    float	         super_dy;  /* dy of the superscript */
	    struct node_type	 *super;    /* pointer to the superscript */
	    float	         sub_hgt;   /* height of the subscript */
	    float	         sub_lgt;   /* length of the subscript */
	    float	         sub_dy;    /* dy of the subscript */
	    struct node_type	 *sub;	    /* pointer to the subscript */
	    float	         arg_hgt;   /* height of the argument */
	    float	         arg_lgt;   /* length of the argument */
	    float	         arg_dy;    /* dy of the argument */
	    struct node_type	 *arg;	    /* pointer to the argument */
	    u_char		 sum;	    /* sum character */
	    float		 sum_hgt;   /* char height of the sum char */
	    float		 sum_lgt;   /* length of the sum char */
	    float		 chr_hgt;   /* char height of sub and super */
        } sum;

	/* root */

	struct {
	    struct node_type	 *degree;   /* pointer to the degree */
	    float                deg_hgt;   /* char height of the degree */
	    float	         arg_hgt;   /* height of the argument */
	    float	         arg_lgt;   /* length of the argument */
	    float	         arg_dy;    /* dy of the argument */
	    struct node_type	 *arg;	    /* pointer to the argument */
	    u_char		 root;	    /* root character */
	    float		 root_hgt;  /* char height of the root char */
	    float		 root_lgt;  /* length of the root char */
        } root;

	/* paranthesis */

	struct {
	    float	         arg_hgt;   /* height of the argument */
	    float	         arg_lgt;   /* length of the argument */
	    float	         arg_dy;    /* dy of the argument */
	    struct node_type	 *arg;	    /* pointer to the argument */
	    u_char		 par;	    /* paranthesis character */
	    float		 par_hgt;   /* char height of the par char */
	    float		 par_lgt;   /* length of the par char */
        } par;

	/* operand */

    	struct {
	    font_type chr_type;		    /* type of the font */
	    u_char    chr;		    /* character */
	    float     height;		    /* height of the character */    
	    float     length;		    /* length of the character */    
        } oper;

	/* string */

    	struct {
	    u_char    string[80];	    /* string */
	    float     length;		    /* length of the string */    
        } string;

    } e;

} node_type_struct;


/* Pattern strings */

static u_char greek_letters[] = "alpha beta chi delta epsilon phi gamma eta\
 iota kappa lambda mu nu omicron pi theta rho sigma tau upsilon psi omega xi\
 zeta varphi vartheta";

static u_char math_symbols[] = "emptyset degree epsilon pm mp cross mult div\
 neq equiv leq geq propto subset cap supset cup in right_arrow up_arrow\
 left_arrow down_arrow partial nabla surd lwr rwr infty exists lpar rpar\
 lbracket rbracket lbrace rbrace";

static u_char spec_operators[] = "alpha beta chi delta epsilon phi gamma eta\
 iota kappa lambda mu nu omicron pi theta rho sigma tau upsilon psi omega xi\
 zeta varphi vartheta emptyset degree epsilon pm mp cross mult div neq\
 equiv leq geq propto subset cap supset cup in right_arrow up_arrow left_arrow\
 down_arrow partial nabla surd lwr rwr infty exists lpar rpar lbracket rbracket\
 lbrace rbrace lt gt bcksl * / _ ^ ae oe ue ss";

static u_char math_operators[] = "root integral c_integral product sum cap cup";

static u_char pars[] = "par bracket brace abs norm";

static u_char special_chars[] = "lt gt bcksl * / _ ^";

static u_char umlaute[] = "ae oe ue ss";


/* Lookup tables */

static int font_array[22][3] = 
	{ { -1, 4,20}, { -2, 4,20}, { -3, 4,20}, { -4, 4,20}, { -5, 4,20},
	  { -6, 7,20}, { -7, 7,20}, { -8, 7,20}, { -9, 7,20}, {-10, 7,20},
	  {-11,10,20}, {-12,10,20}, {-13,10,20}, {-14, 4,20}, {-15, 7,20},
	  {-16, 4,20}, {-17, 7,20}, {-18, 7,20}, {-19, 7,20}, {-20, 4,20},
	  {-21, 4,20}, {-22, 4,20} };

static int greek_letter_code[26] =
	{  97, 98, 99, 100, 101, 102, 103, 104, 105, 107, 108, 109, 110,
	  111, 112, 113, 114, 115, 116, 117, 121, 119, 120, 122, 106, 74 };

static int uppercase_greek_letter_code[26] =
	{  65, 66, 67, 68, 69, 70, 71, 72, 73, 75, 76, 77, 78,
	   79, 80, 81, 82, 83, 84, 85, 89, 87, 88, 90, 106, 74 };

static int math_symbol_code[35] =
	{  76, 35, 74, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 
	   88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 70, 71,
	   102, 103, 68, 69, 116, 117, 118, 119 };

static int math_operator_code[7] =
	{  114, 115, 101, 104, 105, 89, 91 };

static int par_code[] =
	{  106, 108, 110, 1, 3};

static int special_char_code[7] =
	{ 60, 62, 92, 42, 47, 95, 94 };

static int uppercase_umlaute_code[4] =
	{  196, 214, 220, 223 };

static int umlaute_code[4] =
	{  228, 246, 252, 223 };

static int font_map[22] =
	{ 1, 18, 1, 6, 12, 3, 8, 11, 4, 7, 10, 2, 13, 14, 5, 9, 15, 16, 17, 20,
	  21, 19};

static int german[] = {196, 214, 220, 228, 246, 252, 223, 171, 187, 183, 169};

static u_char ansi[] = {'A', 'O', 'U', 'a', 'o', 'u', 'b', '<', '>', '.', '@'};

static u_char space[2] = " ";

/* Global variables */

static u_char *str, buf[256];
static int ret_stat;
static float expansion, spacing;
static int text_prec, text_font;
static float character_size;
static int wkid;
static float cos_f, sin_f, ux, uy, up_x, up_y;
static float text_slant, cos_s, sin_s;



static void look_up (int font, u_char ch, char *buffer)

/*
 *   read the data of character of font in buffer
 */

{
    char *path, fontdb[80];
    int req_font, umlaut, sharp_s, i, record_number;
    static int font_file, font_file_open;
    static int current_font;
    static buffer_rec font_cache[95];

    static int map[] = { 4,4,4,4,4,7,7,7,10,10,10,7,7,7,4,4,7,7,7,4,4,4 };

    if (!font_file_open)
        {
	path = (char *) getenv ("GLI_HOME");
	if (path)
            {
	    strcpy (fontdb, path);
#ifndef VMS
#ifdef _WIN32
	    strcat (fontdb, "\\");
#else
	    strcat (fontdb, "/");
#endif
#endif
	    strcat (fontdb, "gksfont.dat");
            }
	else
#ifdef VMS
            strcpy (fontdb, "sys$sysdevice:[gli]gksfont.dat");
#else
#ifdef _WIN32
            strcpy (fontdb, "c:\\gli\\gksfont.dat");
#else
            strcpy (fontdb, "/usr/local/gli/gksfont.dat");
#endif
#endif

        font_file = open (fontdb, O_RDONLY, 0);

	if (font_file == -1)
	    {
	    tt_fprintf (stderr, "GUS (gustext): cannot open font database\n");
	    exit (-1);
	    }
	else
	    font_file_open = TRUE;
        }

    /* check for German character with umlaut */

    umlaut = sharp_s = FALSE;
    if ((ch >= 127))
        for (i = 0; i <= 10; i++)
            if ((ch == german[i]))
                {
                ch = ansi[i];
                if (i < 6)
                    umlaut = TRUE;
                else if (i == 6)
                    sharp_s = TRUE;
                }

    req_font = abs(font);
    if (req_font > 22)
	req_font = 1;

    if (sharp_s)
        req_font = map[req_font-1];

    if ((font > 0))
        {
        record_number = ((font_map[req_font - 1] - 1) * 95 + ch - ' ') * 256;
	lseek (font_file, record_number, 0);
	read (font_file, buffer, 256);
        }
    else
        {
        if ((font != current_font))
            {
            record_number = (font_map[req_font - 1] - 1) * 95 * 256;
	    lseek (font_file, record_number, 0);
            for (i = 32; i <= 126; i++)
                {
		read (font_file, font_cache[i-32], 256);
                }
            current_font = font;
            }
	for (i = 0; i < 256; i++)
	    buffer[i] = font_cache[ch-32][i];
        }

    /* append umlaut */

    if (umlaut && (buffer[7] < 120-20))
        buffer[7] = buffer[7] + 10;
}



static int str_index (u_char *object, u_char *pattern)

/*
 *  str_index - locate given pattern string within an object string
 */

{
    int i, j, k;

    for (i=0; object[i] != '\0'; i++)
	{
	for (j=i, k=0; pattern[k] != '\0' && object[j] == pattern[k]; j++, k++)
	    ;
	if (pattern[k] == '\0')
	    return i;
	}

    return (-1);
}



static int verb_index (u_char *verb_list, u_char *verb)

/*
 *   return the verb index
 */

{
    int i, j, n;
    u_char *verb_list_help, *verb_help;

    verb_list_help = (u_char *) malloc(strlen((char *) verb_list) + 3);
    verb_help = (u_char *) malloc(strlen((char *) verb) + 3);

    /* copy verb_list to verb_list_help by adding a blank at the beginning and
       the end and transforming it to lower case */

    j = strlen((char *) verb_list);
    verb_list_help [0] = ' ';
    for (i = 0; i < j; i++)
	if (isupper(verb_list[i]))
	    verb_list_help[i+1] = tolower(verb_list[i]);
	else
	    verb_list_help[i+1] = verb_list[i];

    verb_list_help[j+1] = ' ';
    verb_list_help[j+2] = '\0';

    /* copy verb to verb_help by adding a blank at the beginning and the end */

    j = strlen((char *) verb);
    verb_help [0] = ' ';
    for (i = 0; i < j; i++)
	if (isupper(verb[i]))
	    verb_help[i+1] = tolower(verb[i]);
	else
	    verb_help[i+1] = verb[i];

    verb_help[j+1] = ' ';
    verb_help[j+2] = '\0';

    i = str_index(verb_list_help, verb_help);

    free (verb_list_help);
    free (verb_help);

    /* compute the verb index by calculating the blanks */

    if (i >= 0)
        {
        n = 1;

        for (j = 0; j <= i-1; j++)
            if (verb_list[j] == blank)
                n++;

        return (n);
        }
    else

        return (0);
}


static int font_nr (int font, font_type type)

/*
 *  Returns the fontnumber of the specified font.
 */

{
    int req_font;

    req_font = abs(font);
    if (text_prec != GSTRKP) {
	if (font < 1 || font > 32)
	    req_font = 1;
	}
    else {
	if (req_font < 1 || req_font > 22)
	    req_font = 1;
	}

    if (font < 0)
	return (font_array[req_font - 1][(int) type]);
    else
	switch (type) {

	    case normal: return (req_font);

	    case greek: return (7);

	    case math: return (20);

	    default: return (req_font);
	    }
} 

  
static void char_size (float act_chr_hgt, u_char chr, font_type type,
    float *height, float *length)  

/*
 *  Returns the length and the height of a character 
 */

{
    int len = 1;
    int errind, font;
    float cpx, cpy;
    float x = 0, y = 0;
    float tx[4], ty[4];
    buffer_rec buffer;
    float width, size, chr_hgt;

    chr_hgt = act_chr_hgt;
    font = font_nr (text_font, type);

    if ((text_prec == GSTRKP) || (type == math))
	{
	
	/* text text_prec stroke */

	look_up (font, chr, buffer);

	width = buffer[f_rght] - buffer[f_left];
	size = buffer[f_size];

	*length = expansion * chr_hgt * ch_xp * (width / size + spacing);
	if (chr == blank)
	    *length = 0.5 * *length;
	*height = s_size (chr_hgt);
	}
    else
	{
	
	/* text precison char */

	GSCHH (&chr_hgt);

	GSTXFP (&font, &text_prec);
	gks_inq_text_extent_s (&wkid, &x, &y, &len, (char *)&chr, &errind,
	    &cpx, &cpy, tx, ty);

	*height = ty[2] - ty[0];
	*length = tx[2] - tx[0];
	}
}




static void string_size (float act_chr_hgt, u_char *string, float *height,
    float *length)  

/*
 *  Returns the length and the height of a string
 */

{
    int len;
    int errind, font;
    float cpx, cpy;
    float x = 0, y = 0;
    float tx[4], ty[4];
    buffer_rec buffer;
    float width, size, chr_hgt, lgth;

    chr_hgt = act_chr_hgt;
    font = font_nr (text_font, normal);

    if (text_prec == GSTRKP)
	{
	
	/* text text_prec stroke */

	*length = 0;
	*height = s_size (chr_hgt);

	while (*string != '\0')
	    {
	    look_up (font, *string, buffer);

	    width = buffer[f_rght] - buffer[f_left];
	    size = buffer[f_size];

	    lgth = expansion * chr_hgt * ch_xp * (width / size + spacing);
	    if (*string == blank)
		lgth = 0.5 * lgth;
	    *length = *length + lgth;
            string++;
	    }
	}
    else
	{
	
	/* text precison char */

	GSCHH (&chr_hgt);

	GSTXFP (&font, &text_prec);
	len = strlen((char *) string);
	gks_inq_text_extent_s (&wkid, &x, &y, &len, (char *)string, &errind,
	    &cpx, &cpy, tx, ty);

	*height = ty[2] - ty[0]; 
	*length = tx[2] - tx[0];
	}
}




static void math_size (float height, u_char chr, float *chr_hgt, float *length) 

/*
 *  Returns the length and the height of a character 
 */

{
    int font;
    buffer_rec buffer;
    float width, size;

    switch ((int) chr) {
	case 1:
	case 2:

	    /* abs */

	    font = -1;
            chr = '\174';

	    *chr_hgt = height / math_n_size;
    
	    look_up (font, chr, buffer);

	    width = buffer[f_rght] - buffer[f_left];
	    size = buffer[f_size];

	    *length = expansion * *chr_hgt * ch_xp * (width / size + spacing);
	    break;

	case 3:
	case 4:

	    /* norm */

	    font = -1;
            chr = '\174';

	    *chr_hgt = height / math_n_size;
    
	    look_up (font, chr, buffer);

	    width = buffer[f_rght] - buffer[f_left];
	    size = buffer[f_size];

	    *length = 2. * (expansion * *chr_hgt * ch_xp * (width / size + 
		spacing));
	    break;

	case 115:
	case 106:
	case 107:
	case 108:
	case 109:
	case 110:
	case 111:

	    /* math character with full height */

	    font = font_nr (text_font, math);

	    *chr_hgt = height / math_f_size;

	    *chr_hgt = fact_math * *chr_hgt;

	    look_up (font, chr, buffer);

	    width = buffer[f_rght] - buffer[f_left];
	    size = buffer[f_size];

	    *length = expansion * *chr_hgt * ch_xp * (width / size + spacing);
	    break;

	case 114:

	    /* root */

	    font = font_nr (text_font, math);

	    *chr_hgt = height / root_size;

	    look_up (font, chr, buffer);

	    width = buffer[f_rght] - buffer[f_left];
	    size = buffer[f_size];

	    *length = expansion * *chr_hgt * ch_xp * (width / size + spacing);
	    break;

	default:

	    /* math character with normal height */

	    font = font_nr (text_font, math);

	    *chr_hgt = height / math_n_size;
    
	    look_up (font, chr, buffer);

	    width = buffer[f_rght] - buffer[f_left];
	    size = buffer[f_size];

	    *length = expansion * *chr_hgt * ch_xp * (width / size + spacing);
	    break;
	}
}




static struct node_type *read_s_exp (float, float *, float *, float *);
static struct node_type *read_operand (float, float *, float *, float *);
static struct node_type *read_power (float, float *, float *, float *);
static struct node_type *read_fraction (float, float *, float *, float *);


static struct node_type *read_expression (float act_chr_hgt, float *height,
    float *length, float *dy)

/*
 *  FUNCTIONAL DESCRIPTION:
 *	
 *	Reads an expression into a tree and returns the pointer to the 
 *	root of the tree.
 */

{

    struct node_type *p, *node;
    float fract_hgt, fract_lgt, fract_dy;

    *dy = 0;
    *height = 0;
    *length = 0;

    node = read_fraction (act_chr_hgt, height, length, dy);
    p = node;

    while (odd(ret_stat) && p != NIL)
	{
	while (p->next != NIL)
	    p = p->next;

	p->next = read_fraction (act_chr_hgt, &fract_hgt, &fract_lgt, 
	    &fract_dy);

	*dy = max(*dy, fract_dy);
	*height = max(*height, fract_hgt);
	*length = *length + fract_lgt;

	p = p->next;
	}
    
    return (node);
}



static struct node_type *read_fraction (float act_chr_hgt, float *height,
    float *length, float *dy)

/*
 *  FUNCTIONAL DESCRIPTION:
 *	
 *	Reads a fraction into a tree and returns the pointer to the 
 *	root of the tree.
 */

{

    struct node_type *p, *node;
    float denom_hgt, denom_lgt, denom_dy;

    p = read_power (act_chr_hgt, height, length, dy);

    /* while current char is a slash read fraction */

    if (*str == '/')
	{
	while (*str == '/')
	    {

	    /* create and initialize a new node of type fraction */

	    node = (struct node_type *) malloc (sizeof (struct node_type));
	    node->type = fract;
	    node->next = NIL;
	    node->e.fract.numer = p;
	    node->e.fract.numer_hgt = *height + *dy;
	    node->e.fract.numer_lgt = *length;
	    node->e.fract.numer_dy = *dy;

	    str++;

	    if (p == NIL)
		{
		if (odd(ret_stat))
		    ret_stat = gus__synexpr;
		}
	    else
		{

		/* read denominator */
		
		p = read_power (act_chr_hgt, &denom_hgt, &denom_lgt, &denom_dy);
		node->e.fract.denom = p;

		if (p == NIL)
		    {
		    if (odd(ret_stat))

			/* expression expected */

			ret_stat = gus__synexpr;
		    }
		else
		    {
		    node->e.fract.denom_hgt = denom_hgt + denom_dy;
		    node->e.fract.denom_lgt = denom_lgt;
		    node->e.fract.denom_dy = denom_dy;

		    *height = *height + 2. * delta * c_size (act_chr_hgt) + 
			denom_hgt + denom_dy;
		    *dy = -0.5 * c_size (act_chr_hgt) + delta * 
			c_size (act_chr_hgt) + denom_hgt + denom_dy;
		    *length = max(*length, denom_lgt) + 4. * delta * 
			c_size (act_chr_hgt);

		    p = node;
		    }
		}
	    }

	*height = *height - *dy;
	}
    
    return (p);
}



static struct node_type *read_power (float act_chr_hgt, float *height,
    float *length, float *dy)

/*
 *  FUNCTIONAL DESCRIPTION:
 *	
 *	Reads a power or an argument with index into a tree and returns the 
 *	pointer to the root of the tree.
 */

{

    struct node_type *p, *node;
    float exp_hgt, exp_lgt, exp_dy;
    float ind_hgt, ind_lgt, ind_dy;

    p = read_operand (act_chr_hgt, height, length, dy);

    /* if current char is '**' or '^' or '_' read power */

    if ((str_index (str, (u_char *)"**") == 0) || (*str == '^') ||
	(*str == '_'))
	{

	if (p == NIL)
	    {
	    if (odd(ret_stat))

		ret_stat = gus__synexpr;
	    }
	else
	    if (*str == '_')
		{

		/* create and intialize a new node of type index */

		str++;

		node = (struct node_type *) malloc (sizeof (struct node_type));
		node->type = indx;
		node->next = NIL;
		node->e.index.arg = p;
		node->e.index.arg_hgt = *height + *dy;
		node->e.index.arg_lgt = *length + delta * c_size (act_chr_hgt);

		/* read index */
		
		p = read_power (fact_index * act_chr_hgt, &ind_hgt, &ind_lgt, 
		    &ind_dy);
		node->e.index.ind = p;

		if (p == NIL)
		    {
		    if (odd(ret_stat))
			ret_stat = gus__synexpr;
		    }

		else
		    {
		    node->e.index.ind_hgt = ind_hgt + ind_dy;
		    node->e.index.ind_lgt = ind_lgt;
		    node->e.index.ind_dy = ind_dy;

		    *height = node->e.index.arg_hgt;
		    *dy = node->e.index.ind_hgt - (1. - fact_index) * 
			c_size (act_chr_hgt);
		    *length = node->e.index.arg_lgt + ind_lgt;
		    }

		p = node;
		}
	    else
		{

		/* create and intialize a new node of type power */

		if (*str == '^')
		    str++;
		else
		    str += 2;

		node = (struct node_type *) malloc (sizeof (struct node_type));
		node->type = power;
		node->next = NIL;
		node->e.power.fact = p;
		node->e.power.fact_hgt = *height + *dy;
		node->e.power.fact_lgt = *length + delta * c_size (act_chr_hgt);

		/* read exponent */
		
		p = read_power (fact_exp * act_chr_hgt, &exp_hgt, &exp_lgt, 
		    &exp_dy);
		node->e.power.exp = p;

		if (p == NIL)
		    {
		    if (odd(ret_stat))
			ret_stat = gus__synexpr;
		    }

		else
		    {
		    node->e.power.exp = p;
		    node->e.power.exp_hgt = exp_hgt + exp_dy;
		    node->e.power.exp_lgt = exp_lgt;
		    node->e.power.exp_dy = exp_dy;

		    *height = node->e.power.fact_hgt - (1. - fact_exp) * 
			c_size (act_chr_hgt) + node->e.power.exp_hgt - *dy;
		    *length = node->e.power.fact_lgt + exp_lgt;
		    }

		p = node;
		}

	}

    return (p);
}




static struct node_type *read_operand (float act_chr_hgt, float *height,
    float *length, float *dy)

/*
 *  FUNCTIONAL DESCRIPTION:
 *	
 *	Reads an operand into a tree and returns the pointer to the 
 *	root of the tree.
 */

{

    struct node_type *node;
    int	i;

    if (*str == '{')
	{
	str++;

	/* read expression in paranthesis */
	
	node = read_expression (act_chr_hgt, height, length, dy);

	if (*str != '}')
	    {
	    if (odd(ret_stat))
		ret_stat = gus__synrbrace;
	    }
	else
	    str++;
	}

    else
	if (*str == '<')
	    {
	    str++;

	    /* read special expression */

	    node = read_s_exp (act_chr_hgt, height, length, dy);

	    if (*str != '>')
		{
		if (odd(ret_stat))
		    ret_stat = gus__syngreater;
		}
	    else
		str++;
	    } 	    
	else

	    /* operand is a normal character */

	    if (*str != '\0' && *str != '>' && *str != '}' && *str != '\\' &&
		*str != '/' && *str != '^' && *str != '_' &&
		(str_index(str, (u_char *)"**") != 0))
		
		if (isalnum(*str & 0177) || *str == '.')
		    {
		    node = (struct node_type *) malloc (
			sizeof (struct node_type));
		    node->type = string;
		    node->next = NIL;
		    i = 0;

		    do
			{
			node->e.string.string[i] = *str;
			i++;
			str++;
			}
		    while (isalnum(*str & 0177) || *str == '.');

		    node->e.string.string[i] = '\0';
		    string_size (act_chr_hgt, node->e.string.string, height, 
			length);
		    node->e.string.length = *length;
		    *dy = 0;
		    }
		else
		    {
		    node = (struct node_type *) malloc (
			sizeof (struct node_type));
		    node->type = operand;
		    node->next = NIL;
		    node->e.oper.chr_type = normal;
		    node->e.oper.chr = *str;
		    char_size (act_chr_hgt, *str, normal, height, length);
		    node->e.oper.height = *height;
		    node->e.oper.length = *length;
		    *dy = 0;
		    
		    str++;
		    }

	    else
		{
		node = NIL;
		*height = 0;
		*length = 0;
		*dy = 0;
		}

    return (node);
}



static struct node_type *read_m_sum (int key_index, float act_chr_hgt,
    float *height, float *length, float *dy)

/*
 *  FUNCTIONAL DESCRIPTION:
 *	
 *	Reads a mathematical sum.
 */

{
    struct node_type *node;

    /* test if keyword is root */

    if (key_index != 1)
	{

	/* create and initialize a new node of type mathematical sum */

	node = (struct node_type *) malloc (sizeof (struct node_type));
	node->type = sum;
	node->next = NIL;
	node->e.sum.super = NIL;
	node->e.sum.super_hgt = 0;
	node->e.sum.super_lgt = 0;
	node->e.sum.super_dy = 0;
	node->e.sum.sub = NIL;
	node->e.sum.sub_hgt = 0;
	node->e.sum.sub_lgt = 0;
	node->e.sum.sub_dy = 0;
	node->e.sum.arg = NIL;
	node->e.sum.arg_hgt = 0;
	node->e.sum.arg_lgt = 0;
	node->e.sum.arg_dy = 0;
	node->e.sum.sum = math_operator_code[key_index - 1];
	node->e.sum.chr_hgt = 0;
		    
	if (*str != '\\')
	    ret_stat = gus__synbacksl;
	else 
	    {		

	    /* read the argument */

	    str++;

	    node->e.sum.arg = read_expression (act_chr_hgt, 
		&node->e.sum.arg_hgt, &node->e.sum.arg_lgt, 
		&node->e.sum.arg_dy);
	    node->e.sum.arg_hgt = node->e.sum.arg_hgt + node->e.sum.arg_dy;

	    if (node->e.sum.arg == NIL)
		ret_stat = gus__synexpr;
	    else
		{

		node->e.sum.chr_hgt = char_hgt (node->e.sum.arg_hgt) * fact_sum;

		if (*str == '\\')
		    {
			
		    /* read the subscript */ 

		    str++;

		    node->e.sum.sub = read_expression (node->e.sum.chr_hgt, 
			&node->e.sum.sub_hgt, &node->e.sum.sub_lgt, 
			&node->e.sum.sub_dy);
		    node->e.sum.sub_hgt = node->e.sum.sub_hgt + 
			node->e.sum.sub_dy;

		    if (*str == '\\')
			{		

			/* read the superscript */
			
			str++;

			node->e.sum.super = read_expression (
			    node->e.sum.chr_hgt, &node->e.sum.super_hgt, 
			    &node->e.sum.super_lgt, &node->e.sum.super_dy);
			node->e.sum.super_hgt = node->e.sum.super_hgt +
			    node->e.sum.super_dy;
			}
		    }

		/* calculate the height, length and dy of the sum */ 

		math_size (node->e.sum.arg_hgt, node->e.sum.sum, 
		    &node->e.sum.sum_hgt, &node->e.sum.sum_lgt);
		if (node->e.sum.sub_hgt != 0)
		    node->e.sum.sub_hgt = node->e.sum.sub_hgt + 
			node->e.sum.arg_hgt * delta;
		if (node->e.sum.super_hgt != 0)
		    node->e.sum.super_hgt = node->e.sum.super_hgt + 
			node->e.sum.arg_hgt * delta;
		*height = node->e.sum.arg_hgt + node->e.sum.sub_hgt + 
		    node->e.sum.super_hgt;
		*length = node->e.sum.sum_lgt + node->e.sum.arg_lgt;
		*dy = 0.5 * (node->e.sum.arg_hgt - c_size (act_chr_hgt)) + 
		    node->e.sum.sub_hgt;
		*height = *height - *dy;
		}
	    }
	}

    else
	{

	/* create and initialize a new node of type root */

	node = (struct node_type *) malloc (sizeof (struct node_type));
	node->type = root;
	node->next = NIL;
	node->e.root.degree = NIL;
	node->e.root.arg = NIL;
	node->e.root.root = math_operator_code[0];
		    
	if (*str != '\\')
	    ret_stat = gus__synbacksl;
	else 
	    {		

	    /* read the argument */    

	    str++;

	    node->e.root.arg = read_expression (act_chr_hgt, 
		&node->e.root.arg_hgt, &node->e.root.arg_lgt, 
		&node->e.root.arg_dy);
	    node->e.root.arg_hgt = node->e.root.arg_hgt + node->e.root.arg_dy;

	    if (node->e.root.arg == NIL)
		ret_stat = gus__synexpr;
	    else
		{
		node->e.root.deg_hgt = char_hgt (node->e.root.arg_hgt) * 
		    fact_r_degree;

		if (*str == '\\')
		    {		
		    float hgt = 0, lgt = 0, dy = 0;
		    
		    /* read the degree */

		    str++;
		    
		    node->e.root.degree = read_expression (node->e.root.deg_hgt,
			&hgt, &lgt, &dy);
		    }

		/* calculate the height, length and dy of the root */ 

		math_size (node->e.root.arg_hgt, node->e.root.root, 
		    &node->e.root.root_hgt, &node->e.root.root_lgt);
		node->e.root.root_lgt = node->e.root.root_lgt + delta *
		    c_size (act_chr_hgt); 
		*height = node->e.root.arg_hgt;
		*length = node->e.root.root_lgt + node->e.root.arg_lgt;
		*dy = 0.5 * node->e.root.arg_hgt - 0.5 *
		    c_size (act_chr_hgt);
		*height = *height - *dy;
		}
	    }
	}

    return (node);
}



static struct node_type *read_s_par (int key_index, float act_chr_hgt,
    float *height, float *length)

/*
 *  FUNCTIONAL DESCRIPTION:
 *	
 *	Reads a paranthesis.
 */

{
    struct node_type *node;

    /* create an initialize a new node of type parathesis */

    node = (struct node_type *) malloc (sizeof (struct node_type));
    node->type = par;
    node->next = NIL;
    node->e.par.arg = NIL;
    node->e.par.par = par_code[key_index - 1];
		
    if (*str != '\\')
	ret_stat = gus__synbacksl;
    else 
	{		

	/* read the argument */

	str++;

	node->e.par.arg = read_expression (act_chr_hgt, &node->e.par.arg_hgt, 
	    &node->e.par.arg_lgt, &node->e.par.arg_dy);
	node->e.par.arg_hgt = node->e.par.arg_hgt + node->e.par.arg_dy;

	if (node->e.par.arg == NIL)
	    ret_stat = gus__synexpr;
	else
	    {
	    math_size ((1. + delta) * node->e.par.arg_hgt, node->e.par.par, 
		&node->e.par.par_hgt, &node->e.par.par_lgt);
	    *height = (1. + delta) * node->e.par.arg_hgt;
	    *length = node->e.par.arg_lgt + 2. * node->e.par.par_lgt;
	    }
	}

    return (node);
}


    
static struct node_type *read_s_char (u_char *keyword, float act_chr_hgt,
    float *height, float *length)

/*
 *  FUNCTIONAL DESCRIPTION:
 *	
 *	Reads a special character.
 */

{
    int key_index;    
    struct node_type *node;

    /* test if keyword is a greek letter */

    if ((key_index = verb_index (greek_letters, keyword)) > 0)
	{

	/* create and initialize a new node of type operand */

	node = (struct node_type *) malloc (sizeof (struct node_type));
	node->type = operand;
	node->next = NIL;
	node->e.oper.chr_type = greek;

	if (islower(*str)) 

	    /* lowercase greek letter */
	
	    node->e.oper.chr = greek_letter_code[key_index-1];
	else

	    /* uppercase greek letter */
 
	    node->e.oper.chr = uppercase_greek_letter_code[key_index-1];

	char_size (act_chr_hgt, node->e.oper.chr, greek, height, length);
	node->e.oper.length = *length;
	node->e.oper.height = *height;
	}
    else
	{

	/* test if keyword is a math symbol */

	if ((key_index = verb_index (math_symbols, keyword)) > 0)
	    {

	    /* create and initialize a new node of type operand */

	    node = (struct node_type *) malloc (sizeof (struct node_type));
	    node->type = operand;
	    node->next = NIL;
	    node->e.oper.chr_type = math;
	    node->e.oper.chr = math_symbol_code[key_index-1];

	    char_size (act_chr_hgt, node->e.oper.chr, math, height, length);
	    node->e.oper.length = *length;
	    node->e.oper.height = *height;
	    }
	else
	    {

	    /* test if keyword is a special character */

	    if ((key_index = verb_index (special_chars, keyword)) > 0)
		{

		/* create and initialize a new node of type operand */

		node = (struct node_type *) malloc (sizeof (struct node_type));
		node->type = operand;
		node->next = NIL;
		node->e.oper.chr_type = normal;
		node->e.oper.chr = special_char_code[key_index-1];

		char_size (act_chr_hgt, node->e.oper.chr, normal, height, 
		    length);
		node->e.oper.length = *length;
		node->e.oper.height = *height;
		}
	    else
		if ((key_index = verb_index (umlaute, keyword)) > 0)
		    {

		    /* create and initialize a new node of type operand */

		    node = (struct node_type *) malloc (sizeof (struct 
			node_type));
		    node->type = operand;
		    node->next = NIL;
		    node->e.oper.chr_type = normal;

		    if (islower(*str)) 

			/* lowercase umlaute */
		    
			node->e.oper.chr = umlaute_code[key_index-1];
		    else

			/* uppercase umlaute */
	     
			node->e.oper.chr = uppercase_umlaute_code[key_index-1];

		    char_size (act_chr_hgt, node->e.oper.chr, normal, height, 
			length);
		    node->e.oper.length = *length;
		    node->e.oper.height = *height;
		    }
		else
		    {

		    if (odd(ret_stat))
			ret_stat = gus__syninvkey;

		    node = NIL;
		    }
	    }
	}

    return (node);
    }
	


static struct node_type *read_s_exp (float act_chr_hgt, float *height,
    float *length, float *dy)

/*
 *  FUNCTIONAL DESCRIPTION:
 *	
 *	Reads a special expression into a tree and returns the pointer to the 
 *	root of the tree.
 */

{
    int key_index, key_len;
    u_char keyword[256];
    struct node_type *p, *node;

    /* extract keyword of special expression */
    
    *keyword ='\0';
    key_len = 0;
    while (str[key_len] != '\0' && str[key_len] != '\\' && str[key_len] != '>')
	key_len++;
    strncat ((char *) keyword, (char *) str, key_len);

    /* test if keyword is a special operand */

    if ((*(str + key_len) == '>') && ((key_index = 
	verb_index (spec_operators, keyword)) > 0))
	{
	
	/* read special character */

	node = read_s_char (keyword, act_chr_hgt, height, length);
	*dy = 0;
	str = str + key_len;
	}

    /* test if keyword is a math operator */

    else if ((key_index = verb_index (math_operators, keyword)) > 0)
	{

	/* read mathoperator */

	str = str + key_len;
	node = read_m_sum (key_index, act_chr_hgt, height, length, dy);
	}

    /* test if keyword is a paranthesis */

    else if ((key_index = verb_index (pars, keyword)) > 0)
	{

	/* read parathesis */

	str = str + key_len;
	node = read_s_par (key_index, act_chr_hgt, height, length);

	*dy = 0.5 * (*height - c_size (act_chr_hgt));
	*height = *height - *dy;
	}
    else
    
	{

	/* create and initialize a new node of type special expression */

	node = (struct node_type *) malloc (sizeof (struct node_type));
	node->type = s_expr;
	node->next = NIL;
	node->e.s_expr.index = NIL;
	node->e.s_expr.index_hgt = 0;
	node->e.s_expr.index_lgt = 0;
	node->e.s_expr.index_dy = 0;
	node->e.s_expr.exp = NIL;
	node->e.s_expr.exp_hgt = 0;
	node->e.s_expr.exp_lgt = 0;
	node->e.s_expr.exp_dy = 0;
	node->e.s_expr.super = NIL;
	node->e.s_expr.super_hgt = 0;
	node->e.s_expr.super_lgt = 0;
	node->e.s_expr.super_dy = 0;
	node->e.s_expr.sub = NIL;
	node->e.s_expr.sub_hgt = 0;
	node->e.s_expr.sub_lgt = 0;
	node->e.s_expr.sub_dy = 0;
	node->e.s_expr.arg = NIL;
	node->e.s_expr.arg_hgt = 0;
	node->e.s_expr.arg_lgt = 0;
	node->e.s_expr.arg_dy = 0;

	/* read the argument */

	p = read_expression (act_chr_hgt, &node->e.s_expr.arg_hgt, 
	    &node->e.s_expr.arg_lgt, &node->e.s_expr.arg_dy);
	node->e.s_expr.arg_hgt = node->e.s_expr.arg_hgt + node->e.s_expr.arg_dy;
	node->e.s_expr.arg_lgt = node->e.s_expr.arg_lgt + c_size (act_chr_hgt) *
	    delta;

	if (p == NIL)
	    {
	    if (odd(ret_stat))
		ret_stat = gus__synexpr;
	    }
	else
	    {
	    node->e.s_expr.arg = p;

	    if (*str != '\\' && *str != '>')
		ret_stat = gus__synbacksl;

	    else if (*str != '>')
		{		
		
		/* read the index */

		str++;

		node->e.s_expr.index = read_expression (fact_index * 
		    act_chr_hgt, &node->e.s_expr.index_hgt, 
		    &node->e.s_expr.index_lgt, &node->e.s_expr.index_dy);
		node->e.s_expr.index_hgt = node->e.s_expr.index_hgt + 
		    node->e.s_expr.index_dy;

		if (*str != '\\' && *str != '>')
		    ret_stat = gus__synbacksl;

		else if (*str != '>')
		    {		

		    /* read the exponent */
		    
		    str++;
		    
		    node->e.s_expr.exp = read_expression (fact_exp * 
			act_chr_hgt, &node->e.s_expr.exp_hgt, 
			&node->e.s_expr.exp_lgt, &node->e.s_expr.exp_dy);
		    node->e.s_expr.exp_hgt = node->e.s_expr.exp_hgt +
			node->e.s_expr.exp_dy;

		    if (*str != '\\' && *str != '>')
			ret_stat = gus__synbacksl;

		    else if (*str != '>')
			{		

			/* read the subscript */
			
			str++;
			
			node->e.s_expr.sub = read_expression (fact_spec * 
			    act_chr_hgt, &node->e.s_expr.sub_hgt, 
			    &node->e.s_expr.sub_lgt, &node->e.s_expr.sub_dy);
			node->e.s_expr.sub_hgt = node->e.s_expr.sub_hgt +
			    node->e.s_expr.sub_dy;

			if (*str != '\\' && *str != '>')
			    ret_stat = gus__synbacksl;

			else if (*str != '>')
			    {		

			    /* read the superscript */
	
			    str++;

			    node->e.s_expr.super = read_expression (fact_spec * 
				act_chr_hgt, &node->e.s_expr.super_hgt, 
				&node->e.s_expr.super_lgt, 
				&node->e.s_expr.super_dy);
			    node->e.s_expr.super_hgt = node->e.s_expr.super_hgt
				+ node->e.s_expr.super_dy;
			    }
			}
		    }
		}

	    if (node->e.s_expr.sub_hgt != 0)
		node->e.s_expr.sub_hgt = node->e.s_expr.sub_hgt + 
		    c_size (act_chr_hgt) * delta;
	    if (node->e.s_expr.super_hgt != 0)
		node->e.s_expr.super_hgt = node->e.s_expr.super_hgt + 
		    c_size (act_chr_hgt) * delta;
	    *dy = max(node->e.s_expr.sub_hgt, node->e.s_expr.index_hgt -
		(1. - fact_index) * c_size (act_chr_hgt));
	    *height = node->e.s_expr.arg_hgt + max(node->e.s_expr.super_hgt,
		 node->e.s_expr.exp_hgt - (1. - fact_exp) * 
		 c_size (act_chr_hgt));
	    *length = node->e.s_expr.arg_lgt + max(node->e.s_expr.index_lgt, 
		node->e.s_expr.exp_lgt);
	    }
	}

    return (node);
}



static void draw_line (float px1, float px2, float py, float act_chr_hgt)
{
    int n = 2;
    float x[2], y[2];

    x[0] = cos_f * (px1 - ux) - sin_f * (py - uy) + ux;
    x[1] = cos_f * (px2 - ux) - sin_f * (py - uy) + ux;
    y[0] = sin_f * (px1 - ux) + cos_f * (py - uy) + uy;
    y[1] = sin_f * (px2 - ux) + cos_f * (py - uy) + uy;

    GPL (&n, x, y);
}



static void draw_soft_char (float px, float py, u_char chr, int font,
    float act_chr_hgt, int v_align, float *length)
{

    buffer_rec buffer;
    int i, np, len;
    float x[248], y[248], xx, yy;
    float left, size, base, cap, half, bot, width;

    look_up (font, chr, buffer);
    
    left = (float) buffer[f_left];
    len = 2 * (int) buffer[f_len];
    size = (float) buffer[f_size];
    base = (float) buffer[f_base];
    cap = (float) buffer[f_cap];
    bot = (float) buffer[f_bot];
    width = (float) buffer[f_rght] - (float) buffer[f_left];

    *length = expansion * act_chr_hgt * ch_xp * (width / size + spacing);
    if (chr == blank)
	{
	*length = 0.5 * *length;
	return;
	}

    half = (base + cap) * 0.5;
    if (v_align == GABOTT)
	py = py + (half - bot) / size * act_chr_hgt;	

    np = 1;

    for (i = 0; i < len; i=i+2)
	{
	xx = (float) buffer[f_len+1+i];
	if (xx > 127) xx = xx-256;

	if (xx < 0)
	    {
	    xx = - xx;
	    if (np > 1)
		{
		GPL (&np, x, y);
		np = 1;
		}
	    }
	else
	    np++;

	xx = (xx - left) / size * ch_xp * act_chr_hgt * expansion;
	yy = (float) buffer[f_len+2+i];
	yy = (yy - half) / size * act_chr_hgt;
	xx = cos_s * xx - sin_s * yy + px;
	yy = cos_s * yy + py; 
	x[np - 1] = cos_f * (xx - ux) - sin_f * (yy - uy) + ux;
	y[np - 1] = sin_f * (xx - ux) + cos_f * (yy - uy) + uy;
	}
    GPL (&np, x, y);
}


static void draw_char (float act_chr_hgt, u_char chr, font_type type, float px,
    float py)
{
    int font, len = 1;
    float pxt, pyt, chr_hgt, width;
    u_char ch;


    ch = chr;
    chr_hgt = act_chr_hgt;

    if (ch != blank)
	{
	font = font_nr (text_font, type);
	if (type == math)
	    {
	    switch ((int) ch) {
		case 1:
		case 2:
		    ch = '\174';
		    font = -1;
		    draw_soft_char (px, py, ch, font, chr_hgt, GABOTT, 
			&width);
		    break;

		case 3:
		case 4:
		    font = -1;
		    ch = '\174';
		    draw_soft_char (px, py, ch, font, chr_hgt, GABOTT, 
			&width);
		    draw_soft_char (px + width, py, ch, font, chr_hgt, GABOTT, 
			&width);
		    break;

		case 114:
		case 115:
		case 106:
		case 107:
		case 108:
		case 109:
		case 110:
		case 111:
		    draw_soft_char (px, py, ch, font, chr_hgt, GABOTT, 
			&width);
		    break;

		default:
		    py = py + 0.5 * s_size (chr_hgt);
		    draw_soft_char (px, py, ch, font, chr_hgt, GAHALF, 
			&width);
		    break;
		}
	    }
	else
	    {
	    if (text_prec == GSTRKP)
		{
		draw_soft_char (px, py, ch, font, chr_hgt, GABOTT, &width);
		}
	    else
		{
		GSTXFP (&font, &text_prec);
		GSTXAL (&GAHNOR, &GABOTT);
		GSCHH (&chr_hgt);
		GSCHUP (&up_x, &up_y);
		pxt = cos_f * (px - ux) - sin_f * (py - uy) + ux;
		pyt = sin_f * (px - ux) + cos_f * (py - uy) + uy;
		gks_text_s (&pxt, &pyt, &len, (char *)&ch);
		}
	    }
	}
}



static void draw_string (float act_chr_hgt, u_char *string, float px, float py)
{
    int font, len;
    float pxt, pyt, chr_hgt, length;

    chr_hgt = act_chr_hgt;

    font = font_nr (text_font, normal);
    if (text_prec == GSTRKP)
	while (*string != '\0')
	    {
	    draw_soft_char (px, py, *string, font, chr_hgt, GABOTT, 
		&length);
	    px = px + length;
	    string++;
	    }
    else
	{
	GSTXFP (&font, &text_prec);
	GSTXAL (&GAHNOR, &GABOTT);
	GSCHH (&chr_hgt);
	GSCHUP (&up_x, &up_y);
	pxt = cos_f * (px - ux) - sin_f * (py - uy) + ux;
	pyt = sin_f * (px - ux) + cos_f * (py - uy) + uy;
	len = strlen((char *) string);
	gks_text_s (&pxt, &pyt, &len, (char *)string);
	}
}



static void draw_expression (struct node_type *node, float px, float py,
    float dy, float act_chr_hgt)
{
    float fract_lgt, y, bot, half;
    static float top;
    float pl_width;
    
    if (node != NIL)
	{

	y = py + dy;


	pl_width = width_fact * act_chr_hgt;

	switch (node->type) {
	    
	    case operand:
		GSLWSC (&pl_width);
		draw_char (act_chr_hgt, node->e.oper.chr, node->e.oper.chr_type,
		    px, y);
		top = y + node->e.oper.height;
		px = px + node->e.oper.length;
		break;

	    case string:
		GSLWSC (&pl_width);
		draw_string (act_chr_hgt, node->e.string.string, px, y);
		top = y + c_size (act_chr_hgt);
		px = px + node->e.string.length;
		break;

	    case s_expr:
		draw_expression (node->e.s_expr.arg, px, y, 
		    node->e.s_expr.arg_dy, act_chr_hgt);
		draw_expression (node->e.s_expr.index, px + 
		    node->e.s_expr.arg_lgt, y - node->e.s_expr.index_hgt + 
		   (1. - fact_index * 0.875) * c_size (act_chr_hgt), 
		    node->e.s_expr.index_dy, fact_index * act_chr_hgt);
		draw_expression (node->e.s_expr.exp, px + 
		    node->e.s_expr.arg_lgt, y + node->e.s_expr.arg_hgt - 
		   (1. - fact_exp * 0.875) * c_size (act_chr_hgt), 
		    node->e.s_expr.exp_dy, fact_exp * act_chr_hgt);
		draw_expression (node->e.s_expr.sub, px + 0.5 *
		    node->e.s_expr.arg_lgt - 0.5 * node->e.s_expr.sub_lgt, y -
		    node->e.s_expr.sub_hgt * 0.875, node->e.s_expr.sub_dy,
                    fact_spec * act_chr_hgt);
		draw_expression (node->e.s_expr.super, px + 0.5 *
		    node->e.s_expr.arg_lgt - 0.5 * node->e.s_expr.super_lgt, y +
		    node->e.s_expr.arg_hgt * 0.875 + c_size (act_chr_hgt) *
                    delta, node->e.s_expr.sub_dy, fact_spec * act_chr_hgt);
		top = y + node->e.s_expr.arg_hgt + node->e.s_expr.super_hgt;
		px = px + node->e.s_expr.arg_lgt + 
		    max(node->e.s_expr.index_lgt, node->e.s_expr.exp_lgt);
		break;
		
	    case power:
		draw_expression (node->e.power.fact, px, py, dy, act_chr_hgt); 
		top = top - (1. - fact_index * 0.875) * c_size (act_chr_hgt);
		draw_expression (node->e.power.exp, px + 
		    node->e.power.fact_lgt, top, node->e.power.exp_dy, 
		    fact_exp * act_chr_hgt);
		top = top + node->e.power.exp_hgt;
		px = px + node->e.power.fact_lgt + node->e.power.exp_lgt;
		break;
		
	    case indx:
		draw_expression (node->e.index.arg, px, py, dy, act_chr_hgt); 
		draw_expression (node->e.index.ind, px + 
		    node->e.index.arg_lgt, y - node->e.index.ind_hgt + 
		   (1. - fact_index * 0.875) * c_size (act_chr_hgt), 
		    node->e.index.ind_dy, fact_index * act_chr_hgt);
		px = px + node->e.index.arg_lgt + node->e.index.ind_lgt;
		break;
		
	    case fract:
		fract_lgt = max(node->e.fract.numer_lgt, 
		    node->e.fract.denom_lgt) + 4. * delta * 
		    c_size (act_chr_hgt);
		half = y + 0.5 * c_size (act_chr_hgt);
		draw_expression (node->e.fract.numer, px + 0.5 * fract_lgt - 
		    0.5 * node->e.fract.numer_lgt, half + c_size (act_chr_hgt) *
		    delta, node->e.fract.numer_dy, act_chr_hgt);
		GSLWSC (&pl_width);
		draw_line (px + delta * c_size (act_chr_hgt), px + fract_lgt -
		    delta * c_size (act_chr_hgt), half, act_chr_hgt);
		draw_expression (node->e.fract.denom, px + 0.5 * fract_lgt - 
		    0.5 * node->e.fract.denom_lgt, half - c_size (act_chr_hgt) 
		    * delta - node->e.fract.denom_hgt, node->e.fract.denom_dy, 
		    act_chr_hgt);
		top = half + node->e.fract.numer_hgt;
		px = px + fract_lgt;
		break;

	    case sum:
		bot = y + 0.5 * (c_size (act_chr_hgt) - node->e.sum.arg_hgt);
		GSLWSC (&pl_width);
		draw_char (node->e.sum.sum_hgt, node->e.sum.sum, math, px, bot);
		draw_expression (node->e.sum.arg, px + node->e.sum.sum_lgt, bot,
		    node->e.sum.arg_dy, act_chr_hgt);
		draw_expression (node->e.sum.sub, px + 0.5 * 
		   (node->e.sum.sum_lgt - node->e.sum.sub_lgt), bot - 
		    node->e.sum.sub_hgt, node->e.sum.sub_dy, 
		    node->e.sum.chr_hgt);    
		top = bot + node->e.sum.arg_hgt;
		draw_expression (node->e.sum.super, px + 0.5 * 
		    node->e.sum.sum_lgt - 0.5 * node->e.sum.super_lgt, top +
		    node->e.sum.arg_hgt * delta, node->e.sum.super_dy, 
		    node->e.sum.chr_hgt);    
		top = top + node->e.sum.super_hgt;
		px = px + node->e.sum.sum_lgt + node->e.sum.arg_lgt;
		break;

	    case par:
		bot = y + 0.5 * (c_size (act_chr_hgt) - node->e.par.arg_hgt);
		GSLWSC (&pl_width);
		draw_char (node->e.par.par_hgt, node->e.par.par, math, px, bot);
		draw_expression (node->e.par.arg, px + node->e.par.par_lgt, bot,
		    node->e.par.arg_dy, act_chr_hgt);
		GSLWSC (&pl_width);
		draw_char (node->e.par.par_hgt, (u_char)(node->e.par.par + 1),
		    math, px + node->e.par.arg_lgt + node->e.par.par_lgt, bot);
		top = bot + node->e.par.arg_hgt;
		px = px + 2. * node->e.par.par_lgt + node->e.par.arg_lgt;
		break;

	    case root:
		bot = y + 0.5 * (c_size (act_chr_hgt) - node->e.root.arg_hgt);
		GSLWSC (&pl_width);
		draw_char (node->e.root.root_hgt, node->e.root.root, math, px, 
		    bot);
		draw_expression (node->e.root.degree, px, bot + 0.5 *
		    node->e.root.arg_hgt, 0.0, node->e.root.deg_hgt);
		draw_expression (node->e.root.arg, px + node->e.root.root_lgt, 
		    bot, node->e.root.arg_dy, act_chr_hgt);
	        top = bot + node->e.root.arg_hgt;
		px = px + node->e.root.root_lgt + node->e.root.arg_lgt;
		GSLWSC (&pl_width);
		draw_line (px - node->e.root.arg_lgt - delta * 
		    c_size (act_chr_hgt), px, top, act_chr_hgt);
		break;
	    }

	draw_expression (node->next, px, py, dy, act_chr_hgt);
	}
}



static void free_expression (struct node_type *node)
{
    if (node != NIL)
	{
	switch (node->type) {
	    
	    case operand:
            case string:
		break;

	    case s_expr:
		free_expression (node->e.s_expr.arg);
		free_expression (node->e.s_expr.index);
		free_expression (node->e.s_expr.super);
		free_expression (node->e.s_expr.sub);
		break;
		
	    case power:
		free_expression (node->e.power.fact);
		free_expression (node->e.power.exp);
		break;

	    case indx:
		free_expression (node->e.index.arg);
		free_expression (node->e.index.ind);
		break;
		
	    case fract:
		free_expression (node->e.fract.numer);
		free_expression (node->e.fract.denom);
		break;

	    case sum:
		free_expression (node->e.sum.super);
		free_expression (node->e.sum.sub);
		free_expression (node->e.sum.arg);
		break;

	    case par:
		free_expression (node->e.par.arg);
		break;

	    case root:
		free_expression (node->e.root.degree);
		free_expression (node->e.root.arg);
		break;
	    }

	free_expression (node->next);
	free (node);
	}
}



static void init_char_size (void)

/*
 *  Initializes the charcater size for normal characters.
 */

{
    int len = 1;
    int errind, font;
    float cpx, cpy;
    float x = 0, y = 0;
    float tx[4], ty[4];
    float chr_hgt = 1.;

    font = font_nr (text_font, normal);

    GSCHH (&chr_hgt);
    GSTXFP (&font, &GSTRKP);

    gks_inq_text_extent_s (&wkid, &x, &y, &len, (char *)space, &errind,
	&cpx, &cpy, tx, ty);
    character_size = ty[2] - ty[0];

    GSTXFP (&font, &text_prec);
}



static void align (float *chr_hgt, int *align_h, int *align_v, float *d_al_h,
    float *d_al_v)

/*
 *  Calulates the deltas between normal (normal, bottom) and announced 
 *  alignment.
 */

{
    int len = 1;
    int errind, font;
    float cpx, cpy;
    float x = 0, y = 0;
    float tx[4], ty[4];

    font = font_nr (text_font, normal);

    GSCHH (chr_hgt);
    GSTXFP (&font, &text_prec);

    GSTXAL (&GAHNOR, &GABOTT);
    gks_inq_text_extent_s (&wkid, &x, &y, &len, (char *)space, &errind,
	&cpx, &cpy, tx, ty);
    *d_al_h = tx[0];
    *d_al_v = ty[0];

    GSTXAL (align_h, align_v);
    gks_inq_text_extent_s (&wkid, &x, &y, &len, (char *)space, &errind,
	&cpx, &cpy, tx, ty);
    *d_al_h = *d_al_h - tx[0];
    *d_al_v = *d_al_v - ty[0];

    GSTXAL (&GAHNOR, &GABOTT);
}



static void text_routine (float *px, float *py, u_char *chars)

/*
 *  Text output routine
 */

{
    int i, len;

    int errind, align_h, align_v, path, text_color, line_color, line_type;
    float text_height, line_width;
    float rad;
    float n_up_x = 0, n_up_y = 1.;
    float d_al_h, d_al_v;
    float x, y;
 
    struct node_type *root;
    float height, length, dy;

    GQTXFP (&errind, &text_font, &text_prec);

    /* make a copy of the input string */

    strcpy ((char *) buf, (char *) chars);

    str = buf;
    len = strlen((char *) str);

    /* substitute all nonprinting characters */

    for (i = 0; i < len; i++)
	if (!isprint(str[i] & 0177))
	    str[i] = ' ';

    /* inquire gks attributes */

    GQCHXP (&errind, &expansion);		/* text expansion factor */
    GQCHSP (&errind, &spacing);			/* text spacing */
    GQCHH (&errind, &text_height);		/* text height */
    GQCHUP (&errind, &up_x, &up_y);		/* text up vector */
    GQTXAL (&errind, &align_h, &align_v);	/* text alignment */
    GQTXP (&errind, &path);			/* text path */
    GQTXCI (&errind, &text_color);		/* text color */
    GQLN (&errind, &line_type);			/* polyline linetype */
    GQLWSC (&errind, &line_width);		/* polyline linewidth */
    GQPLCI (&errind, &line_color);		/* polyline lincolor */

    /* set new gks attributes */

    GSLN (&GLSOLI);
    GSPLCI (&text_color);
    GSTXP (&GRIGHT);
    GSCHUP (&n_up_x, &n_up_y);

    /* set rotation matrix */

    rad = - atan2 (up_x, up_y);
    cos_f = cos (rad);
    sin_f = sin (rad);

    /* set slant */

    rad = - text_slant / 180. * PI;
    cos_s = cos (rad);
    sin_s = sin (rad);

    /* init character size */

    init_char_size ();

    /* calculate deltas for the alignment */

    align (&text_height, &align_h, &align_v, &d_al_h, &d_al_v);

    /* read the input expression into a tree */

    root = read_expression (text_height, &height, &length, &dy);
    height = height + dy;

    if (odd(ret_stat) && *str != '\0')
	if (*str == '}')
	    ret_stat = gus__synlbrace;
	else
	    ret_stat = gus__synlower;
    
    GSCHUP (&up_x, &up_y);

    if (odd(ret_stat))
	{
	ux = *px;
	uy = *py;
	
        switch (align_h) {

	    /* alignment left */
	    case 1: x = *px - d_al_h; break;

	    /* alignment right */
	    case 3: x = *px - length; break;

	    /* alignment center */
	    case 2: x = *px - 0.5 * (length + d_al_h) + d_al_h; break;

	    /* alignment normal */
	    case 0: x = *px; break;
	    }

	switch (align_v) {

	    /* alignment normal, base */
	    case 4:
	    case 0: y = *py - dy - d_al_v; break;

	    /* alignment cap */
	    case 2: y = *py - dy - d_al_v; break;

	    /* alignment half */
	    case 3: y = *py - dy - d_al_v; break;

	    /* alignment bottom */
	    case 5: y = *py; break;

	    /* alignment top */
	    case 1: y = *py - height - d_al_v + c_size (text_height); break;
	    }

	draw_expression (root, x, y, dy, text_height);
	}

    free_expression (root);

    /* set saved gks attribuites */

    GSCHXP (&expansion);
    GSCHH (&text_height);
    GSCHUP (&up_x, &up_y);
    GSTXAL (&align_h, &align_v);
    GSTXP (&path);
    GSTXCI (&text_color);
    GSTXFP (&text_font, &text_prec);
    GSLN (&line_type);
    GSLWSC (&line_width);
    GSPLCI (&line_color);
}



void gus_text_routine (float *px, float *py, char *chars, int *status)

/*
 * FUNCTIONAL DESCRIPTION:
 *
 *	Output text.
 *	The text is interpreted to be a mathematical formula.
 *
 * FORMAL ARGUMENT(S):
 *
 *	PX,PY		Text position in world co-ordinates
 *	CHARS		String of characters
 *	STATUS		Optional status value
 *
 * IMPLICIT INPUTS:
 *
 *	NONE
 *
 * IMPLICIT OUTPUTS:
 *
 *	NONE
 *
 * COMPLETION CODES:
 *
 *	GUS__NORMAL	normal successful completion
 *	GUS__NOTACT	GKS not in proper state. GKS must be either in the
 *			state WSAC or SGOP
 *	GUS__INVWINLIM	cannot apply logarithmic transformation to current
 *			window
 *
 * SIDE EFFECTS:
 *
 *	NONE
 *
 */

{
    int count = 1;
    int opsta, errind, ol;

    ret_stat = gus__normal;

    GQOPS (&opsta);
    if (opsta >= GWSAC)
	{
	GQACWK (&count, &errind, &ol, &wkid);

	text_routine (px, py, (u_char *)chars);
	}
    else
	/* GKS not in proper state. GKS must be either in the state WSAC
	   or SGOP */

	ret_stat = gus__notact;

    if present (status)
	*status = ret_stat;
    else
	if (!odd(ret_stat))
	    gus_signal (ret_stat, "GUS_TEXT_ROUTINE");
}	 



void gus_set_text_slant (float *slant)
{
    text_slant = *slant;
}



static void inq_text_routine (float *px, float *py, u_char *chars, float *tbx,
    float *tby)

/*
 *  Text inquire routine
 */

{
    int i, len;

    int errind, align_h, align_v, path;
    float text_height, up_x, up_y;
    float rad;
    float n_up_x = 0, n_up_y = 1.;
    float d_al_h, d_al_v;
    float x, y;
 
    struct node_type *root;
    float height, length, dy;

    GQTXFP (&errind, &text_font, &text_prec);

    /* make a copy of the input string */

    strcpy ((char *) buf, (char *) chars);

    str = buf;
    len = strlen((char *) str);

    /* substitute all nonprinting characters */

    for (i = 0; i < len; i++)
	if (!isprint(str[i] & 0177))
	    str[i] = ' ';

    /* inquire gks attributes */

    GQCHXP (&errind, &expansion);		/* text expansion factor */
    GQCHSP (&errind, &spacing);			/* text spacing */
    GQCHH (&errind, &text_height);		/* text height */
    GQCHUP (&errind, &up_x, &up_y);		/* text up vector */
    GQTXAL (&errind, &align_h, &align_v);	/* text alignment */
    GQTXP (&errind, &path);			/* text path */

    /* set new gks attributes */

    GSTXP (&GRIGHT);
    GSCHUP (&n_up_x, &n_up_y);

    /* set rotation matrix */

    rad = - atan2 (up_x, up_y);
    cos_f = cos (rad);
    sin_f = sin (rad);

    /* set slant */

    rad = - text_slant / 180. * PI;
    cos_s = cos (rad);
    sin_s = sin (rad);

    /* init character size */

    init_char_size ();

    /* calculate deltas for the alignment */

    align (&text_height, &align_h, &align_v, &d_al_h, &d_al_v);

    /* read the input expression into a tree */

    root = read_expression (text_height, &height, &length, &dy);
    height = height + dy;

    if (odd(ret_stat) && *str != '\0')
	if (*str == '}')
	    ret_stat = gus__synlbrace;
	else
	    ret_stat = gus__synlower;
    
    GSCHUP (&up_x, &up_y);

    if (odd(ret_stat))
	{
	ux = *px;
	uy = *py;

	switch (align_h) {

	    /* alignment left */
	    case 1: x = *px - d_al_h; break;

	    /* alignment right */
	    case 3: x = *px - length; break;

	    /* alignment center */
	    case 2: x = *px - 0.5 * (length + d_al_h) + d_al_h; break;

	    /* alignment normal */
	    case 0: x = *px; break;
	    }

	switch (align_v) {

	    /* alignment normal, base */
	    case 4:
	    case 0: y = *py - dy - d_al_v; break;

	    /* alignment cap */
	    case 2: y = *py - dy - d_al_v; break;

	    /* alignment half */
	    case 3: y = *py - dy - d_al_v; break;

	    /* alignment bottom */
	    case 5: y = *py; break;

	    /* alignment top */
	    case 1: y = *py - height - d_al_v + c_size (text_height); break;
	    }

	tbx[0] = cos_f * (x - ux) - sin_f * (y - uy) + ux;
	tby[0] = sin_f * (x - ux) + cos_f * (y - uy) + uy;
	tbx[1] = cos_f * (x + length - ux) - sin_f * (y - uy) + ux;
	tby[1] = sin_f * (x + length - ux) + cos_f * (y - uy) + uy;
	tbx[2] = cos_f * (x + length - ux) - sin_f * (y + height - uy) + ux;
	tby[2] = sin_f * (x + length - ux) + cos_f * (y + height - uy) + uy;
	tbx[3] = cos_f * (x - ux) - sin_f * (y + height - uy) + ux;
	tby[3] = sin_f * (x - ux) + cos_f * (y + height - uy) + uy;
	}

    free_expression (root);

    /* set saved gks attribuites */

    GSCHXP (&expansion);
    GSCHH (&text_height);
    GSCHUP (&up_x, &up_y);
    GSTXAL (&align_h, &align_v);
    GSTXP (&path);
    GSTXFP (&text_font, &text_prec);
}



void gus_inq_text_extent_routine (float *px, float *py, char *chars, float *tbx,
    float *tby, int *status)

/*
 * FUNCTIONAL DESCRIPTION:
 *
 *	Evalutes the rectangle of the text which is interpreted to be a 
 *	mathematical formula.
 *
 * FORMAL ARGUMENT(S):
 *
 *	PX,PY		Text position in world co-ordinates
 *	CHARS		String of characters
 *      TBX,TBY		Rectangle of the text
 *	STATUS		Optional status value
 *
 * IMPLICIT INPUTS:
 *
 *	NONE
 *
 * IMPLICIT OUTPUTS:
 *
 *	NONE
 *
 * COMPLETION CODES:
 *
 *	GUS__NORMAL	normal successful completion
 *	GUS__NOTACT	GKS not in proper state. GKS must be either in the
 *			state WSAC or SGOP
 *	GUS__INVWINLIM	cannot apply logarithmic transformation to current
 *			window
 *
 * SIDE EFFECTS:
 *
 *	NONE
 *
 */

{
    int count = 1;
    int opsta, errind, ol;

    ret_stat = gus__normal;

    GQOPS (&opsta);
    if (opsta >= GWSAC)
	{
	GQACWK (&count, &errind, &ol, &wkid);

	inq_text_routine (px, py, (u_char *)chars, tbx, tby);
	}
    else
	/* GKS not in proper state. GKS must be either in the state WSAC
	   or SGOP */

	ret_stat = gus__notact;

    if present (status)
	*status = ret_stat;
    else
	if (!odd(ret_stat))
	    gus_signal (ret_stat, "GUS_INQ_TEXT_EXT_ROUTINE");
}	 
