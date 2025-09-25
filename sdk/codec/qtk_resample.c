#include "qtk_resample.h"
#include "wtk/bfio/resample/wtk_resample.h"

struct qtk_resample
{
    wtk_resample_t *resample;
    void *ths;
    qtk_resample_notify_f notify;
};

void qtk_resample_on_data(qtk_resample_t *s, char *data, int len)
{
    s->notify(s->ths, data, len);
}

qtk_resample_t* qtk_resample_new(void)
{
    qtk_resample_t *s;

    s=(qtk_resample_t*)malloc(sizeof(*s));

    s->ths = NULL;
    s->notify = NULL;
    s->resample = NULL;

    s->resample=wtk_resample_new(1024);
    if(!s->resample){goto end;}
    wtk_resample_set_notify(s->resample, s, (wtk_resample_notify_f)qtk_resample_on_data);

end:
    return s;
}

void qtk_resample_delete(qtk_resample_t* s)
{
    if(s->resample)
    {
        wtk_resample_close(s->resample);
        wtk_resample_delete(s->resample);
    }
    free(s);
}

void qtk_resample_set_notify(qtk_resample_t *s,void *ths, qtk_resample_notify_f notify)
{
    s->ths = ths;
    s->notify = notify;
}

int qtk_resample_start(qtk_resample_t *s,int src_rate, int dst_rate)
{
   return wtk_resample_start(s->resample, src_rate, dst_rate);
}

int qtk_resample_feed(qtk_resample_t *s,const char *data,int len)
{
    return wtk_resample_feed(s->resample, (char *)data, len, 0);
}

void qtk_resample_reset(qtk_resample_t *s)
{
    wtk_resample_feed(s->resample, NULL, 0, 1);
    wtk_resample_close(s->resample);
}
