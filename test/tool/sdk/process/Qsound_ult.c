#include "sdk/qtk_mod_ult.h"
#include "wtk/core/wtk_arg.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/wtk_riff.h"
#include "wtk/os/wtk_fd.h"
#include <sys/prctl.h>
#include <signal.h>
#include <malloc.h>
#include "wtk/os/wtk_pid.h"
#include "sdk/codec/qtk_usb_uevent.h"

// #define DEBUG
// #define TEST_QTKCOUNT

pid_t master_pid;
pid_t worker_pid;
int is_start=0;

int qtk_test_count(int count, char *binfn)
{
	wtk_rbin2_t *rbin;
    wtk_rbin2_item_t *item;
    int ret;
    char *cfg_fn="./cfg";
    wtk_rbin2_t *rbwrite;
    wtk_string_t name;
	int cc;
	// wtk_strbuf_t *inbuf;

	// wtk_debug("binfn=%s\n",binfn);

    rbin = wtk_rbin2_new();
	ret = wtk_rbin2_read(rbin, binfn);
	if(ret != 0){
        wtk_debug("error read\n");
	}
	item = wtk_rbin2_get2(rbin, cfg_fn, strlen(cfg_fn));
	if(!item){
		wtk_debug("%s not found\n", cfg_fn);
		ret = -1;
	}
    printf("count=[%.*s]\n",item->data->len,item->data->data);
	cc=wtk_str_atoi2(item->data->data,item->data->len,NULL);
	if(cc > count)
	{
		wtk_rbin2_delete(rbin);
		return -1;
	}

    wtk_rbin2_delete(rbin);

	cc++;
    wtk_string_set(&name,"cfg",sizeof("cfg")-1);
    
    rbwrite = wtk_rbin2_new();
    char *in;
    in = wtk_itoa(cc);

	wtk_heap_add_large(rbwrite->heap,in,strlen(in));
	wtk_rbin2_add2(rbwrite,&name,in,strlen(in));
    wtk_rbin2_write(rbwrite, binfn);

    wtk_rbin2_delete(rbwrite);
	return 0;
}

int qformssl_run(char *cfg_fn, char *infn, int channel)
{
	wtk_main_cfg_t *main_cfg = NULL;
	qtk_mod_ult_cfg_t *cfg = NULL;
	qtk_mod_ult_t *mod = NULL;
// #ifdef USE_KTC3308
// 	qtk_usb_uevent_t *qu;
// #endif
	int starttime=(int)time_get_ms();
	int overtime=3600*1000*2;

	main_cfg = wtk_main_cfg_new_type(qtk_mod_ult_cfg, cfg_fn);
	if(!main_cfg){
		wtk_debug("main_cfg new failed.\n");
		goto end;
	}
	
#ifdef TEST_QTKCOUNT
	int pos=strlen(cfg_fn);
	while(1)
	{
		if(cfg_fn[pos] == '/')
		{
			break;
		}
		pos--;
	}
	char tmp[128]={0};
	sprintf(tmp,"%.*s%s",pos+1,cfg_fn,"aecsys.bin");
	if(qtk_test_count(5000, tmp) != 0) {
		printf("The dynamic library is out of number limit.\n");
		goto end;
	}
	system("sync");
#endif

	cfg = (qtk_mod_ult_cfg_t *)main_cfg->cfg;
	mod = qtk_mod_ult_new(cfg);
	if(!mod){
		wtk_debug("mod new failed.\n")
		goto end;
	}

	qtk_mod_ult_start(mod);

#ifndef OFFLINE_TEST
#ifndef DEBUG
	// double tm=time_get_ms();
	// double tm1=0.0;
	wtk_debug("==========>>>>>>>starttime=%f\n",time_get_ms() - starttime);
	while(1)
	{
#ifdef USE_RUN_TIMELIMIT
		wtk_debug("===================>>>>>>%d\n",time_get_ms() - starttime);
		if((time_get_ms() - starttime) > overtime)
		{
			printf("Trial time timeout.\n");
			exit(0);
		}
#endif
		sleep(mod->cfg->sleep_time);
		system("echo 2 /proc/sys/vm/drop_caches");
		// wtk_debug("###############msg=%d/%d modmsg=%d/%d aec=%d denose=%d derverb=%d play=%d qform=%d\n",
		// mod->aecnspickup->aecnspickup->msg_lockhoard.use_length,
		// mod->aecnspickup->aecnspickup->msg_lockhoard.cur_free,
		// mod->msg->msg_hoard.use_length,
		// mod->msg->msg_hoard.cur_free,
		// mod->aecnspickup->aecnspickup->aec_q.length,mod->aecnspickup->aecnspickup->denoise_q.length,
		// mod->aecnspickup->aecnspickup->dereverb_q.length,mod->usbaudio_queue.length,mod->bfio_queue.length);
		// tm1 = time_get_ms() - tm;
		// if(tm1 > 30*1000)
		// {
		// 	wtk_debug("============================>>>>>>>>>>>>>>>>>>>>>>.restart\n");
		// 	system("reboot");
		// }
	}
#else
	getchar();
#endif
#else
	char *data;	
	int len = 0;
	data = file_read_buf(infn, &len);
	int step = 32* 32 * channel;
	int flen;
	int pos = 44;
	wtk_debug("step=%d\n",step);
	while(pos < len){
		flen = min(step, len - pos);
		qtk_mod_ult_feed(mod, data+pos, flen);
		// wtk_msleep(20);
		usleep(32* 1000);
		pos += flen;
	}
	free(data);
#endif
	qtk_mod_ult_stop(mod);
end:

	if(mod){
		qtk_mod_ult_delete(mod);
	}
	if(main_cfg){
		wtk_main_cfg_delete(main_cfg);
	}

	return 0;
}

int main(int argc, char **argv)
{
	wtk_arg_t *arg = NULL;
	char *cfg_fn=NULL;
	char *infn=NULL;
	int channel=0;
	int ret;

	arg = wtk_arg_new(argc, argv);
	ret = wtk_arg_get_str_s(arg, "c", &cfg_fn);
	if(ret != 0){
		wtk_debug("cfg not set\n");
		goto end;
	}

	ret = wtk_arg_get_str_s(arg, "i", &infn);
	if(ret != 0){
		infn=NULL;
	}

	ret = wtk_arg_get_int_s(arg, "channel", &channel);
	if(ret != 0)
	{
		channel = 8;
	}

	qformssl_run(cfg_fn, infn, channel);

end:
	if(arg){
		wtk_arg_delete(arg);
	}
	return 0;
}
