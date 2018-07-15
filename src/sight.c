/*
 *
 * FACILITY:
 *
 *	GLI (Graphics Language Interpreter)
 *
 * ABSTRACT:
 *
 *	This module contains a Simple Interactive Graphics Handling
 *	Tool (Sight).
 *
 * AUTHOR:
 *
 *	Matthias Steinert
 *	Josef Heinen
 *
 * VERSION:
 *
 *	V1.3-00
 *
 * MODIFIED BY:
 * 
 */


#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#ifndef MSDOS
#include <sys/types.h>
#endif

#include "system.h"
#include "gksdefs.h"
#include "gus.h"
#include "strlib.h"
#include "symbol.h"
#include "variable.h"
#include "terminal.h"
#include "image.h"
#include "sight.h"


#define WindowWidth         686	        /* window width */
#define WindowHeight	    485	        /* window height */
#define Ratio               1.41421

#define NominalMarkerSize   6   	/* nominal marker size */
#define CapSize             0.75        /* size of capital letters */
#define MWidth   	    26.94       /* width */

#define EPS		    1.0e-5	

#define odd(status)	    ((status) & 01)
#ifndef min
#define min(x1,x2)	    (((x1)<(x2)) ? (x1) : (x2))
#endif
#ifndef max
#define max(x1,x2)	    (((x1)>(x2)) ? (x1) : (x2))
#endif
#define IN		    &

#define NIL		    0
#define BOOL		    int
#define TRUE		    1
#define FALSE		    0

#define MAX_DISTANCE	0.02
#define MARGIN		0.005
#define LARGE		1.0

#define MAX_SEGMENT_DEPTH 50

#define caddr_t		char *

#define SightCreator		"GLI Sight V4.0"
#define SightVersionID		3



typedef struct _SightHeader {

    char creator[40];			/* name of creator and machine */
    char date[40];			/* date and time of creation */
    int version;			/* program version */
    char filename[80];			/* filename */
    SightOrientation orientation;	/* orientation */
    int records;			/* number of SIF data records */

} SightHeader; 



typedef char String[256];


BOOL sight_open = FALSE;		/* Flags to indicate SIGHT state */
BOOL sight_gui_open = FALSE;
int sight_orientation = OrientationLandscape;

#ifdef TCL
char *sight_info_cb = NULL;
#endif

SightGC gc;				/* Current graphic state of SIGHT */
SightAnyGC DashedGC;
SightAnyGC SnapGC;
SightAnyGC PointGC;
BOOL sight_save_needed = FALSE;
int sight_dpi = 75;

float snap_grid = 0.0;

static int snap_count = 0;
static float *snap_buffer_x = NIL, *snap_buffer_y = NIL;

static SightDisplayList *root = NIL;	/* First element in object list */
static SightDisplayList *last_selected_object = NIL;	
static SightDisplayList *last_created_object = NIL;	
static BOOL select_pending = FALSE;	/* Flag to indicate that a selection is 
					   pending */
static BOOL modify_pending = FALSE;	/* Flag to indicate that a modify 
					   function is possible */
static int modify_index = -1;		/* Index to mark the positon in an
					   object for editing */

static int depth = 0;		    /* Current segment depth */
static float xcut = 0, ycut = 0;    /* Reference position for paste command */


    /* Return strings for inquire function */

static char *SightObjectName[] = { "Polyline", "Spline", "Polymarker", 
    "Error Bar", "Fill Area", "Text", "Circle", "Filled Circle", "Rectangle", 
    "Filled Rectangle", "Axes", "Grid", "Open Hierarchie", "End Segment",
    "Bar Graph", "Simple Text", "Image" };

#define NUM_OBJECTS (sizeof(SightObjectName)/sizeof(SightObjectName[0]))

static char *SightObjectLineType[] = { 
    "Triple Dot", "Double Dot", "Spaced Dot", "Spaced Dash", "Long, short Dash",
    "Long Dash", "Dash, 3 Dots", "Dash, 2 Dots", 
    "", "Solid", "Dashed", "Dotted", "Dash-dotted" };
  
static char *SightObjectMarkerType[] = { 
    "O-Marker", "Hollow Plus", "Solid Triangle right", "Solid Triangle left",
    "Triangle up down", "Solid Star", "Star", "Solid Diamond", "Diamond",
    "Solid Hourglass", "Hourglass", "Solid Bowtie", "Bowtie", "Solid Square",
    "Square", "Solid Triangle down", "Triangle down", "Solid Triangle up",
    "Triangle up", "Solid Circle",
    "", "Dot", "Plus", "Asterisk", "Circle", "Diagonal Cross" };

static char *SightObjectStyle[] = { "Hollow", "Solid", "Pattern", "Hatch" };

static char *SightObjectFont[] = { "ITC Avant Garde Gothic", "Courier",
    "Helvetica", "ITC Lubalin Graph", "New Century Schoolbook", "ITC Souvenir",
    "Symbol", "Times" };

static char *SightObjectFontType[] = { "Normal", "Bold", "Italic", 
    "Bold Italic" };

static char *SightObjectHalign[] = { "Normal", "Left justified", "Centered",
    "Right justified" };

static char *SightObjectValign[] = { "Normal", "Top", "Cap", "Half", "Base",
    "Bottom" };

static char *SightObjectColor[] = { "White", "Black", "Red", "Green", "Blue",
    "Cyan", "Yellow", "Magenta", "n/a" };

static char *SightClipOptions[] = { "Off", "On" };

static char *SightScaleOptions[] = { "Linear", "X-log", "Y-log", "XY-log" };

static char *SightFlipOptions[] = { "None", "X", "Y", "XY" };

static float vp[4] = { 0, 1, 0, 1 };
static float wn[4] = { 0, 1, 0, 1 };



static
SightDisplayList *FindEnd (SightDisplayList *obj)
    {
        SightDisplayList *result;
	int depth;

    result = NIL;

    if (obj != NIL) {
	depth = obj->depth;

	while (obj != NIL)
	    {
	    if (obj->type == EndSegment && obj->depth == depth)
		{
		result = obj;
		obj = NIL;
		}
	    else
		obj = obj->next;
	    }
	}

    return (result);	
    }



static
SightDisplayList *FindBegin (SightDisplayList *obj)
    {
        SightDisplayList *result;
	int depth;

    result = NIL;

    if (obj != NIL) {
	depth = obj->depth-1;

	while (obj != NIL)
	    {
	    if (obj->type == BeginSegment && obj->depth == depth)
		{
		result = obj;
		obj = NIL;
		}
	    else
		obj = obj->prev;
	    }
	}

    return (result);	
    }



static
void RemoveObject (SightDisplayList *obj)
    {
	SightDisplayList *prev, *next;
	
    if (obj != NIL)
	{
	prev = obj->prev;
	next = obj->next;

	if (prev != NIL)
	    prev->next = next;

	if (next != NIL)
	    next->prev = prev;

	obj->prev = NIL;
	obj->next = NIL;

	if (obj == root)
	    root = next;
	}
    }



static
void RemoveSegment (SightDisplayList *obj)
    {
	SightDisplayList *prev, *next, *obj_end;
	
    if (obj != NIL)
	{
	if (obj->type == BeginSegment) {
	    obj_end = FindEnd(obj->next);	

	    prev = obj->prev;
	    next = obj_end->next;

	    if (prev != NIL)
		prev->next = next;

	    if (next != NIL)
		next->prev = prev;

	    obj->prev = NIL;
	    obj_end->next = NIL;

	    if (obj == root)
		root = next;
	    }
	}
    }
    


static
SightDisplayList *LastObject (void)
    {
	SightDisplayList *p;

    if (root == NIL)
	p = NIL;
    else
	{
	p = root;
	while (p->next != NIL)
	    p = p->next; 
	}

    return (p);
    }    



static
void InsertBefore (SightDisplayList *obj, SightDisplayList *pos)
    {	
	SightDisplayList *prev, *obj_end;

    if (obj != NIL) 
	{
	if (root != NIL) 
	    {
	    if (obj->type == BeginSegment)
		obj_end = FindEnd(obj->next);
	    else
		obj_end = obj;

	    if (pos == NIL)
		pos = root;

	    prev = pos->prev;
	    pos->prev = obj_end;
	    
	    if (prev != NIL)
		prev->next = obj;

	    obj->prev = prev;
	    obj_end->next = pos;

	    if (pos == root)
		root = obj;
	    }
	else
	    root = obj;
	}
    }



static
void InsertAfter (SightDisplayList *obj, SightDisplayList *pos)
    {	
	SightDisplayList *next, *obj_end;

    if (obj != NIL) 
	{
	if (root != NIL)
	    {
	    if (obj->type == BeginSegment)
		obj_end = FindEnd(obj->next);
	    else
		obj_end = obj;

	    if (pos == NIL)
		pos = LastObject ();

	    if (pos->type == BeginSegment)
		pos = FindEnd(pos->next);

	    next = pos->next;
	    pos->next = obj;

	    if (next != NIL)
		next->prev = obj_end;

	    obj->prev = pos;
	    obj_end->next = next;
	    }
	else
	    root = obj;
	}
    }



static
SightDisplayList *PrevObject (SightDisplayList *obj)
    {
	SightDisplayList *result;

    result = NIL;

    if (obj != NIL) {
	obj = obj->prev;

	while (obj != NIL) {

	    if (obj->type == EndSegment)
		obj = obj->prev;
	    else 
		{
		result = obj;
		obj = NIL;
		}
	    }
	}

    if (result == NIL)
	result = LastObject ();

    return (result);
    }



static
SightDisplayList *NextObject (SightDisplayList *obj)
    {
	SightDisplayList *result;

    result = NIL;

    if (obj != NIL) {
	obj = obj->next;

	while (obj != NIL) {
	    if (obj->type == EndSegment)
		obj = obj->next;
	    else 
		{
		result = obj;
		obj = NIL;
		}
	    }
	}

    if (result == NIL)
	result = root;

    return (result);
    }


   
static
SightDisplayList *FirstSegmentObject (SightDisplayList *obj)
    {
        SightDisplayList *result;

    result = FindBegin(obj);

    if (result == NIL)
	result = root;
    else
	result = result->next;

    return (result);	
    }



static
SightDisplayList *LastSegmentObject (SightDisplayList *obj)
    {
        SightDisplayList *result;

    result = FindEnd(obj);

    if (result == NIL)
	result = LastObject ();
    else
	result = result->prev;

    return (result);	
    }



static
SightDisplayList *DeleteObject (SightDisplayList *obj)
/* 
 *  DeleteObject - Frees the space of an object and returns the next position.
 */
    {
	SightDisplayList *result;
	int status;

    result = NIL;

    if (obj != NIL)
	{
	if (obj == last_selected_object)
	    last_selected_object = NIL;

	result = obj->next;

	RemoveObject (obj);

	switch (obj->type) {
	    case Pline :
	    case Pmarker :
	    case FillArea :
	    case BarGraph :
		free (obj->obj.data->x);
		free (obj->obj.data->y);
		free (obj->obj.data);
		break;

	    case Spline :
		free (obj->obj.spline->x);
		free (obj->obj.spline->y);
		free (obj->obj.spline);
		break;

	    case ErrorBars :
		free (obj->obj.bar->x);
		free (obj->obj.bar->y);
		free (obj->obj.bar->e1);
		free (obj->obj.bar->e2);
		free (obj->obj.bar);
		break;

	    case SimpleText :
	    case Text :
		free (obj->obj.text->chars);
		free (obj->obj.text);
		break;

	    case Axes :
	    case Grid :
		free (obj->obj.axes);
		break;

    	    case Image :
    	    	img_delete (obj->obj.image->name, &status);
		if (odd(status))
		    free (obj->obj.image->img);
    	    	free (obj->obj.image);
    	    	break;

	    case BeginSegment :
	    case EndSegment :
		break;
	    }

	free (obj);
	}

    return (result);
    }



static
void MinMax (int n, float *x, float *xmin, float *xmax)
/*
 *  MinMax - Calculates the minimum and maximum of a vector of real values.
 */
    {
	int i;

    *xmin = x[0];
    *xmax = x[0];

    for (i = 1; i < n; i++) 
	{
	*xmin = min(*xmin, x[i]);
	*xmax = max(*xmax, x[i]);
	}
    }		



static
void translate_symbol (char *string, char *equ_string)
    {
	int stat;

    if (string[0] == '$')
	{
	sym_translate (&string[1], equ_string, &stat);
	if (stat != sym__normal)
	    strcpy (equ_string, string);
	}
    else
	strcpy (equ_string, string);
    }



int _SightCreateObject (SightObjectType type, caddr_t data)

/*
 *  SightCreateObject - Adds an object to the object list.
 */

    {
 	int len, i, stat;
	SightDisplayList *last, *obj;
	SightPoint *point;
	SightSpline *spline;
	SightBar *bar;
	SightString *text;
	SightAxes *axes;
    	SightImage *image;
	String equ_string;

    obj = (SightDisplayList *) malloc(sizeof(SightDisplayList));

    obj->type = type;
    obj->state = StateNormal;
    obj->tnr = gc.tnr;
    obj->clsw = gc.clsw;
    obj->depth = depth;

    obj->prev = NIL;
    obj->next = NIL;


    switch (type) {

	case Pline :
	case Spline :
	    obj->gc.line = gc.line;
	    break;

	case Pmarker :
	    obj->gc.marker = gc.marker;
	    break;

	case ErrorBars :
	    obj->gc.bar = gc.bar;
	    break;

	case FillArea :
	case BarGraph :
	    obj->gc.fill = gc.fill;
	    break;

	case SimpleText :
	case Text :
	    obj->gc.text = gc.text;
	    break;

	case Axes :
	case Grid :
	    obj->gc.axes = gc.axes;
	    break;
	}

    if (root == NIL) 
	root = obj;
    else
	{
	last = LastObject ();
	obj->prev = last;
	obj->next = NIL;
	last->next = obj;
	}

    switch (type) {

	case Pline :
	case Pmarker :
	case FillArea :
	case BarGraph :
	    point = (SightPoint *)data;

	    obj->obj.data = (SightPoint *) malloc(sizeof(SightPoint));
	    obj->obj.data->n = point->n;
	    obj->obj.data->x = (float *) malloc(point->n*sizeof(float));
	    obj->obj.data->y = (float *) malloc(point->n*sizeof(float));

	    MinMax (point->n, point->x, &obj->xmin, &obj->xmax);
	    MinMax (point->n, point->y, &obj->ymin, &obj->ymax);

	    for (i = 0; i < point->n; i++) {
		obj->obj.data->x[i] = point->x[i];
		obj->obj.data->y[i] = point->y[i];
		}

	    last_selected_object = obj;
	    last_created_object = obj;
	    break;

	case Spline :
	    spline = (SightSpline *)data;

	    obj->obj.spline = (SightSpline *) malloc(sizeof(SightSpline));
	    obj->obj.spline->smooth = spline->smooth;
	    obj->obj.spline->n = spline->n;
	    obj->obj.spline->x = (float *) malloc(spline->n*sizeof(float));
	    obj->obj.spline->y = (float *) malloc(spline->n*sizeof(float));

	    MinMax (spline->n, spline->x, &obj->xmin, &obj->xmax);
	    MinMax (spline->n, spline->y, &obj->ymin, &obj->ymax);

	    for (i = 0; i < spline->n; i++) {
		obj->obj.spline->x[i] = spline->x[i];
		obj->obj.spline->y[i] = spline->y[i];
		}

	    last_selected_object = obj;
	    last_created_object = obj;
	    break;

	case ErrorBars :
	    bar = (SightBar *)data;

	    obj->obj.bar = (SightBar *) malloc(sizeof(SightBar));
	    obj->obj.bar->orientation = bar->orientation;
	    obj->obj.bar->n = bar->n;
	    obj->obj.bar->x = (float *) malloc(bar->n*sizeof(float));
	    obj->obj.bar->y = (float *) malloc(bar->n*sizeof(float));
	    obj->obj.bar->e1 = (float *) malloc(bar->n*sizeof(float));
	    obj->obj.bar->e2 = (float *) malloc(bar->n*sizeof(float));

	    MinMax (bar->n, bar->x, &obj->xmin, &obj->xmax);
	    MinMax (bar->n, bar->y, &obj->ymin, &obj->ymax);

	    for (i = 0; i < bar->n; i++) {
		obj->obj.bar->x[i] = bar->x[i];
		obj->obj.bar->y[i] = bar->y[i];
		obj->obj.bar->e1[i] = bar->e1[i];
		obj->obj.bar->e2[i] = bar->e2[i];
		}

	    last_selected_object = obj;
	    last_created_object = obj;
	    break;

	case SimpleText :
	case Text :
	    text = (SightString *)data;

	    len = strlen(text->chars) + 1;

	    obj->obj.text = (SightString *) malloc(sizeof(SightString));
	    obj->obj.text->chars = (char *) malloc(len*sizeof(char));

	    obj->tnr = SightNDC;

	    _SightApplyXform (gc.tnr, text->x, text->y, &obj->obj.text->x, 
		&obj->obj.text->y);

	    strcpy (obj->obj.text->chars, text->chars);

	    translate_symbol (obj->obj.text->chars, equ_string);

	    {	    SightString text_obj;

		text_obj.x = obj->obj.text->x;
		text_obj.y = obj->obj.text->y;
		text_obj.chars = equ_string;
		_SightInquireTextExtent (&text_obj, (SightTextGC *)&obj->gc,
		    &obj->xmin, &obj->ymin, &obj->xmax, &obj->ymax);
	    }

	    last_selected_object = obj;
	    last_created_object = obj;
	    break;
    
    	case Image :
    	    image = (SightImage *)data;

    	    obj->obj.image = (SightImage *) malloc(sizeof(SightImage));
    	    *(obj->obj.image) = *image;
    
	    obj->tnr = SightNDC;

	    obj->xmin = obj->obj.image->x_min;
	    obj->xmax = obj->obj.image->x_max;
	    obj->ymin = obj->obj.image->y_min;
	    obj->ymax = obj->obj.image->y_max;

	    last_selected_object = obj;
	    last_created_object = obj;
	    break;
    	    
	case Axes :
	case Grid :
	    axes = (SightAxes *)data;

	    obj->xmin = gc.xform[gc.tnr].wn[0];
	    obj->xmax = gc.xform[gc.tnr].wn[1];
	    obj->ymin = gc.xform[gc.tnr].wn[2];
	    obj->ymax = gc.xform[gc.tnr].wn[3];

	    obj->obj.axes = (SightAxes *) malloc(sizeof(SightAxes));
	    *(obj->obj.axes) = *axes;
	    last_selected_object = obj;
	    last_created_object = obj;
	    break;

	case BeginSegment :
	    depth++;
	    break;

	case EndSegment :
	    depth--;
	    break;
	}

    if (obj->type != BeginSegment && obj->type != EndSegment)
        {
        sight_save_needed = TRUE;
	stat = SightRedraw (obj);

	if (!odd(stat)) 
	    DeleteObject (obj);
	}
    else
	stat = SIGHT__NORMAL;

    return (stat);
    }


void SightClear (void)

/*
 *  SightClear - Clears the display and the object list.
 */

    {
    if (sight_open) 
	{
	_SightClearDisplay ();

	while (root != NIL) 
	    root = DeleteObject (root);

	select_pending = FALSE;
	modify_pending = FALSE;
	last_selected_object = NIL;
	last_created_object = NIL;
        sight_save_needed = FALSE;

	_SightRestoreClipping ();
	_SightRestoreXform ();
	_SightRestoreGC ();

	_SightInit ();
	}
    }



static
float PolylineDistance (SightDisplayList *obj, float px, float py)

/*
 *  PolylineDistance - Returns the distance between a point and a polygon.
 */

    {
	int i;
	float x1, y1, x2, y2, l, l1, l2, dx, dy, dx1, dy1, dx2, dy2, cosarg;
	float distance, min_distance;

    min_distance = LARGE;

    x2 = obj->obj.line->x[0]; 
    y2 = obj->obj.line->y[0];

    _SightApplyXform (obj->tnr, x2, y2, &x2, &y2);

    for (i = 1; i < obj->obj.line->n; i++)
	{
	x1 = x2;
	y1 = y2;

	x2 = obj->obj.line->x[i]; 
	y2 = obj->obj.line->y[i];

	_SightApplyXform (obj->tnr, x2, y2, &x2, &y2);

	dx1 = px - x1;
	dy1 = py - y1;
	l1 = sqrt(dx1*dx1 + dy1*dy1);

	dx2 = px - x2;
	dy2 = py - y2;
	l2 = sqrt(dx2*dx2 + dy2*dy2);

	if (l1 < l2)
	    {
	    dx = x2 - x1;
	    dy = y2 - y1;
	    l = sqrt(dx*dx + dy*dy);
	    
	    if (l1 == (float) 0)
		distance = 0;
	    else
		{
		if (l == (float) 0)
		    distance = l1;
		else
		    {
		    cosarg = (dx*dx1 + dy*dy1)/(l1*l);	
		    if (cosarg < 0)
			distance = l1;
		    else
			distance = l1*sin(acos(cosarg));
		    }
		}
	    }
	else
	    {
	    dx = x1 - x2;
	    dy = y1 - y2;
	    l = sqrt(dx*dx + dy*dy);

	    if (l2 == (float) 0)
		distance = 0;
	    else
		{
		if (l == (float) 0)
		    distance = l2;
		else
		    {
		    cosarg = (dx*dx2 + dy*dy2)/(l2*l);	
		    if (cosarg < 0)
			distance = l2;
		    else
			distance = l2*sin(acos(cosarg));
		    }
		}
	    }

	if (distance < min_distance)
	    min_distance = distance;
	}

    return (min_distance);
    }



static
float SplineDistance (SightDisplayList *obj, float px, float py)
    {
	int i;
	float x1, y1, x2, y2, l, l1, l2, dx, dy, dx1, dy1, dx2, dy2, cosarg;
	float distance, min_distance;

    min_distance = LARGE;

    x2 = obj->obj.spline->x[0]; 
    y2 = obj->obj.spline->y[0];

    _SightApplyXform (obj->tnr, x2, y2, &x2, &y2);

    for (i = 1; i < obj->obj.spline->n; i++)
	{
	x1 = x2;
	y1 = y2;

	x2 = obj->obj.spline->x[i]; 
	y2 = obj->obj.spline->y[i];

	_SightApplyXform (obj->tnr, x2, y2, &x2, &y2);

	dx1 = px - x1;
	dy1 = py - y1;
	l1 = sqrt(dx1*dx1 + dy1*dy1);

	dx2 = px - x2;
	dy2 = py - y2;
	l2 = sqrt(dx2*dx2 + dy2*dy2);

	if (l1 < l2)
	    {
	    dx = x2 - x1;
	    dy = y2 - y1;
	    l = sqrt(dx*dx + dy*dy);
	    
	    if (l1 == (float) 0)
		distance = 0;
	    else
		{
		if (l == (float) 0)
		    distance = l1;
		else
		    {
		    cosarg = (dx*dx1 + dy*dy1)/(l1*l);	
		    if (cosarg < 0)
			distance = l1;
		    else
			distance = l1*sin(acos(cosarg));
		    }
		}
	    }
	else
	    {
	    dx = x1 - x2;
	    dy = y1 - y2;
	    l = sqrt(dx*dx + dy*dy);

	    if (l2 == (float) 0)
		distance = 0;
	    else
		{
		if (l == (float) 0)
		    distance = l2;
		else
		    {
		    cosarg = (dx*dx2 + dy*dy2)/(l2*l);	
		    if (cosarg < 0)
			distance = l2;
		    else
			distance = l2*sin(acos(cosarg));
		    }
		}
	    }

	if (distance < min_distance)
	    min_distance = distance;
	}

    return (min_distance);
    }



static
float PolymarkerDistance (SightDisplayList *obj, float px, float py)
    {
	int i;
	float x, y, dx, dy, distance, min_distance;

    min_distance = LARGE;

    for (i = 0; i < obj->obj.marker->n; i++)
	{
	x = obj->obj.marker->x[i];
	y = obj->obj.marker->y[i];

	_SightApplyXform (obj->tnr, x, y, &x, &y);

	dx = px - x;
	dy = py - y;

	distance = sqrt(dx*dx + dy*dy);
	if (distance < min_distance)
	    min_distance = distance;
	
	}

    return (min_distance);
    }



static
float ErrorBarsDistance (SightDisplayList *obj, float px, float py)
    {
	int i;
	float x, y, dx, dy, distance, min_distance;

    min_distance = LARGE;

    for (i = 0; i < obj->obj.bar->n; i++)
	{
	x = obj->obj.bar->x[i];
	y = obj->obj.bar->y[i];

	_SightApplyXform (obj->tnr, x, y, &x, &y);

	dx = px - x;
	dy = py - y;

	distance = sqrt(dx*dx + dy*dy);
	if (distance < min_distance)
	    min_distance = distance;
	
	}

    return (min_distance);
    }



static
float AxesDistance (SightDisplayList *obj, float px, float py)
    {
	float *wn, x_org_ur, y_org_ur;
	float x, y, dist, dx, dy;
    	float x2, y2, dx2, dy2;

    _SightApplyXform (obj->tnr, obj->obj.axes->x_org, obj->obj.axes->y_org,
	&x, &y);

    wn = gc.xform[obj->tnr].wn;
    x_org_ur = wn[1];
    y_org_ur = wn[3];

    _SightApplyXform (obj->tnr, x_org_ur, y_org_ur, &x2, &y2);

    dx = fabs(px-x);
    dy = fabs(py-y);
   
    dx2 = fabs(px-x2);
    dy2 = fabs(py-y2);

    if (dx2 < dx)
    	dx = dx2;

    if (dy2 < dy)
    	dy = dy2; 

    
    if (dx < MAX_DISTANCE) {
	if (dy < dx)
	    dist = dy;
	else
	    dist = dx;
	}
    else {
	if (dy < MAX_DISTANCE)
	    dist = dy;
	else
	    dist = LARGE;
	}
    
    return (dist);
    }



static
float GridDistance (SightDisplayList *obj, float px, float py)
    {
    	float x, y, dist, dx, dx_new, dy, dy_new, x_line, y_line;

    dx = LARGE;
    for (x_line = obj->obj.axes->x_org; x_line <= obj->obj.axes->maj_x; 
    	 x_line += obj->obj.axes->x_tick) 
    	{
        _SightApplyXform (obj->tnr, x_line, obj->obj.axes->y_org, &x, &y);
    	    
	dx_new = fabs(px - x);
    	
    	if (dx_new < dx)
    	    dx = dx_new;
    	}

    dy = LARGE;
    for (y_line = obj->obj.axes->y_org; y_line <= obj->obj.axes->maj_y; 
    	 y_line += obj->obj.axes->y_tick) 
    	{
        _SightApplyXform (obj->tnr, obj->obj.axes->x_org, y_line, &x, &y);
    	    
	dy_new = fabs(py - y);
    	
    	if (dy_new < dy)
    	    dy = dy_new;
    	}
    
    if (dx < MAX_DISTANCE) {
	if (dy < dx)
	    dist = dy;
	else
	    dist = dx;
	}
    else {
	if (dy < MAX_DISTANCE)
	    dist = dy;
	else
	    dist = LARGE;
	}

    return(dist);
    }



static
float ImageDistance (SightDisplayList *obj, float px, float py)
    {
    	float distance, dx, dy, x, y;
    	float dx2, dy2, x2, y2;

    _SightApplyXform(obj->tnr, obj->obj.image->x_min, obj->obj.image->y_min,
		     &x, &y);

    _SightApplyXform(obj->tnr, obj->obj.image->x_max, obj->obj.image->y_max,
    		     &x2, &y2);

    if (px > x && px < x2 && py > y && py < y2) 
    	return (0); 
    else
    	{
    	dx = fabs(px-x);
    	dy = fabs(py-y);
   
        dx2 = fabs(px-x2);
        dy2 = fabs(py-y2);

        if (dx2 < dx)
    	    dx = dx2;

    	if (dy2 < dy)
    	    dy = dy2; 
    
    	if (dx < MAX_DISTANCE) {
	    if (dy < dx)
	    	distance = dy;
	    else
	    	distance = dx;
	}
    	else {
	    if (dy < MAX_DISTANCE)
	    	distance = dy;
	    else
	    	distance = LARGE;
	}

    	return(distance);
    	}
    }



static
void SetGCValue (SightAttribute type, SightAnyGC *gc, caddr_t value)
    {
    switch (type) {
	case LineType : gc->line.linetype = *(int *)value; break;
	case LineWidth : gc->line.linewidth = *(float *)value; break;
	case LineColor : gc->line.linecolor = *(int *)value; break;
	case MarkerType : gc->marker.markertype = *(int *)value; break;
	case MarkerSize : gc->marker.markersize = *(float *)value; break;
	case MarkerColor : gc->marker.markercolor = *(int *)value; break;
	case BarLineWidth : gc->bar.linewidth = *(float *)value; break;
	case BarLineColor : gc->bar.linecolor = *(int *)value; break;
	case BarMarkerType : gc->bar.markertype = *(int *)value; break;
	case BarMarkerSize : gc->bar.markersize = *(float *)value; break;
	case BarMarkerColor : gc->bar.markercolor = *(int *)value; break;
	case FillStyle : gc->fill.fillstyle = *(int *)value; break;
	case FillIndex : gc->fill.fillindex = *(int *)value; break;
	case FillColor : gc->fill.fillcolor = *(int *)value; break;
	case TextFont : gc->text.textfont = *(int *)value; break;
	case TextSize : gc->text.textsize = *(float *)value; break;
	case TextHalign : gc->text.texthalign = *(int *)value; break;
	case TextValign : gc->text.textvalign = *(int *)value; break;
	case TextDirection : gc->text.textdirection = *(int *)value; break;
	case TextColor : gc->text.textcolor = *(int *)value; break;
	case AxesLineWidth : gc->axes.linewidth = *(float *)value; break;
	case AxesLineColor : gc->axes.linecolor = *(int *)value; break;
	case AxesTextFont : gc->axes.textfont = *(int *)value; break;
	case AxesTextSize : gc->axes.textsize = *(float *)value; break;
	case AxesTextColor : gc->axes.textcolor = *(int *)value; break;
	}
    }



static
caddr_t InquireGCValue (SightAttribute type, SightAnyGC *gc)
    {
	caddr_t result;

    switch (type) {
	case LineType : result = (caddr_t)&gc->line.linetype; break;
	case LineWidth : result = (caddr_t)&gc->line.linewidth; break;
	case LineColor : result = (caddr_t)&gc->line.linecolor; break;
	case MarkerType : result = (caddr_t)&gc->marker.markertype; break;
	case MarkerSize : result = (caddr_t)&gc->marker.markersize; break;
	case MarkerColor : result = (caddr_t)&gc->marker.markercolor; break;
	case BarLineWidth : result = (caddr_t)&gc->bar.linewidth; break;
	case BarLineColor : result = (caddr_t)&gc->bar.linecolor; break;
	case BarMarkerType : result = (caddr_t)&gc->bar.markertype; break;
	case BarMarkerSize : result = (caddr_t)&gc->bar.markersize; break; 
	case BarMarkerColor : result = (caddr_t)&gc->bar.markercolor; break;
	case FillStyle : result = (caddr_t)&gc->fill.fillstyle; break;
	case FillIndex : result = (caddr_t)&gc->fill.fillindex; break;
	case FillColor : result = (caddr_t)&gc->fill.fillcolor; break;
	case TextFont : result = (caddr_t)&gc->text.textfont; break;
	case TextSize : result = (caddr_t)&gc->text.textsize; break;
	case TextHalign : result = (caddr_t)&gc->text.texthalign; break;
	case TextValign : result = (caddr_t)&gc->text.textvalign; break;
	case TextDirection : result = (caddr_t)&gc->text.textdirection; break;
	case TextColor : result = (caddr_t)&gc->text.textcolor; break;
	case AxesLineWidth : result = (caddr_t)&gc->axes.linewidth; break;
	case AxesLineColor : result = (caddr_t)&gc->axes.linecolor; break;
	case AxesTextFont : result = (caddr_t)&gc->axes.textfont; break;
	case AxesTextSize : result = (caddr_t)&gc->axes.textsize; break;
	case AxesTextColor : result = (caddr_t)&gc->axes.textcolor; break;
	}

    return (result);
    }



static
BOOL type_compare (SightObjectType obj_type, SightAttribute attr_type)
    {
	BOOL result;

    result = FALSE;

    switch (obj_type) {
	case Pline :
	case Spline :
	    result = (attr_type == LineType || attr_type == LineWidth || 
		attr_type == LineColor);
	    break;
	case Pmarker :
	    result = (attr_type == MarkerType || attr_type == MarkerSize || 
		attr_type == MarkerColor);
	    break;
	case ErrorBars :
	    result = (attr_type == BarLineWidth || attr_type == BarLineColor || 
		attr_type == BarMarkerType || attr_type == BarMarkerSize || 
		attr_type == BarMarkerColor);
	    break;
	case FillArea :
	case BarGraph :
	    result = (attr_type == FillStyle || attr_type == FillColor ||
		attr_type == FillIndex);
	    break;
	case SimpleText :
	case Text :
	    result = (attr_type == TextFont || attr_type == TextSize || 
		attr_type == TextHalign || attr_type == TextValign || 
		attr_type == TextDirection || attr_type == TextColor);
	    break;
	case Axes :
	case Grid :
	    result = (attr_type == AxesLineWidth ||
		attr_type == AxesLineColor || attr_type == AxesTextFont ||
		attr_type == AxesTextSize || attr_type == AxesTextColor);
	    break;
	}

    return (result);
    }



static
int ChangeGC (SightAttribute type, caddr_t gc_value)
    {
	SightDisplayList *p;
	int n;
	BOOL text_update;
	String equ_string;
	caddr_t result;

    n = 0;

    if (select_pending)
	{
	text_update = (type == TextFont || type == TextSize || 
	    type == TextHalign || type == TextValign || type == TextDirection);

	p = root;
	while (p != NIL) {

	    if (p->state == StateSelected && type_compare(p->type, type)) 
		{	
		result = InquireGCValue(type, &p->gc);

		if (*(unsigned *)result != *(unsigned *)gc_value) 
		    {
		    SetGCValue (type, &p->gc, gc_value);
		    n++;
		    
		    if (text_update) 
			{	SightString text_obj;	
			translate_symbol (p->obj.text->chars, equ_string);
			text_obj.x = p->obj.text->x;
			text_obj.y = p->obj.text->y;
			text_obj.chars = equ_string;
			_SightInquireTextExtent (&text_obj,
			    (SightTextGC *)&p->gc, &p->xmin, &p->ymin,
			    &p->xmax, &p->ymax);
			}
		    }
		}
	    p = p->next;
	    }
	}

    return (n); /* number of changes */
    }



void SightDelete (void)
    {
	SightDisplayList *p;

    if (!sight_open)
	return;

    if (select_pending) {
	p = root;
	while (p != NIL)
	    {
	    if (p->state == StateSelected) {
		p->state = StateDeleted;
		if (last_created_object == p)
		    last_created_object = NIL;
		}
	    p = p->next;
	    }
	select_pending = FALSE;
	modify_pending = FALSE;
	last_selected_object = NIL;

	if (gc.update == UpdateAlways)
	    SightRedraw (NIL);
	}
    }



void SightDeselect (void)
    {
	SightDisplayList *p;

    if (!sight_open)
	return;

    if (select_pending) {
	p = root;
	while (p != NIL)
	    {
	    if (p->state == StateSelected)
		p->state = StateNormal;
	    p = p->next;
	    }
	select_pending = FALSE;
	modify_pending = FALSE;
	last_selected_object = NIL;

	if (gc.update == UpdateAlways)
	    SightRedraw (NIL);
	}
    }



int SightSelectAll (void)
    {
	SightDisplayList *p;
	int stat;


    if (sight_open)
	{
	if (depth == 0) {
	    p = root;
	    while (p != NIL)
		{
		if (p->state == StateNormal)
		    p->state = StateSelected;
		p = p->next;
		}
	    select_pending = TRUE;
	    modify_pending = FALSE;

	    p = LastObject ();
	    while (p != NIL) {
		if (p->state == StateSelected) {
		    last_selected_object = p;
		    p = NIL;
		    }
		else
		    p = p->prev;
		}
	    if (last_selected_object != NIL)
		if (last_selected_object->type == EndSegment)
		    last_selected_object =
                        FindBegin(last_selected_object->prev);

	    if (gc.update == UpdateAlways)
		SightRedraw (NIL);
	    
	    stat = SIGHT__NORMAL;  /* Successful completion */
	    }
	else
	    stat = SIGHT__SEGOPE;  /* Segment not closed */
	}
    else
	stat = SIGHT__CLOSED;  /* Sight not open */

    return (stat);
    }



static
SightDisplayList *NearestObject (float x_ndc, float y_ndc)
    {
	static int priority[NUM_OBJECTS] = {
	    4, 3, 1, 2, 5, 8, 10, 10, 10, 10, 6, 7, 10, 10, 10, 8, 9 };

	float	xmin, xmax, ymin, ymax, tmp;
	float	distance;
	SightDisplayList *p, *result;

    result = NIL;

    _SightSaveXform ();

    p = root;    
    while (p != NIL)
	{
	if (p->state != StateDeleted && p->state != StateCut) {
	    distance = LARGE;

	    /* update window limits */
	    if (p->type == Axes || p->type == Grid) {
		p->xmin = gc.xform[p->tnr].wn[0];
		p->xmax = gc.xform[p->tnr].wn[1];
		p->ymin = gc.xform[p->tnr].wn[2];
		p->ymax = gc.xform[p->tnr].wn[3];
		}

	    _SightApplyXform (p->tnr, p->xmin, p->ymin, &xmin, &ymin);
	    _SightApplyXform (p->tnr, p->xmax, p->ymax, &xmax, &ymax);
 
	    if (option_flip_x & gc.xform[p->tnr].scale)
		{
		tmp = xmax; xmax = xmin; xmin = tmp;
		}
	    if (option_flip_y & gc.xform[p->tnr].scale)
		{
		tmp = ymax; ymax = ymin; ymin = tmp;
		}

	    if (x_ndc <= (xmax+MAX_DISTANCE) && x_ndc >= (xmin-MAX_DISTANCE) &&
		y_ndc <= (ymax+MAX_DISTANCE) && y_ndc >= (ymin-MAX_DISTANCE)) {
		switch (p->type) {
		    case Pline :
			distance = PolylineDistance(p, x_ndc, y_ndc);
			break;
		    case Spline :
			distance = SplineDistance(p, x_ndc, y_ndc);
			break;
		    case Pmarker :
			distance = PolymarkerDistance(p, x_ndc, y_ndc);
			break;
		    case ErrorBars :
			distance = ErrorBarsDistance(p, x_ndc, y_ndc);
			break;
		    case FillArea :
		    case BarGraph :
			distance = PolylineDistance(p, x_ndc, y_ndc);
			break;
		    case SimpleText :
		    case Text :
			distance = 0;
			break;
		    case Axes :
			distance = AxesDistance(p, x_ndc, y_ndc);
			break;
		    case Grid :
			distance = GridDistance(p, x_ndc, y_ndc);
			break;
    	    	    case Image :
			distance = ImageDistance(p, x_ndc, y_ndc);
			break;
		    }
		}
	    

	    if (distance < MAX_DISTANCE)
		{
		if (result == NIL) 
		    {
		    result = p;
		    }
		else
		    { 
		    if (priority[(int)p->type] <= priority[(int)result->type]) 
			{
			result = p;
			}
		    }
		}
	    }
	p = p->next;
	}

    _SightRestoreXform ();
    
    return (result);
    }



void SightDefineXY (int n, float *x, float *y)
    {
	int i, ignore;

    _SightSaveXform ();

    for (i=0; i<n; i++) {
	x[i] = gc.xform[gc.tnr].vp[0] + x[i]*(gc.xform[gc.tnr].vp[1]-
	    gc.xform[gc.tnr].vp[0]);
	y[i] = gc.xform[gc.tnr].vp[2] + y[i]*(gc.xform[gc.tnr].vp[3]-
	    gc.xform[gc.tnr].vp[2]);
	_SightApplyInverseXform (gc.tnr, x[i], y[i], &x[i], &y[i]);
	}

    _SightRestoreXform ();

    var_delete ("X", &ignore);
    var_delete ("Y", &ignore);
    var_define ("X", 0, n, x, FALSE, NIL);
    var_define ("Y", 0, n, y, FALSE, NIL); 
    }



void SightDefineX (char *name, int n, float *x)
    {
	int i, ignore;
	float y;

    _SightSaveXform ();

    y = 0;
    for (i=0; i<n; i++) {
	x[i] = gc.xform[gc.tnr].vp[0] + x[i]*(gc.xform[gc.tnr].vp[1]-
	    gc.xform[gc.tnr].vp[0]);
	_SightApplyInverseXform (gc.tnr, x[i], y, &x[i], &y);
	}

    _SightRestoreXform ();

    var_delete (name, &ignore);
    var_define (name, 0, n, x, FALSE, NIL); 
    }



void SightDefineY (char *name, int n, float *y)
    {
	int i, ignore;
	float x;

    _SightSaveXform ();

    x = 0;
    for (i=0; i<n; i++) {
	y[i] = gc.xform[gc.tnr].vp[2] + y[i]*(gc.xform[gc.tnr].vp[3]-
	    gc.xform[gc.tnr].vp[2]);
	_SightApplyInverseXform (gc.tnr, x, y[i], &x, &y[i]);
	}

    _SightRestoreXform ();

    var_delete (name, &ignore);
    var_define (name, 0, n, y, FALSE, NIL); 
    }



static
BOOL CheckInside (
    float x, float y, float xmin, float xmax, float ymin, float ymax)
    {
    return ((x >= xmin) && (x <= xmax) && (y >= ymin) && (y <= ymax));
    }



void SightPickRegion (float xmin, float xmax, float ymin, float ymax)
    {
	SightDisplayList *p;
	BOOL inside;
	int np[3], ni;
	int i, n_pick;
	int ignore;
	float x, y;
	String pick_nr, var_x, var_y, str_x, str_y;

    n_pick = 0;

    p = root;
    while (p != NIL) 
	{
	if (p->state == StateSelected)
	    {
	    switch (p->type) {
		case Pline :
		case Pmarker :
		case FillArea :
		case BarGraph :
		    inside = FALSE;
		    
		    ni = 0;
		    np[0] = 0;
		    np[1] = 0;
		    np[2] = 0;
		    i = 0;
		    while (i < p->obj.data->n && ni <= 2)
			{
			_SightApplyXform (p->tnr, p->obj.data->x[i], 
			    p->obj.data->y[i], &x, &y);
			if (inside != CheckInside(x, y, xmin, xmax, ymin, ymax))
			    {
			    inside = !inside;
			    ni++;
			    }
			else
			    {
			    np[ni] = np[ni] + 1;
			    i++;
			    }
			}

		    if (ni <= 2)
			{
			n_pick++;

			str_dec (pick_nr, n_pick);

			strcpy (str_x, "X");
			strcat (str_x, pick_nr);

			strcpy (str_y, "Y");
			strcat (str_y, pick_nr);

			if (np[0])
			    {
			    /* left part of picked region */

			    strcpy (var_x, str_x);
			    strcat (var_x, "L");

			    var_delete (var_x, &ignore);
			    var_define (var_x, 0, np[0], p->obj.data->x, 
				FALSE, NIL);

			    strcpy (var_y, str_y);
			    strcat (var_y, "L");

			    var_delete (var_y, &ignore);
			    var_define (var_y, 0, np[0], p->obj.data->y, 
				FALSE, NIL);
			    }

			if (np[1])
			    {
			    /* middle part of picked region */

			    var_delete (str_x, &ignore);
			    var_define (str_x, 0, np[1], 
				&p->obj.data->x[np[0]], FALSE, NIL);

			    var_delete (str_y, &ignore);
			    var_define (str_y, 0, np[1], 
				&p->obj.data->y[np[0]], FALSE, NIL);
			    }

			if (np[2])
			    {
			    /* right part of picked region */

			    strcpy (var_x, str_x);
			    strcat (var_x, "R");

			    var_delete (var_x, &ignore);
			    var_define (var_x, 0, np[2], 
				&p->obj.data->x[np[0]+np[1]], FALSE, NIL);

			    strcpy (var_y, str_y);
			    strcat (var_y, "R");

			    var_delete (var_y, &ignore);
			    var_define (var_y, 0, np[2],
				&p->obj.data->y[np[0]+np[1]], FALSE, NIL);
			    }
			}
		    break;
		}
	    }

	p = p->next;
	}
    }



void SightPickObject (float x_ndc, float y_ndc)
    {
	SightDisplayList *p;
	BOOL redraw;
	int ignore;
	String str;
	SightUpdateOption update;

    p = NearestObject(x_ndc, y_ndc);

    if (p != NIL) 
	{
	update = gc.update;
	gc.update = UpdateConditionally;
	SightDeselect ();

	last_selected_object = p;

	switch (p->type) {
	    case Pline :
	    case Pmarker :
	    case FillArea :
	    case BarGraph :
		var_delete ("X", &ignore);
		var_define ("X", 0, p->obj.data->n, p->obj.data->x, FALSE, NIL);

		var_delete ("Y", &ignore);
		var_define ("Y", 0, p->obj.data->n, p->obj.data->y, FALSE, NIL);

		if (p->state != StateSelected) {
		    p->state = StateSelected;
		    redraw = TRUE;
		    select_pending = TRUE;
		    modify_pending = TRUE;
		    modify_index = -1;
		    }
		break;

	    case Spline :
		var_delete ("X", &ignore);
		var_define ("X", 0, p->obj.spline->n, p->obj.spline->x, FALSE,
		    NIL);

		var_delete ("Y", &ignore);
		var_define ("Y", 0, p->obj.spline->n, p->obj.spline->y, FALSE,
		    NIL);

		if (p->state != StateSelected) {
		    p->state = StateSelected;
		    redraw = TRUE;
		    select_pending = TRUE;
		    modify_pending = TRUE;
		    modify_index = -1;
		    }
		break;

	    case ErrorBars :
		var_delete ("X", &ignore);
		var_define ("X", 0, p->obj.bar->n, p->obj.bar->x, FALSE, NIL);

		var_delete ("Y", &ignore);
		var_define ("Y", 0, p->obj.bar->n, p->obj.bar->y, FALSE, NIL);

		var_delete ("E1", &ignore);
		var_define ("E1", 0, p->obj.bar->n, p->obj.bar->e1, FALSE, NIL);

		var_delete ("E2", &ignore);
		var_define ("E2", 0, p->obj.bar->n, p->obj.bar->e2, FALSE, NIL);

		if (p->state != StateSelected) {
		    p->state = StateSelected;
		    redraw = TRUE;
		    select_pending = TRUE;
		    modify_pending = FALSE;
		    }
		break;

	    case SimpleText :
	    case Text :
		var_delete ("X", &ignore);
		var_define ("X", 0, 1, &p->obj.text->x, FALSE, NIL);

		var_delete ("Y", &ignore);
		var_define ("Y", 0, 1, &p->obj.text->y, FALSE, NIL);

		sym_define ("TEXT", p->obj.text->chars, NIL);

		if (p->state != StateSelected) {
		    p->state = StateSelected;
		    redraw = TRUE;
		    select_pending = TRUE;
		    modify_pending = TRUE;
		    modify_index = -1;
		    }
		break;

	    case Axes :
		str_flt (str, p->obj.axes->x_tick);
		sym_define ("X_TICK", str, NIL);

		str_flt (str, p->obj.axes->y_tick);
		sym_define ("Y_TICK", str, NIL);

		str_flt (str, p->obj.axes->x_org);
		sym_define ("X_ORIGIN", str, NIL);

		str_flt (str, p->obj.axes->y_org);
		sym_define ("Y_ORIGIN", str, NIL);

		str_dec (str, p->obj.axes->maj_x);
		sym_define ("MAJOR_X_COUNT", str, NIL);

		str_dec (str, p->obj.axes->maj_y);
		sym_define ("MAJOR_Y_COUNT", str, NIL);

		str_flt (str, p->obj.axes->tick_size);
		sym_define ("TICK_SIZE", str, NIL);

		if (p->state != StateSelected) {
		    p->state = StateSelected;
		    redraw = TRUE;
		    select_pending = TRUE;
		    modify_pending = FALSE;
		    }
		break;
	    }

	gc.update = update;
	}
	
    if (redraw)
        {
        sight_save_needed = TRUE;
        if (gc.update == UpdateAlways)
            SightRedraw (NIL);
        }
    }



void SightPickElement (float x_ndc, float y_ndc)
    {
	SightDisplayList *p;
	BOOL redraw;
	int i, ignore, index;
	SightUpdateOption update;
	float x, y, dx, dy, distance, min_distance;

    p = NearestObject(x_ndc, y_ndc);

    if (p != NIL) 
	{
	update = gc.update;
	gc.update = UpdateConditionally;
	SightDeselect ();

	switch (p->type) {
	    case Pline :
	    case Pmarker :
	    case FillArea :
	    case BarGraph :

		min_distance = LARGE;
		index = -1;

		for (i = 0; i < p->obj.data->n; i++)
		    {
		    x = p->obj.data->x[i];
		    y = p->obj.data->y[i];

		    _SightApplyXform (p->tnr, x, y, &x, &y);

		    dx = x_ndc - x;
		    dy = y_ndc - y;

		    distance = sqrt(dx*dx + dy*dy);
		    if (distance < min_distance)
			{
			min_distance = distance;
			index = i;
			}
		    
		    }

		if (min_distance < MAX_DISTANCE)
		    {
		    var_delete ("X", &ignore);
		    var_define ("X", 0, 1, &p->obj.data->x[index], 
			FALSE, NIL);

		    var_delete ("Y", &ignore);
		    var_define ("Y", 0, 1, &p->obj.data->y[index], 
			FALSE, NIL);

		    last_selected_object = p;
		    p->state = StateSelected;
		    redraw = TRUE;
		    select_pending = TRUE;
		    modify_pending = TRUE;
		    modify_index = index;
		    }
		break;

	    case Spline :
		break;
	    }

	gc.update = update;
	}
	
    if (redraw)
        {
        sight_save_needed = TRUE;
        if (gc.update == UpdateAlways)
            SightRedraw (NIL);
        }
    }



void SightModifyObject (int n, float *x, float *y, char *text)
    {
	SightDisplayList *p;
	int i;
	float xmin, xmax, ymin, ymax;

    if (modify_pending && last_selected_object != NIL) 
	{
	p = last_selected_object;

	switch (p->type) {
	    case Pline :
	    case Pmarker :
	    case FillArea :
	    case BarGraph :
		free (p->obj.data->x);
		free (p->obj.data->y);

		p->obj.data->x = (float *)malloc(n*sizeof(float));
		p->obj.data->y = (float *)malloc(n*sizeof(float));

		xmin = x[0];
		xmax = x[0];
		ymin = y[0];
		ymax = y[0];

		p->obj.data->n = n;
		for (i = 0; i < n; i++)
		    {
		    p->obj.data->x[i] = x[i];
		    p->obj.data->y[i] = y[i];
		    if (x[i] < xmin) xmin = x[i];
		    if (x[i] > xmax) xmax = x[i];
		    if (y[i] < ymin) ymin = y[i];
		    if (y[i] > ymax) ymax = y[i];
		    }

		p->xmin = xmin;
		p->xmax = xmax;
		p->ymin = ymin;
		p->ymax = ymax;
		break;

	    case Spline :
		break;

	    case SimpleText :
	    case Text :
		free (p->obj.text->chars);

		p->obj.text->chars = (char *)malloc((n+1)*sizeof(char));

		strcpy (p->obj.text->chars, text);
		break;
	    }
        sight_save_needed = TRUE;
	if (gc.update == UpdateAlways)
	    SightRedraw (NIL);
	}
	
    }



void SightModifyElement (int n, float *x, float *y)
    {
	SightDisplayList *p;
	int i, np;
	float xmin, xmax, ymin, ymax;
	float *px, *py;

    if (modify_pending && modify_index >= 0 && last_selected_object != NIL) 
	{
	p = last_selected_object;

	switch (p->type) {
	    case Pline :
	    case Pmarker :
	    case FillArea :
	    case BarGraph :
		if (n == 0)
		    {
		    for (i = modify_index+1; i < p->obj.data->n; i++)
			{
			p->obj.data->x[i-1] = p->obj.data->x[i];
			p->obj.data->y[i-1] = p->obj.data->y[i];
			}
		    p->obj.data->n--;

		    if (modify_index >= p->obj.data->n)
			modify_index = p->obj.data->n - 1;
		    }
		else if (n == 1)
		    {
		    p->obj.data->x[modify_index] = *x;
		    p->obj.data->y[modify_index] = *y;
		    }
		else if (n > 1)
		    {
		    np = n+p->obj.data->n-1;
		    px = (float *)malloc(np*sizeof(float));
		    py = (float *)malloc(np*sizeof(float));

		    for (i = 0; i < modify_index; i++)
			{
			px[i] = p->obj.data->x[i];
			py[i] = p->obj.data->y[i];
			}

		    for (i = 0; i < n; i++)
			{
			px[i+modify_index] = x[i];
			py[i+modify_index] = y[i];
			}

		    for (i = modify_index+1; i < p->obj.data->n; i++)
			{
			px[i+n-1] = p->obj.data->x[i];
			py[i+n-1] = p->obj.data->y[i];
			}

		    free (p->obj.data->x);
		    free (p->obj.data->y);

		    p->obj.data->n = np;
		    p->obj.data->x = px;
		    p->obj.data->y = py;
		    }

		xmin = p->obj.data->x[0];
		xmax = p->obj.data->x[0];
		ymin = p->obj.data->y[0];
		ymax = p->obj.data->y[0];

		for (i = 0; i < p->obj.data->n; i++)
		    {
		    if (p->obj.data->x[i] < xmin) xmin = p->obj.data->x[i];
		    if (p->obj.data->x[i] > xmax) xmax = p->obj.data->x[i];
		    if (p->obj.data->y[i] < ymin) ymin = p->obj.data->y[i];
		    if (p->obj.data->y[i] > ymax) ymax = p->obj.data->y[i];
		    }

		p->xmin = xmin;
		p->xmax = xmax;
		p->ymin = ymin;
		p->ymax = ymax;
		break;

	    case Spline :
		break;

	    }
        sight_save_needed = TRUE;
	if (gc.update == UpdateAlways)
	    SightRedraw (NIL);
	}
	
    }



static
int SetSelect (SightDisplayList *p)
    {
	int depth, n;

    n = 0;

    if (p != NIL)
	{
	if (p->state == StateNormal) {
	    n++;
	    p->state = StateSelected;
	    }

	if (p->type == BeginSegment) {
	    depth = p->depth;

	    p = p->next;
	    while (p != NIL) {
		if (p->depth > depth) {
		    if (p->state == StateNormal) {
			n++;
			p->state = StateSelected;
			}
		    p = p->next;
		    }
		else
		    p = NIL;
		}
	    }
	}

    return (n);  /* number of selected objects */
    }



static
void SetDeselect (SightDisplayList *p)
    {
	int depth;

    if (p != NIL)
	{
	if (p->state == StateSelected)
	    p->state = StateNormal;
	if (p->type == BeginSegment) {
	    depth = p->depth;
	    p = p->next;
	    while (p != NIL) {
		if (p->depth > depth) {
		    if (p->state == StateSelected)
			p->state = StateNormal;
		    p = p->next;
		    }
		else
		    p = NIL;
		}
	    }
	}
    }



int SightSelectObject (float x_ndc, float y_ndc)
    {
	int	i, j;
	BOOL	ready;
	SightDisplayList	*p;
	SightDisplayList	*min_obj;
	static SightDisplayList	*parent[MAX_SEGMENT_DEPTH];
	int count, stat;
    
    if (sight_open)
	{
	if (depth == 0)
	    {

	    min_obj = NearestObject(x_ndc, y_ndc);

	    if (min_obj != NIL)
		{
		/* change current tnr to selected object */
		gc.tnr = min_obj->tnr;

		count = 0;
		p = min_obj;
		i = 0;
		parent[0] = min_obj;

		while (p = FindBegin(p))
		    {
		    parent[++i] = p;

		    if (i >= MAX_SEGMENT_DEPTH)
			return SIGHT__NOSELECT;
		    }

		j = i;
		ready = FALSE;
		while (j >= 0 && !ready)
		    {
		    if (parent[j]->state == StateSelected)
			{
			SetDeselect (parent[j]);
			last_selected_object = NIL;

			if (j > 0)
			    {
			    count = SetSelect(parent[j-1]);
			    last_selected_object = parent[j-1];
			    select_pending = TRUE;
			    modify_pending = FALSE;
			    }
			ready = TRUE;
			}
		    j--;
		    }

		if (!ready)
		    {
		    /* select all */
		    count = SetSelect(parent[i]);
		    last_selected_object = parent[i];
		    select_pending = TRUE;
		    modify_pending = FALSE;
		    } 

		if (gc.update == UpdateAlways)
		    SightRedraw (NIL);
		
		if (count == 0)
		    stat = SIGHT__NOSELECT;  /* No selection done */
		else
		    stat = SIGHT__NORMAL;  /* Successful completion */
		}
	    else
		stat = SIGHT__NOSELECT;  /* No selection done */
	    }
	else
	    stat = SIGHT__SEGOPE;  /* Segment not closed */
	}
    else
	stat = SIGHT__CLOSED;  /* Sight not open */

    return (stat);
    }



int SightSelectGroup (
    float xmin_ndc, float ymin_ndc, float xmax_ndc, float ymax_ndc)
    {
	SightDisplayList *p;
        float xmin_wc, ymin_wc, xmax_wc, ymax_wc, tmp;
	BOOL redraw;
	int stat;

    if (sight_open) 
	{
	if (depth == 0) 
	    {
	    redraw = FALSE;

	    _SightSaveXform ();

	    p = root;
	    while (p != NIL)
		{
		if (p->state == StateNormal)
		    {
		    if (p->tnr != SightNDC) 
			{
			_SightApplyInverseXform (p->tnr, xmin_ndc, ymin_ndc, 
			    &xmin_wc, &ymin_wc);
			_SightApplyInverseXform (p->tnr, xmax_ndc, ymax_ndc, 
			    &xmax_wc, &ymax_wc);

			if (option_flip_x & gc.xform[p->tnr].scale)
			    {
			    tmp = xmax_wc; xmax_wc = xmin_wc; xmin_wc = tmp;
			    }
			if (option_flip_y & gc.xform[p->tnr].scale)
			    {
			    tmp = ymax_wc; ymax_wc = ymin_wc; ymin_wc = tmp;
			    }

			if (xmin_wc <= p->xmin && xmax_wc >= p->xmax &&
			    ymin_wc <= p->ymin && ymax_wc >= p->ymax) {
			    p->state = StateSelected;
			    last_selected_object = p;
			    select_pending = TRUE;
			    modify_pending = FALSE;
			    redraw = TRUE;
			    }
			}
		    else {
			if (xmin_ndc <= p->xmin && xmax_ndc >= p->xmax &&
			    ymin_ndc <= p->ymin && ymax_ndc >= p->ymax) {
			    p->state = StateSelected;
			    last_selected_object = p;
			    select_pending = TRUE;
			    modify_pending = FALSE;
			    redraw = TRUE;
			    }
			}
		    }
		p = p->next;
		}	

	    if (redraw) 
		{
		stat = SIGHT__NORMAL;  /* Successful completion */
		if (gc.update == UpdateAlways)
		    SightRedraw (NIL);
		}
	    else
		stat = SIGHT__NOSELECT;  /* No selection done */

	    _SightRestoreXform ();
	    }
	else
	    stat = SIGHT__SEGOPE;  /* Segment not closed */
	}
    else
	stat = SIGHT__CLOSED;  /* Sight not open */

    return (stat);
    }



int SightSelectLast (void)

    {
    	SightDisplayList *p;
    	int count, stat;

    if (sight_open)
    	{
    	if (depth == 0)
    	    {
    	    if (last_created_object != NIL)
    	    	{
    	    	p = last_created_object;
    	    	if (p->state != StateDeleted && p->state != StateCut)
    	    	    {
    	    	    SetDeselect (last_selected_object);
    	    	    count = SetSelect(p);
    	    	    last_selected_object = p;
    	    	    select_pending = TRUE;
    	    	    modify_pending = FALSE;
    	    	    }
    	       	}
    	    
    	    if (count == 0)
    	    	stat = SIGHT__NOSELECT; 	/* No selection done */
    	    else {
    	    	stat = SIGHT__NORMAL; 		/* Successful completion */
    	    	if (gc.update == UpdateAlways)
    	    	    SightRedraw (NIL);
    	    	}

    	    }
    	else 
    	    stat = SIGHT__SEGOPE;   	/* Segment not closed */
    	}
    else 
    	stat = SIGHT__CLOSED;	/* Sight not open */  	    	

    return (stat);
    }




static
void MoveObject (SightDisplayList *p, float x, float y)
    {
	int i;
    
    if (p != NIL) {
	p->xmin = p->xmin + x;
	p->xmax = p->xmax + x;
	p->ymin = p->ymin + y;
	p->ymax = p->ymax + y;

	switch (p->type) {
	    case Pline :
	    case Pmarker :
	    case FillArea :
	    case BarGraph :
		for (i = 0; i < p->obj.line->n; i++) {
		    p->obj.line->x[i] += x;
		    p->obj.line->y[i] += y;
		    }
		break;

	    case Spline :
		for (i = 0; i < p->obj.spline->n; i++) {
		    p->obj.spline->x[i] += x;
		    p->obj.spline->y[i] += y;
		    }
		break;

	    case ErrorBars :
		for (i = 0; i < p->obj.bar->n; i++) {
		    p->obj.bar->x[i] += x;
		    p->obj.bar->y[i] += y;
		    p->obj.bar->e1[i] += x;
		    p->obj.bar->e2[i] += y;
		    }
		break;

	    case SimpleText :
	    case Text :
		p->obj.text->x += x;
		p->obj.text->y += y;
		break;
    	
    	    case Image :
    	    	p->obj.image->x_min += x;
    	    	p->obj.image->x_max += x;
    	    	p->obj.image->y_min += y;
    	    	p->obj.image->y_max += y;
		break;
	    }
	}
    }



int SightMove (float x_ndc, float y_ndc)
    {
	SightDisplayList *p;
	float x0, y0, x1, y1, x_wc, y_wc;
	int stat;
	
    if (sight_open)
	{
	if (depth == 0)
	    {
	    if (select_pending) 
		{
		p = root;
		while (p != NIL)
		    {
		    if (p->state == StateSelected && p->type != BeginSegment &&
			p->type != EndSegment)
			{
			if (p->tnr == SightNDC)
			    {
			    MoveObject (p, x_ndc, y_ndc);
			    modify_pending = FALSE;
			    }
			else
			    if (gc.xform[p->tnr].scale == 0) 
				{
				_SightApplyInverseXform (p->tnr, 0.0, 0.0, 
				    &x0, &y0);
				_SightApplyInverseXform (p->tnr, x_ndc, y_ndc,
				    &x1, &y1);

				x_wc = x1 - x0;
				y_wc = y1 - y0;

				MoveObject (p, x_wc, y_wc);
				modify_pending = FALSE;
				}
			}
		    p = p->next;
		    }

                sight_save_needed = TRUE;
        	if (gc.update == UpdateAlways)
        	    SightRedraw (NIL);
		}
	    stat = SIGHT__NORMAL;  /* Successful completion */
	    }
	else
	    stat = SIGHT__SEGOPE;  /* Segment not closed */
	}
    else
	stat = SIGHT__CLOSED;  /* Sight not open */

    return (stat);
    }



int SightCut (float x, float y)
    {
	SightDisplayList *p;
	int stat;

    if (sight_open)
	{
	if (depth == 0) 
	    {
	
	    xcut = x;
	    ycut = y;
	 
	    if (select_pending) 
		{
		p = root;
		while (p != NIL)
		    {
		    switch (p->state) {
			case StateCut :
			    p->state = StateDeleted;
			    if (last_created_object == p)
				last_created_object = NIL;
			    break;
			case StateSelected :
			    p->state = StateCut;
			    break;
			}
		    p = p->next;
		    }
		select_pending = FALSE;
		modify_pending = FALSE;
		last_selected_object = NIL;

                sight_save_needed = TRUE;
        	if (gc.update == UpdateAlways)
        	    SightRedraw (NIL);
		}
	    stat = SIGHT__NORMAL;
	    }
	else
	    stat = SIGHT__SEGOPE;
	}
    else
	stat = SIGHT__CLOSED;
    
    return (stat);
    }



static
SightDisplayList *Copy (SightDisplayList *obj)
    {
	SightDisplayList *to;

    to = (SightDisplayList *) malloc(sizeof(SightDisplayList));
    to->type = obj->type;
    to->state = obj->state;
    to->clsw = obj->clsw;
    to->tnr = obj->tnr;

    to->xmin = obj->xmin;
    to->xmax = obj->xmax;
    to->ymin = obj->ymin;
    to->ymax = obj->ymax;
    to->prev = NIL;
    to->next = NIL;
    to->depth = obj->depth;

    to->gc = obj->gc;

    switch (obj->type) {
	case Pline :
	case Pmarker :
	case FillArea :
	case BarGraph :
	    {
		SightPoint *data, *points;
		int i;

	    points = (SightPoint *)obj->obj.data;
	    data = (SightPoint *) malloc(sizeof(SightPoint));
	    data->n = points->n;
	    data->x = (float *) malloc(points->n*sizeof(float));
	    data->y = (float *) malloc(points->n*sizeof(float));

	    for (i = 0; i < points->n; i++) {
		data->x[i] = points->x[i];
		data->y[i] = points->y[i];
		}
	    to->obj.data = data;
	    }
	    break;

	case Spline :
	    {
		SightSpline *data, *spline;
		int i;

	    spline = (SightSpline *)obj->obj.spline;
	    data = (SightSpline *) malloc(sizeof(SightSpline));
	    data->smooth = spline->smooth;
	    data->n = spline->n;
	    data->x = (float *) malloc(spline->n*sizeof(float));
	    data->y = (float *) malloc(spline->n*sizeof(float));

	    for (i = 0; i < spline->n; i++) {
		data->x[i] = spline->x[i];
		data->y[i] = spline->y[i];
		}
	    to->obj.spline = data;
	    }
	    break;

	case ErrorBars :
	    {
		SightBar *data, *bar;
		int i;

	    bar = (SightBar *)obj->obj.bar;
	    data = (SightBar *) malloc(sizeof(SightBar));
	    data->orientation = bar->orientation;
	    data->n = bar->n;
	    data->x = (float *) malloc(bar->n*sizeof(float));
	    data->y = (float *) malloc(bar->n*sizeof(float));
	    data->e1 = (float *) malloc(bar->n*sizeof(float));
	    data->e2 = (float *) malloc(bar->n*sizeof(float));

	    for (i = 0; i < bar->n; i++) {
		data->x[i] = bar->x[i];
		data->y[i] = bar->y[i];
		data->e1[i] = bar->e1[i];
		data->e2[i] = bar->e2[i];
		}
	    to->obj.bar = data;
	    }
	    break;

	case SimpleText :
	case Text :
	    {
	        SightString *data, *item;
		int len;

	    item = obj->obj.text;
	    data = (SightString *) malloc(sizeof(SightString));
	    data->x = item->x;
	    data->y = item->y;
	    len = strlen(item->chars) + 1;
	    data->chars = (char *) malloc(len*sizeof(char));
	    strcpy (data->chars, item->chars);
	    to->obj.text = data;
	    }
	    break;

	case Axes :
	case Grid :
	    to->obj.axes = (SightAxes *) malloc(sizeof(SightAxes));
	    *(to->obj.axes) = *(obj->obj.axes);
	    break;

    	case Image :
    	    to->obj.image = (SightImage *) malloc(sizeof(SightImage));
    	    *(to->obj.image) = *(obj->obj.image);
    	    break;

	case Circle :
	case FilledCircle :
	case Rect :
	case FilledRect :
	    break;
	}
    return (to);
    }



int SightPaste (float x, float y)
    {
	int cur_depth, stat;
	float x_ndc, y_ndc, x_wc, y_wc, x0, y0, x1, y1;
	SightDisplayList *p, *obj, *pos, *next;
	BOOL redraw;

    if (sight_open)
	{
	if (depth == 0)
	    {
	    redraw = FALSE;

	    x_ndc = x - xcut;
	    y_ndc = y - ycut;
	    
	    pos = LastObject ();
	    cur_depth = 0;

	    p = root;
	    while (p != NIL)
		{
		if (p->state == StateCut)
		    {
		    redraw = TRUE;

		    obj = Copy(p);  
		    obj->state = StateNormal;
		    obj->tnr = gc.tnr;

		    if (obj->tnr == SightNDC) 
			MoveObject (obj, x_ndc, y_ndc);	
		    else 
			if (gc.xform[p->tnr].scale == 0) 
			    {
			    _SightApplyInverseXform (p->tnr, xcut, ycut, 
				&x0, &y0);
			    _SightApplyInverseXform (p->tnr, x, y, &x1, &y1);

			    x_wc = x1 - x0;
			    y_wc = y1 - y0;

			    MoveObject (obj, x_wc, y_wc);	
			    }

		    obj->depth = cur_depth;

		    next = pos->next;
		    pos->next = obj;

		    if (next != NIL)
			next->prev = obj;

		    obj->prev = pos;
		    obj->next = next;
		    pos = obj;

		    switch (obj->type) {
			case BeginSegment :
			    cur_depth++;
			    break;
			case EndSegment :
			    cur_depth--;
			    break;
			}
		    }
		p = p->next;
		}    

            if (redraw)
                {
                sight_save_needed = TRUE;
                if (gc.update == UpdateAlways)
                    SightRedraw (NIL);
                }

	    last_selected_object = pos;

	    stat = SIGHT__NORMAL;  /* Successful completion */
	    }
	else
	    stat = SIGHT__SEGOPE;  /* Segment not closed */
	}
    else
	stat = SIGHT__CLOSED;  /* Sight not open */


    return (stat);
    }



int SightPushBehind (void)
    {
	SightDisplayList *p;
	SightDisplayList *first;
	int stat;

    if (sight_open)
	{
	if (depth == 0)
	    {
	    if (select_pending) 
		{
		p = last_selected_object;

		if (p == NIL)
		    p = root;

		while (p != NIL)
		    {
		    if (p->state == StateSelected)
			{
			first = FirstSegmentObject(p);

			if (first != p)
			    {
			    if (p->type == BeginSegment)
				RemoveSegment (p);
			    else
				RemoveObject (p);

			    InsertBefore (p, first);
			    }
			p = NIL;
			}
		    else
			p = p->next;
		    }

                sight_save_needed = TRUE;
        	if (gc.update == UpdateAlways)
        	    SightRedraw (NIL);
		}
	    stat = SIGHT__NORMAL;  /* Successful completion */
	    }
	else
	    stat = SIGHT__SEGOPE;  /* Segment not closed */
	}
    else
	stat = SIGHT__CLOSED;  /* Sight not open */

    return (stat);
    }



int SightPopInFront (void)
    {
	SightDisplayList *p;
	SightDisplayList *last;
	int stat;

    if (sight_open)
	{
	if (depth == 0)
	    {
	    if (select_pending) 
		{
		p = last_selected_object;

		if (p == NIL)
		    p = root;

		while (p != NIL)
		    {
		    if (p->state == StateSelected)
			{
			last = LastSegmentObject(p);

			if (last != p)
			    {
			    if (p->type == BeginSegment)
				RemoveSegment (p);
			    else
				RemoveObject (p);

			    InsertAfter (p, last);
			    }
			p = NIL;
			}
		    else
			p = p->next;
		    }

                sight_save_needed = TRUE;
        	if (gc.update == UpdateAlways)
        	    SightRedraw (NIL);
		}
	    stat = SIGHT__NORMAL;  /* Successful completion */
	    }
	else
	    stat = SIGHT__SEGOPE;  /* Segment not closed */
	}
    else
	stat = SIGHT__CLOSED;  /* Sight not open */

    return (stat);
    }



int SightRedraw (SightDisplayList *position)
    {
	SightDisplayList *p;
	unsigned *data;
	SightString text_obj;
	float xmin, xmax, ymin, ymax;
	float xmargin, ymargin;
	int stat;
	String equ_string;
	
    if (!sight_open)
	return SIGHT__CLOSED;

    stat = SIGHT__NORMAL;

    _SightSaveXform ();
    _SightSaveGC ();
    _SightSaveClipping ();

    if (position == NIL) {
        _SightClearDisplay ();
	if (snap_grid != 0.0)
	    {
	    _SightChangeGC (Pmarker, (SightAnyGC *)&SnapGC);
	    _SightChangeXform (SightNDC);
	    _SightChangeClipping (ClippingOff);

	    _SightDrawSnapGrid (snap_count, snap_buffer_x, snap_buffer_y);
	    }
        p = root;
	}
    else
	p = position;

    while (p != NIL)
	{
	if (p->type != BeginSegment && p->type != EndSegment) 
	    {
	    if (p->state != StateCut && p->state != StateDeleted) {
		_SightChangeGC (p->type, (SightAnyGC *)&p->gc);
		_SightChangeXform (p->tnr);
		_SightChangeClipping (p->clsw);

		if (p->type == SimpleText || p->type == Text)
		    {
		    /* translate symbol */
		    translate_symbol (p->obj.text->chars, equ_string);
		    text_obj.x = p->obj.text->x;
		    text_obj.y = p->obj.text->y;
		    text_obj.chars = equ_string;
		    _SightInquireTextExtent (&text_obj, (SightTextGC *)&p->gc,
			&p->xmin, &p->ymin, &p->xmax, &p->ymax);
		    data = (unsigned *)&text_obj;
		    }
		else
		    data = (unsigned *)p->obj.data;

		_SightDisplayObject (p, data, &stat);
		}

	    if (p->state == StateSelected) {
		switch (p->type) {
		    case Axes :
		    case Grid :
			xmin = gc.xform[p->tnr].wn[0];
			xmax = gc.xform[p->tnr].wn[1];
			ymin = gc.xform[p->tnr].wn[2];
			ymax = gc.xform[p->tnr].wn[3];
			break;
		    default :
			xmin = p->xmin;
			xmax = p->xmax;
			ymin = p->ymin;
			ymax = p->ymax;
			break;
		    }
		    
		if (p->tnr != SightNDC) 
		    {
		    _SightApplyXform (p->tnr, xmin, ymin, &xmin, &ymin);
		    _SightApplyXform (p->tnr, xmax, ymax, &xmax, &ymax);
		    }

		_SightChangeGC (Pline, (SightAnyGC *)&DashedGC);
		_SightChangeXform (SightNDC);
		_SightChangeClipping (ClippingOff);

		xmargin = option_flip_x & gc.xform[p->tnr].scale ?
		    -MARGIN : MARGIN;
		ymargin = option_flip_y & gc.xform[p->tnr].scale ?
		    -MARGIN : MARGIN;

		_SightDrawBorder (xmin - xmargin, ymin - ymargin,
		    xmax + xmargin, ymax + ymargin);
		}
	    }
	p = p->next;
	}

    if (modify_pending && modify_index >= 0 && last_selected_object != NIL) 
	{
	p = last_selected_object;
	_SightChangeGC (Pmarker, (SightAnyGC *)&PointGC);
	_SightChangeXform (p->tnr);
	_SightChangeClipping (ClippingOff);

	_SightDrawPoint (p->obj.data->x[modify_index], 
	    p->obj.data->y[modify_index]);
	}

    _SightUpdate ();

    _SightRestoreClipping ();
    _SightRestoreXform ();
    _SightRestoreGC ();

    return (stat);
    }


int SightSelectNext (void)
    {
	SightDisplayList *p;
	int loop, stat, count;

    if (sight_open)
	{
	if (depth == 0)
	    {
	    if (last_selected_object != NIL)
		{
		p = last_selected_object;
		loop = TRUE;
		while (loop)
		    { 
		    p = NextObject(p);
		    if (p == NIL)
			loop = FALSE;
		    else
			loop = (p->state == StateDeleted || 
			    p->state == StateCut);
		    }
		if (p == NIL)
		    {
		    p = root;
		    loop = TRUE;
		    while (loop)
			{ 
			p = NextObject(p);
			if (p == NIL)
			    loop = FALSE;
			else
			    loop = (p->state == StateDeleted || 
				p->state == StateCut);
			}
		    }
		if (p != NIL)
		    {	
		    SetDeselect (last_selected_object);
		    count = SetSelect (p);
		    last_selected_object = p;
		    select_pending = TRUE;
		    modify_pending = FALSE;
		    }
		}
	    else {
		count = SetSelect (root);
		last_selected_object = root;
		select_pending = TRUE;
		modify_pending = FALSE;
		}

	    if (count == 0) 
		stat = SIGHT__NOSELECT;  /* No selection done */
	    else
		{
		stat = SIGHT__NORMAL;  /* Successful completion */
		if (gc.update == UpdateAlways)
		    SightRedraw (NIL);
		}
	    }
	else
	    stat = SIGHT__SEGOPE;  /* Segment not closed */
	}
    else
	stat = SIGHT__CLOSED;  /* Sight not open */

    return (stat);
    }



int SightSelectPrevious (void)
    {
	SightDisplayList *p;
	int loop, stat, count;

    if (sight_open)
	{
	if (depth == 0)
	    {
	    if (last_selected_object != NIL)
		{
		p = last_selected_object;
		loop = TRUE;
		while (loop)
		    { 
		    p = PrevObject(p);
		    if (p == NIL)
			loop = FALSE;
		    else
			loop = (p->state == StateDeleted || 
			    p->state == StateCut);
		    }
		if (p == NIL)
		    {
		    p = LastObject();
		    loop = (p->state == StateDeleted || 
			p->state == StateCut);
		    while (loop)
			{ 
			p = PrevObject(p);
			if (p == NIL)
			    loop = FALSE;
			else
			    loop = (p->state == StateDeleted || 
				p->state == StateCut);
			}
		    }
		if (p != NIL)
		    {	
		    SetDeselect (last_selected_object);
		    count = SetSelect (p);
		    last_selected_object = p;
		    select_pending = TRUE;
		    modify_pending = FALSE;
		    }
		}
	    else {
		p = LastObject ();
		count = SetSelect (p);
		last_selected_object = p;
		select_pending = TRUE;
		modify_pending = FALSE;
		}

	    if (count == 0) 
		stat = SIGHT__NOSELECT;  /* No selection done */
	    else
		{
		stat = SIGHT__NORMAL;  /* Successful completion */
		if (gc.update == UpdateAlways)
		    SightRedraw (NIL);
		}
	    }
	else
	    stat = SIGHT__SEGOPE;  /* Segment not closed */
	}
    else
	stat = SIGHT__CLOSED;  /* Sight not open */

    return (stat);
    }



int SightSelectExcluded (void)
    {
	SightDisplayList *p;
	int stat;

    if (sight_open) 
	{
	if (depth == 0)
	    {
	    last_selected_object = NIL;
	    select_pending = FALSE;
	    modify_pending = FALSE;

	    p = root;
	    while (p != NIL)
		{
		if (p->state == StateNormal) {
		    p->state = StateSelected;
		    select_pending = TRUE;
		    last_selected_object = p;
		    }
		else 
		    if (p->state == StateSelected)
			p->state = StateNormal;
		p = p->next;
		}

	    if (last_selected_object != NIL)
		if (last_selected_object->type == EndSegment)
		    last_selected_object =
                        FindBegin(last_selected_object->prev);

	    if (select_pending)
		stat = SIGHT__NORMAL;  /* Successful completion */
	    else
		stat = SIGHT__NOSELECT;  /* No selection done */

	    if (gc.update == UpdateAlways)
		SightRedraw (NIL);
	    }
	else
	    stat = SIGHT__SEGOPE;  /* Segment not closed */
	}
    else
	stat = SIGHT__CLOSED;  /* Sight not open */

    return (stat);
    }



int SightPolyline (int n, float *px, float *py)
    {
	int stat;
	SightPoint point;

    if (!sight_open)
	SightOpen (NIL);

    point.n = n;
    point.x = px;
    point.y = py;
    stat = _SightCreateObject (Pline, (caddr_t)&point);

    return (stat);
    }


int SightDrawSpline (int n, float *px, float *py, float smoothing)
    {
	int stat;
	SightSpline spline;

    if (!sight_open)
	SightOpen (NIL);

    spline.smooth = smoothing;
    spline.n = n;
    spline.x = px;
    spline.y = py;
    stat = _SightCreateObject (Spline, (caddr_t)&spline);

    return (stat);
    }


int SightPolymarker (int n, float *px, float *py)
    {
	int stat;
	SightPoint point;

    if (!sight_open)
	SightOpen (NIL);

    point.n = n;
    point.x = px;
    point.y = py;
    stat = _SightCreateObject (Pmarker, (caddr_t)&point);

    return (stat);
    }



int SightErrorBars (int n, float *px, float *py, float *e1, float *e2,
    SightBarOrientation orientation)
    {
	int stat;
	SightBar bar;

    if (!sight_open)
	SightOpen (NIL);

    bar.orientation = orientation;
    bar.n = n;
    bar.x = px;
    bar.y = py;
    bar.e1 = e1;
    bar.e2 = e2;
    stat = _SightCreateObject (ErrorBars, (caddr_t)&bar);

    return (stat);
    }




int SightImportImage (_SightFileSpecification filename, int startx, int starty,
    int sizex, int sizey, float x_min, float x_max, float y_min, float y_max)
    {
    	int stat;
    	SightImage image;

    if (!sight_open)
	SightOpen (NIL);

    strcpy(image.filename, filename);
    image.img = NULL;
    image.startx = startx;    
    image.starty = starty;    
    image.sizex = sizey;
    image.sizey = sizey;
    image.x_min = x_min;    
    image.x_max = x_max;    
    image.y_min = y_min;    
    image.y_max = y_max;    

    stat = _SightCreateObject(Image, (caddr_t)&image);

    return(stat);
    }




int SightFillArea (int n, float *px, float *py)
    {
	int stat;
	SightPoint point;

    if (!sight_open)
	SightOpen (NIL);

    point.n = n;
    point.x = px;
    point.y = py;
    stat = _SightCreateObject (FillArea, (caddr_t)&point);

    return (stat);
    }



int SightBarGraph (int n, float *px, float *py)
    {
	int stat;
	SightPoint point;

    if (!sight_open)
	SightOpen (NIL);

    point.n = n;
    point.x = px;
    point.y = py;
    stat = _SightCreateObject (BarGraph, (caddr_t)&point);

    return (stat);
    }



int SightText (float x, float y, char *chars)
    {
	int stat;
	SightString text;

    if (!sight_open)
	SightOpen (NIL);

    text.x = x;
    text.y = y;
    text.chars = chars;
    stat = _SightCreateObject (Text, (caddr_t)&text);

    return (stat);
    }



int SightSimpleText (float x, float y, char *chars)
    {
	int stat;
	SightString text;

    if (!sight_open)
	SightOpen (NIL);

    text.x = x;
    text.y = y;
    text.chars = chars;
    stat = _SightCreateObject (SimpleText, (caddr_t)&text);

    return (stat);
    }



int SightDrawAxes (float x_tick, float y_tick, float x_org, float y_org,
    int maj_x, int maj_y, float tick_size)
    {
	int stat;
	SightAxes axes;

    if (!sight_open)
	SightOpen (NIL);

    axes.x_tick = x_tick;
    axes.y_tick = y_tick;
    axes.x_org = x_org;
    axes.y_org = y_org;
    axes.maj_x = maj_x;
    axes.maj_y = maj_y;
    axes.tick_size = tick_size;

    stat = _SightCreateObject (Axes, (caddr_t)&axes);

    return (stat);
    }


int SightDrawGrid (float x_tick, float y_tick, float x_org, float y_org,
    int maj_x, int maj_y)
    {
	int stat;
	SightAxes grid;

    if (!sight_open)
	SightOpen (NIL);

    grid.x_tick = x_tick;
    grid.y_tick = y_tick;
    grid.x_org = x_org;
    grid.y_org = y_org;
    grid.maj_x = maj_x;
    grid.maj_y = maj_y;
    stat = _SightCreateObject (Grid, (caddr_t)&grid);

    return (stat);
    }


int SightOpenSegment (void)
    {
	int stat;

    if (!sight_open)
	SightOpen (NIL);

    stat = _SightCreateObject (BeginSegment, NIL);

    return (stat);
    }



int SightCloseSegment (void)
    {
	int stat;

    if (sight_open)
	{
	if (depth != 0)
	    stat = _SightCreateObject (EndSegment, NIL);
	else
	    stat = SIGHT__SEGCLO;  /* segment not open */
	}
    else
	stat = SIGHT__CLOSED;  /* segment not open */
 
    return (stat);
    }



int SightAlign (void)

    {
	SightDisplayList *p;
	BOOL redraw;
	int stat;
	
    if (sight_open)
	{
	if (depth == 0)
	    {
	    if (select_pending && snap_grid != 0.0) 
		{
		redraw = FALSE;

		p = root;
		while (p != NIL)
		    {
		    if (p->state == StateSelected &&
			(p->type == SimpleText || p->type == Text))
			{
			p->obj.text->x = (int)(p->obj.text->x/snap_grid+0.5)*
			    snap_grid;
			p->obj.text->y = (int)(p->obj.text->y/snap_grid+0.5)*
			    snap_grid;

			redraw = TRUE;
			}
		    p = p->next;
		    }

		if (redraw)
                    {
                    sight_save_needed = TRUE;
                    if (gc.update == UpdateAlways)
                        SightRedraw (NIL);
                    }
		}
	    stat = SIGHT__NORMAL;  /* Successful completion */
	    }
	else
	    stat = SIGHT__SEGOPE;  /* Segment not closed */
	}
    else
	stat = SIGHT__CLOSED;  /* Sight not open */

    return (stat);
    }



void SightSetAttribute (SightAttribute type, caddr_t attribute)
    {
	int changes;
	float size;

    if (!sight_open)
	SightOpen (NIL);

    switch (type) {
	case LineType : 
	    gc.line.linetype = *(int *)attribute; 
	    changes = ChangeGC (type, attribute);
	    break;

	case LineWidth : 
	    gc.line.linewidth = *(float *)attribute; 
	    gc.bar.linewidth = *(float *)attribute; 
	    gc.axes.linewidth = *(float *)attribute; 
	    changes = ChangeGC (LineWidth, attribute) + 
		ChangeGC (BarLineWidth, attribute) + 
		ChangeGC (AxesLineWidth, attribute);
	    break;

	case LineColor : 
	    gc.line.linecolor = *(int *)attribute; 
	    gc.bar.linecolor = *(int *)attribute; 
	    gc.axes.linecolor = *(int *)attribute; 
	    changes = ChangeGC (LineColor, attribute) + 
		ChangeGC (BarLineColor, attribute) + 
		ChangeGC (AxesLineColor, attribute);
	    break;

	case MarkerType :
	    gc.marker.markertype = *(int *)attribute; 
	    gc.bar.markertype = *(int *)attribute; 
	    changes = ChangeGC (MarkerType, attribute) + 
		ChangeGC (BarMarkerType, attribute);
	    break;

	case MarkerSize :
	    size = *(float *)attribute / NominalMarkerSize; 
	    gc.marker.markersize = size;
	    gc.bar.markersize = size;
	    changes = ChangeGC (MarkerSize, (caddr_t)&size) + 
		ChangeGC (BarMarkerSize, (caddr_t)&size);
	    break;

	case MarkerColor :
	    gc.marker.markercolor = *(int *)attribute; 
	    gc.bar.markercolor = *(int *)attribute; 
	    changes = ChangeGC (MarkerColor, attribute) + 
		ChangeGC (BarMarkerColor, attribute);
	    break;

	case FillStyle : 
	    gc.fill.fillstyle = *(int *)attribute; 
	    changes = ChangeGC (type, attribute);
	    break;

	case FillIndex : 
	    gc.fill.fillindex = *(int *)attribute; 
	    changes = ChangeGC (type, attribute);
	    break;

	case FillColor : 
	    gc.fill.fillcolor = *(int *)attribute; 
	    changes = ChangeGC (type, attribute);
	    break;

	case TextFont :
	    gc.text.textfont = *(int *)attribute; 
	    gc.axes.textfont = *(int *)attribute; 
	    changes = ChangeGC (TextFont, attribute) + 
		ChangeGC (AxesTextFont, attribute);
	    break;

	case TextSize : 
            size = *(float *)attribute / WindowHeight * CapSize / Ratio;
	    gc.text.textsize = size;
	    gc.axes.textsize = size;
	    changes = ChangeGC (TextSize, (caddr_t)&size) + 
		ChangeGC (AxesTextSize, (caddr_t)&size);
	    break;

	case TextHalign : 
	    gc.text.texthalign = *(int *)attribute; 
	    changes = ChangeGC (type, attribute);
	    break;

	case TextValign : 
	    gc.text.textvalign = *(int *)attribute; 
	    changes = ChangeGC (type, attribute);
	    break;

	case TextDirection : 
	    gc.text.textdirection = *(int *)attribute; 
	    changes = ChangeGC (type, attribute);
	    break;

	case TextColor :
	    gc.text.textcolor = *(int *)attribute; 
	    gc.axes.textcolor = *(int *)attribute; 
	    changes = ChangeGC (TextColor, attribute) + 
		ChangeGC (AxesTextColor, attribute);

	}

    if (changes)
        {
        sight_save_needed = TRUE;
        if (gc.update == UpdateAlways)
            SightRedraw (NIL);
        }
    }



void SightSetSnapGrid (float value)
    {
	int i,j,n;
        float ndc_value;

    ndc_value = value / MWidth;

    if (ndc_value != snap_grid)
	{
	if (snap_buffer_x) 
	    {
	    free(snap_buffer_x);
	    free(snap_buffer_y);
	    snap_buffer_x = NIL;
	    snap_buffer_y = NIL;
	    snap_count = 0;
	    }
	    
	if (value != 0.0)
	    {
	    if (value < 0.25)
		snap_grid = 0.25 / MWidth;
	    else
		snap_grid = ndc_value;

	    n = (int)(1.0/snap_grid);

	    snap_count = (n+1)*(n+1);
	    snap_buffer_x = (float *)malloc(snap_count*sizeof(float));
	    snap_buffer_y = (float *)malloc(snap_count*sizeof(float));

	    for (i = 0; i <= n; i++)
		{
		for (j = 0; j <= n; j++)
		    {
		    snap_buffer_x[j+i*(n+1)] = i*snap_grid;
		    snap_buffer_y[j+i*(n+1)] = j*snap_grid;
		    }
		}
	    }
	else
	    snap_grid = 0.0;

	if (gc.update == UpdateAlways)
	    SightRedraw (NIL);
	}
    }



void SightSetAxesXtick (float x_tick)
    {
	SightDisplayList *p;
	BOOL redraw;

    if (sight_open)
	{
	if (select_pending)
	    {
	    redraw = FALSE;
	    p = root;
	    while (p != NIL)
		{
		if (p->type == Axes && p->state == StateSelected)
		    {
		    if (p->obj.axes->x_tick != x_tick)
			{
			redraw = TRUE;
			p->obj.axes->x_tick = x_tick;
			}
		    }
		p = p->next;
		}
            if (redraw)
                {
                sight_save_needed = TRUE;
                if (gc.update == UpdateAlways)
                    SightRedraw (NIL);
                }
	    }
	}
    }



void SightSetAxesYtick (float y_tick)
    {
	SightDisplayList *p;
	BOOL redraw;

    if (sight_open)
	{
	if (select_pending)
	    {
	    redraw = FALSE;
	    p = root;
	    while (p != NIL)
		{
		if (p->type == Axes && p->state == StateSelected)
		    {
		    if (p->obj.axes->y_tick != y_tick)
			{
			redraw = TRUE;
			p->obj.axes->y_tick = y_tick;
			}
		    }
		p = p->next;
		}
            if (redraw)
                {
                sight_save_needed = TRUE;
                if (gc.update == UpdateAlways)
                    SightRedraw (NIL);
                }
	    }
	}
    }



void SightSetAxesXorg (float x_org)
    {
	SightDisplayList *p;
	BOOL redraw;

    if (sight_open)
	{
	if (select_pending)
	    {
	    redraw = FALSE;
	    p = root;
	    while (p != NIL)
		{
		if (p->type == Axes && p->state == StateSelected)
		    {
		    if (p->obj.axes->x_org != x_org)
			{
			redraw = TRUE;
			p->obj.axes->x_org = x_org;
			}
		    }
		p = p->next;
		}
            if (redraw)
                {
                sight_save_needed = TRUE;
                if (gc.update == UpdateAlways)
                    SightRedraw (NIL);
                }
	    }
	}
    }



void SightSetAxesYorg (float y_org)
    {
	SightDisplayList *p;
	BOOL redraw;

    if (sight_open)
	{
	if (select_pending)
	    {
	    redraw = FALSE;
	    p = root;
	    while (p != NIL)
		{
		if (p->type == Axes && p->state == StateSelected)
		    {
		    if (p->obj.axes->y_org != y_org)
			{
			redraw = TRUE;
			p->obj.axes->y_org = y_org;
			}
		    }
		p = p->next;
		}
            if (redraw)
                {
                sight_save_needed = TRUE;
                if (gc.update == UpdateAlways)
                    SightRedraw (NIL);
                }
	    }
	}
    }



void SightSetAxesMajorX (int major_x)
    {
	SightDisplayList *p;
	BOOL redraw;

    if (sight_open)
	{
	if (select_pending)
	    {
	    redraw = FALSE;
	    p = root;
	    while (p != NIL)
		{
		if (p->type == Axes && p->state == StateSelected)
		    {
		    if (p->obj.axes->maj_x != major_x)
			{
			redraw = TRUE;
			p->obj.axes->maj_x = major_x;
			}
		    }
		p = p->next;
		}
            if (redraw)
                {
                sight_save_needed = TRUE;
                if (gc.update == UpdateAlways)
                    SightRedraw (NIL);
                }
	    }
	}
    }



void SightSetAxesMajorY (int major_y)
    {
	SightDisplayList *p;
	BOOL redraw;

    if (sight_open)
	{
	if (select_pending)
	    {
	    redraw = FALSE;
	    p = root;
	    while (p != NIL)
		{
		if (p->type == Axes && p->state == StateSelected)
		    {
		    if (p->obj.axes->maj_y != major_y)
			{
			redraw = TRUE;
			p->obj.axes->maj_y = major_y;
			}
		    }
		p = p->next;
		}
            if (redraw)
                {
                sight_save_needed = TRUE;
                if (gc.update == UpdateAlways)
                    SightRedraw (NIL);
                }
	    }
	}
    }



void SightSetAxesTickSize (float tick_size)
    {
	SightDisplayList *p;
	BOOL redraw;

    if (sight_open)
	{
	if (select_pending)
	    {
	    redraw = FALSE;
	    p = root;
	    while (p != NIL)
		{
		if (p->type == Axes && p->state == StateSelected)
		    {
		    if (p->obj.axes->tick_size != tick_size)
			{
			redraw = TRUE;
			p->obj.axes->tick_size = tick_size;
			}
		    }
		p = p->next;
		}
            if (redraw)
                {
                sight_save_needed = TRUE;
                if (gc.update == UpdateAlways)
                    SightRedraw (NIL);
                }
	    }
	}
    }




void SightSetImageSize (int *size)
    {
	SightDisplayList *p;
	BOOL redraw;

    if (sight_open)
	{
	if (select_pending)
	    {
	    redraw = FALSE;
	    p = root;
	    while (p != NIL)
		{
		if (p->type == Image && p->state == StateSelected)
		    {
		    if (p->obj.image->startx != size[0] ||
			p->obj.image->starty != size[1] ||
			p->obj.image->sizex != size[2] ||
			p->obj.image->sizey != size[3])
			{
			redraw = TRUE;
  		    	p->obj.image->startx = size[0];
    	    	    	p->obj.image->starty = size[1];
    	    	    	p->obj.image->sizex = size[2];
    	    	    	p->obj.image->sizey = size[3];
			}
		    }
		p = p->next;
		}

            if (redraw)
                {
                sight_save_needed = TRUE;
                if (gc.update == UpdateAlways)
                    SightRedraw (NIL);
                }
	    }
	}
    }




void SightSetImagePosition (float *position)
    {
	SightDisplayList *p;
	BOOL redraw;

    if (sight_open)
	{
	if (select_pending)
	    {
	    redraw = FALSE;
	    p = root;
	    while (p != NIL)
		{
		if (p->type == Image && p->state == StateSelected)
		    {
		    if (p->obj.image->x_min != position[0] ||
			p->obj.image->x_max != position[1] ||
    	    	    	p->obj.image->y_min != position[2] ||
			p->obj.image->y_max != position[3])
			{
			redraw = TRUE;
  		    	p->obj.image->x_min = position[0];
    	    	    	p->obj.image->x_max = position[1];
    	    	    	p->obj.image->y_min = position[2];
    	    	    	p->obj.image->y_max = position[3];

    	    	    	p->xmin = position[0];
    	    	    	p->xmax = position[1];
    	    	    	p->ymin = position[2];
    	    	    	p->ymax = position[3];
			}
		    }
		p = p->next;
		}

            if (redraw)
                {
                sight_save_needed = TRUE;
                if (gc.update == UpdateAlways)
                    SightRedraw (NIL);
                }
	    }
	}
    }



void SightSetSmoothing (float smoothing)
    {
	SightDisplayList *p;
	BOOL redraw;

    if (sight_open)
	{
	if (select_pending)
	    {
	    redraw = FALSE;
	    p = root;
	    while (p != NIL)
		{
		if (p->type == Spline && p->state == StateSelected)
		    {
		    if (p->obj.spline->smooth != smoothing)
			{
			redraw = TRUE;
			p->obj.spline->smooth = smoothing;
			}
		    }
		p = p->next;
		}
            if (redraw)
                {
                sight_save_needed = TRUE;
                if (gc.update == UpdateAlways)
                    SightRedraw (NIL);
                }
	    }
	}
    }



void SightSetClipping (int clipping)
    {
	SightDisplayList *p;
	BOOL redraw;

    if (sight_open)
	{
	gc.clsw = clipping;

	if (select_pending)
	    {
	    redraw = FALSE;
	    p = root;
	    while (p != NIL)
		{
		if (p->state == StateSelected)
		    {
		    if (p->clsw != clipping)
			{
			redraw = TRUE;
			p->clsw = clipping;
			}
		    }
		p = p->next;
		}
            if (redraw)
                {
                sight_save_needed = TRUE;
                if (gc.update == UpdateAlways)
                    SightRedraw (NIL);
                }
	    }
	}
    }



void SightSetUpdate (int flag)
    {
    gc.update = (SightUpdateOption)flag;
    }



void SightObjectID (SightDisplayList **obj)
    {
    *obj = last_selected_object;
    }



int _SightGetTextSize (float size)
    {
	static int text_sizes[] = { 8, 10, 12, 14, 18, 24 };
        int i, pts, nearest_pts, diff;
	
    pts = (int)(size * WindowHeight / CapSize * Ratio + 0.5);

    nearest_pts = 8;
    diff = abs(pts - nearest_pts);

    for (i = 1; i < 6; i++)
	{
	if (abs(pts - text_sizes[i]) < diff)
	    {
	    nearest_pts = text_sizes[i];
	    diff = abs(pts - nearest_pts);
	    }
	}

    return nearest_pts;
    }



void SightInquireObject (char *str)
    {
	SightDisplayList *obj;

    obj = last_selected_object;

    if (obj != NIL && *str) {

	switch (obj->type) {
	    case Pline :
		sprintf (str, "%s [%d], Line Type: %s, Line Width: %d pt,\
 Color: %s", 
		    SightObjectName[(int)obj->type],
		    obj->obj.data->n,
		    SightObjectLineType[obj->gc.line.linetype+8],
		    (int)(obj->gc.line.linewidth+0.5),
		    SightObjectColor[min(obj->gc.line.linecolor, 8)]);
		break;
	    case Spline :
		sprintf (str, "%s [%d], Smoothing: %g, Line Type: %s,\
 Line Width: %d pt, Color: %s", 
		    SightObjectName[(int)obj->type],
		    obj->obj.spline->n,
		    obj->obj.spline->smooth,
		    SightObjectLineType[obj->gc.line.linetype+8],
		    (int)(obj->gc.line.linewidth+0.5),
		    SightObjectColor[min(obj->gc.line.linecolor, 8)]);
		break;
	    case Pmarker :
		sprintf (str, "%s [%d], Marker Type: %s, Marker Size: %d pt,\
 Color: %s", 
		    SightObjectName[(int)obj->type],
		    obj->obj.data->n,
		    SightObjectMarkerType[obj->gc.marker.markertype+20],
		    (int)(obj->gc.marker.markersize*NominalMarkerSize+0.5),
		    SightObjectColor[min(obj->gc.marker.markercolor, 8)]);
		break;
	    case ErrorBars :
		sprintf (str, "%s [%d], Marker Type: %s, Marker Size: %d pt,\
 Marker Color: %s, Line Width: %d pt, Line Color: %s", 
		    SightObjectName[(int)obj->type],
		    obj->obj.bar->n,
		    SightObjectMarkerType[obj->gc.bar.markertype+20],
		    (int)(obj->gc.bar.markersize*NominalMarkerSize+0.5),
		    SightObjectColor[min(obj->gc.bar.markercolor, 8)],
		    (int)(obj->gc.bar.linewidth+0.5),
		    SightObjectColor[min(obj->gc.bar.linecolor, 8)]);
		break;
	    case FillArea :
	    case BarGraph :
		sprintf (str, "%s [%d], Fill Style: %s, Index: %d, Color: %s", 
		    SightObjectName[(int)obj->type],
		    obj->obj.data->n,
		    SightObjectStyle[obj->gc.fill.fillstyle],
		    obj->gc.fill.fillindex,
		    SightObjectColor[min(obj->gc.fill.fillcolor, 8)]);  
		break;
	    case SimpleText :
	    case Text :
		if (obj->type == Text)
		    sprintf (str, "%s, Family: %s, %s, Alignment: (%s, %s),\
 Text Size: %d pt, Color: %s", 
			SightObjectName[(int)obj->type],
			SightObjectFont[(obj->gc.text.textfont-1)%8],
			SightObjectFontType[(obj->gc.text.textfont-1)/8],
			SightObjectHalign[obj->gc.text.texthalign],
			SightObjectValign[obj->gc.text.textvalign],
			_SightGetTextSize(obj->gc.text.textsize),
			SightObjectColor[min(obj->gc.text.textcolor, 8)]);  
		else
		    sprintf (str, "%s, Font Number: %d, Alignment: (%s, %s),\
 Text Size: %g, Color: %s", 
			SightObjectName[(int)obj->type],
			obj->gc.text.textfont,
			SightObjectHalign[obj->gc.text.texthalign],
			SightObjectValign[obj->gc.text.textvalign],
			obj->gc.text.textsize,
			SightObjectColor[min(obj->gc.text.textcolor, 8)]);  
		break;
	    case Axes :
		sprintf (str, "%s, X-tick: %g, Y-tick: %g, X-org: %g,\
 Y-org: %g, Major-X: %d, Major-Y: %d, Tick-size: %g", 
		    SightObjectName[(int)obj->type],
		    obj->obj.axes->x_tick,
		    obj->obj.axes->y_tick,
		    obj->obj.axes->x_org,
		    obj->obj.axes->y_org,
		    obj->obj.axes->maj_x,
		    obj->obj.axes->maj_y,
		    obj->obj.axes->tick_size);
		break;
    	    case Image :
    	    	sprintf(str, "%s, File: %s, X-start: %d, Y-start: %d,\
 X-size: %d, Y-size: %d, X-min: %g, X-max: %g, Y-min: %g, Y-max: %g",
		    SightObjectName[(int)obj->type],
    	    	    obj->obj.image->filename,
    	    	    obj->obj.image->startx,
    	    	    obj->obj.image->starty,
    	    	    obj->obj.image->sizex,
    	    	    obj->obj.image->sizey,
    	    	    obj->obj.image->x_min,
    	    	    obj->obj.image->x_max,
    	    	    obj->obj.image->y_min,
    	    	    obj->obj.image->y_max);
    	    	break;
	    case BeginSegment :
		sprintf (str, "%s, Depth: %d", SightObjectName[(int)obj->type],
		    obj->depth);
		break;
	    default :
		strcpy (str, "");
	    }
	}
    else
	sprintf (str, "Transformation: %d, Window: (%g, %g, %g, %g),\
 Viewport: (%g, %g, %g, %g), Scale: %s, Flip: %s, Clipping: %s", gc.tnr,
	    gc.xform[gc.tnr].wn[0], gc.xform[gc.tnr].wn[1],
	    gc.xform[gc.tnr].wn[2], gc.xform[gc.tnr].wn[3],
	    gc.xform[gc.tnr].vp[0], gc.xform[gc.tnr].vp[1],
	    gc.xform[gc.tnr].vp[2], gc.xform[gc.tnr].vp[3],
	    SightScaleOptions[gc.xform[gc.tnr].scale & 0x3],
	    SightFlipOptions[(gc.xform[gc.tnr].scale >> 3) & 0x3],
	    SightClipOptions[gc.clsw]);
    }


int SightOpenDrawing (char *filename)
    {
	FILE *file;
	SightDisplayList *p, *last;
	SightOldDisplayList old_obj;
	int i, n, ok, stat;
	SightOldGC old_gc;
	SightOldAnyGC dummy;

    if (!sight_open)
	SightOpen (NIL);

    file = fopen(filename, "r");

    if (file) {
	
	stat = SIGHT__NORMAL;

	last = LastObject ();

	fread (&old_gc, sizeof(SightOldGC), 1, file);

	/* line */
	fread (&dummy, sizeof(SightOldAnyGC), 1, file);
	/* marker */
	fread (&dummy, sizeof(SightOldAnyGC), 1, file);
	/* fill */
	fread (&dummy, sizeof(SightOldAnyGC), 1, file);
	/* text */
	fread (&dummy, sizeof(SightOldAnyGC), 1, file);
	/* axes */
	fread (&dummy, sizeof(SightOldAnyGC), 1, file);
	
	gc.xform[1].scale = old_gc.scale;
	for (i = 0; i < 4; i++)
	    {
	    gc.xform[1].wn[i] = old_gc.wn[i];
	    gc.xform[1].vp[i] = old_gc.vp[i];
	    }
	_SightSetWindow (SightWC, gc.xform[1].wn, gc.xform[1].vp);

	ok = fread (&old_obj, sizeof(SightOldDisplayList), 1, file);

	while (!feof(file) && ok) {

	    p = (SightDisplayList *) malloc(sizeof(SightDisplayList));
	    p->type = old_obj.type;
	    p->state = old_obj.state;
	    p->clsw = old_obj.clsw;
	    p->tnr = old_obj.tnr;
	    p->xmin = old_obj.xmin;
	    p->xmax = old_obj.xmax;
	    p->ymin = old_obj.ymin;
	    p->ymax = old_obj.ymax;
	    p->prev = NIL;
	    p->next = NIL;
	    p->depth = old_obj.depth;

	    switch (p->type) {

		case Pline :
		case Pmarker :
		case FillArea :
		case BarGraph :
		    p->obj.data = (SightPoint *) malloc(sizeof(SightPoint));

		    fread (&n, sizeof(int), 1, file);

		    p->obj.data->n = n;
		    p->obj.data->x = (float *) malloc(n*sizeof(float));
		    p->obj.data->y = (float *) malloc(n*sizeof(float));

		    fread (p->obj.data->x, n*sizeof(float), 1, file);
		    fread (p->obj.data->y, n*sizeof(float), 1, file);

		    fread (&dummy, sizeof(SightOldAnyGC), 1, file);
		    p->gc = dummy.Any;
		    break;

		case Spline :
		    p->obj.spline = (SightSpline *) malloc(sizeof(SightSpline));

		    fread (&p->obj.spline->smooth, sizeof(float), 1, file);
		    fread (&n, sizeof(int), 1, file);

		    p->obj.spline->n = n;
		    p->obj.spline->x = (float *) malloc(n*sizeof(float));
		    p->obj.spline->y = (float *) malloc(n*sizeof(float));

		    fread (p->obj.spline->x, n*sizeof(float), 1, file);
		    fread (p->obj.spline->y, n*sizeof(float), 1, file);

		    fread (&dummy, sizeof(SightOldAnyGC), 1, file);
		    p->gc = dummy.Any;
		    break;

		case ErrorBars :
		    p->obj.bar = (SightBar *) malloc(sizeof(SightBar));

		    fread (&p->obj.bar->orientation, sizeof(int), 1, file);
		    fread (&n, sizeof(int), 1, file);

		    p->obj.bar->n = n;
		    p->obj.bar->x = (float *) malloc(n*sizeof(float));
		    p->obj.bar->y = (float *) malloc(n*sizeof(float));
		    p->obj.bar->e1 = (float *) malloc(n*sizeof(float));
		    p->obj.bar->e2 = (float *) malloc(n*sizeof(float));

		    fread (p->obj.bar->x, n*sizeof(float), 1, file);
		    fread (p->obj.bar->y, n*sizeof(float), 1, file);
		    fread (p->obj.bar->e1, n*sizeof(float), 1, file);
		    fread (p->obj.bar->e2, n*sizeof(float), 1, file);

		    fread (&dummy, sizeof(SightOldAnyGC), 1, file);
		    p->gc = dummy.Any;
		    break;

		case SimpleText :
		case Text :
		    p->obj.text = (SightString *) malloc(sizeof(SightString));

		    fread (&n, sizeof(int), 1, file);
		    fread (&p->obj.text->x, sizeof(float), 1, file);
		    fread (&p->obj.text->y, sizeof(float), 1, file);

		    p->obj.text->chars = (char *) malloc(n);

		    fread (p->obj.text->chars, n, 1, file);

		    fread (&dummy, sizeof(SightOldAnyGC), 1, file);
		    p->gc = dummy.Any;
		    break;

		case Axes :
		case Grid :
		    p->obj.axes = (SightAxes *) malloc(sizeof(SightAxes));

		    fread (p->obj.axes, sizeof(SightAxes), 1, file);

		    fread (&dummy, sizeof(SightOldAnyGC), 1, file);
		    p->gc = dummy.Any;
		    break;
		}

	    if (root == NIL) {
		root = p;
		p->prev = NIL;
		p->next = NIL;
		last = p;
		}
	    else {
		last->next = p;
		p->prev = last;
		p->next = NIL;
		last = p;
		}
	    ok = fread (&old_obj, sizeof(SightOldDisplayList), 1, file);
	    }

	if (fclose (file))
	    stat = SIGHT__CLOSEFAI; /* File close failure */

	SightRedraw (NIL);
	}
    else
	stat = SIGHT__OPENFAI; /* File open failure */

    return (stat);
    }



int SightCapture (char *filename, BOOL epsf)
    {
	int stat;
	SightUpdateOption update;

    if (sight_open)
	{
	if (root != NIL)
	    {
	    stat = _SightOpenFigureFile (filename, epsf);

	    if (stat >= 0) 
		{
		update = gc.update;
		gc.update = UpdateConditionally;

		SightDeselect ();
		SightSetSnapGrid (0.0);
		SightRedraw (NIL);

		gc.update = update;

		_SightCloseFigureFile ();

		stat = SIGHT__NORMAL;  /* Successful completion */
		}
	    else
		stat = SIGHT__OPENFAI;  /* File open failure */
	    }
	else
	    stat = SIGHT__NOOBJECT;  /* No objects found */
	}
    else
	stat = SIGHT__CLOSED;  /* Sight not open */

    return (stat);
    }




int SightSetWindow (float xmin, float xmax, float ymin, float ymax)
    {
	int stat, i;
	float cur_wn[4];

    if (!sight_open)
	return SIGHT__CLOSED;

    for (i = 0; i < 4; i++) 
	cur_wn[i] = gc.xform[gc.tnr].wn[i];

    if (xmax > xmin) {
	gc.xform[gc.tnr].wn[0] = xmin;
	gc.xform[gc.tnr].wn[1] = xmax;
	}
    else {
	gc.xform[gc.tnr].wn[0] = wn[0];
	gc.xform[gc.tnr].wn[1] = wn[1];
	}
    if (ymax > ymin) {
	gc.xform[gc.tnr].wn[2] = ymin;
	gc.xform[gc.tnr].wn[3] = ymax;
	}
    else {
	gc.xform[gc.tnr].wn[2] = wn[2];
	gc.xform[gc.tnr].wn[3] = wn[3];
	}

    _SightChangeXform (gc.tnr);

    sight_save_needed = TRUE;
    if (gc.update == UpdateAlways)
	stat = SightRedraw (NIL);
    else 
	stat = SIGHT__NORMAL;

    if (!odd(stat))
        {
        for (i = 0; i < 4; i++)
            gc.xform[gc.tnr].wn[i] = cur_wn[i];
        _SightChangeXform (gc.tnr);
        }

    return (stat);
    }



int SightSetViewport (float xmin, float xmax, float ymin, float ymax)
    {
	int stat, i;
	float cur_vp[4];

    if (!sight_open)
	return SIGHT__CLOSED;

    for (i = 0; i < 4; i++) 
	cur_vp[i] = gc.xform[gc.tnr].vp[i];

    if (xmax > xmin) {
	gc.xform[gc.tnr].vp[0] = xmin;
	gc.xform[gc.tnr].vp[1] = xmax;
	}
    else {
	gc.xform[gc.tnr].vp[0] = vp[0];
	gc.xform[gc.tnr].vp[1] = vp[1];
	}
    if (ymax > ymin) {
	gc.xform[gc.tnr].vp[2] = ymin;
	gc.xform[gc.tnr].vp[3] = ymax;
	}
    else {
	gc.xform[gc.tnr].vp[2] = vp[2];
	gc.xform[gc.tnr].vp[3] = vp[3];
	}

    _SightChangeXform (gc.tnr);

    sight_save_needed = TRUE;
    if (gc.update == UpdateAlways)
	stat = SightRedraw (NIL);
    else
	stat = SIGHT__NORMAL;

    if (!odd(stat))
        {
        for (i = 0; i < 4; i++)
            gc.xform[gc.tnr].vp[i] = cur_vp[i];
        _SightChangeXform (gc.tnr);
        }

    return (stat);
    }



static
void MoveResizeViewport (float xmin, float xmax, float ymin, float ymax)
    {
	int i;
	float cur_vp[4], *req_vp;
	float x_fac, y_fac, x_rel, y_rel;
	SightDisplayList *p;

    for (i = 0; i < 4; i++) 
	cur_vp[i] = gc.xform[gc.tnr].vp[i];

    req_vp = gc.xform[gc.tnr].vp;
    if (xmax > xmin) {
	req_vp[0] = xmin;
	req_vp[1] = xmax;
	}
    else {
	req_vp[0] = vp[0];
	req_vp[1] = vp[1];
	}
    if (ymax > ymin) {
	req_vp[2] = ymin;
	req_vp[3] = ymax;
	}
    else {
	req_vp[2] = vp[2];
	req_vp[3] = vp[3];
	}

    x_fac = (req_vp[1]-req_vp[0])/(cur_vp[1]-cur_vp[0]);
    y_fac = (req_vp[3]-req_vp[2])/(cur_vp[3]-cur_vp[2]);
    
    _SightChangeXform (gc.tnr);

    p = root;
    while (p != NIL)
	{
	if (p->state == StateSelected)
	    {
	    if (p->type == SimpleText || p->type == Text)
		{
		x_rel = req_vp[0] + p->obj.text->x * x_fac;
		y_rel = req_vp[2] + p->obj.text->y * y_fac;
		MoveObject (p, x_rel, y_rel);
		}
	    else if (p->type == Image)
		{
		p->xmin = p->obj.image->x_min = req_vp[0];
		p->xmax = p->obj.image->x_max = req_vp[1];
		p->ymin = p->obj.image->y_min = req_vp[2];
		p->ymax = p->obj.image->y_max = req_vp[3];
		}
	    }
	p = p->next;
	}
    }



void SightSelectXform (int tnr)
    {
	int i;

    if (!sight_open || tnr == gc.tnr)
	return;

    if (tnr >= 0 && tnr < MAX_XFORMS)
	{
	for (i = 0; i < 4; i++) {
	    wn[i] = gc.xform[gc.tnr].wn[i];
	    vp[i] = gc.xform[gc.tnr].vp[i];
	    }
	gc.tnr = tnr;
        _SightChangeXform (gc.tnr);
	}
    }



int SightMoveResizeViewport (float xmin, float xmax, float ymin, float ymax)
    {
	int stat, i;
	float cur_vp[4];

    if (!sight_open)
	return SIGHT__CLOSED;

    for (i = 0; i < 4; i++) 
	cur_vp[i] = gc.xform[gc.tnr].vp[i];

    MoveResizeViewport (xmin, xmax, ymin, ymax);

    sight_save_needed = TRUE;
    if (gc.update == UpdateAlways)
	stat = SightRedraw (NIL);
    else
	stat = SIGHT__NORMAL;

    if (!odd(stat))
        {
        for (i = 0; i < 4; i++)
            gc.xform[gc.tnr].vp[i] = cur_vp[i];
        _SightChangeXform (gc.tnr);
        }

    return (stat);
    }



void SightSetOrientation (int orientation)
    {
    int saved_tnr;
    SightUpdateOption update;

    if (!sight_open)
	return;

    saved_tnr = gc.tnr;

    update = gc.update;
    gc.update = UpdateConditionally;

    SightDeselect ();

    gc.update = update;

    SightSelectXform (SightNDC);

    MoveResizeViewport (gc.xform[SightNDC].vp[2], gc.xform[SightNDC].vp[3],
        gc.xform[SightNDC].vp[0], gc.xform[SightNDC].vp[1]);

    sight_orientation = orientation;
    if (sight_orientation == OrientationLandscape)
        SightConfigure (WindowWidth, WindowHeight);
    else
        SightConfigure (WindowHeight, WindowWidth);

    if (root == NIL)
	_SightInit ();

    SightSelectXform (saved_tnr);
    }



void SightSetScale (int scale)
    {
	int stat, cur_scale;
	SightDisplayList *p;
	float *wn, x_min, x_max, y_min, y_max;
	int flip_x, flip_y;

    if (!sight_open)
	SightOpen (NIL);

    if (scale != gc.xform[gc.tnr].scale)
	{
    	cur_scale = gc.xform[gc.tnr].scale;
	gc.xform[gc.tnr].scale = scale;

	wn = gc.xform[gc.tnr].wn;
	
	p = root;
	while (p != NIL) 
	    {
	    if (p->type == Axes && p->tnr == gc.tnr) 
		{
		flip_x = option_flip_x & cur_scale;
		x_min = flip_x ? wn[1] : wn[0];
		x_max = flip_x ? wn[0] : wn[1];

		flip_x = option_flip_x & scale;
		if (fabs(p->obj.axes->x_org - x_min) < EPS)
		    p->obj.axes->x_org = flip_x ? wn[1] : wn[0];
		else if (fabs(p->obj.axes->x_org - x_max) < EPS)
		    p->obj.axes->x_org = flip_x ? wn[0] : wn[1];

		flip_y = option_flip_y & cur_scale;
		y_min = flip_y ? wn[3] : wn[2];
		y_max = flip_y ? wn[2] : wn[3];

		flip_y = option_flip_y & scale;
		if (fabs(p->obj.axes->y_org - y_min) < EPS)
		    p->obj.axes->y_org = flip_y ? wn[3] : wn[2];
		else if (fabs(p->obj.axes->y_org - y_max) < EPS)
		    p->obj.axes->y_org = flip_y ? wn[2] : wn[3];
		}

	    p = p->next;
	    }

        sight_save_needed = TRUE;
	if (gc.update == UpdateAlways)
	    stat = SightRedraw (NIL);
	else
	    stat = SIGHT__NORMAL;

	if (!odd(stat))
	    {
	    gc.xform[gc.tnr].scale = cur_scale;
	    _SightChangeXform (gc.tnr);
	    }
	}
    }



int SightAutoscale (float xmin, float xmax, float ymin, float ymax, BOOL adjust)
    {
	int stat, i;
	float x0, x1, y0, y1, cur_wn[4];
	int options;
	SightDisplayList *p;

    if (!sight_open)
	return SIGHT__CLOSED;

    if ((xmin < xmax) && (ymin < ymax))
	{
	for (i = 0; i < 4; i++) 
	    cur_wn[i] = gc.xform[gc.tnr].wn[i];

	_SightSaveXform ();
	_SightChangeXform (gc.tnr);

	x0 = xmin; 
	x1 = xmax; 
	y0 = ymin;
	y1 = ymax;

	options = gc.xform[gc.tnr].scale;

	if (adjust)
	    {
	    if (!(option_x_log & options))
		gus_adjust_range (&x0, &x1);
	    if (!(option_y_log & options))
		gus_adjust_range (&y0, &y1);
	    }

	gc.xform[gc.tnr].wn[0] = x0;
	gc.xform[gc.tnr].wn[1] = x1;
	gc.xform[gc.tnr].wn[2] = y0;
	gc.xform[gc.tnr].wn[3] = y1;

        _SightChangeXform (gc.tnr);
	
	p = root;
	while (p != NIL) 
	    {
	    if (p->type == Axes && p->tnr == gc.tnr) 
		{
		if (fabs(p->obj.axes->x_org - p->xmin) < fabs(p->xmin*EPS))
		    p->obj.axes->x_org = option_flip_x & options ? x1 : x0;
		else if (fabs(p->obj.axes->x_org - p->xmax) < fabs(p->xmax*EPS))
		    p->obj.axes->x_org = option_flip_x & options ? x0 : x1;
		else
		    p->obj.axes->x_org = x0 +
			(p->obj.axes->x_org - p->xmin) /
			(p->xmax - p->xmin) * (x1 - x0);

		p->xmin = x0;
		p->xmax = x1;

		if (fabs(p->obj.axes->y_org - p->ymin) < fabs(p->ymin*EPS))
		    p->obj.axes->y_org = option_flip_y & options ? y1 : y0;
		else if (fabs(p->obj.axes->y_org - p->ymax) < fabs(p->ymax*EPS))
		    p->obj.axes->y_org = option_flip_y & options ? y0 : y1;
		else
		    p->obj.axes->y_org = y0 + 
			(p->obj.axes->y_org - p->ymin) /
			(p->ymax - p->ymin) * (y1 - y0);

		p->ymin = y0;
		p->ymax = y1;

		p->obj.axes->x_tick = gus_tick(&x0, &x1);
		p->obj.axes->y_tick = gus_tick(&y0, &y1);
		}
	    p = p->next;
	    }

	_SightRestoreXform ();

        sight_save_needed = TRUE;
	if (gc.update == UpdateAlways)
	    stat = SightRedraw (NIL);
	else
	    stat = SIGHT__NORMAL;

	if (!odd(stat))
	    {
	    for (i = 0; i < 4; i++) 
		gc.xform[gc.tnr].wn[i] = cur_wn[i]; 
            _SightChangeXform (gc.tnr);
	    }
	}

    return (stat);
    }




int SightPlot (int np, float *px, float *py)
    {
	int stat;
	int n = np;
	float min_x, max_x, min_y, max_y;

	float x_tick, y_tick;
    
    if (!sight_open)
	return SIGHT__CLOSED;

    if (n > 1)
        {
	gus_autoscale (&n, px, py, &min_x, &max_x, &min_y, &max_y, NIL);

        x_tick = gus_tick(&min_x, &max_x);
        y_tick = gus_tick(&min_y, &max_y);

        stat = SightSetWindow (min_x, max_x, min_y, max_y);
	stat = SightDrawAxes (x_tick, y_tick, min_x, min_y, 1, 1, 0.01);
	stat = SightPolyline (n, px, py);
        }

    return (stat);
    }



static
int NumberOfObjects (void)
    {
	int n;
	SightDisplayList *p;

    n = 0;
    p = root;
    while (p != NIL)
	{
	if (p->state != StateDeleted && p->state != StateCut)
	    n++;

	p = p->next;
	}

    return (n);
    }



static
void UsedXforms (int *n, int *used_xform)
    {
	int i, count;
	SightDisplayList *p;

    for (i = 1; i < MAX_XFORMS; i++)
	used_xform[i] = FALSE;

    p = root;
    while (p != NIL)
	{
	used_xform[p->tnr] = TRUE;
	p = p->next;
	}
    used_xform[0] = 0;

    count = 0;
    for (i = 1; i < MAX_XFORMS; i++)
	if (used_xform[i])
	    used_xform[i] = ++count;

    *n = count;    
    }



static
void ReadLines (FILE *file, int n)
    {
	char buffer[256];
	int i;

    for (i = 0; i < n; i++)
	fgets (buffer, 256, file);

    tt_printf ("%1d line(s) skipped.\n", n); 
    }


static char Sif[4] = { "SIF" };


int SightExportDrawing (char *filename, BOOL autosave)
/* 
 * SightExportDrawing - Subroutine to store a drawing in 
 *			Sight Interchange Format (SIF)
 */
   {
	int i, n, n_rec, stat;
	int n_used, used_xform[MAX_XFORMS];
	String equ_string;
	time_t date;
	SifType type;
	SightHeader header;
	SightDisplayList *p;
	FILE *file;
	
    if (sight_open)
	{
	if (root != NIL)
	    {
            if (!autosave)
                SightDeselect ();

	    file = fopen(filename, "w");

	    if (file) {

		stat = SIGHT__NORMAL;

		/* write file header */

		type = SifHeader;

		time (&date);
		strcpy (header.creator, SightCreator);
		strcpy (header.date, ctime(&date));
		strcpy (header.filename, filename);
		header.version = SightVersionID;
		header.orientation = (SightOrientation)sight_orientation;
		header.records = 2 + NumberOfObjects ();

		n_rec = 6;

		fprintf (file, "%s %2d %d\n", Sif, type, n_rec);

		fprintf (file, "%d %s\n", strlen(header.creator), 
		    header.creator);
		fprintf (file, "%d %s\n", strlen(header.date), header.date);
		fprintf (file, "%d\n", header.version);
		fprintf (file, "%d %s\n", strlen(header.filename), 
		    header.filename);
		fprintf (file, "%d\n", header.orientation);
		fprintf (file, "%d\n", header.records);
		

		/* write graphic context */

		type = SifGC;

		/* calculate used transformations */

		UsedXforms (&n_used, used_xform);

		n_rec = 8 + n_used;

		fprintf (file, "%s %2d %d\n", Sif, type, n_rec);

		fprintf (file, "%2d %g %2d\n", 
		    gc.line.linetype, 
		    gc.line.linewidth, 
		    gc.line.linecolor);

		fprintf (file, "%2d %g %2d\n", 
		    gc.marker.markertype,
		    gc.marker.markersize, 
		    gc.marker.markercolor);

		fprintf (file, "%g %2d %2d %g %2d\n", 
		    gc.bar.linewidth, 
		    gc.bar.linecolor, 
		    gc.bar.markertype, 
		    gc.bar.markersize, 
		    gc.bar.markercolor);

		fprintf (file, "%2d %2d %2d\n", 
		    gc.fill.fillstyle, 
		    gc.fill.fillindex, 
		    gc.fill.fillcolor);

		fprintf (file, "%2d %g %2d %2d %2d %2d\n", 
		    gc.text.textfont,
		    gc.text.textsize, 
		    gc.text.textvalign,
		    gc.text.texthalign, 
		    gc.text.textdirection,
		    gc.text.textcolor);

		fprintf (file, "%g %2d %2d %g %2d\n", 
		    gc.axes.linewidth,
		    gc.axes.linecolor, 
		    gc.axes.textfont, 
		    gc.axes.textsize, 
		    gc.axes.textcolor);

		fprintf (file, "%2d %2d %2d\n", 
		    (int)gc.update, gc.clsw, 1);

		/* write used transformations */
		
		fprintf (file, "%3d\n", n_used);

		for (i = 1; i < MAX_XFORMS; i++) 
		    { 
		    if (used_xform[i])
			{
			fprintf (file, "%2d %g %g %g %g %g %g %g %g\n", 
			    gc.xform[i].scale, 
			    gc.xform[i].vp[0], gc.xform[i].vp[1], 
			    gc.xform[i].vp[2], gc.xform[i].vp[3],
			    gc.xform[i].wn[0], gc.xform[i].wn[1], 
			    gc.xform[i].wn[2], gc.xform[i].wn[3]);
			}
		    }


		/* write objects */

		type = SifObject;

		p = root;
		while (p != NIL) 
		    {
		    if (p->state != StateDeleted && p->state != StateCut)
			{
			switch (p->type) {
			    case Pline :
			    case Pmarker :
			    case FillArea :
			    case BarGraph :
				n = p->obj.data->n;
				n_rec = 3 + (int)(n/10)+1;

				fprintf (file, "%s %2d %d\n", Sif, type, n_rec);

				fprintf (file, "%2d %2d %2d %2d %2d", 
				    (int)p->type, (int)p->state, p->clsw, 
				    used_xform[p->tnr], p->depth);

				fprintf (file, " %g %g %g %g\n", 
				    p->xmin, p->xmax, p->ymin, p->ymax);

				fprintf (file, "%d", n);
				
				for (i = 0; i < n; i++)
				    {
				    if (!(i % 10))
					fprintf (file, "\n");
				    fprintf (file, " %g %g", 
					p->obj.data->x[i], p->obj.data->y[i]);
				    }
				fprintf (file, "\n");

				switch (p->type) {
				    case Pline :
					fprintf (file, " %2d %g %2d\n", 
					    p->gc.line.linetype, 
					    p->gc.line.linewidth, 
					    p->gc.line.linecolor);
					break;
				    case Pmarker :
					fprintf (file, " %2d %g %2d\n", 
					    p->gc.marker.markertype, 
					    p->gc.marker.markersize, 
					    p->gc.marker.markercolor);
					break;
				    case FillArea :
				    case BarGraph :
					fprintf (file, " %2d %2d %2d\n", 
					    p->gc.fill.fillstyle, 
					    p->gc.fill.fillindex, 
					    p->gc.fill.fillcolor);
					break;
				    }
				break;

			    case Spline :
				n = p->obj.spline->n;

				n_rec = 3 + (int)(n/10)+1;

				fprintf (file, "%s %2d %d\n", Sif, type, n_rec);

				fprintf (file, "%2d %2d %2d %2d %2d", 
				    (int)p->type, (int)p->state, p->clsw, 
				    used_xform[p->tnr], p->depth);

				fprintf (file, " %g %g %g %g", 
				    p->xmin, p->xmax, p->ymin, p->ymax);

				fprintf (file, " %g\n", p->obj.spline->smooth);

				fprintf (file, "%d", n);

				for (i = 0; i < n; i++) 
				    {
				    if (!(i % 10))
					fprintf (file, "\n");
				    fprintf (file, " %g %g", 
					p->obj.spline->x[i], 
					p->obj.spline->y[i]);
				    }
				fprintf (file, "\n");

				fprintf (file, " %2d %g %2d\n", 
				    p->gc.line.linetype,
				    p->gc.line.linewidth, 
				    p->gc.line.linecolor);
				break;

			    case ErrorBars :
				n = p->obj.bar->n;

				n_rec = 3 + (int)(n/5)+1;

				fprintf (file, "%s %2d %d\n", Sif, type, n_rec);

				fprintf (file, "%2d %2d %2d %2d %2d", 
				    (int)p->type, (int)p->state, p->clsw, 
				    used_xform[p->tnr], p->depth);

				fprintf (file, " %g %g %g %g", 
				    p->xmin, p->xmax, p->ymin, p->ymax);

				fprintf (file, " %2d\n", 
				    (int)p->obj.bar->orientation);

				fprintf (file, "%d", n);

				for (i = 0; i < n; i++) 
				    {
				    if (!(i % 5))
					fprintf (file, "\n");
				    fprintf (file, " %g %g %g %g", 
					p->obj.bar->x[i], p->obj.bar->y[i],  
					p->obj.bar->e1[i], p->obj.bar->e2[i]);
				    }
				fprintf (file, "\n");

				fprintf (file, " %g %2d %2d %g %2d\n", 
				    p->gc.bar.linewidth, 
				    p->gc.bar.linecolor, 
				    p->gc.bar.markertype, 
				    p->gc.bar.markersize, 
				    p->gc.bar.markercolor);
				break;

			    case SimpleText :
			    case Text :
				translate_symbol (p->obj.text->chars, 
				    equ_string);

				n = strlen(equ_string) + 1;

				n_rec = 4;

				fprintf (file, "%s %2d %d\n", Sif, type, n_rec);

				fprintf (file, "%2d %2d %2d %2d %2d", 
				    (int)p->type, (int)p->state, p->clsw, 
				    used_xform[p->tnr], p->depth);

				fprintf (file, " %g %g %g %g\n", 
				    p->xmin, p->xmax, p->ymin, p->ymax);

				fprintf (file, "%d\n", n);
				fprintf (file, "%g %g %*s\n", 
				    p->obj.text->x, p->obj.text->y, n, 
				    equ_string);

				fprintf (file, " %2d %g %2d %2d %2d %2d\n", 
				    p->gc.text.textfont, 
				    p->gc.text.textsize, 
				    p->gc.text.textvalign, 
				    p->gc.text.texthalign, 
				    p->gc.text.textdirection, 
				    p->gc.text.textcolor);
				break;

			    case Axes :
			    case Grid :
				n_rec = 3;

				fprintf (file, "%s %2d %d\n", Sif, type, n_rec);

				fprintf (file, "%2d %2d %2d %2d %2d", 
				    (int)p->type, (int)p->state, p->clsw, 
				    used_xform[p->tnr], p->depth);

				fprintf (file, " %g %g %g %g\n", 
				    p->xmin, p->xmax, p->ymin, p->ymax);

				fprintf (file, "%g %g %g %g %d %d %g\n", 
				    p->obj.axes->x_tick, p->obj.axes->y_tick,
				    p->obj.axes->x_org,p->obj.axes->y_org,
				    p->obj.axes->maj_x,p->obj.axes->maj_y,
				    p->obj.axes->tick_size);

				fprintf (file, " %g %2d %2d %g %2d\n", 
				    p->gc.axes.linewidth, 
				    p->gc.axes.linecolor, 
				    p->gc.axes.textfont, 
				    p->gc.axes.textsize, 
				    p->gc.axes.textcolor);
				break;
    	    	    	    
    	    	    	    case Image :
				n = strlen(p->obj.image->filename) + 1;

    	    	    	    	n_rec = 4;
    	    	    	    	fprintf (file, "%s %2d %d\n", Sif, type, n_rec);

				fprintf (file, "%2d %2d %2d %2d %2d",
				    (int)p->type, (int)p->state, p->clsw, 
				    used_xform[p->tnr], p->depth);

				fprintf (file, " %g %g %g %g\n", 
				    p->xmin, p->xmax, p->ymin, p->ymax);

				fprintf (file, "%d\n", n);
				fprintf (file, "%*s\n",
				    n, p->obj.image->filename);

				fprintf (file, "%d %d %d %d %g %g %g %g\n", 
				    p->obj.image->startx, p->obj.image->starty,
    	    	    	    	    p->obj.image->sizex, p->obj.image->sizey,
    	    	    	    	    p->obj.image->x_min, p->obj.image->x_max,
    	    	    	    	    p->obj.image->y_min, p->obj.image->y_max);
    	    	    	    	break;    	    	    	    

			    case BeginSegment :
			    case EndSegment :
				n_rec = 2;

				fprintf (file, "%s %2d %d\n", Sif, type, n_rec);

				fprintf (file, "%2d %2d %2d %2d %2d\n", 
				    (int)p->type, (int)p->state, p->clsw, 0, 
				    p->depth);

				fprintf (file, "%g %g %g %g\n", 
				    0.0, 1.0, 0.0, 1.0);
				break;
			    }
			}
		    p = p->next;
		    }

		if (fclose (file))
		    stat = SIGHT__CLOSEFAI; /* File close failure */
		}
	    else
		stat = SIGHT__OPENFAI; /* File open failure */
	    }
	else
	    stat = SIGHT__NOOBJECT;  /* No objects found */
	}
    else
	stat = SIGHT__CLOSED;  /* Sight not open */
	
    return (stat);
    }



void SightAutoSave (void)
/* 
 * SightAutoSave - store a drawing (autosave)
 */
    {
    if (sight_open)
	{
	if (sight_save_needed)
	    SightExportDrawing ("auto.sight", TRUE);
	}
    }


int SightImportDrawing (char *filename)
/* 
 * SightImportDrawing - Subroutine to interpret a saved drawing in 
 *			Sight Interchange Format (SIF)
 */
   {
	int i, n, stat;
	int n_used, used_xform[MAX_XFORMS];
	SightDisplayList *p, *last;
	SightGC new_gc; 
	SightDisplayList *new_root;

	SightHeader header;
	char ident[4];
	int n_rec, record_nr, count;
	SifType type;
	FILE *file;

    if (!sight_open)
	SightOpen (NIL);

    file = fopen(filename, "r");

    if (file) {
	
	stat = SIGHT__NORMAL;

	record_nr = 0;
	new_root = NIL;

	/* read file header */
	
	count = fscanf (file, "%s %d %d", ident, &type, &n_rec);
	if (count != 3)
	    {
	    stat = SIGHT__READFAI; /* unrecoverable read error */
	    goto file_read_error;
	    }
	record_nr++;	    

	if (strncmp(ident, Sif, strlen(Sif)) != 0)
	    {
	    stat = SIGHT__INVSIFID; /* invalid SIF identification */
	    goto file_read_error;
	    }

	if (type != SifHeader)
	    {
	    stat = SIGHT__INVHEAD; /* invalid header */
	    goto file_read_error;
	    }

	fscanf (file, "%d", &n);
	fgetc (file);
	fread (header.creator, n, 1, file);
	header.creator[n] = '\0';

	fscanf (file, "%d", &n);
	fgetc (file);
	fread (header.date, n, 1, file);
	header.date[n] = '\0';

	fscanf (file, "%d", &header.version);

	fscanf (file, "%d", &n);
	fgetc (file);
	fread (header.filename, n, 1, file);
	header.filename[n] = '\0';

	fscanf (file, "%d", &header.orientation);
	fscanf (file, "%d", &header.records);


	/* read graphic context */

	count = fscanf (file, "%s %d %d", ident, &type, &n_rec);
	if (count != 3)
	    {
	    stat = SIGHT__READFAI; /* unrecoverable read error */
	    goto file_read_error;
	    }
	record_nr++;	    

	if (strncmp(ident, Sif, strlen(Sif)) != 0)
	    {
	    stat = SIGHT__INVSIFID; /* invalid SIF identification */
	    goto file_read_error;
	    }

	if (type != SifGC)
	    {
	    stat = SIGHT__INVGC; /* invalid GC */
	    goto file_read_error;
	    }

	count = fscanf (file, "%d %e %d", 
	    &new_gc.line.linetype, &new_gc.line.linewidth, &new_gc.line.linecolor);
	if (count != 3)
	    {
	    stat = SIGHT__READFAI; /* unrecoverable read error */
	    goto file_read_error;
	    }

	count = fscanf (file, "%d %e %d", 
	    &new_gc.marker.markertype, &new_gc.marker.markersize, &new_gc.marker.markercolor);
	if (count != 3)
	    {
	    stat = SIGHT__READFAI; /* unrecoverable read error */
	    goto file_read_error;
	    }

	count = fscanf (file, "%e %d %d %e %d", 
	    &new_gc.bar.linewidth, &new_gc.bar.linecolor, &new_gc.bar.markertype, 
	    &new_gc.bar.markersize, &new_gc.bar.markercolor);
	if (count != 5)
	    {
	    stat = SIGHT__READFAI; /* unrecoverable read error */
	    goto file_read_error;
	    }

	switch (header.version) {
	    case 1:
		count = fscanf (file, "%d %d", 
		    &new_gc.fill.fillstyle, &new_gc.fill.fillcolor);
		new_gc.fill.fillindex = 0;
		if (count != 2)
		    {
		    stat = SIGHT__READFAI; /* unrecoverable read error */
		    goto file_read_error;
		    }
		break;
	    case 2:
	    case 3:
		count = fscanf (file, "%d %d %d", 
		    &new_gc.fill.fillstyle, &new_gc.fill.fillindex,
		    &new_gc.fill.fillcolor);
		if (count != 3)
		    {
		    stat = SIGHT__READFAI; /* unrecoverable read error */
		    goto file_read_error;
		    }
		break;
	    }

	count = fscanf (file, "%d %e %d %d %d %d", 
	    &new_gc.text.textfont, &new_gc.text.textsize, &new_gc.text.textvalign,
	    &new_gc.text.texthalign, &new_gc.text.textdirection, &new_gc.text.textcolor);
	if (count != 6)
	    {
	    stat = SIGHT__READFAI; /* unrecoverable read error */
	    goto file_read_error;
	    }
        if (header.version < 3)
            new_gc.text.textsize *= (CapSize / Ratio);

	count = fscanf (file, "%e %d %d %e %d", 
	    &new_gc.axes.linewidth, &new_gc.axes.linecolor, &new_gc.axes.textfont, 
	    &new_gc.axes.textsize, &new_gc.axes.textcolor);
	if (count != 5)
	    {
	    stat = SIGHT__READFAI; /* unrecoverable read error */
	    goto file_read_error;
	    }
        if (header.version < 3)
            new_gc.axes.textsize *= (CapSize / Ratio);

	count = fscanf (file, "%d %d %d", 
	    &new_gc.update, &new_gc.clsw, &new_gc.tnr);
	if (count != 3)
	    {
	    stat = SIGHT__READFAI; /* unrecoverable read error */
	    goto file_read_error;
	    }


	/* calculate used transformations */
	UsedXforms (&n_used, used_xform);

	for (i = 1; i < MAX_XFORMS; i++)
	    {
	    if (used_xform[i] < i)
		new_gc.xform[used_xform[i]] = gc.xform[i];

	    new_gc.xform[i] = gc.xform[i];
	    }

	/* read transformations */

	count = fscanf (file, "%d", &n);
	if (count != 1)
	    {
	    stat = SIGHT__READFAI; /* unrecoverable read error */
	    goto file_read_error;
	    }
	    
	for (i = 1+n_used; i <= n+n_used; i++)
	    {
	    count = fscanf (file, "%d %e %e %e %e %e %e %e %e", 
		&new_gc.xform[i].scale, 
		&new_gc.xform[i].vp[0], &new_gc.xform[i].vp[1], 
		&new_gc.xform[i].vp[2], &new_gc.xform[i].vp[3],
		&new_gc.xform[i].wn[0], &new_gc.xform[i].wn[1], 
		&new_gc.xform[i].wn[2], &new_gc.xform[i].wn[3]);
	    if (count != 9)
		{
		stat = SIGHT__READFAI; /* unrecoverable read error */
		goto file_read_error;
		}
	    }

	while (odd(stat) && !feof(file)) 
	    {
	    count = fscanf (file, "%s %d %d", ident, &type, &n_rec);
	    if (count != 3)
		{
		if (feof(file))
		    stat = SIGHT__NORMAL;  /* end of file reached */
		else
		    stat = SIGHT__READFAI; /* unrecoverable read error */

		goto end_of_loop;
		}
	    record_nr++;	    

	    if (strncmp(ident, Sif, strlen(Sif)) != 0)
		{
		stat = SIGHT__INVSIFID; /* invalid SIF identification */
		goto end_of_loop;
		}

	    if (type != SifObject)
		{
		stat = SIGHT__INVTYPE; /* invalid SIF type */
		ReadLines (file, n_rec);
		goto end_of_loop;
		}

	    p = (SightDisplayList *) malloc(sizeof(SightDisplayList));
	    p->prev = NIL;
	    p->next = NIL;

	    count = fscanf (file, "%d %d %d %d %d", 
		&p->type, &p->state, &p->clsw, &p->tnr, &p->depth);
	    if (count != 5)
		{
		stat = SIGHT__INVOBJ; /* invalid object */
		free (p);
		ReadLines (file, n_rec);
		goto end_of_loop;
		}

	    /* add new transformations */	

	    p->depth += depth;
	    if (p->tnr != 0)  p->tnr += n_used;

	    count = fscanf (file, "%e %e %e %e", 
		&p->xmin, &p->xmax, &p->ymin, &p->ymax);
	    if (count != 4)
		{
		stat = SIGHT__INVOBJ; /* invalid object */
		free (p);
		ReadLines (file, n_rec);
		goto end_of_loop;
		}
	
	    p->state = StateNormal;

	    switch (p->type) {
	      case Pline :
	      case Pmarker :
	      case FillArea :
	      case BarGraph :
		p->obj.data = (SightPoint *) malloc(sizeof(SightPoint));

		count = fscanf (file, "%d", &n);
		if (count != 1)
		    {
		    stat = SIGHT__INVOBJ; /* invalid object */
		    free (p->obj.data);
		    free (p);
		    ReadLines (file, n_rec);
		    goto end_of_loop;
		    }

		p->obj.data->n = n;
		p->obj.data->x = (float *) malloc(n*sizeof(float));
		p->obj.data->y = (float *) malloc(n*sizeof(float));

		for (i = 0; i < n; i++) 
		    {
		    count = fscanf (file, "%e %e", 
			&p->obj.data->x[i], &p->obj.data->y[i]);
		    if (count != 2)
			{
			stat = SIGHT__INVOBJ; /* invalid object */
			free (p->obj.data->x);
			free (p->obj.data->y);
			free (p->obj.data);
			free (p);
			ReadLines (file, n_rec);
			goto end_of_loop;
			}
		    }

		switch (p->type) {

		  case Pline :
		    count = fscanf (file, "%d %e %d", 
			&p->gc.line.linetype, &p->gc.line.linewidth, 
			&p->gc.line.linecolor);
		    if (count != 3)
			{
			stat = SIGHT__INVOBJ; /* invalid object */
			free (p->obj.data->x);
			free (p->obj.data->y);
			free (p->obj.data);
			free (p);
			ReadLines (file, n_rec);
			goto end_of_loop;
			}
		    break;

		  case Pmarker :
		    count = fscanf (file, "%d %e %d", 
			&p->gc.marker.markertype, &p->gc.marker.markersize, 
			&p->gc.marker.markercolor);
		    if (count != 3)
			{
			stat = SIGHT__INVOBJ; /* invalid object */
			free (p->obj.data->x);
			free (p->obj.data->y);
			free (p->obj.data);
			free (p);
			ReadLines (file, n_rec);
			goto end_of_loop;
		        }
		    break;

		  case FillArea :
		  case BarGraph :
		    switch (header.version) {
			case 1:
			    count = fscanf (file, "%d %d", 
				&p->gc.fill.fillstyle, &p->gc.fill.fillcolor);
			    p->gc.fill.fillindex = 0;
			    if (count != 2)
				{
				stat = SIGHT__INVOBJ; /* invalid object */
				free (p->obj.data->x);
				free (p->obj.data->y);
				free (p->obj.data);
				free (p);
				ReadLines (file, n_rec);
				goto end_of_loop;
				}
			    break;
			case 2:
			case 3:
			    count = fscanf (file, "%d %d %d", 
				&p->gc.fill.fillstyle, &p->gc.fill.fillindex,
				&p->gc.fill.fillcolor);
			    if (count != 3)
				{
				stat = SIGHT__INVOBJ; /* invalid object */
				free (p->obj.data->x);
				free (p->obj.data->y);
				free (p->obj.data);
				free (p);
				ReadLines (file, n_rec);
				goto end_of_loop;
				}
			    break;
			}
		    break;
		  }
		  break;

	      case Spline :
		p->obj.spline = (SightSpline *) malloc(sizeof(SightSpline));

		count = fscanf (file, "%e", &p->obj.spline->smooth);
		if (count != 1)
		    {
		    stat = SIGHT__INVOBJ; /* invalid object */
		    free (p->obj.spline);
		    free (p);
		    ReadLines (file, n_rec);
		    goto end_of_loop;
		    }

		count = fscanf (file, "%d", &n);
		if (count != 1)
		    {
		    stat = SIGHT__INVOBJ; /* invalid object */
		    free (p->obj.spline);
		    free (p);
		    ReadLines (file, n_rec);
		    goto end_of_loop;
		    }

		p->obj.spline->n = n;
		p->obj.spline->x = (float *) malloc(n*sizeof(float));
		p->obj.spline->y = (float *) malloc(n*sizeof(float));

		for (i = 0; i < n; i++) 
		    {
		    count = fscanf (file, "%e %e", 
			&p->obj.spline->x[i], &p->obj.spline->y[i]);
		    if (count != 2)
			{
			stat = SIGHT__INVOBJ; /* invalid object */
			free (p->obj.spline->x);
			free (p->obj.spline->y);
			free (p->obj.spline);
			free (p);
			ReadLines (file, n_rec);
			goto end_of_loop;
			}
		    }

		count = fscanf (file, "%d %e %d", 
		    &p->gc.line.linetype, &p->gc.line.linewidth, 
		    &p->gc.line.linecolor);
		if (count != 3)
		    {
		    stat = SIGHT__INVOBJ; /* invalid object */
		    free (p->obj.spline->x);
		    free (p->obj.spline->y);
		    free (p->obj.spline);
		    free (p);
		    ReadLines (file, n_rec);
		    goto end_of_loop;
		    }
		break;

	      case ErrorBars :
		p->obj.bar = (SightBar *) malloc(sizeof(SightBar));

		count = fscanf (file, "%d", &p->obj.bar->orientation);
		if (count != 1)
		    {
		    stat = SIGHT__INVOBJ; /* invalid object */
		    free (p->obj.bar);
		    free (p);
		    ReadLines (file, n_rec);
		    goto end_of_loop;
		    }

		count = fscanf (file, "%d", &n);
		if (count != 1)
		    {
		    stat = SIGHT__INVOBJ; /* invalid object */
		    free (p->obj.bar);
		    free (p);
		    ReadLines (file, n_rec);
		    goto end_of_loop;
		    }

		p->obj.bar->n = n;
		p->obj.bar->x = (float *) malloc(n*sizeof(float));
		p->obj.bar->y = (float *) malloc(n*sizeof(float));
		p->obj.bar->e1 = (float *) malloc(n*sizeof(float));
		p->obj.bar->e2 = (float *) malloc(n*sizeof(float));

		for (i = 0; i < n; i++) 
		    {
		    count = fscanf (file, "%e %e %e %e", 
			&p->obj.bar->x[i], &p->obj.bar->y[i],
			&p->obj.bar->e1[i], &p->obj.bar->e2[i]);
		    if (count != 4)
			{
			stat = SIGHT__INVOBJ; /* invalid object */
			free (p->obj.bar->x);
			free (p->obj.bar->y);
			free (p->obj.bar->e1);
			free (p->obj.bar->e2);
			free (p->obj.bar);
			free (p);
			ReadLines (file, n_rec);
			goto end_of_loop;
			}
		    }

		count = fscanf (file, "%e %d %d %e %d", 
		    &p->gc.bar.linewidth, &p->gc.bar.linecolor, 
		    &p->gc.bar.markertype, &p->gc.bar.markersize, 
		    &p->gc.bar.markercolor);
		if (count != 5)
		    {
		    stat = SIGHT__INVOBJ; /* invalid object */
		    free (p->obj.bar->x);
		    free (p->obj.bar->y);
		    free (p->obj.bar->e1);
		    free (p->obj.bar->e2);
		    free (p->obj.bar);
		    free (p);
		    ReadLines (file, n_rec);
		    goto end_of_loop;
		    }
		break;

	      case SimpleText :
	      case Text :
		p->obj.text = (SightString *) malloc(sizeof(SightString));

		count = fscanf (file, "%d", &n);
		if (count != 1)
		    {
		    stat = SIGHT__INVOBJ; /* invalid object */
		    free (p->obj.text);
		    free (p);
		    ReadLines (file, n_rec);
		    goto end_of_loop;
		    }

		p->obj.text->chars = (char *) malloc(n);

		count = fscanf (file, "%e %e",
		    &p->obj.text->x, &p->obj.text->y);
		if (count != 2)
		    {
		    stat = SIGHT__INVOBJ; /* invalid object */
		    free (p->obj.text->chars);
		    free (p->obj.text);
		    free (p);
		    ReadLines (file, n_rec);
		    goto end_of_loop;
		    }

		/* read over 2 blanks */
		fgetc(file);
		fgetc(file);

		fread (p->obj.text->chars, n-1, 1, file);
		p->obj.text->chars[n-1] = '\0';

		count = fscanf (file, "%d %e %d %d %d %d", 
		    &p->gc.text.textfont, &p->gc.text.textsize, 
		    &p->gc.text.textvalign, &p->gc.text.texthalign, 
		    &p->gc.text.textdirection, &p->gc.text.textcolor);
		if (count != 6)
		    {
		    stat = SIGHT__INVOBJ; /* invalid object */
		    free (p->obj.text->chars);
		    free (p->obj.text);
		    free (p);
		    ReadLines (file, n_rec);
		    goto end_of_loop;
		    }
                if (header.version < 3)
                    p->gc.text.textsize *= (CapSize / Ratio);
		break;

	      case Axes :
	      case Grid :
		p->obj.axes = (SightAxes *) malloc(sizeof(SightAxes));

		count = fscanf (file, "%e %e %e %e %d %d %e", 
		    &p->obj.axes->x_tick, &p->obj.axes->y_tick,
		    &p->obj.axes->x_org, &p->obj.axes->y_org,
		    &p->obj.axes->maj_x, &p->obj.axes->maj_y,
		    &p->obj.axes->tick_size);
		if (count != 7)
		    {
		    stat = SIGHT__INVOBJ; /* invalid object */
		    free (p->obj.axes);
		    free (p);
		    ReadLines (file, n_rec);
		    goto end_of_loop;
		    }

		count = fscanf (file, "%e %d %d %e %d", 
		    &p->gc.axes.linewidth, &p->gc.axes.linecolor, 
		    &p->gc.axes.textfont, &p->gc.axes.textsize, 
		    &p->gc.axes.textcolor);
		if (count != 5)
		    {
		    stat = SIGHT__INVOBJ; /* invalid object */
		    free (p->obj.axes);
		    free (p);
		    ReadLines (file, n_rec);
		    goto end_of_loop;
		    }
                if (header.version < 3)
                    p->gc.axes.textsize *= (CapSize / Ratio);
		break;

    	      case Image :
    	    	p->obj.image = (SightImage *) malloc(sizeof(SightImage));

		count = fscanf (file, "%d", &n);
		if (count != 1)
		    {
		    stat = SIGHT__INVOBJ; /* invalid object */
		    free (p->obj.image);
		    free (p);
		    ReadLines (file, n_rec);
		    goto end_of_loop;
		    }

		/* read over 2 blanks */
		fgetc(file);
		fgetc(file);

		fread (p->obj.image->filename, n-1, 1, file);
		p->obj.image->filename[n-1] = '\0';

    	    	count = fscanf(file, "%d %d %d %d %e %e %e %e",
    	    	    &p->obj.image->startx,
    	    	    &p->obj.image->starty,
    	    	    &p->obj.image->sizex,
    	    	    &p->obj.image->sizey,
    	    	    &p->obj.image->x_min,
    	    	    &p->obj.image->x_max,
    	    	    &p->obj.image->y_min,
    	    	    &p->obj.image->y_max);
		if (count != 8)
		    {
		    stat = SIGHT__INVOBJ; /* invalid object */
		    free (p->obj.image);
		    free (p);
		    ReadLines (file, n_rec);
		    goto end_of_loop;
		    }

    	    	p->obj.image->img = NULL;    
    	    	break;    

	      case BeginSegment :
	      case EndSegment :
		break;

	      default :
		/* free object space */

		stat = SIGHT__INVOBJ; /* invalid object */
		free (p);
		ReadLines (file, n_rec);
		goto end_of_loop;
	      }

	    if (new_root == NIL)
		{
		new_root = p;
		new_root->next = NIL;
		new_root->prev = NIL;
		last = new_root;
		}
	    else
		{
		last->next = p;
		p->next = NIL;
		p->prev = last;
		last = p;
		}

end_of_loop: ;
	    }

	if (new_root != NIL)
	    {
	    gc = new_gc;

	    /* reeorganize transformation numbers */
	    p = root;
	    while (p != NIL)
		{
		p->tnr = used_xform[p->tnr];		    
		p = p->next;
		}

	    p = LastObject ();
	    if (p == NIL)
		root = new_root;
	    else
		{
		p->next = new_root;
		new_root->prev = p;
		}
	    }

	if (header.orientation != sight_orientation)
	    SightSetOrientation (header.orientation);
	else
	    SightRedraw (NIL);

file_read_error: ;

	fclose (file);
	}
    else
	stat = SIGHT__OPENFAI; /* File open failure */
	
    return (stat);
    }
