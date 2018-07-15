/*
 * Copyright @ 1997   Josef Heinen
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
 */


#if !defined (cray) && !(defined (_WIN32) && !defined (__GNUC__))
#if defined (VMS) || ((defined (hpux) || defined (aix)) && !defined(NAGware))

#define GCONTR gcontr
#define FILL0 fill0
#define DRAW draw 
#define CUBGCV cubgcv
#define GRIDIT gridit
#define GPIXEL	gpixel

#else

#define GCONTR gcontr_
#define FILL0 fill0_
#define DRAW draw_ 
#define CUBGCV cubgcv_
#define GRIDIT gridit_
#define GPIXEL	gpixel_

#endif
#endif /* cray */

#if defined (_WIN32) && !defined (__GNUC__)

extern void __stdcall CUBGCV(double *, double *, double *, int *, double *,
    double *, int *, double *, int *, double *, double *, int *);
extern void __stdcall GCONTR(float *, int *, int *, int *, float *, int *,
    float *, int *, float *, float *, float *, float *,
    void (*)(float *, float *, float *, int *));
extern void __stdcall GRIDIT(int *,float *, float *, float *, int *, int *,
    float *, float *, float *, int *, float *);
extern void __stdcall GPIXEL(float, float, float, float,
    int, int, int *, int, int, int *, int, int *, int *);

#else

#if !defined(VMS) && !defined(cray)

void CUBGCV(double *, double *, double *, int *, double *, double *, int *,
    double *, int *, double *, double *, int *);
void GCONTR(float *, int *, int *, int *, float *, int *, float *, int *,
    float *, float *, float *, float *,
    void (*)(float *, float *, float *, int *));
void GRIDIT(int *,float *, float *, float *, int *, int *, float *, float *,
    float *, int *, float *);
void GPIXEL(float, float, float, float,
    int, int, int *, int, int, int *, int, int *, int *);

#endif

#endif
