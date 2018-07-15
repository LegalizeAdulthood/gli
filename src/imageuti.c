/*
 *
 * FACILITY:
 *
 *	GLI IMAGE
 *
 * ABSTRACT:
 *
 *	This module contains the basic application routines for the GLI
 *      IMAGE system
 *
 * AUTHOR:
 *
 *	Jochen Werner
 *
 * VERSION:
 *
 *	V1.0-00
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "system.h"
#include "strlib.h"
#include "image.h"

#define BOOL			    unsigned int
#define BYTE			    unsigned char

#define NIL			    0
#define FALSE			    0
#define TRUE			    1

#define odd(status)		    ((status) & 01)
#define round(x)		    (int)((x)+0.5)

#define N_BITS	    16
#define BLOCK_MASK  0x80
#define CHECK_GAP   10000 
#define INIT_BITS   9
#define FIRST	    257
#define HSIZE	    69001
#define CLEAR	    256 
#define BIT_MASK    0x1f

#define codetabof(i)	codetab[i]
#define htabof(i)       htab[i]
#define MAXCODE(n_bits)	((1 << (n_bits)) - 1)
#define tab_prefixof(i) codetabof(i)
#define tab_suffixof(i) ((BYTE *)(htab))[i]
#define de_stack	((BYTE *)&tab_suffixof(1<<N_BITS))

#define read_buf        ((buffer_count < buffer_size) ? \
                         (data_buffer[buffer_count++]) : (EOF))
#define write_buf(i)    ((buffer_count < buffer_size) ? \
                         (data_buffer[buffer_count++] = (BYTE) (i)) : (EOF))


typedef struct octnode_t {
    unsigned long int colorcount;
    rgb_color rgb;
    rgb_color colorsum;
    unsigned int leaf;
    unsigned int depth;
    struct octnode_t *child[8];
    int colorindex;
    } octnode;

typedef struct listnode_t {
    octnode *node;
    struct listnode_t *next;
    struct listnode_t *prev;
    } listnode;

typedef long int code_int;
typedef long int count_int;


/* Global data structures */

static FILE *img_file;
static struct {
    listnode *head[8];
    listnode *tail[8];
    } lcontext;
static int nleafs;

static char buf[N_BITS];

static BYTE magic_header[] = { "\037\235" };
static BYTE lmask[9] = {0xff, 0xfe, 0xfc, 0xf8, 0xf0, 0xe0, 0xc0, 0x80, 
    0x00};
static BYTE rmask[9] = {0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 
    0xff};

static int block_compress;
static int maxbits;
static int offset;
static long int bytes_out;
static long int out_count;
static int clear_flg;
static long int ratio;
static long int in_count;
static count_int checkpoint;
static int n_bits;
static code_int maxcode;
static code_int free_ent;
static code_int hsize;
static code_int maxmaxcode;
static count_int htab [HSIZE];
static unsigned short codetab [HSIZE];

static BYTE *data_buffer;
static long int buffer_count, buffer_size;


void img_free (image_dscr *img)

/*
 *  img_free - free the memory of an image
 */

{
    free (img->data);
    if (img->type == pcm)
	{
	free (img->color_t);
	}
    if (img->coeff != NULL)
        {
        free (img->coeff);
        }
}



void img_create (image_dscr *img, image_type type, int width, int height,
    int *status)

/*
 *  img_create - allocate memory for an image
 */

{
    int stat;
    register int i, j;

    stat = img__normal;

    switch (type) {
	case pbm:
	    img->color_t = NIL;

	    i = width * height / 8;
	    if (i * 8 != width * height)
		i++;

	    img->data = (BYTE *) malloc (i);
            if (img->data == NULL)
		stat = img__nomem;
            else
                for (j = 0; j < i; j++)
                    img->data[j] = 0;
	    break;

	case pgm:
	    img->color_t = NIL;

	    img->data = (BYTE *) malloc (width * height);
            if (img->data == NULL)
		stat = img__nomem;
	    break;

	case pcm:
	    img->color_t = (color_table *) malloc (sizeof (color_table));
	    if (img->color_t == NULL)
                stat = img__nomem;
	    else
		{
		img->data = (BYTE *) malloc (width * height);
		if (img->data == NULL)
                    stat = img__nomem;
		}
	    break;

	case ppm:
	    img->color_t = NIL;

	    img->data = (BYTE *) malloc (3 * width * height);
            if (img->data == NULL)
		stat = img__nomem;
	    break;
	
	default:
	    stat = img__invimgtyp;
	}

    if (odd (stat))
	{
	img->width = width;
	img->height = height;
	img->type = type;
        img->coeff = NULL;
	}

    if (status != NIL)
	*status = stat;
    else
	if (!odd (stat))
	    raise_exception (stat, 0, NULL, NULL);
}



static
int getint (void)

/*
 *  getint - get the next positive intger from an image file
 */

{
    register int i, integer;

    while ((i = fgetc (img_file)) != EOF)
	{
	if (i == '#')
	    do
		{
		i = fgetc (img_file);
		}
	    while (i != EOF && i != '\n');

	if (!isspace (i)) break;
	}
    
    if (i != EOF)
	{
	if (isdigit (i))
	    {
	    integer = i - '0';
	    while ((i = fgetc (img_file)) != EOF)
		{
		if (!isdigit (i))
		    { 
		    break;
		    }
		else
		    {
		    integer = integer * 10 + (i - '0');
		    }
		}
		
	    return (integer);
	    }
	else
	    return (-1);
	}
    else
	return (-1);
}
    


static
void output (code_int code, int *status)
{
    register int r_off = offset, bits = n_bits;
    register char *bp = buf;

    if (code >= 0) 
        {
	bp += (r_off >> 3);
	r_off &= 7;
	*bp = (*bp & rmask[r_off]) | (code << r_off) & lmask[r_off];
	bp++;
	bits -= (8 - r_off);
	code >>= 8 - r_off;

	if (bits >= 8) 
            {
	    *bp++ = code;
	    code >>= 8;
	    bits -= 8;
	    }

	if (bits)
	    *bp = code;

	offset += n_bits;
	if (offset == (n_bits << 3)) 
            {
	    bp = buf;
	    bits = n_bits;
	    bytes_out += bits;
	    do 
                {
		fputc (*bp++, img_file);
		if (ferror (img_file))
                    {
                    *status = img__writerr;
                    return;
                    }
	        } 

            while (--bits);
	    offset = 0;
	    }

	if (free_ent > maxcode || (clear_flg > 0))
	    {
	    if (offset > 0) 
                {
		if (fwrite (buf, 1, n_bits, img_file) != n_bits)
                    {
                    *status = img__writerr;
                    return;
                    }

		bytes_out += n_bits;
	        }
	    offset = 0;

	    if (clear_flg) 
                {
    	        maxcode = MAXCODE (n_bits = INIT_BITS);
	        clear_flg = 0;
	        }
	    else 
                {
	    	n_bits++;
	    	if (n_bits == maxbits)
		    maxcode = maxmaxcode;
	    	else
		    maxcode = MAXCODE(n_bits);
	        }
	    }
        } 
    else 
        {
	if (offset > 0) 
            {
	    offset = (offset + 7) / 8;
            if (fwrite (buf, 1, offset, img_file) != offset)
		{
                *status = img__writerr;
                return;
                }
	    bytes_out += offset;
	    }

	offset = 0;
        }
}



static
void cl_hash (count_int hsz)
{
    register count_int *htab_p = htab+hsz;
    register long i;
    register long m1 = -1;

    i = hsz - 16;
    do 
	{
	*(htab_p-16) = m1;
	*(htab_p-15) = m1;
	*(htab_p-14) = m1;
	*(htab_p-13) = m1;
	*(htab_p-12) = m1;
	*(htab_p-11) = m1;
	*(htab_p-10) = m1;
	*(htab_p-9) = m1;
	*(htab_p-8) = m1;
	*(htab_p-7) = m1;
	*(htab_p-6) = m1;
	*(htab_p-5) = m1;
	*(htab_p-4) = m1;
	*(htab_p-3) = m1;
	*(htab_p-2) = m1;
	*(htab_p-1) = m1;
	htab_p -= 16;
	} 
    while ((i -= 16) >= 0);

    for ( i += 16; i > 0; i-- )
	*--htab_p = m1;
}



static
void cl_block (int *status)
{
    register long int rat;

    checkpoint = in_count + CHECK_GAP;

    if (in_count > 0x007fffff)
	{
	rat = bytes_out >> 8;
	if (rat == 0) 
	    rat = 0x7fffffff;
	else
	    rat = in_count / rat;
	}
    else
	rat = (in_count << 8) / bytes_out;	/* 8 fractional bits */

    if (rat > ratio)
	ratio = rat;
    else
	{
	ratio = 0;
 	cl_hash ((count_int) hsize );
	free_ent = FIRST;
	clear_flg = 1;
	output ((code_int) CLEAR, status);
	}
}



static
void img_compress (BYTE *data, long int nbytes, int *status)
{
    register long fcode;
    register code_int i = 0;
    register int c;
    register code_int ent;
    register int disp;
    register code_int hsize_reg;
    register int hshift;

    maxbits = N_BITS;
    data_buffer = data;
    buffer_size = nbytes;
    buffer_count = 0;
    block_compress = BLOCK_MASK;

    fputc (magic_header[0], img_file);
    fputc (magic_header[1], img_file);
    fputc ((char)(maxbits | block_compress), img_file);
    if (ferror (img_file))
	{
        *status = img__writerr;
        return;
        }

    offset = 0;
    bytes_out = 3;
    out_count = 0;
    clear_flg = 0;
    ratio = 0;
    in_count = 1;
    checkpoint = CHECK_GAP;
    hsize = HSIZE;
    maxmaxcode = 1 << N_BITS;

    maxcode = MAXCODE (n_bits = INIT_BITS);
    free_ent = ((block_compress) ? FIRST : 256 );

    ent = read_buf;

    hshift = 0;
    for (fcode = (long)hsize;  fcode < 65536L; fcode *= 2L)
    	hshift++;

    hshift = 8 - hshift;

    hsize_reg = hsize;
    cl_hash ((count_int) hsize_reg);

    while ((c = read_buf) != EOF) 
        {
	in_count++;
	fcode = (long) (((long) c << maxbits) + ent);
 	i = ((c << hshift) ^ ent);

	if (htabof (i) == fcode) 
            {
	    ent = codetabof (i);
	    continue;
	    } 
        else 
            if ((long)htabof (i) < 0 )
	        goto nomatch;

 	disp = hsize_reg - i;
	if ( i == 0 )
	    disp = 1;

probe:
	if ((i -= disp) < 0)
	    i += hsize_reg;

	if (htabof (i) == fcode) 
            {
	    ent = codetabof (i);
	    continue;
	    }
	if ((long)htabof (i) > 0) 
	    goto probe;

nomatch:
	output ((code_int) ent, status);
	out_count++;
 	ent = c;
	if (free_ent < maxmaxcode) 
            {
 	    codetabof (i) = free_ent++;
	    htabof (i) = fcode;
	    }
	else 
            if ((count_int)in_count >= checkpoint && block_compress)
	        cl_block (status);
        }

    output ((code_int)ent, status);
    out_count++;
    output ((code_int)-1, status);

    return;
}



static
code_int getcode (void)
{
    register code_int code;
    static int offset = 0, size = 0;
    static BYTE buf[N_BITS];
    register int r_off, bits;
    register BYTE *bp = buf;

    if (clear_flg > 0 || offset >= size || free_ent > maxcode) 
	{
	if (free_ent > maxcode) 
	    {
	    n_bits++;
	    if (n_bits == maxbits)
		maxcode = maxmaxcode;
	    else
		maxcode = MAXCODE (n_bits);
	    }
	if (clear_flg > 0) 
	    {
    	    maxcode = MAXCODE (n_bits = INIT_BITS);
	    clear_flg = 0;
	    }

	size = fread (buf, 1, n_bits, img_file);
	if (size <= 0)
	    return -1;

	offset = 0;
	size = (size << 3) - (n_bits - 1);
	}

    r_off = offset;
    bits = n_bits;

    bp += (r_off >> 3);
    r_off &= 7;

    code = (*bp++ >> r_off);
    bits -= (8 - r_off);
    r_off = 8 - r_off;

    if (bits >= 8) 
	{
	code |= *bp++ << r_off;
	r_off += 8;
	bits -= 8;
	}
	/* high order bits. */

    code |= (*bp & rmask[bits]) << r_off;
    offset += n_bits;

    return code;
}



static
void img_decompress (BYTE *data, long int nbytes, int *status) 
{
    register BYTE *stackp;
    register int finchar;
    register code_int code, oldcode, incode;
    int  n, offset, i;
    char buff[BUFSIZ];

    data_buffer = data;
    buffer_size = nbytes;
    buffer_count = 0;

    if ((fgetc (img_file) != (magic_header[0] & 0xFF)) || 
	(fgetc (img_file) != (magic_header[1] & 0xFF)))
	{
	*status = img__corrupted;
	return;
	}
    maxbits = fgetc (img_file);
    if (ferror (img_file))
	{
        *status = img__corrupted;
        return;
        }

    clear_flg = 0;
    block_compress = maxbits & BLOCK_MASK;
    maxbits &= BIT_MASK;
    maxmaxcode = 1 << maxbits;
    maxcode = MAXCODE (n_bits = INIT_BITS);

    for (code = 255; code >= 0; code--) 
	{
	tab_prefixof(code) = 0;
	tab_suffixof(code) = (BYTE)code;
	}
    free_ent = ((block_compress) ? FIRST : 256 );

    finchar = oldcode = getcode ();
    if (oldcode == -1)
	{
	*status = img__corrupted;
	return;
	}

    n=0;
    buff[n++] = (char)finchar;

    stackp = de_stack;

    while ((code = getcode ()) > -1) 
	{
	if ((code == CLEAR) && block_compress) 
	    {
	    for ( code = 255; code >= 0; code-- )
		tab_prefixof(code) = 0;

	    clear_flg = 1;
	    free_ent = FIRST - 1;
	    if ((code = getcode ()) == -1)
		break;
	    }

	incode = code;

	if (code >= free_ent) 
	    {
            *stackp++ = finchar;
	    code = oldcode;
	    }

	while (code >= 256) 
	    {
	    *stackp++ = tab_suffixof (code);
	    code = tab_prefixof (code);
	    }
	*stackp++ = finchar = tab_suffixof (code);
	do 
	    {
	    buff[n++] = *--stackp;
	    if (n == BUFSIZ) 
		{
		offset = 0;

                for (i = 0; i < n; i++)
                    if (write_buf (buff[offset++]) == EOF)
	                *status = img__corrupted;

                n = 0;
		}
	    } 
	while (stackp > de_stack);

	if ((code = free_ent) < maxmaxcode) 
	    {
	    tab_prefixof(code) = (unsigned short) oldcode;
	    tab_suffixof(code) = finchar;
	    free_ent = code+1;
	    } 

	oldcode = incode;
	}

    offset = 0;

    for (i = 0; i < n; i++)
        if (write_buf (buff[offset++]) == EOF)
	    *status = img__corrupted;
}



void img_read (char *name, image_dscr *img, FILE **file, int *status)

/*
 *  img_read - read an image from a file
 */

{
    char magic_number[3];
    register int width, height;
    register int i, j, k, l, c, n;
    int maxgray, maxcolorlevel, ncolors;
    double x;
    int stat;
    BYTE *buf;

    stat = img__normal;

    if (file)
	{
#if !defined(VMS) && !defined(MSDOS) && !defined(_WIN32)
	if (*name == '|')
	    img_file = popen (name+1, "r");
	else
#endif
	    img_file = (*file == NULL) ? fopen (name, "rb") : *file;
	}
    else
	{
#if !defined(VMS) && !defined(MSDOS) && !defined(_WIN32)
	if (*name == '|')
	    img_file = popen (name+1, "r");
	else
#endif
            img_file = fopen (name, "rb");
	}

    if (img_file)
	{
        if (fgets (magic_number, 3, img_file))
            {
            magic_number[2] = '\0';

            /* PBM format ? */
	    
            if (!strcmp (magic_number, "P1"))
                {
                if ((width = getint ()) <= 0) stat = img__invdim;
                if ((height = getint ()) <= 0) stat = img__invdim;
		
                if (odd (stat))
                    {
                    img_create (img, pbm, width, height, &stat);

                    if (odd (stat))
                        {
                        j = width * height;
                        for (i = 0; i < j; i++)
                            {
                            k = getint ();
                            if (k == 1)
                                img->data[i>>03] = img->data[i>>03] | 
                                    (01 << (7 - i % 8));
                            else if (k != 0)
                                {
                                stat = img__corrupted;
                                break;
                                }
                            }
                        }
                    }
                }

            /* PBM format with RAWBITS ? */

            else if (!strcmp (magic_number, "P4"))
                {
                if ((width = getint ()) <= 0) stat = img__invdim;
                if ((height = getint ()) <= 0) stat = img__invdim;
		
                if (odd (stat))
                    {
                    img_create (img, pbm, width, height, &stat);

                    if (odd (stat))
                        {
                        i = width / 8;
                        if (i * 8 != width)
                            i++;

                        buf = (BYTE *) malloc (i);
                        for (j = 0; j < height; j++)
                            {
                            if (fread (buf, 1, i, img_file))
                                {
                                for (k = 0; k < width; k++)
                                    {
                                    l = j * width + k;

                                    if (buf[k>>03] & (01 << (7 - k % 8)))
                                        img->data[l>>03] |=
                                            (01 << (7 - l % 8));
                                    }
                                }
                            else
                                {
                                stat = img__corrupted;
                                break;
                                }
                            }
                        free (buf);
                        }
                    }
                }

            /* PBM compressed format ? */

            else if (!strcmp (magic_number, "C1"))
                {
                if ((width = getint ()) <= 0) stat = img__invdim;
                if ((height = getint ()) <= 0) stat = img__invdim;

                if (odd (stat))
                    {
                    img_create (img, pbm, width, height, &stat);

                    if (odd (stat))
                        {
                        i = img->width * img->height / 8;
                        if (i * 8 != img->width * img->height)
                            i++;

                        img_decompress (img->data, i, &stat);
                        }
                    }
                }

            /* PGM format ? */

            else if (!strcmp (magic_number, "P2"))
                {
                if ((width = getint ()) <= 0) stat = img__invdim;
                if ((height = getint ()) <= 0) stat = img__invdim;
                maxgray = getint ();
                if (maxgray <= 0 && maxgray >= 256)
                    stat = img__invgraylev;

                if (odd (stat))
                    {
                    img_create (img, pgm, width, height, &stat);

                    if (odd (stat))
                        {
                        img->maxgray = maxgray;
                        n = width * height;
                        for (i = 0; i < n; i++)
                            {
                            c = getint ();
                            if (c < 0)
                                {
                                stat = img__corrupted;
                                break;
                                }
                            else
                                img->data[i] = c;
                            }
                        }
                    }
                }

            /* PGM format with RAWBITS ? */

            else if (!strcmp (magic_number, "P5"))
                {
                if ((width = getint ()) <= 0) stat = img__invdim;
                if ((height = getint ()) <= 0) stat = img__invdim;
                maxgray = getint ();
                if (maxgray <= 0 && maxgray >= 256)
                    stat = img__invgraylev;

                if (odd (stat))
                    {
                    img_create (img, pgm, width, height, &stat);

                    if (odd (stat))
                        {
                        img->maxgray = maxgray;

                        if (!(fread (img->data, 1, width * height, 
                            img_file)))
                            stat = img__corrupted;
                        }
                    }
                }

            /* PGM compressed format ? */

            else if (!strcmp (magic_number, "C2"))
                {
                if ((width = getint ()) <= 0) stat = img__invdim;
                if ((height = getint ()) <= 0) stat = img__invdim;
                maxgray = getint ();
                if (maxgray <= 0 && maxgray >= 256)
                    stat = img__invgraylev;

                if (odd (stat))
                    {
                    img_create (img, pgm, width, height, &stat);

                    if (odd (stat))
                        {
                        img->maxgray = maxgray;
                        img_decompress (img->data, width * height, &stat);
                        }
                    }
                }

            /* PPM format ? */

            else if (!strcmp (magic_number, "P3"))
                {
                if ((width = getint ()) <= 0) stat = img__invdim;
                if ((height = getint ()) <= 0) stat = img__invdim;
                maxcolorlevel = getint ();
                if (maxcolorlevel <= 0 && maxcolorlevel >= 256)
                    stat = img__invcollev;

                if (odd (stat))
                    {
                    x = 255. / maxcolorlevel;

                    img_create (img, ppm, width, height, &stat);

                    if (odd (stat))
                        {
                        n = width * height * 3;
                        for (i = 0; i < n; i++)
                            {
                            c = getint ();
                            if (c < 0)
                                {
                                stat = img__corrupted;
                                break;
                                }
                            else
                                img->data[i] = round (c * x);
                            }
                        }
                    }
                }

            /* PPM format with RAWBITS ? */

            else if (!strcmp (magic_number, "P6"))
                {
                if ((width = getint ()) <= 0) stat = img__invdim;
                if ((height = getint ()) <= 0) stat = img__invdim;
                maxcolorlevel = getint ();
                if (maxcolorlevel <= 0 && maxcolorlevel >= 256)
                    stat = img__invcollev;

                if (odd (stat))
                    {
                    x = 255. / maxcolorlevel;

                    img_create (img, ppm, width, height, &stat);

                    if (odd (stat))
                        {
                        if (!(fread (img->data, 1, width * height * 3, 
                            img_file)))
                            stat = img__corrupted;
                        else
                            {
                            n = width * height * 3;
                            for (i = 0; i < n; i++)
                                img->data[i] = round (img->data[i] * x);
                            }
                        }
                    }
                }

            /* PPM compressed format ? */

            else if (!strcmp (magic_number, "C3"))
                {
                if ((width = getint ()) <= 0) stat = img__invdim;
                if ((height = getint ()) <= 0) stat = img__invdim;
                maxcolorlevel = getint ();
                if (maxcolorlevel <= 0 && maxcolorlevel >= 256)
                    stat = img__invcollev;

                if (odd (stat))
                    {
                    x = 255. / maxcolorlevel;

                    img_create (img, ppm, width, height, &stat);

                    if (odd (stat))
                        {
                        j = img->width * img->height;
                        buf = (BYTE *) malloc (3 * j);
                        if (buf == NULL)
                            stat = img__nomem;
                        else
                            {                            
                            img_decompress (buf, j * 3, &stat);
                            if (odd (stat))
                                {
                                for (i = 0; i < j; i++)
                                    {
                                    img->data[3*i] = round (buf[i] * x);
                                    img->data[3*i+1] = round (buf[i+j] * x);
                                    img->data[3*i+2] = round (
                                        buf[i+2*j] * x);
                                    }
                                }
                            free (buf);
                            }
                        }
                    }
                }

            /* PCM format ? */

            else if (!strcmp (magic_number, "P7"))
                {
                if ((width = getint ()) <= 0) stat = img__invdim;
                if ((height = getint ()) <= 0) stat = img__invdim;
                ncolors = getint ();
                if (ncolors <= 0 && ncolors > 256)
                    stat = img__invnumcol;

                if (odd (stat))
                    {
                    img_create (img, pcm, width, height, &stat);

                    if (odd (stat))
                        {
                        img->color_t->ncolors = ncolors;
                        for (i = 0; i < img->color_t->ncolors; i++)
                            {
                            c = getint ();
                            if (c < 0 || c > 255)
                                {
                                stat = img__corrupted;
                                break;
                                }
                            else
                                img->color_t->rgb[i].r = c;

                            c = getint ();
                            if (c < 0 || c > 255)
                                {
                                stat = img__corrupted;
                                break;
                                }
                            else
                                img->color_t->rgb[i].g = c;

                            c = getint ();
                            if (c < 0 || c > 255)
                                {
                                stat = img__corrupted;
                                break;
                                }
                            else
                                img->color_t->rgb[i].b = c;
                            }
                        }
                    }
                if (odd (stat))
                    {
                    n = width * height;
                    for (i = 0; i < n; i++)
                        {
                        c = getint ();
                        if (c < 0 || c > 255)
                            {
                            stat = img__corrupted;
                            break;
                            }
                        else
                            img->data[i] = c;
                        }
                    }
                }

            /* PCM format with RAWBITS ? */

            else if (!strcmp (magic_number, "P8"))
                {
                if ((width = getint ()) <= 0) stat = img__invdim;
                if ((height = getint ()) <= 0) stat = img__invdim;
                ncolors = getint ();
                if (ncolors <= 0 && ncolors > 256)
                    stat = img__invnumcol;

                if (odd (stat))
                    {
                    img_create (img, pcm, width, height, &stat);
                    img->color_t->ncolors = ncolors;

                    if (odd (stat))
                        {
                        img->color_t->ncolors = ncolors;
                        for (i = 0; i < img->color_t->ncolors; i++)
                            {
                            c = fgetc (img_file);
                            if (c > 255)
                                {
                                stat = img__corrupted;
                                break;
                                }
                            else
                                img->color_t->rgb[i].r = c;

                            c = fgetc (img_file);
                            if (c > 255)
                                {
                                stat = img__corrupted;
                                break;
                                }
                            else
                                img->color_t->rgb[i].g = c;

                            c = fgetc (img_file);
                            if (c > 255)
                                {
                                stat = img__corrupted;
                                break;
                                }
                            else
                                img->color_t->rgb[i].b = c;
                            }
                        }
                    }
                if (odd (stat))
                    {
                    if (!(fread (img->data, 1, width * height, 
                        img_file)))
                        stat = img__corrupted;
                    }
                }

            /* PCM compressed format ? */

            else if (!strcmp (magic_number, "C4"))
                {
                if ((width = getint ()) <= 0) stat = img__invdim;
                if ((height = getint ()) <= 0) stat = img__invdim;
                ncolors = getint ();
                if (ncolors <= 0 && ncolors > 256)
                    stat = img__invnumcol;

                if (odd (stat))
                    {
                    img_create (img, pcm, width, height, &stat);
                    img->color_t->ncolors = ncolors;

                    if (odd (stat))
                        {
                        img->color_t->ncolors = ncolors;
                        for (i = 0; i < img->color_t->ncolors; i++)
                            {
                            c = fgetc (img_file);
                            if (c > 255)
                                {
                                stat = img__corrupted;
                                break;
                                }
                            else
                                img->color_t->rgb[i].r = c;

                            c = fgetc (img_file);
                            if (c > 255)
                                {
                                stat = img__corrupted;
                                break;
                                }
                            else
                                img->color_t->rgb[i].g = c;

                            c = fgetc (img_file);
                            if (c > 255)
                                {
                                stat = img__corrupted;
                                break;
                                }
                            else
                                img->color_t->rgb[i].b = c;
                            }
                        }
                    }
                if (odd (stat))
                    {
                    img_decompress (img->data, width * height, &stat);
                    }
                }
            else
                stat = img__corrupted;
            }
        else
            stat = img__corrupted;

        if (file)
            {
            if (fgetc (img_file) == EOF)
                {
#if !defined(VMS) && !defined(MSDOS) && !defined(_WIN32)
		if (*name == '|')
		    pclose (img_file);
		else
#endif
		    fclose (img_file);

                *file = NULL;
                stat = img__normal;
                }
            else
                *file = img_file;
            }
        else
	    {
#if !defined(VMS) && !defined(MSDOS) && !defined(_WIN32)
	    if (*name == '|')
		pclose (img_file);
	    else
#endif
        	fclose (img_file);
	    }
	}
    else
	stat = img__opefaiinp;

    if (status != NIL)
	*status = stat;
    else
	if (!odd (stat))
	    raise_exception (stat, 0, NULL, NULL);
}



void img_write (char *name, image_dscr *img, img_format format, int *status)

/*
 *  img_write - write an image to a file
 */

{
    register int i, j, k, l;
    int stat;
    BYTE *buf;

    stat = img__normal;

    if (img_file = fopen (name, "wb"))
	{
	switch (img->type) {

	    case pbm:
                switch (format) {

                    case ascii:
			fprintf (img_file, "P1\n");
			fprintf (img_file, "%d %d", img->width, img->height);
			j = img->width * img->height;

			for (i = 0; i < j; i++)
			    {
			    if (!(i % 35))
				fprintf (img_file, "\n");

			    if (img->data[i>>03] & (01 << (7 - i % 8)))
				fprintf (img_file, "1 ");
			    else
				fprintf (img_file, "0 ");
			    }

			fprintf (img_file, "\n");
			break;

                    case binary:
			fprintf (img_file, "P4\n");
			fprintf (img_file, "%d %d\n", img->width, img->height);

			i = img->width / 8;
			if (i * 8 != img->width)
			    i++;

			buf = (BYTE *) malloc (i);
			for (j = 0; j < img->height; j++)
			    {
			    for (k = 0; k < i; k++)
				buf[k] = 0;

			    for (k = 0; k < img->width; k++)
				{
				l = j * img->width + k;

				if (img->data[l>>03] & (01 << (7 - l % 8)))
				    buf[k>>03] |= (01 << (7 - k % 8));
				}

			    if (!fwrite (buf, 1, i, img_file))
				{
				stat = img__writerr;
				break;
				}
			    }
                        free (buf);
			break;

		    case compressed:
		        fprintf (img_file, "C1\n");
		        fprintf (img_file, "%d %d\n", img->width, img->height);
                
			i = img->width * img->height / 8;
			if (i * 8 != img->width * img->height)
			    i++;

                        img_compress (img->data, i, &stat);
			break;
		    }
		break;

	    case pgm:
                switch (format) {

                    case ascii:
		        fprintf (img_file, "P2\n");
		        fprintf (img_file, "%d %d\n", img->width, img->height);
		        fprintf (img_file, "%d", img->maxgray);

			j = img->width * img->height;
			for (i = 0; i < j; i++)
			    {
			    if (!(i % 17))
				fprintf (img_file, "\n");

			    fprintf (img_file, "%3d ", img->data[i]);
			    }

			fprintf (img_file, "\n");
			break;

                    case binary:
		        fprintf (img_file, "P5\n");
		        fprintf (img_file, "%d %d\n", img->width, img->height);
		        fprintf (img_file, "%d\n", img->maxgray);
                
		        if (!fwrite (img->data, 1, img->width * img->height, 
                            img_file))
		        stat = img__writerr;
                        break;

                    case compressed:
		        fprintf (img_file, "C2\n");
		        fprintf (img_file, "%d %d\n", img->width, img->height);
		        fprintf (img_file, "%d\n", img->maxgray);
                
                        img_compress (img->data, img->width * img->height, 
                            &stat);
                        break;
                    }
		break;

	    case ppm:
                switch (format) {

                    case ascii:
		        fprintf (img_file, "P3\n");
		        fprintf (img_file, "%d %d\n", img->width, img->height);
		        fprintf (img_file, "255");
                
			j = img->width * img->height;
			for (i = 0; i < j; i++)
			    {
			    if (!(i % 5))
				fprintf (img_file, "\n");

			    fprintf (img_file, "%3d %3d %3d  ", img->data[3*i],
				img->data[3*i+1], img->data[3*i+2]);
			    }

			fprintf (img_file, "\n");
			break;

                    case binary:
		        fprintf (img_file, "P6\n");
		        fprintf (img_file, "%d %d\n", img->width, img->height);
		        fprintf (img_file, "255\n");
                
		        if (!fwrite (img->data, 1, 3 * img->width * 
			    img->height, img_file))
		        stat = img__writerr;
                        break;

                    case compressed:
		        fprintf (img_file, "C3\n");
		        fprintf (img_file, "%d %d\n", img->width, img->height);
		        fprintf (img_file, "255\n");
                        
                        j = img->width * img->height;
                        buf = (BYTE *) malloc (3 * j);
		        if (buf == NULL)
                            stat = img__nomem;
                        else
                            {                            
                            for (i = 0; i < j; i++)
                                {
                                buf[i] = img->data[3*i];
                                buf[i+j] = img->data[3*i+1];
                                buf[i+2*j] = img->data[3*i+2];
                                }
                            img_compress (buf, j*3, &stat);
                            free (buf);
                            }
                        break;
                    }
		break;

	    case pcm:
                switch (format) {

                    case ascii:
			fprintf (img_file, "P7\n");
			fprintf (img_file, "%d %d\n", img->width, img->height);
			fprintf (img_file, "%d", img->color_t->ncolors);

			for (i = 0; i < img->color_t->ncolors; i++)
			    {
			    fprintf (img_file, "\n%3d %3d %3d", 
				img->color_t->rgb[i].r, img->color_t->rgb[i].g,
				img->color_t->rgb[i].b);
			    }
                
			j = img->width * img->height;
			for (i = 0; i < j; i++)
			    {
			    if (!(i % 17))
				fprintf (img_file, "\n");

			    fprintf (img_file, "%3d ", img->data[i]);
			    }

			fprintf (img_file, "\n");
			break;

                    case binary:
			fprintf (img_file, "P8\n");
			fprintf (img_file, "%d %d\n", img->width, img->height);
			fprintf (img_file, "%d\n", img->color_t->ncolors);

			for (i = 0; i < img->color_t->ncolors; i++)
			    {
			    fputc (img->color_t->rgb[i].r, img_file);
			    fputc (img->color_t->rgb[i].g, img_file);
			    fputc (img->color_t->rgb[i].b, img_file);
			    }
                
		        if (!fwrite (img->data, 1, img->width * img->height, 
                            img_file))
		        stat = img__writerr;
                        break;

                    case compressed:
			fprintf (img_file, "C4\n");
			fprintf (img_file, "%d %d\n", img->width, img->height);
			fprintf (img_file, "%d\n", img->color_t->ncolors);

			for (i = 0; i < img->color_t->ncolors; i++)
			    {
			    fputc (img->color_t->rgb[i].r, img_file);
			    fputc (img->color_t->rgb[i].g, img_file);
			    fputc (img->color_t->rgb[i].b, img_file);
			    }
                
                        img_compress (img->data, img->width * img->height, 
                            &stat);
                        break;
                    }
		break;
	    }

	fclose (img_file);
	}
    else
	stat = img__opefaiout;

    if (status != NIL)
	*status = stat;
    else
	if (!odd (stat))
	    raise_exception (stat, 0, NULL, NULL);
}



/*
 *  childnr - get the number of the child in the octree, wich the color of
 *	      first argument refering to second argument
 */ 

#define childnr(c1, c2) (int)((c1)->r > (c2)->r) +\
			((((c1)->g > (c2)->g))<<01) +\
		        ((((c1)->b > (c2)->b))<<02)



static
void reduce_tree (octnode *node)

/*
 *  reduce_tree - reduces the octree
 */

{
    rgb_color sum;
    register int i;

    sum.r = 0;
    sum.g = 0;
    sum.b = 0;

    for (i = 0; i < 8; i++)
	{
	if (node->child[i] != NIL)
	    {
	    sum.r = sum.r + node->child[i]->colorsum.r;
	    sum.g = sum.g + node->child[i]->colorsum.g;
	    sum.b = sum.b + node->child[i]->colorsum.b;

	    free (node->child[i]);
	    node->child[i] = NIL;

	    nleafs--;
	    }
	}

    node->leaf = TRUE;
    node->colorsum.r = sum.r;
    node->colorsum.g = sum.g;
    node->colorsum.b = sum.b;
    nleafs++;
}



static
void appendnode (octnode *node)

/*
 *  appendnode - appends an octree node to the list of reducible nodes
 */

{
    register listnode *p;

    p = (listnode *) malloc (sizeof (listnode));

    if (lcontext.head[node->depth] != NIL)
	{
	lcontext.tail[node->depth]->next = p;
	p->prev = lcontext.tail[node->depth];
	}
    else
	{
	p->prev = NIL;
	lcontext.head[node->depth] = p;
	}
	
    p->next = NIL;
    p->node = node;
    lcontext.tail[node->depth] = p;
}
    


static
octnode *getreducible (void)

/*
 *  getreducible - get the next reducible octnode
 */

{
    register BOOL found;
    register int i;
    register listnode *p, *reducible;
    register octnode *rnode;

    found = FALSE;
    i = 8;

    while (!found)
	{
	i--;
	if ((reducible = lcontext.head[i]) != NIL)
	    {
	    found = TRUE;
	    p = reducible->next;

	    while (p != NIL)
		{
		if (reducible->node->colorcount > p->node->colorcount)
		    {
		    reducible = p;
		    }

		p =  p->next;
		}
	    }
	}

    if (reducible->prev == NIL)
	{
	lcontext.head[i] = reducible->next;
	}
    else
	{
	reducible->prev->next = reducible->next;
	}
    if (reducible->next == NIL)
	{
	lcontext.tail[i] = reducible->prev;
	}
    else
	{
	reducible->next->prev = reducible->prev;
	}

    rnode = reducible->node;

    free (reducible);

    return (rnode);
}



static
void insert_color (octnode *node, rgb_color *rgb)

/*
 *  insert_color - insert a color in the octree
 */

{
    register int i, j, depth;

    node->colorcount++;
    if (node->leaf)
	{
	node->colorsum.r = node->colorsum.r + rgb->r;
	node->colorsum.g = node->colorsum.g + rgb->g;
	node->colorsum.b = node->colorsum.b + rgb->b;
	}
    else
	{
	i = childnr (rgb, &(node->rgb));
	if (node->child[i] == NIL)
	    {
	    node->child[i] = (octnode *) malloc (sizeof (octnode));
	    node->child[i]->colorcount = 0;
	    node->child[i]->depth = node->depth + 1;

	    for (j = 0; j < 8; j++)
		node->child[i]->child[j] = NIL;

	    if (node->depth < 7)
		{
		appendnode (node->child[i]);

		node->child[i]->leaf = FALSE;

		depth = node->child[i]->depth;
		if (node->rgb.r < rgb->r)
		    node->child[i]->rgb.r = node->rgb.r + (128>>depth);
		else
		    node->child[i]->rgb.r = node->rgb.r - (128>>depth);

		if (node->rgb.g < rgb->g)
		    node->child[i]->rgb.g = node->rgb.g + (128>>depth);
		else
		    node->child[i]->rgb.g = node->rgb.g - (128>>depth);

		if (node->rgb.b < rgb->b)
		    node->child[i]->rgb.b = node->rgb.b + (128>>depth);
		else
		    node->child[i]->rgb.b = node->rgb.b - (128>>depth);
		
		insert_color (node->child[i], rgb);
		}
	    else
		{
		nleafs++;
		node->child[i]->leaf = TRUE;
		node->child[i]->colorsum.r = rgb->r;
		node->child[i]->colorsum.g = rgb->g;
		node->child[i]->colorsum.b = rgb->b;
		node->child[i]->colorcount++;
		}
	    }
	else
	    {
	    if (!node->leaf)
		insert_color (node->child[i], rgb);
	    }
	}
}



static
void set_colort (octnode *node, int *i, rgb_color *colort)

/*
 *  set_colort - setup a colortable from the colors of an octree
 */

{
    register int j;

    if (node != NIL)
	{
	if (node->leaf)
	    {
	    colort[*i].r = node->colorsum.r / node->colorcount;
	    colort[*i].g = node->colorsum.g / node->colorcount;
	    colort[*i].b = node->colorsum.b / node->colorcount;

	    node->colorindex = *i;
	    (*i)++;
	    }
	else
	    {
	    for (j = 0; j < 8; j++)
		set_colort (node->child[j], i, colort);
	    }
	}
}



static
BYTE color (octnode *node, rgb_color *rgb)

/*
 *  color - find the octnode which represents the color
 */ 

{
    if (node != NIL)
	{
	if (node->leaf)
	    {
	    return ((BYTE)node->colorindex);
	    }
	else
	    {
	    return (color (node->child[childnr (rgb, &(node->rgb))], rgb));
	    }
	}
    else
        return (0);
}



static
void free_octree (octnode *node)

/*
 *  free_octree - free the memory allocated by the octree
 */

{
    register int i;

    if (node != NIL)
	{
	for (i = 0; i < 8; i++)
	    free_octree (node->child[i]);

	free (node);
	}
}


	    
void img_colorquant (image_dscr *img_old, image_dscr *img_new, int ncolors,
    int *status)

/*
 *  img_colorquant - quantizise an image to the desired number of colors
 */

{
    register octnode *root;
    rgb_color rgb;
    int i;
    register int n;
    int stat;
    register listnode *node, *p;
    register octnode *onode;

    stat = img__normal;

    if (ncolors >= 1 && ncolors <= 256)
	{
	switch (img_old->type) {

	    case ppm:
		img_create (img_new, pcm, img_old->width, img_old->height, 
		    &stat);
		if (!odd (stat)) break;

		nleafs = 0;
		root = (octnode *) malloc (sizeof (octnode));

		for (i = 0; i < 8; i++)
		    root->child[i] = NIL;

		root->rgb.r = 127;
		root->rgb.g = 127;
		root->rgb.b = 127;
		root->colorsum.r = 0;
		root->colorsum.g = 0;
		root->colorsum.b = 0;
		root->colorcount = 0;

		root->depth = 0;
		root->leaf = FALSE;

		for (i = 0; i < 8; i++)
		    {
		    lcontext.head[i] = NIL;
		    lcontext.tail[i] = NIL;
		    }

		appendnode (root);

                n = img_old->height * img_old->width;
		for (i = 0; i < n; i++)
		    {
		    rgb.r = img_old->data[3*i];
		    rgb.g = img_old->data[3*i+1];
		    rgb.b = img_old->data[3*i+2];

		    insert_color (root, &rgb);

		    while (nleafs > ncolors)
			{
			onode = getreducible ();
			reduce_tree (onode);
			}
		    }

		i = 0;
		set_colort (root, &i, img_new->color_t->rgb);
		img_new->color_t->ncolors = i;

                n = img_new->height * img_new->width;
		for (i = 0; i < n; i++)
		    {
		    rgb.r = img_old->data[3*i];
		    rgb.g = img_old->data[3*i+1];
		    rgb.b = img_old->data[3*i+2];

		    img_new->data[i] = color (root, &rgb);
		    }

		for (i = 0; i < 8; i++)
		    {
		    node = lcontext.head[i];
		    while (node != NIL)
			{
			p = node;
			node = node->next;
			free (p);
			}
		    }

		free_octree (root);
		break;

	    case pcm:
		img_create (img_new, pcm, img_old->width, img_old->height,
		    &stat);
		if (!odd (stat)) break;

		nleafs = 0;
		root = (octnode *) malloc (sizeof (octnode));

		for (i = 0; i < 8; i++)
		    root->child[i] = NIL;

		root->rgb.r = 127;
		root->rgb.g = 127;
		root->rgb.b = 127;
		root->colorsum.r = 0;
		root->colorsum.g = 0;
		root->colorsum.b = 0;
		root->colorcount = 0;

		root->depth = 0;
		root->leaf = FALSE;

		for (i = 0; i < 8; i++)
		    {
		    lcontext.head[i] = NIL;
		    lcontext.tail[i] = NIL;
		    }

		appendnode (root);

                n = img_old->height * img_old->width;
		for (i = 0; i < n; i++)
		    {
		    rgb.r = img_old->color_t->rgb[img_old->data[i]].r;
		    rgb.g = img_old->color_t->rgb[img_old->data[i]].g;
		    rgb.b = img_old->color_t->rgb[img_old->data[i]].b;

		    insert_color (root, &rgb);

		    while (nleafs > ncolors)
			{
			onode = getreducible ();
			reduce_tree (onode);
			}
		    }

		i = 0;
		set_colort (root, &i, img_new->color_t->rgb);
		img_new->color_t->ncolors = i;

                n = img_new->height * img_new->width;
		for (i = 0; i < n; i++)
		    {
		    rgb.r = img_old->color_t->rgb[img_old->data[i]].r;
		    rgb.g = img_old->color_t->rgb[img_old->data[i]].g;
		    rgb.b = img_old->color_t->rgb[img_old->data[i]].b;

		    img_new->data[i] = color (root, &rgb);
		    }

		for (i = 0; i < 8; i++)
		    {
		    node = lcontext.head[i];
		    while (node != NIL)
			{
			p = node;
			node = node->next;
			free (p);
			}
		    }

		free_octree (root);
		break;

	    default:
		stat = img__invimgtyp;
		break;
	    }
	}
    else
	stat = img__invnumcol;

    if (status != NIL)
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);    
}



void img_copy (image_dscr *img_old, image_dscr *img_new, int *status)

/*
 * img_copy - copy one image to an other image
 */

{
    register int i, j, n;
    int stat, size;

    stat = img__normal;

    switch (img_old->type) {

	case ppm:
	    img_create (img_new, ppm, img_old->width, img_old->height, &stat);
	    if (!odd (stat)) break;

            n = 3 * img_old->width * img_old->height;
	    for (i = 0; i < n; i++)
		img_new->data[i] = img_old->data[i];
	    break;

	case pcm:
	    img_create (img_new, pcm, img_old->width, img_old->height, &stat);
	    if (!odd (stat)) break;

	    img_new->color_t->ncolors = img_old->color_t->ncolors;
	    for (i = 0; i < img_new->color_t->ncolors; i++)
		img_new->color_t->rgb[i] = img_old->color_t->rgb[i];

            n = img_old->width * img_old->height;
	    for (i = 0; i < n; i++)
		img_new->data[i] = img_old->data[i];
	    break;

	case pgm:
	    img_create (img_new, pgm, img_old->width, img_old->height, &stat);
	    if (!odd (stat)) break;

	    img_new->maxgray = img_old->maxgray;
	    
            n = img_old->width * img_old->height;
	    for (i = 0; i < n; i++)
		img_new->data[i] = img_old->data[i];
	    break;

	case pbm:
	    img_create (img_new, pbm, img_old->width, img_old->height, &stat);
	    if (!odd (stat)) break;

	    j = img_old->width * img_old->height / 8;
	    if (j * 8 != img_old->width * img_old->height)
		j++;

	    for (i = 0; i < j; i++)
		img_new->data[i] = img_old->data[i];
	    break;

	default:
	    stat = img__invimgtyp;
	}

    if (odd(stat))
	{ 
	img_new->dimx = img_old->dimx;
	img_new->dimy = img_old->dimy;

	if (img_old->coeff != NULL)
	    {
	    size = img_old->dimx * img_old->dimy * 2 * sizeof(float);
	    img_new->coeff = (float *) malloc(size);

	    if (img_new->coeff != NULL)
		memcpy(img_new->coeff, img_old->coeff, size); 
	    else
		stat = img__nomem;
	    }
	else
	    img_new->coeff = NULL;
	}

    if (status != NIL)
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);
}


	    
void img_ppm (image_dscr *img_old, image_dscr *img_new, int *status)

/*
 *  img_ppm - convert an image to a ppm image
 */

{
    register int i, n;
    int stat;

    stat = img__normal;

    switch (img_old->type) {

	case ppm:
	    img_copy (img_old, img_new, &stat);
	    break;
	    
	case pcm:
	    img_create (img_new, ppm, img_old->width, img_old->height, &stat);
	    if (!odd (stat)) break;

            n = img_old->height * img_old->width;
	    for (i = 0; i < n; i++)
		{
		img_new->data[3*i] = 
		    img_old->color_t->rgb[img_old->data[i]].r;
		img_new->data[3*i+1] = 
		    img_old->color_t->rgb[img_old->data[i]].g;
		img_new->data[3*i+2] = 
		    img_old->color_t->rgb[img_old->data[i]].b;
		}
	    break;

	case pgm:
	    img_create (img_new, ppm, img_old->width, img_old->height, &stat);
	    if (!odd (stat)) break;

            n = img_old->height * img_old->width;
	    for (i = 0; i < n; i++)
		{
		img_new->data[3*i] = img_old->data[i];
		img_new->data[3*i+1] = img_old->data[i];
		img_new->data[3*i+2] = img_old->data[i];
		}
	    break;

	case pbm:
	    img_create (img_new, ppm, img_old->width, img_old->height, &stat);
	    if (!odd (stat)) break;
                
            n = img_old->height * img_old->width;
	    for (i = 0; i < n; i++)
		{
		if (img_old->data[i>>03] & (01 << (7 - i % 8)))
		    {
		    img_new->data[3*i] = 0;
		    img_new->data[3*i+1] = 0;
		    img_new->data[3*i+2] = 0;
		    }
		else
		    {
		    img_new->data[3*i] = 255;
		    img_new->data[3*i+1] = 255;
		    img_new->data[3*i+2] = 255;
		    }
		}
	    break;

	default:
	    stat = img__invimgtyp;
	    break;
	}

    if (status != NIL)
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);
}



void img_pcm (image_dscr *img_old, image_dscr *img_new, int ncolors,
    int *status)

/*
 *  img_pcm - convert an image to a pcm image
 */

{
    register int i, n;
    int stat;

    stat = img__normal;

    if (ncolors < 1 || ncolors > 256)
	stat = img__invnumcol;
    else
	{
	switch (img_old->type) {

	    case ppm:
		img_colorquant (img_old, img_new, ncolors, &stat);
		break;

	    case pcm:
		if (img_old->color_t->ncolors <= ncolors)
		    img_copy (img_old, img_new, &stat);
		else
		    img_colorquant (img_old, img_new, ncolors, &stat);
		break;

	    case pgm:
		img_create (img_new, pcm, img_old->width, img_old->height, 
		    &stat);
		if (!odd (stat)) break;

		img_new->color_t->ncolors = ncolors;
		for (i = 0; i < ncolors; i++)
		    {
		    img_new->color_t->rgb[i].r = round (i / (ncolors - 1.0) * 
                        255.);
		    img_new->color_t->rgb[i].g = round (i / (ncolors - 1.0) *
                        255.);
		    img_new->color_t->rgb[i].b = round (i / (ncolors - 1.0) *
                        255.);
		    }

                n = img_old->height * img_old->width;
		for (i = 0; i < n; i++)
		    {
		    img_new->data[i] = round (img_old->data[i] / 
			(double)img_old->maxgray * (ncolors - 1));
		    }
		break;

	    case pbm:
		img_create (img_new, pcm, img_old->width, img_old->height, 
		    &stat);
		if (!odd (stat)) break;

		img_new->color_t->ncolors = 2;
		img_new->color_t->rgb[0].r = 0;
		img_new->color_t->rgb[0].g = 0;
		img_new->color_t->rgb[0].b = 0;
		img_new->color_t->rgb[1].r = 255;
		img_new->color_t->rgb[1].g = 255;
		img_new->color_t->rgb[1].b = 255;

                n = img_old->height * img_old->width;
		for (i = 0; i < n; i++)
		    {
		    if (img_old->data[i>>03] & (01 << (7 - i % 8)))
			{
			img_new->data[i] = 0;
			}
		    else
			{
			img_new->data[i] = 1;
			}
		    }
		break;

	    default:
		stat = img__invimgtyp;
		break;
	    }
	}

    if (status != NIL)
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);
}



void img_pgm (image_dscr *img_old, image_dscr *img_new, int maxgray,
    int *status)

/*
 *  img_pgm - convert an image to a pgm image
 */

{
    register int i, n;
    int stat;

    stat = img__normal;

    if (maxgray < 0 || maxgray > 255)
	stat = img__invmaxgray;
    else
	{
	switch (img_old->type) {

	    case ppm:
		img_create (img_new, pgm, img_old->width, img_old->height, 
		    &stat);
		if (!odd (stat)) break;

		img_new->maxgray = maxgray;

                n = img_old->height * img_old->width;
		for (i = 0; i < n; i++)
		    {
		    img_new->data[i] = round ((.299 * img_old->data[3*i] + 
		       .587 * img_old->data[3*i+1] + .114 * 
		       img_old->data[3*i+2]) / 255. * img_new->maxgray);
		    }
		break;

	    case pcm:
		img_create (img_new, pgm, img_old->width, img_old->height,
		    &stat);
		if (!odd (stat)) break;

		img_new->maxgray = maxgray;

                n = img_old->height * img_old->width;
		for (i = 0; i < n; i++)
		    {
		    img_new->data[i] = round ((
			.299 * img_old->color_t->rgb[img_old->data[i]].r + 
			.587 * img_old->color_t->rgb[img_old->data[i]].g +
			.114 * img_old->color_t->rgb[img_old->data[i]].b) /
			255. * img_new->maxgray);
		    }
		break;

	    case pgm:
		if (maxgray == img_old->maxgray)
		    img_copy (img_old, img_new, &stat);
		else
		    {
		    img_create (img_new, pgm, img_old->width, img_old->height,
			&stat);
		    if (!odd (stat)) break;

		    img_new->maxgray = maxgray;

                    n = img_old->height * img_old->width;
		    for (i = 0; i < n; i++)
			img_new->data[i] = round ((double)img_old->data[i] /
			    img_old->maxgray * img_new->maxgray);
		    }

	    case pbm:
		img_create (img_new, pgm, img_old->width, img_old->height,
		    &stat);
		if (!odd (stat)) break;

		img_new->maxgray = maxgray;

                n = img_old->height * img_old->width;
		for (i = 0; i < n; i++)
		    {
		    if (img_old->data[i>>03] & (01 << (7 - i % 8)))
			{
			img_new->data[i] = 0;
			}
		    else
			{
			img_new->data[i] = maxgray;
			}
		    }
		break;

	    default:
		stat = img__invimgtyp;
		break;
	    }
	}

    if (status != NIL)
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);
}



void img_pbm (image_dscr *img_old, image_dscr *img_new, double level,
    int *status)

/*
 *  img_pbm - convert an image to a pbm image
 */

{
    register int i, n;
    int stat;

    stat = img__normal;

    if (level <= 0 || level >= 1.)
	stat = img__invlevel;
    else
	{
	switch (img_old->type) {

	    case ppm:
		img_create (img_new, pbm, img_old->width, img_old->height,
		    &stat);
		if (!odd (stat)) break;

                n = img_old->height * img_old->width;
		for (i = 0; i < n; i++)
		    {
		    if (!(i % 8))
			img_new->data[i>>03] = 0;

		    if ((.299 * img_old->data[3*i] + .587 * 
			img_old->data[3*i+1] + .114 * img_old->data[3*i+2]) / 
			    255. < level)
			{
			img_new->data[i>>03] = img_new->data[i>>03] | 
			    (01 << (7 - i % 8));
			}
		    }
		break;

	    case pcm:
		img_create (img_new, pbm, img_old->width, img_old->height,
		    &stat);
		if (!odd (stat)) break;

                n = img_old->height * img_old->width;
		for (i = 0; i < n; i++)
		    {
		    if (!(i % 8))
			img_new->data[i>>03] = 0;

		    if ((.299 * img_old->color_t->rgb[img_old->data[i]].r + 
			.587 * img_old->color_t->rgb[img_old->data[i]].g + 
			.114 * img_old->color_t->rgb[img_old->data[i]].b) / 
			255. < level)
			{
			img_new->data[i>>03] = img_new->data[i>>03] | 
			    (01 << (7 - i % 8));
			}
		    }
		break;

	    case pgm:
		img_create (img_new, pbm, img_old->width, img_old->height, 
		    &stat);
		if (!odd (stat)) break;

                n = img_old->height * img_old->width;
		for (i = 0; i < n; i++)
		    {
		    if (!(i % 8))
			img_new->data[i>>03] = 0;

		    if (img_old->data[i]/(double)img_old->maxgray < level)
			{
			img_new->data[i>>03] = img_new->data[i>>03] | 
			    (01 << (7 - i % 8));
			}
		    }
		break;

	    case pbm:
		img_copy (img_old, img_new, &stat);
		break;

	    default:
		stat = img__invimgtyp;
		break;
	    }
	}

    if (status != NIL)
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);
}



void img_import (float *array, int dimx, int dimy, image_dscr *img, int *status)

/*
 *  img_import - import an array containing float numbers into an image of
 *		 the 'Image System'
 */

{
    register int i, j;
    int stat;
    float amax, amin, a;
    
    stat = img__normal;

    img_create (img, pgm, dimx, dimy, &stat);
    if (odd (stat))
	{
	img->maxgray = 255;

	amax = array[0];
	amin = array[0];

	for (i = 1; i < dimx * dimy; i++)
	    {
	    if (array[i] < amin)
		amin = array[i];
	    if (array[i] > amax)
		amax = array[i];
	    }
	if ((a = amax - amin) == 0) a = 1;

	for (i = 0; i < dimy; i++)
	    for (j = 0; j < dimx; j++)
		img->data[(dimy - i - 1) * dimx + j] = round ((
		    array[i * dimx + j] - amin) / a * 255.);
	}

    if (status != NIL)
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);
}



void img_export (float *array, image_dscr *img, int *status)

/*
 *  img_export - export an image of the 'Image System' to an array containing 
 *		 float numbers between 0.0 and 1.0
 */

{
    register int i, j, k, l;
    int stat;

    stat = img__normal;

    switch (img->type) {

	case ppm:
	    for (i = 0; i < img->height; i++)
		for (j = 0; j < img->width; j++)
		    {
		    k = i * img->width + j;
		    l = 3 * ((img->height - i - 1) * img->width + j);

		    array[k] = (.299 * img->data[l] + .587 * img->data[l+1] +
			.114 * img->data[l+2]) / 255.;
		    }
	    break;

	case pcm:
	    for (i = 0; i < img->height; i++)
                for (j = 0; j < img->width; j++)
                    {
                    k = i * img->width + j;
		    l = (img->height - i - 1) * img->width + j;

		    array[k] = (.299 * img->color_t->rgb[img->data[l]].r +
			.587 * img->color_t->rgb[img->data[l]].g +
			.114 * img->color_t->rgb[img->data[l]].b) / 255.;
		    }
	    break;

	case pgm:
	    for (i = 0; i < img->height; i++)
                for (j = 0; j < img->width; j++)
                    {
                    k = i * img->width + j;
		    l = (img->height - i - 1) * img->width + j;

		    array[k] = img->data[l] / (float)img->maxgray;
		    }
	    break;

	case pbm:
	    for (i = 0; i < img->height; i++)
                for (j = 0; j < img->width; j++)
                    {
                    k = i * img->width + j;
		    l = (img->height - i - 1) * img->width + j;

		    if (img->data[l>>03] & (01 << (7 - l % 8)))
			array[k] = 0;
		    else
			array[k] = 1;
		    }
	    break;

	default:
	    stat = img__invimgtyp;
	    break;
	}

    if (status != NIL)
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);
}
