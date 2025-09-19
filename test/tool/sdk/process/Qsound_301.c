#include "sdk/qtk_mod.h"
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

static void qtk_daemon_fixbeam_save_pidfile()
{
#ifdef DEBUG_FILE
	pid_save_file(master_pid,"./pid.log");
#else
#ifdef USE_R328
	pid_save_file(master_pid,"/mnt/UDISK/qdreamer/pid.log");
#endif
#if (defined USE_KTC3308) || (defined USE_AM32) || (defined USE_802A) || (defined USE_AM60)
	pid_save_file(master_pid,"/tmp/pid.log");
#endif
#endif
}

int file_read_rate(char *filename)
{
	char *data;
	int len;
	int rate;
	data=file_read_buf(filename,&len);

	rate = atoi(data);
	wtk_free(data);

	return rate;
}

void qtk_mod_test_on_usb(qtk_mod_t *mod, qtk_usb_uevent_state_t state, int sample_rate)
{
	switch (state)
	{
	case QTK_USB_STATE_PLAYER_START:
		printf("==================>>>>>>>>>>>>>>start\n");
		if(is_start==0 || sample_rate!=16000)
		{
			qtk_mod_start2(mod, sample_rate);
			is_start=1;
		}
		break;
	case QTK_USB_STATE_PLAYER_STOP:
		printf("===============>>>>>>>>>>>>>>>>>>>>stop\n");
		if(is_start)
		{
			qtk_mod_stop2(mod);
			is_start=0;
		}
		break;
	default:
		break;
	}
}

int qformssl_run(char *cfg_fn, char *infn, char *vpfn, int channel)
{
	wtk_main_cfg_t *main_cfg = NULL;
	qtk_mod_cfg_t *cfg = NULL;
	qtk_mod_t *mod = NULL;
// #ifdef USE_KTC3308
// 	qtk_usb_uevent_t *qu;
// #endif
	int starttime=(int)time_get_ms();
	int overtime=3600*1000*2;

	main_cfg = wtk_main_cfg_new_type(qtk_mod_cfg, cfg_fn);
	if(!main_cfg){
		wtk_debug("main_cfg new failed.\n");
		goto end;
	}
	
#ifdef USE_QTKCOUNT
	int qpos=strlen(cfg_fn);
	while(1)
	{
		if(cfg_fn[qpos] == '/')
		{
			break;
		}
		qpos--;
	}
	char tmp[128]={0};
	sprintf(tmp,"%.*s%s",qpos+1,cfg_fn,"aecsys.bin");
	if(qtk_test_count(5000, tmp) != 0) {
		printf("The dynamic library is out of number limit.\n");
		goto end;
	}
	system("sync");
#endif

	cfg = (qtk_mod_cfg_t *)main_cfg->cfg;
	if(vpfn)
	{
		wtk_string_set(&cfg->enroll_fn, vpfn, strlen(vpfn));
	}

	mod = qtk_mod_new(cfg);
	if(!mod){
		wtk_debug("mod new failed.\n")
		goto end;
	}

	qtk_daemon_fixbeam_save_pidfile();

// #ifdef USE_KTC3308
// 	qu=qtk_usb_uevent_new();
// 	qtk_usb_uevent_set_notify(qu, mod, (qtk_usb_uevent_notify_f)qtk_mod_test_on_usb);
// #endif

#if (defined  USE_3308)
	int rate;
#if (defined  USE_802A) || (defined  USE_BMC)
	if(access("/oem/qdreamer/qsound/filerate",F_OK) == 0)
	{
		rate=file_read_rate("/oem/qdreamer/qsound/filerate");
#else
	if(access("/oem/filerate",F_OK) == 0)
	{
		rate=file_read_rate("/oem/filerate");
#endif
		if(rate <= 0)
		{
			rate=cfg->usbaudio.sample_rate;
		}
	}else{
		rate=cfg->usbaudio.sample_rate;
	}
	wtk_debug("===================>>>>>>>>>>rate=%d\n",rate);
	qtk_mod_start2(mod, rate);
	is_start=1;
#endif

	qtk_mod_start(mod);

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
		qtk_mod_feed(mod, data+pos, flen);
		// wtk_msleep(20);
		usleep(32* 1000);
		pos += flen;
	}
	free(data);
#endif
	qtk_mod_stop(mod);
end:

	if(mod){
		qtk_mod_delete(mod);
	}
	if(main_cfg){
		wtk_main_cfg_delete(main_cfg);
	}

	return 0;
}

int qtk_daemon_fixbeam_run_daemon(char *cfg_fn, char *infn, char *vpfn, int channel)
{
	pid_t pid;
	int status,count;

	pid_daemonize();
	setpgid(0,0);
	master_pid = getpid();
	qtk_daemon_fixbeam_save_pidfile();
	count = 0;
    while(1) {
        pid = fork();
        if(pid == 0) {
            prctl(PR_SET_PDEATHSIG,SIGUSR1);
            wtk_debug("run proc[pid=%d,count=%d].",(int)getpid(),count);
            qformssl_run(cfg_fn, infn, vpfn, channel);
			wtk_debug("RUN END\n");
            exit(0);
        } else {
            worker_pid = pid;
            pid=waitpid(pid,&status,0);
            if(pid==-1)
            {
                perror(__FUNCTION__);
            }
            ++count;
        }
    }
    return 0;
}

int main(int argc, char **argv)
{
	wtk_arg_t *arg = NULL;
	char *cfg_fn=NULL;
	char *infn=NULL;
	char *vpfn=NULL;
	int channel=0;
	int ret;
	int run_daemon=1;

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

	ret = wtk_arg_get_int_s(arg, "d", &run_daemon);
	if(ret != 0)
	{
		run_daemon = 1;
	}

	ret = wtk_arg_get_str_s(arg, "vp", &vpfn);
	if(ret != 0)
	{
		vpfn = NULL;
	}

	pid_setup_signal();

	if(run_daemon)
	{
		qtk_daemon_fixbeam_run_daemon(cfg_fn, infn, vpfn, channel);
	}else
	{
		qformssl_run(cfg_fn, infn, vpfn, channel);
	}

end:
	if(arg){
		wtk_arg_delete(arg);
	}
	return 0;
}
