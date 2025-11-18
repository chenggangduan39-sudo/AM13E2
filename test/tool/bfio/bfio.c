#include "wtk/bfio/wtk_bfio.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/json/wtk_json_parse.h"
#include "wtk/core/rbin/wtk_flist.h"
#include "wtk/core/wtk_arg.h"
#include "wtk/core/wtk_riff.h"
#include "wtk/core/wtk_type.h"
#include "wtk/core/wtk_wavfile.h"

static int theta = 0;
static int phi = 0;
static wtk_wavfile_t *out_wav;
static int wake_cnt = 0;
static int out_len = 0;
// static int skip_channel[2]={4,5};
static int skip_channel[1]={-1};
static wtk_strbuf_t *wake_buf=NULL;

static int save_asr=0;
static int save_wake=0;

int max_buf_len=10*16000*2;
static int pop_len=0;

static void print_usage()
{
    printf("bfio usage:\n");
    printf("\t-c cfg\n");
    printf("\t-i input wav file\n");
    printf("\t-o output wav file\n");
    printf("\n");
}

static void test_bfio_on_ssl2(wtk_bfio_t *bfio, wtk_ssl2_extp_t *nbest_extp, int nbest)
{
    int i;

    for (i = 0; i < nbest; ++i)
    {
        // printf("%d ==> %d %d\n", i, nbest_extp[i].theta, nbest_extp[i].phi);
    }
}

static void test_bfio_on(wtk_bfio_t *bfio, wtk_bfio_cmd_t cmd, short *data, int len)
{
    static int ki = 0;
    static int ki_w = 0;
    static wtk_wavfile_t *wlog = NULL;
    static wtk_wavfile_t *wake_wav=NULL;
    wtk_json_parser_t *jsp = NULL;
    // wtk_json_item_t *res, *conf;
    char buf[512];
    int fs, f_len;

    if(save_wake){
        if(wake_buf==NULL){
            wake_buf=wtk_strbuf_new(1024, 1);
            wtk_strbuf_reset(wake_buf);
        }
    }

    switch (cmd)
    {
    case WTK_BFIO_VAD_START:
        // wtk_debug("vad start fs=%f \n", bfio->vad_ts);
        if(save_asr){
            ++ki;
            wlog = wtk_wavfile_new(bfio->cfg->rate);
            wlog->max_pend = 0;
            sprintf(buf, "vx/%d.wav", ki);
            // wtk_debug("open vad=%s\n", buf);
            wtk_wavfile_open(wlog, buf);
        }
        break;
    case WTK_BFIO_VAD_DATA:
        if (len > 0)
        {
            if (wlog)
            {
                wtk_wavfile_write(wlog, (char *)data, len * sizeof(short));
            }
            wtk_wavfile_write(out_wav, (char *)data, len * sizeof(short));
            out_len += len;
        }
        if(wake_buf){
            wtk_strbuf_push(wake_buf, (char *)data, len *sizeof(short));
            int tmp_len=0;
            if(wake_buf->pos>=max_buf_len){
                tmp_len=wake_buf->pos-max_buf_len;
                wtk_strbuf_pop(wake_buf, NULL, tmp_len);
                pop_len += tmp_len;
            }
        }
        break;
    case WTK_BFIO_VAD_END:
        // wtk_debug("vad end fe=%f \n", bfio->vad_te);
        if (wlog)
        {
            wtk_wavfile_close(wlog);
            wtk_wavfile_delete(wlog);
            wlog = NULL;
        }
        break;
    case WTK_BFIO_WAKE:
        printf("%f %f %f %d\n", bfio->wake_fs + bfio->cfg->wake_ssl_fs, bfio->wake_fe + bfio->cfg->wake_ssl_fe, bfio->out_wake_prob, bfio->wake_key);
        fflush(stdout);
        fs = (int)floor((bfio->wake_fs + bfio->cfg->wake_ssl_fs)*bfio->cfg->rate);
        f_len = (int)floor(((bfio->wake_fe + bfio->cfg->wake_ssl_fe)-(bfio->wake_fs + bfio->cfg->wake_ssl_fs))*bfio->cfg->rate);
        if(wake_buf){
            printf("%d %d %d\n", fs, f_len, wake_buf->pos);
            wake_wav=wtk_wavfile_new(bfio->cfg->rate);
            wake_wav->max_pend = 0;
            ++ki_w;
            sprintf(buf, "wk/%d.wav", ki_w);
            // wtk_debug("open vad=%s\n", buf);
            wtk_wavfile_open(wake_wav, buf);
            wtk_wavfile_write(wake_wav, wake_buf->data+fs*2-pop_len, f_len * sizeof(short));
            wtk_wavfile_delete(wake_wav);
            wake_wav = NULL;
        }
        // printf("[wake] %f %f\n", bfio->wake_fs, bfio->wake_fe);
        ++wake_cnt;
        // --ki;
        break;
    case WTK_BFIO_WAKE_SSL:
        // printf("[ssl] theta=%d\n", bfio->wake_theta);
        break;
    case WTK_BFIO_VAD_CANCEL:
        out_len -= len;
        // wtk_debug("vad cancle len=%d out_len=%d\n", len, out_len);
        if(wlog){
            wtk_wavfile_cancel(wlog, len * sizeof(short));
        }
        wtk_wavfile_cancel(out_wav, len * sizeof(short));
        if(wake_buf){
            wake_buf->pos-=len*sizeof(short);
        }
        break;
    case WTK_BFIO_ASR_CANCEL:
        // wtk_debug("asr cancle len=%d out_len=%d\n", len, out_len);
        break;
    case WTK_BFIO_ASR_RES:
        jsp = wtk_json_parser_new();
        wtk_json_parser_parse(jsp, (char *)data, len);
        // conf = wtk_json_obj_get_s(jsp->json->main, "conf");
        // res = wtk_json_obj_get_s(jsp->json->main, "rec");
        // printf("[asr] %f %f %.*s/%f\n", bfio->vad_ts, bfio->vad_te,
        //        res->v.str->len, res->v.str->data, conf->v.number);
        // break;
    case WTK_BFIO_WAKE_RES:
        break;
    case WTK_BFIO_SIL_END:
        printf("sil end one short\n");
        break;
    case WTK_BFIO_SPEECH_END:
        printf("speech end one short\n");
        break;
    case WTK_BFIO_WAKE2:
        printf("wake2\n");
        break;
    }
    if (jsp)
    {
        wtk_json_parser_delete(jsp);
    }
}

static void test_bfio_file2(wtk_bfio_t *bfio, char *scp, char *odir)
{
    wtk_flist_it_t *it;
    wtk_riff_t *riff;
    wtk_wavfile_t *wav;
    short *pv;
    short **data;
    int channel = bfio->channel + 1;
    int len, bytes;
    int ret;
    int i, j;
    char *ifn = NULL, *ofn = NULL;
    wtk_strbuf_t *buf, *buf2;

    buf = wtk_strbuf_new(256, 1);
    buf2 = wtk_strbuf_new(256, 1);

    wav = wtk_wavfile_new(bfio->cfg->rate);
    wav->max_pend = 0;
    riff = wtk_riff_new();

    len = 20 * 16;
    bytes = sizeof(short) * channel * len;
    pv = (short *)wtk_malloc(sizeof(short) * channel * len);

    data = (short **)wtk_malloc(sizeof(short *) * channel);
    for (i = 0; i < channel; ++i)
    {
        data[i] = (short *)wtk_malloc(sizeof(short) * len);
    }
    wtk_bfio_set_notify(bfio, bfio, (wtk_bfio_notify_f)test_bfio_on);
    wtk_bfio_start(bfio);

    it = wtk_flist_it_new(scp);
    while (1)
    {
        ifn = wtk_flist_it_next(it);
        if (!ifn)
        {
            break;
        }

        printf("%s\n", ifn);
        wtk_strbuf_reset(buf);
        wtk_strbuf_push(buf, ifn, strlen(ifn) + 1);
        ofn = buf->data + (buf->pos - 7);
        wtk_strbuf_reset(buf2);
        wtk_strbuf_push(buf2, odir, strlen(odir));
        wtk_strbuf_push(buf2, "/", 1);
        wtk_strbuf_push(buf2, ofn, strlen(ofn) + 1);
        ofn = buf2->data;
        printf("%s\n", ofn);

        wtk_wavfile_open(wav, ofn);
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
            wtk_bfio_feed(bfio, data, len, 0);
        }

        wtk_riff_close(riff);
        wtk_wavfile_close(wav);
    }
    wtk_bfio_feed(bfio, NULL, 0, 1);

    wtk_strbuf_delete(buf);

    wtk_flist_it_delete(it);
    wtk_riff_delete(riff);
    wtk_wavfile_delete(wav);
    wtk_free(pv);
    for (i = 0; i < channel; ++i)
    {
        wtk_free(data[i]);
    }
    wtk_free(data);
}

static void test_bfio_file(wtk_bfio_t *bfio, char *ifn, char *ofn)
{
    wtk_riff_t *riff;
    short *pv;
    short **data;
    int channel;
    int len, bytes;
    int ret;
    int i, j, k;
    int b;
    int pos;
    double t;
    int cnt;
    int nskip = sizeof(skip_channel) / sizeof(int);

    out_wav = wtk_wavfile_new(bfio->cfg->rate);
    out_wav->max_pend = 0;
    wtk_wavfile_open(out_wav, ofn);

    riff = wtk_riff_new();
    wtk_riff_open(riff, ifn);

    channel = riff->fmt.channels;
    len = 20 * 16;
    bytes = sizeof(short) * channel * len;
    pv = (short *)wtk_malloc(sizeof(short) * channel * len);

    data = (short **)wtk_malloc(sizeof(short *) * channel);
    for (i = 0; i < channel; ++i)
    {
        data[i] = (short *)wtk_malloc(sizeof(short) * len);
    }
    wtk_bfio_set_notify(bfio, bfio, (wtk_bfio_notify_f)test_bfio_on);
    wtk_bfio_set_ssl2_notify(bfio, bfio, (wtk_bfio_notify_ssl2_f)test_bfio_on_ssl2);
    wtk_bfio_start(bfio);
    // wtk_bfio_set_wake_words(bfio, "小爱同学|天猫精灵", strlen("小爱同学|天猫精灵"));
    // wtk_bfio_set_wake_words(bfio, "天猫精灵", strlen("天猫精灵"));
    //wtk_bfio_set_wake_words(bfio, "你好飞燕", strlen("你好飞燕"));

    cnt = 0;
    t = time_get_ms();
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
            pos = 0;
            for (j = 0; j < channel; ++j)
            {
                b = 0;
                for (k = 0; k < nskip; ++k)
                {
                    if (j == skip_channel[k])
                    {
                        b = 1;
                    }
                }
                if (b == 0)
                {
                    data[pos++][i] = pv[i * channel + j];
                }
            }
        }
        // if(cnt==0){  // 0s
        //     // start kws reg
        //     wtk_bfio_set_offline_qform(bfio, 1000, -1);
        // }
        // if(cnt==160000){  // 10s
        //     // end kws reg
        //     wtk_bfio_feed(bfio, NULL, 0, 1);
        //     wtk_bfio_reset(bfio);
        //     wtk_bfio_start(bfio);
        //     wtk_strbuf_reset(wake_buf);
        //     pop_len = 0;
        // }
        // if(cnt==480000){  // 30s
        //     // start kws reg
        //     wtk_bfio_feed(bfio, NULL, 0, 1);
        //     wtk_bfio_reset(bfio);
        //     wtk_bfio_start(bfio);
        //     wtk_bfio_set_offline_qform(bfio, 1000, -1);
        // }
        // if(cnt==640000){  // 40s
        //     // end kws reg
        //     wtk_bfio_feed(bfio, NULL, 0, 1);
        //     wtk_bfio_reset(bfio);
        //     wtk_bfio_start(bfio);
        //     wtk_strbuf_reset(wake_buf);
        //     pop_len = 0;
        // }
        cnt += len;
        wtk_bfio_feed(bfio, data, len, 0);
    }
    wtk_bfio_feed(bfio, NULL, 0, 1);

    t = time_get_ms() - t;
    wtk_debug("===>>> wake_cnt=%d rate=%f t=%f cnt=%d out=%d\n", wake_cnt, t / (cnt / 16.0), t, cnt, out_len);

    wtk_riff_delete(riff);
    wtk_wavfile_delete(out_wav);
    if(wake_buf){
        wtk_strbuf_delete(wake_buf);
    }
    wtk_free(pv);
    for (i = 0; i < channel; ++i)
    {
        wtk_free(data[i]);
    }
    wtk_free(data);
}

int main(int argc, char **argv)
{
    wtk_bfio_cfg_t *cfg=NULL;
    wtk_bfio_t *bfio;
    wtk_arg_t *arg;
    char *cfg_fn, *ifn, *ofn, *bin_fn;
    char *scp = NULL;

    arg = wtk_arg_new(argc, argv);
    if (!arg)
    {
        goto end;
    }
    wtk_arg_get_str_s(arg, "c", &cfg_fn);
    wtk_arg_get_str_s(arg, "b", &bin_fn);
    wtk_arg_get_str_s(arg, "i", &ifn);
    wtk_arg_get_str_s(arg, "o", &ofn);
    wtk_arg_get_int_s(arg, "theta", &theta);
    wtk_arg_get_int_s(arg, "phi", &phi);
    wtk_arg_get_str_s(arg, "scp", &scp);
    if ((!cfg_fn && !bin_fn) || (!ifn && !scp) || !ofn)
    {
        print_usage();
        goto end;
    }

    if (cfg_fn)
    {
        cfg = wtk_bfio_cfg_new(cfg_fn);
    }
    if (bin_fn)
    {
        cfg = wtk_bfio_cfg_new_bin(bin_fn);
    }
    // wtk_bfio_cfg_set_wakeword(cfg, "小爱同学");
    bfio = wtk_bfio_new(cfg);

    if (scp)
    {
        test_bfio_file2(bfio, scp, ofn);
    }
    else
    {
        test_bfio_file(bfio, ifn, ofn);
    }

    wtk_bfio_delete(bfio);
end:
    if (arg)
    {
        wtk_arg_delete(arg);
    }
    if (bin_fn)
    {
        wtk_bfio_cfg_delete_bin(cfg);
    }
    if (cfg_fn)
    {
        wtk_bfio_cfg_delete(cfg);
    }
    return 0;
}
