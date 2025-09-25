#include "wtk_transcription.h"
#include "wtk/core/wtk_str.h"
#include<ctype.h>
wtk_transcription_t* wtk_transcription_new_h(wtk_heap_t *heap)
{
	wtk_transcription_t* trans;

	trans=(wtk_transcription_t*)wtk_heap_malloc(heap,sizeof(*trans));
	trans->forceout=0;
	trans->use_dummy_wrd=0;
	trans->issil=0;
	trans->name=0;
	wtk_queue_init(&(trans->lab_queue));
	return trans;
}

wtk_lablist_t* wtk_lablist_new_h(wtk_heap_t *heap,int naux)
{
	wtk_lablist_t *ll;

	ll=(wtk_lablist_t*)wtk_heap_malloc(heap,sizeof(*ll));
	wtk_queue_init(&(ll->lable_queue));
	ll->max_aux_lab=naux;
	return ll;
}

wtk_lab_t* wtk_lab_new_h(wtk_heap_t *heap,int naux)
{
	wtk_lab_t *lab;
	wtk_string_t **id;
	float *s;
	int i;

	lab=(wtk_lab_t*)wtk_heap_malloc(heap,sizeof(*lab));
	//lab=(wtk_lab_t*)malloc(sizeof(*lab));
	lab->name=0;
	lab->score=0;
	lab->name_id=-1;
	lab->start=lab->end=0;
	if(naux>0)
	{
		id=(wtk_string_t**)wtk_heap_malloc(heap,sizeof(wtk_string_t*)*naux);
		s=(float*)wtk_heap_malloc(heap,sizeof(float)*naux);
		lab->aux_lab=id-1;lab->aux_score=s-1;
		for(i=1;i<=naux;++i)
		{
			lab->aux_lab[i]=0;
			lab->aux_score[i]=0;
		}
	}else
	{
		lab->aux_lab=0;
		lab->aux_score=0;
	}
	return lab;
}

wtk_lab_t* wtk_transcription_peek(wtk_transcription_t *trans,int index)
{
	wtk_queue_node_t *n;
	wtk_lablist_t *ll;
	wtk_lab_t *l=0;
	int i;

	ll=data_offset(trans->lab_queue.pop,wtk_lablist_t,trans_n);
	if(!ll){goto end;}
	for(i=0,n=ll->lable_queue.pop;n && i<index;n=n->next,++i)
	{
		//l=data_offset(n,wtk_lab_t,lablist_n);
	}
	if(!n){goto end;}
	l=data_offset(n,wtk_lab_t,lablist_n);
end:
	return l;
}

double wtk_transcription_all_score(wtk_transcription_t *trans)
{
	wtk_queue_node_t *n;
	wtk_lablist_t *ll;
	wtk_lab_t *l=0;
	double s=0;

	ll=data_offset(trans->lab_queue.pop,wtk_lablist_t,trans_n);
	if(!ll){goto end;}
	for(n=ll->lable_queue.pop;n ;n=n->next)
	{
		l=data_offset(n,wtk_lab_t,lablist_n);
		s+=l->score;
	}
end:
	return s;
}


void wtk_lab_print(wtk_lab_t *l)
{
	printf("%f %f %.*s %f\n",l->start,l->end,l->name->len,l->name->data,l->score);
}

double wtk_transcription_get_tot_prob(wtk_transcription_t* trans)
{
	wtk_queue_node_t *n;
	wtk_lablist_t *ll;
	wtk_lab_t *l;
	double s=0;

	ll=data_offset(trans->lab_queue.pop,wtk_lablist_t,trans_n);
	if(!ll){goto end;}
	for(n=ll->lable_queue.pop;n;n=n->next)
	{
		l=data_offset(n,wtk_lab_t,lablist_n);
		s+=l->score;
	}
end:
	return s;
}

double wtk_transcription_get_overlap_prob(wtk_transcription_t* trans,wtk_lab_t* lab)
{
	wtk_queue_node_t *n;
	wtk_lablist_t *ll;
	wtk_lab_t *l;
	double s=0;

	ll=data_offset(trans->lab_queue.pop,wtk_lablist_t,trans_n);
	if(!ll){goto end;}
	for(n=ll->lable_queue.pop;n;n=n->next)
	{
		l=data_offset(n,wtk_lab_t,lablist_n);
		if(l->end>lab->start)
		{
			if(l->start<lab->end)
			{
				s+=l->score*(min(l->end,lab->end) - max(l->start,lab->start))/(l->end - l->start);
				//wtk_debug("s=%f,%f %f/%f\n",s,l->score,(min(l->end,lab->end) - max(l->start,lab->start)),(l->end - l->start));
				//wtk_debug("score=%f,overlap=%f,dur=%f\n",l->score,(min(l->end,lab->end) - max(l->start,lab->start)),(l->end - l->start));
			}else
			{
				break;
			}
		}
	}
end:
	return s;
}

void wtk_transcription_to_string(wtk_transcription_t *trans,wtk_strbuf_t *buf)
{
	wtk_queue_node_t *cur,*cur2;
	wtk_lablist_t *ll;
	wtk_lab_t *lab;
	wtk_string_t *id;
	int i=0,j=0;

	wtk_strbuf_reset(buf);
	for(cur=(trans->lab_queue.pop);cur;cur=cur->next)
	{
		ll=data_offset(cur,wtk_lablist_t,trans_n);
		if(j>0)
		{
			wtk_strbuf_push_s(buf,"|");
		}
		++j;
		for(cur2=(ll->lable_queue.pop);cur2;cur2=cur2->next)
		{
			lab=data_offset(cur2,wtk_lab_t,lablist_n);
			if(ll->max_aux_lab>=2)
			{
				if(lab->aux_lab[2])
				{
					id=lab->aux_lab[2];
					if(!wtk_string_equal_s(id,"sil") && !wtk_string_equal_s(id,"<s>") && !wtk_string_equal_s(id,"</s>")
							&& !wtk_string_equal_s(id, "<eps>"))
					{
						if(i>0)
						{
							wtk_strbuf_push_s(buf," ");
						}
						wtk_strbuf_push(buf,id->data,id->len);
						++i;
					}
				}
			}else
			{
				id=lab->name;
				if(wtk_string_equal_s(id,"sil") || wtk_string_equal_s(id,"<s>") || wtk_string_equal_s(id,"</s>"))
				{
					continue;
				}
				if(i>0)
				{
					wtk_strbuf_push_s(buf," ");
				}
				wtk_strbuf_push(buf,lab->name->data,lab->name->len);
				++i;
			}
		}
	}
}

void wtk_transcription_to_string2(wtk_transcription_t *trans,wtk_strbuf_t *buf,char sep)
{
	wtk_transcription_to_string3(trans, buf, sep);
}

int wtk_transcription_to_string3(wtk_transcription_t *trans,wtk_strbuf_t *buf,char sep)
{
	wtk_queue_node_t *cur,*cur2;
	wtk_lablist_t *ll;
	wtk_lab_t *lab;
	wtk_string_t *id;
	int i=0,j=0;
	int n_wrds=0;

	wtk_strbuf_reset(buf);
	for(cur=(trans->lab_queue.pop);cur;cur=cur->next)
	{
		ll=data_offset(cur,wtk_lablist_t,trans_n);
		if(j>0)
		{
			wtk_strbuf_push_s(buf,"|");
		}
		++j;
		for(cur2=(ll->lable_queue.pop);cur2;cur2=cur2->next)
		{
			lab=data_offset(cur2,wtk_lab_t,lablist_n);
			if(ll->max_aux_lab>=2)
			{
				if(lab->aux_lab[2])
				{
					id=lab->aux_lab[2];
					if(!wtk_string_equal_s(id,"sil") && !wtk_string_equal_s(id,"<s>") && !wtk_string_equal_s(id,"</s>")
							&& !wtk_string_equal_s(id, "<eps>"))
					{
						if(i>0)
						{
							wtk_strbuf_push_c(buf,sep);
						}
						wtk_strbuf_push(buf,id->data,id->len);
						++i;
					}
				}
			}else
			{
				id=lab->name;
				if(wtk_string_equal_s(id,"sil") || wtk_string_equal_s(id,"<s>") || wtk_string_equal_s(id,"</s>"))
				{
					continue;
				}
				if(i>0)
				{
					wtk_strbuf_push_c(buf,sep);
				}
				wtk_strbuf_push(buf,lab->name->data,lab->name->len);
				++i;
			}
		}
	}
	n_wrds = i;
	return n_wrds;
}

/*don't consider sil*/
wtk_array_t* wtk_transcription_getrec(wtk_transcription_t* trans,wtk_heap_t *heap)
{
	wtk_queue_node_t *cur,*cur2;
	wtk_lablist_t *ll;
	wtk_lab_t *lab;
	wtk_string_t *id;
//	int i=0,j=0;
//	int n_wrds=0;
	wtk_array_t *wrds;

	wrds = wtk_array_new_h(heap,100,sizeof(wtk_string_t*));
	for(cur=(trans->lab_queue.pop);cur;cur=cur->next)
	{
		ll=data_offset(cur,wtk_lablist_t,trans_n);
//		++j;
		for(cur2=(ll->lable_queue.pop);cur2;cur2=cur2->next)
		{
			lab=data_offset(cur2,wtk_lab_t,lablist_n);
			if(ll->max_aux_lab>=2)
			{
				if(lab->aux_lab[2])
				{
					id=lab->aux_lab[2];
					if(!wtk_string_equal_s(id,"sil") && !wtk_string_equal_s(id,"<s>") && !wtk_string_equal_s(id,"</s>")
							&& !wtk_string_equal_s(id, "<eps>"))
					{
						((wtk_string_t **)wtk_array_push(wrds))[0]=id;
//						++i;
					}
				}
			}
		}
	}
//	n_wrds = i;

	return wrds;
}

void wtk_transcription_print2(wtk_transcription_t* trans)
{
	wtk_queue_node_t *cur,*cur2;
	wtk_lablist_t *ll;
	wtk_lab_t *lab;
	wtk_string_t *id;

	for(cur=(trans->lab_queue.pop);cur;cur=cur->next)
	{
		ll=data_offset(cur,wtk_lablist_t,trans_n);
		for(cur2=(ll->lable_queue.pop);cur2;cur2=cur2->next)
		{
			lab=data_offset(cur2,wtk_lab_t,lablist_n);
			if(ll->max_aux_lab>=2 && lab->aux_lab[2])
			{
				id=lab->aux_lab[2];
				if(!wtk_string_equal_s(id,"sil"))
				{
					printf("%*.*s ",id->len,id->len,id->data);
				}
			}
		}
	}
	printf("\n");
}

void wtk_transcription_print(wtk_transcription_t* trans)
{
	printf("#!MLF!#\n");
	wtk_transcription_print3(trans,stdout);
}

void wtk_transcription_print4(wtk_transcription_t* trans,char *fn)
{
	FILE *f;

	f=fopen(fn,"w");
	wtk_transcription_print3(trans,f);
	fclose(f);
}

void wtk_transcription_print3(wtk_transcription_t* trans,FILE* file)
{
	wtk_queue_node_t *cur,*cur2;
	wtk_lablist_t *ll;
	wtk_lab_t *lab;
	int j;
	wtk_string_t *id;

	if(!trans){return;}
	for(cur=(trans->lab_queue.pop);cur;cur=cur->next)
	{
		if(cur!=trans->lab_queue.pop)
		{
			fprintf(file,"///\n");
		}
		ll=data_offset(cur,wtk_lablist_t,trans_n);
		//wtk_debug("len=%d\n",ll->lable_queue.length);
		for(cur2=(ll->lable_queue.pop);cur2;cur2=cur2->next)
		{
			lab=data_offset(cur2,wtk_lab_t,lablist_n);
			if(lab->start>=0.0)
			{
				fprintf(file,"%.0f ",lab->start);
				if(lab->end>=0.0)
				{
					fprintf(file,"%.0f ",lab->end);
				}
			}
			fprintf(file,"%*.*s %.5f",lab->name->len,lab->name->len,lab->name->data,lab->score);
			for(j=1;j<=ll->max_aux_lab;++j)
			{
				id=lab->aux_lab[j];
				if(id)
				{
					fprintf(file," %*.*s",id->len,id->len,id->data);
					if(lab->aux_score[j]!=0.0)
					{
						fprintf(file," %.5f",lab->aux_score[j]);
					}
				}
			}
			fprintf(file,"\n");
		}
	}
	fprintf(file,".\n");
}

void wtk_transcription_print_hresults(wtk_transcription_t* trans,FILE* file)
{
	wtk_queue_node_t *cur,*cur2;
	wtk_lablist_t *ll;
	wtk_lab_t *lab;
	int j;

	if(!trans){return;}
	for(cur=(trans->lab_queue.pop);cur;cur=cur->next)
	{
		if(cur!=trans->lab_queue.pop)
		{
			fprintf(file,"///\n");
		}
		ll=data_offset(cur,wtk_lablist_t,trans_n);
		//wtk_debug("%p\n",ll->lable_queue.pop);
		for(cur2=(ll->lable_queue.pop);cur2;cur2=cur2->next)
		{
			lab=data_offset(cur2,wtk_lab_t,lablist_n);
			if(wtk_string_cmp_s(lab->name,"<s>")==0
					|| wtk_string_cmp_s(lab->name,"</s>")==0
					||wtk_string_cmp_s(lab->name,"sil")==0)
			{

			}else
			{
				if(lab->name->data[0]>0)
				{
					fprintf(file,"%*.*s",lab->name->len,lab->name->len,lab->name->data);
					fprintf(file,"\n");
				}else
				{
					for(j=0;j<lab->name->len;j+=2)
					{
						fprintf(file,"%*.*s",2,2,lab->name->data+j);
						fprintf(file,"\n");
					}
				}
			}
		}
	}
	fprintf(file,".\n");
}

int wtk_transcription_load(wtk_array_t *array, wtk_source_t *sl)
{
	wtk_heap_t *h=array->heap;
	wtk_strbuf_t *buf, *bufstr;
	wtk_transcription_t *tran=0;
	int eof=0, headtag=0;
	int ret=0;

	buf=wtk_strbuf_new(256,1);
	bufstr=wtk_strbuf_new(256,1);
	wtk_strbuf_reset(bufstr);
	while(1){
		ret=wtk_source_skip_sp(sl,NULL);
		if(ret!=0){goto end;}
		ret=wtk_source_read_line2(sl,buf,&eof);
		if(ret!=0){goto end;}
		if(eof && buf->pos==0)
		{
			if (bufstr->pos >0){
				wtk_strbuf_push_c(buf,'.');      //support single file(exclude "." end)
			}
			else {
				ret=0;
				goto end;
			}

		}
		wtk_strbuf_strip(buf);
		if(buf->pos==0){continue;}
		//wtk_debug("[%.*s]\n",buf->pos,buf->data);
		if (!headtag){
			if (isdigit(buf->data[0])){
				headtag=1;
				wtk_strbuf_push(bufstr, buf->data, buf->pos);
				wtk_strbuf_push_c(bufstr, '\n');
			}
			continue;
		}

		if (buf->data[0]=='.'){
			tran = wtk_string_to_transcrpiton2(h,bufstr->data, bufstr->pos, 1, 1, 1);
			if(!tran){ret=-1; goto end;}
			//wtk_transcription_print(tran);
			((wtk_transcription_t**)wtk_array_push(array))[0] = tran;
			headtag=0;
			wtk_strbuf_reset(bufstr);
			if(eof){ret=0;goto end;}
		}else{
			wtk_strbuf_push(bufstr, buf->data, buf->pos);
			wtk_strbuf_push_c(bufstr, '\n');
		}
	}
end:
    wtk_strbuf_delete(buf);
	wtk_strbuf_delete(bufstr);
	return ret;
}

int wtk_transcription_load2(wtk_trans_info_t *info, wtk_source_t *sl)
{
	wtk_heap_t *h=info->a->heap;
	wtk_strbuf_t *buf, *bufstr;
	wtk_transcription_t *tran=0;
	int eof=0, headtag=0;
	int ret=0;

	buf=wtk_strbuf_new(256,1);
	bufstr=wtk_strbuf_new(256,1);
	wtk_strbuf_reset(bufstr);
	while(1){
		ret=wtk_source_skip_sp(sl,NULL);
		if(ret!=0){goto end;}
		ret=wtk_source_read_line2(sl,buf,&eof);
		if(ret!=0){goto end;}
		if(eof && buf->pos==0)
		{
			if (bufstr->pos >0){
				wtk_strbuf_push_c(buf,'.');      //support single file(exclude "." end)
			}
			else {
				ret=0;
				goto end;
			}

		}
		wtk_strbuf_strip(buf);
		if(buf->pos==0){continue;}
		//wtk_debug("[%.*s]\n",buf->pos,buf->data);
		if (!headtag){
			if (isdigit(buf->data[0])){
				headtag=1;
				wtk_strbuf_push(bufstr, buf->data, buf->pos);
				wtk_strbuf_push_c(bufstr, '\n');
			}
			continue;
		}

		if (buf->data[0]=='.'){
			tran = wtk_string_to_transcrpiton(h,bufstr->data, bufstr->pos, info->trace_state, info->trace_model, info->trace_word);
			if(!tran){ret=-1; goto end;}
			//wtk_transcription_print(tran);
			((wtk_transcription_t**)wtk_array_push(info->a))[0] = tran;
			headtag=0;
			wtk_strbuf_reset(bufstr);
			if(eof){ret=0;goto end;}
		}else{
			wtk_strbuf_push(bufstr, buf->data, buf->pos);
			wtk_strbuf_push_c(bufstr, '\n');
		}
	}
end:
    wtk_strbuf_delete(buf);
	wtk_strbuf_delete(bufstr);
	return ret;
}

wtk_array_t* wtk_transcription_load_file(char* fn, wtk_heap_t* heap, int n)
{
	wtk_array_t *array;

	array = wtk_array_new_h(heap, n, sizeof(wtk_transcription_t*));
	wtk_source_load_file(array, (wtk_source_load_handler_t)wtk_transcription_load, fn);

	return array;
}

wtk_trans_info_t* wtk_transcription_load_file2(char* fn, wtk_heap_t* heap, int n, int state, int model, int word)
{
	wtk_trans_info_t *info;
	info=(wtk_trans_info_t*)wtk_heap_zalloc(heap, sizeof(*info));
	info->a = wtk_array_new_h(heap, n, sizeof(wtk_transcription_t*));
	info->trace_state=state;
	info->trace_model=model;
	info->trace_word=word;
	wtk_source_load_file(info, (wtk_source_load_handler_t)wtk_transcription_load2, fn);

	return info;
}

int wtk_num_string_split(wtk_string_t* arr, char*data, int len)
{
	char *s, *e, *cur;
	int i=0;

	s=data;
	e=s+len-1;
	while(s <= e && isspace(*s))s++;
	while(e >= s && isspace(*e))e--;
	if (e-s<=0)goto end;
	cur=s;
	while(cur<=e){
		if (isspace(*cur)){
			arr[i].data = s;
			arr[i].len = cur-s;
			i++;
			s=cur+1;
		}
		cur++;
	}
	arr[i].data = s;
	arr[i].len = cur-s;
	//end tag
	i++;
	arr[i].data=0;
	arr[i].len = 0;

end:
	return i;
}

//"*/post_r_1000.lab"
//0 1800000 s2 -1554.722290 sil -1656.456665 sil
//1800000 1900000 s4 -101.734383
//1900000 2000000 s2 -114.107727 b -677.015198 把
//2000000 2100000 s3 -113.614441
//2100000 2500000 s4 -449.293060
//2500000 2800000 s2 -308.662292 aa -2100.570312
//2800000 4100000 s3 -1129.198975
//4100000 4700000 s4 -662.709167
//4700000 6700000 s2 -2224.627197 ea -3106.677734 恶
//6700000 7100000 s3 -435.573334
//7100000 7500000 s4 -446.477142
//7500000 7700000 s2 -246.113129 n -615.177246 鸟
//7700000 7800000 s3 -126.759506
//7800000 8000000 s4 -242.304581
//8000000 8200000 s2 -227.090469 il -565.961182
//8200000 8300000 s3 -115.139336
//8300000 8500000 s4 -223.731384
//8500000 8800000 s2 -312.836517 ah -1053.932007
//8800000 9100000 s3 -293.402496
//9100000 9600000 s4 -447.692963
//9600000 9800000 s2 -206.024673 ub -423.344025
//9800000 9900000 s3 -107.162445
//9900000 10000000 s4 -110.156914
//.
wtk_transcription_t* wtk_string_to_transcrpiton(wtk_heap_t* heap, char* data, int len, int trace_model,int trace_state,int trace_word)
{
#define NUM_TRAN_MAX_COL 16
	wtk_transcription_t * tran;
	char *s, *e, *cur;
	wtk_string_t arr[NUM_TRAN_MAX_COL];
	wtk_lablist_t *ll;
	wtk_lab_t *lab;
	wtk_string_t *word, *model, str;
	double lm, modlk=0;
	wtk_queue_node_t *qn;
	int i,  naux, num, numfix=0;

	naux=trace_model+trace_word;
	if(naux>0)
	{
		naux=2;     //model, word
	}
	tran=wtk_transcription_new_h(heap);
	s=cur=data;
	e=s+len;
	word=0;
	ll=0;qn=0;
	while(cur < e){
		if(*cur!='\n'){
			cur++;
			continue;
		}
		// lablist
		str.data=s;
		str.len=cur-s;
		if (0==wtk_string_cmp(&str, "///", strlen("///"))){
			ll=wtk_lablist_new_h(heap, naux);
			wtk_queue_push(&(tran->lab_queue),&(ll->trans_n));
			cur++;
			s = cur;
			continue;
		}else if (!ll){
			ll=wtk_lablist_new_h(heap, naux);
			wtk_queue_push(&(tran->lab_queue),&(ll->trans_n));
		}
		num = wtk_num_string_split(arr, s, cur-s);
		if (num < 3){
			wtk_debug("ERROR: num=%d column (%.*s)\n", num, (int)(cur-s),s);
			return 0;
		}
		if (numfix==0)numfix=num;
		lab = wtk_lab_new_h(heap, ll->max_aux_lab);
		i=0;
		lab->start = wtk_str_atof(arr[i].data, arr[i].len);
		i++;
		lab->end = wtk_str_atof(arr[i].data, arr[i].len);
		i++;
		lab->name = wtk_heap_dup_string(heap, arr[i].data, arr[i].len);
		i++;
		lab->score = wtk_str_atof(arr[i].data, arr[i].len);
		if (numfix==num){                //full content: word,state etc.
			model=0;modlk=0.0;lm=0.0;
			qn=0;
			if (i+1<num && trace_model){
				i++;
				model=wtk_heap_dup_string(heap, arr[i].data, arr[i].len);
				if(i+1<num){
					i++;
					modlk=wtk_str_atof(arr[i].data, arr[i].len);
				}
				lab->aux_lab[1]=model;
				lab->aux_score[1]=modlk;
			}
			if (i+1<num && trace_word){
				i++;
				word = wtk_heap_dup_string(heap, arr[i].data, arr[i].len);
				if(i+1<num){
					i++;
					lm = wtk_str_atof(arr[i].data, arr[i].len);
				}
				lab->aux_lab[naux]=word;
				lab->aux_score[naux]=lm;
			}
		}else if(num > 4 && num < numfix){
			if (i+1<num && trace_model){
				i++;
				model=wtk_heap_dup_string(heap, arr[i].data, arr[i].len);
				if(i+1<num){
					i++;
					modlk=wtk_str_atof(arr[i].data, arr[i].len);
				}
				lab->aux_lab[1]=model;
				lab->aux_score[1]=modlk;
			}
		}

		if(!qn)
		{
			wtk_queue_push(&(ll->lable_queue),&(lab->lablist_n));
			qn=&(lab->lablist_n);
		}else
		{
			wtk_queue_insert_to(&(ll->lable_queue),qn,&(lab->lablist_n));
			qn=&(lab->lablist_n);
		}
//		if (numfix==num){
//			wtk_queue_push(&(tran->lab_queue),&(ll->trans_n));
//		}
		cur++;
		s = cur;
	}
	return tran;
}

// 0 100000 nil^nil-pau+b=aa@1-1&nil-nil/A:nil_nil_nil/B:nil_1@1_1&nil#nil$nil/C:nil-2-3/D:nil_nil/E:nil_1!1_11/F:v_1/G:15-11@0/H:5&5@5@5&5!5/I:nil+nil&nil+nil!nil!nil#nil[2] nil^nil-p    au+b=aa@1-1&nil-nil/A:nil_nil_nil/B:nil_1@1_1&nil#nil$nil/C:nil-2-3/D:nil_nil/E:nil_1!1_11/F:v_1/G:15-11@0/H:5&5@5@5&5!5/I:nil+nil&nil+nil!nil!nil#nil
// 100000 150000 nil^nil-pau+b=aa@1-1&nil-nil/A:nil_nil_nil/B:nil_1@1_1&nil#nil$nil/C:nil-2-3/D:nil_nil/E:nil_1!1_11/F:v_1/G:15-11@0/H:5&5@5@5&5!5/I:nil+nil&nil+nil!nil!nil#nil[3]
// 150000 1850000 nil^nil-pau+b=aa@1-1&nil-nil/A:nil_nil_nil/B:nil_1@1_1&nil#nil$nil/C:nil-2-3/D:nil_nil/E:nil_1!1_11/F:v_1/G:15-11@0/H:5&5@5@5&5!5/I:nil+nil&nil+nil!nil!nil#nil[4]
// 1850000 1900000 nil^nil-pau+b=aa@1-1&nil-nil/A:nil_nil_nil/B:nil_1@1_1&nil#nil$nil/C:nil-2-3/D:nil_nil/E:nil_1!1_11/F:v_1/G:15-11@0/H:5&5@5@5&5!5/I:nil+nil&nil+nil!nil!nil#nil[5]
// 1900000 2000000 nil^nil-pau+b=aa@1-1&nil-nil/A:nil_nil_nil/B:nil_1@1_1&nil#nil$nil/C:nil-2-3/D:nil_nil/E:nil_1!1_11/F:v_1/G:15-11@0/H:5&5@5@5&5!5/I:nil+nil&nil+nil!nil!nil#nil[6]
// 2000000 2050000 nil^pau-b+aa=ea@1-2&0-0/A:nil_1_nil/B:nil_2@1_1&3#0$0/C:nil-1-4/D:nil_1/E:v_1!2_10/F:a_1/G:15-11@0/H:5&p@5@1&5!1/I:H+A&nil+H!nil!H#nil[2] nil^pau-b+aa=ea@1-2&0-0/A:n    il_1_nil/B:nil_2@1_1&3#0$0/C:nil-1-4/D:nil_1/E:v_1!2_10/F:a_1/G:15-11@0/H:5&p@5@1&5!1/I:H+A&nil+H!nil!H#nil
// 2050000 2250000 nil^pau-b+aa=ea@1-2&0-0/A:nil_1_nil/B:nil_2@1_1&3#0$0/C:nil-1-4/D:nil_1/E:v_1!2_10/F:a_1/G:15-11@0/H:5&p@5@1&5!1/I:H+A&nil+H!nil!H#nil[3]
// 2250000 2350000 nil^pau-b+aa=ea@1-2&0-0/A:nil_1_nil/B:nil_2@1_1&3#0$0/C:nil-1-4/D:nil_1/E:v_1!2_10/F:a_1/G:15-11@0/H:5&p@5@1&5!1/I:H+A&nil+H!nil!H#nil[4]
// 2350000 2450000 nil^pau-b+aa=ea@1-2&0-0/A:nil_1_nil/B:nil_2@1_1&3#0$0/C:nil-1-4/D:nil_1/E:v_1!2_10/F:a_1/G:15-11@0/H:5&p@5@1&5!1/I:H+A&nil+H!nil!H#nil[5]
// 2450000 2500000 nil^pau-b+aa=ea@1-2&0-0/A:nil_1_nil/B:nil_2@1_1&3#0$0/C:nil-1-4/D:nil_1/E:v_1!2_10/F:a_1/G:15-11@0/H:5&p@5@1&5!1/I:H+A&nil+H!nil!H#nil[6]
// 2500000 2600000 pau^b-aa+ea=n@2-1&1-1/A:nil_1_nil/B:nil_2@1_1&3#0$0/C:nil-1-4/D:nil_1/E:v_1!2_10/F:a_1/G:15-11@0/H:p&1@5@1&5!1/I:T+A&nil+H!nil!H#nil[2] pau^b-aa+ea=n@2-1&1-1/A:nil_1    _nil/B:nil_2@1_1&3#0$0/C:nil-1-4/D:nil_1/E:v_1!2_10/F:a_1/G:15-11@0/H:p&1@5@1&5!1/I:T+A&nil+H!nil!H#nil

wtk_transcription_t* wtk_string_to_transcrpiton2(wtk_heap_t* heap, char* data, int len, int trace_model,int trace_state,int trace_word)
{
#define NUM_TRAN_MAX_COL 16
	wtk_transcription_t * tran;
	char *s, *e, *cur;
	wtk_string_t arr[NUM_TRAN_MAX_COL*16];
	wtk_lablist_t *ll;
	wtk_lab_t *lab;
	wtk_string_t *word, *model, str;
	//double lm, modlk;
	wtk_queue_node_t *qn;
	int i,  naux, num, numfix=0;

	naux=trace_model+trace_state;
	if(naux>0)
	{
		naux=2;     //model, state
	}
	tran=wtk_transcription_new_h(heap);
	s=cur=data;
	e=s+len;
	word=0;
	ll=0;qn=0;
	while(cur < e){
		if(*cur!='\n'){
			cur++;
			continue;
		}
		// lablist
		str.data=s;
		str.len=cur-s;
		if (0==wtk_string_cmp(&str, "///", strlen("///"))){
			ll=wtk_lablist_new_h(heap, naux);
			wtk_queue_push(&(tran->lab_queue),&(ll->trans_n));
			cur++;
			s = cur;
			continue;
		}else if (!ll){
			ll=wtk_lablist_new_h(heap, naux);
			wtk_queue_push(&(tran->lab_queue),&(ll->trans_n));
		}
		num = wtk_num_string_split(arr, s, cur-s);
		if (num < 3){
			wtk_debug("ERROR: num=%d column (%.*s)\n", num, (int)(cur-s),s);
			return 0;
		}
		if (numfix==0)numfix=num;
		lab = wtk_lab_new_h(heap, ll->max_aux_lab);
		i=0;
		lab->start = wtk_str_atof(arr[i].data, arr[i].len);
		i++;
		lab->end = wtk_str_atof(arr[i].data, arr[i].len);
		i++;
		lab->name = wtk_heap_dup_string(heap, arr[i].data, arr[i].len);
		//i++;
		//lab->score = wtk_str_atof(arr[i].data, arr[i].len);
		if (numfix==num){                //full content: model,state etc.
			model=0;//modlk=0.0;lm=0.0;
			qn=0;
			if (i+1<num && trace_model){
				i++;
				model=wtk_heap_dup_string(heap, arr[i].data, arr[i].len);
//				if(i+1<num){
//					i++;
//					modlk=wtk_str_atof(arr[i].data, arr[i].len);
//				}
				lab->aux_lab[1]=model;
				//lab->aux_score[1]=modlk;
			}
			if (i+1<num){
				i++;
				word = wtk_heap_dup_string(heap, arr[i].data, arr[i].len);
//				if(i+1<num){
//					i++;
//					lm = wtk_str_atof(arr[i].data, arr[i].len);
//				}
				lab->aux_lab[naux]=word;
				//lab->aux_score[naux]=lm;
			}
		}else if(num > 4 && num < numfix){
			if (i+1<num && trace_model){
				i++;
				model=wtk_heap_dup_string(heap, arr[i].data, arr[i].len);
//				if(i+1<num){
//					i++;
//					modlk=wtk_str_atof(arr[i].data, arr[i].len);
//				}
				lab->aux_lab[1]=model;
//				lab->aux_score[1]=modlk;
			}
		}

		if(!qn)
		{
			wtk_queue_push(&(ll->lable_queue),&(lab->lablist_n));
			qn=&(lab->lablist_n);
		}else
		{
			wtk_queue_insert_to(&(ll->lable_queue),qn,&(lab->lablist_n));
			qn=&(lab->lablist_n);
		}
		cur++;
		s = cur;
	}
	return tran;
}

void wtk_nbestentry_print2(wtk_nbestentry_t *n)
{
	if(n->path_prev)
	{
		wtk_nbestentry_print2(n->path_prev);
		printf(" ");
	}
	if(n->lnode && n->lnode->info.word)
	{
		printf("%.*s",n->lnode->info.word->name->len,n->lnode->info.word->name->data);
	}else
	{
		printf("!NULL");
	}
}

void wtk_nbestentry_print(wtk_nbestentry_t *n)
{
	wtk_nbestentry_print2(n);
	printf("\n");
}
