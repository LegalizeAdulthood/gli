/*
 *
 * FACILITY:
 *
 *	VAX C Run-Time System
 *
 * ABSTRACT:
 *
 *	This module contains some definitions for the Graphics
 *	Utility System (GUS)
 *
 * AUTHOR:
 *
 *	Josef Heinen
 *
 * VERSION:
 *
 *	V1.0
 *
 */


/* Status code definitions */

#define gus__facility 0x0000085B

#define gus__normal	0x085B8001  /* normal successful completion */
#define gus__gkcl	0x085B8802  /* GKS not in proper state. GKS must be in
				       one of the states GKOP,WSOP,WSAC,SGOP in
				       routine !AS */
#define gus__notact	0x085B880A  /* GKS not in proper state. GKS must be
				       either in the state WSAC or SGOP in
				       routine !AS */
#define gus__notope	0x085B8812  /* GKS not in proper state. GKS must be in
				       one of the states WSOP,WSAC,SGOP in
				       routine !AS */
#define gus__invintlen	0x085B881A  /* invalid interval length for major
				       tick-marks in routine !AS */
#define gus__invnumtic	0x085B8822  /* invalid number for minor tick intervals
				       in routine !AS */
#define gus__invticsiz	0x085B882A  /* invalid tick-size in routine !AS */
#define gus__orgoutwin	0x085B8832  /* origin outside current window in routine
				       !AS */
#define gus__invdiv	0x085B883A  /* invalid division for grid lines in
				       routine !AS */
#define gus__invnumpnt	0x085B8842  /* invalid number of points in routine
				       !AS */
#define gus__invwinlim	0x085B884A  /* cannot apply logarithmic transformation
				       to current window in routine !AS */
#define gus__invpnt	0x085B8852  /* point co-ordinates must be greater than
				       0 in routine !AS */
#define gus__invzaxis	0x085B885A  /* invalid z-axis specification in routine
				       !AS */
#define gus__invangle	0x085B8862  /* invalid angle in routine !AS */
#define gus__invnumdom	0x085B886A  /* invalid number of domain values in
				       routine !AS */
#define gus__notsorasc	0x085B8872  /* points ordinates not sorted in ascending
				       order in routine !AS */
#define gus__invsmolev	0x085B887A  /* invalid smoothing level in routine !AS */
#define gus__illfmt	0x085B8882  /* file has illegal format in routine !AS */
#define gus__notpowtwo	0x085B888A  /* number of points must be a power of two
				     */
#define gus__synbacksl	0x085B8892  /* Syntax "\" expected */
#define gus__synlparen	0x085B889A  /* Syntax: "(" expected */	
#define gus__synrparen	0x085B88A2  /* Syntax: ")" expected */	
#define gus__synexpr	0x085B88AA  /* Syntax: expression expected */	
#define gus__synlbrace	0x085B88B2  /* Syntax: "{" expected */	
#define gus__synrbrace	0x085B88BA  /* Syntax: "}" expected */	
#define gus__syncomma	0x085B88C2  /* Syntax: "," expected */	
#define gus__synlower	0x085B88CA  /* Syntax: "<" expected */	
#define gus__syngreater	0x085B88D2  /* Syntax: ">" expected */	
#define gus__syninvkey	0x085B88DA  /* Syntax: invalid keyword */	
#define gus__invargmat	0x085B88E2  /* invalid argument to math library */
#define gus__invscale	0x085B88EA  /* invalid scale options in routine !AS */


#define option_x_log (1<<0)
#define option_y_log (1<<1)
#define option_z_log (1<<2)

#define option_flip_x (1<<3)
#define option_flip_y (1<<4)
#define option_flip_z (1<<5)


/* Type definitions */

typedef enum {
    option_lines, option_mesh, option_filled_mesh, option_z_shaded_mesh,
    option_colored_mesh, option_cell_array, option_shaded_mesh
    } gus_surface_option;

typedef enum {
    primitive_polyline, primitive_polymarker
    } gus_primitive;

typedef enum {
    window_full, window_upper_half, window_lower_half, 
    window_upper_left, window_upper_right, window_lower_left, 
    window_lower_right
    } gus_viewport_window;

typedef enum {
    size_a5, size_a4, size_a3
    } gus_viewport_size;

typedef enum {
    orientation_landscape, orientation_portrait
    } gus_viewport_orientation;

typedef enum {
    colormap_uniform, colormap_temperature, colormap_grayscale,
    colormap_glowing, colormap_rainbow, colormap_geologic, 
    colormap_greenscale, colormap_cyanscale, colormap_bluescale, 
    colormap_magentascale, colormap_redscale, colormap_flame,
    colormap_brownscale, colormap_user_defined
    } gus_colormap;

typedef enum {
    colormode_normal, colormode_inverted
    } gus_colormode;


/* Entry point definitions */

#if !defined (cray) && !defined (_WIN32)

#if defined (VMS) || ((defined (hpux) || defined (aix)) && !defined(NAGware))

#define gus_adjust_range		gusar
#define gus_apply_inverse_xform		gusait
#define gus_apply_world_xform		gusawt
#define gus_apply_xform			gusat
#define gus_autoplot			gusap
#define gus_autoscale			gusas
#define gus_autoscale_3d		gusas3
#define gus_axes			gusax
#define gus_axes_3d			gusax3
#define gus_titles_3d			gusti3
#define gus_bar_graph			gusbar
#define gus_contour			guscon
#define gus_curve			guscur
#define gus_fft				gusfft
#define gus_fill_area			gusfa
#define gus_grid			gusgri
#define gus_histogram			gushis
#define gus_horizontal_error_bars	guserr
#define gus_inq_scale			gusqsc
#define gus_inq_smoothing		gusqsm
#define gus_inq_space			gusqsp
#define gus_inq_text_extent_routine	gusqte
#define gus_inverse_fft			gusift
#define gus_linfit			guslf
#define gus_linreg			guslr
#define gus_pie_chart			guspie
#define gus_plot			gusplo
#define gus_plot10_adein		guspi
#define gus_plot10_adeout		guspo
#define gus_plot10_finitt		guspfi
#define gus_plot10_initt		guspin
#define gus_polyline			guspl
#define gus_polymarker			guspm
#define gus_set_colormap		gusscm
#define gus_set_logging			gusslo
#define gus_set_scale			gusssc
#define gus_set_smoothing		gusssm
#define gus_set_space			gusssp
#define gus_set_text_slant		gussts
#define gus_set_ws_viewport		gusswv
#define gus_show_colormap		gussho
#define gus_signal			gussig
#define gus_spline			gusspl
#define gus_surface			gussur
#define gus_text			gustx
#define gus_text_routine		gustxr
#define gus_tick			gustic
#define gus_vertical_error_bars		gusver

#else

#define gus_adjust_range		gusar_
#define gus_apply_inverse_xform		gusait_
#define gus_apply_world_xform		gusawt_
#define gus_apply_xform			gusat_
#define gus_autoplot			gusap_
#define gus_autoscale			gusas_
#define gus_autoscale_3d		gusas3_
#define gus_axes			gusax_
#define gus_axes_3d			gusax3_
#define gus_titles_3d			gusti3_
#define gus_bar_graph			gusbar_
#define gus_contour			guscon_
#define gus_curve			guscur_
#define gus_fft				gusfft_
#define gus_fill_area			gusfa_
#define gus_grid			gusgri_
#define gus_histogram			gushis_
#define gus_horizontal_error_bars	guserr_
#define gus_inq_scale			gusqsc_
#define gus_inq_smoothing		gusqsm_
#define gus_inq_space			gusqsp_
#define gus_inq_text_extent_routine	gusqte_
#define gus_inverse_fft			gusift_
#define gus_linfit			guslf_
#define gus_linreg			guslr_
#define gus_pie_chart			guspie_
#define gus_plot			gusplo_
#define gus_plot10_adein		guspi_
#define gus_plot10_adeout		guspo_
#define gus_plot10_finitt		guspfi_
#define gus_plot10_initt		guspin_
#define gus_polyline			guspl_
#define gus_polymarker			guspm_
#define gus_set_colormap		gusscm_
#define gus_set_logging			gusslo_
#define gus_set_scale			gusssc_
#define gus_set_smoothing		gusssm_
#define gus_set_space			gusssp_
#define gus_set_text_slant		gussts_
#define gus_set_ws_viewport		gusswv_
#define gus_show_colormap		gussho_
#define gus_signal			gussig_
#define gus_spline			gusspl_
#define gus_surface			gussur_
#define gus_text			gustx_
#define gus_text_routine		gustxr_
#define gus_tick			gustic_
#define gus_vertical_error_bars		gusver_

#endif /* VMS, hpux, aix */

#else

#define gus_adjust_range		GUSAR
#define gus_apply_inverse_xform		GUSAIT
#define gus_apply_world_xform		GUSAWT
#define gus_apply_xform			GUSAT
#define gus_autoplot			GUSAP
#define gus_autoscale			GUSAS
#define gus_autoscale_3d		GUSAS3
#define gus_axes			GUSAX
#define gus_axes_3d			GUSAX3
#define gus_titles_3d			GUSTI3
#define gus_bar_graph			GUSBAR
#define gus_contour			GUSCON
#define gus_curve			GUSCUR
#define gus_fft				GUSFFT
#define gus_fill_area			GUSFA
#define gus_grid			GUSGRI
#define gus_histogram			GUSHIS
#define gus_horizontal_error_bars	GUSERR
#define gus_inq_scale			GUSQSC
#define gus_inq_smoothing		GUSQSM
#define gus_inq_space			GUSQSP
#define gus_inq_text_extent_routine	GUSQTE
#define gus_inverse_fft			GUSIFT
#define gus_linfit			GUSLF
#define gus_linreg			GUSLR
#define gus_pie_chart			GUSPIE
#define gus_plot			GUSPLO
#define gus_plot10_adein		GUSPI
#define gus_plot10_adeout		GUSPO
#define gus_plot10_finitt		GUSPFI
#define gus_plot10_initt		GUSPIN
#define gus_polyline			GUSPL
#define gus_polymarker			GUSPM
#define gus_set_colormap		GUSSCM
#define gus_set_logging			GUSSLO
#define gus_set_scale			GUSSSC
#define gus_set_smoothing		GUSSSM
#define gus_set_space			GUSSSP
#define gus_set_text_slant		GUSSTS
#define gus_set_ws_viewport		GUSSWV
#define gus_show_colormap		GUSSHO
#define gus_signal			GUSSIG
#define gus_spline			GUSSPL
#define gus_surface			GUSSUR
#define gus_text			GUSTX
#define gus_text_routine		GUSTXR
#define gus_tick			GUSTIC
#define gus_vertical_error_bars		GUSVER

#endif /* cray, _WIN32 */

void gks_inq_text_extent_s (int *, float *, float *, int *, char *, int *,
    float *, float *, float *, float *);
void gks_text_s (float *px, float *py, int *nchars, char *chars);
void gus_adjust_range (float *min, float *max);
int gus_apply_inverse_xform (float *x, float *y);
void gus_apply_world_xform (float *x, float *y, float *z);
int gus_apply_xform (int *tnr, float *x, float *y);
int gus_autoscale (int *n, float *x, float *y, float *x_min, float *x_max,
    float *y_min, float *y_max, int *status);
int gus_autoscale_3d (int *n, float *x, float *y, float *z, float *x_min,
    float *x_max, float *y_min, float *y_max, float *z_min, float *z_max,
    int *status);
int gus_axes (float *x_tick, float *y_tick, float *x_org, float *y_org,
    int *major_x, int *major_y, float *t_size, int *status);
int gus_axes_3d (float *x_tick, float *y_tick, float *z_tick, float *x_org,
    float *y_org, float *z_org, int *major_x, int *major_y, int *major_z,
    float *t_size, int *status);
int gus_titles_3d (char *x_title, char *y_title, char *z_title, int *status);
int gus_bar_graph (int *n, float *px, float *py, float *width, int *status);
int gus_contour (int *nx, int *ny, int *nh, float *px, float *py, float *h,
    float (*fz)(int *ix, int *iy), int *major_h, int *status);
int gus_curve (int *n, float *px, float *py, float *pz,
    gus_primitive *primitive, int *status);
int gus_fft (int *n, float *py, int *m, int *status);
int gus_fill_area (int *n, float *px, float *py, int *status);
int gus_grid (float *x_tick, float *y_tick, float *x_org, float *y_org,
    int *major_x, int *major_y, int *status);
int gus_histogram (int *n, float *px, int *status);
int gus_horizontal_error_bars (int *n, float *px, float *py, float *e1,
    float *e2, int *status);
int gus_inq_scale (int *options, int *status);
int gus_inq_smoothing (int *level, int *status);
int gus_inq_space (float *z_min, float *z_max, int *rotation, int *tilt,
    int *status);
int gus_inverse_fft (int *n, float *fr, float *fi, int *status);
int gus_linfit (int *n, float *px, float *py, int *status);
int gus_linreg (int *n, float *px, float *py, int *status);
int gus_pie_chart (int *n, float *data, int *status);
int gus_plot (int *n, float *px, float *py, int *status);
int gus_polyline (int *n, float *px, float *py, int *status);
int gus_polymarker (int *n, float *px, float *py, int *status);
int gus_set_colormap (gus_colormap *colormap, gus_colormode *colormode,
    int *status);
int gus_set_logging (unsigned *lswitch, int *status);
int gus_set_scale (int *options, int *status);
int gus_set_smoothing (int *level, int *status);
int gus_set_space (float *z_min, float *z_max, int *rotation, int *tilt,
    int *status);
int gus_set_ws_viewport (gus_viewport_window *window, gus_viewport_size *size,
    gus_viewport_orientation *orientation, int *status);
int gus_show_colormap (int *status);
void gus_signal (int cond_value, char *procid);
int gus_spline (int *n, float *px, float *py, int *m, float *smoothing,
    int *status);
int gus_surface (int *nx, int *ny, float *px, float *py,
    float (*fz)(int *ix, int *iy), gus_surface_option *option, int *status);
int gus_text (float *px, float *py, char *chars, int *status);
float gus_tick (float *min, float *max);
int gus_vertical_error_bars (int *n, float *px, float *py, float *e1,
    float *e2, int *status);

void gus_autoplot (int *status);

void gus_plot10_adein (int *nchar, char *kade);
void gus_plot10_adeout (int *nchar, char *string, int *clear);
void gus_plot10_finitt ();
void gus_plot10_initt ();

void gus_inq_text_extent_routine (float *px, float *py, char *chars, float *tbx,
    float *tby, int *status);
void gus_set_text_slant (float *slant);
void gus_text_routine (float *px, float *py, char *chars, int *status);

#ifndef __gus

extern float gus_gf_linreg_m;
extern float gus_gf_linreg_b;
extern float gus_gf_linfit_m;
extern float gus_gf_linfit_b;
extern float gus_gf_linfit_dev;
extern float gus_gf_missing_value;

#endif /* __gus */
