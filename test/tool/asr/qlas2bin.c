#include "wtk/asr/vad/wtk_vad.h"
#include "wtk/core/rbin/wtk_flist.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/wtk_str.h"

void print_usage()
{
	printf("qlas2bin:\n");
	printf("\t-n nnet file\n");
	printf("\t-t transf file\n");
	printf("\t-o fix bin\n");
	printf("\t-c 0|1 use char\n");
	printf("\t-w float w weight\n");
}

int main(int argc,char **argv)
{
	wtk_qlas_cfg_t  cfg;
	wtk_arg_t *arg;
	char *nfn,*tfn,*ofn;
	int c=1;
	double w=127;

	nfn=tfn=ofn=NULL;
	arg=wtk_arg_new(argc,argv);
	if(!arg){goto end;}
	wtk_arg_get_str_s(arg,"n",&nfn);
	wtk_arg_get_str_s(arg,"t",&tfn);
	wtk_arg_get_str_s(arg,"o",&ofn);
	wtk_arg_get_int_s(arg,"c",&c);
	wtk_arg_get_number_s(arg,"w",&w);
	if(!nfn  || !tfn || !ofn)
	{
		print_usage();
		goto end;
	}
	wtk_qlas_cfg_init(&(cfg));
	cfg.net_fn=nfn;
	cfg.trans_fn=tfn;
	cfg.use_char=c;
	cfg.use_fix=1;
	cfg.max_w=w;
	wtk_qlas_cfg_update(&(cfg));
	wtk_qlas_cfg_write_fix_bin(&(cfg),ofn);
end:
	if(arg)
	{
		wtk_arg_delete(arg);
	}
	return 0;
}


