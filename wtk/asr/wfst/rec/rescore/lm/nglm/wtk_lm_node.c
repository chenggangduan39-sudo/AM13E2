#include "wtk_lm_node.h"

int wtk_lm2bin_type_bytes(wtk_lm2bin_type_t type)
{
	switch(type)
	{
	case WTK_LM2BIN_a:
		return 12;
		break;
	case WTK_LM2BIN_A:
		return 12;
		break;
	case WTK_LM2BIN_B:
		return 9;
	case WTK_LM2BIN_C:
		return 10;
		break;
	}
	return 12;
}

void wtk_lm2bin_from_bin(wtk_lm2bin_type_t type,wtk_lm_node_t *n,char *data,float ps,float bs)
{
	switch(type)
	{
	case WTK_LM2BIN_a:
		wtk_lm_node_from_bin(n,data,12,ps,bs);
		break;
	case WTK_LM2BIN_A:
		wtk_lm_node_from_bin(n,data,12,ps,bs);
		break;
	case WTK_LM2BIN_B:
		wtk_lm_node_from_bin_small(n,data,9,ps,bs);
		break;
	case WTK_LM2BIN_C:
		wtk_lm_node_from_bin_small2(n,data,10,ps,bs);
		break;
	}
}

void wtk_lm2bin_from_bin_end(wtk_lm2bin_type_t type,wtk_lm_node_t *n,char *data,float ps)
{
	switch(type)
	{
	case WTK_LM2BIN_a:
		wtk_lm_node_from_bin2(n,data,5,ps);
		break;
	case WTK_LM2BIN_A:
		wtk_lm_node_from_bin2(n,data,5,ps);
		break;
	case WTK_LM2BIN_B:
		wtk_lm_node_from_bin_samll_end(n,data,4,ps);
		break;
	case WTK_LM2BIN_C:
		wtk_lm_node_from_bin_samll_end2(n,data,4,ps);
		break;
	}
}



void wtk_lm_node_init(wtk_lm_node_t *node)
{
	node->parent=NULL;
	node->childs=0;
	node->nchild=0;
	node->child_offset=0;
	node->self_offset=0;
	node->id=0;
	node->prob=0;
	node->bow=0;
	node->ngram=0;
}

int wtk_lm_node_cmp(unsigned int *idx,wtk_lm_node_t **node)
{
	return *idx-(*node)->id;
}

wtk_lm_node_t* wtk_lm_node_find_child2(wtk_lm_node_t *node,unsigned int idx)
{
	wtk_lm_node_t **n;

	n=(wtk_lm_node_t**)wtk_binary_search(node->childs,
			(char*)(node->childs)+sizeof(wtk_lm_node_t*)*(node->nchild-1),
			sizeof(wtk_lm_node_t*),
			(wtk_search_cmp_f)wtk_lm_node_cmp,&idx);
	if(n)
	{
		return *n;
	}else
	{
		return NULL;
	}
}

wtk_lm_node_t* wtk_lm_node_find_child3(wtk_lm_node_t *node,unsigned int idx)
{
	wtk_lm_node_t **pn;
	int i;

	pn=node->childs;
	for(i=0;i<node->nchild;++i)
	{
		if(pn[i]->id==idx)
		{
			return pn[i];
		}
	}
	return NULL;
}


wtk_lm_node_t* wtk_lm_node_find_child4(wtk_lm_node_t *node,unsigned int idx)
{
	wtk_lm_node_t **pn;
	int s,e,i;
	int v;
	unsigned int sid,eid;

	pn=node->childs;
	s=0;e=node->nchild-1;
	///wtk_debug("v[%d]: s=%d e=%d idx=%d\n",ki,s,e,idx);
	while(s<=e)
	{
		//wtk_debug("s=%d e=%d\n",s,e);
		if(s<e)
		{
			//i=(s+e)>>1;
			sid=pn[s]->id;
			v=sid-idx;
			if(v>0)
			{
				return NULL;
			}
			eid=pn[e]->id;
			v=eid-idx;
			if(v<0)
			{
				return NULL;
			}
			//i=s+(idx-sid)*(e-s)/(eid-sid);
			i=(s+e)>>1;
			//wtk_debug("idx=%d/%d s=%d,e=%d,i=%d\n",pn[i]->id,idx,s,e,i);
			v=pn[i]->id-idx;
			if(v<0)
			{
				s=i+1;
			}else if(v>0)
			{
				e=i-1;
			}else
			{
				return pn[i];
				break;
			}
		}else
		{
			if(pn[s]->id==idx)
			{
				return pn[s];
			}
			break;
		}
	}
	return NULL;
}

wtk_lm_node_t* wtk_lm_node_find_child_x(wtk_lm_node_t *node,unsigned int idx)
{
	wtk_lm_node_t **s,**e,**m;
	wtk_lm_node_t *p;
	int v1,v2,v3;
	//static int ki=0;

	s=node->childs;
	e=node->childs+node->nchild-1;
	//wtk_debug("[%d,%d]=%d\n",(*s)->id,(*e)->id,idx);
	while(s<e)
	{
		//++ki;
		v1=idx-(*s)->id;
		wtk_debug("v1=%d/%d [%d,%d]\n",v1,idx,(*s)->id,(*e)->id);
		if(v1<=0)
		{
			p=v1==0?(*s):NULL;
			goto end;
		}
		if(e-s>v1)
		{
			e=s+v1;
		}
		v2=(*e)->id-idx;
		wtk_debug("v2=%d\n",v2);
		if(v2<=0)
		{
			p=v2==0?(*e):NULL;
			goto end;
		}
		if(e-s>v2)
		{
			s=e-v2;
		}
		m=s+((e-s)>>1);
		v3=(*m)->id-idx;
		wtk_debug("v3=%d\n",v3);
		if(v3==0)
		{
			p=*m;
			goto end;
		}else if(v3<0)
		{
			s=m+1;
			//--e;
		}else
		{
			e=m-1;
			//++s;
		}
	}
	wtk_debug("s=%p e=%p\n",s,e);
	if(s==e && (*s)->id==idx)
	{
		p=*s;
		goto end;
	}
	p=NULL;
end:
	//wtk_debug("p[%d]=%p:%d\n",ki,p,p->id);
	//exit(0);
	return p;
}


wtk_lm_node_t* wtk_lm_node_find_child(wtk_lm_node_t *node,unsigned int idx)
{
	wtk_lm_node_t **s,**e,**m;
	wtk_lm_node_t *p;
	int v1,v2,v3;

	s=node->childs;
	e=s+node->nchild-1;
	//wtk_debug("[%d,%d]=%d/%d\n",(*s)->id,(*e)->id,idx,node->nchild);
	while(s<e)
	{
		//++ki;
		v1=idx-(*s)->id;
		//wtk_debug("v1=%d/%d [%d,%d]\n",v1,idx,(*s)->id,(*e)->id);
		if(v1<=0)
		{
			p=v1==0?(*s):NULL;
			goto end;
		}
		if(e-s>v1)
		{
			e=s+v1;
		}
		v2=(*e)->id-idx;
		//wtk_debug("v2=%d\n",v2);
		if(v2<=0)
		{
			p=v2==0?(*e):NULL;
			goto end;
		}
		if(e-s>v2)
		{
			s=e-v2;
		}
		if((*s)->id==idx)
		{
			p=*s;
			goto end;
		}
		m=s+((e-s)>>1);
		v3=(*m)->id-idx;
		//wtk_debug("v3=%d s=%p m=%p e=%p\n",v3,s,m,e);
		if(v3==0)
		{
			p=*m;
			goto end;
		}else if(v3<0)
		{
			s=m+1;
			--e;
		}else
		{
			e=m-1;
			++s;
		}
	}
	//wtk_debug("s=%p e=%p\n",s,e);
	if(s==e && (*s)->id==idx)
	{
		p=*s;
		goto end;
	}
	p=NULL;
end:
	return p;
}

/*
          6位有效数字       6位有效数字            5位有效数字     下一位偏移(137438953472) 128G
         id(1048576)      prob(1+20:131072)     bow(1+17:131072)     offset
    96bit = 20bit     +   21bit      +         18bit        +  37bit

  	115798/17bit  +         14bit	+         14bit  +   27bit
  	131072        -99.99 (-7.164591,-0.009600)
  						 (-1.695097,-0.001791)

*/
void wtk_lm_node_to_bin(wtk_lm_node_t *n,wtk_strbuf_t *buf,float ps,float bs)
{
	unsigned int v[3];
	int b=0,i,t;
	float f;

	//wtk_debug("prob=%f %f/%f\n",n->prob,ps,bs);
	//pack id
	v[0]=n->id<<12;
	//pack prob sign
	f=n->prob;
	if(f<-90)
	{
		//-99.99	<s>	-0.4573531
		f=0;
	}else if(f<0)
	{
		b=0;
		f=-f;
	}else
	{
		b=1;
	}
	v[0]+=b<<11;

	//pack prob
	i=(int)(f*ps+0.5);
	//wtk_debug("i=%d\n",i);
	v[0]+=(i>>9)&0x7ff; //11
	t=i&0x1ff; //9
	v[1]=t<<23;

	f=n->bow;
	if(f<0)
	{
		b=0;
		f=-f;
	}else
	{
		b=1;
	}
	v[1]+=b<<22;
	i=(int)(f*bs+0.5);
	v[1]+=(i&0x01ffff)<<5; //   11 1111 1111 1111 1110 0000
	t=n->child_offset>>32;
	v[1]+=t&0x01f;

	v[2]=n->child_offset&0x00ffffffff;

	wtk_strbuf_push(buf,(char*)v,12);
}

void wtk_lm_node_from_bin(wtk_lm_node_t *n,char *data,int len,float ps,float bs)
{
	unsigned int *v;
	unsigned int vi;
	int b;
	uint64_t t;
	float f;

	v=(unsigned int*)data;
	vi=v[0];
	n->id=vi>>12;
	b=vi & 0x00800;
	t=(vi &0x007ff)<<9;

	vi=v[1];
	t+=vi>>23;
	f=t*ps;
	if(b==0)
	{
		f=-f;
	}
	n->prob=f;

	b=vi &0x0400000;
	t=(vi>>5)&(0x01ffff);
	f=t*bs;
	if(b==0)
	{
		f=-f;
	}
	n->bow=f;

	t=((uint64_t)(vi&0x1f))<<32;
	t+=v[2];
	n->child_offset=t;
}

void wtk_lm_node_to_bin_x(wtk_lm_node_t *n,wtk_strbuf_t *buf)
{
	unsigned int v[3];
	int b,i,t;
	float f;

	v[0]=n->id<<12;
	f=n->prob;
	if(f<0)
	{
		b=0;
		f=-f;
	}else
	{
		b=1;
	}
	/*
	if(f>=100.0)
	{
		wtk_debug("prob can only keep 20bit\n");
		exit(0);
	}*/
	v[0]+=b<<11;
	i=(int)(f*10000.0+0.5);
	v[0]+=i>>9;

	t=i&0x1ff;
	v[1]=t<<23;
	f=n->bow;
	if(f<0)
	{
		b=0;
		f=-f;
	}else
	{
		b=1;
	}
	/*
	if(f>=10.0)
	{
		wtk_debug("bow can only keep 17bit\n");
		exit(0);
	}*/
	v[1]+=b<<22;
	i=(int)(f*10000.0+0.5);
	v[1]+=i<<5;
	t=n->child_offset>>32;
	v[1]+=t;

	v[2]=n->child_offset&0x00ffffffff;

	wtk_strbuf_push(buf,(char*)v,12);
}

void wtk_lm_node_from_bin_x(wtk_lm_node_t *n,char *data,int len)
{
	unsigned int *v;
	unsigned int vi;
	int b;
	uint64_t t;
	float f;

	v=(unsigned int*)data;
	vi=v[0];
	n->id=vi>>12;
	b=vi & 0x00800;
	t=(vi &0x007ff)<<9;

	vi=v[1];
	t+=vi>>23;
	f=t*0.0001;
	if(b==0)
	{
		f=-f;
	}
	n->prob=f;

	b=vi &0x0400000;
	t=(vi>>5)&(0x01ffff);
	f=t*0.0001;
	if(b==0)
	{
		f=-f;
	}
	n->bow=f;

	t=((uint64_t)(vi&0x1f))<<32;
	t+=v[2];
	n->child_offset=t;
}

/*
          6位有效数字       6位有效数字            5位有效数字     下一位偏移(137438953472) 128G
         id(1048576)      prob(1+20:131072)     bow(1+17:131072)     offset
    96bit = 20bit     +   21bit      +         18bit        +  37bit

  	115798/17bit  +         14bit	+         14bit  +   27bit
  	131072        -99.99 (-7.164591,-0.009600)
  						 (-1.695097,-0.001791)

*/
//#define USE_ARM
void wtk_lm_node_from_bin_small(wtk_lm_node_t *n,char *data,int len,float ps,float bs)
{
#ifdef USE_ARM
	int vx[3];
#endif
	unsigned char *us;
	unsigned int *v;
	unsigned int vi;
	float f;

#ifdef USE_ARM
	memcpy(vx,data,len);
	data=(char*)vx;
#endif
	us=(unsigned char*)data;
	v=(unsigned int*)data;
	vi=v[0];
	n->id=vi>>15;

	f=((vi&0x7fff)>>1)*ps;
	n->prob=-f;

	vi=(v[0]&0x01)<<13;
	vi+=v[1]>>19;
	n->bow=-(vi*bs);

	n->child_offset=((v[1]&0x7ffff)<<8)+us[8];

	//wtk_debug("id=%d prob=%f bow=%f vi=%d\n",n->id,n->prob,n->bow,(int)n->child_offset);
}


#include <math.h>
/*
 * id+prob(17bit+15bit)
*/
void wtk_lm_node_to_bin_samll_end2(wtk_lm_node_t *n,wtk_strbuf_t *buf,float ps)
{
	unsigned int v;
	float f;

	v=n->id<<15;
	f=n->prob;
	if(f<0)
	{
		f=-f;
	}
	v+=(unsigned int)(f*ps+0.5);
	wtk_strbuf_push(buf,(char*)&v,4);
}

void wtk_lm_node_from_bin_samll_end2(wtk_lm_node_t *n,char *data,int len,float ps)
{
	unsigned int v;

#ifdef USE_ARM
	memcpy((char*)&(v),data,4);
#else
	v=*(unsigned int *)data;
#endif
	n->id=v>>15;

	n->prob=-((v&0x7fff)*ps);
//	wtk_debug("prob=%f\n",n->prob);
//	exit(0);
}

/*
          6位有效数字       6位有效数字            5位有效数字     下一位偏移(137438953472) 128G
  	115798/17bit  +         1+15bit	+        1+15bit  +   31bit
  	131072        -99.99 (-7.164591,-0.009600)
  						 (-1.695097,-0.001791)
*/
void wtk_lm_node_from_bin_small2(wtk_lm_node_t *n,char *data,int len,float ps,float bs)
{
#ifdef USE_ARM
	int vx[3];
#endif
	unsigned short *us;
	unsigned int *v;
	unsigned int vi;
	float f;
	int b;


#ifdef USE_ARM
	memcpy(vx,data,len);
	data=(char*)vx;
#endif
	us=(unsigned short*)data;
	v=(unsigned int*)data;
	vi=v[0];
	n->id=vi>>15;

	f=(vi&0x7fff)*ps;
	b=v[1]&0x0010000;
	if(b)
	{
		n->prob=f;
	}else
	{
		n->prob=-f;
	}
	//vi=v[1]>>17;
	//wtk_debug("vi=%d\n",vi);
	b=v[1]&0x0008000;
	if(b)
	{
		n->bow=((v[1]>>17)*bs);
	}else
	{
		n->bow=-((v[1]>>17)*bs);
	}

	n->child_offset=((v[1]&0x0007fff)<<16)+us[4];

	//wtk_debug("id=%d prob=%f bow=%f vi=%d\n",n->id,n->prob,n->bow,(int)n->child_offset);
}

/*
          6位有效数字       6位有效数字            5位有效数字     下一位偏移(137438953472) 128G
	115798/17bit  +         1+15bit	+        1+15bit  +   31bit
  	131072        -99.99 (-7.164591,-0.009600)
  						 (-1.695097,-0.001791)

*/
void wtk_lm_node_to_bin_small2(wtk_lm_node_t *n,wtk_strbuf_t *buf,float ps,float bs)
{
	unsigned char v[12];
	float f;
	unsigned int *pv,t;
	int b;
	int b2;

	//wtk_debug("id=%d prob=%f bow=%f\n",n->id,n->prob,n->bow);
//	n->child_offset=105784;
//	n->bow=1.35;
//	wtk_debug("id=%d prob=%f bow=%f set=%d\n",n->id,n->prob,n->bow,(int)n->child_offset);
	pv=(unsigned int*)(v);
	pv[0]=n->id<<15;	//17bit
	//wtk_debug("id=%d/%d\n",n->id,pv[0]>>15);

	f=n->prob;
	if(f<-90)
	{
		//-99.99	<s>	-0.713149
		f=0;
	}
	if(f<0)
	{
		f=-f;
		b=0;
	}else
	{
		b=1;
	}
	t=(unsigned int)(f*ps+0.5);//15bit
	//wtk_debug("%x\n",t);
	pv[0]+=t;
	//wtk_debug("id=%d/%d f=%f/%f\n",n->id,pv[0]>>15,f,f*ps);
	f=n->bow;
	if(f<0)
	{
		f=-f;
		b2=0;
	}else
	{
		b2=1;
	}
	t=(unsigned int)(f*bs+0.5);//15bit
	//wtk_debug("t=%d %f/%f\n",t,n->bow,f*bs);
	pv[1]=t<<17;
	//t=pv[1]>>17;
	//wtk_debug("t=%d\n",t);
	if(b)
	{
		pv[1]+=1<<16;
	}
	if(b2)
	{
		pv[1]+=1<<15;
	}

	pv[1]+=(n->child_offset)>>16;
	((unsigned short*)v)[4]=(n->child_offset&0x00ffff);
	wtk_strbuf_push(buf,(char*)v,10);
#ifndef DEBUG
	{
		wtk_lm_node_t nx;

		wtk_lm_node_from_bin_small2(&(nx),(char*)v,9,1.0/ps,1.0/bs);
		if(n->id!=nx.id || (n->prob>-50 && fabs(nx.prob-n->prob)>0.1) || (fabs(nx.bow-n->bow)>0.1)  || (nx.child_offset!=n->child_offset))
		{
			wtk_debug("id=%d prob=%f bow=%f set=%d\n",n->id,n->prob,n->bow,(int)n->child_offset);
			wtk_debug("id=%d prob=%f bow=%f set=%d\n",nx.id,nx.prob,nx.bow,(int)nx.child_offset);
			exit(0);
		}
	}
#endif
}
/*
          6位有效数字       6位有效数字            5位有效数字     下一位偏移(137438953472) 128G
         id(1048576)      prob(1+20:131072)     bow(1+17:131072)     offset
    96bit = 20bit     +   21bit      +         18bit        +  37bit

  	115798/17bit  +         14bit	+         14bit  +   27bit
  	131072        -99.99 (-7.164591,-0.009600)
  						 (-1.695097,-0.001791)

*/
void wtk_lm_node_to_bin_small(wtk_lm_node_t *n,wtk_strbuf_t *buf,float ps,float bs)
{
	unsigned char v[9];
	float f;
	unsigned int *pv,t;


//	if(n->bow>0)
//	{
//		n->bow=0;
//	}
//	n->child_offset=105784;
//	n->bow=1.35;
//	wtk_debug("id=%d prob=%f bow=%f set=%d\n",n->id,n->prob,n->bow,(int)n->child_offset);
	pv=(unsigned int*)(v);
	pv[0]=n->id<<15;
	//wtk_debug("id=%d/%d\n",n->id,pv[0]>>15);

	f=n->prob;
	if(f<-90)
	{
		//-99.99	<s>	-0.713149
		f=0;
	}
	if(f<0)
	{
		f=-f;
	}
	t=(unsigned int)(f*ps+0.5);//14bit
	pv[0]+=t<<1;

	//wtk_debug("id=%d/%d f=%f/%f\n",n->id,pv[0]>>15,f,f*ps);

	f=n->bow;
	if(f<0)
	{
		f=-f;
	}
	t=(unsigned int)(f*bs+0.5);//14bit
	pv[0]+=t>>13;

	//wtk_debug("id=%d/%d\n",n->id,pv[0]>>15);


	pv[1]=(t&0x1fff)<<19;
	pv[1]+=(n->child_offset)>>8; //27-19
	v[8]=(n->child_offset&0x00ff);
	//wtk_debug("%#x %#x %#x\n",(int)n->child_offset,(pv[1]&0x7ffff),v[8]);

	wtk_strbuf_push(buf,(char*)v,9);
#ifndef DEBUG
	{
		wtk_lm_node_t nx;

		wtk_lm_node_from_bin_small(&(nx),(char*)v,9,1.0/ps,1.0/bs);
		//wtk_debug("prob=%f bow=%f ps=%f bs=%f\n",n->prob,n->bow,ps,bs);
		if(n->id!=nx.id || (n->prob>-50 && fabs(nx.prob-n->prob)>0.1) || (fabs(nx.bow-n->bow)>0.1))
		{
			wtk_debug("id=%d prob=%f bow=%f set=%d\n",n->id,n->prob,n->bow,(int)n->child_offset);
			wtk_debug("id=%d prob=%f bow=%f set=%d\n",nx.id,nx.prob,nx.bow,(int)nx.child_offset);
			exit(0);
		}
	}
#endif
}

/*
 * id+prob(17bit+14bit)
*/
void wtk_lm_node_to_bin_samll_end(wtk_lm_node_t *n,wtk_strbuf_t *buf,float ps)
{
	unsigned int v;
	float f;

	v=n->id<<15;
	f=n->prob;
	if(f<0)
	{
		f=-f;
	}
	v+=(unsigned int)(f*ps+0.5);
	wtk_strbuf_push(buf,(char*)&v,4);
}

void wtk_lm_node_from_bin_samll_end(wtk_lm_node_t *n,char *data,int len,float ps)
{
	unsigned int v;

#ifdef USE_ARM
	memcpy((char*)&(v),data,4);
#else
	v=*(unsigned int *)data;
#endif
	n->id=v>>15;

	n->prob=-((v&0x3fff)*ps);
//	wtk_debug("prob=%f\n",n->prob);
//	exit(0);
}




/*
 * id+prob(20bit+20bit)
          6位有效数字       5位有效数字
         id(1048576)      prob(1+19:524288)
    40bit = 20bit     +   20bit

    20+1+20+1+17+37
*/
void wtk_lm_node_to_bin2(wtk_lm_node_t *n,wtk_strbuf_t *buf,float ps)
{
	unsigned int v0;
	unsigned char v1;
	int b,i;
	float f;

	/*
	if(n->id>=1048576)
	{
		wtk_debug("max id can keep 20 bit\n");
		exit(0);
	}*/
	v0=n->id<<12;
	f=n->prob;
	if(f<0)
	{
		b=0;
		f=-f;
	}else
	{
		b=1;
	}
	/*
	if(f>=10.0)
	{
		wtk_debug("prob can only keep 20bit\n");
		exit(0);
	}*/
	v0+=b<<11;
	i=(int)(f*ps+0.5);
	v0+=i>>8;

	v1=i&0x00ff;

	wtk_strbuf_push(buf,(char*)&v0,4);
	wtk_strbuf_push(buf,(char*)&v1,1);
}

void wtk_lm_node_to_bin2_x(wtk_lm_node_t *n,wtk_strbuf_t *buf)
{
	unsigned int v0;
	unsigned char v1;
	int b,i;
	float f;

	/*
	if(n->id>=1048576)
	{
		wtk_debug("max id can keep 20 bit\n");
		exit(0);
	}*/
	v0=n->id<<12;
	f=n->prob;
	if(f<0)
	{
		b=0;
		f=-f;
	}else
	{
		b=1;
	}
	/*
	if(f>=10.0)
	{
		wtk_debug("prob can only keep 20bit\n");
		exit(0);
	}*/
	v0+=b<<11;
	i=(int)(f*10000.0+0.5);
	v0+=i>>8;

	v1=i&0x00ff;

	wtk_strbuf_push(buf,(char*)&v0,4);
	wtk_strbuf_push(buf,(char*)&v1,1);
}

/*
 * id+prob(20bit+20bit)
          6位有效数字       5位有效数字
         id(1048576)      prob(1+19:524288)
    40bit = 20bit     +   20bit
    20+1+19
    19-8=11;
*/
void wtk_lm_node_from_bin2(wtk_lm_node_t *n,char *data,int len,float ps)
{
	unsigned int v0;
	int b;
	int pi;
	float f;

	//print_hex(data,len);
	v0=*(unsigned int*)data;
	n->id=v0>>12;

	b=v0&0x0800;
	pi=(v0&0x07ff)<<8;

	v0=*((unsigned char*)(data+4));
	v0+=pi;

	f=(v0*1.0)*ps;///10000.0;
	if(b==0)
	{
		f=-f;
	}
	n->prob=f;
}

void wtk_lm_node_from_bin2_x(wtk_lm_node_t *n,char *data,int len)
{
	unsigned int v0;
	int b;
	int pi;
	float f;

	//print_hex(data,len);
	v0=*(unsigned int*)data;
	n->id=v0>>12;

	b=v0&0x0800;
	pi=(v0&0x07ff)<<8;

	v0=*((unsigned char*)(data+4));
	v0+=pi;

	f=(v0*1.0)/10000.0;
	if(b==0)
	{
		f=-f;
	}
	n->prob=f;
}

wtk_lm_node_t* wtk_lm_node_root(wtk_lm_node_t *n)
{
	wtk_lm_node_t *root;

	root=n;
	while(root->parent)
	{
		root=root->parent;
	}
	return root;
}

int wtk_lm_node_trace_id(wtk_lm_node_t *n,int *ids)
{
	int cnt=0;

	if(n->parent)
	{
		cnt=wtk_lm_node_trace_id(n->parent,ids);
	}
	ids[cnt]=n->id;
	++cnt;
	return cnt;
}

void wtk_lm_node_tostring2(wtk_fst_insym_t *sym,wtk_lm_node_t *n,wtk_strbuf_t *buf)
{
	wtk_string_t *v;

	if(n->parent)
	{
		wtk_lm_node_tostring2(sym,n->parent,buf);
	}
	//wtk_debug("id=%d\n",n->id);
	v=sym->ids[n->id]->str;
	if(buf->pos>0)
	{
		wtk_strbuf_push_s(buf," ");
	}
	wtk_strbuf_push(buf,v->data,v->len);
	wtk_strbuf_push_f(buf,"[id=%d,ngram=%d]",n->id,n->ngram);
}

void wtk_lm_node_tostring(wtk_fst_insym_t *sym,wtk_lm_node_t *n,wtk_strbuf_t *buf)
{
	wtk_strbuf_reset(buf);
	wtk_lm_node_tostring2(sym,n,buf);
}

void wtk_lm_node_print2(wtk_fst_insym_t *sym,wtk_lm_node_t *n)
{
	wtk_strbuf_t *buf;
	int i;

	buf=wtk_strbuf_new(256,1);
	for(i=1;i<n->ngram;++i)
	{
		printf("  ");
	}
	wtk_lm_node_tostring(sym,n,buf);
	printf("%f %.*s %f\n",n->prob,buf->pos,buf->data,n->bow);
	wtk_strbuf_delete(buf);
}


void wtk_lm_node_print(wtk_lm_node_t *n)
{
	wtk_debug("========== node=%p ===========\n",n);
	printf("ngram\t%d\n",n->ngram);
	printf("id\t%d\n",n->id);
	printf("prob\t%f\n",n->prob);
	printf("bow\t%f\n",n->bow);
	printf("self-off\t%d\n",(int)n->self_offset);
	printf("child-off\t%d\n",(int)n->child_offset);
	printf("child\t%d\n",n->nchild);//wtk_queue2_len(&(n->child_q)));
}

void wtk_lm_node_print_parent(wtk_lm_node_t *n)
{
	if(n->parent)
	{
		wtk_lm_node_print_parent(n->parent);
	}
	wtk_lm_node_print(n);
}

void wtk_lm_node_print_parent2(wtk_fst_insym_t *sym,wtk_lm_node_t *n)
{
	wtk_lm_node_t *r;

	r=wtk_lm_node_root(n);
	wtk_lm_node_print_child(sym,r);
}

void wtk_lm_node_print_child(wtk_fst_insym_t *sym,wtk_lm_node_t *n)
{
	wtk_string_t *v;
	int i;

	v=sym->ids[n->id]->str;
	for(i=1;i<n->ngram;++i)
	{
		printf("  ");
	}
	printf("%f %.*s %f [%d,child=%d,p=%p]\n",n->prob,v->len,v->data,n->bow,
			n->ngram,(int)n->child_offset,n);
	for(i=0;i<n->nchild;++i)
	{
		wtk_lm_node_print_child(sym,n->childs[i]);
	}
}

