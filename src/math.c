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


#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef DXML
#include <dlfcn.h>
#endif

#include "mathlib.h"
#include "terminal.h"

#define TRUE 1
#define FALSE 0
#define BOOL int

#define pi 3.1415926535
#define eps 0.0001



static float sign (float a1, float a2)

/*
 * sign - transfer of sign  |a1| sign a2
 *
 */

{
    if (a2 == 0)
	return (0);
    else
	if (a2 > 0)
	    return (fabs(a1));
	else
	    return (-fabs(a1));
}



static double dmod (double a1, double a2)

/*
 * dmod - return a1 mod a2
 *
 */

{
    return fabs(a1 - a2 * (int)(a1 / a2));
}



static int compar (float *a, float *b)
{
    return (*a < *b ? -1 : *a == *b ? 0 : 1);
}



static void sort (float *a, int n)
{
    qsort (a, n, sizeof(float), (int(*)(const void *, const void *)) compar);
}



float mth_erfc (float x)

/*
 *
 * mth_erfc - return the complementary error function
 *
 *
 */

{
    float t, z, ans;

    z = fabs(x);
    t = 1. / (1. + .5 * z);
    ans = t * exp(-z * z - 1.26551223 + t * (1.00002368 + t * (0.37409196 + t *
	(0.09678418 + t * (-0.18628806 + t * (0.27886807 + t * (-1.13520398 + 
	t * (1.48851587 + t * (-0.82215223 + t * 0.17087277)))))))));

    if (x >= 0)
	return (ans);
    else
	return (2. - ans);
}



float mth_erf (float x)

/*
 *
 * mth_erf - return the error function
 *
 *
 */

{
    return (1. - mth_erfc (x));
}



void mth_fft (float *d, int n, int sign)

/*
 *
 * FUNCTIONAL DESCRIPTION:
 *
 *	Replaces DATA by its discrete Fourier transform, if ISIGN is input
 *	as 1; or replaces DATA by N times its inverse discrete Fourier
 *	transform, if ISIGN is input as -1. DATA is a complex array of
 *	length N or, equivalently, a real array of length 2*N. N must
 *	be an integer power of 2 (this is not checked for!).
 *
 * FORMAL ARGUMENT(S):
 *
 *	D	Real vector of length 2*N containing alternatly the real
 *		and the imaginary part of N complex numbers	
 *	N	Number of complex points (must be a power of 2)
 *	SIGN	Type of transform (1=direct, -1=inverse)
 *
 * IMPLICIT INPUTS:
 *
 *	NONE
 *
 * IMPLICIT OUTPUTS:
 *
 *	NONE
 *
 * SIDE EFFECTS:
 *
 *	NONE
 *
 * REFERENCE:
 *
 *	William H. Press, Brian P. Flannery, Saul A. Teukolsky and William
 *	T. Vetterling, Numerical Recipes, (Cambridge: Cambridge University
 *	Press, 1986) pp. 390-395.
 *
 * AUTHOR:	
 *
 *	Rainer Schmitz	    16-MAR-1988
 *
 */

{
    int i, j, m, step, m_max;
    double w_real, w_imag, wp_real, wp_imag, wtemp, theta;
    float temp_real, temp_imag;

    /* this is the bit-reversal section of the routine */
    n = 2*n;
    j = 1;
    i = 1;

    do 
	{
	if (j > i) 
	    {

	    /* exchange the two complex numbers */

	    temp_real = d[j-1];
	    temp_imag = d[j];
	    d[j-1] = d[i-1];
	    d[j] = d[i];
	    d[i-1] = temp_real;
	    d[i] = temp_imag; 
	}
	m = n/2;

	while ((m >= 2) && (j > m)) 
	    {
	    j = j - m;
	    m = m / 2; 
	    }
	j = j + m;
	i = i + 2; 
	}
    while (!(i > n));

    /* here begins the Danielson-Lanczos section of the routine;
     * outer loop executed log2 N times */

    m_max = 2;
    while (n > m_max) 
	{

	/* initialize the trigonometric recurrence */

	step = 2 * m_max;
	theta = 6.28318530717959;
	theta = theta / (sign * m_max);
	wp_real = -2. * pow(sin(0.5 * theta), 2.);
	wp_imag = sin(theta);
	w_real = 1;
	w_imag = 0;
	m = 1;

	do 
	    {
	    i = m;
	    do 
		{
		j = i + m_max;

		/* this is the Danielson-Lanczos formula: */

		temp_real = w_real * d[j-1] - w_imag * d[j];
		temp_imag = w_real * d[j] + w_imag * d[j-1];

		d[j-1] = d[i-1] - temp_real;
		d[j] = d[i] - temp_imag;
		d[i-1] = d[i-1] + temp_real;
		d[i] = d[i] + temp_imag;

		i = i + step; 
		}
	    while (!(i > n));

	    /* trigonometric recurrence */

	    wtemp = w_real;
	    w_real = w_real * wp_real - w_imag * wp_imag + w_real;
	    w_imag = w_imag * wp_real + wtemp * wp_imag + w_imag;
	    m = m + 2; 
	    }
	while (!(m > m_max));

	m_max = step; 
	}
}



#define SWAP(a, b) tempr = (a); (a) = (b); (b) = tempr

static
void fourn (float *data, int *nn, int ndim, int sign)
{
    int i2rev, i3rev, ip1, ip2, ip3, ifp1, ifp2;
    int ibit, idim, n, nprev, nrem, ntot;
    register int i1, i2, i3, k1, k2;
    float tempi, tempr;
    float theta, wi, wpi, wpr, wr, wtemp;

    data--;

    ntot=1;
    for (idim = 0; idim < ndim; idim++)
	ntot *= nn[idim];

    nprev = 1;
    for (idim = 0; idim < ndim; idim++)
	{
	n = nn[idim];
	nrem = ntot / (n * nprev);
	ip1 = nprev << 1;
	ip2 = ip1 * n;
	ip3 = ip2 * nrem;
	i2rev = 1;

	for (i2 = 1; i2 <= ip2; i2 += ip1)
	    {
	    if (i2 < i2rev)
	 	{
		for (i1 = i2; i1 <= i2 + ip1 - 2; i1 += 2)
		    {
		    for (i3 = i1; i3 <= ip3; i3 += ip2)
			{
			i3rev = i2rev + i3 - i2;
			SWAP(data[i3], data[i3rev]);
			SWAP(data[i3 + 1], data[i3rev + 1]);
			}
		    }
		}

	    ibit = ip2 >> 1;
	    while (ibit >= ip1 && i2rev > ibit)
		{
		i2rev -= ibit;
		ibit >>= 1;
		}
	    i2rev += ibit;
	    }

	ifp1 = ip1;
	while (ifp1 < ip2)
	    {
	    ifp2 = ifp1 << 1;
	    theta = sign * 6.28318530717959 / (ifp2 / ip1);
	    wtemp = sin(0.5 * theta);
	    wpr = -2.0 * wtemp * wtemp;
	    wpi = sin(theta);
	    wr = 1.0;
	    wi = 0.0;

	    for (i3 = 1; i3 <= ifp1; i3 += ip1)
		{
		for (i1 = i3; i1 <= i3+ip1-2; i1 += 2)
		    {
		    for (i2 = i1; i2 <= ip3; i2 += ifp2)
			{
			k1 = i2;
			k2 = k1 + ifp1;
			tempr = wr * data[k2] - wi * data[k2 + 1];
			tempi = wr * data[k2 + 1] + wi * data[k2];
			data[k2] = data[k1] - tempr;
			data[k2 + 1] = data[k1 + 1] - tempi;
			data[k1] += tempr;
			data[k1 + 1] += tempi;
			}
		    }

		wr = (wtemp = wr) * wpr - wi * wpi + wr;
		wi = wi * wpr + wtemp * wpi + wi;
		}

	    ifp1 = ifp2;
	    }

	nprev *= n;
	}
}

#undef SWAP


void mth_fft2 (float *data, int dimx, int dimy, int sign)
{
#ifdef DXML
    static void *handle = NULL, (*cfft_2d_a)();
    int ni, nj, lda, ni_stride, nj_stride;

    if (handle == NULL)
	{
	if ((handle = dlopen ("/usr/shlib/libdxml.so", RTLD_NOW)) != NULL)
	    cfft_2d_a = (void (*)()) dlsym (handle, "cfft_2d_");

	if (handle == NULL || cfft_2d_a == NULL)
	    {
	    tt_fprintf (stderr, "math.o: can't access DXML library\n");
	    exit (-1);
	    }
        }

    ni = dimx;
    nj = dimy;
    lda = dimx;
    ni_stride = 1;
    nj_stride = 1;
    cfft_2d_a ("C", "C", (sign == 1) ? "F" : "B", data, data, &ni, &nj, &lda,
	&ni_stride, &nj_stride);
#else
    int nn[2], ndim;
    register int i, n;
    register float fac;

    nn[0] = dimx;
    nn[1] = dimy;
    ndim = 2;

    fourn (data, nn, ndim, sign);

    if (sign == -1)
	{
	n = dimx * dimy * 2;
	fac = 2.0 / n;
	for (i = 0; i < n; i++)
	    data[i] *= fac;
	}
#endif
}



void mth_histogram (int n, float *x, int *m, float *f, int logging)

/*
 *
 * FUNCTIONAL DESCRIPTION:
 *
 *	Create a histogram with up to 40 cells. The sample mean,
 *	the sample variance, the number of degrees to calculate them,
 *	and the cell statistics may be logged.
 *
 * FORMAL ARGUMENT(S):
 *
 *	N	Number of data points
 *	X	Data set
 *	M	Number of cells
 *	F	Percent relative frequencies
 *	LOGGING	Logging flag
 *
 * IMPLICIT INPUTS:
 *
 *	NONE
 *
 * IMPLICIT OUTPUTS:
 *
 *	NONE
 *
 * SIDE EFFECTS:
 *
 *	NONE
 *
 * AUTHOR:	
 *
 *	Rainer Schmitz	    23-APR-1988
 *
 */

{
    float xi, xmin, xmax, width, sx, sxq, va;
    float fmax, delta_f;

    int i, j;
    int no, df, left, right;

    float std, std_rec;

    float left_cont, right_cont, cont;
    float sf, prob, prob_left;
    float gamma, chi, pchi, ef;

    BOOL done;
    float r0, r, r2;

    sx = 0;
    sxq = 0;

    for (i = 0; i < n; i++) 
	{
	sx = sx + x[i];
	sxq = sxq + x[i] * x[i]; 
	}

    va = (sxq - sx*sx/n)/(n - 1.);
    *m = 1 + (int)(3.3 * log10((double)n) + 0.5);

    xmin = x[0];
    xmax = x[0];

    for (i = 0; i < n; i++)
	if (x[i] < xmin)
	    xmin = x[i];
	else
	    if (x[i] > xmax)
		xmax = x[i];

    width = (xmax - xmin) / *m;

    delta_f = 100. / n;
    fmax = 0;

    for (i = 0; i < *m; i++)
	f[i] = 0;

    for (i = 1; i <= n; i++) 
	{
	j = (int)((x[i-1] - xmin) / width) + 1;

	if (j >= *m)
	    j = *m;

	f[j-1] = f[j-1] + delta_f;

	if (f[j-1] > fmax)
	    fmax = f[j-1]; 
	}

    if (logging) 
	{
	tt_printf ("\n");
	tt_printf ("%44s\n", "HISTOGRAM");
	tt_printf ("\n");
	tt_printf ("%s%d\n", "The number of observations is  ", n);
	tt_printf ("\n");
	tt_printf ("%s%1.3f\n", "The mean is calculated to be  ", sx/n);
	tt_printf ("%s%1.3f\n", "The variance is calculated to be  ",va);
	tt_printf ("\n");
	tt_printf ("%44s\n", "OBSERVED ");
	tt_printf ("%s%13s%13s%13s\n", "CELL ","MINIMUM","MAXIMUM","FREQUENCY");

	for (i = 1; i <= *m; i++) 
	    {
	    xi = xmin + (i - 1) * width;
	    tt_printf ("%3d%s%11.3f%s%11.3f%s%7d\n", i, "    ", xi, "  ",
		xi+width, "  ", (int)(f[i-1] * n / 100. + 0.5)); 
	    }
	tt_printf ("\n");

	/* perform a goodness-of-fit test to the normal distribution */

	std = sqrt(va);
	std_rec = 1. / std;

	/* find out the left and right cells */

	xi = xmin - sx / n;
	left = 0;
	left_cont = 0;
	sf = 0;
	done = FALSE;
	i = 1;

	do 
	    {
	    sf = sf + f[i-1];
	    xi = xi + width;
	    prob = mth_erf (xi * std_rec);

	    if (n * prob >= 3.)
		if ((n * prob > 7.) && (left_cont > 0))
		    done = TRUE;
		else 
		    {
		    left = i;
		    left_cont = pow((n * prob - sf * n / 100.), 2.) / 
			(n * prob); 
		    }

	    i++; 
	    }
	while (!((i > *m) || done));

	if (done) 
	    {
	    xi = xmax - sx / n;
	    right = 0;
	    right_cont = 0;
	    sf = 0;
	    done = FALSE;
	    i = *m;

	    do 
		{
		sf = sf + f[i-1];
		xi = xi - width;
		prob = 1. - mth_erf(xi * std_rec);

		if (n * prob >= 3.)
		    if ((n * prob > 7.) && (right_cont > 0))
			done = TRUE;
		    else 
			{
			right = i;
			right_cont = pow((n * prob - sf * n / 100.), 2.)  / 
			    (n * prob); 
			}
		i--; 
		}
	    while (!((i > *m) || done));

	    if (done && (right - left >= 0)) 
		{

		/* output a contribution table */

		tt_printf ("%43s%13s%18s\n", "OBSERVED", "EXPECTED", 
		    "CONTRIBUTION TO");
		tt_printf ("%s%13s%13s%13s%13s%15s\n", "CELL ", "MINIMUM", 
		    "MAXIMUM", "FREQUENCY", "FREQUENCY", "CHI-SQUARE");

		prob_left = 0;
		df = 0;
		chi = left_cont + right_cont;
		xi = xmin;

		for (i = 1; i <= *m; i++) 
		    {
		    xi = xi + width;
		    prob = mth_erf((xi - sx / n) * std_rec);
		    no = (int)(n * f[i-1] / 100. + 0.5);
		    ef = n * (prob - prob_left);

		    if (i == *m)
			ef = n * (1. - prob_left);

		    cont = pow((ef - no), 2.) / ef;

		    if ((i > left) && (i < right))
			chi = chi + cont;

		    if ((i >= left) && (i <= right))
			df++;

		    tt_printf ("%3d%s", i,"    ");

		    if ((i == 1) && (i <= left)) 
			{
			tt_printf ("%11s%s%11.3f%s%7d%s%11.3f%s", "-Infinity", 
			    "  ", xi, "  ", no, "      ", ef, "  ");

			if (i == left)
			    tt_printf ("%11.3f\n", left_cont);
			else
			    tt_printf ("%8s\n", "v"); 
			}

		    if ((i > 1) && (i <= left)) 
			{
			tt_printf ("%11.3f%s%11.3f%s%7d%s%11.3f%s", xi-width,
			    "  ", xi, "  ", no, "      ", ef, "  ");

			if (i == left)
			    tt_printf ("%11.3f\n", left_cont);
			else
			    tt_printf ("%8s\n", "v"); 
			}

		    if ((i > left) && (i < right))
			tt_printf ("%11.3f%s%11.3f%s%7d%s%11.3f%s%11.3f\n",
			    xi-width, "  ", xi, "  ", no, "      ", ef, "  ",
			    cont);

		    if ((i < *m) && (i >= right)) 
			{
			tt_printf ("%11.3f%s%11.3f%s%7d%s%11.3f%s",
			    xi-width, "  ", xi, "  ", no, "      ", ef, "  ");

			if (i == right)
			    tt_printf ("%11.3f\n", right_cont);
			else
			    tt_printf ("%8s\n", "^"); 
			}

		    if ((i == *m) && (i >= right)) 
			{
			tt_printf ("%11.3f%13s%s%7d%s%11.3f%s",
			    xi-width, "  Infinity", "  ", no, "      ", ef,
			    "  ");

			if (i == right)
			    tt_printf ("%11.3f\n", right_cont);
			else
			    tt_printf ("%8s\n", "^"); 
			}

		    prob_left = prob; 
		    }
		tt_printf ("\n");

		/* lose 3 degrees of freedom, one for number of cells, one for
		   estimated mean, and one for the estimated variance */
    
		df = df - 3;
		if (df <= 0) 
		    {

		    /* not enough degrees of freedom */

		    tt_printf ("%s%s\n", "The number of degrees of freedom ", 
			"available after collapsing the tail cells");
		    tt_printf ("%s%s\n", "to get large enough expected ",
			"frequencies have made it impossible to perform");
		    tt_printf ("%s\n", "the Chi-squared goodness of fit test.");
		    tt_printf ("\n"); 
		    }
		else 
		    {
		    prob = 0;

		    if (df + 10. * sqrt(2. * df) >= chi) 
			{
			gamma = 1.;
			r = (df + 2.) / 2. - 1.;

			while (r >= 1.5) 
			    {
			    gamma = gamma * r;
			    r = r - 1.; 
			    }

			if (df % 2 == 1)
			    gamma = gamma * 0.886226925;

			r = 0;
			j = 0;

			pchi = 1.;
			do 
			    {
			    j++;
			    pchi *= chi;

			    r2 = 1.;
			    for (i = 1; i <= j; i++)
				r2 = r2 * (df + 2. * i);
			    r0 = r;
			    r = r + pchi / r2; 
			    }
			while (!(((fabs(r - r0) / r) < 1.0e-6) || (j == 40)));
		
			prob = 1. - pow((chi / 2.), (df / 2.)) * 
			    exp(-chi / 2.) / gamma * (r + 1.); 
			}

		    tt_printf ("%s%d%s%s%1.3f\n", "Chi-squared with ", df,
			" degrees of ", "freedom = ", chi);
		    tt_printf ("%s%1.3f%s%1.3f\n", "P ( Chi-squared > ", chi,
			" ) = ", prob);
		    tt_printf ("\n"); }
		}
	    else 
		{
		tt_printf ("%s\n",
"There are not enough observations in the data set to perform the Chi-squared");
		tt_printf ("%s\n", "goodness of fit test.");
		tt_printf ("\n"); 
		}
	    }
	else 
	    {
	    tt_printf ("%s\n",
"There are not enough observations in the data set to perform the Chi-squared");
	    tt_printf ("%s\n", "goodness of fit test.");
	    tt_printf ("\n"); 
	    }
	}
}



float mth_gamma (float x)

/*
 *
 * mth_gamma - evaluate the GAMMA function
 *
 *
 */

{
    float r, a, top, den;
    int i, j;
    BOOL mflag;

    mflag = (x <= 0);
    x = fabs(x);

    if (mflag) 
	{
	i = (int) (x);
	r = pi / sin((x - i) * pi);

	if (i % 2 == 0)
	    r = -r;
	x = x + 1.; 
	}

    if (x <= 12.) 
	{
	i = (int)(x);
	a = 1.;

	if (i == 0) 
	    {
	    a = 1. / (x * (x + 1.));
	    x = x + 2.; 
	    }
	else
	    if (i == 1) 
		{
		a = 1. / x;
		x = x + 1.; 
		}
	    else
		if (i > 2)
		    for (j = 3; j <= i; j++) 
			{
			x = x - 1.;
			a = a * x; 
			}

	top = (((9.895546 - 1.889439 * x) * x - 51.49952) * x + 80.05398) * x -
	    201.4659;
	den = (((x - 19.52375) * x + 130.5263) * x - 303.5898) * x + 26.84174;
	a = (top / den) * a; 
	}
    else 
	{
	top = x * (log(x) - 1.) - 0.5 * log(x);
	x = 1. / x;
	a = exp((-0.2770927E-02 * x * x + 0.8333332E-01) * x + 0.9189385 + top);
	}

    if (mflag)
	a = r / a;

    return (a);
}



float mth_t (float p, int n)

/*
 *
 * mth_t - return the t-value
 *
 *
 */

{
    float d, v, z;

    d = n;
    v = sqrt(log(1. / pow(p, 2.0)));
    z = v - (2.515517 + 0.802853 * v + 0.010328 * pow(v, 2.0)) / (1. + 
	1.432788 * v + 0.189269 * pow(v, 2.0) + 0.001308 * pow(v, 3.0));
    return (z + (pow(z, 3.0) + z) / (4. * n) + (5. * pow(z, 5.0) + 16. * 
	    pow(z, 3.0) + 3. * z) / (96. * pow(d, 2.0)) + (3. * pow(z, 7.0) + 
	    19. * pow(z, 5.0) + 17. * pow(z, 3.0) - 15. * z) / (384. * 
	    pow(d, 3.0)) + (79. * pow(z, 9.0) + 776. * pow(z, 7.0) + 1482. * 
	    pow(z, 5.0) - 1920. * pow(z, 3.0) - 945. * z) / (92160. * 
	    pow(d, 4.0)));
}



void mth_linreg (int n, float *x, float *y, float *m, float *b, int logging)

/*
 *
 * FUNCTIONAL DESCRIPTION:
 *
 *	Compute the linear regression coefficients and perform an
 *	analysis of variance.
 *
 * FORMAL ARGUMENT(S):
 *
 *	N	Number of data points
 *	X, Y	Data points (X,Y)
 *	M, B	Regression coefficients
 *	LOGGING	Logging flag
 *
 * IMPLICIT INPUTS:
 *
 *	NONE
 *
 * IMPLICIT OUTPUTS:
 *
 *	NONE
 *
 * SIDE EFFECTS:
 *
 *	NONE
 *
 * AUTHOR:	
 *
 *	Josef Heinen	    20-APR-1988
 *
 */

{
    int i;
    float sx, sy, sxq, syq, sxy;
    float qx, qy, qxy, cc;
    float totss, regss, resss, regms, resms;
    float f, g;
    int dfreg, dfres, dftot;
    float qq, sb, sm, t;

    sx = 0;
    sy = 0;
    sxq = 0;
    syq = 0;
    sxy = 0;

    for (i = 0; i < n; i++) 
	{
	sx = sx + x[i];
	sy = sy + y[i];
	sxq = sxq + x[i] * x[i];
	syq = syq + y[i] * y[i];
	sxy = sxy + x[i] * y[i]; 
	}

    qx = sxq - sx * sx / n;
    qy = syq - sy * sy / n;
    qxy = sxy - sx * sy / n;

    *m = qxy / qx;			/* slope */
    *b = sy / n - *m * sx / n;		/* intercept */

    if (logging) 
	{
	cc = qxy / sqrt(qx * qy);	/* correlation between X and Y */

	totss = qy;			/* total sum of squares */
	regss = *m * *m * qx;		/* regression sum of squares */
	resss = totss - regss;		/* residual sum of squares */
	regms = regss;			/* regression mean squares */
	resms = resss / (n - 2);	/* residual mean squares */

	f = regms / resms;		/* F ratio */
	g = regss / totss;		/* goodness */

	dfreg = 1;			/* regression degrees of freedom */
	dfres = n - 2;			/* residual degrees of freedom */
	dftot = dfreg + dfres;		/* total degrees of freedom */

	qq = qy * (1 - g) / (n - 2);	/* residual variance */
	sb = sqrt(qq * sxq / (n * qx));
	sm = sqrt(qq / qx);
	t = mth_t (0.05 / 2., dfres);	/* t-value */

	tt_printf ("\n");
	tt_printf ("%s\n", "		      LINEAR REGRESSION");
	tt_printf ("\n");
	tt_printf ("%s\n", "LINEAR MODEL:  Y = MX + B");
	tt_printf ("%s%10.3f\n", "M= ",*m);
	tt_printf ("%s%10.3f\n", "B= ",*b);
	tt_printf ("\n");
	tt_printf ("%s%4.3f\n", "CORRELATION= ",cc);
	tt_printf ("\n");
	tt_printf ("%s\n", "		     ANALYSIS OF VARIANCE");
	tt_printf ("\n");
	tt_printf (
"SOURCE          DF         SQUARES    MEAN SQUARES     F RATIO\n");
	tt_printf ("\n");
	tt_printf ("%s%8d%16.3f%16.3f%12.3f\n", "REGRESSION",
	    dfreg, regss, regms, f);
	tt_printf ("%s%8d%16.3f%16.3f\n", "RESIDUAL  ", dfres, resss, resms);
	tt_printf ("%s%8d%16.3f\n", "TOTAL     ",dftot,totss);
	tt_printf ("\n");
	tt_printf ("%s%4.3f\n", "GOODNESS= ",g);
	tt_printf ("\n");
	tt_printf ("%s\n", "95 % CONFIDENCE INTERVAL ON INTERCEPT, SLOPE");
	tt_printf ("%s\n", "      LOWER LIMIT     UPPER LIMIT");
	tt_printf ("%s%16.3f%16.3f\n", "M", (float)(*m - t * sm),
	    (float)(*m + t * sm));
	tt_printf ("%s%16.3f%16.3f\n", "B", (float)(*b - t * sb),
	    (float)(*b + t * sb));
	tt_printf ("\n");
	}
}



static float rofunc (float m, int n, float *arr, float *x, float *y, float *b,
    float *dev)
{
    float d, sum;
    int i, n1, nmh, nml;

    n1 = n + 1;
    nml = n1 / 2;
    nmh = n1 - nml;

    for (i = 0; i < n; i++)
	arr[i] = y[i] - m * x[i];

    sort (arr, n);

    *b = 0.5 * (arr[nml-1] + arr[nmh-1]);

    sum = 0;
    *dev = 0;

    for (i = 0; i < n; i++) 
	{
	d = y[i] - (m * x[i] + *b);
	*dev = *dev + fabs(d);
	sum = sum + x[i] * sign(1.0, d); 
	}

    return (sum);
}



void mth_linfit (int n, float *x, float *y, float *m, float *b, float *dev,
    int logging)

/*
 *
 * FUNCTIONAL DESCRIPTION:
 *
 *	Fits y = mx + b by the criterion of least absolute deviations.
 *	The arrays X and Y, of length N, are the input experimental points.
 *	The fitted parameters M and B are output, along with DEV which is
 *	the mean deviation (in y) of the experimental points from the
 *	fitted line.
 *
 * FORMAL ARGUMENT(S):
 *
 *	N	Number of data points
 *	X, Y	Data points (X,Y)
 *	M, B	Fitted coefficients
 *	DEV	Mean absolute deviation
 *	LOGGING	Logging flag
 *
 * IMPLICIT INPUTS:
 *
 *	NONE
 *
 * IMPLICIT OUTPUTS:
 *
 *	NONE
 *
 * SIDE EFFECTS:
 *
 *	NONE
 *
 * AUTHOR:	
 *
 *	Josef Heinen	    4-MAY-1988
 *
 */

{
    int i;
    float sx, sy, sxq, sxy, chisq;
    float del, sigm, f, m1, f1, m2, f2;
    float *arr;

    arr = (float *) malloc (n*sizeof(float));

    /* As a first guess for M and B, we will find the least-squares fitting */
    /* line */

    sx = 0;
    sy = 0;
    sxq = 0;
    sxy = 0;

    for (i = 0; i < n; i++) 
	{
	sx = sx + x[i];
	sy = sy + y[i];
	sxq = sxq + x[i] * x[i];
	sxy = sxy + x[i] * y[i]; 
	}

    /* Least square solutions */

    del = n * sxq - pow(sx, 2.0);
    *b = (sxq * sy - sx * sxy) / del;
    *m = (n * sxy - sx * sy) / del;

    chisq = 0;
    for (i = 0; i < n; i++)
	chisq = chisq + pow((y[i] - (*m * x[i] + *b)),2.0);

    /* The standard deviation will give some idea of how big an iteration step
       to take */

    sigm = sqrt(chisq / del);

    m1 = *m;
    f1 = rofunc (m1, n, arr, x, y, b, dev);

    /* Guess bracket as 3-sigma away, in the downhill direction from F1 */

    m2 = *m + sign(3. * sigm, f1);
    f2 = rofunc (m2, n, arr, x, y, b, dev);

    while (f1 * f2 > 0) 
	{
	*m = 2 * m2 - m1;
	m1 = m2;
	f1 = f2;
	m2 = *m;
	f2 = rofunc (m2, n, arr, x, y, b, dev); 
	}

    /* Refine until error a neglible number of standard deviations */

    sigm = 0.01 * sigm;

    while (fabs(m2-m1) > sigm) 
	{
	*m = 0.5 * (m1 + m2);
	if ((*m == m1) || (*m == m2))
	    goto done;
	f = rofunc (*m, n, arr, x, y, b, dev);
	if (f*f1 >= 0) 
	    {
	    f1 = f;
	    m1 = *m; 
	    }
	else 
	    {
	    f2 = f;
	    m2 = *m; 
	    }
	}

    done:
    *dev = *dev / n;

    if (logging) 
	{
	tt_printf ("\n");
	tt_printf ("%s\n", "	ROBUST STRAIGHT-LINE FIT");
	tt_printf ("\n");
	tt_printf ("%s\n", "LINEAR MODEL:  Y = MX + B");
	tt_printf ("%s%10.3f\n", "M= ", *m);
	tt_printf ("%s%10.3f\n", "B= ", *b);
	tt_printf ("\n");
	tt_printf ("%s%10.3f\n", "MEAN ABSOLUTE DEVIATION= ", *dev);
	tt_printf ("\n");
	}

    free (arr);
}



float mth_ran (double *seed)

/*
 * mth_ran - return random number
 *
 */

{
    double result;
    
    do {
	*seed = dmod (16807.0 * *seed, 2147483647.0);
    	result = *seed / 2147483711.0;
	}
    while (result < 0.001);

    return (result);
}



float mth_rand (double *seed)

/*
 * mth_rand - return normally distributed random number
 *
 */

{
    static int iset = 0;
    static float gset;
    float fac, r, v1, v2;
    
    if (iset == 0) {
	do {
	    v1 = 2.0*mth_ran(seed)-1.0;
	    v2 = 2.0*mth_ran(seed)-1.0;
	    r = v1*v1+v2*v2;
	    }
	while (r >= 1.0 || r == 0.0);
	fac = sqrt(-2.0*log(r)/r);
	gset = v1*fac;
	iset = 1;
	return (v2*fac);
	}
    else {
	iset = 0;
	return (gset);
	}
}



void mth_realft (float *d, int n, int sign)

/*
 *
 * FUNCTIONAL DESCRIPTION:
 *
 *	Calculates the Fourier transform of a set of 2*N real valued data
 *	points. Replaces the data (which is stored in array DATA) by the
 *	positive frequency half of its complex Fourier transform. The real
 *	valued first and last components of the complex transform are
 *	returned as elements DATA(1) and DATA(2) respectively. N must be a
 *	power of 2. This routine also calculates the inverse transform of
 *	a complex data array if it is the transform of real data.
 *
 * FORMAL ARGUMENT(S):
 *
 *	D	Real vector of length 2*N containing the real valued data
 *		points
 *	N	Number of points (must be a power of 2)
 *	SIGN	Type of transform (1=direct, -1=inverse)
 *
 * IMPLICIT INPUTS:
 *
 *	NONE
 *
 * IMPLICIT OUTPUTS:
 *
 *	NONE
 *
 * SIDE EFFECTS:
 *
 *	NONE
 *
 * REFERENCE:
 *
 *	William H. Press, Brian P. Flannery, Saul A. Teukolsky and William
 *	T. Vetterling, Numerical Recipes, (Cambridge: Cambridge University
 *	Press, 1986) pp. 398-400.
 *
 * AUTHOR:	
 *
 *	Rainer Schmitz	    16-MAR-1988
 *
 */

{
    int i, i1, i2, i3, i4, n2p3;
    double w_real, w_imag, wp_real, wp_imag, wtemp, theta;
    float c1, c2, h1_real, h1_imag, h2_real, h2_imag, w_real_s, w_imag_s;

    /* initialize the recurrence */

    theta = 6.28318530717959;
    theta = theta / (2.0 * n);
    c1 = 0.5;

    if (sign == 1) 
	{

	/* the forward transform is here */

	c2 = -0.5;
	mth_fft (d, n, 1); 
	}
    else 
	{

	/* otherwise set up an inverse transform */

	c2 = 0.5;
	theta = -theta; 
	}

    wp_real = -2.0 * pow(sin(0.5 * theta), 2.0);
    wp_imag = sin(theta);
    w_real = 1.0 + wp_real;
    w_imag = wp_imag;
    n2p3 = 2 * n + 3;

    /* start loop with i=2, case i=1 done separately below */

    for (i = 2; i <= (int)(n / 2) + 1; i++) 
	{
	i1 = 2 * i - 1;
	i2 = i1 + 1;
	i3 = n2p3 - i2;
	i4 = i3 + 1;
	w_real_s = w_real;
	w_imag_s = w_imag;

	/* two separate transforms are separated out of z */

	h1_real = c1 * (d[i1-1] + d[i3-1]);
	h1_imag = c1 * (d[i2-1] - d[i4-1]);
	h2_real = -c2 * (d[i2-1] + d[i4-1]);
	h2_imag = c2 * (d[i1-1] - d[i3-1]);

	/* here they are recombined to form the true transform of the
	   original real data */

	d[i1-1] = h1_real + w_real_s * h2_real - w_imag_s * h2_imag;
	d[i2-1] = h1_imag + w_real_s * h2_imag + w_imag_s * h2_real;
	d[i3-1] = h1_real - w_real_s * h2_real + w_imag_s * h2_imag;
	d[i4-1] = -h1_imag + w_real_s * h2_imag + w_imag_s * h2_real;

	/* the recurrence */

	wtemp = w_real;
	w_real = w_real * wp_real - w_imag * wp_imag + w_real;
	w_imag = w_imag * wp_real + wtemp * wp_imag + w_imag; 
	}

    if (sign == 1) 
	{
	h1_real = d[0];

	/* squeeze the first and last data together to get them all within
	   the original array */

	d[0] = h1_real + d[1];
	d[1] = h1_real - d[1]; 
	}
    else 
	{
	h1_real = d[0];
	d[0] = c1 * (h1_real + d[1]);
	d[1] = c1 * (h1_real - d[1]);

	/* this is the inverse transform for the case SIGN=-1 */

	mth_fft (d, n, -1);

	/* take care of scaling by 1/N */

	for (i = 0; i < 2*n; i++)
	    d[i] = d[i] / n; 
	}
}



static void running_medians (float *g, float *f, int n, int l)

/*
 *  Calculate running medians of length L from the array F and
 *  put them into G.
 *
 */

{
    float *b;
    int m, mp1, mp2, n1, n3, n5, nm, i, i5, j;
    float fmed;

    b = (float *) malloc((l+1) * sizeof(float));

    m = l / 2;
    mp1 = m + 1;
    mp2 = m + 2;
    n1 = n - l;

    /* first m+1 values */

    for (i = 1; i <= l; i++)
	*(b+i-1) = f[i-1];

    nm = -1;

    for (i = 1; i <= mp1; i++) 
	{
	nm = nm + 2;
	sort (b, nm);
	g[i-1] = *(b+i-1);
	}
    for (j = 1; j <= n1; j++) 
	{
	i = 0;

	do
	    i++;
	while (!((i >= l) || (*(b+i-1) == f[j-1])));

	n3 = i;

	/* n3 is now the position in b of f[j], which is at low end of
	   last range. Before choosing this median, f[j] will be dropped,
	   and f[j+l] picked up. */

	if (f[j+l-1] < *(b+mp1-1))

	    if (n3 >= mp1) 
		{
		fmed = f[j+l-1];
		n5 = 0;

		for (i5 = 1; i5 <= m; i5++)
		    if (*(b+i5-1) > fmed) 
			{
			fmed = *(b+i5-1);
			n5 = i5;
			}

		*(b+n3-1) = *(b+mp1-1);
		*(b+mp1-1) = fmed;

		if (n5 != 0)
		    *(b+n5-1) = f[j+l-1];
		}
	    else
		*(b+n3-1) = f[j+l-1];
	else
	    if (n3 <= mp1) 
		{
		fmed = f[j+l-1];
		n5 = 0;

		for (i5 = mp2; i5 <= l; i5++)
		    if (*(b+i5-1) < fmed) 
			{
			fmed = b[i5-1];
			n5 = i5;
			}

		*(b+n3-1) = *(b+mp1-1);
		*(b+mp1-1) = fmed;

		if (n5 != 0)
		    *(b+n5-1) = f[j+l-1];
		}
	    else
		*(b+n3-1) = f[j+l-1];

	g[j+mp1-1] = *(b+mp1-1);
	}

    for (i = 1; i <= l; i++)
	*(b+i-1) = f[n+1-i-1];

    i = 0;
    nm = 1;
	    
    do {
	sort (b, nm);
	g[n-i-1] = *(b+i+1-1);
	i++;
	nm = nm+2;
	}
    while (!(nm > l));

    free (b);
}



static void running_means (float *g, float *f, int n, int l)

/*
 *  Calculate running means of length L from the array F and put
 *  the result into G.
 *
 */

{
    int i, j, m;
    float rec_j, temp1, temp2;

    m = l / 2;

    /* The first and last M positions of G are filled with the means
       of the first (last) 1,3,..,l2 elements of F */

    temp1 = f[0];
    g[0] = temp1;
    temp2 = f[n-1];
    g[n-1] = temp2;
    j = 1;

    for (i = 1; i <= m; i++) 
	{
	temp2 = temp2 + f[n-j-1] + f[n-j-2];
	j = j + 2;
	temp1 = temp1 + f[j-2] + f[j-1];
	rec_j = 1.0 / j;
	g[i+1-1] = temp1 * rec_j;
	g[n-i-1] = temp2 * rec_j;
	}

    for (i = 1; i <= n-l-1; i++) 
	{
	temp1 = temp1 + f[i+l-1] - f[i-1];
	g[i+m] = temp1 * rec_j;
	}
}



static void quadratic_hanning (float *g, float *f, int n)

/*
 * quadratic hanning routine
 *
 */

{
    int i;

    if (n <= 4)
	for (i = 1; i <= n; i++)
	    g[i-1] = f[i-1];
    else 
	{
	g[0] = f[0];
	g[1] = ((f[0] + f[2]) / 2.0 + f[1]) / 2.0;
	g[2] = ((f[1] + f[3]) / 2.0 + f[2]) / 2.0;

	for (i = 3; i < n-1; i++) 
	    {
	    if ((f[i-2] != f[i-1]) || (f[i-1] != f[i]) ||
	       ((f[i-2] >= f[i-3]) && (f[i+1] >= f[i])) ||
	       ((f[i-2] <= f[i-3]) && (f[i+1] <= f[i])))

		/* normal case, calculate using quadratic fit */

		g[i] = ((f[i-1]+f[i+1])/2.0+f[i])/2.0;
	    else 
		{

		/* special case  f[i-3]<=f[i-2]=f[i-1]f[i]>=f[i+1] (or >=)
		   recalculate smoothed values using cubic fit */

		g[i-2] = f[i-2];
		g[i-1] = (4.0 * (f[i-2] + f[i]) - f[i-3] - f[i+1]) / 6.0;
		g[i] = f[i];
		}
	    }
	g[n-1] = f[n-1];
	}
}



void mth_smooth (int n, float *d, float *g, int level)

/*
 *
 * FUNCTIONAL DESCRIPTION:
 *
 *	Smooth a given sequence of data points.
 *   	The smoother procedure is a non-linear one. As opposed to linear
 *	procedure its performance shouldn't be impaired if the data is not
 *	well behaved.
 *	Y is assumed to be a sequence of observed values of a function
 *	at equally spaced intervals.
 *	At the completion of the call Y is left unmodified and SMY gives
 *	the smoothed sequence.
 *
 * FORMAL ARGUMENT(S):
 *
 *	N	number of points, must be greater than one
 *	Y	real vector of length N containing the observations
 *	SMY	real vector of length N returning the smoothed data
 *	LEVEL	smoothing level (1-20)
 *
 * IMPLICIT INPUTS:
 *
 *	NONE
 *
 * IMPLICIT OUTPUTS:
 *
 *	NONE
 *
 * SIDE EFFECTS:
 *
 *	NONE
 *
 */

{
    float *e, *f;
    int l, n1, n2, nm, i;
    float step, step_2, temp, dqu, uqu;

    e = (float *) malloc(n * sizeof(float));
    f = (float *) malloc(n * sizeof(float));

    if (n <= 2) 
	{
	g[0] = d[0];
	g[1] = d[1];
	}
    else 
	{
	if ((int)(n/2) <= level + 1)
	    l = (int)(n / 2);
	else
	    l = level + 1;

	n1 = 2 * l - 1;			/* n1 = 1,3,5,7,9,... */
	n2 = 2 * ((l - 1) / 2) + 1;	/* n2 = 1,1,3,3,5,... */

	running_medians (e, d, n, n1);
	running_medians (f, e, n, n2);
	quadratic_hanning (g, f, n);

	for (i = 1; i <= n; i++) 
	    {
	    *(f+i-1) = d[i-1] - g[i-1];
	    *(e+i-1) = *(f+i-1);
	    }

	step = 1.5;
	step_2 = step + step;

	sort (e, n);

	/* get 25-percentile points */

	if ((int)(n / 4) > 1)
	    nm = (int)(n / 4);
	else
	    nm = 1;

	dqu = *(e+nm-1);
	uqu = *(e+n+1-nm-1);

	if (1.5 * (uqu - dqu) > 0.2)
	    step = 1.5 * (uqu - dqu);
	else
	    step = 0.2;

	step_2 = step + step;

	for (i = 1; i <= n; i++) 
	    {
	    temp = fabs(*(f+i-1));

	    if (temp > step) 
		{

		if (temp < step_2) 
		    {
		    if (*(f+i-1) > 0)
			*(f+i-1) = fabs(step_2-temp);

		    if (*(f+i-1) < 0)
			*(f+i-1) = -fabs(step_2-temp);
		    }
		else
		    *(f+i-1) = 0.0;
		}
	    }

	running_means (e, f, n, n1);
	running_medians (f, e, n, n2);
	quadratic_hanning (e, f, n);

	for (i = 1; i <= n; i++)
	    g[i-1] = g[i-1] + *(e+i-1);

	running_means (e, g, n, n1);
	running_medians (f, e, n, n2);
	quadratic_hanning (e, f, n);

	for (i = 1; i <= n; i++)
	    *(f+i-1) = g[i-1] - *(e+i-1);

	running_means (g, f, n, n1);
	running_medians (f, g, n, n2);
	quadratic_hanning (g, f, n);

	for (i = 1; i <= n; i++)
	    g[i-1] = *(e+i-1) + g[i-1];
	}

    free (f);
    free (e);
}



void mth_spline (int n, double *x, double *y, int m, double *t, double *s)

/*
 *
 * FUNCTIONAL DESCRIPTION:
 *
 *	Compute a curve s(x) that passes through the N data points (X,Y)
 *	supplied by the user and computes the functional value S(T) at any
 *	point T on the curve.
 *	The procedure to determine s(x) involves the iterative solution of
 *	a set of simultaneous linear equations by Young's method of
 *	successive over-relaxation.
 *
 * FORMAL ARGUMENT(S):
 *
 *	N	Number of data points
 *	X, Y	Data points (X,Y)
 *	M	Number of domain values
 *	T, S	Domain and functional values (T,S)
 *
 * IMPLICIT INPUTS:
 *
 *	NONE
 *
 * IMPLICIT OUTPUTS:
 *
 *	NONE
 *
 * SIDE EFFECTS:
 *
 *	NONE
 *
 * REFERENCES:
 *
 *	1. Ralston and Wilf, Mathematical Methods for Digital Computers,
 *	   Vol. II (New York: John Wiley and Sons, 1967) pp. 156-158.
 *
 *	2. Greville, T.N.E., Editor, "Proceedings of An Advanced Seminar
 *	   Conducted by the Mathematics Research Center", U.S. Army,
 *	   University of Wisconsin, Madison. October 7-9, 1968. Theory
 *	   and Applications of Spline Functions (New York, London: Aca-
 *	   demic Press, 1969), pp. 156-167.
 *
 * AUTHOR:	
 *
 *	Josef Heinen	    06-APR-1988
 *
 */

{
    int i, j;
    double *g;
    double w, u0, u, delta_g;

    g = (double *) malloc(n * sizeof(double));

    for (i = 1; i < n-1; i++) 
	{
	*(g+i) = 2. * ((y[i+1] - y[i]) / (x[i+1] - x[i]) - (y[i] - y[i-1]) /
	    (x[i] - x[i-1])) / (x[i+1] - x[i-1]);
	s[i] = 1.5 * *(g+i);
	}

    *g = 0;
    *(g+n-1) = 0;
    w = 8. - 4. * sqrt(3.);
    u0 = 0;

    do 
	{
	u = 0;

	for (i = 1; i < n-1; i++) 
	    {
	    delta_g = w * (-*(g+i) - 0.5 * (x[i] - x[i-1]) / (x[i+1] - x[i-1]) *
		    *(g+i-1) - (0.5 - 0.5 * (x[i] - x[i-1]) / 
		    (x[i+1] - x[i-1])) * *(g+i+1) + s[i]);
	    *(g+i) = *(g+i) + delta_g;

	    if (fabs(delta_g) > u)
		u = fabs(delta_g);
	    }

	if (u0 == 0)
	    u0 = u;
	}
    while (!(u <= u0*eps));

    for (j = 0; j < m; j++) 
	{
	i = 1;

	while (x[i] < t[j])
	    i++;

	i--;

	if (i >= n)
	    i = n - 1;

	s[j] = y[i] + (y[i+1] - y[i]) / (x[i+1] - x[i]) * (t[j] - x[i]) +
	       (t[j] - x[i]) * (t[j] - x[i+1]) * (2. * *(g+i) + *(g+i+1) + 
	       (*(g+i+1) - *(g+i)) / (x[i+1] - x[i]) * (t[j] - x[i])) / 6.;
	}

    free (g);
}



void mth_b_spline (int n, double *x, double *y, int m, double *sx, double *sy)

/*
 * mth_b_spline takes n input points. It uses parameter t 
 * to compute sx(t) and sy(t) respectively.
 */

{
    double t, bl1, bl2, bl3, bl4;
    int i, j;
    double interval, xi_3, yi_3, xi, yi;

    interval = (double)(n-1) / (double)(m);

    for (i=2, j=0; i<=n; i++)
        {
	if (i == 2) {
	    xi_3 = x[0] - (x[1]-x[0]);
	    yi_3 = (y[1]*(xi_3-x[0]) - y[0]*(xi_3-x[1])) / (x[1]-x[0]);
	    }
	else {
	    xi_3 = x[i-3]; yi_3 = y[i-3];
	    }
	if (i == n) {
	    xi = x[n-1] + (x[n-1]-x[n-2]);
	    yi = (y[n-1]*(xi-x[n-2]) - y[n-2]*(xi-x[n-1])) / (x[n-1]-x[n-2]);
	    }
	else {
	    xi = x[i]; yi = y[i];
	    }

	t = fmod(j*interval, 1.0);

        while (t < 1.0 && j < m)
            {
	    bl1 = (1.0 - t)*(1.0 - t)*(1.0 - t)/6.0;
	    bl2 = (3.0*t*t*t - 6.0*t*t + 4.0)/6.0;
	    bl3 = (-3.0*t*t*t + 3.0*t*t + 3.0*t + 1.0)/6.0;
            bl4 = t*t*t/6.0;

            sx[j] = bl1*xi_3 + bl2*x[i-2] + bl3*x[i-1] + bl4*xi;
            sy[j] = bl1*yi_3 + bl2*y[i-2] + bl3*y[i-1] + bl4*yi;

            t += interval;
	    j++;
            }
	}
}



void mth_normal (int n, float *x, float *y, int m, float *d, float *f,
    int logging)

/*
 *
 * FUNCTIONAL DESCRIPTION:
 *
 *	Compute a normal curve overlay.
 *
 * FORMAL ARGUMENT(S):
 *
 *	N	Number of data points
 *	X, Y	Data points (X,Y)
 *	M	Number of domain values
 *	D, F	Domain and functional values (D,F)
 *
 * IMPLICIT INPUTS:
 *
 *	NONE
 *
 * IMPLICIT OUTPUTS:
 *
 *	NONE
 *
 * SIDE EFFECTS:
 *
 *	NONE
 *
 * AUTHOR:	
 *
 *	Josef Heinen	    28-OCT-1988
 *
 */

{
    int i;
    float p, pmax, mu, sigma, std;

    p = 0;
    pmax = y[0];

    for (i = 1; i < n; i++)
	{
	p = p + y[i];

	if (y[i] > pmax)
	    pmax = y[i];
	}

    mu = 0;
    for (i = 0; i < n; i++)
	mu = mu + x[i] * y[i] / p;

    sigma = 0;
    for (i = 0; i < n; i++)
	sigma = sigma + pow(x[i] - mu, 2.0) * y[i] / p;

    std = sqrt(sigma);
    for (i = 0; i < m; i++)
	{
	d[i] = mu -3.5 * std + i * 7. * std / (m - 1.);
	f[i] = pmax * exp(pow((d[i] - mu) / (-2. * sigma), 2.0));
	}

    if (logging)
	{
	tt_printf ("\n");
	tt_printf ("%s\n", "	    NORMAL CURVE OVERLAY");
	tt_printf ("\n");
	tt_printf ("%s%d\n", "The number of observations is  ",n);
	tt_printf ("\n");
	tt_printf ("%s%1.3f\n", "The mean is calculated to be  ",mu);
	tt_printf ("%s%1.3f\n", "The variance is calculated to be  ",sigma);
	tt_printf ("\n");
	}
}
