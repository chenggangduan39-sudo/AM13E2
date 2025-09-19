#include <stdio.h>
#include <string.h>
#ifndef _WIN32
// #include <sys/time.h>
#include <unistd.h>
#endif
#include "flite.h"
// #include "flite_version.h"
#include "./wtk/tts/cosynthesis/front_end/lang/cmu_us_timix/voxdefs.h"
#include "ais_hts_engine.h"
#include <ctype.h>
#include "wtk/core/wtk_fkv.h"
//cst_voice *vcs;

/*add by hlwang for using function address to call the function with the help of zhangli*/
typedef cst_utterance* (*TF)(cst_utterance *utt, float flite_flx_fct);

char* gbNamePointer[AIS_CFG_ENTRY];

/*initialize synthesis engine*/
cst_voice * init_engine()
{
    cst_features *extra_feats=NULL;
    cst_voice *vcs;
    vcs=REGISTER_VOX(NULL);
    feat_copy_into(extra_feats,vcs->features);
    delete_features(extra_feats);
    return vcs;
}

void print_htslab2(hts_lab *hl, FILE *fp) {
    if(fp!=NULL)
    {
            for (; hl != NULL; hl = hl->next) 
                fprintf(fp,"%s\n", hl->lab);
    }
    else  
        for (; hl != NULL; hl = hl->next) 
            printf("%s\n", hl->lab);
    return;
}

cst_utterance *flite_synth_speech(cst_voice  *cvs, const char *text,  FILE * outfn)
{
    cst_utterance *u;
    //const cst_val *v;
    u = new_utterance();

    utt_set_input_text(u,text);
    utt_init(u, cvs);
    if (flite_text_analysis(u) == NULL)
    {
	delete_utterance(u);
	return NULL;
    }
    else
    {
        hts_lab *lab = NULL;
        lab = get_htslab(u);
        //print_htslab2(lab,outfn);
        free_htslab(lab);    
    }
    // delete_utterance(u);
    delete_utterance_local(u,cvs);
    return NULL;
}

int syn_speech_to_buff(cst_voice  *vcs, char *text, FILE *out)
{
    //cst_utterance *u=NULL;
    text=norm_text(text);
    if(out!=NULL)
    {
        fprintf(out,"%s\n",text);
    }else
    {
        printf("%s\n",text);
    }
    //u = 
    flite_synth_speech(vcs,text,out);
    // if (u == NULL)
    // {
    //     printf("AIS_ERROR_%03d: failed in synthesizing the input text",AIS_ERR_N3);
    //     return AIS_ERR_N3;
    // }
    /*delete_utterance(u);*/
    cst_free(text);

   return 0;
}

hts_lab *flite_synth_speech2(cst_voice  *cvs, const char *text,  FILE * outfn)
{
    hts_lab *lab = NULL;
    cst_utterance *u;
    //const cst_val *v;
    u = new_utterance();

    utt_set_input_text(u,text);
    utt_init(u, cvs);
    if (flite_text_analysis(u) == NULL)
    {
	delete_utterance(u);
	return NULL;
    }
    else
    { 
        lab = get_htslab(u);   
    }
    // delete_utterance(u);
    delete_utterance_local(u,cvs);
    return lab;
}

hts_lab * syn_speech_to_hts_lab(cst_voice  *vcs, char *text)
{
    hts_lab *lab = NULL;
    text=norm_text(text);
    lab = flite_synth_speech2(vcs,text,NULL);

    /*delete_utterance(u);*/
    cst_free(text);
   return lab;
}

/*text normalization*/
char * norm_text(char *intxt)
{
    char *nrmtxt=NULL;
    char pang[]="pang.";
    char appnrm[]=".";
    int i,strl=0,flg=0, chrnum;
    strl=strlen(intxt);
    if(strl>0)
    {
        flg=0;
        for(i=strl-1; i>0; i--)
        {
            if(!isspace(intxt[i]))
            {
                if(ispunct(intxt[i]))
                {
                    if(isspace(intxt[i-1]))
                        flg=1;
                    else
                        flg=2;
                }
                break;
            }
        }
        chrnum=i+1;
        strl=chrnum+1;
        if(flg==0)
            strl=strl+1;

        nrmtxt=(char *)calloc(strl, sizeof(char));
        strncpy(nrmtxt, intxt, chrnum);
        if(flg==0)
            strcat(nrmtxt, appnrm);
        else  if(flg==1)
        {
           nrmtxt[chrnum-2]=intxt[chrnum-1];
           nrmtxt[chrnum-1]=0x00;
        }
 
    } else
    {
        printf("WARNING: input text is empty! sythesize \" pang \"!\n");
        strl=strlen(pang);
        nrmtxt=(char *)calloc(strl+1, sizeof(char));
        strcpy(nrmtxt, pang);
    }

    return nrmtxt;
}

int release_engine(cst_voice  *vcs)
{
    int i=0;
    UNREGISTER_VOX(vcs);
    for(i=0;i<sizeof(gbNamePointer)/sizeof(char*);i++)
    {
        if(gbNamePointer[i])
        {
            free(gbNamePointer[i]);
            gbNamePointer[i]=NULL;
        }
    }
    return 0;
}

void set_user_lexicon(cst_voice  *vcs,char *path)
{
	// user_lexicon **user_lexicon_data = NULL;
    wtk_fkv_t *kv = NULL;
	cst_val *v = NULL;
    if(path){
    	// user_lexicon_data = load_user_lexicon(path);
        kv = wtk_fkv_new(path,17003);
    	v = val_new_typed(41,kv);
    	feat_set(vcs->features, "user_lexicon_data",v);
    }
}

void free_user_lexicon(cst_voice *vcs)
{
	// user_lexicon **user_lexicon_data = NULL;
    wtk_fkv_t *kv = NULL;
	cst_val *v;
	if(feat_present(vcs->features, "user_lexicon_data")){
		v = (cst_val*)feat_val(vcs->features, "user_lexicon_data");
		kv = val_void(v);
		// cst_userLex_free(user_lexicon_data);
        wtk_fkv_delete(kv);
	}
}
