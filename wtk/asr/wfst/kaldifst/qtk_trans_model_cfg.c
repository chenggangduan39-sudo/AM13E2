#include <ctype.h>
#include "qtk_trans_model_cfg.h"

qtk_trans_model_t* qtk_trans_model_new(void)
{
	qtk_trans_model_t* trans = (qtk_trans_model_t*) wtk_malloc(sizeof(qtk_trans_model_t));

	trans->logprob = NULL;
	wtk_queue_init(&(trans->tuple_q));
	wtk_queue_init(&(trans->triple_q));
	trans->entries = NULL;
	trans->phones = NULL;
	trans->id2phone_id_ = NULL;
	return trans;
}

void qtk_trans_model_delete(qtk_trans_model_t *trans, int use_chain) 
{
	wtk_queue_node_t *n;
	qtk_trans_triples_t* triple;
	qtk_trans_tuples_t* tuple;
	qtk_trans_topology_entry_t* entry;
	int i;
	//trans->logprob=NULL;
	if (use_chain != 1) 
	{
		while (1) 
		{
			n = wtk_queue_pop(&(trans->triple_q));
			if (!n) 
			{
				break;
			}
			triple = data_offset(n, qtk_trans_triples_t, q_n);
			wtk_free(triple);
		}

		if (trans->logprob) 
		{
			wtk_free(trans->logprob);
		}
	} else 
	{
		while (1) 
		{
			n = wtk_queue_pop(&(trans->tuple_q));
			if (!n) 
			{
				break;
			}
			tuple = data_offset(n, qtk_trans_tuples_t, q_n);
			wtk_free(tuple);
		}
	}

	if(trans->entries)
	{
		for(i=0;i<trans->nentries;i++)
		{
			entry = trans->entries+i;
			wtk_free(entry->state_trans);
		}
		wtk_free(trans->entries);
	}
	if(trans->phones)
	{
		wtk_free(trans->phones);
	}

	wtk_free(trans->id2pdf_id_);

	if(trans->id2phone_id_)
	{
		wtk_free(trans->id2phone_id_);
	}

	wtk_free(trans);
	trans = NULL;
}

int qtk_trans_model_cfg_init(qtk_trans_model_cfg_t *cfg) 
{
	cfg->trans_model_fn = NULL;
	cfg->trans_model = NULL;
	cfg->use_chain = 0;

	return 0;
}

int qtk_trans_model_cfg_clean(qtk_trans_model_cfg_t *cfg, int use_chain) 
{
	qtk_trans_model_delete(cfg->trans_model, use_chain);
	return 0;
}

int qtk_trans_model_cfg_update_local(qtk_trans_model_cfg_t *cfg, wtk_local_cfg_t *lc) 
{
	wtk_string_t *v;

	wtk_local_cfg_update_cfg_str(lc, cfg, trans_model_fn, v);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_chain, v);
	return 0;
}

int wtk_atoi(char* s, int len) 
{
	int ret;
	char *tmp = (char*) wtk_malloc(len * sizeof(char));
	memcpy(tmp, s, len);
	ret = atoi(tmp);
	if (tmp) 
	{
		wtk_free(tmp);
	}
	return ret;
}
/*
qtk_trans_topology_entry_t* qtk_trans_model_load_entry(
		qtk_trans_model_t *t_model, wtk_strbuf_t *buf, wtk_source_t *src) 
{
	int ret, trans_count[10], state, len, cnt, phone;
	char c;
	char *s, *e;
	char* sub = (char*) wtk_malloc(sizeof(char) * (strlen("<Transition>") + 1));
	sub = strcpy(sub, "<Transition>");
	//char sub[12]="<Transition>";
	qtk_trans_phone_t* phn;

	state = cnt = 0;
	qtk_trans_topology_entry_t* entry = NULL;

	ret = wtk_source_read_string(src, buf);
	if (ret != 0) 
	{
		goto end;
	}

	if (!wtk_str_equal_s(buf->data, buf->pos, "<TopologyEntry>")) 
	{
		//wtk_debug("read failed2.\n");
		ret = 0;
		goto end;
	} else 
	{
		entry = (qtk_trans_topology_entry_t*) wtk_malloc(
				sizeof(qtk_trans_topology_entry_t));
		//wtk_debug("entry %p\n",entry);
	}

	ret = wtk_source_read_string(src, buf);
	if (ret != 0) 
	{
		wtk_debug("read failed3.\n");
		//wtk_debug("read failed.\n");
		goto end;
	}
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<ForPhones>")) 
	{
		wtk_debug("read failed4.\n");
		ret = -1;
		goto end;
	}

	wtk_source_read_line(src, buf);
	wtk_source_read_line(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	s = buf->data;
	e = s + buf->pos;
	while (s < e) 
	{
		c = *s;
		if (isspace(c)) 
		{
			s++;
			continue;
		} else 
		{
			do 
			{
				c = *(s + cnt);
				cnt++;
			} while (isdigit(c));
			phone = wtk_atoi(s, cnt);
			//		wtk_debug("phone: %d\n",phone);
			phn = (qtk_trans_phone_t*) wtk_malloc(sizeof(qtk_trans_phone_t));
			phn->phone_id = phone;
			phn->t_entry = entry;
			wtk_queue_push(&(t_model->phone_q), &(phn->q_n));
			s += cnt;
			cnt = 0;
		}
	}

	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (ret != 0) 
	{
		wtk_debug("read failed5.\n");
		//wtk_debug("read failed.\n");
		goto end;
	}

	if (!wtk_str_equal_s(buf->data, buf->pos, "</ForPhones>")) 
	{
		wtk_debug("read failed6.\n");
		ret = -1;
		goto end;
	}

	while (1) 
	{
		ret = wtk_source_read_string(src, buf);
		if (ret != 0) 
		{
			wtk_debug("read failed7.\n");
			//wtk_debug("read failed.\n");
			goto end;
		}

		if (!wtk_str_equal_s(buf->data, buf->pos, "<State>")) 
		{
			break;
		} else 
		{
			wtk_source_read_line(src, buf);
			//wtk_debug("[%.*s]\n",buf->pos,buf->data);
			int find = 0, trans = 0, add = 0;
			while (find != -1) 
			{
				find = wtk_str_str((buf->data + add), (buf->pos - add), sub, 12);
				add += find + 1;
				//wtk_debug("%d\n",find);
				trans++;
			}
			trans_count[state] = trans - 1;
			//wtk_debug("%d\n",trans);
			state++;
		}
	}
	len = sizeof(int) * state;
	entry->state_cnt = state;
	entry->state_trans = (int*) wtk_malloc(len);
	memcpy(entry->state_trans, trans_count, len);

	end: 
	if (sub) 
	{
		wtk_free(sub);
	}
	return entry;
}
*/

int qtk_trans_model_cal_id2pdf_chain2(qtk_trans_model_t* t_model);
int qtk_trans_model_load_normal(qtk_trans_model_cfg_t *cfg, wtk_source_t *src) {
	int ret;
	int v, i, count,x,ishmm=1,k,j;
	wtk_strbuf_t *buf;
	qtk_trans_model_t* trans_model;
	qtk_trans_triples_t* triple;

	trans_model = qtk_trans_model_new();
	cfg->trans_model = trans_model;
	buf = wtk_strbuf_new(256, 1);

	ret = wtk_source_read_string(src, buf);
	if (ret != 0)
	{
		wtk_debug("00000\n");
		wtk_debug("read failed.\n");
		goto end;
	}
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<TransitionModel>"))
	{
//		wtk_debug("111111\n");
		if (!wtk_str_equal_s(buf->data, buf->pos, "<Topology>"))
		{
			//		wtk_debug("333333\n");
			ret = -1;
			goto end;
		}
	} else
	{
		ret = wtk_source_read_string(src, buf);
		if (ret != 0)
		{
//		wtk_debug("2222222\n");
			//wtk_debug("read failed.\n");
			goto end;
		}

		wtk_debug("[%.*s]\n",buf->pos,buf->data);
		if (!wtk_str_equal_s(buf->data, buf->pos, "<Topology>"))
		{
//		wtk_debug("333333\n");
			ret = -1;
			goto end;
		}
	}
	//ret=wtk_source_read_int(src,&count,1,0);
	ret = wtk_source_read_char(src);
	//wtk_debug("%d\n",ret);
	ret = wtk_source_read_char(src);
	//wtk_debug("%d\n",ret);

	ret = wtk_source_read_int_little(src, &count, 1, 1);
	//wtk_debug("%d\n",count);
	trans_model->phones = (qtk_trans_phone_t*)wtk_malloc(count*sizeof(qtk_trans_phone_t));
	for (i = 0; i < count; i++)
	{
		ret = wtk_source_read_int_little(src, &v, 1, 1);
		(trans_model->phones+i)->phone_id = v;
		//wtk_debug("%d\n",v);
	}

	ret = wtk_source_read_char(src);
	//wtk_debug("%d\n",ret);

	ret = wtk_source_read_int_little(src, &count, 1, 1);
	//wtk_debug("%d\n",count);

	j = 0;
	for (i = 0; i < count; i++)
	{
		ret = wtk_source_read_int_little(src, &v, 1, 1);// phone 所属的entry
		if(v>=0)
		{
			(trans_model->phones+j)->entry_id = v;
			j++;
		}
		//wtk_debug("%d\n",v);
	}

	ret = wtk_source_read_char(src);
	//wtk_debug("%d\n",ret);
	ret = wtk_source_read_int_little(src, &count, 1, 1);
	//wtk_debug("%d\n",count);//entry cnt
	if (count != -1)
	{
		//ishmm=0;
	} else
	{
		ishmm = 0;
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_int_little(src, &count, 1, 1);
		//wtk_debug("%d\n",count);
	}

	trans_model->entries = (qtk_trans_topology_entry_t*)wtk_malloc(sizeof(qtk_trans_topology_entry_t)*count);
	trans_model->nentries = count;
	for (i = 0; i < count; i++)
	{
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_int_little(src, &v, 1, 1);//state conuts -1是pdf
		//wtk_debug("%d\n",v);
		(trans_model->entries+i)->state_cnt = v;
		(trans_model->entries+i)->state_trans = (int*)wtk_malloc(sizeof(int)*v);
		for (j = 0; j < v; j++)
		{
			ret = wtk_source_read_char(src);
			//wtk_debug("%d\n",ret);
			ret = wtk_source_read_int_little(src, &x, 1, 1);
			//wtk_debug("%d\n",x); //pdf
			if (ishmm)
			{
				//TODO
			} else
			{
				ret = wtk_source_read_char(src);
				//wtk_debug("%d\n",ret);
				ret = wtk_source_read_int_little(src, &x, 1, 1);
				//wtk_debug("%d\n",x);
			}
			ret = wtk_source_read_char(src);
			//wtk_debug("%d\n",ret);
			ret = wtk_source_read_int_little(src, &x, 1, 1);//repeat times
			//wtk_debug("%d\n",x);
			*((trans_model->entries+i)->state_trans+j) = x;
			for (k = 0; k < x; k++)
			{
				int aaa;
				float fff;
				ret = wtk_source_read_char(src);
				//wtk_debug("%d\n",ret);
				ret = wtk_source_read_int_little(src, &aaa, 1, 1);
				//wtk_debug("%d\n",aaa);

				ret = wtk_source_read_char(src);
				//wtk_debug("%d\n",ret);
				ret = wtk_source_read_float(src, &fff, 1, 1);
				//wtk_debug("%f\n",fff);
			}
		}
	}

	ret = wtk_source_read_string(src, buf);
	if (ret != 0)
	{
		wtk_debug("00000\n");
		goto end;
	}
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos, "</Topology>"))
	{
		ret = -1;
		goto end;
	}

	ret = wtk_source_read_string(src, buf);
	if (ret != 0)
	{
//		wtk_debug("4444444\n");
		//wtk_debug("read failed.\n");
		goto end;
	}
//	wtk_debug("[%.*s]\n",buf->pos,buf->data);

	if (!wtk_str_equal_s(buf->data, buf->pos, "<Triples>"))
	{
//		wtk_debug("777777\n");
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_int_little(src, &count, 1, 1);
	if (ret != 0)
	{
		wtk_debug("88888888\n");
		goto end;
	}
	//wtk_debug("%d\n",count);
	for (i = 1; i <= count; i++)
	{
		triple = (qtk_trans_triples_t*) wtk_malloc(sizeof(qtk_trans_triples_t));
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_int_little(src, &v, 1, 1);
		if (ret != 0)
		{
			wtk_debug("999999\n");
			goto end;
		}
		triple->phone_id = v;
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_int_little(src, &v, 1, 1);
		if (ret != 0)
		{
			wtk_debug("aaaaaaa\n");
			goto end;
		}
		triple->pdf_id = v;
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_int_little(src, &v, 1, 1);
		if (ret != 0)
		{
			wtk_debug("bbbbbbb\n");
			goto end;
		}
		triple->transition_id = v;
		wtk_queue_push(&(trans_model->triple_q), &(triple->q_n));
	}

	ret = wtk_source_read_string(src, buf);
	if (ret != 0)
	{
		wtk_debug("ccccccc\n");
		goto end;
	}
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos, "</Triples>"))
	{
		//	wtk_debug("ddddddd\n");
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_string(src, buf);
	//	wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<LogProbs>"))
	{
		ret = -1;
		goto end;
	}

	ret = wtk_source_read_char(src);
	//wtk_debug("%d\n",ret);
	ret = wtk_source_read_char(src);
	//wtk_debug("%d\n",ret);
	ret = wtk_source_read_char(src);
	//wtk_debug("%d\n",ret);
	ret = wtk_source_read_char(src);
	//wtk_debug("%d\n",ret);
	ret = wtk_source_read_char(src);
	//wtk_debug("%d\n",ret);
	ret = wtk_source_read_int_little(src, &v, 1, 1);
	//wtk_debug("%d\n",v);
	float f;
	for (i = 0; i < v; i++)
	{
		ret = wtk_source_read_float_little(src, &f, 1, 1);
		//wtk_debug("%f\n",f);
	}

	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos, "</LogProbs>"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos, "</TransitionModel>"))
	{
		ret = -1;
		goto end;
	}
	qtk_trans_model_cal_id2pdf_chain2(trans_model);
	wtk_strbuf_delete(buf);
	end: return ret;
}

int qtk_trans_model_cal_id2pdf_chain2(qtk_trans_model_t* t_model)
{
	int in_label, forword_id, phone_id, pdf_id, repeat=2,j,entry_id;
	int label1[30000];
	int label2[30000];
	//int label2[30000];
	wtk_queue_node_t *qn;
	wtk_queue_t q;
	qtk_trans_triples_t* tuple;

	in_label = 1;
	q = t_model->triple_q;
	for (qn = q.pop; qn; qn = qn->next)
	{
		tuple = data_offset(qn, qtk_trans_triples_t, q_n);
		forword_id = tuple->transition_id;     //pdf-id
		phone_id = tuple->phone_id;            //phone
		pdf_id = tuple->pdf_id;                //state
	
		entry_id = (t_model->phones+phone_id-1)->entry_id;
		repeat = *((t_model->entries+entry_id)->state_trans+pdf_id);
		//wtk_debug("%d %d %d\n",phone_id,pdf_id,repeat);
		for(j=0;j<repeat;j++)
		{
			 label1[in_label] = forword_id;
			 label2[in_label] = phone_id;
			 in_label++;
		}
	}
	//wtk_debug("*** %d\n",in_label);
	t_model->id2pdf_id_ = (int*) wtk_malloc(sizeof(int) * in_label);
	memcpy(t_model->id2pdf_id_, label1, sizeof(int) * in_label);
	t_model->id2phone_id_ = (int*) wtk_malloc(sizeof(int) * in_label);
	memcpy(t_model->id2phone_id_, label2, sizeof(int) * in_label);
	t_model->trans_num = in_label;

	return 0;
}

int qtk_trans_phone_cmp(int *d1, qtk_trans_phone_t *phone) 
{
	if (*d1 == phone->phone_id) 
	{
		return 0;
	} else 
	{
		return -1;
	}
}
/*
int qtk_trans_model_cal_id2pdf(qtk_trans_model_t* t_model) 
{
	int in_label, repeat, pdf_id, trans_id;
	int label[30000];
	int i;
	wtk_queue_node_t *qn;
	wtk_queue_t q;
	qtk_trans_triples_t* triple;
	qtk_trans_phone_t* phone;

	in_label = 1;
	q = t_model->triple_q;

	for (qn = q.pop; qn; qn = qn->next) 
	{
		triple = data_offset(qn, qtk_trans_triples_t, q_n);
		pdf_id = triple->pdf_id;
		trans_id = triple->transition_id;
		//wtk_debug("%d %d %d\n",pdf_id,trans_id,triple->phone_id);
		phone = wtk_queue_find(&(t_model->phone_q),
				offsetof(qtk_trans_phone_t, q_n),
				(wtk_cmp_handler_t) qtk_trans_phone_cmp, &(triple->phone_id));
		//wtk_debug("%p\n",phone);
		repeat = phone->t_entry->state_trans[pdf_id];
		//wtk_debug("%d %d\n",repeat,pdf_id);
		for (i = 0; i < repeat; i++) 
		{
			label[in_label] = trans_id;
			in_label++;
			//		wtk_debug("%d:%d\n",in_label,trans_id);
			printf("%d:%d\n",in_label,trans_id);
		}
	}
	t_model->id2pdf_id_ = (int*) wtk_malloc(sizeof(int) * in_label);
	memcpy(t_model->id2pdf_id_, label, sizeof(int) * in_label);

	return 0;
}
*/
int qtk_trans_model_load_chain(qtk_trans_model_cfg_t *cfg, wtk_source_t *src) 
{
	int ret;
	int v, i, j, count, x, k;
	wtk_strbuf_t *buf;
	float f;
	qtk_trans_model_t* trans_model;
	//qtk_trans_topology_entry_t* entry;
	qtk_trans_tuples_t* tuple;
	int ishmm = 1;

	trans_model = qtk_trans_model_new();
	cfg->trans_model = trans_model;
	//wtk_debug("fffa:%p\n",trans_model);
	buf = wtk_strbuf_new(256, 1);

	ret = wtk_source_read_string(src, buf);
	if (ret != 0) 
	{
		wtk_debug("00000\n");
		wtk_debug("read failed.\n");
		goto end;
	}
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<TransitionModel>")) 
	{
//		wtk_debug("111111\n");
		if (!wtk_str_equal_s(buf->data, buf->pos, "<Topology>")) 
		{
			//		wtk_debug("333333\n");
			ret = -1;
			goto end;
		}
	} else 
	{
		ret = wtk_source_read_string(src, buf);
		if (ret != 0) 
		{
//		wtk_debug("2222222\n");
			//wtk_debug("read failed.\n");
			goto end;
		}

		//wtk_debug("[%.*s]\n",buf->pos,buf->data);
		if (!wtk_str_equal_s(buf->data, buf->pos, "<Topology>")) 
		{
//		wtk_debug("333333\n");
			ret = -1;
			goto end;
		}
	}
	//ret=wtk_source_read_int(src,&count,1,0);
	ret = wtk_source_read_char(src);
	//wtk_debug("%d\n",ret);
	ret = wtk_source_read_char(src);
	//wtk_debug("%d\n",ret);

	ret = wtk_source_read_int_little(src, &count, 1, 1);
	//wtk_debug("%d\n",count);

	for (i = 0; i < count; i++) 
	{
		ret = wtk_source_read_int_little(src, &v, 1, 1);
		//wtk_debug("%d\n",v);
	}

	ret = wtk_source_read_char(src);
	//wtk_debug("%d\n",ret);

	ret = wtk_source_read_int_little(src, &count, 1, 1);
	//wtk_debug("%d\n",count);

	for (i = 0; i < count; i++) 
	{
		ret = wtk_source_read_int_little(src, &v, 1, 1);
		//wtk_debug("%d\n",v);
	}

	ret = wtk_source_read_char(src);
	//wtk_debug("%d\n",ret);
	ret = wtk_source_read_int_little(src, &count, 1, 1);
	//wtk_debug("%d\n",count);
	if (count != -1) 
	{
		//ishmm=0;
	} else 
	{
		ishmm = 0;
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_int_little(src, &count, 1, 1);
		//wtk_debug("%d\n",count);
	}

	for (i = 0; i < count; i++) 
	{
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_int_little(src, &v, 1, 1);
		//wtk_debug("%d\n",v);
		for (j = 0; j < v; j++) 
		{
			ret = wtk_source_read_char(src);
			//wtk_debug("%d\n",ret);
			ret = wtk_source_read_int_little(src, &x, 1, 1);
			//wtk_debug("%d\n",x);
			if (ishmm) 
			{
				//TODO
			} else 
			{
				ret = wtk_source_read_char(src);
				//wtk_debug("%d\n",ret);
				ret = wtk_source_read_int_little(src, &x, 1, 1);
				//wtk_debug("%d\n",x);
			}
			ret = wtk_source_read_char(src);
			//wtk_debug("%d\n",ret);
			ret = wtk_source_read_int_little(src, &x, 1, 1);
			//wtk_debug("%d\n",x);

			for (k = 0; k < x; k++) 
			{
				int aaa;
				float fff;
				ret = wtk_source_read_char(src);
				//wtk_debug("%d\n",ret);
				ret = wtk_source_read_int_little(src, &aaa, 1, 1);
				//wtk_debug("%d\n",aaa);

				ret = wtk_source_read_char(src);
				//wtk_debug("%d\n",ret);
				ret = wtk_source_read_float(src, &fff, 1, 1);
				//wtk_debug("%f\n",fff);
			}
		}
	}

	ret = wtk_source_read_string(src, buf);
	if (ret != 0) 
	{
		wtk_debug("00000\n");
		goto end;
	}
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos, "</Topology>")) 
	{
		ret = -1;
		goto end;
	}

	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);

	ret = wtk_source_read_char(src);
	//wtk_debug("%d\n",ret);
	ret = wtk_source_read_char(src);
	//wtk_debug("%d\n",ret);
	ret = wtk_source_read_int_little(src, &count, 1, 1);
	//wtk_debug("%d\n",count);

	for (i = 0; i < count; i++) 
	{
		tuple = (qtk_trans_tuples_t*) wtk_malloc(sizeof(qtk_trans_tuples_t));
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_int_little(src, &v, 1, 1);
		tuple->phone_id = v;
		//wtk_debug("%d\n",v);

		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_int_little(src, &v, 1, 1);
		tuple->pdf_id = v;
		//wtk_debug("%d\n",v);

		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_int_little(src, &v, 1, 1);
		tuple->loop_trans = v;
		//wtk_debug("%d\n",v);

		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_int_little(src, &v, 1, 1);
		tuple->forward_trans = v;
		wtk_queue_push(&(trans_model->tuple_q), &(tuple->q_n));
		//wtk_debug("%d\n",v);
	}

	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos,
			"</Triples>")&&!wtk_str_equal_s(buf->data,buf->pos,"</Tuples>")) 
	{
		ret = -1;
		goto end;
	}

	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<LogProbs>")) 
	{
		ret = -1;
		goto end;
	}

	ret = wtk_source_read_char(src);
	//wtk_debug("%d\n",ret);
	ret = wtk_source_read_char(src);
	//wtk_debug("%d\n",ret);
	ret = wtk_source_read_char(src);
	//wtk_debug("%d\n",ret);
	ret = wtk_source_read_char(src);
	//wtk_debug("%d\n",ret);
	ret = wtk_source_read_char(src);
	//wtk_debug("%d\n",ret);
	ret = wtk_source_read_int_little(src, &v, 1, 1);
	//wtk_debug("%d\n",v);

	for (i = 0; i < v; i++) 
	{
		ret = wtk_source_read_float_little(src, &f, 1, 1);
		//wtk_debug("%f\n",f);
	}

	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos, "</LogProbs>")) 
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos, "</TransitionModel>")) 
	{
		ret = -1;
		goto end;
	}

	qtk_trans_model_cal_id2pdf_chain(trans_model);

	end:
	wtk_strbuf_delete(buf);
	return ret;
}

int qtk_trans_model_cal_id2pdf_chain(qtk_trans_model_t* t_model) 
{
	int in_label, forword_id, loop_id, cnt;
//	int label1[30000];
//	int label2[30000];
	int *label1, *label2;
	//int i;
	wtk_queue_node_t *qn;
	wtk_queue_t q;
	qtk_trans_tuples_t* tuple;
	//qtk_trans_phone_t* phone;

	q = t_model->tuple_q;

	cnt=1;
	for (qn = q.pop; qn; qn = qn->next)
		cnt+=2;
	label1=wtk_malloc(sizeof(int) * cnt);
	label1[0]=-1;
	label2=wtk_malloc(sizeof(int) * cnt);
	label2[0]=-1;

	in_label = 1;
	for (qn = q.pop; qn; qn = qn->next) 
	{
		tuple = data_offset(qn, qtk_trans_tuples_t, q_n);
		//pdf_id=tuple->pdf_id;
		forword_id = tuple->forward_trans;
		loop_id = tuple->loop_trans;
		//wtk_debug("%d %d %d\n",pdf_id,trans_id,triple->phone_id);
		//phone=wtk_queue_find(&(t_model->phone_q),offsetof(qtk_trans_phone_t,q_n),(wtk_cmp_handler_t)qtk_trans_phone_cmp,&(tuple->phone_id));
		//wtk_debug("%p\n",phone);
		//repeat=phone->t_entry->state_trans[pdf_id];
		//wtk_debug("%d %d\n",repeat,pdf_id);
		//for(i=0;i<repeat;i++)
		//{
		label1[in_label] = forword_id;
		label2[in_label] = tuple->phone_id;
		//printf("id2pdf_id: %d %d\n",in_label,forword_id);
		in_label++;
		label1[in_label] = loop_id;
		label2[in_label] = tuple->phone_id;
		//printf("id2pdf_id: %d %d\n",in_label,loop_id);
		in_label++;
		//printf("%d %d\n",in_label,trans_id);
		//}
	}
	//wtk_debug("*** %d\n",in_label);
	t_model->id2pdf_id_ = (int*) wtk_malloc(sizeof(int) * in_label);
	memcpy(t_model->id2pdf_id_, label1, sizeof(int) * in_label);
	t_model->id2phone_id_ = (int*) wtk_malloc(sizeof(int) * in_label);
	memcpy(t_model->id2phone_id_, label2, sizeof(int) * in_label);
	t_model->trans_num = in_label;
	//t_model->id2loop=(int*)wtk_malloc(sizeof(int)*in_label);
	//memcpy(t_model->id2loop,label1,sizeof(int)*in_label);
	wtk_free(label1);
	wtk_free(label2);

	return 0;
}

int qtk_trans_model_cfg_update(qtk_trans_model_cfg_t *cfg, wtk_source_loader_t *sl) 
{
	int ret = 0;

	/*	if(!cfg->trans_model_fn)
	 {
	 wtk_debug("failed1.\n");
	 ret=-1;
	 goto end;
	 }
	 if(!cfg->use_chain)
	 {
	 ret=wtk_source_loader_load(sl,cfg,(wtk_source_load_handler_t)qtk_trans_model_load_normal,cfg->trans_model_fn);
	 //wtk_debug("failed2.\n");
	 ret=qtk_trans_model_cal_id2pdf(cfg->trans_model);
	 if(ret!=0){wtk_debug("failed3.\n");goto end;}
	 }else
	 {
	 ret=wtk_source_loader_load(sl,cfg,(wtk_source_load_handler_t)qtk_trans_model_load_chain,cfg->trans_model_fn);
	 //wtk_debug("failed2.\n");
	 ret=qtk_trans_model_cal_id2pdf_chain(cfg->trans_model);
	 if(ret!=0){wtk_debug("failed3.\n");goto end;}
	 }
	 end:*/
	return ret;
}
