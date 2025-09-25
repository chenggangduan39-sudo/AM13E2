#include "sdk/qtk_mod_consist.h"
#include "wtk/core/wtk_arg.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/wtk_riff.h"


int test_get_command_result(char *command)
{
	char result[1024];
	char *ddff="pgrep Qsound_302";
	ddff = command;
	FILE *fp=popen(ddff, "r");
	if(fp < 0)
	{
		wtk_debug("read df fiald.\n");
	}

	while(fgets(result, sizeof(result), fp) != NULL)
	{
		if('\n' == result[strlen(result)-1])
		{
			result[strlen(result)-1] = '\0';
		}
	}

	fclose(fp);
	return wtk_str_atoi2(result, strlen(result), NULL);
}

int consist_run(int argc, char **argv)
{
	wtk_arg_t *arg = NULL;
	wtk_main_cfg_t *main_cfg = NULL;
	qtk_mod_consist_cfg_t *cfg = NULL;
	qtk_mod_consist_t *mod = NULL;
	char *cfg_fn=NULL;
	int ret=-1;
	float tm=8;
	int mode=0;
	int micgain=-1;
	int backgain=-1;
	char *fn="/usr/bin/qdreamer/consist/res/test.pcm";

	while((0 != access("/mnt/UDISK/consist", F_OK)) && ret < 0)
	{
		ret = system("mkdir /mnt/UDISK/consist");
	}

	ret=-1;
	while((0 != access("/mnt/UDISK/consist/result", F_OK)) && ret<0)
	{
		ret = system("mkdir /mnt/UDISK/consist/result");
	}
	if(access("/mnt/UDISK/consist/result", F_OK) == 0)
	{
		wtk_debug("================>>>result creadte ok\n");
		system("rm /mnt/UDISK/consist/result/*");
	}
	
	arg = wtk_arg_new(argc, argv);
	ret = wtk_arg_get_float_s(arg, "t", &tm);
	if(ret != 0){
		wtk_debug("time not set\n");
	}
	ret = wtk_arg_get_int_s(arg, "m", &mode);
	if(ret != 0){
		wtk_debug("mode not set\n");
	}
	ret = wtk_arg_get_int_s(arg, "mg", &micgain);
	if(ret != 0){
		wtk_debug("micgain not set\n");
	}
	ret = wtk_arg_get_int_s(arg, "bg", &backgain);
	if(ret != 0){
		wtk_debug("backgain not set\n");
	}
	ret = wtk_arg_get_str_s(arg, "fn", &fn);
	if(ret != 0){
		fn="/usr/bin/qdreamer/consist/res/test.pcm";
	}

	printf("time=%f mode=%d micgain=%d backgain=%d\n",tm,mode,micgain,backgain);
	int pid;
	int tid;
	char buf[30]={0};
	pid=test_get_command_result("pgrep Qsound_303");
	if(pid > 0)
	{
		if(access("/mnt/UDISK/qdreamer/pid.log",F_OK))
		{
			pid=test_get_command_result("pgrep Qsound_303 | awk '{if(NR == 1){print $0}}'");
			snprintf(buf, 30, "cat /proc/%d/stat | awk '{print $4}'",pid);
			tid=test_get_command_result(buf);
			if(tid != 1)
			{
				pid=tid;
			}
		}else{
			pid=test_get_command_result("cat /mnt/UDISK/qdreamer/pid.log");
		}
		snprintf(buf, 30, "kill -9 %d",pid);
		ret = system(buf);
		wtk_debug("system %d\n",ret);
	}

	cfg_fn = "/usr/bin/qdreamer/consist/res/cfg";

	main_cfg = wtk_main_cfg_new_type(qtk_mod_consist_cfg, cfg_fn);
	if(!main_cfg){
		wtk_debug("main_cfg new failed.\n");
		goto end;
	}
	cfg = (qtk_mod_consist_cfg_t *)main_cfg->cfg;
	mod = qtk_mod_consist_new(cfg, tm, mode, micgain, backgain);
	if(!mod){
		wtk_debug("mod_consist new failed.\n")
		goto end;
	}
	qtk_mod_consist_start(mod, fn);

	qtk_mod_consist_stop(mod);

	ret = system("/usr/bin/qdreamer/qsound/restart.sh");
	wtk_debug("system %d\n",ret);
end:
	if(arg){
		wtk_arg_delete(arg);
	}
	if(mod){
		qtk_mod_consist_delete(mod);
	}
	if(main_cfg){
		wtk_main_cfg_delete(main_cfg);
	}

	return 0;
}

int main(int argc, char **argv)
{
	consist_run(argc, argv);
	return 0;
}
