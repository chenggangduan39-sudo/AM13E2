#ifndef QTK_CORE_TIMER_QTK_TIMER
#define QTK_CORE_TIMER_QTK_TIMER

#include "wtk/os/wtk_thread.h"
#include "wtk/os/wtk_lockqueue.h"
#include "wtk/os/wtk_lockhoard.h"
#include "wtk/os/wtk_log.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_timer qtk_timer_t;

typedef void(*qtk_timer_notify_func)(void *user_data);

typedef struct {
	wtk_queue_node_t hoard_n;
	wtk_queue_node_t q_n;
	int timeout;
	double tm;
	void *user_data;
	qtk_timer_notify_func notify_func;
	int touch_total;//-1 每间隔timeout时间触发一次，大于0时为出发count次。
	int touch_count;// 触发次数统计
}qtk_timer_node_t;

struct qtk_timer {
	wtk_log_t *log;
	wtk_thread_t thread;
	wtk_queue_t node_q;
	wtk_lockhoard_t node_hoard;
	wtk_lock_t lock;
	unsigned run:1;
};

qtk_timer_t* qtk_timer_new(wtk_log_t *log);
void qtk_timer_delete(qtk_timer_t *t);

int qtk_timer_start(qtk_timer_t *t);
int qtk_timer_stop(qtk_timer_t *t);

void qtk_timer_add(qtk_timer_t *t,int timeout,void *user_data,qtk_timer_notify_func notify_func);
void qtk_timer_add2(qtk_timer_t *t,int timeout,int count,void *user_data,qtk_timer_notify_func notify_func);
void qtk_timer_remove(qtk_timer_t *t,void *user_data,qtk_timer_notify_func notify_func); //删除所有user_data相关的定时器


#ifdef __cplusplus
};
#endif
#endif
