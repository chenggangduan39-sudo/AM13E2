#include "test-hdgd/qtk_mod_compress.h"
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

static void qtk_daemon_fixbeam_save_pidfile()
{
#ifdef DEBUG_FILE
	pid_save_file(master_pid,"./pid.log");
#else
	pid_save_file(master_pid,"/tmp/pid.log");
#endif
}

int qformssl_run(char *cfg_fn, char *infn, int channel)
{
	wtk_main_cfg_t *main_cfg = NULL;
	qtk_mod_compress_cfg_t *cfg = NULL;
	qtk_mod_compress_t *mod = NULL;

	int starttime=(int)time_get_ms();
	int overtime=3600*1000*2;

	main_cfg = wtk_main_cfg_new_type(qtk_mod_compress_cfg, cfg_fn);
	if(!main_cfg){
		wtk_debug("main_cfg new failed.\n");
		goto end;
	}

	cfg = (qtk_mod_compress_cfg_t *)main_cfg->cfg;
	mod = qtk_mod_compress_new(cfg);
	if(!mod){
		wtk_debug("mod new failed.\n")
		goto end;
	}

	qtk_daemon_fixbeam_save_pidfile();

	qtk_mod_compress_start(mod);

#ifndef OFFLINE_TEST
#ifndef DEBUG
	wtk_debug("==========>>>>>>>starttime=%f\n",time_get_ms() - starttime);
	sleep(2);
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
		// sleep(mod->cfg->sleep_time);
		// system("echo 2 /proc/sys/vm/drop_caches");
		printf("Press any key to start recording.\n");
		getchar();
		printf("start recording.\n");
		qtk_mod_compress_record_start(mod);
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
		qtk_mod_compress_feed(mod, data+pos, flen);
		// wtk_msleep(20);
		usleep(32* 1000);
		pos += flen;
	}
	free(data);
#endif
	qtk_mod_compress_stop(mod);
end:

	if(mod){
		qtk_mod_compress_delete(mod);
	}
	if(main_cfg){
		wtk_main_cfg_delete(main_cfg);
	}

	return 0;
}

int qtk_daemon_fixbeam_run_daemon(char *cfg_fn, char *infn, int channel)
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
            qformssl_run(cfg_fn, infn, channel);
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

	pid_setup_signal();

	if(run_daemon)
	{
		qtk_daemon_fixbeam_run_daemon(cfg_fn, infn, channel);
	}else
	{
		qformssl_run(cfg_fn, infn, channel);
	}

end:
	if(arg){
		wtk_arg_delete(arg);
	}
	return 0;
}
