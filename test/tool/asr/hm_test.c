#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/core/cfg/wtk_opt.h"
#include "public/inc/qvoice_egram.h"

int main(int argc,char **argv)
{
	wtk_arg_t *arg;
	char *cfg_fn;
	char *i_fn;
    char *data;
    int len;

	arg=wtk_arg_new(argc,argv);
	if(!arg){goto end;}
	wtk_arg_get_str_s(arg,"c",&(cfg_fn));
	wtk_arg_get_str_s(arg,"i",&(i_fn));

	if(!cfg_fn || !i_fn)
	{
		goto end;
	}

	void *inst = qvoice_egram_new(cfg_fn);
    data=file_read_buf(i_fn,&len);
	qvoice_egram_feed(inst,data,len);
	qvoice_egram_delete(inst);
end:
	if(arg)
	{
		wtk_arg_delete(arg);
	}
	return 0;
}

