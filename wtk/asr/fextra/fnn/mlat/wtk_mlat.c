#include "wtk_mlat.h" 
#include "../../wtk_fextra.h"
#include <math.h>
#include "wtk/core/cfg/wtk_source.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/math/wtk_matrix.h"
#include "../wtk_fnn.h"

#define WTK_MLAT_INIT_SHIFT(dst,dst_s,dst_e,dst_e2,m,N) \
dst_s=dst; \
while(dst_s<dst_e) \
{\
	dst_s[0]=m[0]<<N; \
	dst_s[1]=m[1]<<N; \
	dst_s[2]=m[2]<<N; \
	dst_s[3]=m[3]<<N; \
	dst_s[4]=m[4]<<N; \
	dst_s[5]=m[5]<<N; \
	dst_s[6]=m[6]<<N; \
	dst_s[7]=m[7]<<N;\
	m+=8; \
	dst_s+=8;\
}\
while(dst_s<dst_e2) \
{\
	*(dst_s++)=(*(m++))<<N;\
}

#define WTK_MLAT_ADD(dst,dst_s,dst_e,dst_e2,m) \
dst_s=dst; \
while(dst_s<dst_e) \
{\
	dst_s[0]+=m[0]; \
	dst_s[1]+=m[1]; \
	dst_s[2]+=m[2]; \
	dst_s[3]+=m[3]; \
	dst_s[4]+=m[4]; \
	dst_s[5]+=m[5]; \
	dst_s[6]+=m[6]; \
	dst_s[7]+=m[7];\
	m+=8; \
	dst_s+=8;\
}\
while(dst_s<dst_e2) \
{\
	*(dst_s++)+=(*(m++));\
}

#define WTK_MLAT_ADD_SHIFT(dst,dst_s,dst_e,dst_e2,m,N) \
dst_s=dst; \
while(dst_s<dst_e) \
{\
	dst_s[0]+=m[0]<<N; \
	dst_s[1]+=m[1]<<N; \
	dst_s[2]+=m[2]<<N; \
	dst_s[3]+=m[3]<<N; \
	dst_s[4]+=m[4]<<N; \
	dst_s[5]+=m[5]<<N; \
	dst_s[6]+=m[6]<<N; \
	dst_s[7]+=m[7]<<N;\
	m+=8; \
	dst_s+=8;\
}\
while(dst_s<dst_e2) \
{\
	*(dst_s++)+=(*(m++))<<N;\
}



wtk_mlat_feature_t* wtk_mlat_new_feature(wtk_mlat_t *m)
{
	wtk_mlat_feature_t *f;

	f=(wtk_mlat_feature_t*)wtk_malloc(sizeof(wtk_mlat_feature_t));
	f->v=wtk_vecf_new(m->cfg->input_size);
	f->f=NULL;
	return f;
}

int wtk_mlat_feature_delete(wtk_mlat_feature_t *f)
{
	wtk_vecf_delete(f->v);
	wtk_free(f);
	return 0;
}

wtk_mlat_feature_t* wtk_mlat_pop_feature(wtk_mlat_t *m)
{
	return (wtk_mlat_feature_t*)wtk_lockhoard_pop(&(m->feature_hoard));
}

void  wtk_mlat_push_feature(wtk_mlat_t *m,wtk_mlat_feature_t *f)
{
	wtk_lockhoard_push(&(m->feature_hoard),f);
}

int wtk_mlat_thread_env_run(wtk_mlat_thread_env_t *m,wtk_thread_t *t);
int wtk_mlat_thread_run(wtk_mlat_t *m,wtk_thread_t *t);

wtk_mlat_fix_env_veci_t* wtk_mlat_fix_env_get_vec(wtk_mlat_fix_env_t *m,int v)
{
	wtk_queue_node_t *qn;
	wtk_mlat_fix_env_veci_t *vec;

	for(qn=m->veci_q.pop;qn;qn=qn->next)
	{
		vec=data_offset2(qn,wtk_mlat_fix_env_veci_t,q_n);
		if(vec->v->len==v)
		{
			wtk_queue_remove(&(m->veci_q),qn);
			return vec;
		}
	}
	vec=(wtk_mlat_fix_env_veci_t*)wtk_malloc(sizeof(wtk_mlat_fix_env_veci_t));
	vec->v=wtk_veci_new(v);
//	{
//		static int ki=0;
//
//		++ki;
//		wtk_debug("new vec=%d/%p\n",ki,vec);
//	}
	return vec;
}

void wtk_mlat_fix_env_push_vec(wtk_mlat_fix_env_t *env,wtk_mlat_fix_env_veci_t *v)
{
	wtk_queue_push(&(env->veci_q),&(v->q_n));
}

wtk_mlat_fix_env_vecuc_t* wtk_mlat_fix_env_get_vecuc(wtk_mlat_fix_env_t *m,int v)
{
	wtk_queue_node_t *qn;
	wtk_mlat_fix_env_vecuc_t *vec;

	for(qn=m->vecuc_q.pop;qn;qn=qn->next)
	{
		vec=data_offset2(qn,wtk_mlat_fix_env_vecuc_t,q_n);
		if(vec->v->len==v)
		{
			wtk_queue_remove(&(m->vecuc_q),qn);
			return vec;
		}
	}
	vec=(wtk_mlat_fix_env_vecuc_t*)wtk_malloc(sizeof(wtk_mlat_fix_env_vecuc_t));
	vec->v=wtk_vecuc_new(v);
//	{
//		static int ki=0;
//
//		++ki;
//		wtk_debug("new veuc=%d/%p len=%d\n",ki,vec,v);
//	}
	return vec;
}

void wtk_mlat_fix_env_push_vecuc(wtk_mlat_fix_env_t *env,wtk_mlat_fix_env_vecuc_t *v)
{
	//wtk_debug("push veuc=%p/%d\n",v,v->v->len,env->v);
	wtk_queue_push(&(env->vecuc_q),&(v->q_n));
}



wtk_mlat_fix_env_t* wtk_mlat_fix_env_new(wtk_mlat_cfg_t *cfg)
{
	wtk_queue_node_t *n;
	wtk_mlat_layer_t *l;
	int i;
	wtk_mlat_fix_env_t *env;

	env=(wtk_mlat_fix_env_t*)wtk_malloc(sizeof(wtk_mlat_fix_env_t));
	for(i=0,n=cfg->layer_q.pop;n;n=n->next,++i)
	{
		//wtk_debug("[%d/%d]\n",f->index,i);
		l=data_offset(n,wtk_mlat_layer_t,q_n);
		//wtk_debug("float=%d\n",l->float_type);
		if(i==0)
		{
			env->layer0=wtk_vecf_new(l->b->len);
			if(cfg->use_int==0)
			{
				env->layer0_output.u=wtk_vecuc_new(l->b->len);
			}else
			{
				env->layer0_output.i=wtk_veci_new(l->b->len);
			}
		}else
		{
			//wtk_debug("col=%d\n",l->fix_wb->b->col);
		}
	}
	//wtk_debug("len=%p/%d\n",env->layer0_output,env->layer0_output->len);
	wtk_queue_init(&(env->veci_q));
	wtk_queue_init(&(env->vecuc_q));
	//exit(0);
	return env;
}

void wtk_mlat_fix_env_delete(wtk_mlat_fix_env_t *env,int use_int)
{
	wtk_queue_node_t *qn;
	wtk_mlat_fix_env_veci_t *vec;
	wtk_mlat_fix_env_vecuc_t *uc;

	while(1)
	{
		qn=wtk_queue_pop(&(env->veci_q));
		if(!qn){break;}
		vec=data_offset2(qn,wtk_mlat_fix_env_veci_t,q_n);
		wtk_veci_delete(vec->v);
		wtk_free(vec);
	}
	while(1)
	{
		qn=wtk_queue_pop(&(env->vecuc_q));
		if(!qn){break;}
		uc=data_offset2(qn,wtk_mlat_fix_env_vecuc_t,q_n);
		wtk_vecuc_delete(uc->v);
		wtk_free(uc);
	}

	wtk_vecf_delete(env->layer0);
	if(!use_int)
	{
		wtk_vecuc_delete(env->layer0_output.u);
	}else
	{
		wtk_veci_delete(env->layer0_output.i);
	}
	wtk_free(env);
}

wtk_mlat_thread_env_vec_t* wtk_mlat_thread_env_get_vec(wtk_mlat_thread_env_t *m,int v)
{
	wtk_queue_node_t *qn;
	wtk_mlat_thread_env_vec_t *vec;

	for(qn=m->vecf_q.pop;qn;qn=qn->next)
	{
		vec=data_offset2(qn,wtk_mlat_thread_env_vec_t,q_n);
		if(vec->v->len==v)
		{
			wtk_queue_remove(&(m->vecf_q),qn);
			return vec;
		}
	}
	vec=(wtk_mlat_thread_env_vec_t*)wtk_malloc(sizeof(wtk_mlat_thread_env_vec_t));
	vec->v=wtk_vecf_new(v);
	return vec;
}

void wtk_mlat_thread_env_push_vec(wtk_mlat_thread_env_t *m,wtk_mlat_thread_env_vec_t *v)
{
	wtk_queue_push(&(m->vecf_q),&(v->q_n));
}

wtk_mlat_thread_env_t* wtk_mlat_thread_env_new(wtk_mlat_t *mlat)
{
	wtk_mlat_thread_env_t *env;

	env=(wtk_mlat_thread_env_t*)wtk_calloc(1,sizeof(wtk_mlat_thread_env_t));
	env->mlat=mlat;
	if(mlat->cfg->use_fix_res)
	{
		env->fix=wtk_mlat_fix_env_new(mlat->cfg);
	}else
	{
		env->fix=NULL;
	}
	wtk_queue_init(&(env->vecf_q));
	wtk_blockqueue_init(&(env->msg_q));
	wtk_thread_init(&(env->thread),(thread_route_handler)wtk_mlat_thread_env_run,env);
	wtk_thread_set_name(&(env->thread),"mlat");
	wtk_thread_start(&(env->thread));
	env->d2i.n.j=0;
	return env;
}

void wtk_mlat_thread_env_delete(wtk_mlat_thread_env_t *env,int use_int)
{
	wtk_queue_node_t *qn;
	wtk_mlat_thread_env_vec_t *vec;

	wtk_blockqueue_wake(&(env->msg_q));
	wtk_thread_join(&(env->thread));
	wtk_thread_clean(&(env->thread));
	if(env->fix)
	{
		wtk_mlat_fix_env_delete(env->fix,use_int);
	}
	while(1)
	{
		qn=wtk_queue_pop(&(env->vecf_q));
		if(!qn){break;}
		vec=data_offset2(qn,wtk_mlat_thread_env_vec_t,q_n);
		wtk_vecf_delete(vec->v);
		wtk_free(vec);
	}
	wtk_free(env);
}

wtk_mlat_msg_t* wtk_mlat_pop_msg(wtk_mlat_t *m)
{
	wtk_mlat_msg_t *msg;

	msg=(wtk_mlat_msg_t*)wtk_lockvpool_pop(m->feature_qn_pool);
	msg->hook=NULL;
	return msg;
}

void wtk_mlat_push_msg(wtk_mlat_t *m,wtk_mlat_msg_t *s)
{
	wtk_lockvpool_push(m->feature_qn_pool,s);
}

wtk_mlat_t* wtk_mlat_new(wtk_mlat_cfg_t *cfg,struct wtk_fnn *dnn)
{
	wtk_mlat_t *m;
	int i;

	m=(wtk_mlat_t*)wtk_malloc(sizeof(wtk_mlat_t));
	m->cfg=cfg;
	m->dnn=dnn;
	wtk_lockhoard_init(&(m->feature_hoard),offsetof(wtk_mlat_feature_t,q_n),20,
			(wtk_new_handler_t)wtk_mlat_new_feature,
			(wtk_delete_handler_t)wtk_mlat_feature_delete,m);
	m->feature_qn_pool=wtk_lockvpool_new(sizeof(wtk_mlat_msg_t),50);
	m->run=1;
	m->route_run=0;
	wtk_sem_init(&(m->wait_sem),0);
	wtk_blockqueue_init(&(m->input_q));
	wtk_blockqueue_init(&(m->output_q));
	m->nthread=cfg->nthread>0?cfg->nthread:1;
	m->thread_env=(wtk_mlat_thread_env_t**)wtk_calloc(m->nthread,sizeof(wtk_mlat_thread_env_t*));
	for(i=0;i<m->nthread;++i)
	{
		m->thread_env[i]=wtk_mlat_thread_env_new(m);
	}
	wtk_thread_init(&(m->output_thread),(thread_route_handler)wtk_mlat_thread_run,m);
	wtk_thread_set_name(&(m->output_thread),"mlat");
	wtk_thread_start(&(m->output_thread));
	return m;
}

void wtk_mlat_delete(wtk_mlat_t *m)
{
	int i;

	m->run=0;
	for(i=0;i<m->nthread;++i)
	{
		wtk_blockqueue_wake(&(m->input_q));
	}
	for(i=0;i<m->nthread;++i)
	{
		wtk_mlat_thread_env_delete(m->thread_env[i],m->cfg->use_int);
	}
	wtk_free(m->thread_env);
	wtk_blockqueue_wake(&(m->output_q));
	wtk_thread_join(&(m->output_thread));
	wtk_thread_clean(&(m->output_thread));
	wtk_blockqueue_clean(&(m->input_q));
	wtk_blockqueue_clean(&(m->output_q));
	wtk_sem_clean(&(m->wait_sem));
	wtk_lockvpool_delete(m->feature_qn_pool);
	wtk_lockhoard_clean(&(m->feature_hoard));
	wtk_free(m);
}

void wtk_mlat_reset(wtk_mlat_t *m)
{
}

void wtk_mlat_trans_process(wtk_mlat_feature_t *m,wtk_mlat_trans_t *trans)
{
	register float *pb,*pm,*pv,*pe;

	pb=trans->b->p;
	pm=trans->w->p;
	pv=m->v->p;
	pe=pv+m->v->len;
	while(pv<pe)
	{
		*pv=(*pv+(*pb++))*(*(pm++));
		++pv;
	}
}


#define wtk_flat_expf(f) wtk_fast_exp2(f)

void wtk_mlat_sigmoid(float *p,int n)
{
	int i;

	for(i=0;i<n;++i)
	{
		p[i]=1.0/(1.0+wtk_flat_expf(-p[i]));
	}
}

void wtk_mlat_softmax_and_log(float* a,int len)
{
	float max;
	float *p,*e;
	double sum;

	max=wtk_math_max(a,len);
	sum=0;
	p=a;e=p+len;
	while(p<e)
	{
		*p=expf(*p-max);
		sum+=*p;
		++p;
	}
	//sum=1.0f/sum;
	sum=-log(sum);
	p=a;e=p+len;
	while(p<e)
	{
		//*p*=sum;
		*p=log(*p)+sum;
		++p;
	}
}

float wtk_mlat_float_mult(float *psrc,float *pm,int count)
{
	float f;
	float *pe;

	f=0;
	pe=psrc+count;
	while(psrc<pe)
	{
		f+=psrc[0]*pm[0]+psrc[1]*pm[1]+psrc[2]*pm[2]+psrc[3]*pm[3];
		f+=psrc[4]*pm[4]+psrc[5]*pm[5]+psrc[6]*pm[6]+psrc[7]*pm[7];
		psrc+=8;
		pm+=8;
	}
	return f;
}


//#if ! defined(ANDROID) && defined(__arm__)
#if defined(__arm__) && defined(USE_NEON_ASM)
float wtk_mlat_float_vm_neon2(float *src_a,float * src_b,int count)
{
  float sse;

  asm volatile (
    // Clear q8, q9, q10, q11
    "veor    q8, q8, q8                            \n"
    "veor    q9, q9, q9                            \n"
    "veor    q10, q10, q10                     \n"
    "veor    q11, q11, q11                     \n"
	"ldr r0,[%[count]] \n"
	"mov r1,%[src_a]\n"
	"mov r2,%[src_b]\n"
	"1: \n"
	"subs r0,r0,#16\n"
	"blt 2f \n"
    "vld1.32     {q1, q2}, [r1]!       \n"
    "vld1.32     {q3, q4}, [r1]!       \n"
    "vld1.32     {q12, q13}, [r2]!  \n"
    "vld1.32     {q14, q15}, [r2]!  \n"
    "vmla.f32   q8, q1, q12                      \n"
    "vmla.f32   q9, q2, q13                     \n"
    "vmla.f32   q10, q3, q14                    \n"
    "vmla.f32   q11, q4, q15                   \n"
	"b 1b \n"
    "2:     \n"
	"vadd.f32   q8, q8, q9                      \n"
	"vadd.f32   q10, q10, q11                \n"
	"vadd.f32   q11, q8, q10                  \n"
	"vpadd.f32  d2, d22, d23                 \n"
	"vpadd.f32  d0, d2, d2                      \n"
	//"vmov.32 r5,d0[0]\n"
	"add r0,r0,#16 \n"
	"3: \n"
	"veor    d1, d1, d1 \n"
	"subs r0,r0,#1 \n"
	"blt 4f \n"
	"vld1.32 {d1[0]},[r1]! \n"
	"vld1.32 {d2[0]},[r2]! \n"
	"vmla.f32 d0,d1,d2[0] \n"
	"b 3b\n"
	"4:\n"
	"vmov.32 %0,d0[0]\n"
    : [sse]"=r"(sse)
    : [src_a]"r"(src_a),
      [src_b]"r"(src_b),
      [count]"r"(&count)
    : "memory", "cc","r0","r1","r2","r3","r4","r5","q1", "q2", "q3","q4","q8", "q9", "q10", "q11",
      "q12", "q13","q14", "q15");
 // printf("t1=%p src_a=%p\n",t1,src_a);
  return sse;
}

void wtk_mlat_float_vm_raw(float *dst, float *src,float *m,int row,int col)
{
	int i;
	for(i=0;i<row;++i)
	{
		//wtk_debug("v[%d/%d] src1=%p m=%p\n",i,row,src,m);
		dst[i]=wtk_mlat_float_vm_neon2(src,m,col);
		m+=col;
	}
}

#else
void wtk_mlat_float_vm_raw(float *dst,float *src,float *m,int row,int col)
{
	register float f,*psrc,*pm,*pe2,*pe;
	int i;
	int col2;

	col2=(col>>3)<<3;
	pm=m;
	if(col==col2)
	{
		pe=src+col;
		for(i=0;i<row;++i)
		{
			f=0;
			psrc=src;
			while(psrc<pe)
			{
				f+=psrc[0]*pm[0];
				f+=psrc[1]*pm[1];
				f+=psrc[2]*pm[2];
				f+=psrc[3]*pm[3];
				f+=psrc[4]*pm[4];
				f+=psrc[5]*pm[5];
				f+=psrc[6]*pm[6];
				f+=psrc[7]*pm[7];
				psrc+=8;
				pm+=8;
			}
			dst[i]=f;
		}
	}else
	{
		pe2=src+col2;
		pe=src+col;
		for(i=0;i<row;++i)
		{
			f=0;
			psrc=src;
			while(psrc<pe2)
			{
				f+=psrc[0]*pm[0];
				f+=psrc[1]*pm[1];
				f+=psrc[2]*pm[2];
				f+=psrc[3]*pm[3];
				f+=psrc[4]*pm[4];
				f+=psrc[5]*pm[5];
				f+=psrc[6]*pm[6];
				f+=psrc[7]*pm[7];
				psrc+=8;
				pm+=8;
			}
			while(psrc<pe)
			{
				f+=(*(psrc++))*(*(pm++));
			}
			dst[i]=f;
		}
	}
}
#endif

void wtk_mlat_float_vm_raw_transpose(float *dst,float *src,register float *m,int row,int col)
{
	register float f;
	int i;
	int col2;
	register float *dst_s,*dst_e;
	float *dst_e2;

	col2=(col>>3)<<3;
	dst_e=dst+col2;
	dst_e2=dst+col;
	f=src[0];
	if(f==0)
	{
		memset(dst,0,sizeof(int)*col);
		m+=col;
	}else
	{
		dst_s=dst;
		while(dst_s<dst_e)
		{
			dst_s[0]=f*m[0];
			dst_s[1]=f*m[1];
			dst_s[2]=f*m[2];
			dst_s[3]=f*m[3];
			dst_s[4]=f*m[4];
			dst_s[5]=f*m[5];
			dst_s[6]=f*m[6];
			dst_s[7]=f*m[7];
			m+=8;
			dst_s+=8;
		}
		while(dst_s<dst_e2)
		{
			*(dst_s++)=f*(*(m++));
		}
	}
	for(i=1;i<row;++i)
	{
		f=src[i];
		if(f==0)
		{
			m+=col;
		}else
		{
			dst_s=dst;
			while(dst_s<dst_e)
			{
				dst_s[0]+=f*m[0];
				dst_s[1]+=f*m[1];
				dst_s[2]+=f*m[2];
				dst_s[3]+=f*m[3];
				dst_s[4]+=f*m[4];
				dst_s[5]+=f*m[5];
				dst_s[6]+=f*m[6];
				dst_s[7]+=f*m[7];
				m+=8;
				dst_s+=8;
			}
			while(dst_s<dst_e2)
			{
				*(dst_s++)+=f*(*(m++));
			}
		}
	}
}


void wtk_mlat_float_vm_raw_transpose_raw(float *dst,float *src,float *m,int row,int col)
{
	register float f;
	int i,j;

	for(i=0;i<row;++i)
	{
		f=src[i];
		if(i==0)
		{
			for(j=0;j<col;++j)
			{
				dst[j]=f*m[j];
			}
		}else
		{
			for(j=0;j<col;++j)
			{
				dst[j]+=f*m[j];
			}
		}
		m+=col;
	}
}

void wtk_mlat_process_dnn_layer(wtk_mlat_t *d,wtk_mlat_layer_t *l,wtk_vecf_t *input,
		wtk_vecf_t *output)
{
	//int n;

	//print_float(output->p,10);
	//wtk_debug("row=%d/%d\n",output->len,l->w->row);
	//wtk_debug("row=%d/%d\n",l->w->row,l->w->col);
	if(d->cfg->use_transpose)
	{
		wtk_mlat_float_vm_raw_transpose(output->p,input->p,l->w->p,l->w->row,l->w->col);
	}else
	{
		wtk_mlat_float_vm_raw(output->p,input->p,l->w->p,l->w->row,l->w->col);
	}

	//wtk_matf_vm(output->p,input->p,l->w);
	//print_float(output->p,10);
//	{
//		static int ki=0;
//		++ki;
//		if(ki==4)
//		{
//			wtk_debug("%d/%d row=%d col=%d\n",input->len,output->len,l->w->row,l->w->col);
//			exit(0);
//		}
//	}

	if(l->b)
	{
		wtk_float_add(output->p,l->b->p,output->len);
	}
	switch(l->type)
	{
	case wtk_fnn_sigmoid:
		//wtk_debug("row=%d col=%d\n",wtk_matrix_rows(output),wtk_matrix_cols(output));
		wtk_mlat_sigmoid(output->p,output->len);
		/*
		n=wtk_matrix_cols(output);
		wtk_ann_sigmoid(output[1]+1,n);
		//wtk_matrix_print(output);
		 */
		break;
	case wtk_fnn_softmax:
		//wtk_matrix_print(output);
		if(!d->dnn->cfg->use_linear_output)
		{
			wtk_mlat_softmax_and_log(output->p,output->len);
		}
		break;
	case wtk_fnn_linear:
		break;
	default:
        wtk_debug("layer->type not in list. %d\n", l->type);
        break;
	}
}


void wtk_mlat_process_layer_matrix(wtk_mlat_thread_env_t *m,wtk_mlat_feature_t *mf)
{
	wtk_mlat_t *d=m->mlat;
	wtk_queue_node_t *n;
	wtk_mlat_layer_t *l;
	wtk_vecf_t *input_f,*output_f;
	wtk_mlat_thread_env_vec_t *env_vec=NULL;
	wtk_mlat_thread_env_vec_t *last_vec=NULL;
	int i;
	wtk_vecf_t f_output;

	f_output.len=wtk_vector_size(mf->f->dnn_v);
	f_output.p=mf->f->dnn_v+1;
	//wtk_debug("v[%d][%d]=%f\n",ki,idx,m[1][idx]);
	wtk_mlat_trans_process(mf,d->cfg->trans);
	input_f=mf->v;
	for(i=0,n=d->cfg->layer_q.pop;n;n=n->next,++i)
	{
		//wtk_debug("[%d/%d]\n",f->index,i);
		l=data_offset(n,wtk_mlat_layer_t,q_n);
		//wtk_debug("float=%d\n",l->float_type);
		//wtk_debug("v[%d]=%d/%d %d\n",i,l->w->row,l->w->col,input_f->len);
		if(n->next)
		{
			env_vec=wtk_mlat_thread_env_get_vec(m,l->w->row);
			output_f=env_vec->v;
		}else
		{
			env_vec=NULL;
			output_f=&(f_output);
		}
		//wtk_debug("======== i=%d type=%d  %d/%d %d/%d======\n",i,l->type,input_f->len,output_f->len,l->w->row,l->w->col);
		//print_float(input_f->p,10);
		//exit(0);
		wtk_mlat_process_dnn_layer(d,l,input_f,output_f);
		input_f=output_f;
		//print_float(input_f->p,10);
		if(last_vec)
		{
			wtk_mlat_thread_env_push_vec(m,last_vec);
		}
		last_vec=env_vec;
	}
	//print_float(f_output.p,10);
	//exit(0);
	//wtk_debug("vec=%d\n",m->vecf_q.length);
	//exit(0);
	//wtk_mlat_raise_feature(d,mf);
}

int wtk_mlat_int_vm_raw_char(unsigned char *src,signed char *m,int col)
{
	int i;
	int v;

	v=0;
	for(i=0;i<col;++i)
	{
		v+=src[i]*m[i];
	}
	return v;
}


#if defined(__armx__) && defined(USE_NEON_ASM)
int wtk_mlat_uchar_char_mla(unsigned char *src_a,signed char * src_b,int count)
{
  int sse;

  asm volatile (
    // Clear q8, q9, q10, q11'
    "veor    q0, q0, q0                            \n"
	"veor    q1, q1, q1                            \n"
    "veor    q2, q2, q2                            \n"
	"veor    q3, q3, q3                            \n"
	"veor    q4, q4, q4                            \n"
	"veor    q5, q5, q5                            \n"
	"veor    q6, q6, q6                            \n"
	"veor    q7, q7, q7                            \n"
    "veor    q8, q8, q8                           \n"
    "veor    q9, q9, q9                            \n"
    "veor    q10, q10, q10                     \n"
    "veor    q11, q11, q11                     \n"
	"ldr r0,[%[count]] \n"
	"mov r1,%[src_a]\n"
	"mov r2,%[src_b]\n"
	"1: \n"
	"subs r0,r0,#8\n"
	"blt 2f \n"
    "vld4.8     {d0[0],d1[0],d2[0],d3[0]}, [r1]!       \n"
    "vld4.8     {d4[0],d5[0],d6[0],d7[0]}, [r1]!       \n"
    "vld4.8     {d8[0],d9[0],d10[0],d11[0]}, [r2]!  \n"
    "vld4.8     {d12[0],d13[0],d14[0],d15[0]}, [r2]!  \n"
	"vshl.i32 q4, q4, #24\n"
	"vshr.s32 q4, q4, #24\n"
	"vshl.i32 q5, q5, #24\n"
	"vshr.s32 q5, q5, #24\n"
	"vshl.i32 q6, q6, #24\n"
	"vshr.s32 q6, q6, #24\n"
	"vshl.i32 q7, q7, #24\n"
	"vshr.s32 q7, q7, #24\n"
    "vmla.i32   q8, q0, q4                    \n"
    "vmla.i32   q9, q1, q5                     \n"
    "vmla.i32   q10, q2, q6                    \n"
    "vmla.i32   q11, q3, q7                   \n"
	"b 1b \n"
    "2:     \n"
	"vadd.i32   q8, q8, q9                      \n"
	"vadd.i32   q10, q10, q11                \n"
	"vadd.i32   q11, q8, q10                  \n"
	"vpadd.i32  d2, d22, d23                 \n"
	"vpadd.i32  d0, d2, d2                      \n"
	"vmov.32 %0,d0[0] \n"
	"add r0,r0,#8 \n"
	"3: \n"
	"subs r0,r0,#1 \n"
	"blt 4f \n"
	"vld1.8 {d1[0]},[r1]! \n"
	"vld1.8 {d2[0]},[r2]! \n"
	"vmla.i32 d0,d1,d2 \n"
	"b 3b\n"
	"4:\n"
	"vmov.32 %0,d0[0]\n"
    : [sse]"=r"(sse)
    : [src_a]"r"(src_a),
      [src_b]"r"(src_b),
      [count]"r"(&count)
    : "memory", "cc","r0","r1","r2","q1", "q2", "q3","q4","q8", "q9", "q10", "q11",
      "q12", "q13","q14", "q15");
 // printf("t1=%p src_a=%p\n",t1,src_a);
  return sse;
}

void wtk_mlat_int_vm_raw(int *dst,unsigned char *src,signed char *m,int row,int col)
{
	int i;

	for(i=0;i<row;++i)
	{
		dst[i]=wtk_mlat_uchar_char_mla(src,m,col);
		{
			int v;

			v=wtk_mlat_int_vm_raw_char(src,m,col);
			if(fabs(v-dst[i])>1)
			{
				wtk_debug("founc bug[%d/%d]=%d/%d col=%d\n",i,row,dst[i],v,col);
				exit(0);
			}
		}
		m+=col;
	}
}
#else
void wtk_mlat_int_vm_raw(int *dst,unsigned char *src,signed char *m,int row,int col)
{
	register int f;
	register unsigned char *psrc,*pe2;
	register signed char *pm;
	unsigned char *pe;
	int i;
	int col2;

	col2=(col>>3)<<3;
	if(col2==col)
	{
		pe2=src+col;
		pm=m;
		for(i=0;i<row;++i)
		{
			f=0;
			psrc=src;
			while(psrc<pe2)
			{
//				f+=psrc[0]*pm[0]+psrc[1]*pm[1]+psrc[2]*pm[2]+psrc[3]*pm[3];
//				f+=psrc[4]*pm[4]+psrc[5]*pm[5]+psrc[6]*pm[6]+psrc[7]*pm[7];
				f+=psrc[0]*pm[0];
				f+=psrc[1]*pm[1];
				f+=psrc[2]*pm[2];
				f+=psrc[3]*pm[3];
				f+=psrc[4]*pm[4];
				f+=psrc[5]*pm[5];
				f+=psrc[6]*pm[6];
				f+=psrc[7]*pm[7];
				psrc+=8;
				pm+=8;
			}
			dst[i]=f;
		}
	}else
	{
		pe2=src+col2;
		pe=src+col;
		pm=m;
		for(i=0;i<row;++i)
		{
			f=0;
			psrc=src;
			while(psrc<pe2)
			{
				f+=psrc[0]*pm[0]+psrc[1]*pm[1]+psrc[2]*pm[2]+psrc[3]*pm[3]+psrc[4]*pm[4]+psrc[5]*pm[5]+psrc[6]*pm[6]+psrc[7]*pm[7];
				psrc+=8;
				pm+=8;
			}
			while(psrc<pe)
			{
				f+=(*(psrc++))*(*(pm++));
			}
			*(dst++)=f;
		}
	}
}
#endif

void wtk_mlat_int_vm_raw2_x(int *dst,int *src,signed char *m,int row,int col)
{
	register int f;
	int i,j;

	for(i=0;i<row;++i)
	{
		f=0;
		for(j=0;j<col;++j)
		{
			f+=src[j]*m[j];
		}
		m+=col;
		dst[i]=f;
	}
}

void wtk_mlat_int_vm_raw2(int *dst,int *src,signed char *m,int row,int col)
{
	register int f,f1,f2,f3;
	register int *psrc,*pe2;
	register signed char *pm;
	int *pe;
	int i;
	int col2;

	col2=(col>>2)<<2;
	pm=m;
	//wtk_debug("col=%d col2=%d\n",col,col2);
	//wtk_debug("src=%p m=%p,dst=%p\n",src,m,dst);
	if(col==col2)
	{
		pe2=src+col2;
		for(i=0;i<row;++i)
		{
			//f=0;
			psrc=src;
			f=f1=f2=f3=0;
			while(psrc<pe2)
			{
//				f+=psrc[0]*pm[0]+psrc[1]*pm[1]+psrc[2]*pm[2]+psrc[3]*pm[3];
//				f+=psrc[4]*pm[4]+psrc[5]*pm[5]+psrc[6]*pm[6]+psrc[7]*pm[7];
				f+=psrc[0]*pm[0];
				f1+=psrc[1]*pm[1];
				f2+=psrc[2]*pm[2];
				f3+=psrc[3]*pm[3];
				psrc+=4;
				pm+=4;
			}
			dst[i]=f+f1+f2+f3;
		}
	}else
	{
		pe2=src+col2;
		pe=src+col;
		for(i=0;i<row;++i)
		{
			psrc=src;
			f=f1=f2=f3=0;
			while(psrc<pe2)
			{
//				f+=psrc[0]*pm[0]+psrc[1]*pm[1]+psrc[2]*pm[2]+psrc[3]*pm[3];
//				f+=psrc[4]*pm[4]+psrc[5]*pm[5]+psrc[6]*pm[6]+psrc[7]*pm[7];
//				psrc+=8;
//				pm+=8;
				f+=psrc[0]*pm[0];
				f1+=psrc[1]*pm[1];
				f2+=psrc[2]*pm[2];
				f3+=psrc[3]*pm[3];
				psrc+=4;
				pm+=4;
			}
			f+=f1+f2+f3;
			while(psrc<pe)
			{
				f+=(*(psrc++))*(*(pm++));
			}
			dst[i]=f;
		}
	}
}

void wtk_mlat_int_vm_raw2_tx(int *dst,int *src,signed char *m,int row,int col)
{
	register int f;
	register int *psrc,*pe2;
	register signed char *pm;
	int *pe;
	int i;
	int col2;

	col2=(col>>3)<<3;
	pm=m;
	//wtk_debug("col=%d col2=%d\n",col,col2);
	//wtk_debug("src=%p m=%p,dst=%p\n",src,m,dst);
	if(col==col2)
	{
		pe2=src+col2;
		for(i=0;i<row;++i)
		{
			//f=0;
			psrc=src;
			f=0;
			while(psrc<pe2)
			{
//				f+=psrc[0]*pm[0]+psrc[1]*pm[1]+psrc[2]*pm[2]+psrc[3]*pm[3];
//				f+=psrc[4]*pm[4]+psrc[5]*pm[5]+psrc[6]*pm[6]+psrc[7]*pm[7];
				f+=psrc[0]*pm[0];
				f+=psrc[1]*pm[1];
				f+=psrc[2]*pm[2];
				f+=psrc[3]*pm[3];
				f+=psrc[4]*pm[4];
				f+=psrc[5]*pm[5];
				f+=psrc[6]*pm[6];
				f+=psrc[7]*pm[7];
				psrc+=8;
				pm+=8;
			}
			dst[i]=f;
		}
	}else
	{
		pe2=src+col2;
		pe=src+col;
		for(i=0;i<row;++i)
		{
			f=0;
			psrc=src;
			while(psrc<pe2)
			{
				f+=psrc[0]*pm[0]+psrc[1]*pm[1]+psrc[2]*pm[2]+psrc[3]*pm[3];
				f+=psrc[4]*pm[4]+psrc[5]*pm[5]+psrc[6]*pm[6]+psrc[7]*pm[7];
				psrc+=8;
				pm+=8;
			}
			while(psrc<pe)
			{
				f+=(*(psrc++))*(*(pm++));
			}
			dst[i]=f;
		}
	}
}

void wtk_mlat_mati_add(wtk_mati_t *a,wtk_mati_t *b)
{
	register int *pa,*epa,*pb;
	int i;

	//wtk_debug("r=%d,c=%d\n",a->row,a->col);
	//wtk_debug("r=%d,c=%d\n",b->row,b->col);
	for(pa=a->p,i=0;i<a->row;++i)
	{
		pb=b->p;
		epa=pa+a->col;
		while(epa-pa>=4)
		{
			*(pa++)+=*(pb++);
			*(pa++)+=*(pb++);
			*(pa++)+=*(pb++);
			*(pa++)+=*(pb++);
		}
		while(epa>pa)
		{
			*(pa++)+=*(pb++);
		}
	}
}

void wtk_mlat_sigmod(wtk_mlat_t *f,wtk_mlat_thread_env_t *env,wtk_vecuc_t *output,wtk_veci_t *input,wtk_mlat_layer_t* l)
{
#define EXP_A (1048576/M_LN2)
#define EXP_C 60801
	register unsigned char *o_p,*o_e;
	register int *i_p;
	float scale;
	wtk_xexp_t *e=&(env->d2i);
	float t1;
	static float t2=(1072693248-EXP_C);
	float max_b=f->cfg->max_b;

	scale=1.0/(f->cfg->max_b*(l->fix_wb->scale));
	o_p=output->p;
	o_e=o_p+output->len;
	i_p=input->p;
	t1=EXP_A*-scale;
	//t2=(1072693248-EXP_C);
	//wtk_debug("len=%d\n",output->len);
	while(o_p<o_e)
	{
		//e->n.i=EXP_A*(-*(i_p++)*scale)+(1072693248-EXP_C);
		e->n.i=*(i_p++)*t1+t2;
		*(o_p++)=(unsigned char)(max_b/(1+e->d));
		//*(o_p++)=(unsigned char)(f->cfg->max_b/(1+wtk_flat_expf(-*(i_p++)*scale)));
	}
}

static void wtk_mlat_veci_scale(wtk_mlat_t *f,wtk_veci_t *m,wtk_mlat_layer_t *d)
{
	float scale;

	//scale=1.0/(127.0/d->fix_wb->max);
	scale=1.0/d->fix_wb->scale;//d->fix_wb->max/(f->cfg->max_w);//127.0);
	wtk_veci_scale(m,scale);
}

void wtk_mlat_process_dnn_fix_layer(wtk_mlat_t *d,
		wtk_mlat_thread_env_t *env,
		wtk_mlat_layer_t *l,
		wtk_veci_t *tmp,
		wtk_vecuc_t *output)
{
	if(l->fix_wb->b)
	{
		wtk_int_add(tmp->p,l->fix_wb->b->p,tmp->len);
	}
	switch(l->type)
	{
	case wtk_fnn_sigmoid:
		wtk_mlat_sigmod(d,env,output,tmp,l);
		//wtk_ann_sigmoid(output[1]+1,n);
		//wtk_matrix_print(output);
		break;
	case wtk_fnn_softmax:
		break;
	case wtk_fnn_linear:
		wtk_mlat_veci_scale(d,tmp,l);
		break;
	default:
        wtk_debug("layer->type not in list. %d\n", l->type);
        break;
	}
}


void wtk_mlat_post_soft_max(wtk_mlat_t *d,wtk_mlat_thread_env_t *env,float *a,int len)
{
#define EXP_A (1048576/M_LN2)
#define EXP_C 60801
	float max,sum;
	float *p,*e;
	float t;
	static float t2=(1072693248-EXP_C);
	wtk_xexp_t *ex=&(env->d2i);

	max=wtk_math_max(a,len);
	sum=0;
	p=a;e=p+len;
	while(p<e)
	{
		ex->n.i=EXP_A*(*p-max)+t2;
		//*p=wtk_flat_expf(*p-max);
		*p=ex->d;
		sum+=*(p++);
		//++p;
	}
	sum=1.0f/sum;
	t=log(sum);
	p=a;//e=p+len;
	while(p<e)
	{
		*p=log(*p)+t;//log(*p*sum);
		++p;
	}
}

#if ! defined(ANDROID) && defined(__arm__) && defined(USE_NEON_ASM)
int wtk_mlat_int_vm_raw2_neon2(int *src_a,int * src_b,int count)
{
  int sse;

  asm volatile (
    // Clear q8, q9, q10, q11
    "veor    q8, q8, q8                            \n"
    "veor    q9, q9, q9                            \n"
    "veor    q10, q10, q10                     \n"
    "veor    q11, q11, q11                     \n"
	"ldr r0,[%[count]] \n"
	"mov r1,%[src_a]\n"
	"mov r2,%[src_b]\n"
	"1: \n"
	"subs r0,r0,#16\n"
	"blt 2f \n"
    "vld1.32     {q1, q2}, [r1]!       \n"
    "vld1.32     {q3, q4}, [r1]!       \n"
    "vld1.32     {q12, q13}, [r2]!  \n"
    "vld1.32     {q14, q15}, [r2]!  \n"
    "vmla.i32   q8, q1, q12                      \n"
    "vmla.i32   q9, q2, q13                     \n"
    "vmla.i32   q10, q3, q14                    \n"
    "vmla.i32   q11, q4, q15                   \n"
	"b 1b \n"
    "2:     \n"
	"vadd.i32   q8, q8, q9                      \n"
	"vadd.i32   q10, q10, q11                \n"
	"vadd.i32   q11, q8, q10                  \n"
	"vpadd.i32  d2, d22, d23                 \n"
	"vpadd.i32  d0, d2, d2                      \n"
	//"vmov.32 r5,d0[0]\n"
	"add r0,r0,#16 \n"
	"3: \n"
	"veor    d1, d1, d1 \n"
	"subs r0,r0,#1 \n"
	"blt 4f \n"
	"vld1.32 {d1[0]},[r1]! \n"
	"vld1.32 {d2[0]},[r2]! \n"
	"vmla.i32 d0,d1,d2[0] \n"
	"b 3b\n"
	"4:\n"
	"vmov.32 %0,d0[0]\n"
    : [sse]"=r"(sse)
    : [src_a]"r"(src_a),
      [src_b]"r"(src_b),
      [count]"r"(&count)
    : "memory", "cc","r0","r1","r2","q1", "q2", "q3","q4","q8", "q9", "q10", "q11",
      "q12", "q13","q14", "q15");
 // printf("t1=%p src_a=%p\n",t1,src_a);
  return sse;
}

void wtk_mlat_int_vm_raw_x(int *dst,int *src,int *m,int row,int col)
{
	int i;
	//float *t=m;

	for(i=0;i<row;++i)
	{
		//wtk_debug("v[%d/%d] src=%p m=%p %f\n",i,row,src,m,dst[i]);
		dst[i]=wtk_mlat_int_vm_raw2_neon2(src,m,col);
		//wtk_debug("src2=%p m=%p/%p\n",src,m,t+i*col);
		m+=col;
		//printf("v[%d/%d] src=%p m=%p %d\n",i,row,src,m,dst[i]);
		//src-=col;
		//src-=col;
		//wtk_debug("v[%d/%d] src=%p m=%p %f\n",i,row,src,m,dst[i]);
		//src-=col;
	}
}
#else

void wtk_mlat_int_vm_raw_x(int *dst,int *src,int *m,int row,int col)
{
	register int f;
	register int *psrc,*pe2;
	register int *pm;
	int *pe;
	int i;
	int col2;

	col2=(col>>3)<<3;
	pe2=src+col2;
	pe=src+col;
	pm=m;
	//wtk_debug("col=%d col2=%d\n",col,col2);
	//wtk_debug("src=%p m=%p,dst=%p\n",src,m,dst);
	if(pe==pe2)
	{
		for(i=0;i<row;++i)
		{
			f=0;
			psrc=src;
			while(psrc<pe2)
			{
				f+=psrc[0]*pm[0]+psrc[1]*pm[1]+psrc[2]*pm[2]+psrc[3]*pm[3];
				f+=psrc[4]*pm[4]+psrc[5]*pm[5]+psrc[6]*pm[6]+psrc[7]*pm[7];
				psrc+=8;
				pm+=8;
			}
			dst[i]=f;
		}
	}else
	{
		for(i=0;i<row;++i)
		{
			f=0;
			psrc=src;
			while(psrc<pe2)
			{
				f+=psrc[0]*pm[0]+psrc[1]*pm[1]+psrc[2]*pm[2]+psrc[3]*pm[3];
				f+=psrc[4]*pm[4]+psrc[5]*pm[5]+psrc[6]*pm[6]+psrc[7]*pm[7];
				psrc+=8;
				pm+=8;
			}
			while(psrc<pe)
			{
				f+=(*(psrc++))*(*(pm++));
			}
			dst[i]=f;
		}
	}
}
#endif

void wtk_mlat_sigmod2(wtk_mlat_t *f,wtk_veci_t *output,wtk_veci_t *input,wtk_mlat_layer_t* l)
{
	int *o_p,*o_e;
	int *i_p;
	float scale;

	scale=1.0/(f->cfg->max_b*(l->fix_wb->scale));
	o_p=output->p;
	o_e=o_p+output->len;
	i_p=input->p;
	while(o_p<o_e)
	{
		*(o_p++)=(int)(f->cfg->max_b/(1+wtk_flat_expf(-*(i_p++)*scale)));
	}
}

void wtk_mlat_process_dnn_fix_layer2(wtk_mlat_t *d,wtk_mlat_layer_t *l,
		wtk_veci_t *tmp,
		wtk_veci_t *output)
{
	if(l->fix_wb->b)
	{
		wtk_int_add(tmp->p,l->fix_wb->b->p,tmp->len);
	}
	switch(l->type)
	{
	case wtk_fnn_sigmoid:
		wtk_mlat_sigmod2(d,output,tmp,l);
		//wtk_ann_sigmoid(output[1]+1,n);
		//wtk_matrix_print(output);
		break;
	case wtk_fnn_softmax:
		break;
	case wtk_fnn_linear:
		wtk_mlat_veci_scale(d,tmp,l);
		break;
	default:
        wtk_debug("layer->type not in list. %d\n", l->type);
        break;
	}
}

#define WTK_MLAT_INIT(dst,dst_s,dst_e,dst_e2,m) \
dst_s=dst; \
while(dst_s<dst_e) \
{\
	dst_s[0]=m[0]; \
	dst_s[1]=m[1]; \
	dst_s[2]=m[2]; \
	dst_s[3]=m[3]; \
	dst_s[4]=m[4]; \
	dst_s[5]=m[5]; \
	dst_s[6]=m[6]; \
	dst_s[7]=m[7];\
	m+=8; \
	dst_s+=8;\
}\
while(dst_s<dst_e2) \
{\
	*(dst_s++)=(*(m++));\
}


void wtk_mlat_int_vm_raw2_transpose(int *dst,register int *src,register signed char *m,int row,int col)
{
	register int f;
	int i;
	int col2;
	register int *dst_s,*dst_e;
	int *dst_e2;

	col2=(col>>3)<<3;
	dst_e=dst+col2;
	dst_e2=dst+col;
	f=src[0];
	switch(f)
	{
	case 0:
		memset(dst,0,sizeof(int)*col);
		m+=col;
		break;
	case 1:
		WTK_MLAT_INIT(dst,dst_s,dst_e,dst_e2,m);
		break;
	case 2:
		WTK_MLAT_INIT_SHIFT(dst,dst_s,dst_e,dst_e2,m,1);
		break;
	case 4:
		WTK_MLAT_INIT_SHIFT(dst,dst_s,dst_e,dst_e2,m,2);
		break;
	case 8:
		WTK_MLAT_INIT_SHIFT(dst,dst_s,dst_e,dst_e2,m,3);
		break;
	case 16:
		WTK_MLAT_INIT_SHIFT(dst,dst_s,dst_e,dst_e2,m,4);
		break;
	case 32:
		WTK_MLAT_INIT_SHIFT(dst,dst_s,dst_e,dst_e2,m,5);
		break;
	case 64:
		WTK_MLAT_INIT_SHIFT(dst,dst_s,dst_e,dst_e2,m,6);
		break;
	case 128:
		WTK_MLAT_INIT_SHIFT(dst,dst_s,dst_e,dst_e2,m,7);
		break;
	default:
		dst_s=dst;
		while(dst_s<dst_e)
		{
			dst_s[0]=f*m[0];
			dst_s[1]=f*m[1];
			dst_s[2]=f*m[2];
			dst_s[3]=f*m[3];
			dst_s[4]=f*m[4];
			dst_s[5]=f*m[5];
			dst_s[6]=f*m[6];
			dst_s[7]=f*m[7];
			m+=8;
			dst_s+=8;
		}
		while(dst_s<dst_e2)
		{
			*(dst_s++)=f*(*(m++));
		}
		break;
	}
	for(i=1;i<row;++i)
	{
		f=src[i];
		switch(f)
		{
		case 0:
			m+=col;
			break;
		case 1:
			WTK_MLAT_ADD(dst,dst_s,dst_e,dst_e2,m);
			break;
		case 2:
			WTK_MLAT_ADD_SHIFT(dst,dst_s,dst_e,dst_e2,m,1);
			break;
		case 4:
			WTK_MLAT_ADD_SHIFT(dst,dst_s,dst_e,dst_e2,m,2);
			break;
		case 8:
			WTK_MLAT_ADD_SHIFT(dst,dst_s,dst_e,dst_e2,m,3);
			break;
		case 16:
			WTK_MLAT_ADD_SHIFT(dst,dst_s,dst_e,dst_e2,m,4);
			break;
		case 32:
			WTK_MLAT_ADD_SHIFT(dst,dst_s,dst_e,dst_e2,m,5);
			break;
		case 64:
			WTK_MLAT_ADD_SHIFT(dst,dst_s,dst_e,dst_e2,m,6);
			break;
		case 128:
			WTK_MLAT_ADD_SHIFT(dst,dst_s,dst_e,dst_e2,m,7);
			break;
		default:
			dst_s=dst;
			while(dst_s<dst_e)
			{
				dst_s[0]+=f*m[0];
				dst_s[1]+=f*m[1];
				dst_s[2]+=f*m[2];
				dst_s[3]+=f*m[3];
				dst_s[4]+=f*m[4];
				dst_s[5]+=f*m[5];
				dst_s[6]+=f*m[6];
				dst_s[7]+=f*m[7];
				m+=8;
				dst_s+=8;
			}
			while(dst_s<dst_e2)
			{
				*(dst_s++)+=f*(*(m++));
			}
			break;
		}
	}
}

void wtk_mlat_int_vm_raw2_transpose_raw(int *dst,int *src,signed char *m,int row,int col)
{
	register int f;
	int i,j;

	for(i=0;i<row;++i)
	{
		f=src[i];
		if(i==0)
		{
			for(j=0;j<col;++j)
			{
				dst[j]=f*m[j];
			}
		}else
		{
			for(j=0;j<col;++j)
			{
				dst[j]+=f*m[j];
			}
		}
		m+=col;
	}
}

void wtk_mlat_int_vm_raw_transpose(int *dst,unsigned char *src,register signed char *m,int row,int col)
{
	register unsigned char f;
	int i;
	int col2;
	register int *dst_s,*dst_e;
	int *dst_e2;

	col2=(col>>3)<<3;
	dst_e=dst+col2;
	dst_e2=dst+col;
	f=src[0];
	switch(f)
	{
	case 0:
		memset(dst,0,sizeof(int)*col);
		m+=col;
		break;
	case 1:
		WTK_MLAT_INIT(dst,dst_s,dst_e,dst_e2,m);
		break;
	case 2:
		WTK_MLAT_INIT_SHIFT(dst,dst_s,dst_e,dst_e2,m,1);
		break;
	case 4:
		WTK_MLAT_INIT_SHIFT(dst,dst_s,dst_e,dst_e2,m,2);
		break;
	case 8:
		WTK_MLAT_INIT_SHIFT(dst,dst_s,dst_e,dst_e2,m,3);
		break;
	case 16:
		WTK_MLAT_INIT_SHIFT(dst,dst_s,dst_e,dst_e2,m,4);
		break;
	case 32:
		WTK_MLAT_INIT_SHIFT(dst,dst_s,dst_e,dst_e2,m,5);
		break;
	case 64:
		WTK_MLAT_INIT_SHIFT(dst,dst_s,dst_e,dst_e2,m,6);
		break;
	case 128:
		WTK_MLAT_INIT_SHIFT(dst,dst_s,dst_e,dst_e2,m,7);
		break;
	default:
		dst_s=dst;
		while(dst_s<dst_e)
		{
			dst_s[0]=f*m[0];
			dst_s[1]=f*m[1];
			dst_s[2]=f*m[2];
			dst_s[3]=f*m[3];
			dst_s[4]=f*m[4];
			dst_s[5]=f*m[5];
			dst_s[6]=f*m[6];
			dst_s[7]=f*m[7];
			m+=8;
			dst_s+=8;
		}
		while(dst_s<dst_e2)
		{
			*(dst_s++)=f*(*(m++));
		}
		break;
	}
	for(i=1;i<row;++i)
	{
		f=src[i];
		switch(f)
		{
		case 0:
			m+=col;
			break;
		case 1:
			WTK_MLAT_ADD(dst,dst_s,dst_e,dst_e2,m);
			break;
		case 2:
			WTK_MLAT_ADD_SHIFT(dst,dst_s,dst_e,dst_e2,m,1);
			break;
		case 4:
			WTK_MLAT_ADD_SHIFT(dst,dst_s,dst_e,dst_e2,m,2);
			break;
		case 8:
			WTK_MLAT_ADD_SHIFT(dst,dst_s,dst_e,dst_e2,m,3);
			break;
		case 16:
			WTK_MLAT_ADD_SHIFT(dst,dst_s,dst_e,dst_e2,m,4);
			break;
		case 32:
			WTK_MLAT_ADD_SHIFT(dst,dst_s,dst_e,dst_e2,m,5);
			break;
		case 64:
			WTK_MLAT_ADD_SHIFT(dst,dst_s,dst_e,dst_e2,m,6);
			break;
		case 128:
			WTK_MLAT_ADD_SHIFT(dst,dst_s,dst_e,dst_e2,m,7);
			break;
		default:
			dst_s=dst;
			while(dst_s<dst_e)
			{
				dst_s[0]+=f*m[0];
				dst_s[1]+=f*m[1];
				dst_s[2]+=f*m[2];
				dst_s[3]+=f*m[3];
				dst_s[4]+=f*m[4];
				dst_s[5]+=f*m[5];
				dst_s[6]+=f*m[6];
				dst_s[7]+=f*m[7];
				m+=8;
				dst_s+=8;
			}
			while(dst_s<dst_e2)
			{
				*(dst_s++)+=f*(*(m++));
			}
			break;
		}
	}
}

void wtk_mlat_int_vm_raw_transpose_raw(int *dst,unsigned char *src,signed char *m,int row,int col)
{
	register int f;
	int i,j;

	for(i=0;i<row;++i)
	{
		f=src[i];
		if(i==0)
		{
			for(j=0;j<col;++j)
			{
				dst[j]=f*m[j];
			}
		}else
		{
			for(j=0;j<col;++j)
			{
				dst[j]+=f*m[j];
			}
		}
		m+=col;
	}
}

void wtk_mlat_calc_char_fixpoint(wtk_mlat_thread_env_t *env,wtk_mlat_feature_t *mf)
{
	wtk_mlat_t *d=env->mlat;
	wtk_queue_node_t *n;
	wtk_mlat_layer_t *l;
	wtk_mlat_fix_env_t *fix=env->fix;
	int uc=1;
	wtk_mlat_fix_env_veci_t *env_vec,*last_env_vec;
	wtk_mlat_fix_env_vecuc_t *env_uc,*last_env_uc;
	wtk_matc_t *matc;
	wtk_vecuc_t *output_uc=NULL;
	wtk_veci_t *output_i=NULL;

	//wtk_debug("[%d]\n",fix->layer0_output->len);
	wtk_mlat_trans_process(mf,d->cfg->trans);
	n=d->cfg->layer_q.pop;
	l=data_offset(n,wtk_mlat_layer_t,q_n);
	//wtk_debug("calc char fixpoint type=%d\n",l->type);
	//wtk_debug("[%d/%d]\n",fix->layer0_output->len,mf->v->len);
	wtk_mlat_process_dnn_layer(d,l,mf->v,fix->layer0);
	//wtk_debug("[%d]\n",fix->layer0_output->len);
	//exit(0);
	output_uc=fix->layer0_output.u;
	wtk_vecuc_init(output_uc,fix->layer0->p,d->cfg->max_b);
	//wtk_debug("[%d]\n",output_uc->len);
	//exit(0);
	last_env_vec=NULL;
	last_env_uc=NULL;
	for(n=n->next;n;n=n->next)
	{
		l=data_offset(n,wtk_mlat_layer_t,q_n);
		env_vec=wtk_mlat_fix_env_get_vec(fix,l->fix_wb->b->col);
		matc=l->fix_wb->w.c;
		//wtk_debug("col=%d/%d\n",matc->row,matc->col);
		if(d->cfg->use_transpose)
		{
			if(uc)
			{
				//wtk_debug("%d/%d\n",env_vec->v->len,output_uc->len);
				wtk_mlat_int_vm_raw_transpose(env_vec->v->p,output_uc->p,matc->p,matc->row,matc->col);
			}else
			{
				wtk_mlat_int_vm_raw2_transpose(env_vec->v->p,output_i->p,matc->p,matc->row,matc->col);
			}
		}else
		{
			if(uc)
			{
				//wtk_debug("%d/%d\n",env_vec->v->len,output_uc->len);
				wtk_mlat_int_vm_raw(env_vec->v->p,output_uc->p,matc->p,matc->row,matc->col);
			}else
			{
				wtk_mlat_int_vm_raw2(env_vec->v->p,output_i->p,matc->p,matc->row,matc->col);
			}
		}
		if(l->type==wtk_fnn_sigmoid)
		{
			env_uc=wtk_mlat_fix_env_get_vecuc(fix,l->fix_wb->b->col);
			output_uc=env_uc->v;
		}else
		{
			env_uc=NULL;
			output_uc=NULL;
		}
		wtk_mlat_process_dnn_fix_layer(d,env,l,env_vec->v,output_uc);
		if(last_env_vec)
		{
			wtk_mlat_fix_env_push_vec(fix,last_env_vec);
		}
		last_env_vec=env_vec;
		if(last_env_uc)
		{
			//wtk_debug("uc=%p/%p\n",last_env_uc,env_uc);
			wtk_mlat_fix_env_push_vecuc(fix,last_env_uc);
		}
		//wtk_debug("uc=%p/%p\n",last_env_uc,env_uc);
		last_env_uc=env_uc;
		switch(l->type)
		{
		case wtk_fnn_sigmoid:
			uc=1;
			break;
		case wtk_fnn_relu:
			//TODO
			break;
		case wtk_fnn_softmax:
			uc=0;
			output_i=env_vec->v;//d->fix_tmp[i-1];
			//if(!d->dnn->cfg->use_linear_output && !n->next)
			{
				double scale;
				float *p,*pe;
				int *pi;

				pi=output_i->p;
				scale=1.0/(l->fix_wb->scale*d->cfg->max_b);
				p=mf->f->dnn_v;
				pe=p+output_i->len;//wtk_vector_size(f->dnn_v);
				while(p<pe)
				{
					*(++p)=*(pi++)*scale;//output_i->p;
					//wtk_debug("%f/%f\n",*(p),l->fix_wb->max);
				}
				if(!d->dnn->cfg->use_linear_output || !n->next)
				{
					wtk_mlat_post_soft_max(d,env,mf->f->dnn_v+1,output_i->len);
				}
				//print_float(mf->f->dnn_v+1,10);
			}
			break;
		case wtk_fnn_linear:
			uc=0;
			output_i=env_vec->v;//d->fix_tmp[i-1];
			break;
		case wtk_fnn_rescale:
			break;
		case wtk_fnn_pnorm:
			break;
		default:
			break;
		}
	}
	if(last_env_vec)
	{
		wtk_mlat_fix_env_push_vec(fix,last_env_vec);
	}
	if(last_env_uc)
	{
		wtk_mlat_fix_env_push_vecuc(fix,last_env_uc);
	}
//	wtk_debug("vec=%d/%d\n",fix->veci_q.length,fix->vecuc_q.length);
//	exit(0);
}


void wtk_mlat_calc_int_fixpoint(wtk_mlat_thread_env_t *env,wtk_mlat_feature_t *mf)
{
	wtk_mlat_t *d=env->mlat;
	wtk_queue_node_t *n;
	wtk_mlat_layer_t *l;
	wtk_mlat_fix_env_t *fix=env->fix;
	wtk_mlat_fix_env_veci_t *env_vec,*last_env_vec;
	wtk_mati_t *matc;
	wtk_veci_t *output_i=NULL;
	wtk_veci_t *output_uc=NULL;
	wtk_mlat_fix_env_veci_t *env_uc,*last_env_uc=NULL;

	//wtk_debug("[%d]\n",fix->layer0_output->len);
	wtk_mlat_trans_process(mf,d->cfg->trans);
	n=d->cfg->layer_q.pop;
	l=data_offset(n,wtk_mlat_layer_t,q_n);
	//wtk_debug("calc char fixpoint type=%d\n",l->type);
	//wtk_debug("[%d/%d]\n",fix->layer0_output->len,mf->v->len);
	wtk_mlat_process_dnn_layer(d,l,mf->v,fix->layer0);
	//wtk_debug("[%d]\n",fix->layer0_output->len);
	//exit(0);
	//wtk_vecuc_init(fix->layer0_output,fix->layer0->p,d->cfg->max_b);
	output_i=fix->layer0_output.i;
	wtk_veci_init(output_i,fix->layer0->p,d->cfg->max_b);
	//wtk_debug("[%d]\n",output_uc->len);
	//exit(0);
	last_env_vec=NULL;
	for(n=n->next;n;n=n->next)
	{
		l=data_offset(n,wtk_mlat_layer_t,q_n);
		env_vec=wtk_mlat_fix_env_get_vec(fix,l->fix_wb->b->col);
		matc=l->fix_wb->w.i;
		//wtk_debug("col=%d/%d\n",matc->row,matc->col);
		wtk_mlat_int_vm_raw_x(env_vec->v->p,output_i->p,matc->p,matc->row,matc->col);
		if(l->type==wtk_fnn_sigmoid)
		{
			env_uc=wtk_mlat_fix_env_get_vec(fix,l->fix_wb->b->col);
			output_uc=env_uc->v;
		}else
		{
			env_uc=NULL;
			output_uc=NULL;
		}
		wtk_mlat_process_dnn_fix_layer2(d,l,env_vec->v,output_uc);
		if(last_env_vec)
		{
			wtk_mlat_fix_env_push_vec(fix,last_env_vec);
		}
		last_env_vec=env_vec;
		if(last_env_uc)
		{
			//wtk_debug("uc=%p/%p\n",last_env_uc,env_uc);
			wtk_mlat_fix_env_push_vec(fix,last_env_uc);
		}
		//wtk_debug("uc=%p/%p\n",last_env_uc,env_uc);
		last_env_uc=env_uc;
		switch(l->type)
		{
		case wtk_fnn_sigmoid:
			output_i=last_env_uc->v;
			break;
		case wtk_fnn_relu:
			//TODO
			break;
		case wtk_fnn_softmax:
			output_i=env_vec->v;//d->fix_tmp[i-1];
			//if(!d->dnn->cfg->use_linear_output && !n->next)
			{
				double scale;
				float *p,*pe;
				int *pi;

				pi=output_i->p;
				scale=1.0/(l->fix_wb->scale*d->cfg->max_b);
				p=mf->f->dnn_v;
				pe=p+output_i->len;//wtk_vector_size(f->dnn_v);
				while(p<pe)
				{
					*(++p)=*(pi++)*scale;//output_i->p;
					//wtk_debug("%f/%f\n",*(p),l->fix_wb->max);
				}
				if(!d->dnn->cfg->use_linear_output || !n->next)
				{
					wtk_mlat_post_soft_max(d,env,mf->f->dnn_v+1,output_i->len);
				}
				//print_float(mf->f->dnn_v+1,10);
			}
			break;
		case wtk_fnn_linear:
			output_i=env_vec->v;//d->fix_tmp[i-1];
			break;
		case wtk_fnn_rescale:
			break;
		case wtk_fnn_pnorm:
			break;
		default:
			break;
		}
	}
	if(last_env_vec)
	{
		wtk_mlat_fix_env_push_vec(fix,last_env_vec);
	}
	if(last_env_uc)
	{
		//wtk_debug("uc=%p/%p\n",last_env_uc,env_uc);
		wtk_mlat_fix_env_push_vec(fix,last_env_uc);
	}
	//exit(0);
	//wtk_debug("vec=%d\n",fix->veci_q.length);
//	exit(0);
}

void wtk_mlat_calc_layer(wtk_mlat_thread_env_t *m,wtk_mlat_feature_t *mf)
{
	if(m->mlat->cfg->use_fix_res)
	{
		if(m->mlat->cfg->use_int)
		{
			wtk_mlat_calc_int_fixpoint(m,mf);
		}else
		{
			wtk_mlat_calc_char_fixpoint(m,mf);
		}
	}else
	{
		wtk_mlat_process_layer_matrix(m,mf);
	}
}

int wtk_mlat_thread_env_run(wtk_mlat_thread_env_t *m,wtk_thread_t *t)
{
	wtk_mlat_msg_t *msg;
	wtk_mlat_feature_t *mf;
	wtk_queue_node_t *qn;
	int wait=1;

	//wtk_debug("run thread %d\n",t->ppid);
	while(m->mlat->run)
	{
		if(wait)
		{
			qn=wtk_blockqueue_pop(&(m->msg_q),-1,NULL);
			if(!qn){continue;}
			msg=data_offset2(qn,wtk_mlat_msg_t,q_n);
			wait=0;
			wtk_mlat_push_msg(m->mlat,msg);
		}else
		{
			qn=wtk_blockqueue_pop(&(m->mlat->input_q),-1,NULL);
			if(!qn){continue;}
			msg=data_offset2(qn,wtk_mlat_msg_t,q_n);
			//wtk_debug("process feature=%d\n",mf->f->index);
			if(msg->hook)
			{
				mf=(wtk_mlat_feature_t*)(msg->hook);
				wtk_mlat_calc_layer(m,mf);
				msg->hook=mf->f;
				//wtk_debug("thread[%d] feature=%d/%p used=%d\n",t->ppid,mf->f->index,mf->f,mf->f->used);
				wtk_mlat_push_feature(m->mlat,mf);
			}else
			{
				//wtk_debug("thread %d get end\n",t->ppid);
				wait=1;
			}
			wtk_blockqueue_push(&(m->mlat->output_q),qn);
		}
	}
	wtk_debug("run thread %d end\n",t->ppid);
	return 0;
}

void wtk_mlat_feature_to_matrix(wtk_mlat_t *m,wtk_feat_t **pv,int npv,wtk_mlat_feature_t *mf)
{
	wtk_vector_t *v;
	int i,j,n;
	float *p;
	int k;

	n=wtk_vector_size(pv[0]->v);
	p=mf->v->p;
	for(i=0;i<npv;++i)
	{
		v=pv[i]->v;
		//wtk_debug("i=%d/%d\n",i,npv);
		for(k=i,j=1;j<=n;++j,k+=npv)
		{
			p[k]=v[j];
		}
	}
}

void wtk_mlat_feed_dnn_thread_start(wtk_mlat_t *m)
{
	wtk_mlat_msg_t *msg;
	int i;

	m->route_run=1;
	for(i=0;i<m->nthread;++i)
	{
		msg=wtk_mlat_pop_msg(m);
		msg->hook=NULL;
		wtk_blockqueue_push(&(m->thread_env[i]->msg_q),&(msg->q_n));
	}
}

void wtk_mlat_feed_dnn_thread_end(wtk_mlat_t *m)
{
	wtk_mlat_msg_t *msg;
	int i;

	for(i=0;i<m->nthread;++i)
	{
		msg=wtk_mlat_pop_msg(m);
		msg->hook=NULL;
		wtk_blockqueue_push(&(m->input_q),&(msg->q_n));
	}
	wtk_sem_acquire(&(m->wait_sem),-1);
	m->route_run=0;
}

void wtk_mlat_process_layer(wtk_mlat_t *d,wtk_feat_t **pv,int npv,wtk_feat_t *f)
{
	wtk_mlat_msg_t *msg;
	wtk_mlat_feature_t *mf;

	if(f->index==1)
	{
		d->index=1;
		wtk_mlat_feed_dnn_thread_start(d);
	}else
	{
		++d->index;
	}
	//wtk_debug("process index=%d/%p used=%d\n",f->index,f,f->used);
//	if(d->index!=f->index)
//	{
//		wtk_debug("found bug[%d/%d]\n",d->index,f->index);
//		exit(0);
//	}
	msg=wtk_mlat_pop_msg(d);
	++f->used;
	if((d->cfg->skip_frame==0) || (f->index%d->dnn->cfg->skip_frame==1))
	{
		mf=wtk_mlat_pop_feature(d);
		mf->f=f;
		wtk_mlat_feature_to_matrix(d,pv,npv,mf);
		msg->hook=mf;
		//wtk_debug("push messg=%d\n",f->index);
		wtk_blockqueue_push(&(d->input_q),&(msg->q_n));
	}else
	{
		msg->hook=f;
		wtk_blockqueue_push(&(d->output_q),&(msg->q_n));
	}
	//exit(0);
}

int wtk_feature_cmp_index(wtk_queue_node_t *n1,wtk_queue_node_t *n2)
{
	wtk_feat_t *f1,*f2;

	f1=data_offset2(n1,wtk_feat_t,queue_n);
	f2=data_offset2(n2,wtk_feat_t,queue_n);
	return f2->index-f1->index;
}

typedef struct
{
	wtk_queue_t q;
	wtk_queue_t feature_q;
	wtk_feat_t *last_f;
	int index;
	int cnt;
	int skip;
	wtk_mlat_t *m;
}wtk_mlat_output_env_t;

void wtk_mlat_output_env_init(wtk_mlat_output_env_t *env)
{
	wtk_queue_init(&(env->q));
	wtk_queue_init(&(env->feature_q));
	env->last_f=NULL;
	env->index=1;
	env->cnt=0;
}

void wtk_mlat_output_env_raise_feature(wtk_mlat_output_env_t *env,wtk_feat_t *f);

void wtk_mlat_raise_feautre(wtk_mlat_t *m,wtk_feat_t *f)
{
	//++m->raise_index;
//	if(m->raise_index!=f->index)
//	{
//		wtk_debug("found bug index %d/%d\n",m->raise_index,f->index);
//		exit(0);
//	}
	//wtk_debug("riase index=%d/%p used=%d\n",f->index,f,f->used);
	wtk_fnn_raise_feature(m->dnn,f);
}

void wtk_mlat_output_env_flush(wtk_mlat_output_env_t *env)
{
	wtk_queue_node_t *qn;
	wtk_feat_t *f;

	while(env->q.length>0)
	{
		qn=wtk_queue_peek(&(env->q),0);
		if(!qn){break;}
		f=data_offset2(qn,wtk_feat_t,queue_n);
		//wtk_debug("index=%d/%d\n",f->index,env->index);
		if(f->index==env->index)
		{
			++env->index;
			++env->cnt;
			f->app_hook=env->last_f;
			++env->last_f->used;
			wtk_queue_pop(&(env->q));
			if(env->cnt>=env->m->cfg->skip_frame)
			{
				--env->last_f->used;
				wtk_fextra_push_feature(env->m->dnn->parm,env->last_f);
				env->last_f=NULL;
				--f->used;
				wtk_mlat_raise_feautre(env->m,f);
				break;
			}else
			{
				--f->used;
				wtk_mlat_raise_feautre(env->m,f);
			}
		}else
		{
			break;
		}
	}
	if(!env->last_f && env->feature_q.length>0)
	{
		qn=wtk_queue_peek(&(env->feature_q),0);
		f=data_offset2(qn,wtk_feat_t,queue_n);
		if(f->index==env->index)
		{
			wtk_queue_pop(&(env->feature_q));
			wtk_mlat_output_env_raise_feature(env,f);
		}
	}
}

void wtk_mlat_output_env_flush2(wtk_mlat_output_env_t *env)
{
	wtk_queue_node_t *qn;
	wtk_feat_t *f;
	wtk_queue_t *q=&(env->feature_q);

	while(q->length>0)
	{
		qn=wtk_queue_peek(q,0);
		if(!qn){break;}
		f=data_offset2(qn,wtk_feat_t,queue_n);
		//wtk_debug("index=%d/%d\n",f->index,env->index);
		if(f->index==env->index)
		{
			++env->index;
			wtk_queue_pop(q);
			//--f->used;
			//wtk_parm_dec_feature_use(env->m->dnn->parm,f);
			wtk_mlat_raise_feautre(env->m,f);
		}else
		{
			break;
		}
	}
}

void wtk_mlat_print_env(wtk_mlat_output_env_t *env)
{
	wtk_queue_node_t *qn;
	wtk_feat_t *f;

	for(qn=env->q.pop;qn;qn=qn->next)
	{
		f=data_offset2(qn,wtk_feat_t,queue_n);
		wtk_debug("env[%d]\n",f->index);
	}
	for(qn=env->feature_q.pop;qn;qn=qn->next)
	{
		f=data_offset2(qn,wtk_feat_t,queue_n);
		wtk_debug("feat %d/%p used=%d\n",f->index,f,f->used);
	}
}

void wtk_mlat_output_env_raise_feature(wtk_mlat_output_env_t *env,wtk_feat_t *f)
{
	if(f->index==env->index)
	{
		if(env->skip==0)
		{
			//--f->used;
			// wtk_parm_dec_feature_use(env->m->dnn->parm,f);
		}
		//wtk_debug("raise feature=%d\n",f->index);
		wtk_mlat_raise_feautre(env->m,f);
		if(env->skip==0)
		{
			++env->index;
			//wtk_debug("skip=%d %d lenth=%d\n",env->index,env->skip,env->feature_q.length);
			wtk_mlat_output_env_flush2(env);
		}else
		{
			env->last_f=f;
			++env->index;
			env->cnt=1;
			wtk_mlat_output_env_flush(env);
		}
	}else
	{
//		{
//			wtk_queue_node_t *qn;
//
//			qn=wtk_queue_find_node(&(env->feature_q),&(f->queue_n));
//			if(qn)
//			{
//				wtk_debug("=========================> found bug %d/%p used=%d\n",f->index,f,f->used);
//				wtk_mlat_print_env(env);
//				getchar();
//				getchar();
//				getchar();
//				getchar();
//			}
//		}
		wtk_queue_insert(&(env->feature_q),&(f->queue_n),(wtk_cmp_handler_t)wtk_feature_cmp_index);
	}
}



int wtk_mlat_thread_run(wtk_mlat_t *m,wtk_thread_t *t)
{
	wtk_mlat_msg_t *msg;
	wtk_feat_t *f;
	wtk_queue_node_t *qn;
	wtk_mlat_output_env_t env;
	int thread=0;

	//wtk_debug("run output thread=%d\n",t->ppid);
	wtk_mlat_output_env_init(&(env));
	env.m=m;
	env.skip=m->cfg->skip_frame>0;
	m->raise_index=0;
	while(m->run)
	{
		qn=wtk_blockqueue_pop(&(m->output_q),-1,NULL);
		if(!qn){continue;}
		msg=data_offset2(qn,wtk_mlat_msg_t,q_n);
		if(msg->hook)
		{
			f=(wtk_feat_t*)(msg->hook);
			if((env.skip==0) || ((f->index%m->cfg->skip_frame)==1))
			{
				//wtk_debug("raise feature=%d/%p used=%d feature=%d   %d %d/%d\n",f->index,f,f->used,env.feature_q.length,env.index,m->input_q.length,m->output_q.length);
				//exit(0);
//				if(f->used==0)
//				{
//					wtk_debug("foudn bug=================\n\n");
//					getchar();
//					getchar();
//				}
				wtk_mlat_output_env_raise_feature(&env,f);
			}else
			{
				wtk_queue_insert(&(env.q),&(f->queue_n),(wtk_cmp_handler_t)wtk_feature_cmp_index);
				if(env.last_f && f->index==env.index)
				{
					wtk_mlat_output_env_flush(&env);
				}
			}
		}else
		{
			++thread;
			if(thread==m->nthread)
			{
				if(env.q.length>0 || env.feature_q.length>0)
				{
					wtk_debug("==========>  feed t=%d env=%d feature=%d index=%d/%d output=%d input=%d\n",thread,env.q.length,env.feature_q.length,
							m->index,m->raise_index,m->output_q.length,m->input_q.length);
					wtk_mlat_print_env(&(env));
					//exit(0);
				}
				//wtk_debug("================== notify end...........\n");
				wtk_sem_release(&(m->wait_sem),1);
				wtk_mlat_output_env_init(&(env));
				thread=0;
				m->raise_index=0;
			}
		}
		wtk_mlat_push_msg(m,msg);
		//wtk_debug("msg=%d %d/%d\n",m->feature_qn_pool->pool->cache->used,m->feature_qn_pool->pool->hoard.use_length,m->feature_qn_pool->pool->hoard.cur_free);
	}
	wtk_debug("run output thread end=%d\n",t->ppid);
	return 0;
}

void wtk_mlat_wait_end(wtk_mlat_t *m)
{
	//double t;

	if(!m->route_run)
	{
		return;
	}
//	wtk_debug("========= feed %d/%d ==========\n",m->input_q.length,m->output_q.length);
	wtk_mlat_feed_dnn_thread_end(m);
//	wtk_debug("========= feed end %d/%d feature=%d/%d qn=%d/%d ==========\n",m->input_q.length,m->output_q.length,
//			m->feature_hoard.use_length,m->feature_hoard.cur_free,
//			m->feature_qn_pool->pool->hoard.use_length,m->feature_qn_pool->pool->hoard.cur_free);
//	wtk_debug("parm feature=%d/%d\n",m->dnn->parm->feature_hoard.use_length,m->dnn->parm->feature_hoard.cur_free);
}

void wtk_mlat_flush(wtk_mlat_t *f)
{
//	wtk_debug("flush\n");
//	exit(0);
}

void wtk_mlat_flush_end(wtk_mlat_t *f)
{
//	wtk_debug("flush end\n");
//	exit(0);
}
