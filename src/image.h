/*
 *
 * FACILITY:
 *
 *	GLI IMAGE
 *
 * ABSTRACT:
 *
 *	GLI IMAGE Run-Time Library definitions.
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


/* Status code definitions */

#define img__facility	0x0000085D

#define img__normal	0x085D8001	/* normal successful completion */

#define img__nmi	0x085D8323	/* no more images */
#define img__existimg	0x085D832B	/* existing image */
#define img__toppm	0x085D8333	/* promoting to ppm */
#define img__topgm	0x085D833B	/* promoting to pgm */
#define img__supersed	0x085D8343	/* image has been superseded */

#define img__nonalpha	0x085D8962	/* Syntax: identifier must begin with 
					   alphabetic character */
#define img__invimg	0x085D896A	/* Syntax: invalid image */
#define img__illimglen	0x085D8972	/* image name exceeds 31 characters */
#define img__undimg	0x085D897A	/* undefined image */
#define img__noimage	0x085D8982	/* no images found */
#define img__nomem	0x085D898A	/* no more memory */
#define img__corrupted	0x085D8992	/* corrupted image file */
#define img__opefaiinp	0x085D899A	/* can't open image file for reading */ 
#define img__opefaiout	0x085D89A2	/* can't open image file for writing */
#define img__invgraylev	0x085D89AA	/* invalid gray level */
#define img__invmaxgray	0x085D89B2	/* invalid maximum gray value */
#define img__invcollev	0x085D89BA	/* invalid color level */
#define img__invnumcol	0x085D89C2	/* invalid number of colors */
#define img__notimgfil	0x085D89CA	/* not an image file */
#define img__invimgtyp	0x085D89D2	/* invalid image type */
#define img__invmode    0x085D89DA	/* invalid display mode */
#define img__invangle	0x085D89E2	/* invalid angle */
#define img__invgamma	0x085D89EA	/* invalid gamma value */
#define img__invdim	0x085D89F2	/* invalid dimension */
#define img__invcolor	0x085D89FA	/* invalid color table name */
#define img__writerr	0x085D8A02	/* image write error */
#define img__invlevel	0x085D8A0A	/* invalid level for converting PBM 
					   images */
#define img__invdirect	0x085D8A12	/* invalid direction */
#define img__invenhance	0x085D8A1A	/* invalid enhance level */
#define img__invcontr	0x085D8A22	/* invalid contrast level */

#define img__openfai	0x085D8A2A	/* file open failure */ 
#define img__closefai	0x085D8A32	/* file close failure */ 
#define img__closed	0x085D8A3A	/* IMAGE not open */ 

#define img_ident_length	    31


typedef enum {
    pbm, 
    pgm, 
    pcm, 
    ppm
    } image_type;

typedef enum {
    normal, 
    pixel,
    NDC
    } display_mode;

typedef enum {
    uniform, 
    temperature,
    grayscale,
    glowing,
    rainbow,
    geologic,
    greenscale,
    cyanscale,
    bluescale,
    magentascale,
    redscale,
    flame
    } color_tables;    

typedef enum {
    vertical,
    horizontal
    } flip_direct;

typedef enum {
    ascii,
    binary,
    compressed
    } img_format;

typedef enum {
    original,
    fft
    } data_source;
    

typedef struct {
    unsigned long r, g, b; } rgb_color;

typedef struct {
    unsigned long h, l, s; } hls_color;

typedef struct {
    rgb_color rgb[256];
    int ncolors;
    } color_table;

typedef struct {
    image_type type;
    int width;
    int height;
    unsigned char *data;
    color_table *color_t;
    int maxgray;
    int dimx;
    int dimy; 
    float *coeff;
    } image_dscr;

/* Type definitions */

typedef struct img_image_descr_struct {
    struct img_image_descr_struct *prev;
    struct img_image_descr_struct *next;
    char ident[img_ident_length+1];
    image_dscr image;
    } img_image_descr;


/* Entry point definitions */


void img_define (char *, image_dscr *, int *);
void img_delete (char *, int *);
void img_delete_all (int *);
void img_rename (char *, char *, int *);
img_image_descr *img_inquire_image (img_image_descr *, char *, image_dscr *,
    int *);
void img_translate (char *, image_dscr *, int *);
int ImageCapture (char *, char *, int);

void ImageRequestRectangle (float *, float *, float *, float *);
void ImageRequestCircle (float *);
void ImageResize (int, int);
void img_display (image_dscr *, display_mode, int *);
void img_cell_array (image_dscr *, int *, int *, int *, int *, float *, float *,
    float *, float *, int *);
void ImageOpen (int *);
void ImageClose (int *);
int _ImageOpenFigureFile (char *, int);
void _ImageCloseFigureFile (void);
void _ImageDisplay (char *, int *);

void img_free (image_dscr *);
void img_create (image_dscr *, image_type, int, int, int *);
void img_read (char *, image_dscr *, FILE **, int *);
void img_write (char *, image_dscr *, img_format, int *);
void img_colorquant (image_dscr *, image_dscr *, int, int *);
void img_copy (image_dscr *, image_dscr *, int *);
void img_ppm (image_dscr *, image_dscr *, int *);
void img_pcm (image_dscr *, image_dscr *, int, int *);
void img_pgm (image_dscr *, image_dscr *, int, int *);
void img_pbm (image_dscr *, image_dscr *, double, int *);
void img_import (float *, int, int, image_dscr *, int *);
void img_export (float *, image_dscr *, int *);

void img_shearx (image_dscr *, image_dscr *, double, int, int *);
void img_sheary (image_dscr *, image_dscr *, double, int, int *);
void img_rot (image_dscr *, image_dscr *, int, int *);
void img_rotate (image_dscr *, image_dscr *, double, int, int *);
void img_pgmgamma (image_dscr *, image_dscr *, double, int *);
void img_rgbgamma (image_dscr *, image_dscr *, double, double, double, int *);
void img_gamma (image_dscr *, image_dscr *, double, int *);
void img_invert (image_dscr *, image_dscr *, int *);
void img_autocrop (image_dscr *, image_dscr *, int *);
void img_scale (image_dscr *, image_dscr *, int, int, int *);
void img_filter (image_dscr *i, image_dscr *, unsigned char (*)(unsigned char),
    int *);
void img_rgbfilter (image_dscr *, image_dscr *, rgb_color (*)(rgb_color *),
    int *);
void img_hlsfilter (image_dscr *, image_dscr *, hls_color (*)(hls_color *),
    int *);
void img_set_color (image_dscr *, color_table *, int *);
void img_get_color (image_dscr *, color_table *, int *);
void img_get_stdcolor (color_table *, color_tables, int, int *);
void img_flip (image_dscr *, image_dscr *, flip_direct, int *);
void img_cut (image_dscr *, image_dscr *, int, int, int, int, int *);
void img_edge (image_dscr *, image_dscr *, int *);
void img_median (image_dscr *, image_dscr *, int *);
void img_enhance (image_dscr *, image_dscr *, double, int *);
void img_contrast (image_dscr *, image_dscr *, double, int *);
void img_gradx (image_dscr *, image_dscr *, int *);
void img_grady (image_dscr *, image_dscr *, int *);
void img_norm (image_dscr *, image_dscr *, int *);
void img_histnorm (image_dscr *, image_dscr *, int *);
void img_fft (image_dscr *, image_dscr *, int, int *);
void img_apply_filter (image_dscr *, image_dscr *, int, int, int, int, int *);

void ImageMainLoop(void);
