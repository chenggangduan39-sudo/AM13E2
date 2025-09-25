#include "qtk_key.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/inotify.h>
#include <linux/input.h>

int qtk_key_run_entry(qtk_key_t *key, wtk_thread_t *t);
qtk_key_t *qtk_key_new(qtk_key_cfg_t *cfg)
{
	qtk_key_t *key;
	int ret;

	key = (qtk_key_t *)wtk_calloc(1,sizeof(*key));
	key->cfg = cfg;
	wtk_thread_init(&key->thread, (thread_route_handler)qtk_key_run_entry, key);
	wtk_debug("%s\n", key->cfg->event_node.data);
	key->fd = open(key->cfg->event_node.data, O_RDWR);
	if(key->fd <0){
		wtk_debug("open[%.*s]failed.\n", key->cfg->event_node.len, key->cfg->event_node.data);
		ret = -1;goto end;
	}
	ret = 0;
end:
	if(ret != 0){
		qtk_key_delete(key);
		key = NULL;
	}
	return key;
}

void qtk_key_delete(qtk_key_t *key)
{
	wtk_thread_clean(&key->thread);
	if(key->fd > 0){
		close(key->fd);
	}
}

int qtk_key_start(qtk_key_t *key)
{
	key->run = 1;
	wtk_thread_start(&key->thread);	
	return 0;
}

int qtk_key_stop(qtk_key_t *key)
{
	key->run = 0;
	wtk_thread_join(&key->thread);
	return 0;
}

void qtk_key_set_notify(qtk_key_t *key, void *ths, qtk_key_notify_f notify)
{
	key->ths = ths;
	key->notify = notify;
}

void qtk_key_event_handle(qtk_key_t *key, struct input_event *event)
{
	if(event->type == EV_KEY){
		wtk_debug("key code: %d  key value:%d\n", event->code, event->value);
		if(event->value == 1){//press
			switch(event->code){
				case POWR_KEY:
					break;
				case VOL_KEY:
					break;
				default:
					break;
			}	
		}else{
			switch(event->code){
				case POWR_KEY:
					break;
				case VOL_KEY:
					break;
				default:
					break;
			}	
		
		}
	}
}

int qtk_key_run_entry(qtk_key_t *key, wtk_thread_t *t)
{
	fd_set read_fds;	
	struct timeval timeout;
	int ret;
	struct input_event event;
	int size;
	
	size = (int)sizeof(event);
	while(key->run){
		FD_ZERO(&read_fds);
		FD_SET(key->fd, &read_fds);
		timeout.tv_sec = 0;
		timeout.tv_usec = 500000;
		ret = select(key->fd+1, &read_fds, NULL, NULL, &timeout);
		if(ret > 0){
			if(FD_ISSET(key->fd, &read_fds)){
				if(read(key->fd, &event, size) == size){
					qtk_key_event_handle(key, &event);	
				}
			}
		}
	}
	return 0;
}
