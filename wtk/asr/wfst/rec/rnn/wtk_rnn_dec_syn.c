#include "wtk_rnn_dec_syn.h" 
#include "wtk/core/cfg/wtk_source.h"

wtk_rnn_dec_fsyn_t* wtk_rnn_dec_fsyn_new(int hid_size,int voc_size)
{
	wtk_rnn_dec_fsyn_t *syn;

	syn=(wtk_rnn_dec_fsyn_t*)wtk_malloc(sizeof(wtk_rnn_dec_fsyn_t));
	syn->hid_size=hid_size;
	syn->voc_size=voc_size;
	syn->input_wrd=wtk_matf_new(voc_size,hid_size);
	syn->input_hid=wtk_matf_new(hid_size,hid_size);
	syn->output_wrd=wtk_matf_new(voc_size,hid_size);
	return syn;
}

void wtk_rnn_dec_fsyn_delete(wtk_rnn_dec_fsyn_t *syn)
{
	wtk_matf_delete(syn->input_wrd);
	wtk_matf_delete(syn->input_hid);
	wtk_matf_delete(syn->output_wrd);
	wtk_free(syn);
}

static int wtk_matf_load(wtk_matf_t *m,wtk_source_t *src,wtk_strbuf_t *buf)
{
	int nl;
	int ret;

	wtk_source_skip_sp(src,&nl);
	wtk_source_read_line(src,buf);
	ret=wtk_source_read_float(src,m->p,m->row*m->col,0);
	return  ret;
}

int wtk_rnn_dec_syn_load_fsyn(wtk_rnn_dec_syn_t *syn,wtk_source_t *src)
{
	wtk_strbuf_t *buf;
	int ret;
	int voc_size;
	int hid_size;
	int v;
	int nl;

	buf=wtk_strbuf_new(256,1);
	hid_size=voc_size=0;
	//load info;
	while(1)
	{
		ret=wtk_source_read_string(src,buf);
		if(ret!=0)
		{
			wtk_debug("read info failed\n");
			goto end;
		}
		ret=wtk_source_read_int(src,&v,1,0);
		if(ret!=0)
		{
			wtk_debug("read info value failed\n");
			goto end;
		}
		//wtk_debug("[%.*s]=%d\n",buf->pos,buf->data,v);
		if(wtk_str_equal_s(buf->data,buf->pos,"voc:"))
		{
			voc_size=v;
		}else if(wtk_str_equal_s(buf->data,buf->pos,"hid:"))
		{
			hid_size=v;
		}else
		{
			//wtk_debug("[%.*s]=%d\n",buf->pos,buf->data,v);
		}
		wtk_source_skip_sp(src,&nl);
		if(nl>=2)
		{
			break;
		}
	}
	syn->fsyn=wtk_rnn_dec_fsyn_new(hid_size,voc_size);
	//read input wrd matrix;
	ret=wtk_matf_load(syn->fsyn->input_wrd,src,buf);
	if(ret!=0){goto end;}

	ret=wtk_matf_load(syn->fsyn->input_hid,src,buf);
	if(ret!=0){goto end;}

	ret=wtk_matf_load(syn->fsyn->output_wrd,src,buf);
	if(ret!=0){goto end;}

	ret=0;
end:
	wtk_strbuf_delete(buf);
	return ret;
}

wtk_rnn_dec_fix_syn_t* wtk_rnn_dec_fix_syn_new(int hid_size,int voc_size)
{
	wtk_rnn_dec_fix_syn_t *syn;

	syn=(wtk_rnn_dec_fix_syn_t*)wtk_malloc(sizeof(wtk_rnn_dec_fix_syn_t));
	syn->hid_size=hid_size;
	syn->voc_size=voc_size;
	syn->input_wrd=wtk_matb_new(voc_size,hid_size);
	syn->input_hid=wtk_matb_new(hid_size,hid_size);
	syn->output_wrd=wtk_matb_new(voc_size,hid_size);
	return syn;
}

void wtk_rnn_dec_fix_syn_delete(wtk_rnn_dec_fix_syn_t *syn)
{
	wtk_matb_delete(syn->input_hid);
	wtk_matb_delete(syn->input_wrd);
	wtk_matb_delete(syn->output_wrd);
	wtk_free(syn);
}

wtk_rnn_dec_fix_syn_t* wtk_rnn_dec_fsyn_to_fix(wtk_rnn_dec_fsyn_t *syn)
{
	wtk_rnn_dec_fix_syn_t *fix;

	fix=wtk_rnn_dec_fix_syn_new(syn->hid_size,syn->voc_size);
	wtk_matb_fix(fix->input_wrd,syn->input_wrd);
	wtk_matb_fix(fix->input_hid,syn->input_hid);
	wtk_matb_fix(fix->output_wrd,syn->output_wrd);
	//exit(0);
	return fix;
}

void wtk_matf_write_bin(wtk_matf_t *m,FILE *f)
{
	fwrite(m->p,m->row*m->col*sizeof(float),1,f);
}

void wtk_matb_write_bin(wtk_matb_t *m,FILE *f)
{
	fwrite(&(m->scale),sizeof(float),1,f);
	fwrite(m->p,m->row*m->col,1,f);
}

void wtk_rnn_dec_fsyn_write(wtk_rnn_dec_fsyn_t *syn,char *fn)
{
	FILE *f;

	wtk_debug("write bin %s\n",fn);
	f=fopen(fn,"wb");
	wtk_matf_write_bin(syn->input_wrd,f);
	wtk_matf_write_bin(syn->input_hid,f);
	wtk_matf_write_bin(syn->output_wrd,f);
	fclose(f);
}

void wtk_rnn_dec_fix_syn_write(wtk_rnn_dec_fix_syn_t *syn,char *fn)
{
	FILE *f;

	wtk_debug("write bin %s\n",fn);
	f=fopen(fn,"wb");
	fwrite(&(syn->voc_size),sizeof(int),1,f);
	fwrite(&(syn->hid_size),sizeof(int),1,f);
	wtk_matb_write_bin(syn->input_wrd,f);
	wtk_matb_write_bin(syn->input_hid,f);
	wtk_matb_write_bin(syn->output_wrd,f);
}

static int wtk_matb_load_bin(wtk_matb_t *m,wtk_source_t *src)
{
	wtk_source_fill(src,(char*)(&(m->scale)),sizeof(float));
	return  wtk_source_fill(src,(char*)(m->p),m->row*m->col);
}

int wtk_rnn_dec_syn_load_fix_fsyn(wtk_rnn_dec_syn_t *syn,wtk_source_t *src)
{
	int v[2];
	int ret;

	src->swap=0;
	//wtk_debug("in\n");
	ret=wtk_source_read_int(src,v,2,1);
	if(ret!=0)
	{
		wtk_debug("read int value failed\n");
		goto end;
	}
	//wtk_debug("[%d/%d]\n",v[1],v[0]);
	syn->fix=wtk_rnn_dec_fix_syn_new(v[1],v[0]);
	ret=wtk_matb_load_bin(syn->fix->input_wrd,src);
	if(ret!=0)
	{
		wtk_debug("load input word failed.\n");
		goto end;
	}
	ret=wtk_matb_load_bin(syn->fix->input_hid,src);
	if(ret!=0)
	{
		wtk_debug("load input hid failed.\n");
		goto end;
	}
	ret=wtk_matb_load_bin(syn->fix->output_wrd,src);
	if(ret!=0)
	{
		wtk_debug("load output word failed.\n");
		goto end;
	}
	ret=0;
end:
	return ret;
}

void wtk_rnn_dec_syn_write_fix(wtk_rnn_dec_syn_t *syn,char *fn)
{
	syn->fix=wtk_rnn_dec_fsyn_to_fix(syn->fsyn);
	wtk_rnn_dec_fix_syn_write(syn->fix,fn);
}

wtk_rnn_dec_syn_t* wtk_rnn_dec_syn_new(int use_fix,char *fn,wtk_source_loader_t *sl)
{
	wtk_rnn_dec_syn_t *syn;

	syn=(wtk_rnn_dec_syn_t*)wtk_malloc(sizeof(wtk_rnn_dec_syn_t));
	syn->fsyn=NULL;
	syn->fix=NULL;
	if(sl)
	{
		if(use_fix)
		{
			wtk_source_loader_load(sl,syn,(wtk_source_load_handler_t)wtk_rnn_dec_syn_load_fix_fsyn,fn);
		}else
		{
			wtk_source_loader_load(sl,syn,(wtk_source_load_handler_t)wtk_rnn_dec_syn_load_fsyn,fn);
		}
	}else
	{
		if(use_fix)
		{
			wtk_source_load_file(syn,(wtk_source_load_handler_t)wtk_rnn_dec_syn_load_fix_fsyn,fn);
		}else
		{
			wtk_source_load_file(syn,(wtk_source_load_handler_t)wtk_rnn_dec_syn_load_fsyn,fn);
			//wtk_rnn_dec_fsyn_write(syn->fsyn,"syn.bin");
			if(0)
			{
				syn->fix=wtk_rnn_dec_fsyn_to_fix(syn->fsyn);
				wtk_rnn_dec_fix_syn_write(syn->fix,"fix.5.7.bin");
				exit(0);
			}
		}
	}
	//wtk_debug("fix=%p %d\n",syn->fix,use_fix);
	return syn;
}

void wtk_rnn_dec_syn_delete(wtk_rnn_dec_syn_t *syn)
{
	if(syn->fsyn)
	{
		wtk_rnn_dec_fsyn_delete(syn->fsyn);
	}
	if(syn->fix)
	{
		wtk_rnn_dec_fix_syn_delete(syn->fix);
	}
	wtk_free(syn);
}

