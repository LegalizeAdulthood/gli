/* Motif Global variables */
Display         *display;
XtAppContext    app_context;	       /* application context */
Widget          toplevel_widget;       /* Root widget ID of application */
MrmHierarchy    s_MrmHierarchy;	       /* MRM database hierarchy ID */

/* Literals from the UIL code */
#define k_help 12
#define k_cancel 11
#define k_ok 10
#define k_quit 9
#define k_open 8
#define k_file_cancel 7
#define k_file_ok 6
#define k_next_picture 5
#define k_redraw_picture 4
#define k_previous_picture 3
#define k_format_clear_text 2
#define k_format_binary 1
#define k_draw_cb 0

#define k_main_height 400
#define k_main_width  400
#define k_draw_height 512 
#define k_draw_width  512
