#include "wtk_vprint.h" 


wtk_vprint_t* wtk_vprint_new(wtk_vprint_cfg_t *cfg)
{
	wtk_vprint_t *p;

	p=(wtk_vprint_t*)wtk_malloc(sizeof(wtk_vprint_t));
	p->cfg=cfg;
	if(cfg->use_share_vparm)
	{
		p->parm=wtk_vparm_new(&(cfg->vparm));
		p->train=wtk_vtrain_new(&(cfg->train),&(cfg->vparm),1);
		p->detect=wtk_vdetect_new(&(cfg->detect),NULL);
	}else
	{
		p->parm=NULL;
		p->train=wtk_vtrain_new(&(cfg->train),&(cfg->vparm),0);
		p->detect=wtk_vdetect_new(&(cfg->detect),&(cfg->vparm));
	}
	p->buf=wtk_strbuf_new(256,1);
	p->json=wtk_json_new();
	p->last_usr=wtk_strbuf_new(256,1);
	p->ask=0;
	return p;
}

void wtk_vprint_delete(wtk_vprint_t *v)
{
	wtk_strbuf_delete(v->last_usr);
	if(v->parm)
	{
		wtk_vparm_delete(v->parm);
	}
	wtk_json_delete(v->json);
	wtk_strbuf_delete(v->buf);
	wtk_vdetect_delete(v->detect);
	wtk_vtrain_delete(v->train);
	wtk_free(v);
}

void wtk_vprint_train_start(wtk_vprint_t *v)
{
	wtk_vtrain_start(v->train);
}

void wtk_vprint_train_reset(wtk_vprint_t *v)
{
	wtk_vtrain_reset(v->train);
}

void wtk_vprint_train_feed(wtk_vprint_t *v,char *data,int bytes,int is_end)
{
	wtk_vtrain_feed(v->train,data,bytes,is_end);
}

void wtk_vprint_train_update_acc(wtk_vprint_t *v)
{
	wtk_vtrain_update_acc2(v->train);
}

void wtk_vprint_train_reset_acc(wtk_vprint_t *v)
{
	wtk_vtrain_reset_acc2(v->train);
}

void wtk_vprint_train_save(wtk_vprint_t *v,char *name,int len)
{
	wtk_strbuf_t *buf=v->buf;
	wtk_string_t fn;

	//wtk_debug("use_vad=%d\n",v->cfg->vparm.use_vad);
	//wtk_debug("===================================> train cnt=%d [%.*s]\n",v->train->cnt,len,name);
	wtk_vtrain_update_hmm(v->train);
	wtk_strbuf_reset(buf);
	//wtk_vtrain_write_hmm(v->train,"fix.hmm");
	//wtk_debug("write bin\n");
	if(v->cfg->detect.ubin)
	{
		wtk_strbuf_push(buf,name,len);
		wtk_string_set(&(fn),buf->data,buf->pos);
		wtk_vtrain_write_hmm_bin2(v->train,buf->data,buf->pos,v->cfg->detect.ubin);
	}else
	{
		wtk_strbuf_push(buf,v->detect->cfg->usr_dn.data,v->detect->cfg->usr_dn.len);
		wtk_strbuf_push_c(buf,'/');
		wtk_strbuf_push(buf,name,len);
		wtk_strbuf_push_s(buf,".bin");
		wtk_string_set(&(fn),buf->data,buf->pos);
		wtk_strbuf_push_c(buf,0);
		wtk_vtrain_write_hmm_bin(v->train,buf->data);
	}
	//wtk_debug("load bin start\n");
	//wtk_debug("len=%d\n",v->detect->cfg->usr_q.length);
	wtk_vdetect_cfg_load_usr_dn_fn(v->detect->cfg,&fn);
	//wtk_debug("len=%d\n",v->detect->cfg->usr_q.length);
	//wtk_debug("load bin end\n");
}

void wtk_vprint_train_del(wtk_vprint_t *v,char *name,int len)
{
	wtk_strbuf_t *buf=v->buf;
	wtk_string_t fn;

	wtk_strbuf_reset(buf);
	if(v->cfg->detect.ubin)
	{
		wtk_strbuf_push(buf,name,len);
		wtk_string_set(&(fn),buf->data,buf->pos);
	}else
	{
		wtk_strbuf_push(buf,v->detect->cfg->usr_dn.data,v->detect->cfg->usr_dn.len);
		wtk_strbuf_push_c(buf,'/');
		wtk_strbuf_push(buf,name,len);
		wtk_strbuf_push_s(buf,".bin");
		wtk_string_set(&(fn),buf->data,buf->pos);
		wtk_strbuf_push_c(buf,0);
	}

	//wtk_debug("len=%d\n",v->detect->cfg->usr_q.length);
	wtk_vdetect_cfg_del_usr_dn_fn(v->detect->cfg,&fn);
	//wtk_debug("len=%d\n",v->detect->cfg->usr_q.length);
}

void wtk_vprint_detect_start(wtk_vprint_t *v)
{
	if(v->detect->cfg->usr_q.length>0)
	{
		wtk_vdetect_start(v->detect);
	}
}

void wtk_vprint_detect_reset(wtk_vprint_t *v)
{
	wtk_json_reset(v->json);
	if(v->detect->cfg->usr_q.length>0)
	{
		wtk_vdetect_reset(v->detect);
	}
}

void wtk_vprint_detect_feed(wtk_vprint_t *v,char *data,int bytes,int is_end)
{
	if(v->detect->cfg->usr_q.length>0)
	{
		wtk_vdetect_feed(v->detect,data,bytes,is_end);
	}
}

void wtk_vprint_detect_print(wtk_vprint_t *v)
{
	wtk_vdetect_print(v->detect);
}

void wtk_vprint_get_usr(wtk_vprint_t *v,wtk_string_t *result)
{
	wtk_json_item_t *item=0,*arr=0;
	wtk_queue_node_t *qn;
	wtk_vdetect_usr_t *usr;
	wtk_vdetect_t *d=v->detect;
	int ret=-1;

	wtk_json_reset(v->json);
	wtk_strbuf_reset(v->buf);
	result->len=0;result->data=0;
	item=NULL;
	if(d->cfg->usr_q.length)
	{
		item=wtk_json_new_object(v->json);
		if(!item){goto end;}
		arr=wtk_json_new_array(v->json);
		if(!arr){goto end;}
		for(qn=d->cfg->usr_q.pop;qn;qn=qn->next)
		{
			usr=data_offset2(qn,wtk_vdetect_usr_t,q_n);
			if(usr->attr & WTK_UBIN_ATTR_INVALID){continue;}
			wtk_json_array_add_str(v->json,arr,usr->name.data,usr->name.len);
		}
		wtk_json_obj_add_item2_s(v->json,item,"usr",arr);
	}
    if(item)
    {
	    v->json->main=item;
	    wtk_json_item_print(item,v->buf);
	    result->data=v->buf->data;
	    result->len=v->buf->pos;
	    wtk_strbuf_push_c(v->buf,'\0');
    }
	ret=0;
end:
	if(ret==-1)
	{
		wtk_json_reset(v->json);
		wtk_strbuf_reset(v->buf);
		result->len=0;result->data=0;
	}
	return;
}

/**
 *	wav =>  train|detect
 *	if not last_usr:
 *		if detect_usr:
 *			update_acc2
 *		else:
 *			update_acc2 and ask who
 *	if last_usr != detect_usr:
 *		reset_acc2, update_acc2,clean last_usr ask who
 *	else:
 *		update_acc2,if training is enough update hmm with detect_usr
 *	return 1 system ask who
 *
 */
int wtk_vprint_touch(wtk_vprint_t *v)
{
	wtk_vdetect_t *d=v->detect;
	int ret=0;

	if(v->last_usr->pos==0)
	{
		if(d->usr)
		{
			///wtk_debug("update ...\n");
			wtk_vtrain_update_acc2(v->train);
			wtk_strbuf_push(v->last_usr,d->usr->name.data,d->usr->name.len);
		}else
		{
			//wtk_debug("update ...\n");
			wtk_vtrain_update_acc2(v->train); //system ask who is this
			ret=1;
		}
	}else
	{
		if(d->usr && wtk_str_equal(v->last_usr->data,v->last_usr->pos,d->usr->name.data,d->usr->name.len))
		{
			//wtk_debug("update ...\n");
			wtk_vtrain_update_acc2(v->train);
			//if training is enough update hmm
		}else
		{
			//wtk_debug("update [%.*s] %p...\n",v->last_usr->pos,v->last_usr->data,d->usr);
			wtk_vtrain_reset_acc2(v->train);
			wtk_vtrain_update_acc2(v->train);
			wtk_strbuf_reset(v->last_usr);
			if(d->usr)
			{
				wtk_strbuf_push(v->last_usr,d->usr->name.data,d->usr->name.len);
			}else
			{
				//ask who;
				ret=1;
			}
		}
	}
	return ret;
}

void wtk_vprint_save(wtk_vprint_t *v,char *name,int len)
{
	wtk_debug("cnt=%d\n",v->train->cnt);
	if(v->train->cnt<=0){return;}
	wtk_strbuf_reset(v->last_usr);
	wtk_strbuf_push(v->last_usr,name,len);
	wtk_vprint_train_save(v,name,len);
}

void wtk_vprint_get_result(wtk_vprint_t *v,wtk_string_t *result)
{
	wtk_json_item_t *item=0,*nbest,*item2;
	wtk_queue_node_t *qn;
	wtk_vdetect_usr_t *usr;
	wtk_vdetect_t *d=v->detect;
	int ret=-1;
	//int ask;

	wtk_json_reset(v->json);
	wtk_strbuf_reset(v->buf);
	result->len=0;result->data=0;
	if(d->usr)
	{
		item=wtk_json_new_object(v->json);
		if(!item){goto end;}
		wtk_json_obj_add_ref_str(v->json,item,"usr",sizeof("usr")-1,&(v->detect->usr->name));
		wtk_json_obj_add_ref_number_s(v->json,item,"conf",d->max_llr);
	}
	if(!item)
	{
		item=wtk_json_new_object(v->json);
		if(!item){goto end;}
	}
	//wtk_debug("indx=%d\n",v->train->index);
//	if(v->cfg->use_share_vparm)
//	{
//		ask=wtk_vprint_touch(v);
//		v->ask=ask;
//		wtk_json_obj_add_ref_number_s(v->json,item,"ask",ask);
//	}else
//	{
//		v->ask=0;
//	}
	if(d->cfg->usr_q.length)
	{
		nbest=wtk_json_new_array(v->json);
		if(!nbest){goto end;}
		for(qn=d->cfg->usr_q.pop;qn;qn=qn->next)
		{
			usr=data_offset2(qn,wtk_vdetect_usr_t,q_n);
			if(usr->attr & WTK_UBIN_ATTR_INVALID){continue;}
			item2=wtk_json_new_object(v->json);
			wtk_json_obj_add_ref_str(v->json,item2,"usr",sizeof("usr")-1,&(usr->name));
			wtk_json_obj_add_ref_number_s(v->json,item2,"conf",usr->llr);
			wtk_json_array_add_item(v->json,nbest,item2);
		}
		wtk_json_obj_add_item2_s(v->json,item,"nbest",nbest);
	}
	v->json->main=item;
	wtk_json_item_print(item,v->buf);
	result->data=v->buf->data;
	result->len=v->buf->pos;
	wtk_strbuf_push_c(v->buf,'\0');

	ret=0;
end:
	if(ret==-1)
	{
		result->len=0;result->data=0;
		wtk_json_reset(v->json);
		wtk_strbuf_reset(v->buf);
	}
	return;
}

void wtk_vprint_start(wtk_vprint_t *v)
{
	if(v->parm)
	{
		wtk_vparm_start(v->parm);
	}
	if(v->detect->cfg->usr_q.length>0)
	{
		wtk_vdetect_start(v->detect);
	}
	wtk_vtrain_start(v->train);
}

void wtk_vprint_reset(wtk_vprint_t *v)
{
	if(v->parm)
	{
		wtk_vparm_reset(v->parm);
	}
	if(v->detect->cfg->usr_q.length>0)
	{
		wtk_vdetect_reset(v->detect);
	}
	wtk_vtrain_reset(v->train);
}

void wtk_vprint_feed(wtk_vprint_t *v,char *data,int bytes,int is_end)
{
	wtk_vparm_t *parm;
	wtk_queue_node_t *qn;
	wtk_feat_t *f;
	wtk_vparm_feature_t fx;
	int use_detect;

	//wtk_debug("vprint feed=%d\n",bytes);
	use_detect=v->detect->cfg->usr_q.length>0;
	if(v->parm)
	{
		parm=v->parm;
		fx.use_fix=0;
		fx.parm=parm;
		wtk_vparm_feed(parm,data,bytes,is_end);
		//wtk_debug("fead parm len=%d bytes=%d\n",parm->parm_output_q.length,bytes);
//		if(v->train->log)
//		{
//			wtk_log_log(v->train->log,"wav=%d out=%d.",bytes,parm->parm_output_q.length);
//		}
		while(1)
		{
			qn=wtk_queue_pop(&(parm->parm_output_q));
			if(!qn){break;}
			f=data_offset2(qn,wtk_feat_t,queue_n);
			fx.v.feature=f;
			if(use_detect)
			{
				wtk_vdetect_feed_feature(v->detect,&fx,0);
			}
			wtk_vtrain_feed_feature(v->train,&fx,0);
			wtk_fextra_push_feature(parm->parm,f);
		}
		if(is_end)
		{
			//wtk_debug("train frames=%d\n",parm->parm->n_frame_index);
			if(use_detect)
			{
				wtk_vdetect_feed_feature(v->detect,NULL,1);
			}
			wtk_vtrain_feed_feature(v->train,NULL,1);
			//wtk_vprint_touch(v);
		}
	}else
	{
		if(use_detect)
		{
			wtk_vdetect_feed(v->detect,data,bytes,is_end);
		}
		wtk_vtrain_feed(v->train,data,bytes,is_end);
	}
}

void wtk_vprint_print(wtk_vprint_t *v)
{
	wtk_vdetect_print(v->detect);
}
