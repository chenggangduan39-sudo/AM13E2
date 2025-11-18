#include "wtk/bfio/sweetspot/qtk_sweetspot.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/json/wtk_json_parse.h"
#include "wtk/core/rbin/wtk_flist.h"
#include "wtk/core/wtk_arg.h"
#include "wtk/core/wtk_riff.h"
#include "wtk/core/wtk_type.h"
#include "wtk/core/wtk_wavfile.h"

void _on_data(void *upval, short **out, int len){
    for (int i = 0; i < len; i++){
        fwrite(out[0] + i, sizeof(short), 1, (FILE *)upval);
        fwrite(out[1] + i, sizeof(short), 1, (FILE *)upval);
    }
}

static void test_sweet(qtk_sweetspot_t *ss, char *ifn, char *ofn)
{
    wtk_riff_t *riff;
    short *pv;
    short **data;
    int channel = 2;
    int len, bytes;
    int ret;
    int i, j;

    FILE *fp = fopen(ofn, "w");
    if (fp == NULL){
        exit(0);
    }
    
    qtk_sweetspot_set_notify(ss, fp, _on_data);
    riff = wtk_riff_new();
    qtk_sweetspot_set(ss, 0.003, 0.5);
    len = 256;
    bytes = sizeof(short) * channel * len;
    pv = (short *)wtk_malloc(sizeof(short) * channel * len);

    data = (short **)wtk_malloc(sizeof(short *) * channel);
    for (i = 0; i < channel; ++i)
    {
        data[i] = (short *)wtk_malloc(sizeof(short) * len);
    }
    wtk_riff_open(riff, ifn);
    while (1)
    {
        ret = wtk_riff_read(riff, (char *)pv, bytes);
        if (ret <= 0)
        {
            wtk_debug("break ret=%d\n", ret);
            break;
        }
        len = ret / (channel * sizeof(short));
        for (i = 0; i < len; ++i)
        {
            for (j = 0; j < channel; ++j)
            {
                data[j][i] = pv[i * channel + j];
            }
        }
        qtk_sweetspot_feed(ss, data, len, 0);
    }

    wtk_riff_delete(riff);
    wtk_free(pv);
    for (i = 0; i < channel; ++i)
    {
        wtk_free(data[i]);
    }
    wtk_free(data);
}

static void test_sweet2(qtk_sweetspot_t *ss, char *ifn, char *ofn, float x, float y)
{
    wtk_riff_t *riff;
    short *pv;
    short **data;
    int channel = 2;
    int len, bytes;
    int ret;
    int i, j;

    FILE *fp = fopen(ofn, "w");
    if (fp == NULL){
        exit(0);
    }
    
    qtk_sweetspot_set_notify(ss, fp, _on_data);
    riff = wtk_riff_new();
    qtk_sweetspot_update(ss, x, y);
    len = 256;
    bytes = sizeof(short) * channel * len;
    pv = (short *)wtk_malloc(sizeof(short) * channel * len);

    data = (short **)wtk_malloc(sizeof(short *) * channel);
    for (i = 0; i < channel; ++i)
    {
        data[i] = (short *)wtk_malloc(sizeof(short) * len);
    }
    wtk_riff_open(riff, ifn);
    while (1)
    {
        ret = wtk_riff_read(riff, (char *)pv, bytes);
        if (ret <= 0)
        {
            wtk_debug("break ret=%d\n", ret);
            break;
        }
        len = ret / (channel * sizeof(short));
        for (i = 0; i < len; ++i)
        {
            for (j = 0; j < channel; ++j)
            {
                data[j][i] = pv[i * channel + j];
            }
        }
        qtk_sweetspot_feed(ss, data, len, 0);
    }

    wtk_riff_delete(riff);
    wtk_free(pv);
    for (i = 0; i < channel; ++i)
    {
        wtk_free(data[i]);
    }
    wtk_free(data);
}

int main(int argc, char **argv)
{
    qtk_sweetspot_cfg_t *cfg=NULL;
    wtk_main_cfg_t *main_cfg=0;
    qtk_sweetspot_t *ss;
    wtk_arg_t *arg;
    char *cfg_fn, *ifn, *ofn;
    float x=-1000.0,y=-1.0;
    arg = wtk_arg_new(argc, argv);
    if (!arg)
    {
        goto end;
    }
    wtk_arg_get_str_s(arg, "c", &cfg_fn);
    wtk_arg_get_str_s(arg, "i", &ifn);
    wtk_arg_get_str_s(arg, "o", &ofn);
    wtk_arg_get_float_s(arg, "x", &x);
    wtk_arg_get_float_s(arg, "y", &y);


    main_cfg=wtk_main_cfg_new_type(qtk_sweetspot_cfg,cfg_fn);
    cfg=(qtk_sweetspot_cfg_t*)main_cfg->cfg;
    ss = qtk_sweetspot_new(cfg);

    if(x < -999){
        test_sweet(ss, ifn, ofn);
    }else{
        test_sweet2(ss, ifn, ofn, x, y);
    }

    qtk_sweetspot_delete(ss);
end:
    if (arg)
    {
        wtk_arg_delete(arg);
    }
    wtk_main_cfg_delete(main_cfg);
    return 0;
}
