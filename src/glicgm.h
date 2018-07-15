/* the metafile descriptor elements */
/* sub-definitions */
enum    vdc_enum        {vdc_int,       vdc_real};
struct  r_struct        {int fixed;     int exp;        int fract;};
struct  c_v_struct      {int min[3];    int max[3];};
/* the structure */
struct mf_d_struct {
        enum vdc_enum           vdc_type;       /* vdc type             */
        int                     int_prec;       /* integer precision    */
        struct r_struct         real_prec;      /* real precision       */
        int                     ind_prec;       /* index precision      */
        int                     col_prec;       /* colour precision     */
        int                     col_i_prec;     /* colour index prec.   */
        int                     max_c_index;    /* max. colour index    */
        struct c_v_struct       c_v_extent;     /* colour value extent  */
        int                     char_c_an;      /* char code announcer  */
};

/* the picture descriptor elements (class 2) */
/* sub-defs */
struct  scale_struct    {int s_mode;    float m_scaling;};
enum    cs_enum         {i_c_mode,      d_c_mode};
enum    spec_enum       {absolute,      scaled}; 
struct  vdc_struct      {float r[4];    int i[4];};
struct  rgb_struct      {float red;     float green;    float blue;};
/* the actual structure */
struct pic_d_struct {
        struct scale_struct     scale_mode;     /* scaling mode         */
        enum cs_enum            c_s_mode;       /* colour selection mode*/
        enum spec_enum          l_w_s_mode;     /* line width spec mode */
        enum spec_enum          m_s_s_mode;     /* marker size spec mode*/
        enum spec_enum          e_w_s_mode;     /* edge width spec mode */
        struct vdc_struct       vdc_extent;     /* the vdc extent       */
        struct rgb_struct       back_col;       /* background colour    */
};

/* the control elements (class 3) */
/* sub-defs */
struct  rgbi_struct     {float red;     float green;    float blue;
                        int ind;};
enum    bool_enum       {off,           on};
/* the actual structure */
struct control_struct {
        int                     vdc_i_prec;     /* vdc integer precision*/
        struct r_struct         vdc_r_prec;     /* vdc real precision   */
        struct rgbi_struct      aux_col;        /* auxiliary colour     */
        enum bool_enum          transparency;   /* background trans.    */
        struct vdc_struct       clip_rect;      /* clipping rectangle   */
        enum bool_enum          clip_ind;       /* clipping indicator   */
};

/* now the big one; the attribute elements (class 5) */
/* sub-defs */
struct  i_r_struct      {int i; float r;};
enum    txt_enum        {string, character, stroke};
struct  orient_struct   {int x_up; int y_up; int x_base; int y_base;};
enum    path_enum       {right, left, up, down};
enum hor_align { normal_h, left_h, center_h, right_h, cont_h };
enum ver_align { normal_v, top_v, cap_v, half_v, base_v, bottom_v, cont_v };
struct  align_struct    {enum hor_align hor; enum ver_align ver; 
                        float cont_hor; float cont_ver;};
enum    is_enum         {hollow, solid_i, pattern, hatch, empty};
struct  pt_struct       {int i[2]; float r[2];};
/* the actual structure */
struct attrib_struct {
        int                     l_b_index;
        int			line_type;
        struct i_r_struct       line_width;
        struct rgbi_struct      line_colour;
        int                     mk_b_index;     /* marker bundle index  */
        int                     mk_type;        /* marker type          */
        struct i_r_struct       mk_size;        /* marker size          */
        struct rgbi_struct      mk_colour;      /* marker colour        */
        int                     t_b_index;      /* text bundle index    */
        int                     t_f_index;      /* text font index      */
        enum txt_enum           t_prec;         /* text precision       */
        float                   c_exp_fac;      /* character exp. factor*/
        float                   c_space;        /* additional spacing   */
        struct rgbi_struct      text_colour;    
        int                     c_height;       /* character height     */
        struct  orient_struct   c_orient;       /* character orientation*/
        enum path_enum          text_path;
        struct align_struct     text_align;     /* text alignment       */
        int                     c_set_index;    /* character set index  */
        int                     a_c_set_index;  /* alt char set index   */
        int                     f_b_index;      /* fill bundle index    */
        enum is_enum            int_style;      /* interior style       */
        struct rgbi_struct      fill_colour;    /* for polygons         */
        int                     hatch_index;    
        int                     pat_index;      /* pattern index        */
        int                     e_b_index;      /* edge bundle index    */
        int			edge_type;      /* for polygons         */
        struct i_r_struct       edge_width;     /* edge width           */
        struct rgbi_struct      edge_colour;    /* edge colour          */
        enum bool_enum          edge_vis;       /* edge visibility      */
        struct pt_struct        fill_ref;       /* fill reference pt    */
        struct vdc_struct       pat_size;       /* pattern size         */
        float                   *ctab;          /* colour tables        */
};
/* do the pattern, bundle tables after we've thought about them */


/* now the command line options, use an array of structures */
#define flags_l 20
#define max_str 128
enum optval_enum {onoff, real, integer, str, lst};
struct one_opt {
        char            flag_str[flags_l];      /* flag string cmd line */
        char            flag_char;              /* flag char cmd line   */
        int             set;                    /* was it explicitly set*/
        enum optval_enum        tag;            /* tag for type         */
        union {
                int     i;
                float   r;
                char    str[max_str];
        } val;                                  /* actual value         */
};
/* we will use an array of these structure to hold the options */
/*
enum opt_enum {copies, debug, degrees, device, diaquest, frames, 
index_file, included, list, title_string, nindex,
pages, pxl_in, ypxl_in, screen, start, tty, x_offset, x_size, y_offset, 
y_size, text_mag, font_type, scale_output, clear_text, in_name, out_name};
*/

enum opt_enum { pages, screen, clear_text, in_name};
#define opt_size ((int)in_name + 1)



/* now our enumerated classes for the CGM elements */
enum cgmcls0 /* class 0, the delimiter elements */
{No_Op = 0, B_Mf, E_Mf, B_Pic, B_Pic_Body, E_Pic};
#define Delim_Size ((int) E_Pic + 1)

enum cgmcls1 /* class 1, the metafile descriptor elements */
{MfVersion = 1, MfDescrip, vdcType, IntPrec, RealPrec, IndexPrec, ColPrec, 
CIndPrec, MaxCInd, CVExtent, MfElList, MfDefRep, FontList, CharList, 
CharAnnounce};
#define MfDesc_Size ((int) CharAnnounce + 1)

enum cgmcls2 /* class 2, the page descriptor elements */
{ScalMode = 1, ColSelMode, LWidSpecMode, MarkSizSpecMode, EdWidSpecMode, 
vdcExtent, BackCol};
#define PDesc_Size ((int) BackCol + 1)

enum cgmcls3 /* class 3, the control elements */
{vdcIntPrec = 1, vdcRPrec, AuxCol, Transp, ClipRect, ClipIndic};
#define Control_Size ((int) ClipIndic + 1)

enum cgmcls4 /* class 4, the graphical primitives */
{PolyLine = 1, Dis_Poly, PolyMarker, TeXt, Rex_Text, App_Text, Polygon, 
Poly_Set, Cell_Array, Gen_D_Prim, RectAngle, Cgm_Circle, Circ_3, Circ_3_Close,
Circ_Centre, Circ_C_Close, Ellipse, Ellip_Arc, El_Arc_Close};
#define GPrim_Size ((int) El_Arc_Close + 1)

enum cgmcls5    /* class 5 the attribute elements */
{ LBIndex = 1,  LType,  LWidth,  LColour,  MBIndex,  MType,  MSize,  MColour,
TBIndex, TFIndex, TPrec, CExpFac,  CSpace, TColour, CHeight,  COrient,  
TPath, TAlign, CSetIndex, AltCSetIndex, FillBIndex, IntStyle, FillColour,
HatchIndex,  PatIndex, EdBIndex, EType, EdWidth, EdColour, EdVis, FillRef,
PatTab,  PatSize, ColTab,  AspsFlags};
#define Att_Size ((int) AspsFlags + 1)

enum cgmcls6    /* the escape element */
{Escape = 1};
#define Esc_Size ((int) Escape + 1)

enum cgmcls7    /* the external elements */
{Message = 1, Ap_Data};
#define Ext_Size ((int) Ap_Data + 1)

/* we will use these both in the main CGM program and to create arrays 
   of device-specific functions for the main program to call */


/* now define the device info structure and relevant flags */
/* declare the structure which holds the info on PDL cpabilities */
#define port_land       1       /* portrait/landscape capability */
#define arb_rot         (1<<1)  /* arbitrary rotations available */
#define arb_trans       (1<<2)  /* can translate the origin */
#define no_cr           (1<<3)  /* needs no carriage return at end of line */
#define need_fix        (1<<4)  /* needs a fixed_length output record */
#define v_center        (1<<5)  /* can vertically position text */
#define h_center        (1<<6)  /* can horizontally position text */
#define pad_rows        (1<<7)  /* need to pad rows in font desriptions */
#define brk_ok          (1<<8)  /* O.K. to break the records (e.g., cr) */
#define stroke_text     (1<<9)  /* Driver can handle stroke prec. text */
#define char_text       (1<<10) /* Driver can handle character prec. text */
#define string_text     (1<<11) /* Driver can handle string prec. text */
#define no_def_calls    (1<<12) /* no calls to driver during mf def. rep. */
#define no_colour       (1<<13) /* cannot handle colour well */
#define rot1_font       (1<<14) /* must rotate fonts in default mode */
#define rot2_font       (1<<15) /* must rotate fonts in rotated mode */
#define can_clip        (1<<16) /* device can handle clipping */

/* the device info structure */
struct  info_struct {
        float pxl_in;           /* resolution in pixels per inch */
        float ypxl_in;          /* y resolution in pixels per inch */
        float x_size;           /* page size (x, y) */
        float y_size;           
        float x_offset;         /* page origin offset (x, y) */
        float y_offset;         
        int c_height;           /* character height in pixels */
        int c_width;            /* character width in pixels */
        int d_l_width;          /* default line width in pixels */
        int d_e_width;          /* default edge width in pixels */
        int d_m_size;           /* default marker size in pixels */
        char out_name[max_str]; /* default for output file name */
        long int capability;            /* what can it do ?     */
        int rec_size;           /* size of output record/line size */
};

/* and a font info structure */
#define max_chars 128   /* max no of chars in a font */
#define font_name_length 50
#define flag_no (max_chars / sizeof(int))

struct font{
int num;        /* font_number */
int chk_sum;    /* check sum */
int scale;      /* scale factor */
int design;     /* design size */
int a;          /* area length */
int l;          /* name length */
int ino;        /* internal number */
int used;       /* no. of characters used */
int load_flag[flag_no]; /* character use flag */
int want_flag[flag_no]; /* wanted flag */
char name[font_name_length];    /* font name */
char local[font_name_length];   /* local font file name */
int width[max_chars];           /* character widths in sp*/
int pxl[max_chars];             /* character widths in pixels*/
char found;     /* did we actually find this font ? */
char read;      /* and did we try to read it ? */
int local_mag;  /* the mag we will use */
int want_mag;   /* the mag it asked for */
int voff[max_chars];    /* the pixel offset required v direction */
int hoff[max_chars];    /* the pixel offset required h direction */
int w_bytes[max_chars]; /* width in bytes of the raster map */
int h_bits[max_chars];  /* height in bits of the raster map */
int w_bits[max_chars];  /* width in bits of the raster map */
char loaded_once;
int no_bytes[max_chars];                /* stored size */
unsigned char *ptr[max_chars];  /* pointer to stored vectors */
unsigned char *exp[max_chars];  /* pointer to stored expansion */
int exp_size[max_chars];        /* size of info in .exp */
int exp_prec[max_chars];        /* precision of expansion */
int exp_index[max_chars];       /* the colour index stored */
};
/* and a structure for caching the font information */
#define key_str_l 80    /* for character I.D. */
#define cmsize 30000
struct record_type {
        char    c_id[key_str_l];
        int     date, tfm, pxl, bytes;
        char    info[cmsize];
        };
/* now common address structures for indexing */
#define INDEX_FLAG -64
/* basic address structure */
struct ad_struct {
        long int        b_ad;           /* byte address */
        int             r_ad;           /* record address */
        int             offset;         /* offset in the record */
        };
/* the page info structure */
struct p_struct {
        struct p_struct *next;
        struct ad_struct ad;            /* address structure */
        char            *str;           /* string associated */
        int             len;            /* string length */
        int             no;             /* page number in the file */
        };

/* the macro mapping from real r, g, b to index, aim for speed */
#define int_rgb(r, g, b, inds) ((int) ((r) * (inds[0] - 1) + 0.49) + \
((int) ((g) * (inds[1] - 1) + 0.49)) * inds[0] + \
((int) ((b) * (inds[2] - 1) + 0.49)) * inds[0] * inds[1]);

/* macro to get from integer cgm input to a real direct colour value */
/* i is the rgb index (0, 1 or 2), and ival the integer value */
#define dcind(i, ival, ext) ((float)(ival)-ext.min[i])/(ext.max[i]-ext.min[i])

/* macro to get single index from i, j, k 3-tuple */
#define ind_ijk(inds, i, j, k) (inds[0] * inds[1] * i + inds[0] * j + k)
/* the clear text equivalent names of the metafile elements */
/* first as we will write them */
/* the delimiters */
static char *cc_delim[Delim_Size] = {"",
"BegMF", "EndMF", "BegPic", "BegPicBody", "EndPic"
};
/* now the metafile descriptors */
static char *cc_mfdesc[MfDesc_Size] = {"",
"MFVersion", "MFDesc", "VDCType", "IntegerPrec", "RealPrec", "IndexPrec",
"ColrPrec", "ColrIndexPrec", "MaxColrIndex", "ColrValueExt", "MFElemList",
"MFDefaults", "FontList", "CharSetList", "CharCoding"
};
/* the page descriptors */
static char *cc_pdesc[PDesc_Size] = {"",
"ScaleMode", "ColrMode", "LineWidthMode", "MarkerSizeMode", "EdgeWidthMode",
"VDCExt", "BackColr"
};
/* the control elements */
static char *cc_control[Control_Size] = {"",
"VDCIntegerPrec", "VDCRealPrec", "AuxColr", 
"Transparency", "ClipRect", "Clip"
};
/* the graphical primitives */
static char *cc_gprim[GPrim_Size] = {"",
"Line", "DisjtLine", "Marker", "Text", "RestrText", "ApndText", "Polygon",
"PolygonSet", "CellArray", "GDP", "Rect", "Circle", "Arc3Pt", "Arc3PtClose",
"ArcCtr", "ArcCtrClose", "Ellipse", "EllipArc", "EllipArcClose"
};
/* the attribute elements */
static char *cc_attr[Att_Size] = {"",
"LineIndex", "LineType", "LineWidth", "LineColr", "MarkerIndex", "MarkerType",
"MarkerSize", "MarkerColr", "TextIndex", "TextFontIndex", "TextPrec", 
"CharExpan", "CharSpace", "TextColr", "CharHeight", "CharOri", "TextPath",
"TextAlign", "CharSetIndex", "AltCharSetIndex", "FillIndex", "IntStyle", 
"FillColr", "HatchIndex", "PatIndex", "EdgeIndex", "EdgeType", "EdgeWidth",
"EdgeColr", "EdgeVis", "FillRefPt", "PatTable", "PatSize", "ColrTable", 
"ASF"
};
/* the escape element */
static char *cc_escape[Esc_Size] = {"",
"Escape"
};
/* the external elements */
static char *cc_external[Ext_Size] = {"",
"Message", "ApplData"
};
/* now the array of pointers that ties them all together */
static char **cc_cptr[8] = {cc_delim, cc_mfdesc, cc_pdesc, cc_control, cc_gprim, 
cc_attr, cc_escape, cc_external};
/* and the associated dimensions (by hand, for convenience) */
static int cc_size[8] = {Delim_Size, MfDesc_Size, PDesc_Size, Control_Size,
GPrim_Size, Att_Size, Esc_Size, Ext_Size};

/* Now as we store them internally for speed */
/* the delimiters */
static char *CC_delim[Delim_Size] = {"",
"BEGMF", "ENDMF", "BEGPIC", "BEGPICBODY", "ENDPIC"
};
/* now the metafile descriptors */
static char *CC_mfdesc[MfDesc_Size] = {"",
"MFVERSION", "MFDESC", "VDCTYPE", "INTEGERPREC", "REALPREC", "INDEXPREC",
"COLRPREC", "COLRINDEXPREC", "MAXCOLRINDEX", "COLRVALUEEXT", "MFELEMLIST",
"MFDEFAULTS", "FONTLIST", "CHARSETLIST", "CHARCODING"
};
/* the page descriptors */
static char *CC_pdesc[PDesc_Size] = {"",
"SCALEMODE", "COLRMODE", "LINEWIDTHMODE", "MARKERSIZEMODE", "EDGEWIDTHMODE",
"VDCEXT", "BACKCOLR"
};
/* the control elements */
static char *CC_control[Control_Size] = {"",
"VDCINTEGERPREC", "VDCREALPREC", "AUXCOLR", 
"TRANSPARENCY", "CLIPRECT", "CLIP"
};
/* the graphical primitives */
static char *CC_gprim[GPrim_Size] = {"",
"LINE", "DISJTLINE", "MARKER", "TEXT", "RESTRTEXT", "APNDTEXT", "POLYGON",
"POLYGONSET", "CELLARRAY", "GDP", "RECT", "CIRCLE", "ARC3PT", "ARC3PTCLOSE",
"ARCCTR", "ARCCTRCLOSE", "ELLIPSE", "ELLIPARC", "ELLIPARCCLOSE"
};
/* the attribute elements */
static char *CC_attr[Att_Size] = {"",
"LINEINDEX", "LINETYPE", "LINEWIDTH", "LINECOLR", "MARKERINDEX", "MARKERTYPE",
"MARKERSIZE", "MARKERCOLR", "TEXTINDEX", "TEXTFONTINDEX", "TEXTPREC", 
"CHAREXPAN", "CHARSPACE", "TEXTCOLR", "CHARHEIGHT", "CHARORI", "TEXTPATH",
"TEXTALIGN", "CHARSETINDEX", "ALTCHARSETINDEX", "FILLINDEX", "INTSTYLE", 
"FILLCOLR", "HATCHINDEX", "PATINDEX", "EDGEINDEX", "EDGETYPE", "EDGEWIDTH",
"EDGECOLR", "EDGEVIS", "FILLREFPT", "PATTABLE", "PATSIZE", "COLRTABLE", 
"ASF"
};
/* the escape element */
static char *CC_escape[Esc_Size] = {"",
"ESCAPE"
};
/* the external elements */
static char *CC_external[Ext_Size] = {"",
"MESSAGE", "APPLDATA"
};
/* now the array of pointers that ties them all together */
static char **CC_cptr[8] = {CC_delim, CC_mfdesc, CC_pdesc, CC_control, 
CC_gprim, CC_attr, CC_escape, CC_external};


/* now the Aspect source flags */
#define NO_ASPS_FLAGS 18
static char *asps_flags[NO_ASPS_FLAGS] = {
"LINETYPE", "LINEWIDTH", "LINECOLR",
"MARKERTYPE", "MARKERSIZE", "MARKERCOLR",
"TEXTFONTINDEX", "TEXTPREC", "CHAREXP",
"CHARSPACE", "TEXTCOLR", "INTSTYLE",
"FILLCOLR", "HATCHINDEX", "PATINDEX",
"EDGETYPE", "EDGEWIDTH", "EDGECOLR"};


/* now some of the special characters and macros */
#define term_char ';'		/* standard termination character */
#define term1_char '/'		/* secondary termination character */
#define is_term(c) (((c) == term_char) || ((c) == term1_char))

#define quote_char '\"'		/* standard quote character */
#define quote1_char '\''	/* secondary quote character */
#define is_quote(c) (((c) == quote_char) || ((c) == quote1_char))

#define sep_char ' '		/* standard separation character */
#define sep1_char '\011'	/* secondary separation character */
#define sep2_char '\012'	/* secondary separation character */
#define sep3_char '\013'	/* secondary separation character */
#define sep4_char '\014'	/* secondary separation character */
#define sep5_char '\015'	/* secondary separation character */

#define is_sep(c) (((c) == sep_char) || (((c) <= sep5_char) \
	&& ((c) >= sep1_char)))

#define null_char '_'		/* primary null character */
#define null1_char '$'		/* secondary null character */

#define is_null(c) (((c) == null_char) || ((c) == null1_char))

#define comment_char '%'	/* comment start */

#define is_comment(c) ((c) == comment_char)

