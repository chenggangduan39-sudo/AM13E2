#include "wtk_qvad.h" 

wtk_qvad_t* wtk_qvad_new(wtk_qvad_cfg_t *cfg)
{
	wtk_qvad_t *v;

	v=(wtk_qvad_t*)wtk_malloc(sizeof(wtk_qvad_t));
	v->cfg=cfg;
	wb_vad_init(&(v->inst));
	return v;
}

void wtk_qvad_delete(wtk_qvad_t *v)
{
	if(v->inst)
	{
		wb_vad_exit(&(v->inst));
	}
	wtk_free(v);
}

void wtk_qvad_reset(wtk_qvad_t *v)
{
	wb_vad_reset(v->inst);
}


void wtk_qvad_feed(wtk_qvad_t *v,short *data,int len)
{
	float *t;
	int i;
	static int ki=0;

	++ki;
	t=(float*)wtk_malloc(sizeof(float)*len);
	for(i=0;i<len;++i)
	{
		t[i]=data[i]/32768.0;
	}
	i=wb_vad(v->inst,t);
	wtk_debug("v[%d]=%d\n",ki,i);
}


