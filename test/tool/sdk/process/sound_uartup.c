#include "sdk/qtk_uart_client.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/wtk_arg.h"

#include "wtk/core/wtk_riff.h"
#include "wtk/os/wtk_fd.h"
#include <sys/prctl.h>
#include <signal.h>
#include <malloc.h>
#include "wtk/os/wtk_pid.h"

pid_t master_pid;
pid_t worker_pid;

int test_update(int argc, char**argv)
{
	wtk_arg_t *arg = NULL;
	wtk_main_cfg_t *main_cfg = NULL;
	qtk_uart_client_cfg_t *cfg = NULL;
	qtk_uart_client_t *uc = NULL;
	wtk_log_t *log = NULL;
	char *cfg_fn, *log_fn;
	int ret;

	arg = wtk_arg_new(argc, argv);
	ret = wtk_arg_get_str_s(arg, "l", &log_fn);
	if(0 == ret){
		log = wtk_log_new(log_fn);
	}
#ifdef USE_KTC3308
	main_cfg = wtk_main_cfg_new_type(qtk_uart_client_cfg, "/oem/uart.cfg");
#else
#if (defined  USE_AM32) || (defined  USE_802A) || (defined  USE_AM60)
	main_cfg = wtk_main_cfg_new_type(qtk_uart_client_cfg, "/oem/qdreamer/qsound/uart.cfg");
	// wtk_debug("-------------------------_>>>>>>>>>>>>>>>>>>>>>>>>main_cfg=%p\n",main_cfg);
#else
	ret = wtk_arg_get_str_s(arg, "c", &cfg_fn);
	if(ret != 0){
		wtk_debug("cfg not set\n");
		goto end;
	}
	main_cfg = wtk_main_cfg_new_type(qtk_uart_client_cfg, cfg_fn);
#endif
#endif
	if(!main_cfg){
		wtk_debug("main_cfg new failed.\n");
		goto end;
	}
	cfg = (qtk_uart_client_cfg_t *)main_cfg->cfg;
	uc = qtk_uart_client_new(cfg, log);
	if(!uc){
		wtk_debug("uc new failed.\n");
		goto end;
	}
	qtk_uart_client_start(uc);
	// while(1){
	// char frame_buf[12];  
	// int frame_length = 12;  
	// memcpy(frame_buf, "nihao!", 11);
	// qtk_uart_write2(uc->uart, (char *)frame_buf, frame_length);
	// sleep(3);
	// }
	while(1){
		sleep(3600);
	}

	qtk_uart_client_stop(uc);
end:
	if(arg){
		wtk_arg_delete(arg);
	}
	if(uc){
		qtk_uart_client_delete(uc);
	}
	if(main_cfg){
		wtk_main_cfg_delete(main_cfg);
	}
	if(log){
		wtk_log_delete(log);
	}
	return 0;
}


int qtk_daemon_update_run_daemon(int argc, char **argv)
{
	pid_t pid;
	int status,count;

	pid_daemonize();
	setpgid(0,0);
	master_pid = getpid();

	count = 0;
    while(1) {
        pid = fork();
        if(pid == 0) {
            prctl(PR_SET_PDEATHSIG,SIGUSR1);
            wtk_debug("run proc[pid=%d,count=%d].",(int)getpid(),count);
            test_update(argc, argv);
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


int main(int argc, char*argv[])
{
	// qtk_daemon_update_run_daemon(argc, argv);
	test_update(argc, argv);
	return 0;
}
