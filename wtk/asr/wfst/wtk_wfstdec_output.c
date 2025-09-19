#include "wtk/asr/wfst/wtk_wfstdec.h"
#include "wtk_wfstdec_output.h" 

wtk_wfstdec_output_t* wtk_wfstdec_output_new(wtk_wfstdec_output_cfg_t *cfg,struct wtk_wfstdec *dec)
{
	wtk_wfstdec_output_t *output;

	output=(wtk_wfstdec_output_t*)wtk_malloc(sizeof(wtk_wfstdec_output_t));
	output->cfg=cfg;
	output->dec=dec;
	output->result=wtk_strbuf_new(256,1);
	output->hint=wtk_strbuf_new(256,1);
	output->json=wtk_json_new();
	return  output;
}

void wtk_wfstdec_output_delete(wtk_wfstdec_output_t *output)
{
	wtk_json_delete(output->json);
	wtk_strbuf_delete(output->result);
	wtk_strbuf_delete(output->hint);
	wtk_free(output);
}

void wtk_wfstdec_output_reset(wtk_wfstdec_output_t *output)
{
	wtk_json_reset(output->json);
	wtk_strbuf_reset(output->result);
	wtk_strbuf_reset(output->hint);
}

void wtk_wfstdec_output_start(wtk_wfstdec_output_t *output)
{
}

void wtk_wfstdec_output_set_result(wtk_wfstdec_output_t *output,char *result,int len)
{
	wtk_strbuf_t *buf=output->result;

	//wtk_debug("[%.*s]\n",len,result);
	wtk_strbuf_reset(buf);
	wtk_strbuf_push(buf,result,len);
}

void wtk_wfstdec_output_get_str_result(wtk_wfstdec_output_t *output,wtk_string_t *v)
{
	wtk_string_set(v,output->result->data,output->result->pos);
}

void wtk_wfstdec_output_attach_ebnfdec(wtk_wfstdec_output_t *output,wtk_json_item_t *item)
{
	wtk_json_t *json=output->json;
	wtk_wfstr_t *rec=output->dec->ebnfdec2->dec->rec;
	float f;
	wtk_json_item_t *ti;
	wtk_strbuf_t *buf;


	buf=wtk_strbuf_new(256,1);
	ti=wtk_json_new_object(json);
	f=wtk_wfstr_get_conf(rec);
	wtk_json_obj_add_ref_number_s(json,ti,"conf",f);
	wtk_wfstr_finish2(rec,buf," ",1);
	wtk_json_obj_add_str2_s(json,ti,"rec",buf->data,buf->pos);
	wtk_json_obj_add_item2_s(json,item,"usrec",ti);
	wtk_strbuf_delete(buf);
}

void wtk_wfstdec_output_attach_usrec(wtk_wfstdec_output_t *output,wtk_json_item_t *item)
{
	wtk_json_t *json=output->json;
	wtk_wfstdec_t *d=output->dec;
	float f;
	wtk_json_item_t *ti;
	wtk_strbuf_t *buf;

	//wtk_debug("=====================>\n");
	buf=wtk_strbuf_new(256,1);
	ti=wtk_json_new_object(json);
	f=wtk_wfstr_get_conf(d->usrec->rec);
	wtk_json_obj_add_ref_number_s(json,ti,"conf",f);
	wtk_wfstr_finish2(d->usrec->rec,buf," ",1);
	wtk_json_obj_add_str2_s(json,ti,"rec",buf->data,buf->pos);
	wtk_json_obj_add_item2_s(json,item,"usrec",ti);
	wtk_strbuf_delete(buf);
}


void wtk_wfstdec_output_attach_time(wtk_wfstdec_output_t *output,wtk_json_item_t *xitem)
{
	wtk_json_t *json=output->json;
	wtk_wfstdec_t *d=output->dec;
	double f,t;
	wtk_json_item_t *item;

	item=wtk_json_new_object(json);
	t=1000.0/(2*wtk_fextra_cfg_get_sample_rate(&(d->cfg->extra)));
	f=d->wav_bytes*t;
	wtk_json_obj_add_ref_number_s(json,item,"wav",(int)f);
	f=d->rec_wav_bytes*t;
	wtk_json_obj_add_ref_number_s(json,item,"vad",(int)f);
	t=time_get_cpu();
	if(d->cfg->use_dnnc)
	{
		f=t-d->time_start;
	}else
	{
		f=t-d->time_start+d->time_rec;
	}
	wtk_json_obj_add_ref_number_s(json,item,"sys",(int)f);
	t=time_get_ms();
	f=t-d->time_start;
	wtk_json_obj_add_ref_number_s(json,item,"ses",(int)f);
	if(!d->rec->best_final_tokset.path)
	{
		wtk_json_obj_add_ref_number_s(json,item,"forceout",1);
	}
	f=t-d->time_stop;
	//wtk_debug("t=%f - %f\n",t,d->time_stop);
	wtk_json_obj_add_ref_number_s(json,item,"dly",(int)f);
	wtk_json_obj_add_ref_number_s(json,item,"dfm",d->delay_frame);
	wtk_json_obj_add_item2_s(json,xitem,"time",item);
}

void wtk_wfstdec_output_get_result(wtk_wfstdec_output_t *output,wtk_string_t *v)
{
	wtk_wfstdec_output_cfg_t *cfg=output->cfg;
	wtk_json_item_t *item,*obj;
	wtk_json_t *json=output->json;
	wtk_strbuf_t *buf=output->hint;
	float f;

	item=wtk_json_new_object(json);
	wtk_json_obj_add_ref_str_s(json,item,"version",&(cfg->version.ver));
	wtk_json_obj_add_ref_str_s(json,item,"res",&(cfg->res));
	wtk_json_obj_add_str2_s(json,item,"rec",output->result->data,output->result->pos);
	if(output->dec->xbnf)
	{
		wtk_xbnf_rec_process(output->dec->xbnf,output->result->data,output->result->pos);
		obj=wtk_xbnf_rec_get_json(output->dec->xbnf,json);
		if(item)
		{
			wtk_json_obj_add_item2_s(json,item,"sem",obj);
		}
	}
	f=wtk_wfstr_get_conf(output->dec->rec);
	wtk_json_obj_add_ref_number_s(json,item,"conf",f);
	wtk_wfstdec_output_attach_time(output,item);
	if(output->dec->env.use_ebnfdec && output->dec->ebnfdec2)
	{
		wtk_wfstdec_output_attach_ebnfdec(output,item);
	}else if(output->dec->usrec && output->dec->env.use_usrec)
	{
		wtk_wfstdec_output_attach_usrec(output,item);
	}
	wtk_json_item_print(item,buf);
	wtk_strbuf_push_c(buf,0);
	buf->pos-=1;
	wtk_string_set(v,buf->data,buf->pos);
}



