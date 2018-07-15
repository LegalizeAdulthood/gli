[ENVIRONMENT ('gusdefs.pen')]

MODULE	gusdefs;

{ Type definitions }

TYPE
    scale_option = (
	option_x_log, option_y_log, option_z_log,
	option_flip_x, option_flip_y, option_flip_z);

    scale_options = [LONG] set of scale_option;

    surface_option = (
	option_lines, option_mesh, option_filled_mesh, option_z_shaded_mesh,
	option_colored_mesh, option_cell_array, option_shaded_mesh);

    viewport_size = (size_a5, size_a4, size_a3);

    viewport_window = (
	window_full, window_upper_half, window_lower_half,
	window_upper_left, window_upper_right, window_lower_left,
	window_lower_right);

    viewport_orientation = (orientation_landscape, orientation_portrait);

{ Entry point definitions }

[ASYNCHRONOUS, EXTERNAL(guspl_)] FUNCTION GUS_POLYLINE(
   %REF	n : INTEGER;
   %REF	px,
	py : [UNSAFE] ARRAY[L..H: INTEGER] OF REAL;
    VAR status : INTEGER := %IMMED 0) : INTEGER; EXTERNAL;

[ASYNCHRONOUS, EXTERNAL(guspm_)] FUNCTION GUS_POLYMARKER(
   %REF	n : INTEGER;
   %REF	px,
	py : [UNSAFE] ARRAY[l..h : INTEGER] OF REAL;
    VAR status : INTEGER := %IMMED 0) : INTEGER; EXTERNAL;

[ASYNCHRONOUS, EXTERNAL(gus_text_p)] FUNCTION GUS_TEXT(
	px,
	py : REAL;
	chars : VARYING[u] OF CHAR;
    VAR status : INTEGER := %IMMED 0) : INTEGER; EXTERNAL;

[ASYNCHRONOUS, EXTERNAL(gusspl_)] FUNCTION GUS_SPLINE(
   %REF n : INTEGER;
   %REF	px,
	py : [UNSAFE] ARRAY[l..h : INTEGER] OF REAL;
   %REF m : INTEGER;
   %REF smoothing : REAL;
    VAR status : INTEGER := %IMMED 0) : INTEGER; EXTERNAL;

[ASYNCHRONOUS, EXTERNAL(guslf_)] FUNCTION GUS_LINFIT(
   %REF n : INTEGER;
   %REF	px,
	py : [UNSAFE] ARRAY[l..h : INTEGER] OF REAL;
    VAR status : INTEGER := %IMMED 0) : INTEGER; EXTERNAL;

[ASYNCHRONOUS, EXTERNAL(guslr_)] FUNCTION GUS_LINREG(
   %REF n : INTEGER;
   %REF	px,
	py : [UNSAFE] ARRAY[l..h : INTEGER] OF REAL;
    VAR status : INTEGER := %IMMED 0) : INTEGER; EXTERNAL;

[ASYNCHRONOUS, EXTERNAL(gushis_)] FUNCTION GUS_HISTOGRAM(
   %REF n : INTEGER;
   %REF	px : [UNSAFE] ARRAY[l..h : INTEGER] OF REAL;
    VAR status : INTEGER := %IMMED 0) : INTEGER; EXTERNAL;

[ASYNCHRONOUS, EXTERNAL(gusfft_)] FUNCTION GUS_FFT(
   %REF n : INTEGER;
   %REF	py : [UNSAFE] ARRAY[l..h : INTEGER] OF REAL;
   %REF m : INTEGER;
    VAR status : INTEGER := %IMMED 0) : INTEGER; EXTERNAL;

[ASYNCHRONOUS, EXTERNAL(gusift_)] FUNCTION GUS_INVERSE_FFT(
   %REF n : INTEGER;
   %REF	pr,
	pi : [UNSAFE] ARRAY[l..h : INTEGER] OF REAL;
    VAR status : INTEGER := %IMMED 0) : INTEGER; EXTERNAL;

[ASYNCHRONOUS, EXTERNAL(gusplo_)] FUNCTION GUS_PLOT(
   %REF n : INTEGER;
   %REF	px,
	py : [UNSAFE] ARRAY[l..h : INTEGER] OF REAL;
    VAR status : INTEGER := %IMMED 0) : INTEGER; EXTERNAL;

[ASYNCHRONOUS, EXTERNAL(gusax_)] FUNCTION GUS_AXES(
   %REF	xtic,
	ytic,
	xorg,
	yorg : REAL;
   %REF	majx,
	majy : INTEGER;
   %REF	ticsiz : REAL;
    VAR status : INTEGER := %IMMED 0) : INTEGER; EXTERNAL;

[ASYNCHRONOUS, EXTERNAL(gusgri_)] FUNCTION GUS_GRID(
   %REF	xtic,
	ytic,
	xorg,
	yorg : REAL;
   %REF	majx,
	majy : INTEGER;
    VAR status : INTEGER := %IMMED 0) : INTEGER; EXTERNAL;

[ASYNCHRONOUS, EXTERNAL(guscur_)] FUNCTION GUS_CURVE(
   %REF	x : INTEGER;
   %REF	px,
	py,
	pz : [UNSAFE] ARRAY[l..h : INTEGER] OF REAL;
    VAR status : INTEGER := %IMMED 0) : INTEGER; EXTERNAL;

[ASYNCHRONOUS, EXTERNAL(gussur_)] FUNCTION GUS_SURFACE(
   %REF nx,
	ny : INTEGER;
   %REF	px,
	py : ARRAY[l..h : INTEGER] OF REAL;
 %IMMED [UNBOUND] FUNCTION f(VAR ix, iy : INTEGER) : REAL;
   %REF	option : surface_option;
    VAR status : INTEGER := %IMMED 0) : INTEGER; EXTERNAL;

[ASYNCHRONOUS, EXTERNAL(guscon_)] FUNCTION GUS_CONTOUR(
   %REF nx,
	ny,
	nh : INTEGER;
   %REF	px,
	py,
	ph : ARRAY[l..h : INTEGER] OF REAL;
 %IMMED [UNBOUND] FUNCTION f(VAR ix, iy : INTEGER) : REAL;
   %REF majh : INTEGER := 0;
    VAR status : INTEGER := %IMMED 0) : INTEGER; EXTERNAL;

[ASYNCHRONOUS, EXTERNAL(gusax3_)] FUNCTION GUS_AXES_3D(
   %REF	xtic,
	ytic,
	ztic,
	xorg,
	yorg,
	zorg : REAL;
   %REF	majx,
	majy,
	majz : INTEGER;
   %REF	ticsiz : REAL;
    VAR status : INTEGER := %IMMED 0) : INTEGER; EXTERNAL;

[ASYNCHRONOUS, EXTERNAL(gustic_)] FUNCTION GUS_TICK(
   %REF min,
	max : REAL) : REAL; EXTERNAL;

[ASYNCHRONOUS, EXTERNAL(gusas_)] FUNCTION GUS_AUTOSCALE(
   %REF n : INTEGER;
   %REF	px,
	py : [UNSAFE] ARRAY[l..h : INTEGER] OF REAL;
    VAR x_min,
	x_max,
	y_min,
	y_max : REAL;
    VAR status : INTEGER := %IMMED 0) : INTEGER; EXTERNAL;

[ASYNCHRONOUS, EXTERNAL(gusap_)] FUNCTION GUS_AUTOPLOT(
    VAR status : INTEGER := %IMMED 0) : INTEGER; EXTERNAL;

[ASYNCHRONOUS, EXTERNAL(gusslo_)] FUNCTION GUS_SET_LOGGING(
   %REF switch : BOOLEAN := %REF TRUE;
    VAR status : INTEGER := %IMMED 0) : INTEGER; EXTERNAL;

[ASYNCHRONOUS, EXTERNAL(gusssc_)] FUNCTION GUS_SET_SCALE(
   %REF	options : scale_options;
    VAR status : INTEGER := %IMMED 0) : INTEGER; EXTERNAL;

[ASYNCHRONOUS, EXTERNAL(gusswv_)] FUNCTION GUS_SET_WS_VIEWPORT(
   %REF	window : viewport_window;
   %REF	size : viewport_size;
   %REF orientation : viewport_orientation;
    VAR status : INTEGER := %IMMED 0) : INTEGER; EXTERNAL;

[ASYNCHRONOUS, EXTERNAL(gusssm_)] FUNCTION GUS_SET_SMOOTHING(
   %REF	level : INTEGER;
    VAR status : INTEGER := %IMMED 0) : INTEGER; EXTERNAL;

[ASYNCHRONOUS, EXTERNAL(gusssp_)] FUNCTION GUS_SET_SPACE(
   %REF	zmin,
	zmax : REAL;
   %REF	rotation,
	tilt : INTEGER;
    VAR status : INTEGER := %IMMED 0) : INTEGER; EXTERNAL;

[ASYNCHRONOUS, EXTERNAL(gussts_)] FUNCTION GUS_SET_TEXT_SLANT(
   %REF	SLANT : REAL) : INTEGER; EXTERNAL;

[ASYNCHRONOUS, EXTERNAL(gusqsp_)] FUNCTION GUS_INQ_SPACE(
    VAR zmin,
	zmax : REAL;
    VAR rotation,
	tilt : INTEGER;
    VAR status : INTEGER := %IMMED 0) : INTEGER; EXTERNAL;

END. { gusdefs }
