[ENVIRONMENT ('gksdefs.pen')]

MODULE	GKSDEFS;

{ Mnemonic PASCAL names and their values for GKS ENUMERATION type values }

CONST
	GKS_K_CONID_DEFAULT = 0;       { default connection identifier }

	{ workstation types }
CONST
	GKS_K_WSTYPE_DEFAULT = 0;       { default workstation }
	GKS_K_GKSM_OUTPUT = 2;          { GKS output metafile }
	GKS_K_GKSM_INPUT = 3;           { GKS input metafile }
	GKS_K_WSTYPE_WISS = 5;          { Workstation Independent Storage }
	GKS_K_CGM_OUTPUT = 7;           { CGM output metafile }
	GKS_K_VT330 = 16;               { DIGITAL VT330 b&w }
	GKS_K_VT340 = 17;               { DIGITAL VT340 color }
	GKS_K_LN03_PLUS = 38;           { LN03 Plus printer }
	GKS_K_VSI = 42;                 { DIGITAL uVAX I }
	GKS_K_VSII = 41;                { DIGITAL uVAX II }
	GKS_K_VSII_GPX = 41;            { DIGITAL uVAX II/GPX }
	GKS_K_VS2000 = 41;              { DIGITAL uVAX 2000 }
	GKS_K_LVP16A = 51;              { DIGITAL LVP16 }
	GKS_K_HP7475 = 51;              { Hewlett Packard HP7475 }
	GKS_K_HP7550 = 53;              { Hewlett Packard HP7550's }
	GKS_K_POSTSCRIPT = 61;          { Postscript }
	GKS_K_COLOR_POSTSCRIPT = 62;    { Color Postscript }
	GKS_K_TEK4014 = 72;             { Tektronix 4014 }
	GKS_K_TEK4107 = 82;             { Tektronix 4107 }
	GKS_K_LJ250_180DPI = 92;	{ Digital LJ250 180 DPI }
	GKS_K_DECWINDOWS_OUTPUT = 210;	{ DECwindows output }
	GKS_K_DECWINDOWS = 211;		{ DECwindows output/input }
	GKS_K_DECWINDOWS_DRAWABLE = 212;{ DECwindows drawable }
	GKS_K_DECWINDOWS_WIDGET = 213;	{ DECwindows widget }
	GKS_K_MOTIF_OUTPUT = 230;	{ Motif output }
	GKS_K_MOTIF = 231;		{ Motif output/input }
	GKS_K_MOTIF_DRAWABLE = 232;	{ Motif drawable }
	GKS_K_MOTIF_WIDGET = 233;	{ Motif widget }
 
	{ input device types }
CONST
	GKS_K_INPUT_DEV_DEFAULT = 0;    { default input device }

	{ aspect source }
CONST
	GKS_K_ASF_BUNDLED = 0;          { aspect bundled }
	GKS_K_ASF_INDIVIDUAL = 1;       { aspect individual }

	{ clear control flag }
CONST
	GKS_K_CLEAR_CONDITIONALLY = 0;  { clear conditionally }
	GKS_K_CLEAR_ALWAYS = 1;         { clear always }
 
	{ clipping indicator }
CONST
	GKS_K_NOCLIP = 0;               { no clipping }
	GKS_K_CLIP = 1;                 { clipping }
 
	{ Color available }
CONST
	GKS_K_MONOCHROME = 0;           { monochrome display }
	GKS_K_COLOR = 1;                { color display }

	{ coordinate switch }
CONST
	GKS_K_COORDINATES_WC = 0;       { world coordinates }
	GKS_K_COORDINATES_NDC = 1;      { NDC }

	{ deferral mode }
CONST
	GKS_K_ASAP = 0;                 { as soon as possible }
	GKS_K_BNIG = 1;                 { before next global interaction }
	GKS_K_BNIL = 2;                 { before next interaction }
	GKS_K_ASTI = 3;                 { at some time }

	{ detectability }
CONST
	GKS_K_UNDETECTABLE = 0;         { undetectable }
	GKS_K_DETECTABLE = 1;           { detectable }

	{ device coordinate units }
CONST
	GKS_K_METERS = 0;               { meters }
	GKS_K_OTHER_UNITS = 1;          { other units }
 
	{ display surface empty }
CONST
	GKS_K_NOTEMPTY = 0;             { display surface not empty }
	GKS_K_EMPTY = 1;                { display surface empty }

	{ dynamic modification }
CONST
	GKS_K_IRG = 0;                  { implicit regeneration }
	GKS_K_IMM = 1;                  { immediate }

	{ echo switch }
CONST
	GKS_K_NOECHO = 0;               { echo disabled }
	GKS_K_ECHO = 1;                 { echo enabled }

	{ fill area interior style }
CONST
	GKS_K_INTSTYLE_HOLLOW = 0;      { interior style hollow }
	GKS_K_INTSTYLE_SOLID = 1;       { interior style solid }
	GKS_K_INTSTYLE_PATTERN = 2;     { interior style pattern }
	GKS_K_INTSTYLE_HATCH = 3;       { interior style hatch }

	{ highlighting }
CONST
	GKS_K_NORMAL = 0;               { unhighlighted }
	GKS_K_HIGHLIGHTED = 1;          { highlighted }

	{ input device status }
CONST
	GKS_K_STATUS_NONE = 0;          { no input obtained }
	GKS_K_STATUS_OK = 1;            { input obtained }
	GKS_K_STATUS_NOCHOICE = 2;      { no choice input }
	GKS_K_STATUS_NOPICK = 2;

	{ input class }
CONST
	GKS_K_INPUT_CLASS_NONE = 0;     { none	}
	GKS_K_INPUT_CLASS_LOCATOR = 1;  { locator }
	GKS_K_INPUT_CLASS_STROKE = 2;   { stroke }
	GKS_K_INPUT_CLASS_VALUATOR = 3; { valuator }
	GKS_K_INPUT_CLASS_CHOICE = 4;   { choice }
	GKS_K_INPUT_CLASS_PICK = 5;     { pick }
	GKS_K_INPUT_CLASS_STRING = 6;   { string }

	{ implicit regeneration mode }
CONST
	GKS_K_IRG_SUPPRESSED = 0;       { implicit regeneration suppressed }
	GKS_K_IRG_ALLOWED = 1;          { implicit regeneration allowed }

	{ level of GKS }
CONST
	GKS_K_LEVEL_0A = 0;             { level 0a }
	GKS_K_LEVEL_0B = 1;             { level 0b }
	GKS_K_LEVEL_0C = 2;             { level 0c }
	GKS_K_LEVEL_1A = 3;             { level 1a }
	GKS_K_LEVEL_1B = 4;             { level 1b }
	GKS_K_LEVEL_1C = 5;             { level 1c }
	GKS_K_LEVEL_2A = 6;             { level 2a }
	GKS_K_LEVEL_2B = 7;             { level 2b }
	GKS_K_LEVEL_2C = 8;             { level 2c }

	{ new frame action necessary }
CONST
	GKS_K_NEWFRAME_NOTNECESSARY = 0;{ new frame action not necessary on update }
	GKS_K_NEWFRAME_NECESSARY = 1;   { new frame necessary on update }
 
	{ operating mode }
CONST
	GKS_K_INPUT_MODE_REQUEST = 0;   { request mode }
	GKS_K_INPUT_MODE_SAMPLE = 1;    { sample mode }
	GKS_K_INPUT_MODE_EVENT = 2;     { event mode }

	{ operating state value }
CONST
	GKS_K_GKCL = 0;                 { GKS closed }
	GKS_K_GKOP = 1;                 { GKS open }
	GKS_K_WSOP = 2;                 { at least one workstation open }
	GKS_K_WSAC = 3;                 { at least one workstation active }
	GKS_K_SGOP = 4;                 { at least one segment open }

	{ presence of invalid values }
CONST
	GKS_K_INVALID_ABSENT = 0;       { invalid values absent }
	GKS_K_INVALID_PRESENT = 1;      { invalid values present }

	{ regeneration flag }
CONST
	GKS_K_POSTPONE_FLAG = 0;        { regeneration flag suppress }
	GKS_K_PERFORM_FLAG = 1;         { regeneration flag perform }

	{ relative input priority }
CONST
	GKS_K_INPUT_PRIORITY_HIGHER = 0;{ relative input priority higher }
	GKS_K_INPUT_PRIORITY_LOWER = 1; { relative input priority lower }

	{ simultaneous events flag }
CONST
	GKS_K_NOMORE_EVENTS = 0;        { nomore }
	GKS_K_MORE_EVENTS = 1;          { more }

	{ text alignment horizontal }
CONST
	GKS_K_TEXT_HALIGN_NORMAL = 0;   { horizontal align normal }
	GKS_K_TEXT_HALIGN_LEFT = 1;     { horizontal align left }
	GKS_K_TEXT_HALIGN_CENTER = 2;   { horizontal align center }
	GKS_K_TEXT_HALIGN_RIGHT = 3;    { horizontal align right }
 
	{ text alignment vertical }
CONST
	GKS_K_TEXT_VALIGN_NORMAL = 0;   { vertical align normal }
	GKS_K_TEXT_VALIGN_TOP = 1;      { vertical align left }
	GKS_K_TEXT_VALIGN_CAP = 2;      { vertical align cap }
	GKS_K_TEXT_VALIGN_HALF = 3;     { vertical align half }
	GKS_K_TEXT_VALIGN_BASE = 4;     { vertical align base }
	GKS_K_TEXT_VALIGN_BOTTOM = 5;   { vertical align bottom }

	{ text path }
CONST
	GKS_K_TEXT_PATH_RIGHT = 0;      { path right }
	GKS_K_TEXT_PATH_LEFT = 1;       { path left }
	GKS_K_TEXT_PATH_UP = 2;         { path up }
	GKS_K_TEXT_PATH_DOWN = 3;       { path down }

	{ text precision }
CONST
	GKS_K_TEXT_PRECISION_STRING = 0;{ text precision string }
	GKS_K_TEXT_PRECISION_CHAR = 1;  { text precision character }
	GKS_K_TEXT_PRECISION_STROKE = 2;{ text precision stroke }

	{ type of returned values }
CONST
	GKS_K_VALUE_SET = 0;            { returned value is set }
	GKS_K_VALUE_REALIZED = 1;       { returned value is realized }

	{ update state }
CONST
	GKS_K_NOTPENDING = 0;           { not pending }
	GKS_K_PENDING = 1;              { pending }

	{ vector/raster/other type }
CONST
	GKS_K_WSCLASS_VECTOR = 0;       { vector workstation }
	GKS_K_WSCLASS_RASTER = 1;       { raster workstation }
	GKS_K_WSCLASS_OTHERD = 2;       { other device }

	{ visibility }
CONST
	GKS_K_INVISIBLE = 0;            { ivisible }
	GKS_K_VISIBLE = 1;              { visible }

	{ workstation category }
CONST
	GKS_K_WSCAT_OUTPUT = 0;         { output workstation }
	GKS_K_WSCAT_INPUT = 1;          { input workstation }
	GKS_K_WSCAT_OUTIN = 2;          { output/input workstation }
	GKS_K_WSCAT_WISS = 3;           { workstation independent segment storage }
	GKS_K_WSCAT_MO = 4;             { metafile output }
	GKS_K_WSCAT_MI = 5;             { metafile input }

	{ workstation state }
CONST
	GKS_K_WS_INACTIVE = 0;          { work station active }
	GKS_K_WS_ACTIVE = 1;            { work station inactive }

	{ list of GDP attributes }
CONST
	GKS_K_POLYLN_ATTRI = 0;         { GDP polyline bundle }
	GKS_K_POLYMR_ATTRI = 1;         { GDP polymarker bundled }
	GKS_K_TEXT_ATTRI = 2;           { GDP text bundle }
	GKS_K_FILLAR_ATTRI = 3;         { GDP fill area bundle }

	{ standard linetypes }
CONST
	GKS_K_LINETYPE_SOLID = 1;       { linetype solid }
	GKS_K_LINETYPE_DASHED = 2;      { linetype dashed }
	GKS_K_LINETYPE_DOTTED = 3;      { linetype dotted }
	GKS_K_LINETYPE_DASHED_DOTTED = 4; { linetype dashed-dotted }
 
	{ GKS specific linetypes }
CONST
	GKS_K_LINETYPE_DASH_2_DOT = -1; { linetype dash-2-dots }
	GKS_K_LINETYPE_DASH_3_DOT = -2; { linetype dash-3-dots }
	GKS_K_LINETYPE_LONG_DASH = -3;  { linetype long-dash }
	GKS_K_LINETYPE_LONG_SHORT_DASH = -4; { linetype long-short-dash }
	GKS_K_LINETYPE_SPACED_DASH = -5; { linetype spaced-dash }
	GKS_K_LINETYPE_SPACED_DOT = -6; { linetype spaced-dot }
	GKS_K_LINETYPE_DOUBLE_DOT = -7; { linetype double dots }
	GKS_K_LINETYPE_TRIPLE_DOT = -8; { linetype triple dots }
 
	{ standard markertypes }
CONST
	GKS_K_MARKERTYPE_DOT = 1;       { markertype dot }
	GKS_K_MARKERTYPE_PLUS = 2;      { markertype plus }
	GKS_K_MARKERTYPE_ASTERISK = 3;  { markertype asterisk }
	GKS_K_MARKERTYPE_CIRCLE = 4;    { markertype circle }
	GKS_K_MARKERTYPE_DIAGONAL_CROSS = 5; { markertype diagonal cross }
 
	{ GKS specific markertypes }
CONST
	GKS_K_MARKERTYPE_SOLID_CIRCLE = -1; { markertype solid circle }
	GKS_K_MARKERTYPE_TRIANGLE_UP = -2; { markertype hollow up triangle }
	GKS_K_MARKERTYPE_SOLID_TRI_UP = -3; { markertype solid up triangle }
	GKS_K_MARKERTYPE_TRIANGLE_DOWN = -4; { markertype hollow down triangle }
	GKS_K_MARKERTYPE_SOLID_TRI_DOWN = -5; { markertype solid down triangle }
	GKS_K_MARKERTYPE_SQUARE = -6;   { markertype hollow square }
	GKS_K_MARKERTYPE_SOLID_SQUARE = -7; { markertype solid square }
	GKS_K_MARKERTYPE_BOWTIE = -8;   { markertype hollow bow tie }
	GKS_K_MARKERTYPE_SOLID_BOWTIE = -9; { markertype solid bow tie }
	GKS_K_MARKERTYPE_HOURGLASS = -10; { markertype hollow hour glass }
	GKS_K_MARKERTYPE_SOLID_HGLASS = -11; { markertype solid hour glass }
	GKS_K_MARKERTYPE_DIAMOND = -12; { markertype hollow diamond }
	GKS_K_MARKERTYPE_SOLID_DIAMOND = -13; { markertype solid diamond }
 
	{ escapes }
CONST
	GKS_K_ESC_SET_SPEED = -100;     { set speed }
	GKS_K_ESC_PRINT = -101;		{ print screen }

	GKS_K_ESC_WRITE_DISPLAY = -801; { write display }
	GKS_K_ESC_READ_DISPLAY = -802;  { read display }
	GKS_K_ESC_INTENSITY_SYSTEM = -901;{ intensity system }
	GKS_K_ESC_COLOR_SYSTEM = -902;	{ color system }
	GKS_K_ESC_SHADING_SYSTEM = -903;{ shading system }

TYPE
	GKS_ASF_ARRAY = ARRAY[1..13] OF INTEGER;

[HIDDEN]
TYPE
	BYTE = [BYTE] -128..127;


{ GRAPHIC KERNEL SYSTEM entry point descriptions }

[ASYNCHRONOUS, EXTERNAL(GOPKS_)] FUNCTION GKS_OPEN_GKS(
   %REF ERRFIL : INTEGER;
   %REF BUFA : INTEGER := -1) : INTEGER; FORTRAN;

[ASYNCHRONOUS, EXTERNAL(GCLKS_)] FUNCTION GKS_CLOSE_GKS 
	: INTEGER; FORTRAN;

[ASYNCHRONOUS, EXTERNAL(GOPWK_)] FUNCTION GKS_OPEN_WS(
   %REF WKID : INTEGER;
   %REF CONID : [UNSAFE] INTEGER;
   %REF WTYPE : INTEGER) : INTEGER; FORTRAN;

[ASYNCHRONOUS, EXTERNAL(GCLWK_)] FUNCTION GKS_CLOSE_WS(
   %REF WKID : INTEGER) : INTEGER; FORTRAN;

[ASYNCHRONOUS, EXTERNAL(GACWK_)] FUNCTION GKS_ACTIVATE_WS(
   %REF WKID : INTEGER) : INTEGER; FORTRAN;

[ASYNCHRONOUS, EXTERNAL(GDAWK_)] FUNCTION GKS_DEACTIVATE_WS(
   %REF WKID : INTEGER) : INTEGER; FORTRAN;

[ASYNCHRONOUS, EXTERNAL(GCLRWK_)] FUNCTION GKS_CLEAR_WS(
   %REF WKID, 
	COFL : INTEGER) : INTEGER; FORTRAN;

[ASYNCHRONOUS, EXTERNAL(GUWK_)] FUNCTION GKS_UPDATE_WS(
   %REF WKID : INTEGER;
   %REF REGFL : INTEGER) : INTEGER; FORTRAN;

[ASYNCHRONOUS, EXTERNAL(GSASF_)] FUNCTION GKS_SET_ASF(
   %REF FLAG : GKS_ASF_ARRAY) : INTEGER; FORTRAN;

[ASYNCHRONOUS, EXTERNAL(GPL_)] FUNCTION GKS_POLYLINE(
   %REF N : INTEGER;
   %REF PX, 
	PY : [UNSAFE] ARRAY[L..H : INTEGER] OF REAL) : INTEGER; FORTRAN;

[ASYNCHRONOUS, EXTERNAL(GPM_)] FUNCTION GKS_POLYMARKER(
   %REF N : INTEGER;
   %REF PX, 
	PY : [UNSAFE] ARRAY[L..H : INTEGER] OF REAL) : INTEGER; FORTRAN;

[ASYNCHRONOUS, EXTERNAL(gks_text_p)] FUNCTION GKS_TEXT(
	PX, 
	PY : REAL;
	CHARS : VARYING[_U3] OF CHAR) : INTEGER;
FORTRAN;

[ASYNCHRONOUS, EXTERNAL(GTXS_)] FUNCTION GKS_TEXT_S(
   %REF PX, 
	PY : REAL;
   %REF NCHARS : INTEGER;
   %REF	CHARS : PACKED ARRAY[L..H : INTEGER] OF CHAR) : INTEGER;
FORTRAN;

[ASYNCHRONOUS, EXTERNAL(GFA_)] FUNCTION GKS_FILL_AREA(
   %REF N : INTEGER;
   %REF PX, 
	PY : [UNSAFE] ARRAY[L..H : INTEGER] OF REAL) : INTEGER; FORTRAN;

[ASYNCHRONOUS, EXTERNAL(GSLN_)] FUNCTION GKS_SET_PLINE_LINETYPE(
   %REF LTYPE : INTEGER) : INTEGER; FORTRAN;

[ASYNCHRONOUS, EXTERNAL(GSLWSC_)] FUNCTION GKS_SET_PLINE_LINEWIDTH(
   %REF LWIDTH : REAL) : INTEGER; FORTRAN;

[ASYNCHRONOUS, EXTERNAL(GSPLCI_)] FUNCTION GKS_SET_PLINE_COLOR_INDEX(
   %REF COLI : INTEGER) : INTEGER; FORTRAN;

[ASYNCHRONOUS, EXTERNAL(GSMK_)] FUNCTION GKS_SET_PMARK_TYPE(
   %REF MTYPE : INTEGER) : INTEGER; FORTRAN;

[ASYNCHRONOUS, EXTERNAL(GSMKSC_)] FUNCTION GKS_SET_PMARK_SIZE(
   %REF MSZSC : REAL) : INTEGER; FORTRAN;

[ASYNCHRONOUS, EXTERNAL(GSPMCI_)] FUNCTION GKS_SET_PMARK_COLOR_INDEX(
   %REF COLI : INTEGER) : INTEGER; FORTRAN;

[ASYNCHRONOUS, EXTERNAL(GSTXFP_)] FUNCTION GKS_SET_TEXT_FONTPREC(
   %REF FONT, 
	PREC : INTEGER) : INTEGER; FORTRAN;

[ASYNCHRONOUS, EXTERNAL(GSCHXP_)] FUNCTION GKS_SET_TEXT_EXPFAC(
   %REF CHXP : REAL) : INTEGER; FORTRAN;

[ASYNCHRONOUS, EXTERNAL(GSCHSP_)] FUNCTION GKS_SET_TEXT_SPACING(
   %REF CHSP : REAL) : INTEGER; FORTRAN;

[ASYNCHRONOUS, EXTERNAL(GSTXCI_)] FUNCTION GKS_SET_TEXT_COLOR_INDEX(
   %REF COLI : INTEGER) : INTEGER; FORTRAN;

[ASYNCHRONOUS, EXTERNAL(GSCHH_)] FUNCTION GKS_SET_TEXT_HEIGHT(
   %REF CHH : REAL) : INTEGER; FORTRAN;

[ASYNCHRONOUS, EXTERNAL(GSCHUP_)] FUNCTION GKS_SET_TEXT_UPVEC(
   %REF CHUX, 
	CHUY : REAL) : INTEGER; FORTRAN;

[ASYNCHRONOUS, EXTERNAL(GSTXP_)] FUNCTION GKS_SET_TEXT_PATH(
   %REF TXP : INTEGER) : INTEGER; FORTRAN;

[ASYNCHRONOUS, EXTERNAL(GSTXAL_)] FUNCTION GKS_SET_TEXT_ALIGN(
   %REF TXALH, 
	TXALV : INTEGER) : INTEGER; FORTRAN;

[ASYNCHRONOUS, EXTERNAL(GSFAIS_)] FUNCTION GKS_SET_FILL_INT_STYLE(
   %REF INTS : INTEGER) : INTEGER; FORTRAN;

[ASYNCHRONOUS, EXTERNAL(GSFASI_)] FUNCTION GKS_SET_FILL_STYLE_INDEX(
   %REF STYLI : INTEGER) : INTEGER; FORTRAN;

[ASYNCHRONOUS, EXTERNAL(GSFACI_)] FUNCTION GKS_SET_FILL_COLOR_INDEX(
   %REF COLI : INTEGER) : INTEGER; FORTRAN;

[ASYNCHRONOUS, EXTERNAL(GSWN_)] FUNCTION GKS_SET_WINDOW(
   %REF TNR : INTEGER;
   %REF XMIN, 
	XMAX, 
	YMIN, 
	YMAX : REAL) : INTEGER; FORTRAN;

[ASYNCHRONOUS, EXTERNAL(GSVP_)] FUNCTION GKS_SET_VIEWPORT(
   %REF TNR : INTEGER;
   %REF XMIN, 
	XMAX, 
	YMIN, 
	YMAX : REAL) : INTEGER; FORTRAN;

[ASYNCHRONOUS, EXTERNAL(GSELNT_)] FUNCTION GKS_SELECT_XFORM(
   %REF TNR : INTEGER) : INTEGER; FORTRAN;

[ASYNCHRONOUS, EXTERNAL(GSCLIP_)] FUNCTION GKS_SET_CLIPPING(
   %REF CLSW : INTEGER) : INTEGER; FORTRAN;

[ASYNCHRONOUS, EXTERNAL(GSWKWN_)] FUNCTION GKS_SET_WS_WINDOW(
   %REF WKID : INTEGER;
   %REF XMIN, 
	XMAX, 
	YMIN, 
	YMAX : REAL) : INTEGER; FORTRAN;

[ASYNCHRONOUS, EXTERNAL(GSWKVP_)] FUNCTION GKS_SET_WS_VIEWPORT(
   %REF WKID : INTEGER;
   %REF XMIN, 
	XMAX, 
	YMIN, 
	YMAX : REAL) : INTEGER; FORTRAN;

[ASYNCHRONOUS, EXTERNAL(GSCR_)] FUNCTION GKS_SET_COLOR_REP(
   %REF WKID, 
	INDEX : INTEGER;
   %REF R, 
	G, 
	B : REAL) : INTEGER; FORTRAN;

[ASYNCHRONOUS, EXTERNAL(GRQLC_)] FUNCTION GKS_REQUEST_LOCATOR(
   %REF WKID, 
	LCDNR : INTEGER;
    VAR STAT, 
	TNR : INTEGER;
    VAR PX, 
	PY : REAL) : INTEGER; FORTRAN;

[ASYNCHRONOUS, EXTERNAL(GRQSK_)] FUNCTION GKS_REQUEST_STROKE(
   %REF WKID, 
	SKDNR, 
	N : INTEGER;
    VAR STAT, 
	TNR, 
	NP : INTEGER;
   %REF PX, 
	PY : [UNSAFE] ARRAY[L..H : INTEGER] OF REAL) : INTEGER; FORTRAN;

[ASYNCHRONOUS, EXTERNAL(GRQVL_)] FUNCTION GKS_REQUEST_VALUATOR(
   %REF WKID, 
	VLDNR : INTEGER;
    VAR STAT : INTEGER;
    VAR VAL : REAL) : INTEGER; FORTRAN;

[ASYNCHRONOUS, EXTERNAL(GRQCH_)] FUNCTION GKS_REQUEST_CHOICE(
   %REF WKID, 
	CHDNR : INTEGER;
    VAR STAT, 
	CHNR : INTEGER) : INTEGER; FORTRAN;

{ inquire functions }

[ASYNCHRONOUS, EXTERNAL(GQOPS_)] FUNCTION GKS_INQ_OPERATING_STATE(
    VAR OPSTA : INTEGER) : INTEGER; FORTRAN;

[ASYNCHRONOUS, EXTERNAL(GQLVKS_)] FUNCTION GKS_INQ_LEVEL(
    VAR ERRIND : INTEGER;
    VAR LEV : INTEGER) : INTEGER; FORTRAN;
 
[ASYNCHRONOUS, EXTERNAL(GQEWK_)] FUNCTION GKS_INQ_WSTYPE(
   %REF N : INTEGER;
    VAR	ERRIND, 
	NUMBER, 
	WTYPE : INTEGER) : INTEGER; FORTRAN;
 
[ASYNCHRONOUS, EXTERNAL(GQMNTN_)] FUNCTION GKS_INQ_MAX_XFORM(
    VAR ERRIND, 
	MAXTNR : INTEGER) : INTEGER; FORTRAN;
 
[ASYNCHRONOUS, EXTERNAL(GQOPWK_)] FUNCTION GKS_INQ_OPEN_WS(
   %REF N : INTEGER;
    VAR ERRIND, 
	OL, 
	WKID : INTEGER) : INTEGER; FORTRAN;
 
[ASYNCHRONOUS, EXTERNAL(GQACWK_)] FUNCTION GKS_INQ_ACTIVE_WS(
   %REF N : INTEGER;
    VAR ERRIND, 
	OL, 
	WKID : INTEGER) : INTEGER; FORTRAN;
 
[ASYNCHRONOUS, EXTERNAL(GQLN_)] FUNCTION GKS_INQ_PLINE_LINETYPE(
    VAR ERRIND, 
	LTYPE : INTEGER) : INTEGER; FORTRAN;

[ASYNCHRONOUS, EXTERNAL(GQLWSC_)] FUNCTION GKS_INQ_PLINE_LINEWIDTH(
    VAR ERRIND : INTEGER;
    VAR LWIDTH : REAL) : INTEGER; FORTRAN;

[ASYNCHRONOUS, EXTERNAL(GQPLCI_)] FUNCTION GKS_INQ_PLINE_COLOR_INDEX(
    VAR ERRIND, 
	COLI : INTEGER) : INTEGER; FORTRAN;

[ASYNCHRONOUS, EXTERNAL(GQMK_)] FUNCTION GKS_INQ_PMARK_TYPE(
    VAR ERRIND, 
	MTYPE : INTEGER) : INTEGER; FORTRAN;

[ASYNCHRONOUS, EXTERNAL(GQMKSC_)] FUNCTION GKS_INQ_PMARK_SIZE(
    VAR ERRIND : INTEGER;
    VAR MSZSC : REAL) : INTEGER; FORTRAN;

[ASYNCHRONOUS, EXTERNAL(GQPMCI_)] FUNCTION GKS_INQ_PMARK_COLOR_INDEX(
    VAR ERRIND, 
	COLI : INTEGER) : INTEGER; FORTRAN;

[ASYNCHRONOUS, EXTERNAL(GQTXFP_)] FUNCTION GKS_INQ_TEXT_FONTPREC(
    VAR ERRIND, 
	FONT, 
	PREC : INTEGER) : INTEGER; FORTRAN;

[ASYNCHRONOUS, EXTERNAL(GQCHXP_)] FUNCTION GKS_INQ_TEXT_EXPFAC(
    VAR ERRIND : INTEGER;
    VAR CHXP : REAL) : INTEGER; FORTRAN;

[ASYNCHRONOUS, EXTERNAL(GQCHSP_)] FUNCTION GKS_INQ_TEXT_SPACING(
    VAR ERRIND : INTEGER;
    VAR CHSP : REAL) : INTEGER; FORTRAN;

[ASYNCHRONOUS, EXTERNAL(GQTXCI_)] FUNCTION GKS_INQ_TEXT_COLOR_INDEX(
    VAR ERRIND, 
	COLI : INTEGER) : INTEGER; FORTRAN;

[ASYNCHRONOUS, EXTERNAL(GQCHH_)] FUNCTION GKS_INQ_TEXT_HEIGHT(
    VAR ERRIND : INTEGER;
    VAR CHH : REAL) : INTEGER; FORTRAN;

[ASYNCHRONOUS, EXTERNAL(GQCHUP_)] FUNCTION GKS_INQ_TEXT_UPVEC(
    VAR ERRIND : INTEGER;
    VAR CHUX, 
	CHUY : REAL) : INTEGER; FORTRAN;

[ASYNCHRONOUS, EXTERNAL(GQTXP_)] FUNCTION GKS_INQ_TEXT_PATH(
    VAR ERRIND, 
	TXP : INTEGER) : INTEGER; FORTRAN;

[ASYNCHRONOUS, EXTERNAL(GQTXAL_)] FUNCTION GKS_INQ_TEXT_ALIGN(
    VAR ERRIND, 
	TXALH, 
	TXALV : INTEGER) : INTEGER; FORTRAN;

[ASYNCHRONOUS, EXTERNAL(GQFAIS_)] FUNCTION GKS_INQ_FILL_INT_STYLE(
    VAR ERRIND, 
	INTS : INTEGER) : INTEGER; FORTRAN;

[ASYNCHRONOUS, EXTERNAL(GQFASI_)] FUNCTION GKS_INQ_FILL_STYLE_INDEX(
    VAR ERRIND, 
	STYLI : INTEGER) : INTEGER; FORTRAN;

[ASYNCHRONOUS, EXTERNAL(GQFACI_)] FUNCTION GKS_INQ_FILL_COLOR_INDEX(
    VAR ERRIND, 
	COLI : INTEGER) : INTEGER; FORTRAN;

[ASYNCHRONOUS, EXTERNAL(GQCNTN_)] FUNCTION GKS_INQ_CURRENT_XFORMNO(
    VAR ERRIND, 
	TNR : INTEGER) : INTEGER; FORTRAN;
 
[ASYNCHRONOUS, EXTERNAL(GQNT_)] FUNCTION GKS_INQ_XFORM(
   %REF TNR : INTEGER;
    VAR ERRIND : INTEGER;
   %REF WN, 
	VP : [UNSAFE] ARRAY[L..H : INTEGER] OF REAL) : INTEGER; FORTRAN;
 
[ASYNCHRONOUS, EXTERNAL(GQCLIP_)] FUNCTION GKS_INQ_CLIP(
    VAR ERRIND, 
	CLSW : INTEGER;
   %REF CLRT : [UNSAFE] ARRAY[L..H : INTEGER] OF REAL) : INTEGER; FORTRAN;
 
[ASYNCHRONOUS, EXTERNAL(GQWKC_)] FUNCTION GKS_INQ_WS_CONNTYPE(
   %REF WKID : INTEGER;
    VAR ERRIND, 
	CONID, 
	WTYPE : INTEGER) : INTEGER; FORTRAN;
 
[ASYNCHRONOUS, EXTERNAL(GQTXX_)] FUNCTION GKS_INQ_TEXT_EXTENT(
   %REF WKID : INTEGER;
   %REF PX, 
	PY : REAL;
   %REF CHARS : PACKED ARRAY[L4..H4 : INTEGER] OF CHAR;
    VAR ERRIND : INTEGER;
    VAR CPX, 
	CPY : REAL;
   %REF TRX, 
	TRY : [UNSAFE] ARRAY[L..H : INTEGER] OF REAL) : INTEGER; FORTRAN;
 
[ASYNCHRONOUS, EXTERNAL(GQTXXS_)] FUNCTION GKS_INQ_TEXT_EXTENT_S(
   %REF WKID : INTEGER;
   %REF PX, 
	PY : REAL;
   %REF NCHARS : INTEGER;
   %REF CHARS : PACKED ARRAY[L5..H5 : INTEGER] OF CHAR;
    VAR ERRIND : INTEGER;
    VAR CPX, 
	CPY : REAL;
   %REF TRX, 
	TRY : [UNSAFE] ARRAY[L..H : INTEGER] OF REAL) : INTEGER; FORTRAN;
 
[ASYNCHRONOUS, EXTERNAL(GQDSP_)] FUNCTION GKS_INQ_MAX_DS_SIZE(
   %REF WTYPE : INTEGER;
    VAR ERRIND, 
	DCUNIT : INTEGER;
    VAR RX, 
	RY : REAL;
    VAR LX, 
	LY : INTEGER) : INTEGER; FORTRAN;

[ASYNCHRONOUS, EXTERNAL(GECLKS_)] FUNCTION GKS_EMERGENCY_CLOSE
	: INTEGER; FORTRAN;

[ASYNCHRONOUS, EXTERNAL(GKS_OPEN_CONNECTION_P)] FUNCTION GKS_OPEN_CONNECTION(
	NAME : VARYING[U] OF CHAR) : INTEGER; EXTERNAL;

[ASYNCHRONOUS, EXTERNAL(GKCLOS_)] PROCEDURE GKS_CLOSE_CONNECTION(
   %REF CONID : INTEGER); FORTRAN; 

END. { GKSDEFS }
