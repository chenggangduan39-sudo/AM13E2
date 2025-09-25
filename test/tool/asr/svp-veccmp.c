#include "wtk/asr/kws/qtk_audio2vec.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/core/cfg/wtk_source.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/math/wtk_matrix.h"
#include "wtk/core/rbin/wtk_flist.h"

typedef struct wtk_rate {
    double wave_time;
    double cpu_time;
    double time;
    double rescore_time;
    double rec_mem;
    double rescore_mem;
} wtk_rate_t;

void wtk_rate_init(wtk_rate_t *r) {
    r->wave_time = 0;
    r->cpu_time = 0;
    r->time = 0;
    r->rescore_time = 0;
    r->rec_mem = 0;
    r->rescore_mem = 0;
}

void test_wav_file(qtk_audio2vec_t *dec, char *fn, wtk_rate_t *rate, float **vv,
                   int *ll) {
    double t, t2;
    double wav_t;
    wtk_strbuf_t **buf;
    int n;
    buf = wtk_riff_read_channel(fn, &n);

    printf("==>fn: %s\n", fn);

    wav_t = ((buf[0]->pos) * 1.0 / 32); // for 16k
    if (rate) {
        rate->wave_time += wav_t;
    }
    t2 = time_get_cpu();
    t = time_get_ms();
    qtk_audio2vec_start(dec);
    {
        char *s, *e;
        int step = 4000;
        int nx;
        double nt;

        nt = 0;
        s = buf[0]->data;
        e = s + buf[0]->pos;
        while (s < e) {
            nx = min(step, e - s);
            nt += nx / 2;
            qtk_audio2vec_feed(dec, (short *)s, nx >> 1);
            s += nx;
        }
        qtk_audio2vec_feed_end(dec);
        //		wtk_debug("want end delay= %f\n",time_get_ms()-tx1);
    }
    qtk_audio2vec_get_result(dec, vv, ll);
    // wtk_debug("%d\n",*ll);
    // print_float(*vv,10);
    t2 = time_get_cpu() - t2;
    t = time_get_ms() - t;
    if (rate) {
        rate->cpu_time += t2;
        rate->time += t;
    }
    wtk_debug("time=%f,timerate=%f,cpurate=%f\n", t, t / wav_t, t2 / wav_t);
    qtk_audio2vec_reset(dec);
    //	wtk_free(data);
}

int main(int argc, char **argv) {
    wtk_main_cfg_t *main_cfg = 0;
    wtk_arg_t *arg;
    qtk_audio2vec_t *dec = 0;
    qtk_audio2vec_cfg_t *cfg = NULL;
    char *cfg_fn = 0;
    char *wav_fn1 = 0;
    char *wav_fn2 = 0;
    int bin = 0;

    arg = wtk_arg_new(argc, argv);

    wtk_arg_get_str_s(arg, "c", &cfg_fn);
    wtk_arg_get_str_s(arg, "wav1", &wav_fn1);
    wtk_arg_get_str_s(arg, "wav2", &wav_fn2);
    wtk_arg_get_int_s(arg, "b", &bin);
    //	if(!cfg_fn || (!scp_fn && !wav_fn))
    //	{
    //		print_usage(argc,argv);
    //		goto end;
    //	}

    // wtk_debug("======== rec is ready =============\n");
    if (cfg_fn) {

        main_cfg = wtk_main_cfg_new_type(qtk_audio2vec_cfg, cfg_fn);
        if (!main_cfg) {
            wtk_debug("load configure failed.\n");
            goto end;
        }
        cfg = (qtk_audio2vec_cfg_t *)main_cfg->cfg;

        dec = qtk_audio2vec_new(cfg);
        if (!dec) {
            wtk_debug("create decoder failed.\n");
            goto end;
        }
    }
    //    qtk_audio2vec_enroll(dec,"liuyong",0);
    //    qtk_audio2vec_enroll(dec,"liuyong",strlen("liuyong"));

    if (wav_fn1) {
        float **a1, **a2;
        float x[256];
        a1 = wtk_malloc(sizeof(float *));
        a2 = wtk_malloc(sizeof(float *));
        int len;
        test_wav_file(dec, wav_fn1, NULL, a1, &len);
        memcpy(x, *a1, sizeof(float) * 256);
        if (wav_fn2) {
            test_wav_file(dec, wav_fn2, NULL, a2, &len);
            // print_float(x,len);
            // print_float(*a2,len);
            float prob;
            prob = qtk_audio2vec_eval(dec, x, *a2, len);
            printf("prob: %f\n", prob);
        }
    }

end:

    if (dec) {
        qtk_audio2vec_delete(dec);
    }
    if (main_cfg) {
        wtk_main_cfg_delete(main_cfg);
    }
    wtk_arg_delete(arg);

    return 0;
}
