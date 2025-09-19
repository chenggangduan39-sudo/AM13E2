#include "wtk/os/wtk_thread.h"
#include "wtk/os/wtk_log.h"
#include "qtk/rcd/qtk_recorder.h"
#include "wtk/core/qtk_lockmsg.h"
#include "wtk/asr/img/qtk_img2.h"
#include "wtk/core/wtk_wavfile.h"

typedef struct qtk_module_cfg qtk_module_cfg_t;
struct qtk_module_cfg{
	qtk_recorder_cfg_t recorder;
	wtk_string_t img2_cfg_fn;
	unsigned int log_wav:1;
};

typedef struct qtk_module qtk_module_t;
struct qtk_module{
	qtk_module_cfg_t *cfg;
	qtk_recorder_t *recorder;
	wtk_log_t *log;
	qtk_lockmsg_t *msg;
	wtk_main_cfg_t *main_cfg;
	qtk_img2_cfg_t *img2_cfg;
	qtk_img2_rec_t *img2;
	wtk_thread_t recorder_thread;
	wtk_thread_t img_thread;
	wtk_blockqueue_t queue;
	wtk_wavfile_t *mic;
    unsigned int run:1;
};

void qtk_module_on_img2(void *instance, int id, float prob, int start, int end);
int qtk_module_recorder_run(qtk_module_t *module, wtk_thread_t *thread);
int qtk_module_img_run(qtk_module_t *module, wtk_thread_t *thread);

int qtk_module_cfg_init(qtk_module_cfg_t *cfg)
{
	qtk_recorder_cfg_init(&(cfg->recorder));
	wtk_string_set(&cfg->img2_cfg_fn, 0, 0);
	cfg->log_wav = 0;
	return 0;
}
int qtk_module_cfg_clean(qtk_module_cfg_t *cfg)
{
	qtk_recorder_cfg_clean(&(cfg->recorder));
	return 0;
}

int qtk_module_cfg_update_local(qtk_module_cfg_t *cfg, wtk_local_cfg_t *main)
{
	wtk_local_cfg_t *lc;
	wtk_string_t *v;
	lc = wtk_local_cfg_find_lc_s(main, "recorder");
	if(lc){
		qtk_recorder_cfg_update_local(&(cfg->recorder), lc);
	}
	wtk_local_cfg_update_cfg_string_v(main, cfg, img2_cfg_fn, v);
	wtk_local_cfg_update_cfg_b(main, cfg, log_wav, v);
	return 0;
}

int qtk_module_cfg_update(qtk_module_cfg_t *cfg)
{
	qtk_recorder_cfg_update(&(cfg->recorder));

	return 0;
}

qtk_module_t *qtk_module_new(qtk_module_cfg_t *cfg, wtk_log_t *log) {
	qtk_module_t *module;
	int ret;

	module = (qtk_module_t *)wtk_malloc(sizeof(qtk_module_t));
	memset(module, 0, sizeof(qtk_module_t));
	module->log = log;
	module->cfg = cfg;
	module->recorder = qtk_recorder_new(&(module->cfg->recorder), module->log);
	if (!module->recorder) {
		wtk_debug(" recorder new failed.\n")
		wtk_log_warn0(module->log, "recorder new failed.");
		ret = -1;
		goto end;
	}
	if (module->cfg->img2_cfg_fn.len > 0) {
		module->main_cfg = wtk_main_cfg_new_type(qtk_img2_cfg, module->cfg->img2_cfg_fn.data);
		if (!module->main_cfg) {
			wtk_debug("img2 cfg new failed.\n");
			ret = -1; goto end;
		}
	} else {
		wtk_debug("img2 cfg not set\n");
		ret = -1; goto end;
	}
	module->img2_cfg = (qtk_img2_cfg_t *)module->main_cfg->cfg;
	module->img2 = qtk_img2_rec_new(module->img2_cfg);
	if (!module->img2) {
		wtk_debug("img2 new failed.\n");
		ret = -1; goto end;
	}
	qtk_img2_rec_set_notify(module->img2, (qtk_img2_rec_notify_f)qtk_module_on_img2, module);
	module->msg = qtk_lockmsg_new(2048);
	wtk_blockqueue_init(&module->queue);
    wtk_thread_init(&module->recorder_thread,(thread_route_handler)qtk_module_recorder_run, (void*)module);
	wtk_thread_set_name(&module->recorder_thread,"recorder");
    wtk_thread_init(&module->img_thread,(thread_route_handler)qtk_module_img_run, (void*)module);
	wtk_thread_set_name(&module->recorder_thread,"recorder");
	if (module->cfg->log_wav) {
		module->mic = wtk_wavfile_new(16000);
		module->mic->max_pend = 0;
		wtk_wavfile_set_channel(module->mic, qtk_recorder_get_channel(module->recorder));
		wtk_wavfile_open(module->mic, "mul.wav");
	}
	ret = 0;
end:
	if (ret != 0) {
		qtk_module_delete(module);
		module = NULL;
	}
	return module;
}

int qtk_module_delete(qtk_module_t *module) 
{
	if (module->recorder) { 
		qtk_recorder_delete(module->recorder);
	}
	if (module->mic) {
		wtk_wavfile_close(module->mic);
		wtk_wavfile_delete(module->mic);
	}
	if (module->msg) {
		qtk_lockmsg_delete(module->msg);
	}
	if (module->img2) {
		qtk_img2_rec_delete(module->img2);
	}
	if (module->main_cfg) {
		wtk_main_cfg_delete(module->main_cfg);
	}
	wtk_free(module);
	return 0;
}

int qtk_module_start(qtk_module_t *module)
{
    if (0 ==module->run) {
    	module->run = 1;
		wtk_thread_start(&module->img_thread);
        qtk_recorder_start(module->recorder);
        wtk_thread_start(&module->recorder_thread);
    }
	return 0;
}

int qtk_module_stop(qtk_module_t *module) 
{
    if (1 == module->run) {
    	module->run = 0;
        wtk_thread_join(&module->recorder_thread);
        qtk_recorder_stop(module->recorder);
		wtk_blockqueue_wake(&module->queue);
		wtk_thread_join(&module->img_thread);
    }
	return 0;
}


int qtk_module_recorder_run(qtk_module_t *module, wtk_thread_t *thread) 
{
	wtk_strbuf_t *buf = NULL;
	qtk_lockmsg_node_t *msg;

	while (module->run) {
		buf = qtk_recorder_read(module->recorder);
		if (buf->pos <= 0) {
			wtk_log_log0(module->log,"recorder error");
			continue;
		}
		msg = qtk_lockmsg_pop_node(module->msg);
		if (msg) {
			wtk_strbuf_push(msg->buf, buf->data, buf->pos);
			wtk_blockqueue_push(&module->queue, &msg->qn);
		}
		if (module->mic) {
			wtk_wavfile_write(module->mic, buf->data, buf->pos);
		}
	}
	return 0;
}

int qtk_module_img_run(qtk_module_t *module, wtk_thread_t *thread)
{
	qtk_lockmsg_node_t *msg;
	wtk_queue_node_t *qn;

	while (module->run) {
		qn = wtk_blockqueue_pop(&module->queue, -1, NULL);
		if (!qn) {
			break;
		}
		msg = data_offset2(qn, qtk_lockmsg_node_t, qn);
		if (module->img2) {
			// wtk_debug("-------------->pos = %d\n", msg->buf->pos);
			qtk_img2_rec_feed(module->img2, msg->buf->data, msg->buf->pos, 0);
		}

		qtk_lockmsg_push_node(module->msg, msg);
	}
	if (module->img2) {
		qtk_img2_rec_feed(module->img2, NULL, 0, 1);
		qtk_img2_rec_reset(module->img2);
	}
	return 0;
}

void qtk_module_on_img2(void *instance, int id, float prob, int start, int end)
{
    // qtk_module_t * *module = (qtk_module_t*)instance;
	// ++count;
    // a = 1;
    printf("%d %d ", start*80,end*80);
#if 0
    printf("note:%d id:%d prob:%f ", count, id, prob);
    printf("%f %f\n", start*0.08,end*0.08);
#endif
#if 1
    switch (id)
    {
        case 1:
            printf("打开相册");
            break;
        case 2:
            printf("稍后提醒");
            break;
        case 3:
            printf("打开日程");
            break;
        case 4:
            printf("查找手机");
            break;
        case 5:
            printf("增大亮度");
            break;
        case 6:
            printf("开始录音");
            break;
        default:
            break;
    }
    printf("\n");
#endif
    //qtk_img2_rec_feed(img2,NULL,0,1);
    //qtk_img2_rec_reset2(img2);
}

int main(int argc, char *argv[]) {
  	wtk_arg_t *arg = NULL;
	wtk_main_cfg_t *main_cfg;
	qtk_module_cfg_t *cfg;
	qtk_module_t *m = NULL;
	char *cfg_fn = NULL;

	arg = wtk_arg_new(argc, argv);
	wtk_arg_get_str_s(arg, "c", &cfg_fn);
	main_cfg = wtk_main_cfg_new_type(qtk_module_cfg, cfg_fn);
	if (!main_cfg) {
		wtk_debug("cfg new failed.\n");
		goto end;
	}
	cfg = (qtk_module_cfg_t *)main_cfg->cfg;
	m = qtk_module_new(cfg, NULL);
	if (!m) {
		wtk_debug("module new failed.\n");
		goto end;
	}
	qtk_module_start(m);
	getchar();
	qtk_module_stop(m);
end:
	if (m) {
		qtk_module_delete(m);
	}
	if (arg) {
		wtk_arg_delete(arg);
	}
	return 0;
  
}
