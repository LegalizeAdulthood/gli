/*
 *
 * FACILITY:
 *
 *	VAX C Run-Time System
 *
 * ABSTRACT:
 *
 *	This module contains some mathematical routines.
 *
 * AUTHOR(S):
 *
 *	J.Heinen, R.Schmitz
 *	M.Steinert (C Version)
 *
 * VERSION:
 *
 *	V1.0-00
 *
 */


float mth_erfc(float);
float mth_erf(float);
void mth_fft(float *, int, int);
void mth_fft2(float *, int, int, int);
void mth_histogram(int, float *, int *, float *, int);
float mth_gamma(float x);
float mth_t(float, int);
void mth_linreg(int, float *, float *, float *, float *, int);
void mth_linfit(int, float *, float *, float *, float *, float *, int);
float mth_ran(double *);
float mth_rand(double *);
void mth_realft(float *, int, int);
void mth_smooth(int, float *, float *, int);
void mth_spline(int, double *, double *, int, double *, double *);
void mth_b_spline(int, double *, double *, int, double *, double *);
void mth_normal(int, float *, float *, int, float *, float *, int);
