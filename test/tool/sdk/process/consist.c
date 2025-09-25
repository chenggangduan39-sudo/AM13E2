#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_opt.h"
#include "wtk/core/wtk_riff.h"
#include "wtk/core/wtk_wavfile.h"
#include "wtk/core/rbin/wtk_flist.h"
#include "wtk/bfio/consist/wtk_consist.h"

void print_usage()
{
	printf("consist usage:\n");
	printf("\t-c configure file\n");
	printf("\t-i input wav  file\n");
	printf("\n\n");
}
void test_on_consist(void *ths, wtk_consist_micerror_type_t type, int channel)
{
	switch (type)
	{
	case WTK_CONSIST_MICERR_NIL:
		wtk_debug("=========WTK_CONSIST_MICERR_NIL==================+>>>>%d\n",channel);
		break;
	case WTK_CONSIST_MICERR_ALIGN:
		wtk_debug("=========WTK_CONSIST_MICERR_ALIGN==================+>>>>%d\n",channel);
		break;
	case WTK_CONSIST_MICERR_MAX:
		wtk_debug("=========WTK_CONSIST_MICERR_MAX==================+>>>>%d\n",channel);
		break;
	case WTK_CONSIST_MICERR_CORR:
		wtk_debug("=========WTK_CONSIST_MICERR_CORR==================+>>>>%d\n",channel);
		break;
	case WTK_CONSIST_MICERR_ENERGY:
		wtk_debug("=========WTK_CONSIST_MICERR_ENERGY==================+>>>>%d\n",channel);
		break;
	case WTK_CONSIST_SPKERR_NIL:
		wtk_debug("=========WTK_CONSIST_SPKERR_NIL==================+>>>>%d\n",channel);
		break;
	case WTK_CONSIST_SPKERR_ALIGN:
		wtk_debug("=========WTK_CONSIST_SPKERR_ALIGN==================+>>>>%d\n",channel);
		break;
	case WTK_CONSIST_SPKERR_MAX:
		wtk_debug("=========WTK_CONSIST_SPKERR_MAX==================+>>>>%d\n",channel);
		break;
	case WTK_CONSIST_SPKERR_CORR:
		wtk_debug("=========WTK_CONSIST_SPKERR_CORR==================+>>>>%d\n",channel);
		break;
	case WTK_CONSIST_SPKERR_ENERGY:
		wtk_debug("=========WTK_CONSIST_SPKERR_ENERGY==================+>>>>%d\n",channel);
		break;
	default:
		break;
	}
}


void consist_test_file(wtk_consist_t *consist,char *ifn)
{
	char *data;
	int len;
	double tm,tall;
	float stm;

	wtk_debug("%s \n",ifn);
	data=file_read_buf(ifn,&len);
	wtk_debug("+=================>>>channe=%d\n",consist->channel);
	int i;
	for(i=0;i<1;++i)
	{
		tm = time_get_ms();
	#if 0
		char *ss,*ee;
		int step=(consist->channel+2)*32*2000;
		int nx;
		ss=data+44;
		ee=data+len-44;

		while(ss < ee)
		{
			nx=ee-ss;
			if(nx < step){break;}
			wtk_consist_feed3(consist,(short*)ss,step/(consist->channel+2)/sizeof(short),1);
			wtk_consist_reset(consist);
			ss+=step;
		}
	#else
		wtk_consist_feed2(consist,(short*)(data+44),(len-44)/(consist->channel+2)/sizeof(short),1);
		// wtk_consist_feed2(consist,(short*)(data),(len)/(consist->channel+2)/sizeof(short),1);
	#endif
		tall=time_get_ms() - tm;
		// stm = (len-44)/(consist->channel + 2)/32;
		stm = len/(consist->channel + 2)/32;

		printf("consist %d %f/%f=%f\n",consist->consist,tall, stm, tall/stm);
		wtk_consist_reset(consist);
	}
	wtk_free(data);
}

int main(int argc,char **argv)
{
	wtk_consist_t *consist=NULL;
	wtk_opt_t *opt=NULL;
	char *wav=NULL;

	opt=wtk_opt_new_type(argc,argv,wtk_consist_cfg);
	if(!opt){goto end;}
	wtk_arg_get_str_s(opt->arg,"i",&wav);

	if(!wav)
	{
		print_usage();
		goto end;
	}
	consist=wtk_consist_new((wtk_consist_cfg_t*)(opt->main_cfg->cfg));
	wtk_consist_set_notify(consist, NULL, (wtk_consist_notify_f)test_on_consist);

	if(wav)
	{
		int i;

		for(i=0;i<1;++i)
		{
			consist_test_file(consist,wav);
		}
	}
end:
	if(consist)
	{
		wtk_consist_delete(consist);
	}
	if(opt)
	{
		wtk_opt_delete(opt);
	}
	return  0;
}
