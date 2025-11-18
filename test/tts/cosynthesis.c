#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/core/cfg/wtk_opt.h"
#include "wtk/core/wtk_type.h"
#include "wtk/core/wtk_wavfile.h"
#include "cosynthesis/wtk_cosynthesis.h"
#include "qvoice_cosynthesis.h"
#include <limits.h>


typedef struct struct_ths{
	wtk_cosynthesis_t *cs;
	char *outwavfn;
}struct_ths_t;

static void print_usage(int argc, char **argv)
{
    printf("usage: %s -c cfg [-b cfgbin] -i ifn [-s str] -o outpath\n", argv[0]);
}

void cosynthesis_notify(void* ths,short *d, int l,short *uit,int len) {
     wtk_wavfile_t *wav;
     char *fp, *s;
     struct_ths_t* user_ths;

     user_ths = (struct_ths_t*)ths;

     s=wtk_cosynthesis_get_rawtxt(user_ths->cs);
     if (s)
     {
    	 wtk_debug("raw_txt: [%s]\n", s);
     }
     else
     {
    	 wtk_debug("raw_txt: []\n");
     }
     s=wtk_cosynthesis_get_proctxt(user_ths->cs);
     if (s)
     {
    	 wtk_debug("proc_txt: [%s]\n", wtk_cosynthesis_get_proctxt(user_ths->cs));
     }
     else
     {
    	 wtk_debug("proc_txt: []\n");
     }
     if (l > 0)
     {
 	     fp = user_ths->outwavfn;
 	     wav = wtk_wavfile_new(16000);
 	     if (wtk_wavfile_open(wav,fp)) {
 	         wtk_debug("Error Open %s\n", fp);
 	     }
 	     int i;
 	     printf("====>output unit_id: ");
 	     for(i=0;i<len;++i)
 	     {
 	         printf("%d ",uit[i]);
 	     }
 	     printf("\n");
 	     wtk_wavfile_write(wav, (const char*)d, l * 2);
 	     wtk_wavfile_close(wav);
 	     wtk_wavfile_delete(wav);
     }
}

int main(int argc, char *argv[]) {

    wtk_main_cfg_t *main_cfg=NULL;
	wtk_mbin_cfg_t *mbin_cfg=NULL;
	wtk_arg_t *arg;
	wtk_cosynthesis_t *cs=NULL;
	wtk_cosynthesis_cfg_t *cfg=NULL;
	char *ifn;
	char *cfn;
    char *bfn;
    char *ofn=NULL;
    char *str;
    double tm;
    int i;
    char tmp[512];
    wtk_flist_t *f;
    wtk_queue_node_t *n;
    wtk_fitem_t *item;
    int idx_start=0, idx_end=INT_MAX;
    wtk_cosynthesis_info_t* output=NULL;

//    idx_end=1000;

    arg=wtk_arg_new(argc,argv);
	cfn=NULL;
    wtk_arg_get_str_s(arg,"b",&bfn);
	wtk_arg_get_str_s(arg,"c",&cfn);
    wtk_arg_get_str_s(arg,"i",&ifn);
    wtk_arg_get_str_s(arg,"o",&ofn);
    wtk_arg_get_str_s(arg,"s",&str);

    if(cfn)
    {
		main_cfg=wtk_main_cfg_new_type(wtk_cosynthesis_cfg,cfn);
		cfg=(wtk_cosynthesis_cfg_t*)(main_cfg->cfg);
		if (!cfg) {
			printf("res read failed\n");
			goto end;
		}
        cs = wtk_cosynthesis_new(cfg);
    }else if(bfn)
    {
    	printf("don't support this res type\n");
        // mbin_cfg=wtk_mbin_cfg_new_type(wtk_cosynthesis_cfg,bfn,"./cfg");
        // cfg=(wtk_cosynthesis_cfg_t*)(mbin_cfg->cfg);
        // if (!cfg) goto end;
        // cs = wtk_cosynthesis_new(cfg);
    }else
    {
    	print_usage(argc, argv);
    	exit(0);
    }

    if (!cs){
    	printf("engine build failed\n");
    	goto end;
    }

    //user define struct ths and set value, pl:struct_ths_t
    struct_ths_t user_ths;
    user_ths.cs = cs;
    user_ths.outwavfn = NULL;

    char *sep=".";
//    sep=NULL;
    //set notify
    wtk_cosynthesis_set_notify(cs, &user_ths,(wtk_cosynthesis_notity_f)cosynthesis_notify);
    if(ifn)
    {
    	printf("=====>ifn= %s\n", ifn);
        f=wtk_flist_new(ifn);
        if(f)
        {
            for(i=0,n=f->queue.pop;n;n=n->next,++i)
            {
            	if (i<idx_start)
            		continue;
            	if (i > idx_end)
            		break;
                item=data_offset(n,wtk_fitem_t,q_n);
                printf("===>input text[%d]: [%s]\n",i, item->str->data);
                if(item->str->len > 500)
                {
                	printf("Warning: text len overcome MAX[500], [%.*s]\n", item->str->len, item->str->data);
                	continue;
                }
                if (ofn)
                	//sprintf(tmp,"%s/%.*s.wav",ofn,item->str->len,item->str->data);
                	sprintf(tmp,"%s/%d.wav",ofn,i);
                else
                	//sprintf(tmp,"%.*s.wav",item->str->len,item->str->data);
                	sprintf(tmp,"%d.wav",i);
                if (item->str->data[0]=='#')
                	continue;
                user_ths.outwavfn = tmp;
                wtk_cosynthesis_feedm(cs, item->str->data, strlen(item->str->data), sep);
                output = wtk_cosynthesis_getOutput(cs);
                if (output->log > 0)
                {
                	printf("===>Result: {'status': %d, 'cost': %f, 'log': \"%s\"}\n", output->state, output->loss, output->log);
                }
                else
                {
                	printf("===>Result: {'status': %d, 'cost': %f}\n", output->state, output->loss);
                }
                wtk_cosynthesis_reset(cs);
                user_ths.outwavfn = NULL;
                tm=time_get_ms();
                wtk_debug("running time: %f\n", time_get_ms()-tm);
            }
            wtk_flist_delete(f);
        }
    }else if(str){
    	printf("===> input text: [%s]\n",str);
        if (ofn)
        	sprintf(tmp,"%s/%s.wav",ofn,str);
        else
        	sprintf(tmp,"%s.wav",str);
        user_ths.outwavfn = tmp;
        wtk_cosynthesis_feedm(cs, str, strlen(str), sep);
        output = wtk_cosynthesis_getOutput(cs);
        if (output->log > 0)
        {
        	printf("===>Result: {'status': %d, 'cost': %f, 'log': \"%s\"}\n", output->state, output->loss, output->log);
        }
        else
        {
        	printf("===>Result: {'status': %d, 'cost': %f}\n", output->state, output->loss);
        }
        wtk_cosynthesis_reset(cs);
        user_ths.outwavfn = tmp;
        tm=time_get_ms();
        wtk_debug("running time: %f\n", time_get_ms()-tm);
    }

end:

	wtk_debug("release engine....\n");
	if(cs)
	{
		wtk_cosynthesis_delete(cs);
	}
	wtk_debug("release res....\n");
	if(main_cfg)
	{
		wtk_main_cfg_delete(main_cfg);
	}else if(mbin_cfg)
	{
		// wtk_mbin_cfg_delete(mbin_cfg);
	}
	if(arg)
	{
		wtk_arg_delete(arg);
	}
	return 0;
}
