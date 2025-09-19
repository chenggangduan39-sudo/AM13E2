#include "getf0.h"

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/* Compute the pp+1 autocorrelation lags of the windowsize samples in s.
* Return the normalized autocorrelation coefficients in r.
* The rms is returned in e.
*/
void xautoc( register int windowsize, register float *s, register int p, register float *r, register float *e )
{
	register int i, j;
	register float *q, *t, sum, sum0;

	for( i=windowsize, q=s, sum0=0.0; i--;) {
		sum = *q++;
		sum0 += sum*sum;
	}
	*r = 1.;			/* r[0] will always =1. */
	if(sum0 == 0.0) {		/* No energy: fake low-energy white noise. */
		*e = 1.;			/* Arbitrarily assign 1 to rms. */
		/* Now fake autocorrelation of white noise. */
		for ( i=1; i<=p; i++){
			r[i] = 0.;
		}
		return;
	}
	*e = (float) sqrt((double)(sum0/windowsize));
	sum0 = (float) (1.0/sum0);
	for( i=1; i <= p; i++){
		for( sum=0.0, j=windowsize-i, q=s, t=s+i; j--; )
			sum += (*q++) * (*t++);
		*(++r) = sum*sum0;
	}
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/* Using Durbin's recursion, convert the autocorrelation sequence in r
* to reflection coefficients in k and predictor coefficients in a.
* The prediction error energy (gain) is left in *ex.
* Note: durbin returns the coefficients in normal sign format.
*	(i.e. a[0] is assumed to be = +1.)
*/
void xdurbin ( register float *r,register float * k, register float *a,register int p, register float *ex)
{
	float  bb[BIGSORD];
	register int i, j;
	register float e, s, *b = bb;

	e = *r;
	*k = -r[1]/e;
	*a = *k;
	e *= (float) (1. - (*k) * (*k));
	for ( i=1; i < p; i++){
		s = 0;
		for ( j=0; j<i; j++){
			s -= a[j] * r[i-j];
		}
		k[i] = ( s - r[i+1] )/e;
		a[i] = k[i];
		for ( j=0; j<=i; j++){
			b[j] = a[j];
		}
		for ( j=0; j<i; j++){
			a[j] += k[i] * b[i-j-1];
		}
		e *= (float) ( 1. - (k[i] * k[i]) );
	}
	*ex = e;
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/*  Compute the autocorrelations of the p LP coefficients in a.
*  (a[0] is assumed to be = 1 and not explicitely accessed.)
*  The magnitude of a is returned in c.
*  2* the other autocorrelation coefficients are returned in b.
*/
void xa_to_aca (float * a, float *b, float *c, register int p )
{
	register float  s, *ap, *a0;
	register int  i, j;

	for ( s=1., ap=a, i = p; i--; ap++ )
		s += *ap * *ap;

	*c = s;
	for ( i = 1; i <= p; i++){
		s = a[i-1];
		for (a0 = a, ap = a+i, j = p-i; j--; )
			s += (*a0++ * *ap++);
		*b++ = (float) (2. * s);
	}

}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/* Compute the Itakura LPC distance between the model represented
* by the signal autocorrelation (r) and its residual (gain) and
* the model represented by an LPC autocorrelation (c, b).
* Both models are of order p.
* r is assumed normalized and r[0]=1 is not explicitely accessed.
* Values returned by the function are >= 1.
*/
float xitakura (register int p,register float * b,register float * c, register float *r,register float * gain )
{
	register float s;

	for( s= *c; p--; )
	{
		//wtk_debug("r=%f\n",*r);
		//wtk_debug("b=%f\n",*b);
		s += *r++ * *b++;
		//wtk_debug("s=%f\n",s);
	}
	//wtk_debug("%f=%d\n",*c,p);
	//wtk_debug("%f\n",s);
	return (s/ *gain);
}


/*      ----------------------------------------------------------      */
int lc_lin_fir(register float	fc,int	*nf,float	*coef)
/* create the coefficients for a symmetric FIR lowpass filter using the
window technique with a Hanning window. */
{
	register int	i, n;
	register double	twopi, fn, c;

	if(((*nf % 2) != 1))
		*nf = *nf + 1;
	n = (*nf + 1)/2;

	/*  Compute part of the ideal impulse response (the sin(x)/x kernel). */
	twopi = M_PI * 2.0;
	coef[0] = (float) (2.0 * fc);
	c = M_PI;
	fn = twopi * fc;
	for(i=1;i < n; i++) coef[i] = (float)(sin(i * fn)/(c * i));

	/* Now apply a Hanning window to the (infinite) impulse response. */
	/* (Probably should use a better window, like Kaiser...) */
	fn = twopi/(double)(*nf);
	for(i=0;i<n;i++)
		coef[n-i-1] *= (float)((.5 - (.5 * cos(fn * ((double)i + 0.5)))));

	return(TRUE);
} // lc_lin_fir()

/* ----------------------------------------------------------------------- */
/* Use parabolic interpolation over the three points defining the peak
* vicinity to estimate the "true" peak. */
void peak(float *y, float *xp, float *yp)
{
	register float a, c;

	a = (float)((y[2]-y[1])+(.5*(y[0]-y[2])));
	if(fabs(a) > .000001) {
		*xp = c = (float)((y[0]-y[2])/(4.0*a));
		*yp = y[1] - (a*c*c);
	} else {
		*xp = 0.0;
		*yp = y[1];
	}
}


/*--------------------------------------------------------------------*/
int get_Nframes(long buffsize,int pad,int step)
{
	if (buffsize < pad)
		return (0);
	else
		return ((buffsize - pad)/step);
}

void xrwindow(register float * din, register float *dout,register int n, register float preemp)
{
	register float *p;

	/* If preemphasis is to be performed,  this assumes that there are n+1 valid
	samples in the input buffer (din). */
	if(preemp != 0.0) {
		for( p=din+1; n-- > 0; )
			*dout++ = (float)((*p++) - (preemp * *din++));
	} else {
		for( ; n-- > 0; )
			*dout++ =  *din++;
	}
}

/* ----------------------------------------------------------------------- */
/* Get likely candidates for F0 peaks. */
void get_cand(Cross *cross,float *peak,int *loc,int nlags,int *ncand,float cand_thresh)
{
	register int i, lastl, *t;
	register float o, p, q, *r, *s, clip;
	int start, ncan;//, maxl;

	clip = (float) (cand_thresh * cross->maxval);
	//maxl = cross->maxloc;
	lastl = nlags - 2;
	start = cross->firstlag;

	r = cross->correl;
	o= *r++;			/* first point */
	q = *r++;	                /* middle point */
	p = *r++;
	s = peak;
	t = loc;
	ncan=0;
	for(i=1; i < lastl; i++, o=q, q=p, p= *r++){
		if((q > clip) &&		/* is this a high enough value? */
			(q >= p) && (q >= o)){ /* NOTE: this finds SHOLDERS and PLATEAUS
								   as well as peaks (is this a good idea?) */
				*s++ = q;		/* record the peak value */
				*t++ = i + start;	/* and its location */
				ncan++;			/* count number of peaks found */
		}
	}
	*ncand = ncan;
}
