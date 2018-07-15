/*
 *
 * FACILITY:
 *
 *	GLI IMAGE
 *
 * ABSTRACT:
 *
 *	This module contains the basic image storage routines for the GLI
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


#include <stdio.h>
#include <stdlib.h>
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

#define terminator '\0'

extern BOOL image_open;

/* Global data structures */

static img_image_descr *first_image = NIL, *image = NIL;



static
void find_image (char *name, int *status)

/*
 * find_image - lookup image in table
 */

{
    static char letter[] =
	    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    static char alpha[] =
	    "0123456789_$-abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

    int stat;
    char character, *cp;
    int read_index;
    BOOL match;
    img_image_descr *prev_image;

    character = name[0];
    read_index = 1;

    if (strchr(letter, character) != 0)
	{
	character = name[read_index++];
	while (strchr (alpha, character) != 0 && character != terminator)
	    character = name[read_index++];

	if (character == '*')
	    {
	    character = name[read_index++];
	    while (strchr (alpha, character) != 0 && character != terminator)
		character = name[read_index++];
	    }
	if (character == terminator)
	    stat = img__normal;
	else
	    stat = img__invimg; /* Syntax: invalid image */
	}
    else
	/* Syntax: identifier must begin with alphabetic character */
	stat = img__nonalpha;

    if (odd(stat))
	{
	if (strlen(name) <= img_ident_length)
	    {
	    prev_image = NIL;
	    image = first_image;
	    match = FALSE;
	    cp = strchr (name,'*');
	    if (cp != 0)
		*cp = '\0';
	    while (image != NIL && !match)
		{
		prev_image = image;
		match = str_match (name, image->ident, FALSE);
		if (!match)
		    image = image->next;
		}
	    if (cp != 0)
		*cp = '*';
	    if (!match)
		{
		image = prev_image;
		stat = img__undimg; /* undefined image */
		}
	    else
		stat = img__existimg; /* existing image */
	    }
	else
	    stat = img__illimglen; /* image exceeds 31 characters */
	}

    if (status != NIL)
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);
}



void img_define (char *name, image_dscr *img, int *status)

/*
 * define - define an image
 */

{
    int i;
    char id[img_ident_length+1];	
    int stat;
    img_image_descr *prev_image;

    strcpy (id, name);

    for (i = 0; i < strlen (id); i++)
	if (islower(id[i])) id[i] = toupper(id[i]);

    find_image (id, &stat);
    if (stat == img__existimg)
	{
	img_free (&image->image);

	image->image = *img;
	stat = img__supersed;
	}

    else if (stat == img__undimg)
	{
	prev_image = image;

	image = (img_image_descr *) malloc (sizeof (img_image_descr));

	image->prev = prev_image;
	image->next = NIL;
	image->image = *img;

	strcpy (image->ident, id);

	if (prev_image != NIL)
	    prev_image->next = image;
	else
	    first_image = image;

	stat = img__normal;
	}

    if (status != NIL)
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);
}



void img_delete (char *name, int *status)

/*
 * delete - delete an image
 */

{
    int i;
    char id[img_ident_length+1];
    int stat;
    img_image_descr *prev_image;
    img_image_descr *next_image;

    strcpy (id, name);

    for (i = 0; i < strlen (id); i++)
	if (islower(id[i])) id[i] = toupper(id[i]);

    find_image (id, &stat);
    if (stat == img__existimg)
	{
	prev_image = image->prev;
	next_image = image->next;

	img_free (&image->image);

	free (image);

	if (next_image != NIL)
	    next_image->prev = prev_image;
	if (prev_image != NIL)
	    prev_image->next = next_image;
	else
	    first_image = next_image;
	stat = img__normal;
	}

    if (status != NIL)
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);
}



void img_delete_all (int *status)

/*
 * delete - delete all images
 */

{
    int stat, ignore;
    img_image_descr *image, *next_image;

    image = first_image;
    if (image != NIL)
	{
	while (image != NIL)
            {
            next_image = image->next;
            img_delete (image->ident, &ignore);
            image = next_image;
            }
        stat = img__normal;
	}
    else
	stat = img__noimage; /* no images found */

    if (status != NIL)
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);
}



void img_rename (char *old_name, char *new_name, int *status)

/*
 * rename - rename an image
 */

{
    int i;
    char old_id[img_ident_length+1], new_id[img_ident_length+1];
    int stat;

    strcpy (new_id, new_name);
    strcpy (old_id, old_name);

    for (i = 0; i < strlen (new_id); i++)
	if (islower(new_id[i])) new_id[i] = toupper(new_id[i]);
    for (i = 0; i < strlen (old_id); i++)
	if (islower(old_id[i])) old_id[i] = toupper(old_id[i]);

    find_image (new_id, &stat);
    if (stat == img__existimg)
	img_delete (new_id, &stat);

    find_image (old_id, &stat);
    if (stat == img__existimg)
	{
        strcpy (image->ident, new_id);

	stat = img__normal;
	}

    if (status != NIL)
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);
}



void img_translate (char *name, image_dscr *img, int *status)

/*
 * translate - translate image
 */

{
    int i;
    char id[img_ident_length];
    int stat;

    strcpy (id, name);
    for (i = 0; i < strlen (id); i++)
	 if (islower(id[i])) id[i] = toupper(id[i]);

    find_image (id, &stat);
    if (stat == img__existimg) 
	*img = image->image;

    if (status != NIL)
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);
}



img_image_descr *img_inquire_image (img_image_descr *context, char *name,
    image_dscr *img, int *status)

/*
 * inquire_image - inquire image information
 */

{
    int i;
    int stat;
    img_image_descr *image;

    image = context;
    if (image == NIL)
	image = first_image;
    if (image != NIL)
	{
	strcpy (name, image->ident);
	*img = image->image;

	for (i = 0; i < strlen (name); i++)
		if (islower(name[i])) name[i] = toupper(name[i]);

	image = image->next;
	context = image;
	if (image != NIL)
	    stat = img__normal;
	else
	    stat = img__nmi; /* no more images */
	}
    else
	stat = img__noimage; /* no images found */

    if (status != NIL)
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);

    return context;
}



int ImageCapture (char *name, char *filename, int epsf)
{
    int stat;

    if (image_open)
	{
	stat = _ImageOpenFigureFile (filename, epsf);

	if (stat >= 0)
	    {
	    _ImageDisplay (name, &stat);
	    _ImageCloseFigureFile ();
	    }
	else
	    stat = img__openfai;  /* File open failure */
        }
    else
        stat = img__closed;  /* IMAGE not open */

    return (stat);
}
