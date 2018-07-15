/*
 *
 * FACILITY:
 *
 *	Graphics Language Interpreter (GLI)
 *
 * ABSTRACT:
 *
 *	This module contains the IMAGE system command language
 *	interpreter.
 *
 * AUTHOR:
 *
 *	Jochen Werner, Josef Heinen
 *
 * VERSION:
 *
 *	V1.1
 *
 * MODIFIED BY:
 *
 */


#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#if !defined(VMS) && !defined(MSDOS) && !defined(_WIN32)
#include <unistd.h>
#endif

#include "system.h"
#include "strlib.h"
#include "variable.h"
#include "function.h"
#include "terminal.h"
#include "command.h"
#include "gksdefs.h"
#include "image.h"

#define BOOL			    int
#define BYTE			    unsigned char

#define NIL			    0
#define FALSE			    0
#define TRUE			    1

#define odd(status)		    ((status) & 01)

#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif
#define present(status) ((status) != NIL)


typedef char identifier[32];
typedef char string[256];
typedef char file_specification[80];

typedef enum {
    command_apply_filter, command_autocrop, command_capture, command_contrast,
    command_copy, command_cut, command_delete, command_display, command_edge,
    command_enhance, command_export, command_fft, command_flip, command_gamma,
    command_grad, command_hist, command_import, command_inverse_fft,
    command_invert, command_median, command_norm, command_pbm, command_pcm,
    command_pgm, command_ppm, command_print, command_read, command_rename,
    command_rgb_gamma, command_rotate, command_scale, command_set,
    command_shear, command_show, command_write
    } img_command;
    
typedef enum {
    mode_lowpass, mode_highpass, mode_bandpass, mode_bandstop
    } filter_mode;

typedef enum {
    option_colormap
    } set_option;

typedef enum {
    x_direct,
    y_direct
    } directions;

static cli_verb_list img_command_table = "apply_filter autocrop capture\
 contrast copy cut delete display edge enhance export fft flip gamma gradient\
 histogram import inverse_fft invert median normalize pbm pcm pgm ppm print\
 read rename rgb_gamma rotate scale set shear show write";

static cli_verb_list filter_modes = "lowpass highpass bandpass bandstop";

static cli_verb_list set_options = "colormap";

static cli_verb_list colormaps = "uniform temperature grayscale glowing\
 rainbow geologic greenscale cyanscale bluescale magentascale redscale flame";

static cli_verb_list shear_direction = "XDirection YDirection";

static cli_verb_list flip_direction = "Vertical Horizontal";

static cli_verb_list grad_direction = "XDirection YDirection";

static cli_verb_list option_switch = "Off On";

static cli_verb_list display_option = "Normal Pixel NDC";

static cli_verb_list format_option = "ASCII Binary Compressed";

static file_specification def_spec = "image";


static
void get_file_specification (char *file_spec, char *def_spec, int *stat)

/*
 * get_file_specification - get a file specification
 */

{
    char *result_spec;

    cli_get_parameter ("File", file_spec, " ,", FALSE, TRUE, stat);

    if (odd(*stat))
	{
	result_spec = (char *) getenv (file_spec);
	if (result_spec != NIL)
	    strcpy (file_spec, result_spec);

        if (*file_spec != '|')
	    {
	    str_parse (file_spec, def_spec, FAll, file_spec);

	    if (access (file_spec, 0))
		*stat = RMS__FNF;
	    }
	}
}



static void read_command (int *stat)

{
    string file_spec, imgname;
    image_dscr img;

    strcpy (file_spec, "");
    get_file_specification (file_spec, "", stat);

    if (odd (*stat))
	{
	cli_get_parameter ("Name", imgname, " ,", FALSE, TRUE, stat);

	if (cli_end_of_command ())
	    {
	    if (odd (*stat))
		{
		img_read (file_spec, &img, NULL, stat);
     
		if (odd (*stat))
		    img_define (imgname, &img, stat);
		}
	    }
	else
	    *stat = cli__maxparm;
	}
}



static void write_command (int *stat)

{
    string file_spec, imgname;
    image_dscr img;
    img_format format;

    strcpy (file_spec, "");
    get_file_specification (file_spec, "", stat);

    if (*stat == RMS__FNF)
	*stat = RMS__NORMAL;

    if (odd (*stat))
	{
	cli_get_parameter ("Name", imgname, " ,", FALSE, TRUE, stat);

	if (odd(*stat)) img_translate (imgname, &img, stat);

        if (odd(*stat))
        {
            if (cli_end_of_command ())
	        format = binary;
	    else
                if (odd (*stat))
	            cli_get_keyword ("Format", format_option, (int *)&format,
			stat);

	    if (cli_end_of_command ())
	        {
	        if (odd (*stat))
		    {
		    img_write (file_spec, &img, format, stat);
		    }
	        }
	    else
	        *stat = cli__maxparm;
	    }
        }
}
			    


static void show_command (int *stat)

{
    string name;
    image_dscr img;
    img_image_descr *img_dscr;

    img_dscr = NIL;
    do
	{
	img_dscr = img_inquire_image (img_dscr, name, &img, stat);

	if (*stat != img__noimage)
	    {
	    tt_printf ("  %s (%dx%d pixels, ", name, img.width, img.height);
	    switch (img.type) {
		case ppm:
		    tt_printf ("PPM)\n");
		    break;
		case pcm:
		    tt_printf ("PCM, %d colors)\n", img.color_t->ncolors); 
		    break;
		case pgm:
		    tt_printf ("PGM, %d gray levels)\n", img.maxgray);
		    break;
		case pbm:
		    tt_printf ("PBM)\n");
		    break;
                }
	    }
	}
    while (*stat != img__noimage && *stat != img__nmi);
}



static void delete_command (int *stat)

{
    string imgname;

    cli_get_parameter ("Name", imgname, " ,", FALSE, TRUE, stat);

    if (odd (*stat))
	{
	if (cli_end_of_command ())
	    img_delete (imgname, stat);
	else
	    *stat = cli__maxparm;
	}
}
			    


static void display_command (int *stat)

{
    string imgname;
    image_dscr img;
    display_mode mode;

    cli_get_parameter ("Name", imgname, " ,", FALSE, TRUE, stat);

    if (odd(*stat)) img_translate (imgname, &img, stat);

    if (odd (*stat))
	{
        if (cli_end_of_command ())
	    mode = normal;
	else
	    cli_get_keyword ("Mode", display_option, (int *)&mode, stat);
		    
	if (odd (*stat))
	    if (cli_end_of_command ())
	        {
		img_display (&img, mode, stat);

		strcpy (def_spec, imgname);
		str_parse (def_spec, "", FAll & ~FType & ~FVersion, def_spec);
	        }
	    else
	        *stat = cli__maxparm;
	}
}
			    


static void ppm_command (int *stat)

{
    string source, dest;
    image_dscr source_img, dest_img;

    cli_get_parameter ("Source", source, " ,", FALSE, TRUE, stat);

    if (odd(*stat)) img_translate (source, &source_img, stat);

    if (odd (*stat))
	{
	cli_get_parameter ("Dest", dest, " ,", FALSE, TRUE, stat);

	if (odd (*stat))
	    {
	    if (cli_end_of_command ())
		{     
		img_ppm (&source_img, &dest_img, stat);
		if (odd (*stat))
		    img_define (dest, &dest_img, stat);
		else
		    img_free (&dest_img);
		}
	    else
		*stat = cli__maxparm;
	    }
	}
}
			    


static void pcm_command (int *stat)

{
    string source, dest, colors;
    int ncolors;
    image_dscr source_img, dest_img;

    cli_get_parameter ("Source", source, " ,", FALSE, TRUE, stat);

    if (odd(*stat)) img_translate (source, &source_img, stat);

    if (odd (*stat))
	{
	cli_get_parameter ("Dest", dest, " ,", FALSE, TRUE, stat);

	if (odd (*stat))
	    {
	    if (cli_end_of_command ())
		ncolors = 256;
	    else
		{
		cli_get_parameter ("Colors", colors, " ,", FALSE, TRUE, stat);
		if (odd (*stat))
		    ncolors = cli_integer (colors, stat);
		}

	    if (odd (*stat))
		if (cli_end_of_command ())
		    {
		    img_pcm (&source_img, &dest_img, ncolors, stat);
		    if (odd (*stat))
		        img_define (dest, &dest_img, stat);
		    else
		        img_free (&dest_img);
		    }
		else
		    *stat = cli__maxparm;
	    }
	}
}
			    


static void pgm_command (int *stat)

{
    string source, dest, grays;
    int maxgray;
    image_dscr source_img, dest_img;

    cli_get_parameter ("Source", source, " ,", FALSE, TRUE, stat);

    if (odd(*stat)) img_translate (source, &source_img, stat);

    if (odd (*stat))
	{
	cli_get_parameter ("Dest", dest, " ,", FALSE, TRUE, stat);

	if (odd (*stat))
	    {
	    if (cli_end_of_command ())
		maxgray = 255;
	    else
		{
		cli_get_parameter ("grays", grays, " ,", FALSE, TRUE, stat);
		if (odd (*stat))
		    maxgray = cli_integer (grays, stat);
		}

	    if (odd (*stat))
		if (cli_end_of_command ())
		    {
		    img_pgm (&source_img, &dest_img, maxgray, stat);
		    if (odd (*stat))
			img_define (dest, &dest_img, stat);
		    else
			img_free (&dest_img);
		    }
		else
		    *stat = cli__maxparm;
	    }
	}
}
			    


static void pbm_command (int *stat)

{
    string source, dest, levels;
    double level;
    image_dscr source_img, dest_img;

    cli_get_parameter ("Source", source, " ,", FALSE, TRUE, stat);

    if (odd(*stat)) img_translate (source, &source_img, stat);

    if (odd (*stat))
	{
	cli_get_parameter ("Dest", dest, " ,", FALSE, TRUE, stat);

	if (odd (*stat))
	    {
	    if (cli_end_of_command ())
		level = 0.5;
	    else
		{
		cli_get_parameter ("Level", levels, " ,", FALSE, TRUE, stat);
		if (odd (*stat))
		    level = cli_real (levels, stat);
		}

	    if (odd (*stat))
		if (cli_end_of_command ())
		    {
		    img_pbm (&source_img, &dest_img, level, stat);
		    if (odd (*stat))
			img_define (dest, &dest_img, stat);
		    else
			img_free (&dest_img);
		    }
		else
		    *stat = cli__maxparm;
	    }
	}
}
			    


static void shear_command (int *stat)

{
    string source, dest, angles;
    image_dscr source_img, dest_img;
    int direct;
    double angle;
    BOOL antialias;

    cli_get_keyword ("Direction", shear_direction, &direct, stat);

    if (odd (*stat))
	{
	cli_get_parameter ("Source", source, " ,", FALSE, TRUE, stat);

        if (odd(*stat)) img_translate (source, &source_img, stat);

	if (odd (*stat))
	    {
	    cli_get_parameter ("Dest", dest, " ,", FALSE, TRUE, stat);

	    if (odd (*stat))
		{
		cli_get_parameter ("Angle", angles, " ,", FALSE, TRUE, stat);
		if (odd (*stat))
		    angle = cli_real (angles, stat);

		if (odd (*stat))
		    {
		    if (cli_end_of_command ())
			antialias = FALSE;
		    else
			cli_get_keyword ("Antialias", option_switch, 
			    &antialias, stat);
		    
		    if (odd (*stat))
			if (cli_end_of_command ())
			    {
			    if (direct)
				img_sheary (&source_img, &dest_img, angle,
				    antialias, stat);
			    else
				img_shearx (&source_img, &dest_img, angle, 
				    antialias, stat);

			    if (odd (*stat))
				img_define (dest, &dest_img, stat);
			    else
				img_free (&dest_img);
			    }
			else
			    *stat = cli__maxparm;
		    }
		}
	    }
	}
}
			    


static void rotate_command (int *stat)

{
    string source, dest, angles;
    image_dscr source_img, dest_img;
    double angle;
    BOOL antialias;


    cli_get_parameter ("Source", source, " ,", FALSE, TRUE, stat);

    if (odd(*stat)) img_translate (source, &source_img, stat);

    if (odd (*stat))
	{
	cli_get_parameter ("Dest", dest, " ,", FALSE, TRUE, stat);

	if (odd (*stat))
	    {
	    cli_get_parameter ("Angle", angles, " ,", FALSE, TRUE, stat);
	    if (odd (*stat))
		angle = cli_real (angles, stat);

	    if (odd (*stat))
		{
		if (cli_end_of_command ())
		    antialias = FALSE;
		else
		    cli_get_keyword ("Antialias", option_switch, &antialias, 
			stat);
		
		if (odd (*stat))
		    if (cli_end_of_command ())
			{
		        img_rotate (&source_img, &dest_img, angle, antialias, stat);

			if (odd (*stat))
			    img_define (dest, &dest_img, stat);
			else
			    img_free (&dest_img);
			}
		    else
		        *stat = cli__maxparm;
		}
	    }
	}
}
			    


static void gamma_command (int *stat)

{
    string source, dest, gammas;
    image_dscr source_img, dest_img;
    double gammav;

    cli_get_parameter ("Source", source, " ,", FALSE, TRUE, stat);

    if (odd(*stat)) img_translate (source, &source_img, stat);
 
    if (odd (*stat))
	{
	cli_get_parameter ("Dest", dest, " ,", FALSE, TRUE, stat);

	if (odd (*stat))
	    {
	    cli_get_parameter ("Value", gammas, " ,", FALSE, TRUE, stat);
	    if (odd (*stat))
		gammav = cli_real (gammas, stat);

	    if (odd (*stat))
		if (cli_end_of_command ())
		    {
		    img_gamma (&source_img, &dest_img, gammav, stat);

		    if (odd (*stat))
			img_define (dest, &dest_img, stat);
		    else
			img_free (&dest_img);
		    }
		else
		    *stat = cli__maxparm;
	    }
	}
}
			    


static void invert_command (int *stat)

{
    string source, dest;
    image_dscr source_img, dest_img;

    cli_get_parameter ("Source", source, " ,", FALSE, TRUE, stat);
    if (odd (*stat))
	{
        img_translate (source, &source_img, stat);

        if (odd (*stat))
            {
	    cli_get_parameter ("Dest", dest, " ,", FALSE, TRUE, stat);

	    if (odd (*stat))
                {
	        if (cli_end_of_command ())
		    {
     		    img_invert (&source_img, &dest_img, stat);

		    if (odd (*stat))
			img_define (dest, &dest_img, stat);
		    else
			img_free (&dest_img);
		    }
	        else
		    *stat = cli__maxparm;
                }
	    }
	}
}
			    

static void scale_command (int *stat)

{
    string source, dest;
    string width, height;
    image_dscr source_img, dest_img;
    int new_width, new_height;

    cli_get_parameter ("Source", source, " ,", FALSE, TRUE, stat);

    if (odd(*stat)) img_translate (source, &source_img, stat);

    if (odd (*stat))
	{
	cli_get_parameter ("Dest", dest, " ,", FALSE, TRUE, stat);

	if (odd (*stat))
	    {
	    cli_get_parameter ("Width", width, " ,", FALSE, TRUE, stat);
	    if (odd (*stat))
		new_width = cli_integer (width, stat);

	    if (odd (*stat))
		{
		cli_get_parameter ("Height", height, " ,", FALSE, TRUE, stat);
		if (odd (*stat))
		    new_height = cli_integer (height, stat);
	
		if (odd (*stat))
		    if (cli_end_of_command ())
			{
			img_scale (&source_img, &dest_img, new_width, new_height, 
			    stat);

			if (odd (*stat))
			    img_define (dest, &dest_img, stat);
			else
			    img_free (&dest_img);
			}
		    else
			*stat = cli__maxparm;
		}
	    }
	}
}
			    


static void fft_command (int sign, int *stat)

{
    string source, dest;
    image_dscr source_img, dest_img;

    cli_get_parameter ("Source", source, " ,", FALSE, TRUE, stat);

    if (odd(*stat)) img_translate (source, &source_img, stat);

    if (odd (*stat))
	{
	cli_get_parameter ("Dest", dest, " ,", FALSE, TRUE, stat);

	if (odd (*stat))
	    {
	    if (cli_end_of_command ())
		{
		img_fft (&source_img, &dest_img, sign, stat);
		if (odd (*stat))
		    img_define (dest, &dest_img, stat);
		else
		    img_free (&dest_img);
		}
	    else
	        *stat = cli__maxparm;
	    }
	}
}



static void flip_command (int *stat)

{
    string source, dest;
    image_dscr source_img, dest_img;
    int direct;

    cli_get_parameter ("Source", source, " ,", FALSE, TRUE, stat);

    if (odd(*stat)) img_translate (source, &source_img, stat);

    if (odd (*stat))
	{
	cli_get_parameter ("Dest", dest, " ,", FALSE, TRUE, stat);

	if (odd (*stat))
	    {
	    cli_get_keyword ("Direction", flip_direction, &direct, stat);

	    if (odd (*stat))
		if (cli_end_of_command ())
		    {
	            img_flip (&source_img, &dest_img, (flip_direct)direct,
			stat);

		    if (odd (*stat))
			img_define (dest, &dest_img, stat);
		    else
			img_free (&dest_img);
		    }
		else
		    *stat = cli__maxparm;
	    }
	}
}
			    


static void grad_command (int *stat)

{
    string source, dest;
    image_dscr source_img, dest_img;
    directions direct;


    cli_get_keyword ("Direction", grad_direction, (int *)&direct, stat);

    if (odd (*stat))
	{
        cli_get_parameter ("Source", source, " ,", FALSE, TRUE, stat);

        if (odd(*stat)) img_translate (source, &source_img, stat);

	if (odd (*stat))
	    {
	    cli_get_parameter ("Dest", dest, " ,", FALSE, TRUE, stat);

	    if (odd (*stat))
		if (cli_end_of_command ())
		    {
                    switch (direct)
                        {
                        case x_direct:
                            img_gradx (&source_img, &dest_img, stat);
                            break;

                        case y_direct:
                            img_grady (&source_img, &dest_img, stat);
                            break;
                        }

		    if (odd (*stat))
			img_define (dest, &dest_img, stat);
                    else
			img_free (&dest_img);
		    }
		else
		    *stat = cli__maxparm;
	    }
	}
}
			    


static void cut_command (int *stat)

{
    string source, dest;
    string x_min, x_max, y_min, y_max;
    image_dscr source_img, dest_img;
    int x1, x2, y1, y2;
    float xl, xr, yb, yt;

    cli_get_parameter ("Source", source, " ,", FALSE, TRUE, stat);

    if (odd(*stat)) img_translate (source, &source_img, stat);

    if (odd (*stat))
	{
	cli_get_parameter ("Dest", dest, " ,", FALSE, TRUE, stat);

	if (odd (*stat))
	    {
	    if (!cli_end_of_command ())
		{
		cli_get_parameter ("X-Min", x_min, " ,", FALSE, TRUE, stat);
		if (odd (*stat))
		    x1 = cli_integer (x_min, stat);

		if (odd (*stat))
		    {
		    cli_get_parameter ("X-Max", x_max, " ,", FALSE, TRUE, stat);
		    if (odd (*stat))
			x2 = cli_integer (x_max, stat);
		    }

		if (odd (*stat))
		    {
		    cli_get_parameter ("Y-Min", y_min, " ,", FALSE, TRUE, stat);
		    if (odd (*stat))
			y1 = cli_integer (y_min, stat);
		    }
	
		if (odd (*stat))
		    {
		    cli_get_parameter ("Y-Max", y_max, " ,", FALSE, TRUE, stat);
		    if (odd (*stat))
			y2 = cli_integer (y_max, stat);
		    }
		}
	    else
		{
		ImageRequestRectangle (&xl, &yb, &xr, &yt);

		x1 = (int) (xl * source_img.width + 0.5);
		x2 = (int) (xr * source_img.width + 0.5);
		y1 = (int) ((1 - yb) * source_img.height + 0.5);
		y2 = (int) ((1 - yt) * source_img.height + 0.5);
		}
	    }

	if (odd (*stat))
	    if (cli_end_of_command ())
		{
		img_cut (&source_img, &dest_img, x1, x2, y1, y2, stat);

		if (odd (*stat))
		    img_define (dest, &dest_img, stat);
		else
		    img_free (&dest_img);
		}
	     else
		*stat = cli__maxparm;
	}
}
			    


static void autocrop_command (int *stat)

{
    string source, dest;
    image_dscr source_img, dest_img;

    cli_get_parameter ("Source", source, " ,", FALSE, TRUE, stat);

    if (odd(*stat)) img_translate (source, &source_img, stat);

    if (odd (*stat))
	{
	cli_get_parameter ("Dest", dest, " ,", FALSE, TRUE, stat);
    	
	if (cli_end_of_command ())
	    {
	    img_autocrop (&source_img, &dest_img, stat);
    	    
	    if (odd (*stat))
    		img_define (dest, &dest_img, stat);
	    else
		img_free (&dest_img);
	    }
	else
	    *stat = cli__maxparm;
    	}
        
}



static void edge_command (int *stat)

{
    string source, dest;
    image_dscr source_img, dest_img;

    cli_get_parameter ("Source", source, " ,", FALSE, TRUE, stat);

    if (odd(*stat)) img_translate (source, &source_img, stat);

    if (odd (*stat))
	{
	cli_get_parameter ("Dest", dest, " ,", FALSE, TRUE, stat);

	if (odd (*stat))
	    {
	    if (cli_end_of_command ())
		{
		img_edge (&source_img, &dest_img, stat);
		if (odd (*stat))
		    img_define (dest, &dest_img, stat);
		else
		    img_free (&dest_img);
		}
	    else
	        *stat = cli__maxparm;
	    }
	}
}
			    


static void import_command (int *stat)

{
    string source, dest, xdim;
    image_dscr img;
    float *array;
    var_variable_descr *addr;
    int size, dimx, dimy;

    cli_get_parameter ("Image", source, " ,", FALSE, TRUE, stat);

    if (odd (*stat))
        {
        cli_get_parameter ("Variable", dest, " ,", FALSE, TRUE, stat);

        if (odd(*stat)) addr = var_address (dest, stat);
	
	if (odd (*stat))
	    {
	    cli_get_parameter ("X-Dimension", xdim, " ,", FALSE, TRUE, stat);

	    if (odd (*stat))
		dimx = cli_integer (xdim, stat);

	    if (odd (*stat))
                {
                if (cli_end_of_command())
		    {
		    var_size (dest, &size, NIL);

		    dimy = size / dimx;

		    if (dimy * dimx == size)
			{
			array = (float *) malloc (sizeof (float) * size);
			var_read_variable (addr, size, array, NIL);
	 
			img_import (array, dimx, dimy, &img, stat);

			if (odd (*stat))
			    img_define (source, &img, stat);
			else
			    img_free (&img);

			free (array);
			}
		    else
			*stat = img__invdim;
		}
                else 
	            *stat = cli__maxparm;
		}
	    }
	}
}



static void export_command (int *stat)

{
    string source, dest;
    image_dscr img;
    float *array;
    int ignore;

    cli_get_parameter ("Image", source, " ,", FALSE, TRUE, stat);

    if (odd(*stat)) img_translate (source, &img, stat);

    if (odd (*stat))
	{
	cli_get_parameter ("Variable", dest, " ,", FALSE, TRUE, stat);

	if (odd (*stat))
	    {
	    if (cli_end_of_command ())
		{
		array = (float *) malloc (sizeof (float) * img.width *
		    img.height);

		img_export (array, &img, stat);

		if (odd (*stat))
		    {
		    var_delete (dest, &ignore);
                    var_define (dest, 0, img.width * img.height, array,
                        FALSE, stat);
		    }

		free (array);
		}
                else
                    *stat = cli__maxparm;
	    }
	}
}



static void set_colormap (int *stat)

{
    string name;
    color_tables color_map;
    color_table colort;
    image_dscr img;

    cli_get_parameter ("Image", name, " ,", FALSE, TRUE, stat);

    if (odd(*stat)) img_translate (name, &img, stat);

    if (odd (*stat))
	{
	cli_get_keyword ("Colormap", colormaps, (int *)&color_map, stat);

	if (odd (*stat))
	    if (cli_end_of_command ())
		{
                if (img.type == pcm)
                    {
                    img_get_stdcolor (&colort, color_map,
                        img.color_t->ncolors, stat);

                    if (odd (*stat))
                        img_set_color (&img, &colort, stat);
                    }
                else
                    *stat = img__invimgtyp;
		}
	    else
		*stat = cli__maxparm;
	}
}
			    


void set_command (int *status)

{
    int what;
    set_option option;

    cli_get_keyword ("What", set_options, &what, status);

    option = (set_option)what;

    if (odd (*status))
	{
	switch (option) {

	    case option_colormap:
		set_colormap (status);
		break;
	    }
	}
}



static void copy_command (int *stat)

{
    string source, dest;
    image_dscr source_img, dest_img;

    cli_get_parameter ("Source", source, " ,", FALSE, TRUE, stat);

    if (odd(*stat)) img_translate (source, &source_img, stat);

    if (odd (*stat))
	{
	cli_get_parameter ("Dest", dest, " ,", FALSE, TRUE, stat);

	if (odd (*stat))
	    {
	    if (cli_end_of_command ())
		{
		img_copy (&source_img, &dest_img, stat);
		if (odd (*stat))
		    img_define (dest, &dest_img, stat);
		else
		    img_free (&dest_img);
		}
	    else
	        *stat = cli__maxparm;
	    }
	}
}
			    


static void rename_command (int *stat)

{
    string new_img, old_img;
    image_dscr img;

    cli_get_parameter ("Old", old_img, " ,", FALSE, TRUE, stat);

    if (odd(*stat)) img_translate (old_img, &img, stat);

    if (odd (*stat))
	{
	cli_get_parameter ("New", new_img, " ,", FALSE, TRUE, stat);

	if (odd (*stat))
	    if (cli_end_of_command ())
		img_rename (old_img, new_img, stat);
	else
	    *stat = cli__maxparm;
	}
}
			    


static void rgb_gamma_command (int *stat)

{
    string source, dest, gammar, gammag, gammab;
    image_dscr source_img, dest_img;
    double gammavr, gammavg, gammavb;

    cli_get_parameter ("Source", source, " ,", FALSE, TRUE, stat);

    if (odd(*stat)) img_translate (source, &source_img, stat);

    if (odd (*stat))
	{
	cli_get_parameter ("Dest", dest, " ,", FALSE, TRUE, stat);

	if (odd (*stat))
	    {
	    cli_get_parameter ("R-Value", gammar, " ,", FALSE, TRUE, stat);
	    if (odd (*stat))
		gammavr = cli_real (gammar, stat);
	    }

	if (odd (*stat))
	    {
	    cli_get_parameter ("G-Value", gammag, " ,", FALSE, TRUE, stat);
	    if (odd (*stat))
		gammavg = cli_real (gammag, stat);
            }

	if (odd (*stat))
	    {
	    cli_get_parameter ("B-Value", gammab, " ,", FALSE, TRUE, stat);
	    if (odd (*stat))
		gammavb = cli_real (gammab, stat);
            }

	if (odd (*stat))
	    if (cli_end_of_command ())
		{
		img_rgbgamma (&source_img, &dest_img, gammavr, gammavg, 
                    gammavb, stat);

		if (odd (*stat))
		    img_define (dest, &dest_img, stat);
		else
		    img_free (&dest_img);
		}
	    else
	        *stat = cli__maxparm;
	}
}
			    


static void median_command (int *stat)

{
    string source, dest;
    image_dscr source_img, dest_img;

    cli_get_parameter ("Source", source, " ,", FALSE, TRUE, stat);

    if (odd(*stat)) img_translate (source, &source_img, stat);

    if (odd (*stat))
	{
	cli_get_parameter ("Dest", dest, " ,", FALSE, TRUE, stat);

	if (odd (*stat))
	    {
	    if (cli_end_of_command ())
		{
		img_median (&source_img, &dest_img, stat);
		if (odd (*stat))
		    img_define (dest, &dest_img, stat);
		else
		    img_free (&dest_img);
		}
	    else
	        *stat = cli__maxparm;
	    }
	}
}
			    


static void norm_command (int *stat)

{
    string source, dest;
    image_dscr source_img, dest_img;

    cli_get_parameter ("Source", source, " ,", FALSE, TRUE, stat);

    if (odd(*stat)) img_translate (source, &source_img, stat);

    if (odd (*stat))
	{
	cli_get_parameter ("Dest", dest, " ,", FALSE, TRUE, stat);

	if (odd (*stat))
	    {
	    if (cli_end_of_command ())
		{
		img_norm (&source_img, &dest_img, stat);
		if (odd (*stat))
		    img_define (dest, &dest_img, stat);
		else
		    img_free (&dest_img);
		}
	    else
	        *stat = cli__maxparm;
	    }
	}
}
			    


static void hist_command (int *stat)

{
    string source, dest;
    image_dscr source_img, dest_img;

    cli_get_parameter ("Source", source, " ,", FALSE, TRUE, stat);

    if (odd(*stat)) img_translate (source, &source_img, stat);

    if (odd (*stat))
	{
	cli_get_parameter ("Dest", dest, " ,", FALSE, TRUE, stat);

	if (odd (*stat))
	    {
	    if (cli_end_of_command ())
		{
		img_histnorm (&source_img, &dest_img, stat);
		if (odd (*stat))
		    img_define (dest, &dest_img, stat);
		else
		    img_free (&dest_img);
		}
	    else
	        *stat = cli__maxparm;
	    }
	}
}
			    


static void enhance_command (int *stat)

{
    string source, dest, levels;
    image_dscr source_img, dest_img;
    double levelv;

    cli_get_parameter ("Source", source, " ,", FALSE, TRUE, stat);

    if (odd(*stat)) img_translate (source, &source_img, stat);

    if (odd (*stat))
	{
	cli_get_parameter ("Dest", dest, " ,", FALSE, TRUE, stat);

	if (odd (*stat))
	    {
	    cli_get_parameter ("Level", levels, " ,", FALSE, TRUE, stat);
	    if (odd (*stat))
		levelv = cli_real (levels, stat);

	    if (odd (*stat))
		if (cli_end_of_command ())
		    {
		    img_enhance (&source_img, &dest_img, levelv, stat);

		    if (odd (*stat))
			img_define (dest, &dest_img, stat);
		    else
			img_free (&dest_img);
		    }
		else
		    *stat = cli__maxparm;
	    }
	}
}



static void contrast_command (int *stat)

{
    string source, dest, levels;
    image_dscr source_img, dest_img;
    double levelv;

    cli_get_parameter ("Source", source, " ,", FALSE, TRUE, stat);

    if (odd(*stat)) img_translate (source, &source_img, stat);

    if (odd (*stat))
	{
	cli_get_parameter ("Dest", dest, " ,", FALSE, TRUE, stat);

	if (odd (*stat))
	    {
	    cli_get_parameter ("Level", levels, " ,", FALSE, TRUE, stat);
	    if (odd (*stat))
		levelv = cli_real (levels, stat);

	    if (odd (*stat))
		if (cli_end_of_command ())
		    {
	            img_contrast (&source_img, &dest_img, levelv, stat);

		    if (odd (*stat))
			img_define (dest, &dest_img, stat);
		    else
			img_free (&dest_img);
		    }
		else
		    *stat = cli__maxparm;
	    }
	}
}



static void apply_filter_command (int *stat)

{
    string source, dest, radius;
    image_dscr source_img, dest_img;
    filter_mode mode;
    float radius1, radius2, temp;
    int fac, low, high, level0, level1;

    cli_get_keyword ("Mode", filter_modes, (int *)&mode, stat);

    if (odd (*stat))
	{
        cli_get_parameter ("Source", source, " ,", FALSE, TRUE, stat);

        if (odd (*stat))
	    {
	    img_translate (source, &source_img, stat);

	    if (odd (*stat))
		{
		if (source_img.type != pgm || source_img.coeff == NULL)
		    *stat = img__invimgtyp;
		}
	    }

	if (odd (*stat))
	    {
	    cli_get_parameter ("Dest", dest, " ,", FALSE, TRUE, stat);

	    if (odd (*stat))
		{
		if (cli_end_of_command ())
		    {
		    ImageRequestCircle (&radius1);

		    if (mode == mode_bandpass || mode == mode_bandstop)
			ImageRequestCircle (&radius2);
		    }
		else
		    {
		    cli_get_parameter ("Radius", radius, " ,", FALSE, TRUE,
			stat);
		    if (odd (*stat))
			{
			radius1 = cli_real (radius, stat);

			if (mode == mode_bandpass || mode == mode_bandstop)
			    {
			    cli_get_parameter ("Radius", radius, " ,", FALSE,
				TRUE, stat);
			    if (odd (*stat))
				radius2 = cli_real (radius, stat);
			    }
			}
		    }

		if (odd (*stat))
		    {
		    if (cli_end_of_command ())
			{
			level0 = 0;
			level1 = 1;

			fac = max(source_img.dimx, source_img.dimy);

			switch (mode)
			    {
			    case mode_lowpass:
				low = (int) (radius1 * fac + 0.5);
				high = fac;
				break;

			    case mode_highpass:
				low = 0;
				high = (int) (radius1 * fac + 0.5);
				break;

			    case mode_bandstop:
				level0 = 1;
				level1 = 0;

			    case mode_bandpass:
				if (radius1 > radius2)
				    {
				    temp = radius1;
				    radius1 = radius2;
				    radius2 = temp;
				    }
				low = (int) (radius1 * fac + 0.5);
				high = (int) (radius2 * fac + 0.5);
				break;
			    }

			img_apply_filter (&source_img, &dest_img, low, high,
			    level0, level1, stat);
			}
		    else
			*stat = cli__maxparm;

		    if (odd (*stat))
			img_define (dest, &dest_img, stat);
                    else
			img_free (&dest_img);
		    }
		}
	    }
	}
}
			    


static void capture_command (int *stat)

/*
 * capture_command - create a postscript figure file
 */

{
    file_specification file_spec;

    if (cli_end_of_command())
        {
        strcpy (file_spec, def_spec);
        get_file_specification (file_spec, ".eps", stat);

        if (*stat == RMS__FNF)
            *stat = RMS__NORMAL;

        if (odd(*stat))
            *stat = ImageCapture (def_spec, file_spec, TRUE);
        }
    else
        *stat = cli__maxparm; /* maximum parameter count exceeded */

}  /* capture_command */



static void print_command (int *stat)

/*
 * print_command - print image
 */

{
    file_specification file_spec;
    char *print_spec;
    string command;

    if (cli_end_of_command())
        {
        strcpy (file_spec, def_spec);
        get_file_specification (file_spec, ".ps", stat);

        if (*stat == RMS__FNF)
            *stat = RMS__NORMAL;

        if (odd(*stat)) {
            *stat = ImageCapture (def_spec, file_spec, FALSE);

            if (odd(*stat)) {
#ifndef _WIN32
                print_spec = (char *) getenv ("GLI_LPR");
                if (!print_spec)
#ifdef VMS
                    print_spec = "print/delete";
#else
                    print_spec = "lpr";
#endif
                strcpy (command, print_spec);
                strcat (command, " ");
                strcat (command, file_spec);

                system (command);
#else
                print_spec = (char *) getenv ("PRINTER");
                if (!print_spec)
                    print_spec = "LPT1";

                sprintf (command, "COPY %s %s", file_spec, print_spec);

                system (command);
#endif
                }
            }
        }
    else
        *stat = cli__maxparm; /* maximum parameter count exceeded */

}  /* print_command */



void do_image_command (int *status)

{
    int ind;
    img_command command;

    if (!cli_end_of_command())
	{
	cli_get_keyword ("IMAGE", img_command_table, &ind, status);

	command = (img_command)ind;

	if (odd (*status))
	    {
	    switch (command) {

		case command_read:
		    read_command (status);
		    break;

		case command_write:
		    write_command (status);
		    break;

		case command_show:
		    show_command (status);
		    break;

		case command_delete:
		    delete_command (status);
		    break;

		case command_display:
		    display_command (status);
		    break;

		case command_ppm:
		    ppm_command (status);
		    break;

		case command_pcm:
		    pcm_command (status);
		    break;

		case command_pgm:
		    pgm_command (status);
		    break;

		case command_pbm:
		    pbm_command (status);
		    break;

		case command_shear:
		    shear_command (status);
		    break;

		case command_rotate:
		    rotate_command (status);
		    break;

		case command_gamma:
		    gamma_command (status);
		    break;

		case command_invert:
		    invert_command (status);
		    break;

		case command_scale:
		    scale_command (status);
		    break;

		case command_flip:
		    flip_command (status);
		    break;

		case command_cut:
		    cut_command (status);
		    break;

                case command_autocrop:
                    autocrop_command(status);
                    break;

		case command_edge:
		    edge_command (status);
		    break;

		case command_import:
		    import_command (status);
		    break;

		case command_export:
		    export_command (status);
		    break;

		case command_set:
		    set_command (status);
		    break;

		case command_copy:
		    copy_command (status);
		    break;

		case command_rgb_gamma:
		    rgb_gamma_command (status);
		    break;

		case command_median:
		    median_command (status);
		    break;

		case command_enhance:
		    enhance_command (status);
		    break;

		case command_grad:
		    grad_command (status);
		    break;

		case command_norm:
		    norm_command (status);
		    break;

                case command_hist:
                    hist_command (status);
                    break;

		case command_rename:
		    rename_command (status);
		    break;

		case command_contrast:
		    contrast_command (status);
		    break;

		case command_fft:
		    fft_command (1, status);
		    break;

		case command_apply_filter:
		    apply_filter_command (status);
		    break;

		case command_inverse_fft:
		    fft_command (-1, status);
		    break;

		case command_capture:
		    capture_command (status);
		    break;

		case command_print:
		    print_command (status);
		    break;
		}
	    }
	}
    else
        {
#ifdef MOTIF
	ImageMainLoop ();
#else
	tt_fprintf (stderr, "Can't access OSF/MOTIF software libraries.\n");
#endif
        }
}
