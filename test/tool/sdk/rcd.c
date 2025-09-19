#include "qtk/rcd/qtk_recorder.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/wtk_arg.h"
#include "wtk/os/wtk_thread.h"
#include "wtk/core/wtk_wavfile.h"

int run=0;
qtk_recorder_t *rcd=NULL;
void record_thread_entry(wtk_wavfile_t *mul,wtk_thread_t *t)
{
    wtk_strbuf_t *buf;
	while(run == 1){
		buf = qtk_recorder_read(rcd);
		if(buf->pos == 0){
			printf("buf->pos = %d\n", buf->pos);
			continue;
		}
		wtk_wavfile_write(mul, buf->data, buf->pos);
	}
}

int main(int argc,char **argv)
{
    wtk_main_cfg_t *main_cfg = NULL;
    qtk_recorder_cfg_t *cfg=NULL;
    wtk_arg_t *arg=NULL;
    wtk_thread_t t;
    wtk_wavfile_t *mul=NULL;
    int channel;
    char *cfg_fn;
    wtk_log_t *log=NULL;
    int ret;

    arg=wtk_arg_new(argc,argv);
    ret=wtk_arg_get_str_s(arg,"c",&cfg_fn);
    if(ret != 0){
		printf("usage: ./rcd -c cfg\n");
		goto end;
	}
    main_cfg=wtk_main_cfg_new_type(qtk_recorder_cfg,cfg_fn);
    if(!main_cfg)
    {
        printf("cfg new failed./n");
        goto end;
    }
    cfg=(qtk_recorder_cfg_t *)main_cfg->cfg;
    log=wtk_log_new("./qvoice");
    rcd=qtk_recorder_new(cfg,log);
    if(!rcd)
    {
        printf("rcd new failed.\n");
        goto end;
    }
    ret=qtk_recorder_start(rcd);
    if(ret != 0)
    {
        printf("rcd start failed.\n");
    }
    channel=cfg->channel-cfg->nskip;
    mul=wtk_wavfile_new(cfg->sample_rate);
    mul->max_pend=1;
    wtk_wavfile_set_channel(mul,channel);
    wtk_wavfile_open(mul,"./mul.wav");
    wtk_thread_init(&t,(thread_route_handler)record_thread_entry,mul);
    run=1;
    wtk_thread_start(&t);
    getchar();
    run=0;
    wtk_thread_join(&t);

end:
    if(mul)
    {
        wtk_wavfile_close(mul);
        wtk_wavfile_delete(mul);
    }
    if(rcd)
    {
        qtk_recorder_delete(rcd);
    }
    if(main_cfg)
    {
        wtk_main_cfg_delete(main_cfg);
    }
    if(arg)
    {
        wtk_arg_delete(arg);
    }
    if(log)
    {
        wtk_log_delete(log);
    }
    return 0;
}