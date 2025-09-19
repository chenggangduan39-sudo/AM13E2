#include "wtk_checkmic.h"

int wtk_checkmic_standard_phose(float *mic1, float *mic2, float sv, int rate){
    float x,y,z;
    float pos;
    int phase;
    x = mic1[0] - mic2[0];
    y = mic1[1] - mic2[1];
    z = mic1[2] - mic2[2];
    pos = sqrt(x * x + y * y + z * z);
    phase = pos * rate / sv;
    return phase;
}

wtk_checkmic_t* wtk_checkmic_new(wtk_checkmic_cfg_t *cfg)
{
    wtk_checkmic_t *checkmic;
    int i,j,k;

    checkmic = (wtk_checkmic_t *)wtk_malloc(sizeof(wtk_checkmic_t));
    checkmic->cfg = cfg;
    checkmic->ths = NULL;
    checkmic->notify = NULL;
    checkmic->channel = cfg->channel;
    checkmic->combination = checkmic->channel * (checkmic->channel - 1) / 2;
    checkmic->mic = wtk_strbufs_new(checkmic->channel);

    checkmic->phase_diff = (wtk_checkmic_diff_t *)wtk_malloc(sizeof(wtk_checkmic_diff_t)*checkmic->combination);
    k=0;
    j=k+1;
    for(i=0;i<checkmic->combination;++i){
        checkmic->phase_diff[i].channel[0] = k;
        checkmic->phase_diff[i].channel[1] = j++;
        checkmic->phase_diff[i].phase = 0;
        checkmic->phase_diff[i].st_phase = 0;
        if(j==checkmic->channel){
            ++k;
            j = k+1;
        }
    }

    wtk_checkmic_reset(checkmic);
    return checkmic;
}

void wtk_checkmic_delete(wtk_checkmic_t *checkmic)
{
    wtk_strbufs_delete(checkmic->mic, checkmic->channel);
    wtk_free(checkmic->phase_diff);
    wtk_free(checkmic);
}

void wtk_checkmic_reset(wtk_checkmic_t *checkmic)
{
	wtk_strbufs_reset(checkmic->mic,checkmic->channel);
}

void wtk_checkmic_set_notify(wtk_checkmic_t *checkmic, void *ths, wtk_checkmic_notify_t notify)
{
    checkmic->ths = ths;
    checkmic->notify = notify;
}
void wtk_checkmic_print(wtk_checkmic_t *checkmic){
    int i;
    for(i=0;i<checkmic->combination;++i){
        printf("mic %d and mic %d phase=[%d] st_phase=[%d]\n", checkmic->phase_diff[i].channel[0], 
        checkmic->phase_diff[i].channel[1], checkmic->phase_diff[i].phase, checkmic->phase_diff[i].st_phase);
    }
}

void wtk_checkmic_feed(wtk_checkmic_t *checkmic, short **data, int len, int is_end)
{

#ifdef DEBUG_WAV
	static wtk_wavfile_t *mic_log=NULL;

	if(!mic_log)
	{
		mic_log=wtk_wavfile_new(16000);
		wtk_wavfile_set_channel(mic_log,checkmic->channel);
		wtk_wavfile_open2(mic_log,"checkmic");
	}
	if(len>0)
	{
		wtk_wavfile_write(mic_log,(char *)data,len*sizeof(short)*(checkmic->channel));
	}
	if(is_end && mic_log)
	{
		wtk_wavfile_close(mic_log);
		wtk_wavfile_delete(mic_log);
		mic_log=NULL;
	}
#endif


    wtk_checkmic_diff_t *diff=checkmic->phase_diff;
    wtk_strbuf_t **mic=checkmic->mic;
    float **mic_pos;
    int combination;
    int channel;
    int chn1, chn2;
    int i,j;

    combination = checkmic->combination;
    channel = checkmic->channel;
    mic_pos = checkmic->cfg->mic_pos;

    if(!is_end){
        for(j=0;j<channel;++j){
            wtk_strbuf_push(mic[j],(char *)data[j], len);
        }
    }else{
        for(i=0;i<combination;++i){
            chn1 = diff[i].channel[0];
            chn2 = diff[i].channel[1];
            diff[i].st_phase = wtk_checkmic_standard_phose(mic_pos[chn1], mic_pos[chn2], 340, checkmic->cfg->rate);
            diff[i].phase = wtk_rfft_xcorr(mic[chn1]->data, mic[chn1]->pos, mic[chn2]->data, mic[chn2]->pos);
        }
        if(checkmic->notify){
            checkmic->notify(checkmic, checkmic->phase_diff);
        }
    }
}