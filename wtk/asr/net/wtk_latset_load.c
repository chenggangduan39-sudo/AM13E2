#include "wtk_latset.h"
#include <ctype.h>

typedef struct
{
	wtk_strbuf_t *buf;
	char del;
	unsigned char is_nl:1;
	unsigned char is_eof:1;
}wtk_lattoken_t;

void wtk_lattoken_print(wtk_lattoken_t* t)
{
	printf("name:\t%*.*s\n",t->buf->pos,t->buf->pos,t->buf->data);
	printf("del:\t%c\n",t->del);
	printf("newline:\t%d\n",t->is_nl);
}

int wtk_lattoken_next(wtk_lattoken_t *tok,wtk_source_t *s)
{
	wtk_strbuf_t *b=tok->buf;
	int c;
	int ret;

	ret=-1;tok->is_nl=0;
	wtk_strbuf_reset(b);
	do
	{
		c=wtk_source_get(s);
	}while(isspace(c) && c!='\n' && c!=EOF);
	//wtk_debug("%d,%d\n",c,EOF);
	if(c==EOF){tok->is_eof=1;goto end;}
	if(c=='\n'){tok->is_nl=1;ret=0;goto end;}
	if(!isalnum(c) && c!='.')
	{
		if(c!='#'){goto end;}
		//skip comment.
		while(c!='\n' && c!=EOF){c=wtk_source_get(s);}
		ret=wtk_lattoken_next(tok,s);
		goto end;
	}
	if(c=='.')
	{
		wtk_strbuf_push(b,(char*)&c,1);
		ret=0;goto end;
	}
	while(isalnum(c))
	{
		wtk_strbuf_push(b,(char*)&c,1);
		c=wtk_source_get(s);
		if(c==EOF){goto end;}
	}
	if(c!='=' && c!='~'){goto end;}
	tok->del=c;ret=0;
end:
	return ret;
}

int wtk_lattoken_get_int(wtk_lattoken_t *tok,wtk_source_t *s,int *v)
{
	int ret;

	if(tok->del=='=')
	{
		ret=wtk_source_read_string(s,tok->buf);
		if(ret!=0){goto end;}
		*v=wtk_str_atoi(tok->buf->data,tok->buf->pos);
	}else
	{
		ret=wtk_source_read_int(s,v,1,1);
	}
end:
	return ret;
}

int wtk_lattoken_get_float(wtk_lattoken_t *tok,wtk_source_t *s,float *v)
{
	return wtk_source_read_float(s,v,1,tok->del!='=');
}

int wtk_latset_load_lathdr(wtk_latset_t *ls,wtk_lat_t *lat,wtk_source_t *s,wtk_lattoken_t *tok)
{
	wtk_strbuf_t *b=tok->buf;
	int n,ret,nodes,arcs;
	char *p;

	nodes=arcs=0;
	while(1)
	{
		ret=wtk_source_skip_sp(s,0);
		if(ret!=0){goto end;}
		ret=wtk_lattoken_next(tok,s);
		if(ret!=0){wtk_debug("read lattice header name field failed.\n");goto end;}
		if(b->pos==1)
		{
			switch(b->data[0])
			{
			case 'N':
				ret=wtk_lattoken_get_int(tok,s,&nodes);
				break;
			case 'L':
				ret=wtk_lattoken_get_int(tok,s,&arcs);
				break;
			default:
				ret=-1;
				break;
			}
		}else
		{
			p=b->data;n=b->pos;
			if(wtk_str_equal_s(p,n,"SUBLAT"))
			{
				if(lat->name){ret=-1;goto end;}
				ret=wtk_source_read_string(s,b);
				if(ret!=0){goto end;}
				lat->name=wtk_label_find(ls->label,b->data,b->pos,1)->name;
			}else if(wtk_str_equal_s(p,n,"wdpenalty"))
			{
				ret=wtk_lattoken_get_float(tok,s,&(lat->wdpenalty));
			}else if(wtk_str_equal_s(p,n,"lmscale"))
			{
				ret=wtk_lattoken_get_float(tok,s,&(lat->lmscale));
			}else if(wtk_str_equal_s(p,n,"prscale"))
			{
				ret=wtk_lattoken_get_float(tok,s,&(lat->prscale));
			}else if(wtk_str_equal_s(p,n,"acscale"))
			{
				ret=wtk_lattoken_get_float(tok,s,&(lat->acscale));
			}else if(wtk_str_equal_s(p,n,"base"))
			{
				ret=wtk_lattoken_get_float(tok,s,&(lat->logbase));
			}else if(wtk_str_equal_s(p,n,"tscale"))
			{
				ret=wtk_lattoken_get_float(tok,s,&(lat->tscale));
			}else
			{
				//wtk_debug("unhandled lattice field(%*.*s).\n",n,n,p);
				ret=wtk_source_read_string(s,b);
			}
		}
		if(ret!=0){goto end;}
		if(nodes!=0 && arcs!=0)
		{
			//wtk_debug("%d-%d\n",nodes,arcs);
			wtk_lat_create(lat,nodes,arcs);
			break;
		}
	}
end:
	return ret;
}

/*
 *read node: node=N"="short [L|W]"="string
 */
int wtk_latset_load_node(wtk_latset_t *ls,wtk_lat_t *lat,wtk_source_t *s,wtk_lattoken_t *tok)
{
	wtk_strbuf_t *b=tok->buf;
	wtk_lnode_t *node;
	wtk_dict_word_t *w;
	wtk_lat_t *sub;
	int ret,v;
	char type;
	float t;
	float x;

	ret=wtk_lattoken_get_int(tok,s,&v);
	if(ret!=0){goto end;}
	if(v<0 || v>=lat->nn){ret=-1;goto end;}
	node=lat->lnodes+v;
	//wtk_debug("%d\n",v);
	while(1)
	{
		ret=wtk_lattoken_next(tok,s);
		if(ret!=0){wtk_debug("read lattice node name failed.\n");goto end;}
		if(tok->is_nl){break;}
		type=b->pos>0?b->data[0]:0;
		switch(type)
		{
			case 't':
				ret=wtk_source_read_float(s,&t,1,0);
				if(ret!=0){wtk_debug("read t failed.\n");goto end;}
				node->time=t;
				break;
			case 'W':
				ret=wtk_source_read_string(s,b);
				if(ret!=0){wtk_debug("read W failed.\n");goto end;}
				//wtk_debug("[%.*s]\n",b->pos,b->data);
				if(ls->dwf)
				{
					w=ls->dwf(ls->dwf_data,b->data,b->pos);
				}else
				{

					if(strncmp("!NULL",b->data,b->pos)==0)
					{
						w=NULL;
						node->info.word=w;
						node->type=WTK_LNODE_WORD;
						continue;
					}else
					{
						w=(wtk_dict_word_t*)wtk_heap_zalloc(ls->heap,sizeof(*w));
						w->name=wtk_heap_dup_string(ls->heap,b->data,b->pos);
					}
				}
				//w=wtk_dict_find_word(ls->dict,b->data,b->pos);
				//print_data(w->name->data,w->name->len);
				if(!w){wtk_debug("%*.*s not found.\n",b->pos,b->pos,b->data);ret=-1;goto end;}
				node->info.word=w;
				node->type=WTK_LNODE_WORD;
				break;
			case 'L':
				ret=wtk_source_read_string(s,b);
				if(ret!=0){wtk_debug("read L failed.\n");goto end;}
				sub=wtk_latset_find_lat(ls,b->data,b->pos);
				if(!sub)
				{
					wtk_debug("%*.*s sub-lattice not found.\n",b->pos,b->pos,b->data);
					ret=-1;goto end;
				}
				node->info.lat=sub;
				node->type=WTK_LNODE_SUBLAT;
				break;
			default:
				//wtk_debug("%c not implement.\n",type);
				wtk_source_read_float(s,&x,1,0);
				break;
		}
	}
end:
	return ret;
}

int wtk_latset_load_arc(wtk_latset_t *ls,wtk_lat_t *lat,wtk_source_t *s,wtk_lattoken_t *tok)
{
	wtk_strbuf_t *b=tok->buf;
	wtk_larc_t *arc;
	wtk_lnode_t *ns,*ne;
	int ret,v,start,end;
	float lmlike,aclike;
	char type;

	ret=wtk_lattoken_get_int(tok,s,&v);
	if(ret!=0){goto end;}
	if(v<0 || v>=lat->na){ret=-1;goto end;}
	arc=lat->larcs+v;
	start=end=-1;lmlike=aclike=0;
	while(1)
	{
		ret=wtk_lattoken_next(tok,s);
		if(ret!=0){goto end;}
		if(tok->is_nl){break;}
		type=b->pos>0?b->data[0]:0;
		switch(type)
		{
			case 'S':
				ret=wtk_lattoken_get_int(tok,s,&start);
				break;
			case 'E':
				ret=wtk_lattoken_get_int(tok,s,&end);
				break;
			case 'a':
				ret=wtk_lattoken_get_float(tok,s,&aclike);
				break;
			case 'l':
				ret=wtk_lattoken_get_float(tok,s,&lmlike);
				break;
			default:
				wtk_debug("%c not implement.\n",type);
				ret=-1;
				break;
		}
		if(ret!=0){goto end;}
	}
	if(start==-1||end==-1){ret=-1;goto end;}
	ns=lat->lnodes+start;
	ne=lat->lnodes+end;
	arc->start=ns;
	arc->end=ne;
	arc->aclike=aclike;
	arc->lmlike=lmlike;
	arc->farc=ns->foll;
	arc->parc=ne->pred;
	ns->foll=ne->pred=arc;
end:
	return ret;
}

int wtk_latset_load_latinfo(wtk_latset_t *ls,wtk_lat_t *lat,wtk_source_t *s,wtk_lattoken_t *tok)
{
	wtk_strbuf_t *b=tok->buf;
	int ret;
	char type;

	ret=-1;
	while(1)
	{
		ret=wtk_source_skip_sp(s,0);
		if(ret!=0){ret=0;goto end;}
		ret=wtk_lattoken_next(tok,s);
		//wtk_debug("%d,%d\n",tok->is_eof,tok->is_nl);
		if(ret!=0)
		{
			ret=tok->is_eof?0:-1;break;
		}
		if(b->pos<=0){continue;}
		//wtk_lattoken_print(tok);
		type=(b->pos>0)?(b->data[0]):0;
		if(type=='.'){break;}
		switch(type)
		{
			case '\n':
				break;
			case 'I':
				ret=wtk_latset_load_node(ls,lat,s,tok);
				break;
			case 'J':
				ret=wtk_latset_load_arc(ls,lat,s,tok);
				break;
			default:
				wtk_debug("\"%c\"(%d) not implement.\n",type,type);
				ret=-1;break;
		}
		if(ret!=0){break;}
	}
end:
	return ret;
}


int wtk_latset_load_lat(wtk_latset_t *ls,wtk_lat_t* lat,wtk_source_t *s,wtk_lattoken_t *tok)
{
	int ret;

	ret=wtk_latset_load_lathdr(ls,lat,s,tok);
	if(ret!=0){goto end;}
	ret=wtk_latset_load_latinfo(ls,lat,s,tok);
end:
	return ret;
}

int wtk_latset_load_file(wtk_latset_t *ls,char *fn)
{
	return wtk_source_load_file(ls,(wtk_source_load_handler_t)wtk_latset_load,fn);
}

int wtk_latset_load(wtk_latset_t *ls,wtk_source_t *s)
{
	wtk_lattoken_t tok;
	wtk_lat_t *lat;
	int ret;

	tok.buf=wtk_strbuf_new(64,1);
	tok.is_eof=tok.is_nl=0;
	while(1)
	{
		lat=wtk_latset_new_lat(ls);
		ret=wtk_latset_load_lat(ls,lat,s,&tok);
		if(ret!=0){goto end;}
		//wtk_lat_print(lat);
		if(!lat->name){ls->main=lat;}
		if(tok.is_eof){break;}
	}
end:
	if(ret!=0 && lat)
	{
		wtk_lat_clean(lat);
	}
	wtk_strbuf_delete(tok.buf);
	return ret;
}

