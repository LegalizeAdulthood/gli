/*
 *
 * FACILITY:
 *
 *	VAX C Run-Time System
 *
 * ABSTRACT:
 *
 *	This module contains some definitions for the Simple interactive 
 *	graphics handling tool (Sight).
 *
 * AUTHOR:
 *
 *	Matthias Steinert 
 *
 * VERSION:
 *
 *	V1.2
 *
 */


#define CWhite		0     /* color white	    */
#define CBlack		1     /* color black	    */
#define CRed	   	2     /* color red	    */
#define CGreen		3     /* color green	    */
#define CBlue		4     /* color blue	    */
#define CCyan		5     /* color cyan	    */
#define CYellow		6     /* color yellow	    */
#define CMagenta	7     /* color magenta	    */

#define SightNDC	0
#define SightWC		1

#define MAX_XFORMS	256	    /* Maximum number of transformations */

#define SIGHT__NORMAL	0x085C8001  /* normal successful completion */
#define SIGHT__ERROR	0x085C8008  /* error status */
#define SIGHT__SEGOPE	0x085C8010  /* segment not closed */
#define SIGHT__CLOSED	0x085C8018  /* SIGHT not open */
#define SIGHT__NOSELECT	0x085C8020  /* no selection done */
#define SIGHT__NOOBJECT	0x085C8028  /* no objects found */
#define SIGHT__SEGCLO	0x085C8030  /* segment not open */
#define SIGHT__OPENFAI	0x085C8038  /* file open failure */
#define SIGHT__CLOSEFAI	0x085C8040  /* file close failure */
#define SIGHT__READFAI	0x085C8048  /* unrecoverable read error */
#define SIGHT__INVSIFID	0x085C8050  /* invalid SIF identification */
#define SIGHT__INVTYPE	0x085C8058  /* invalid SIF type */
#define SIGHT__INVHEAD	0x085C8060  /* invalid header */
#define SIGHT__INVGC	0x085C8068  /* invalid GC */
#define SIGHT__INVOBJ	0x085C8070  /* invalid object */
#define SIGHT__INVARG   0x085C8078  /* invalid argument */

#ifndef caddr_t
#define caddr_t char *
#endif


typedef char _SightFileSpecification[80];


typedef enum 
    _SightObjectType { 
	Pline, Spline, Pmarker, ErrorBars, FillArea, Text,
	Circle, FilledCircle, Rect, FilledRect,
	Axes, Grid,
	BeginSegment, EndSegment,
	BarGraph,
	SimpleText,
    	Image
} SightObjectType;


typedef enum 
    _SightAttribute { 
	LineType, LineWidth, LineColor, 
	MarkerType, MarkerSize, MarkerColor, 
	BarLineWidth, BarLineColor, BarMarkerType, BarMarkerSize,
	BarMarkerColor, 
	FillStyle, FillColor,  
	TextFont, TextSize, TextValign, TextHalign, TextDirection, TextColor,
	AxesLineWidth, AxesLineColor, AxesTextFont, AxesTextSize, AxesTextColor,
	FillIndex
} SightAttribute;


typedef enum 
    _SifType {
	SifHeader, SifGC, SifObject
} SifType;


typedef enum 
    _SightObjectState {
	StateNormal, StateSelected, StateCut, StateDeleted
} SightObjectState;


typedef enum
    _SightOrientation {
	OrientationPortrait, OrientationLandscape
} SightOrientation;


typedef enum 
    _SightClipOption {
	ClippingOff, ClippingOn
} SightClipOption;


typedef enum 
    _SightUpdateOption {
	UpdateConditionally, UpdateAlways
} SightUpdateOption;


typedef struct _SightPoint {
    int n;
    float *x, *y;
} SightPoint;

typedef struct _SightSpline {
    float smooth;
    int n;
    float *x, *y;
} SightSpline;

typedef enum _SightBarOrientation {
    SightVerticalBar, SightHorizontalBar
} SightBarOrientation;

typedef struct _SightBar {
    SightBarOrientation orientation;
    int n;
    float *x, *y, *e1, *e2;
} SightBar;

typedef struct _SightString {
    float x, y;
    char *chars;
} SightString;

typedef struct _SightAxes {
    float x_tick, y_tick, x_org, y_org;
    int maj_x, maj_y;
    float tick_size;
} SightAxes;


typedef struct _SightImage {
    image_dscr *img;
    _SightFileSpecification filename;
    int startx, starty, sizex, sizey;
    float x_min, x_max, y_min, y_max;
    char name[255];
} SightImage;    


typedef struct _SightLineGC {
    int linetype;
    float linewidth;
    int linecolor;
} SightLineGC;

typedef struct _SightMarkerGC {
    int markertype;
    float markersize;
    int markercolor;
} SightMarkerGC;

typedef struct _SightBarGC {
	float linewidth;
	int linecolor;
	int markertype;
	float markersize;
	int markercolor;
} SightBarGC;

typedef struct _SightFillGC_V1 {
	int fillstyle;
	int fillcolor;
} SightFillGC_V1;

typedef struct _SightFillGC {
	int fillstyle;
	int fillindex;
	int fillcolor;
} SightFillGC;

typedef struct _SightTextGC {
	int textfont;
	float textsize;
	int textvalign;
	int texthalign;
	int textdirection;
	int textcolor;
} SightTextGC;

typedef struct _SightAxesGC {
	float linewidth;
	int linecolor;
	int textfont;
	float textsize;
	int textcolor;
} SightAxesGC;

typedef union 
    _SightAnyGC {
	struct _SightLineGC line;
	struct _SightMarkerGC marker;
	struct _SightBarGC bar;
	struct _SightFillGC_V1 fill_v1;
	struct _SightFillGC fill;
	struct _SightTextGC text;
	struct _SightAxesGC axes;
} SightAnyGC;


typedef union _SightObject {
    SightPoint *data;
    SightPoint *line;
    SightSpline *spline;
    SightPoint *marker;
    SightBar *bar;
    SightPoint *fill;
    SightImage *image;
    SightString *text;
    SightAxes *axes;
    SightAxes *grid;
} SightObject;


typedef struct _SightDisplayList {
    SightObjectType type;
    SightObjectState state;
    int clsw, tnr;
    float xmin, ymin, xmax, ymax;
    struct _SightDisplayList *prev;
    struct _SightDisplayList *next;
    int depth;
    union _SightObject obj;
    union _SightAnyGC gc;
} SightDisplayList;


typedef struct _SightTransformation {
    int	    scale;	/* scaling indicator (logarithm, linear) */
    float   vp[4];	/* viewport */
    float   wn[4];	/* window */
} SightTransformation;


typedef struct _SightGC {
    SightLineGC line;			    /* polyline attributes */
    SightMarkerGC marker;		    /* polymarker attributes */
    SightBarGC bar;			    /* error bar attributes */
    SightFillGC fill;			    /* fill area attributes */
    SightTextGC text;			    /* text attributes */
    SightAxesGC axes;			    /* axes attributes */
    SightUpdateOption update;		    /* update flag */
    int clsw;				    /* clipping indicator */
    int	tnr;				    /* transformation number */
    SightTransformation xform[MAX_XFORMS];  /* transformation specifications */
} SightGC;



/* OLD DECLARATIONS */


typedef struct _SightOldAnyGC {
    int ref_count;
    SightAnyGC Any;
} SightOldAnyGC;

typedef union _SightOldObject {
    SightPoint *data;
    SightPoint *polyline;
    SightSpline *spline;
    SightPoint *polymarker;
    SightBar *bar;
    SightPoint *fill_area;
    SightString *text;
    SightAxes *axes;
    SightAxes *grid;
} SightOldObject;


typedef struct _SightOldDisplayList {
    SightObjectType type;
    SightObjectState state;
    int clsw, tnr;
    float xmin, ymin, xmax, ymax;
    struct _SightOldDisplayList *prev;
    struct _SightOldDisplayList *next;
    int depth;
    union _SightOldObject obj;
    struct _SightOldAnyGC *gc;
} SightOldDisplayList;


typedef struct _SightOldGC {
    SightAnyGC *line_gc;
    SightAnyGC *marker_gc;
    SightAnyGC *bar_gc;
    SightAnyGC *fill_gc;
    SightAnyGC *text_gc;
    SightAnyGC *axes_gc;
    float wn[4], vp[4];
    int clsw, tnr, scale;
} SightOldGC;


int _SightCreateObject (SightObjectType, caddr_t);
void SightClear (void);
void SightDelete (void);
void SightDeselect (void);
int SightSelectAll (void);
void SightDefineXY (int, float *, float *y);
void SightDefineX (char *, int, float *x);
void SightDefineY (char *, int, float *y);
void SightPickRegion (float, float, float, float);
void SightPickObject (float, float);
void SightPickElement (float, float);
void SightModifyObject (int, float *, float *, char *);
void SightModifyElement (int, float *, float *);
int SightSelectObject (float, float);
int SightSelectGroup (float, float, float, float);
int SightSelectLast (void);
int SightMove (float, float);
int SightCut (float, float);
int SightPaste (float, float);
int SightPushBehind (void);
int SightPopInFront (void);
int SightRedraw (SightDisplayList *);
int SightSelectNext (void);
int SightSelectPrevious (void);
int SightSelectExcluded (void);
int SightPolyline (int, float *, float *);
int SightDrawSpline (int, float *, float *, float);
int SightPolymarker (int, float *, float *);
int SightErrorBars (int, float *, float *, float *, float *,
    SightBarOrientation);
int SightImportImage (_SightFileSpecification, int, int, int, int,
    float, float, float, float);
int SightFillArea (int, float *, float *);
int SightBarGraph (int, float *, float *);
int SightText (float, float, char *);
int SightSimpleText (float, float, char *);
int SightDrawAxes (float, float, float, float, int, int, float);
int SightDrawGrid (float, float, float, float, int, int);
int SightOpenSegment (void);
int SightCloseSegment (void);
int SightAlign (void);
void SightSetAttribute (SightAttribute, caddr_t);
void SightSetSnapGrid (float);
void SightSetAxesXtick (float);
void SightSetAxesYtick (float);
void SightSetAxesXorg (float);
void SightSetAxesYorg (float);
void SightSetAxesMajorX (int);
void SightSetAxesMajorY (int);
void SightSetAxesTickSize (float);
void SightSetImageSize (int *);
void SightSetImagePosition (float *);
void SightSetSmoothing (float);
void SightSetClipping (int);
void SightSetUpdate (int);
void SightObjectID (SightDisplayList **);
int _SightGetTextSize (float);
void SightInquireObject (char *);
int SightOpenDrawing (char *);
int SightCapture (char *, int);
int SightSetWindow (float, float, float, float);
int SightSetViewport (float, float, float, float);
void SightSelectXform (int);
int SightMoveResizeViewport (float, float, float, float);
void SightSetOrientation (int);
void SightSetScale (int);
int SightAutoscale (float, float, float, float, int);
int SightPlot (int, float *, float *);
int SightExportDrawing (char *, int);
void SightAutoSave (void);
int SightImportDrawing (char *);

void _SightSetWindow (int, float *, float *);
void _SightChangeXform (int);
void _SightApplyXform (int tnr, float, float, float *, float *);
void _SightApplyInverseXform (int tnr, float, float, float *, float *);
void _SightDrawSnapGrid (int, float *, float *);
void _SightDrawBorder (float, float, float, float);
void _SightDrawPoint (float, float);
void _SightChangeGC (SightObjectType, SightAnyGC *);
void _SightChangeClipping (int);
void _SightDisplayObject (SightDisplayList *, unsigned *, int *);
void _SightSaveXform (void);
void _SightRestoreXform (void);
void _SightSaveClipping (void);
void _SightRestoreClipping (void);
void _SightSaveGC (void);
void _SightRestoreGC (void);
void _SightInit (void);
void SightOpen (int *);
void SightRequestLocatorNDC (float *, float *, int);
void SightRequestLocatorWC (float *, float *, int);
void _SightUpdate (void);
void SightRequestStroke (int *, float *, float *);
void SightRequestMarker (int *, float *, float *);
void SightSampleLocator (float *, float *, int);
void SightRequestRectangle (int, float *, float *, float *, float *, int);
void SightRequestString (char *);
void _SightInquireTextExtent (SightString *, SightTextGC *, float *, float *,
    float *, float *);
void SightClose (int *);
void SightResize (int, int);
int _SightOpenFigureFile (char *, int);
void _SightCloseFigureFile (void);
void _SightClearDisplay (void);

void SightConfigure(int, int);
void SightMainLoop(void);
