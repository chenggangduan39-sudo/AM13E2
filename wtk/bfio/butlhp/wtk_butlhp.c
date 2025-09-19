#include "wtk_butlhp.h"

#define ROOT2 (1.4142135623730950488)

wtk_butlhp_t *wtk_butlhp_new(int fc, int rate, int is_hp)
{
    wtk_butlhp_t *butf = NULL;
    float *a = NULL;
    float c = 0.0f;

    butf = wtk_malloc(sizeof(wtk_butlhp_t));
    butf->rate = rate;
    butf->lkf = fc;
    butf->pidsr = PI/butf->rate*1.0f;
    butf->x[0] = butf->x[1] = 0.0f;

    if(!is_hp)
    {
        a = butf->a;
        c = 1.0f/tanf(butf->pidsr*butf->lkf);
        a[0] = 0.0f;
        a[1] = 1.0 / ( 1.0 + ROOT2 * c + c * c);
        a[2] = a[1] + a[1];
        a[3] = a[1];
        a[4] = 2.0 * ( 1.0 - c*c) * a[1];
        a[5] = ( 1.0 - ROOT2 * c + c * c) * a[1];
    }else
    {
        a = butf->a;
        c = tanf(butf->pidsr*butf->lkf);
        a[0] = 0.0f;
        a[1] = 1.0 / ( 1.0 + ROOT2 * c + c * c);
        a[2] = -(a[1] + a[1]);
        a[3] = a[1];
        a[4] = 2.0 * ( c*c-1.0) * a[1];
        a[5] = ( 1.0 - ROOT2 * c + c * c) * a[1];
    }


    return butf;
}

int wtk_butlhp_feed(wtk_butlhp_t *butlhp,short *in, int len)
{
    int ret = 0;
    int i = 0;
    float t = 0.0f,y=0.0f;
    float *a = butlhp->a;
    float *x = butlhp->x;

    for(i = 0; i < len; ++i)
    {
        t = in[i] - a[4]*x[0]-a[5]*x[1];
        y = t*a[1]+a[2]*x[0]+a[3]*x[1];
        x[1] = x[0];
        x[0] = t;
        if(y>32767)
        {
            y=32767;
        }else if(y<-32767)
        {
            y=-32767;
        }
        in[i] = y;
    }
    return ret;
}

int wtk_butlhp_feed_float(wtk_butlhp_t *butlhp,float *in, int len)
{
    int ret = 0;
    int i = 0;
    float t = 0.0f,y=0.0f;
    float *a = butlhp->a;
    float *x = butlhp->x;

    for(i = 0; i < len; ++i)
    {
        t = in[i] - a[4]*x[0]-a[5]*x[1];
        y = t*a[1]+a[2]*x[0]+a[3]*x[1];
        x[1] = x[0];
        x[0] = t;
        in[i] = y;
        if(y>32767.0f)
        {
            y=32767.0f;
        }else if(y<-32767.0f)
        {
            y=-32767.0f;
        }
    }
    return ret;
}

int wtk_butlhp_delete(wtk_butlhp_t *but)
{
    wtk_free(but);
    return 0;
}