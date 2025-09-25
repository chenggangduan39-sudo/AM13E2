#include "qtk_wake_e2e.h"

qtk_wake_e2e_t* qtk_wake_e2e_new(qtk_wake_e2e_cfg_t *cfg,wtk_queue_t *output_queue)
{
	qtk_wake_e2e_t *wake = (qtk_wake_e2e_t*)wtk_malloc(sizeof(qtk_wake_e2e_t));

	return wake;
}

void qtk_wake_e2e_delete(qtk_wake_e2e_t *wake)
{

}

int qtk_wake_e2e_start(qtk_wake_e2e_t *wake)
{
	return 0;
}

int qtk_wake_e2e_reset(qtk_wake_e2e_t *wake)
{
	return 0;
}

int qtk_wake_e2e_feed(qtk_wake_e2e_t *wake,char *data,int bytes,int is_end)
{
	return 0;
}
