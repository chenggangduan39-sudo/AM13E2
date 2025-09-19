#include "wtk_equalizer.h"
#include "wtk/core/math/wtk_math.h"

#define GAIN_F0 1.0f
#define TETA(f,sfreq) (2*(3.14159265358979f)*(float)f/sfreq)
#define TWOPOWER(value) (value * value)
#define GAIN_F1 GAIN_F0 / (1.41421356237309504880f)

#define BETA2(tf0, tf) \
(TWOPOWER(GAIN_F1)*TWOPOWER(cos(tf0)) \
 - 2.0 * TWOPOWER(GAIN_F1) * cos(tf) * cos(tf0) \
 + TWOPOWER(GAIN_F1) \
 - TWOPOWER(GAIN_F0) * TWOPOWER(sin(tf)))
#define BETA1(tf0, tf) \
    (2.0 * TWOPOWER(GAIN_F1) * TWOPOWER(cos(tf)) \
     + TWOPOWER(GAIN_F1) * TWOPOWER(cos(tf0)) \
     - 2.0 * TWOPOWER(GAIN_F1) * cos(tf) * cos(tf0) \
     - TWOPOWER(GAIN_F1) + TWOPOWER(GAIN_F0) * TWOPOWER(sin(tf)))
#define BETA0(tf0, tf) \
    (0.25 * TWOPOWER(GAIN_F1) * TWOPOWER(cos(tf0)) \
     - 0.5 * TWOPOWER(GAIN_F1) * cos(tf) * cos(tf0) \
     + 0.25 * TWOPOWER(GAIN_F1) \
     - 0.25 * TWOPOWER(GAIN_F0) * TWOPOWER(sin(tf)))

#define GAMMA(beta, tf0) ((0.5f + beta) * cos(tf0))
#define ALPHA(beta) ((0.5f - beta)/2.0f)


int wtk_equalizer_set_value(wtk_equalizer_t *eq,float value,int band)
{
    int ret = 0;
    if(band < 0)
    {
        eq->preamp = 9.9999946497217584440165E-01f * exp(6.9314738656671842642609E-02f * value) + 3.7119444716771825623636E-07f;
    }else
    {
        eq->band[band].gain =  2.5220207857061455181125E-01f * exp(8.0178361802353992349168E-02f * value) - 2.5220207852836562523180E-01f;
        eq->band[band].value = value;
    }
    return ret;
}

static int wtk_equalizer_find_f1f2(int rate, float octave, float *f1, float *f2)
{
    int ret = 0;
    float octave_factor = pow(2.0f, octave/2.0f);
    *f1 = rate/octave_factor;
    *f2 = rate*octave_factor;
    return ret;
}

static int wtk_equalizer_find_root(float a, float b, float c, float *x0)
{
  float k = c-((b*b)/(4.f*a));
  float h = -(b/(2.f*a));
  float x1 = 0.f;

  if (-(k/a) < 0.f){ return -1; }
  *x0 = h - sqrt(-(k/a));
  x1 = h + sqrt(-(k/a));
  if (x1 < *x0){ *x0 = x1; }

  return 0;
}


int wtk_equalizer_calc_coeffs(wtk_equalizer_t *eq, float *eqrate)
{
    int ret = 0;
    int i;
    float f1,f2;
    float x0;
    int band_n=eq->cfg->band_count;

    for(i = 0; i < band_n; ++i)
    {
        wtk_equalizer_find_f1f2(eqrate[i],eq->cfg->octave,&f1,&f2);
        ret = wtk_equalizer_find_root(
            BETA2(TETA(eqrate[i],eq->sfreq),TETA(f1,eq->sfreq)),
            BETA1(TETA(eqrate[i],eq->sfreq),TETA(f1,eq->sfreq)),
            BETA0(TETA(eqrate[i],eq->sfreq),TETA(f1,eq->sfreq)),
            &x0
            );
        if(ret == 0)
        {
            /* Got a solution, now calculate the rest of the factors */
            /* Take the smallest root always (find_root returns the smallest one)
            *
            * NOTE: The IIR equation is
            *	y[n] = 2 * (alpha*(x[n]-x[n-2]) + gamma*y[n-1] - beta*y[n-2])
            *  Now the 2 factor has been distributed in the coefficients
            */
            /* Now store the coefficients */
            eq->band[i].coeffs.beta = 2.0f*x0;
            eq->band[i].coeffs.alpha = 2.0 * ALPHA(x0);
            eq->band[i].coeffs.gamma = 2.0 * GAMMA(x0, TETA(eqrate[i],eq->sfreq));
            // printf("Freq[%d]: %f beta %.10e alpha %.10e gamma %.10e %f %f %f  %f\n",i,eqrate[i],eq->band[i].coeffs.beta,eq->band[i].coeffs.alpha,eq->band[i].coeffs.gamma,x0,f1,f2,QCTAVE);
        }else
        {
            /* Shouldn't happen error*/
            eq->band[i].coeffs.beta = 0;
            eq->band[i].coeffs.alpha = 0;
            eq->band[i].coeffs.gamma = 0;
        }
    }
    return ret;
}

wtk_equalizer_t* wtk_equalizer_new(wtk_equalizer_cfg_t *cfg)
{
    wtk_equalizer_t *eq = NULL;

    eq = wtk_malloc(sizeof(wtk_equalizer_t));
    eq->cfg=cfg;
    eq->sfreq=cfg->sfreq;
    eq->band = wtk_malloc(cfg->band_count*sizeof(wtk_equalizer_band_t));
    wtk_equalizer_reset(eq);

    return eq;
}


//init the filters
void wtk_equalizer_reset(wtk_equalizer_t *eq)
{
    int i;
    int band_n=eq->cfg->band_count;

    memset(eq->band,0,sizeof(wtk_equalizer_band_t)*band_n);
    wtk_equalizer_set_value(eq,20.f,-1);
    for(i = 0; i < band_n; ++i)
    {
        wtk_equalizer_set_value(eq,eq->cfg->value[i],i);
    }
    wtk_equalizer_calc_coeffs(eq,eq->cfg->rate);
    for(i = 0; i < 256; ++i)
    {
        eq->dither[i] = (rand()%4)-2;
    }
    eq->di = 0;
    eq->ni = 2;
    eq->nj = 1;
    eq->nk = 0;
}

/*
use IIR fliter y[n] = 2 * (alpha*(x[n]-x[n-2]) + gamma*y[n-1] - beta*y[n-2])
*/

int wtk_equalizer_feed(wtk_equalizer_t *eq,short *data,int len)
{
    int ret = 0;
    int i, j;
    float p = 0.0f,out = 0.0f;
    int outi = 0;
    int band_n=eq->cfg->band_count;
    wtk_equalizer_band_t *band = NULL;
    float alpha,gamma,beta;

    for(i = 0; i < len; ++i)
    {
        p = data[i] * eq->preamp;
        p += eq->dither[eq->di];
        out = 0;
        band=eq->band;
        for(j = 0; j < band_n; ++j, ++band)
        {
            alpha=band->coeffs.alpha;
            gamma=band->coeffs.gamma;
            beta=band->coeffs.beta;

            band->x[eq->ni] = p;
            band->y[eq->ni] = alpha*(band->x[eq->ni] - band->x[eq->nk])
                                                + gamma*band->y[eq->nj]-beta*band->y[eq->nk];
            out += band->gain * band->y[eq->ni]; 
        }
        if(eq->cfg->extra_filter)
        {
            band=eq->band;
            for(j = 0; j < band_n; ++j, ++band)
            {
                alpha=band->coeffs.alpha;
                gamma=band->coeffs.gamma;
                beta=band->coeffs.beta;

                band->x2[eq->ni] = out;
                band->y2[eq->ni] = alpha*(band->x2[eq->ni] - band->x2[eq->nk])
                                                    + gamma*band->y2[eq->nj]-beta*band->y2[eq->nk];
                out += band->gain * band->y2[eq->ni]; 
            }
        }
    
        out += p * 0.25;
        out -= eq->dither[eq->di] * 0.25;

        if(out < -32768){ out = -32768; }
        else if(out > 32767) {out = 32767; }
        outi = out;
        data[i] = outi;

        eq->ni = (eq->ni+1)%3;
        eq->nj = (eq->nj+1)%3;
        eq->nk = (eq->nk+1)%3;
        eq->di = (eq->di+1)%256;
    }

    return ret;
}

int wtk_equalizer_feed_float(wtk_equalizer_t *eq,float *data,int len)
{
    int ret = 0;
    int i, j;
    float p = 0.0f,out = 0.0f;
    int band_n=eq->cfg->band_count;
    wtk_equalizer_band_t *band = NULL;
    float alpha,gamma,beta;

    for(i = 0; i < len; ++i)
    {
        p = data[i] * eq->preamp;
        p += eq->dither[eq->di];
        out = 0;
        band=eq->band;
        for(j = 0; j < band_n; ++j, ++band)
        {
            alpha=band->coeffs.alpha;
            gamma=band->coeffs.gamma;
            beta=band->coeffs.beta;

            band->x[eq->ni] = p;
            band->y[eq->ni] = alpha*(band->x[eq->ni] - band->x[eq->nk])
                                                + gamma*band->y[eq->nj]-beta*band->y[eq->nk];
            out += band->gain * band->y[eq->ni]; 
        }
        if(eq->cfg->extra_filter)
        {
            band=eq->band;
            for(j = 0; j < band_n; ++j, ++band)
            {
                alpha=band->coeffs.alpha;
                gamma=band->coeffs.gamma;
                beta=band->coeffs.beta;

                band->x2[eq->ni] = out;
                band->y2[eq->ni] = alpha*(band->x2[eq->ni] - band->x2[eq->nk])
                                                    + gamma*band->y2[eq->nj]-beta*band->y2[eq->nk];
                out += band->gain * band->y2[eq->ni]; 
            }
        }
    
        out += p * 0.25;
        out -= eq->dither[eq->di] * 0.25;
        data[i] = out;

        eq->ni = (eq->ni+1)%3;
        eq->nj = (eq->nj+1)%3;
        eq->nk = (eq->nk+1)%3;
        eq->di = (eq->di+1)%256;
    }

    return ret;
}

void wtk_equalizer_delete(wtk_equalizer_t *equalizer)
{
    wtk_free(equalizer->band);
    wtk_free(equalizer);
}
