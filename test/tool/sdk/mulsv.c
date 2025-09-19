#include "sdk/mulsv/qtk_mulsv_api.h"
#include "sdk/mulsv/qtk_mulsv.h"
#include "wtk/core/wtk_arg.h"
#include "wtk/core/wtk_riff.h"
#include "wtk/core/rbin/wtk_flist.h"

static void print_usage(int argc,char **argv);

#define PCM_CACHE_TIME (20)     //20ms

int mulsv_test_wakeup1st(qtk_mulsv_api_t *m, char *wav_fn, int use_end,
                         int is_enroll) {
    char *src_pcm = NULL;
    wtk_riff_t *riff = NULL;
    int ret = 0, channel = 0, bytes = 0, len = 0;

    riff = wtk_riff_new();
    ret = wtk_riff_open(riff, wav_fn);
    if (ret!=0){goto end;}
    channel = riff->fmt.channels;
    bytes=16*sizeof(short)*channel*PCM_CACHE_TIME;
    src_pcm=wtk_malloc(bytes);   //src wav buf

    while(1)
    {
    	len=wtk_riff_read(riff,src_pcm,bytes);
        if(len<=0){break;}
        qtk_mulsv_api_feed(m,src_pcm,len,QTK_MULSV_API_DATA_TYPE_PCM,0);
        // usleep(PCM_CACHE_TIME*1000);
    }

    if (use_end) {
        qtk_mulsv_api_feed(m,0,0,QTK_MULSV_API_DATA_TYPE_PCM,1);
    }

end:
    if(riff)
    {
    	wtk_riff_delete(riff);
    }
    if(src_pcm)
    {
        wtk_free(src_pcm);
    }
    return ret;
}

int mulsv_test_enroll(qtk_mulsv_api_t *m,char *wav_fn,char *name)
{
    int ret = 0;

    if (name == NULL) {
        printf("need enroll name!\n");
        exit(0);
    }

    qtk_mulsv_api_start(m);
    qtk_mulsv_api_feed(m, name, strlen(name),
                       QTK_MULSV_API_DATA_TYPE_ENROLL_START, 0);
    mulsv_test_wakeup1st(m, wav_fn, NULL, 1);
    qtk_mulsv_api_feed(m, NULL, 0, QTK_MULSV_API_DATA_TYPE_PCM, 1);
    qtk_mulsv_api_feed(m, NULL, 0, QTK_MULSV_API_DATA_TYPE_ENROLL_END, 0);
    qtk_mulsv_api_reset(m);

    return ret;
}

int mulsv_test_enroll_scp(qtk_mulsv_api_t *m,char *scp_fn,char *name)
{
    int ret = 0;
    wtk_flist_t *f;
    wtk_fitem_t *item;
    wtk_queue_node_t *qn;

    if (name == NULL) {
        printf("need enroll name!\n");
        exit(0);
    }

    f = wtk_flist_new(scp_fn);

    qtk_mulsv_api_start(m);
    qtk_mulsv_api_feed(m, name, strlen(name),
                       QTK_MULSV_API_DATA_TYPE_ENROLL_START, 0);
    for(qn=f->queue.pop;qn;qn=qn->next)
    {
        item=data_offset(qn,wtk_fitem_t,q_n);
        mulsv_test_wakeup1st(m, item->str->data, 0, 1);
    }
    qtk_mulsv_api_feed(m,0,0,QTK_MULSV_API_DATA_TYPE_PCM,1);
    qtk_mulsv_api_feed(m,0,0,QTK_MULSV_API_DATA_TYPE_ENROLL_END,0);
    qtk_mulsv_api_reset(m);

    if(f)
    {
		wtk_flist_delete(f);
    }
    return ret;
}

int mulsv_test_enroll_list(qtk_mulsv_api_t *m, char *scp_fn) {
    int ret = 0;
    wtk_flist_t *f = NULL;
    wtk_queue_node_t *qn = NULL;
    wtk_fitem_t *item = NULL;
    wtk_array_t *detach_str = NULL;
    wtk_string_t *prev_name = NULL, **detached_strs = NULL;
    wtk_heap_t *heap = NULL;

    heap = wtk_heap_new(4096);
    f = wtk_flist_new(scp_fn);

    if (f) {
        for (qn = f->queue.pop; qn; qn = qn->next) {
            item = data_offset(qn, wtk_fitem_t, q_n);
            detach_str =
                wtk_str_to_array(heap, item->str->data, item->str->len, ' ');
            detached_strs = (wtk_string_t **)detach_str->slot;
            detached_strs[1]->data[detached_strs[1]->len] = '\0';
            if (!prev_name) {
                // printf("%.*s\n", detached_strs[0]->len,
                // detached_strs[0]->data);
                prev_name = detached_strs[0];
                qtk_mulsv_api_start(m);
                qtk_mulsv_api_feed(m, detached_strs[0]->data,
                                   detached_strs[0]->len,
                                   QTK_MULSV_API_DATA_TYPE_ENROLL_START, 0);
            } else if (!wtk_str_equal(prev_name->data, prev_name->len,
                                      detached_strs[0]->data,
                                      detached_strs[0]->len)) {
                // printf("%.*s\n", detached_strs[0]->len,
                // detached_strs[0]->data);
                qtk_mulsv_api_feed(m, 0, 0, QTK_MULSV_API_DATA_TYPE_PCM, 1);
                qtk_mulsv_api_feed(m, 0, 0, QTK_MULSV_API_DATA_TYPE_ENROLL_END,
                                   0);
                qtk_mulsv_api_reset(m);
                prev_name = detached_strs[0];
                qtk_mulsv_api_start(m);
                qtk_mulsv_api_feed(m, detached_strs[0]->data,
                                   detached_strs[0]->len,
                                   QTK_MULSV_API_DATA_TYPE_ENROLL_START, 0);
            }
            mulsv_test_wakeup1st(m, detached_strs[1]->data, 0, 1);
        }
        qtk_mulsv_api_feed(m, 0, 0, QTK_MULSV_API_DATA_TYPE_PCM, 1);
        qtk_mulsv_api_feed(m, 0, 0, QTK_MULSV_API_DATA_TYPE_ENROLL_END, 0);
        qtk_mulsv_api_reset(m);
    }

    wtk_heap_delete(heap);
    if (f) {
        wtk_flist_delete(f);
    }
    return ret;
}

int mulsv_test_eval(qtk_mulsv_api_t *m, char *wav_fn) {
    qtk_mulsv_api_start(m);
    mulsv_test_wakeup1st(m, wav_fn, 0, 0);
    qtk_mulsv_api_feed(m,0,0,QTK_MULSV_API_DATA_TYPE_PCM,1);
    qtk_mulsv_api_reset(m);

    return 0;
}

int mulsv_test_eval_scp(qtk_mulsv_api_t *m,char *scp_fn)
{
    wtk_flist_t *f;
    wtk_fitem_t *item;
    wtk_queue_node_t *qn;

    f=wtk_flist_new(scp_fn);

    qtk_mulsv_api_start(m);
    for (qn = f->queue.pop; qn; qn = qn->next) {
        item=data_offset(qn,wtk_fitem_t,q_n);
        mulsv_test_wakeup1st(m, item->str->data, 0, 0);
    }
    qtk_mulsv_api_feed(m,0,0,QTK_MULSV_API_DATA_TYPE_PCM,1);
    qtk_mulsv_api_reset(m);

    if(f)
    {
		wtk_flist_delete(f);
    }
    return 0;
}

int main(int argc,char **argv)
{
    wtk_arg_t *arg=NULL;
    qtk_mulsv_api_t *m = NULL;
    char *cfg_fn=0;
    char *wav_fn=0;
    char *name=0;
    char *scp=0;
    char *scp_enroll = 0;
    char *bin_fn = 0;
    char *prob = 0;
    char *notify_bias = 0;

    arg=wtk_arg_new(argc,argv);

    wtk_arg_get_str_s(arg, "b", &bin_fn);
    wtk_arg_get_str_s(arg, "c", &cfg_fn);
    wtk_arg_get_str_s(arg,"i",&wav_fn);
    wtk_arg_get_str_s(arg,"n",&name);
    wtk_arg_get_str_s(arg,"s",&scp);
    wtk_arg_get_str_s(arg, "se", &scp_enroll);
    wtk_arg_get_str_s(arg, "p", &prob);
    wtk_arg_get_str_s(arg, "nb", &notify_bias);

    if (!cfg_fn && !bin_fn) {
        print_usage(argc,argv);
        goto end;
    }

    if (cfg_fn) {
        m = qtk_mulsv_api_new(cfg_fn, 0);
    } else if (bin_fn) {
        m = qtk_mulsv_api_new(bin_fn, 1);
    }

    if (m && prob) {
        qtk_mulsv_api_feed(m, prob, strlen(prob),
                           QTK_MULSV_API_DATA_TYPE_VPRINT_THRESH, 0);
    }

    if (m && notify_bias) {
        qtk_mulsv_api_feed(m, notify_bias, strlen(notify_bias),
                           QTK_MULSV_API_DATA_TYPE_NOTIFY_BIAS, 0);
    }

    if (scp_enroll) {
        mulsv_test_enroll_list(m, scp_enroll);
    } else {
        if (name) {
            if (scp) {
                mulsv_test_enroll_scp(m, scp, name);
            } else {
                mulsv_test_enroll(m, wav_fn, name);
            }
        } else {
            if (scp) {
                mulsv_test_eval_scp(m, scp);
            } else {
                mulsv_test_eval(m, wav_fn);
            }
        }
    }

end:
    if (m) {
        if (cfg_fn) {
            qtk_mulsv_api_delete(m, 0);
        } else if (bin_fn) {
            qtk_mulsv_api_delete(m, 1);
        }
    }

    if (arg)
    {
	    wtk_arg_delete(arg);
    }
    return 0;
}


static void print_usage(int argc,char **argv)
{
    printf("Usage:\n"
           "\t: ./mulsv -c res/mulsv.cfg -i wav_fn -n name\n"
           "\t: ./mulsv -b mulsv_enroll.bin -i wav_fn -n name -p prob\n"
           "\t: ./mulsv -b mulsv_normal.bin -i wav_fn -p prob\n"
           "\t: ./mulsv -b mulsv_aec.bin -i wav_fn -p prob\n\n"
           "\t\t -c engine cfg file\n"
           "\t\t -b engine bin file\n"
           "\t\t -i input wave file\n"
           "\t\t -n enroll person name\n"
           "\t\t -s input wave scp file\n"
           "\t\t -se enroll scp file\n"
           "\t\t -nb 1st wake notify bias\n"
           "\t\t -p vprint prob -1.0~1.0\n");
}
