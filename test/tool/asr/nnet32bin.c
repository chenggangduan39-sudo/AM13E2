#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/core/cfg/wtk_source.h"
#include "wtk/asr/wfst/kwdec/qtk_wakeup_decoder_cfg.h"
#include "wtk/asr/wfst/kwdec/qtk_wakeup_decoder.h"
#include "wtk/asr/fextra/nnet3/qtk_nnet3_cfg.c"

int main(int argc,char **argv)
{
	wtk_main_cfg_t *main_cfg=0;
	wtk_arg_t *arg;
	qtk_wakeup_decoder_cfg_t *cfg=NULL;
	char *cfg_fn=0;

	
	arg=wtk_arg_new(argc,argv);

	wtk_arg_get_str_s(arg,"c",&cfg_fn);

        if(cfg_fn)
        {
		
		main_cfg=wtk_main_cfg_new_type(qtk_wakeup_decoder_cfg,cfg_fn);
		if(!main_cfg)
		{
			wtk_debug("configure load failed");
			goto end;
		}
		cfg=(qtk_wakeup_decoder_cfg_t*)main_cfg->cfg;
        }
	wtk_debug("write %s\n",cfg->extra.nnet3.bin_fn);
	qtk_nnet3_cfg_write_fix_bin(&cfg->extra.nnet3, cfg->extra.nnet3.bin_fn);
end:

	if(main_cfg)
	{
		wtk_main_cfg_delete(main_cfg);
	}else
	{
		if(cfg)
		{
			qtk_wakeup_decoder_cfg_delete_bin(cfg);
		}
	}
	wtk_arg_delete(arg);

	return 0;
}


