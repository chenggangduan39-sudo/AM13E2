#include "sdk/qtk_mod_rp.h"
#include "wtk/core/wtk_arg.h"
#include "wtk/core/cfg/wtk_main_cfg.h"

// #define DEBUG

int qformssl_run(int argc, char **argv)
{
	wtk_arg_t *arg = NULL;
	wtk_main_cfg_t *main_cfg = NULL;
	qtk_mod_rp_cfg_t *cfg = NULL;
	qtk_mod_rp_t *mod = NULL;
	char *cfg_fn=NULL;
	int ret;

	arg = wtk_arg_new(argc, argv);
	ret = wtk_arg_get_str_s(arg, "c", &cfg_fn);
	if(ret != 0){
		wtk_debug("cfg not set\n");
		goto end;
	}

	main_cfg = wtk_main_cfg_new_type(qtk_mod_rp_cfg, cfg_fn);
	if(!main_cfg){
		wtk_debug("main_cfg new failed.\n");
		goto end;
	}

	cfg = (qtk_mod_rp_cfg_t *)main_cfg->cfg;
	mod = qtk_mod_rp_new(cfg);
	if(!mod){
		wtk_debug("mod new failed.\n")
		goto end;
	}

	qtk_mod_rp_start(mod);

#ifndef DEBUG
	while(1)
	{
		sleep(3600);
		system("echo 2 /proc/sys/vm/drop_caches");
	}
#else
	getchar();
#endif

	qtk_mod_rp_stop(mod);
end:
	if(arg){
		wtk_arg_delete(arg);
	}
	if(mod){
		qtk_mod_rp_delete(mod);
	}
	if(main_cfg){
		wtk_main_cfg_delete(main_cfg);
	}

	return 0;
}

pthread_t tp;
pthread_attr_t tpa;

static void* test_static_route(void* d)
{
	while(1)
	{
		sleep(1);
	}
	return NULL;
}

int test_on_qsound(void *data,wtk_thread_t *t)
{
	while(1)
	{
		sleep(1);
	}
	return 0;
}

void test_qsound()
{
	// wtk_thread_t thread;

	// wtk_thread_init(&thread, (thread_route_handler)test_on_qsound, NULL);

	// wtk_thread_start(&thread);
	pthread_attr_init(&tpa);
	pthread_attr_setstacksize(&tpa, 1024*1024);
	pthread_create(&tp, &tpa, test_static_route, NULL);

	while(1)
	{
		sleep(1000);
	}
}

int main(int argc, char **argv)
{

	// test_qsound();
	qformssl_run(argc, argv);
	return 0;
}
