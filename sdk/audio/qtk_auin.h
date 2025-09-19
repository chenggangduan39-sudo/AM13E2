#ifndef SDK_AUDIO_QTK_AUIN
#define SDK_AUDIO_QTK_AUIN

#include "wtk/os/wtk_thread.h"
#include "wtk/os/wtk_blockqueue.h"
#include "wtk/os/wtk_log.h"
#include "wtk/core/wtk_wavfile.h"

#include "qtk_auin_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_auin qtk_auin_t;
typedef void(*qtk_auin_process_f)(void *ths,char *data,int bytes);
typedef int(*qtk_auin_start_f)(void *ths);
typedef wtk_strbuf_t*(*qtk_auin_read_f)(void *ths);
typedef void(*qtk_auin_stop_f)(void *ths);
typedef int(*qtk_auin_clean_f)(void *ths);

typedef enum
{
	QTK_AUIN_RUN,
	QTK_AUIN_PAUSE,
}qtk_auin_state_t;

typedef enum
{
	QTK_AUIN_MSG_PAUSE,
	QTK_AUIN_MSG_RESUME,
}qtk_auin_msg_type_t;

typedef struct
{
	wtk_queue_node_t q_n;
	qtk_auin_msg_type_t type;
}qtk_auin_msg_t;

struct qtk_auin
{
	qtk_auin_cfg_t *cfg;
	wtk_log_t *log;
	void *process_ths;
	qtk_auin_process_f process;
	wtk_thread_t thread;
	qtk_auin_state_t state;
	wtk_blockqueue_t input_q;
	wtk_sem_t signal_sem;
	qtk_auin_start_f auin_start;
	qtk_auin_read_f auin_read;
	qtk_auin_stop_f auin_stop;
	qtk_auin_clean_f auin_clean;
	void *ths;
	unsigned run:1;
};

qtk_auin_t* qtk_auin_new(qtk_auin_cfg_t *cfg,wtk_log_t *log);
void qtk_auin_delete(qtk_auin_t *a);
void qtk_auin_set_callback(qtk_auin_t *a,void *ths,qtk_auin_start_f start,qtk_auin_read_f read,qtk_auin_stop_f stop,qtk_auin_clean_f clean);
void qtk_auin_set_process(qtk_auin_t *a,void *ths,qtk_auin_process_f process);
int qtk_auin_start(qtk_auin_t *a);
void qtk_auin_stop(qtk_auin_t *a);
void qtk_auin_restart_record(qtk_auin_t *a);
void qtk_auin_pause(qtk_auin_t *a);
void qtk_auin_resume(qtk_auin_t *a);

#ifdef __cplusplus
};
#endif
#endif
