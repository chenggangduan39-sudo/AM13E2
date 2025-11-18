#include <ctype.h>
#include "qtk_wakeup_trans_model_cfg.h"

qtk_wakeup_trans_model_t* qtk_wakeup_trans_model_new(void)
{
	qtk_wakeup_trans_model_t* trans = (qtk_wakeup_trans_model_t*) wtk_malloc(sizeof(qtk_wakeup_trans_model_t));

	trans->logprob = NULL;
	wtk_queue_init(&(trans->entry_q));
	wtk_queue_init(&(trans->tuple_q));
	wtk_queue_init(&(trans->triple_q));
	wtk_queue_init(&(trans->phone_q));

	return trans;
}

void qtk_wakeup_trans_model_delete(qtk_wakeup_trans_model_t *trans, int use_chain) 
{
	wtk_queue_node_t *n;
	qtk_wakeup_trans_topology_entry_t *entry;
	qtk_wakeup_trans_triples_t* triple;
	qtk_wakeup_trans_tuples_t* tuple;
	qtk_wakeup_trans_phone_t* phn;

	//trans->logprob=NULL;
	if (use_chain != 1) 
	{
		while (1) 
		{
			n = wtk_queue_pop(&(trans->entry_q));
			if (!n) 
			{
				break;
			}
			entry = data_offset(n, qtk_wakeup_trans_topology_entry_t, q_n);
			wtk_free(entry->state_trans);
			wtk_free(entry);
		}
		while (1) 
		{
			n = wtk_queue_pop(&(trans->triple_q));
			if (!n) 
			{
				break;
			}
			triple = data_offset(n, qtk_wakeup_trans_triples_t, q_n);
			wtk_free(triple);
		}

		while (1) 
		{
			n = wtk_queue_pop(&(trans->phone_q));
			if (!n) 
			{
				break;
			}
			phn = data_offset(n, qtk_wakeup_trans_phone_t, q_n);
			wtk_free(phn);
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
			tuple = data_offset(n, qtk_wakeup_trans_tuples_t, q_n);
			wtk_free(tuple);
		}
	}
        wtk_free(trans->phone_cnt);
        int i;
    for(i=0;i<trans->phone_tot_cnt;++i)
    {
//              wtk_debug("i=%d,cnt=%d\n",i,trans->phone_tot_cnt);
        wtk_free(trans->phone2pdf_id_[i]);
    }
    wtk_free(trans->phone2pdf_id_);
    wtk_free(trans->id2phone_id_);
	wtk_free(trans->id2pdf_id_);
	wtk_free(trans);
	trans = NULL;
}

int qtk_wakeup_trans_model_cfg_init(qtk_wakeup_trans_model_cfg_t *cfg) 
{
	cfg->trans_model_fn = NULL;
	cfg->trans_model = NULL;
	cfg->use_chain = 0;

	return 0;
}

int qtk_wakeup_trans_model_cfg_clean(qtk_wakeup_trans_model_cfg_t *cfg, int use_chain) 
{
	qtk_wakeup_trans_model_delete(cfg->trans_model, use_chain);
	return 0;
}

int qtk_wakeup_trans_model_cfg_update_local(qtk_wakeup_trans_model_cfg_t *cfg, wtk_local_cfg_t *lc) 
{
	wtk_string_t *v;

	wtk_local_cfg_update_cfg_str(lc, cfg, trans_model_fn, v);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_chain, v);
	return 0;
}

int wtk_atoi2(char* s, int len) 
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

qtk_wakeup_trans_topology_entry_t* qtk_wakeup_trans_model_load_entry(
		qtk_wakeup_trans_model_t *t_model, wtk_strbuf_t *buf, wtk_source_t *src) 
{
	int ret, trans_count[10], state, len, cnt, phone;
	char c;
	char *s, *e;
	char* sub = (char*) wtk_malloc(sizeof(char) * (strlen("<Transition>") + 1));
	sub = strcpy(sub, "<Transition>");
	//char sub[12]="<Transition>";
	qtk_wakeup_trans_phone_t* phn;

	state = cnt = 0;
	qtk_wakeup_trans_topology_entry_t* entry = NULL;

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
		entry = (qtk_wakeup_trans_topology_entry_t*) wtk_malloc(
				sizeof(qtk_wakeup_trans_topology_entry_t));
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
			phone = wtk_atoi2(s, cnt);
			//		wtk_debug("phone: %d\n",phone);
			phn = (qtk_wakeup_trans_phone_t*) wtk_malloc(sizeof(qtk_wakeup_trans_phone_t));
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

/*
int qtk_wakeup_trans_model_load_normal(qtk_wakeup_trans_model_cfg_t *cfg, wtk_source_t *src) {
	int ret;
	int v, i, count;
	wtk_strbuf_t *buf;
	qtk_wakeup_trans_model_t* trans_model;
	qtk_wakeup_trans_topology_entry_t* entry;
	qtk_wakeup_trans_triples_t* triple;

	trans_model = qtk_wakeup_trans_model_new(cfg);
	cfg->trans_model = trans_model;
	buf = wtk_strbuf_new(256, 1);

	ret = wtk_source_read_string(src, buf);
	if (ret != 0) 
	{
		wtk_debug("00000\n");
		//wtk_debug("read failed.\n");
		goto end;
	}
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<TransitionModel>")) 
	{
//		wtk_debug("111111\n");
		ret = -1;
		goto end;
	}

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

	while (1) 
	{
		entry = qtk_wakeup_trans_model_load_entry(trans_model, buf, src);
		if (!entry) 
		{
			break;
		}
		wtk_queue_push(&(trans_model->entry_q), &(entry->q_n));
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

	ret = wtk_source_read_int(src, &count, 1, 0);
	if (ret != 0) 
	{
		wtk_debug("88888888\n");
		goto end;
	}
	//wtk_debug("%d\n",count);
	for (i = 1; i <= count; i++) 
	{
		triple = (qtk_wakeup_trans_triples_t*) wtk_malloc(sizeof(qtk_wakeup_trans_triples_t));

		ret = wtk_source_read_int(src, &v, 1, 0);
		if (ret != 0) 
		{
			wtk_debug("999999\n");
			goto end;
		}
		triple->phone_id = v;

		ret = wtk_source_read_int(src, &v, 1, 0);
		if (ret != 0) 
		{
			wtk_debug("aaaaaaa\n");
			goto end;
		}
		triple->pdf_id = v;

		ret = wtk_source_read_int(src, &v, 1, 0);
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
	if (ret != 0) 
	{
		wtk_debug("eeeeeee\n");
		goto end;
	}
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<LogProbs>")) {
		//	wtk_debug("ffffffff\n");
		ret = -1;
		goto end;
	}
//	trans_model->logprob=(float*)wtk_malloc(sizeof(float)*23607);
	//trans_model->logprob=(float*)wtk_malloc(sizeof(float)*23515);

	 //trans_model->logprob=(float*)wtk_malloc(sizeof(float)*23607);
	 //ret=wtk_source_read_float(src,trans_model->logprob,23607,0);
	 ret=wtk_source_read_float(src,trans_model->logprob,23607,0);
	 if(ret!=0){wtk_debug("gggggggg\n");goto end;}

	 ret=wtk_source_read_string(src,buf);
	 if(ret!=0){wtk_debug("hhhhhhhhh\n");goto end;}
	 wtk_debug("[%.*s]\n",buf->pos,buf->data);
	 if(!wtk_str_equal_s(buf->data,buf->pos,"</LogProbs>"))
	 {
	 wtk_debug("iiiiiiii\n");
	 ret=-1;
	 goto end;
	 }

	 ret=wtk_source_read_string(src,buf);
	 if(ret!=0){wtk_debug("jjjjjjjj\n");goto end;}

	 if(!wtk_str_equal_s(buf->data,buf->pos,"</TransitionModel>"))
	 {
	 wtk_debug("kkkkkkk\n");
	 ret=-1;
	 goto end;
	 }
	wtk_strbuf_delete(buf);
	end: return ret;
}*/

int qtk_wakeup_trans_phone_cmp(int *d1, qtk_wakeup_trans_phone_t *phone) 
{
	if (*d1 == phone->phone_id) 
	{
		return 0;
	} else 
	{
		return -1;
	}
}

int qtk_wakeup_trans_model_cal_id2pdf(qtk_wakeup_trans_model_t* t_model) 
{
	int in_label, repeat, pdf_id, trans_id;
	int label[30000];
	int i;
	wtk_queue_node_t *qn;
	wtk_queue_t q;
	qtk_wakeup_trans_triples_t* triple;
	qtk_wakeup_trans_phone_t* phone;

	in_label = 1;
	q = t_model->triple_q;

	for (qn = q.pop; qn; qn = qn->next) 
	{
		triple = data_offset(qn, qtk_wakeup_trans_triples_t, q_n);
		pdf_id = triple->pdf_id;
		trans_id = triple->transition_id;
		//wtk_debug("%d %d %d\n",pdf_id,trans_id,triple->phone_id);
		phone = wtk_queue_find(&(t_model->phone_q),
				offsetof(qtk_wakeup_trans_phone_t, q_n),
				(wtk_cmp_handler_t) qtk_wakeup_trans_phone_cmp, &(triple->phone_id));
		//wtk_debug("%p\n",phone);
		repeat = phone->t_entry->state_trans[pdf_id];
		//wtk_debug("%d %d\n",repeat,pdf_id);
		for (i = 0; i < repeat; i++) 
		{
			label[in_label] = trans_id;
			in_label++;
			//		wtk_debug("%d:%d\n",in_label,trans_id);
		}
	}
	t_model->id2pdf_id_ = (int*) wtk_malloc(sizeof(int) * in_label);
	memcpy(t_model->id2pdf_id_, label, sizeof(int) * in_label);

	return 0;
}

int qtk_wakeup_trans_model_load_chain(qtk_wakeup_trans_model_cfg_t *cfg, wtk_source_t *src) 
{
	int ret;
	int v, i, j, count, x, k;
	wtk_strbuf_t *buf;
	float f;
	qtk_wakeup_trans_model_t* trans_model;
	//qtk_trans_topology_entry_t* entry;
	qtk_wakeup_trans_tuples_t* tuple;
	int ishmm = 1;

	trans_model = qtk_wakeup_trans_model_new();
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
		tuple = (qtk_wakeup_trans_tuples_t*) wtk_malloc(sizeof(qtk_wakeup_trans_tuples_t));
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

	wtk_strbuf_delete(buf);
	qtk_wakeup_trans_model_cal_id2pdf_chain(trans_model);

	end: return ret;
}

#define MAX_PDFID_CNT 300
int qtk_wakeup_trans_model_cal_id2pdf_chain(qtk_wakeup_trans_model_t* t_model) 
{
	int in_label, forword_id, loop_id, phone_id=0;
	int label1[50000];
    int label_phone2pdf[400][MAX_PDFID_CNT]={{0}};
    int label_id2phone[50000]={0};  
    int count[400]={0};
    int tmp_phone_id = -1;
    int num_of_phone=0;
    int i=0,j=0,k;

	wtk_queue_node_t *qn;
	wtk_queue_t q;
	qtk_wakeup_trans_tuples_t* tuple;
	//qtk_trans_phone_t* phone;

	in_label = 1;
	q = t_model->tuple_q;
	for (qn = q.pop; qn; qn = qn->next) 
	{
		tuple = data_offset(qn, qtk_wakeup_trans_tuples_t, q_n);
		//pdf_id=tuple->pdf_id;
		forword_id = tuple->forward_trans;
		loop_id = tuple->loop_trans;
        phone_id = tuple->phone_id;
        //wtk_debug("phone_id=%d\n",phone_id);
        if(phone_id!=tmp_phone_id)
        {               
            num_of_phone ++;
            if(i!=0)
            {
        //                  printf("=====i=%d====\n",i);
                count[j++]=i;
                i=0;
            }
        }
        label_phone2pdf[phone_id-1][i] = forword_id;
        //      wtk_debug("l=%d\n",label2[phone_id-1][i] );
        i++;
        label_phone2pdf[phone_id-1][i] = loop_id;
        //      wtk_debug("l=%d\n",label2[phone_id-1][i] );
    	i++;
        label_id2phone[in_label]=phone_id;
		label1[in_label] = forword_id;
		//wtk_debug("in_label:%d %d\n",in_label,forword_id);
		in_label++;
        label_id2phone[in_label]=phone_id;
		label1[in_label] = loop_id;
		//wtk_debug("in_label:%d %d\n",in_label,loop_id);
		in_label++;
		tmp_phone_id = phone_id;
		//		wtk_debug("%d:%d\n",in_label,trans_id);
	}
	//wtk_debug("*** %d\n",in_label);

		count[j++]=i;
        t_model->phone_cnt = (int*)wtk_malloc(sizeof(int)*num_of_phone);
        memcpy(t_model->phone_cnt,count,sizeof(int)*num_of_phone);

        t_model->phone2pdf_id_ = (int**)wtk_malloc(sizeof(int*)*num_of_phone);
        for(k=0;k<num_of_phone;++k)
        {
                t_model->phone2pdf_id_[k]=(int*)wtk_malloc(sizeof(int)*count[k]);
                memcpy(t_model->phone2pdf_id_[k],label_phone2pdf[k],sizeof(int)*count[k]);
        //      count2+=count[k];
        /*      for(l=0;l<count[k];++l)
                {
                //      printf("phone[%d][%d]=%d,\n",k,l,label2[k][l]);
                        printf("phone[%d][%d]=%d ",k,l,t_model->phone2pdf_id_[k][l]);
                }
                printf("%d\n",l);*/
        }
        t_model->id2phone_id_ = (int*)wtk_malloc(sizeof(int)* in_label);
        memcpy(t_model->id2phone_id_, label_id2phone, sizeof(int)*in_label);

        t_model->phone_tot_cnt = num_of_phone;

	t_model->id2pdf_id_ = (int*) wtk_malloc(sizeof(int) * in_label);
	memcpy(t_model->id2pdf_id_, label1, sizeof(int) * in_label);

	//t_model->id2loop=(int*)wtk_malloc(sizeof(int)*in_label);
	//memcpy(t_model->id2loop,label1,sizeof(int)*in_label);

	return 0;
}

int qtk_wakeup_trans_model_cal_id2pdf_chain2(qtk_wakeup_trans_model_t* t_model)
{
	int in_label, forword_id,phone_id=0;
	int label1[30000];
    int label_phone2pdf[400][MAX_PDFID_CNT]={{0}};
    int label_id2phone[50000]={0};  
    int count[400]={0};
    int tmp_phone_id = -1;
    int num_of_phone=0;
    int i=0,j=0,k;
	wtk_queue_node_t *qn;
	wtk_queue_t q;
	qtk_wakeup_trans_triples_t* tuple;

	in_label = 1;
	q = t_model->triple_q;
	for (qn = q.pop; qn; qn = qn->next)
	{
		tuple = data_offset(qn, qtk_wakeup_trans_triples_t, q_n);
		forword_id = tuple->transition_id;
		phone_id = tuple->phone_id;
		if(phone_id!=tmp_phone_id)
        {               
            num_of_phone ++;
            if(i!=0)
            {
               // printf("=====i=%d====\n",i);
                count[j++]=i;
                i=0;
            }
        }
        label_phone2pdf[phone_id-1][i] = forword_id;
            //  wtk_debug("l=%d,phone=%d\n",label_phone2pdf[phone_id-1][i],phone_id);
        i++;
        label_phone2pdf[phone_id-1][i] = forword_id;
            //  wtk_debug("l=%d,phone=%d\n",label_phone2pdf[phone_id-1][i],phone_id);
    	i++;
		label_id2phone[in_label]=phone_id;
		label1[in_label] = forword_id;
		//wtk_debug("in_label:%d %d\n",in_label,forword_id);
		in_label++;
		label_id2phone[in_label]=phone_id;
		label1[in_label] = forword_id;
		//wtk_debug("in_label:%d %d\n",in_label,forword_id);
		in_label++;
		tmp_phone_id = phone_id;
	}
	//wtk_debug("*** %d\n",in_label);
	//wtk_debug("num_of_phone=%d\n",num_of_phone);
	// int l;
	count[j++]=i;
	t_model->phone_cnt = (int*)wtk_malloc(sizeof(int)*num_of_phone);
    memcpy(t_model->phone_cnt,count,sizeof(int)*num_of_phone);
  	t_model->phone2pdf_id_ = (int**)wtk_malloc(sizeof(int*)*num_of_phone);
    for(k=0;k<num_of_phone;++k)
    {
            t_model->phone2pdf_id_[k]=(int*)wtk_malloc(sizeof(int)*count[k]);
            memcpy(t_model->phone2pdf_id_[k],label_phone2pdf[k],sizeof(int)*count[k]);
        //      count2+=count[k];
          /*    for(l=0;l<count[k];++l)
                {
                //      printf("phone[%d][%d]=%d,\n",k,l,label2[k][l]);
                        printf("phone[%d][%d]=%d ",k,l,t_model->phone2pdf_id_[k][l]);
                }
                printf("%d\n",l);*/
    }
    t_model->id2phone_id_ = (int*)wtk_malloc(sizeof(int)* in_label);
    memcpy(t_model->id2phone_id_, label_id2phone, sizeof(int)*in_label);

    t_model->phone_tot_cnt = num_of_phone;
	t_model->id2pdf_id_ = (int*) wtk_malloc(sizeof(int) * in_label);
	memcpy(t_model->id2pdf_id_, label1, sizeof(int) * in_label);
	return 0;
}

int qtk_wakeup_trans_model_load_normal(qtk_wakeup_trans_model_cfg_t *cfg, wtk_source_t *src) {
	int ret;
	int v, i, count,x,ishmm=1,k,j;
	wtk_strbuf_t *buf;
	qtk_wakeup_trans_model_t* trans_model;
	qtk_wakeup_trans_triples_t* triple;

	trans_model = qtk_wakeup_trans_model_new();
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
		triple = (qtk_wakeup_trans_triples_t*) wtk_malloc(sizeof(qtk_wakeup_trans_triples_t));
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
	qtk_wakeup_trans_model_cal_id2pdf_chain2(trans_model);
	wtk_strbuf_delete(buf);
	end: return ret;
}

int qtk_wakeup_trans_model_cfg_update(qtk_wakeup_trans_model_cfg_t *cfg, wtk_source_loader_t *sl) 
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
