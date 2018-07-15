/*
 * Copyright @ 1984 - 1993   Josef Heinen
 *
 * Permission to use, copy, and distribute this software and its
 * documentation for any purpose with or without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.
 *
 * Permission to modify the software is granted, but not the right to
 * distribute the modified code.  Modifications are to be distributed
 * as patches to released version.
 *
 * This software is provided "as is" without express or implied warranty.
 *
 * Send your comments or suggestions to
 *  J.Heinen@kfa-juelich.de.
 *
 *
 * FACILITY:
 *
 *      GLI GKS V4.5
 *
 * ABSTRACT:
 *
 *      This module contains the C environment for the GKS Workstation
 *      Independent Segment Storage (WISS) logical device driver.
 *
 * AUTHOR:
 *
 *      Josef Heinen
 *
 * VERSION:
 *
 *      V1.0
 *
 */


#include <stdlib.h>
#include <stdio.h>

#ifdef VMS
#include <descrip.h>
#endif

#ifdef cray
#include <fortran.h>
#endif

#include "gksdefs.h"

typedef char Char132[132];

#if defined(_WIN32) && !defined(__GNUC__)
#define STDCALL __stdcall
#else
#define STDCALL
#endif


void STDCALL GKCSG (
    int *wkid, int *segn, int *lia, int *lr1, int *lr2, int *lc, int *clear)
{
    int *ia;
    float *r1, *r2;
#ifdef VMS
    struct dsc$descriptor_s chars;
#else
#ifdef cray
    _fcd chars;
#endif
#endif
    Char132 text;

    ia = (int *) malloc (*lia * sizeof (int));
    r1 = (float *) malloc (*lr1 * sizeof (float));
    r2 = (float *) malloc (*lr2 * sizeof (float));
#ifdef VMS
    chars.dsc$b_dtype = DSC$K_DTYPE_T;
    chars.dsc$b_class = DSC$K_CLASS_S;
    chars.dsc$a_pointer = text;
    chars.dsc$w_length = sizeof (Char132);
#else
#ifdef cray
    chars = _cptofcd(text, sizeof (Char132));
#endif
#endif

    if (*clear)
	GCLRWK (wkid, &GALWAY);

#if !defined(VMS) && !defined(cray)
    GKDCSG (wkid, segn, ia, r1, r2, text, 132);
#else
    GKDCSG (wkid, segn, ia, r1, r2, chars);
#endif

    free (r2);
    free (r1);
    free (ia);
}
