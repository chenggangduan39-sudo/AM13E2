#include "qtk_api.h"

#include <unistd.h>

#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/types.h>


#define run_daemon 0
pid_t master_pid;
pid_t worker_pid;

double time_ms()
{
    //struct timeval_vad tv;
    struct timeval tv;
    double ret;
    int err;

    err = gettimeofday(&tv, 0);
    if(err == 0){
        ret = tv.tv_sec * 1000.0 + tv.tv_usec * 1.0/1000;
        //ret maybe is NAN
        if(ret != ret){
            printf("NAN(%.0f,sec=%.d,usec=%.d).\n", ret, (int)tv.tv_sec, (int)tv.tv_usec);
            ret = 0;
        }
    }else{
        perror(__FUNCTION__);
        ret = 0;
    }
    return ret;
}

void test_mwakeup_on_process(void *ths, qtk_var_t *var){
    switch (var->type){
    case QTK_SPEECH_START:
        printf("QTK_SPEECH_START\n");
        break;
    case QTK_SPEECH_END:
        printf("QTK_SPEECH_END\n");
        break;
    case QTK_ASR_TEXT:
        printf("QTK_ASR_TEXT: %.*s\n", var->v.str.len, var->v.str.data);
        break;
    case QTK_SPEECH_DATA_PCM://处理后的音频
	//printf("QTK_SPEECH_DATA_PCM len=%d\n",var->v.str.len);
        break;
    case QTK_AEC_DIRECTION://定位信息
        printf("QTK_AEC_DIRECTION theta=%d phi=%d nbest=%d nspecsum=%f\n",var->v.ii.theta,var->v.ii.phi,var->v.ii.nbest,var->v.ii.nspecsum);
        break;
    default:
        break;
    }
}

void test_module(qtk_session_t *session){
    qtk_module_t *m;
    int ret;
    char ch;

    m = qtk_module_new(session, "mqform", "./res/module.cfg");
    if(!m){
        printf("new module failed \n");
        exit(0);
    }
    qtk_module_set_notify(m, NULL, (qtk_engine_notify_f)test_mwakeup_on_process);

    ret = qtk_module_start(m);
    if(ret != 0){
        printf("module start failed\n");
        exit(1);
    }

    while(1){
    	sleep(3);
        ch = getchar();
        switch (ch){
        case 'q':
            goto end;
        }
    }

end:
    qtk_module_stop(m);
    qtk_module_delete(m);
}

static void test_module_on_errcode(qtk_session_t *session, int errcode, char *errstr){
    printf("==================>errcode : %d errstr: %s  \n", errcode, errstr);
}

void pid_daemonize()
{
	signal(SIGTTOU, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	if (0 != fork()) exit(0);
	if (-1 == setsid()) exit(0);
	signal(SIGHUP, SIG_IGN);
	if (0 != fork()) exit(0);
}

int main(int argc, char* argv[]){
    qtk_session_t *session;
    char *params;
	params = "appid=613679ec-9cf0-11eb-b526-00163e13c8a2;secretkey=f358afec-f82a-3fb2-be2d-ed096070b83e;"
						"cache_path=./qvoice;log_wav=0;use_timer=1;";
    session = qtk_session_init(params, QTK_WARN, NULL, (qtk_errcode_handler)test_module_on_errcode);
    if(!session){
        printf("session init failed.\n");
        exit(0);
    }
    if(run_daemon){
        pid_t pid;
        int status, count;

        pid_daemonize();
        setpgid(0, 0);
        master_pid = getpid();
        count = 1;
        while(1){
            pid = fork();
            if(pid == 0){
                prctl(PR_SET_PDEATHSIG, SIGUSR1);
                printf("run proc[pid = %d, count = %d]. \n", (int)getpid(), count);
                test_module(session);
                printf("RUN END \n");
                exit(0);
            }else{
                worker_pid = pid;
                pid = waitpid(pid, &status, 0);
                if(pid == -1){
                    perror(__FUNCTION__);
                }
                ++count;
                if(count > 4){
                    break;
                }
            }
        }
    }else{
        test_module(session);
    }

    qtk_session_exit(session);
    return 0;
}
