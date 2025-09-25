#include "qtk_timer.h"

static qtk_timer_node_t* qtk_timer_node_new(qtk_timer_t *t)
{
	qtk_timer_node_t *node;

	node = (qtk_timer_node_t*)wtk_malloc(sizeof(*node));
	return node;
}

static int qtk_timer_node_del(qtk_timer_node_t *node)
{
	wtk_free(node);
	return 0;
}

static qtk_timer_node_t* qtk_timer_pop_node(qtk_timer_t *t)
{
	qtk_timer_node_t *node;

	node = (qtk_timer_node_t*)wtk_lockhoard_pop(&t->node_hoard);
	node->notify_func = NULL;
	node->user_data = NULL;
	node->tm = 0.0;

	return node;
}

static void qtk_timer_push_node(qtk_timer_t *t,qtk_timer_node_t *node)
{
	wtk_lockhoard_push(&t->node_hoard,node);
}

static void qtk_timer_clean_q(qtk_timer_t *t)
{
	wtk_queue_node_t *qn;
	qtk_timer_node_t *node;

	wtk_lock_lock(&t->lock);
	while(1) {
		qn = wtk_queue_pop(&t->node_q);
		if(!qn) {
			break;
		}
		node = data_offset2(qn,qtk_timer_node_t,q_n);
		qtk_timer_push_node(t,node);
	}
	wtk_lock_unlock(&t->lock);
}

static int qtk_timer_run(qtk_timer_t *t,wtk_thread_t *thread)
{
	wtk_queue_node_t *qn;
	qtk_timer_node_t *node;
	wtk_sem_t loop_sem;
	double tm;

	wtk_sem_init(&loop_sem,0);

	while(t->run) {
		tm = time_get_ms();
		while(1) {
			wtk_lock_lock(&t->lock);
			qn = wtk_queue_pop(&t->node_q);
			wtk_lock_unlock(&t->lock);
			if(!qn) {
				break;
			}
			node = data_offset2(qn,qtk_timer_node_t,q_n);
			if(node->tm <= tm) {
				if(node->notify_func) {
					node->notify_func(node->user_data);
				}
				node->touch_count ++;
				if(node->touch_total<0){
					node->tm = time_get_ms() + node->timeout;
					wtk_lock_lock(&t->lock);
					wtk_queue_push_front(&t->node_q,qn);
					wtk_lock_unlock(&t->lock);
					break;
				}else{
					if(node->touch_total <= node->touch_count){
						qtk_timer_push_node(t,node);
					}else{
						node->tm = time_get_ms() + node->timeout;
						wtk_lock_lock(&t->lock);
						wtk_queue_push_front(&t->node_q,qn);
						wtk_lock_unlock(&t->lock);
						break;
					}
				}

			} else {
				wtk_lock_lock(&t->lock);
				wtk_queue_push_front(&t->node_q,qn);
				wtk_lock_unlock(&t->lock);
				break;
			}
		}
		

		wtk_sem_acquire(&loop_sem,10);
	}

	wtk_sem_clean(&loop_sem);
	return 0;
}

qtk_timer_t* qtk_timer_new(wtk_log_t *log)
{
	qtk_timer_t *t;

	t = (qtk_timer_t*)wtk_malloc(sizeof(*t));
	if(!t) {
		return NULL;
	}
	memset(t,0,sizeof(*t));

	t->log = log;
	wtk_queue_init(&t->node_q);
	wtk_lockhoard_init(&t->node_hoard,offsetof(qtk_timer_node_t,hoard_n),10,
			(wtk_new_handler_t)qtk_timer_node_new,
			(wtk_delete_handler_t)qtk_timer_node_del,
			t);
	wtk_lock_init(&t->lock);
	wtk_thread_init(&t->thread,(thread_route_handler)qtk_timer_run,t);
	wtk_thread_set_name(&t->thread,"dlgtiming");

	return t;
}

void qtk_timer_delete(qtk_timer_t *t)
{
	if(t->run) {
		qtk_timer_stop(t);
	}
	wtk_thread_clean(&t->thread);

	wtk_lock_clean(&t->lock);
	wtk_lockhoard_clean(&t->node_hoard);

	wtk_free(t);
}

int qtk_timer_start(qtk_timer_t *t)
{
	if(t->run) {
		return -1;
	}
	qtk_timer_clean_q(t);
	t->run = 1;
	return wtk_thread_start(&t->thread);
}

int qtk_timer_stop(qtk_timer_t *t)
{
	if(!t->run) {
		return -1;
	}
	t->run = 0;
	wtk_thread_join(&t->thread);
	return 0;
}

void qtk_timer_add(qtk_timer_t *t,int timeout,void *user_data,qtk_timer_notify_func notify_func)
{
	qtk_timer_node_t *node,*tmpnode;
	wtk_queue_node_t *qn;

	node = qtk_timer_pop_node(t);
	node->timeout = timeout;
	node->tm = time_get_ms() + node->timeout;
	//wtk_debug("add timing = %lf\n",node->tm);
	node->user_data = user_data;
	node->notify_func = notify_func;
	node->touch_total = 1;
	node->touch_count = 0;

	wtk_lock_lock(&t->lock);
	for(qn=t->node_q.pop;qn;qn=qn->next) {
		tmpnode = data_offset2(qn,qtk_timer_node_t,q_n);
		if(tmpnode->tm > node->tm) {
			wtk_queue_insert_before(&t->node_q,qn,&node->q_n);
			goto end;
		}
	}
	wtk_queue_push(&t->node_q,&node->q_n);

end:
	wtk_lock_unlock(&t->lock);
}
void qtk_timer_add2(qtk_timer_t *t,int timeout,int count,void *user_data,qtk_timer_notify_func notify_func)
{
	qtk_timer_node_t *node,*tmpnode;
	wtk_queue_node_t *qn;

	node = qtk_timer_pop_node(t);
	node->timeout = timeout;
	node->tm = time_get_ms() + node->timeout;
	node->user_data = user_data;
	node->notify_func = notify_func;
	node->touch_total = count;
	node->touch_count = 0;

	wtk_lock_lock(&t->lock);
	for(qn=t->node_q.pop;qn;qn=qn->next) {
		tmpnode = data_offset2(qn,qtk_timer_node_t,q_n);
		if(tmpnode->tm > node->tm) {
			wtk_queue_insert_before(&t->node_q,qn,&node->q_n);
			goto end;
		}
	}
	wtk_queue_push(&t->node_q,&node->q_n);

end:
	wtk_lock_unlock(&t->lock);
}


void qtk_timer_remove(qtk_timer_t *t,void *user_data,qtk_timer_notify_func notify_func) //删除所有user_data相关的定时器
{
	qtk_timer_node_t *node;
	wtk_queue_node_t *qn;

	wtk_lock_lock(&t->lock);
	for(qn=t->node_q.pop;qn;qn=qn->next) {
		node = data_offset2(qn,qtk_timer_node_t,q_n);
		if(node->user_data == user_data && node->notify_func == notify_func) {
			node->user_data = NULL;
			node->notify_func = NULL;
		}
	}
	wtk_lock_unlock(&t->lock);
}
