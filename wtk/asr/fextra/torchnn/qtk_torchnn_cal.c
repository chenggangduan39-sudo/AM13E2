#include "qtk_torchnn_cal.h"
#include "wtk/core/math/wtk_matrix.h"
#include <float.h>
qtk_torchnn_conv2d_t* qtk_torchnn_conv2d_new(int ker_row,int ker_col)
{
	qtk_torchnn_conv2d_t *conv2d = (qtk_torchnn_conv2d_t*)wtk_malloc(sizeof(qtk_torchnn_conv2d_t));

	conv2d->kernel_row = ker_row;
	conv2d->kernel_col = ker_col;
	conv2d->bias = NULL;
	conv2d->weight = NULL;

	return conv2d;
}

void qtk_torchnn_conv2d_delete(qtk_torchnn_conv2d_t *conv)
{
	qtk_blas_matrix_delete(conv->weight);
	if(conv->bias)
	{
		qtk_blas_matrix_delete(conv->bias);
	}
	qtk_blas_matrix_delete(conv->trans);
}

int qtk_torchnn_conv2d_write(void *mdl,FILE *f){

	int ret = 0,len;
	qtk_torchnn_conv2d_t *conv2d = (qtk_torchnn_conv2d_t*)mdl;

	ret = fwrite(&(conv2d->i),4,1,f);
	ret = fwrite(&(conv2d->j),4,1,f);
	ret = fwrite(&(conv2d->kernel_row),4,1,f);
	ret = fwrite(&(conv2d->kernel_col),4,1,f);
	wtk_debug("%d %d %d %d\n",conv2d->i,conv2d->j,conv2d->kernel_col,conv2d->kernel_row);
	ret = fwrite(&(conv2d->padding_row),4,1,f);
	ret = fwrite(&(conv2d->padding_col),4,1,f);
	ret = fwrite(&(conv2d->stride1),4,1,f);
	ret = fwrite(&(conv2d->stride2),4,1,f);
	ret = fwrite(&(conv2d->dilation1),4,1,f);
	ret = fwrite(&(conv2d->dilation2),4,1,f);
	wtk_debug("%d %d %d %d %d %d\n",conv2d->padding_row,conv2d->padding_col,conv2d->stride1,conv2d->stride2,conv2d->dilation1,conv2d->dilation2);

	len = conv2d->weight->col * conv2d->weight->row;
	wtk_debug("%d\n",len);
	//print_float(conv2d->weight->m,100);
	ret = fwrite((conv2d->weight->m),sizeof(float),len,f);	

	if(conv2d->bias){
		len = 1;
		ret = fwrite(&(len),4,1,f);
		len = conv2d->bias->col*conv2d->bias->row;
		ret = fwrite((conv2d->bias->m),4,len,f);
	}else{
		len = 0;
		ret = fwrite(&(len),4,1,f);
	}

	return ret;
}

qtk_torchnn_conv2d_t* qtk_torchnn_conv2d_read(wtk_source_t *src,wtk_strbuf_t *buf,int bin)
{
	int ret = 0;
	int col, row,i,j,kernel_row,kernel_col;
	qtk_blas_matrix_t *m = 0;
	qtk_torchnn_conv2d_t *conv2d;

	ret = wtk_source_read_string(src, buf);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<Shape>"))
	{
		ret = -1;
		goto end;
	}

	ret = wtk_source_read_int(src, &i, 1, bin);
	ret = wtk_source_read_int(src, &j, 1, bin);
	ret = wtk_source_read_int(src, &kernel_row, 1, bin);
	ret = wtk_source_read_int(src, &kernel_col, 1, bin);


	col =  j * kernel_row * kernel_col;
	row = i;
	wtk_debug("xxxxxxxxxx %d\n",row*col);
	m = qtk_blas_matrix_new(row,col);

	conv2d = qtk_torchnn_conv2d_new(kernel_row,kernel_col);
	conv2d->i = i;
	conv2d->j = j;
	ret = wtk_source_read_int(src, &(conv2d->padding_row), 1, bin);
	ret = wtk_source_read_int(src, &(conv2d->padding_col), 1, bin);

	ret = wtk_source_read_int(src, &(conv2d->stride1), 1, bin);
	ret = wtk_source_read_int(src, &(conv2d->stride2), 1, bin);

	ret = wtk_source_read_int(src, &(conv2d->dilation1), 1, bin);
	ret = wtk_source_read_int(src, &(conv2d->dilation2), 1, bin);

	ret = wtk_source_read_string(src, buf);//</Shape>
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	ret = wtk_source_read_float(src, m->m, row * col, bin);
	conv2d->weight=m;
	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	ret = wtk_source_read_string(src, buf);
	conv2d->bias = NULL;
	if (wtk_str_equal_s(buf->data, buf->pos, "<Bias>"))
	{
		m = qtk_blas_matrix_new(1,i);
		ret = wtk_source_read_float(src, m->m, i, bin);
		conv2d->bias=m;
		ret = wtk_source_read_string(src, buf);
		ret = wtk_source_read_string(src, buf);
	}

	end:
		if(ret==-1)
		{
			conv2d=NULL;
		}
		return conv2d;
}

qtk_torchnn_conv2d_t* qtk_torchnn_conv2d_read_bin(wtk_source_t *src,wtk_strbuf_t *buf,int bin)
{
	int ret = 0;
	int col, row,i,j,kernel_row,kernel_col;
	qtk_blas_matrix_t *m = 0;
	qtk_torchnn_conv2d_t *conv2d;
	ret = wtk_source_read_int_little(src, &i, 1, bin);
	ret = wtk_source_read_int_little(src, &j, 1, bin);
	ret = wtk_source_read_int_little(src, &kernel_row, 1, bin);
	ret = wtk_source_read_int_little(src, &kernel_col, 1, bin);

	//wtk_debug("%d %d %d %d\n",i,j,kernel_col,kernel_row);

	col =  j * kernel_row * kernel_col;
	row = i;
	m = qtk_blas_matrix_new(row,col);

	conv2d = qtk_torchnn_conv2d_new(kernel_row,kernel_col);
	conv2d->i = i;
	conv2d->j = j;
	ret = wtk_source_read_int_little(src, &(conv2d->padding_row), 1, bin);
	ret = wtk_source_read_int_little(src, &(conv2d->padding_col), 1, bin);

	ret = wtk_source_read_int_little(src, &(conv2d->stride1), 1, bin);
	ret = wtk_source_read_int_little(src, &(conv2d->stride2), 1, bin);

	ret = wtk_source_read_int_little(src, &(conv2d->dilation1), 1, bin);
	ret = wtk_source_read_int_little(src, &(conv2d->dilation2), 1, bin);
	//wtk_debug("%d %d %d %d %d %d\n",conv2d->padding_row,conv2d->padding_col,conv2d->stride1,conv2d->stride2,conv2d->dilation1,conv2d->dilation2);
	//ret=wtk_source_fill(src,(char*)(m->m),row*col*4);
	ret = wtk_source_read_float_little(src, m->m, row * col, bin);
	conv2d->weight=m;
	ret = wtk_source_read_int_little(src, &(j), 1, bin);
	if (j == 1)
	{
		m = qtk_blas_matrix_new(1,i);
		ret = wtk_source_read_float(src, m->m, i, bin);
		conv2d->bias=m;
	}

	if(ret==-1)
	{
		conv2d=NULL;
	}
	return conv2d;
}

//need input pad0 before this function
void qtk_torchnn_conv2d_cal(qtk_torchnn_conv2d_t *conv, qtk_blas_matrix_t *input, qtk_blas_matrix_t *output,int batch_row, int batch_col)
{
	//qtk_blas_matrix_t *trans;
	int i,j,m,n;
	float *p1,*p2,*p3,*p4;
	int stride = conv->stride2;

	//col = conv->kernel_col*conv->kernel_row*batch_row;//9 3*3*1
	//row = batch_col/stride;//40 (42-2)*(3-2)
	//trans = qtk_blas_matrix_new(row,col);
	//wtk_debug("%d %d %d\n",stride,row,col);
	p1 = conv->trans->m;
	//if(trans->row != conv->trans->row || trans->col != conv->trans->col)
	//{
	//	wtk_debug("woc\n");
	//	exit(0);
	//}
//wtk_debug("--------------conv info---------------\n");
//	qtk_blas_matrix_print(conv->weight);
//	qtk_blas_matrix_print(conv->bias);

	//wtk_debug("%d %d\n",input->row,input->col);
	//wtk_debug("%d %d %d %d\n",batch_col,batch_row,conv->kernel_row,conv->kernel_col);
	for(m=0;m<batch_col;m+=stride)//40
	{
		for(n=0;n<batch_row;n++)//1
		{
			p2 = input->m + n*input->col+m;//add stride
			////wtk_debug("%d\n",n*input->col+m);
			for(i=0;i<conv->kernel_row;i++)//5
			{
				p3 = p2 + i*input->col*batch_row;
				////wtk_debug("%d\n",i*input->col*batch_row);
				for(j=0;j<conv->kernel_col;j++)//5
				{
					p4 = p3 + j;
					//wtk_debug("%d %d %d\n",n*input->col+m,i*input->col*batch_row,j);
					*p1 = *p4;
					////wtk_debug("%d %f\n",j,*p2);
					//if(j+i*input->col*batch_row+n*input->col+m<100)
					//{
					////wtk_debug("%d %f %p\n",j+i*input->col*batch_row+n*input->col+m,*p4,p4);
					//}
					p1++;
				}
			}
		}
	}
//	//wtk_debug("-------conv2d trans------------\n");
//	qtk_blas_matrix_print(trans);
//	wtk_debug("%d %d\n",conv->weight->row,conv->weight->col);
//	wtk_debug("%d %d\n",trans->row,trans->col);
//	wtk_debug("%d %d\n",output->row,output->col);
	qtk_blas_matrix_mul(conv->weight,output,NULL,NULL,conv->trans,conv->bias);//TODO bias has some issue?
//	qtk_blas_matrix_print(output);
//	exit(0);
}

qtk_torchnn_lstm_t* qtk_torchnn_lstm_new(void)
{
	qtk_torchnn_lstm_t *lstm;
	lstm=(qtk_torchnn_lstm_t*)wtk_malloc(sizeof(qtk_torchnn_lstm_t));

	return lstm;
}

void qtk_torchnn_lstm_delete(qtk_torchnn_lstm_t *lstm)
{
	qtk_blas_matrix_delete(lstm->ih_w);
	qtk_blas_matrix_delete(lstm->hh_w);
	qtk_blas_matrix_delete(lstm->ih_b);
	qtk_blas_matrix_delete(lstm->hh_b);
}

int qtk_torchnn_lstm_write(void *mdl,FILE *f){
	int ret,len;
	qtk_torchnn_lstm_t* lstm = (qtk_torchnn_lstm_t*)mdl;

	ret = fwrite(&(lstm->ih_w->row),4,1,f);
	ret = fwrite(&(lstm->ih_w->col),4,1,f);
	len = lstm->ih_w->col * lstm->ih_w->row;
	ret = fwrite((lstm->ih_w->m),4,len,f);

	ret = fwrite(&(lstm->hh_w->row),4,1,f);
	ret = fwrite(&(lstm->hh_w->col),4,1,f);
	len = lstm->hh_w->col * lstm->hh_w->row;
	ret = fwrite((lstm->hh_w->m),4,len,f);	

	ret = fwrite(&(lstm->ih_b->col),4,1,f);
	len = lstm->ih_b->col * lstm->ih_b->row;
	ret = fwrite((lstm->ih_b->m),4,len,f);	

	ret = fwrite(&(lstm->hh_b->col),4,1,f);
	len = lstm->hh_b->col * lstm->hh_b->row;
	ret = fwrite((lstm->hh_b->m),4,len,f);	

	return ret;
}

qtk_torchnn_lstm_t* qtk_torchnn_lstm_read(wtk_source_t *src,wtk_strbuf_t *buf,int bin)
{
	int ret = 0;
	int col, row;
	qtk_blas_matrix_t *m = 0;
	qtk_torchnn_lstm_t* lstm;

	lstm=qtk_torchnn_lstm_new();

	ret = wtk_source_read_string(src, buf);
	ret = wtk_source_read_int(src, &row, 1, bin);
	ret = wtk_source_read_int(src, &col, 1, bin);
	m=qtk_blas_matrix_new(row,col);
	ret = wtk_source_read_float(src, m->m, row * col, bin);
	lstm->ih_w=m;
	ret = wtk_source_read_string(src, buf);

	ret = wtk_source_read_string(src, buf);
	ret = wtk_source_read_int(src, &row, 1, bin);
	ret = wtk_source_read_int(src, &col, 1, bin);
	m=qtk_blas_matrix_new(row,col);
	ret = wtk_source_read_float(src, m->m, row * col, bin);
	lstm->hh_w=m;
	ret = wtk_source_read_string(src, buf);

	ret = wtk_source_read_string(src, buf);
	ret = wtk_source_read_int(src, &row, 1, bin);
	m=qtk_blas_matrix_new(1,row);
	ret = wtk_source_read_float(src, m->m, row , bin);
	lstm->ih_b=m;
	ret = wtk_source_read_string(src, buf);

	ret = wtk_source_read_string(src, buf);
	ret = wtk_source_read_int(src, &row, 1, bin);
	m=qtk_blas_matrix_new(1,row);
	ret = wtk_source_read_float(src, m->m, row, bin);
	lstm->hh_b=m;
	ret = wtk_source_read_string(src, buf);
	ret = wtk_source_read_string(src, buf);
	if(ret!=0)
	{
		//wtk_debug("error\n");
	}
	return lstm;
}

void qtk_torchnn_lstm_cal(qtk_torchnn_lstm_t *lstm,qtk_blas_matrix_t *input,qtk_blas_matrix_t *c1,qtk_blas_matrix_t *h1,
		qtk_blas_matrix_t *c2,qtk_blas_matrix_t *h2)
{

	//wtk_debug("input: %d %d\n",input->row,input->col);
	//wtk_debug("c1: %d %d\n",c1->row,c1->col);
	//wtk_debug("h1: %d %d\n",h1->row,h1->col);
	//wtk_debug("c2: %d %d\n",c2->row,c2->col);
	//wtk_debug("h2: %d %d\n",h2->row,h2->col);

	int offset,col,i;
        qtk_sub_matrix_t it, ft, gt, ot, sc1, sc2, sc3, sh2;
        qtk_blas_matrix_t *tmp1=NULL,*tmp2=NULL,*tmp3=NULL;//TODO

	tmp1 = qtk_blas_matrix_new(input->row,lstm->ih_w->row);
	tmp2 = qtk_blas_matrix_new(h1->row,lstm->hh_w->row);
	//wtk_debug("tmp1: %d %d\n",tmp1->row,tmp1->col);
	//wtk_debug("tmp2: %d %d\n",tmp2->row,tmp2->col);
	//wtk_debug("ih_w: %d %d\n",lstm->ih_w->row,lstm->ih_w->col);
	//wtk_debug("hh_w: %d %d\n",lstm->hh_w->row,lstm->hh_w->col);
	offset = lstm->hh_w->row/4;
	col = lstm->hh_w->col;
	qtk_blas_matrix_mul(input,tmp1,NULL,NULL,lstm->ih_w,lstm->ih_b);
	qtk_blas_matrix_mul(h1,tmp2,NULL,NULL,lstm->hh_w,lstm->hh_b);

	qtk_blas_matrix_add_mat(tmp1,tmp2);
	//wtk_debug("---------tmp1-----------\n");
//	qtk_blas_matrix_print(tmp1);
//	qtk_sub_matrix_t* qtk_sub_matrix_new(float *f,int row_offset,int num_rows,int col_offset,int num_cols,int stride)

        qtk_sub_matrix_init(&it, tmp1->m, 0, tmp1->row, 0, offset, col);
        //wtk_debug("it: %d %d\n",it->row,it->col);

        for (i = 0; i < it.row; i++) {
                wtk_sigmoid(it.f + it.stride * i, it.col);
        }

        qtk_sub_matrix_init(&ft, tmp1->m, 0, tmp1->row, offset, offset, col);
        //wtk_debug("ft: %d %d\n",ft->row,ft->col);

        for (i = 0; i < ft.row; i++) {
                wtk_sigmoid(ft.f + ft.stride * i, ft.col);
        }

        qtk_sub_matrix_init(&gt, tmp1->m, 0, tmp1->row, offset * 2, offset,
                            col);
        //wtk_debug("gt: %d %d\n",gt->row,gt->col);

        for (i = 0; i < gt.row; i++) {
                wtk_tanh(gt.f + gt.stride * i, gt.col);
        }

        qtk_sub_matrix_init(&ot, tmp1->m, 0, tmp1->row, offset * 3, offset,
                            col);
        //wtk_debug("ot: %d %d\n",ot->row,ot->col);

        for (i = 0; i < ot.row; i++) {
                wtk_sigmoid(ot.f + ot.stride * i, ot.col);
        }

        qtk_sub_matrix_init2(&sc1, c1->m, c1->row, c1->col, c1->col);
        qtk_sub_matrix_init2(&sh2, h2->m, h2->row, h2->col, h2->col);

        qtk_sub_matrix_init2(&sc2, c2->m, c2->row, c2->col, c2->col);
        qtk_sub_matrix_mul_element(&sc2, &ft, &sc1);

        qtk_sub_matrix_mul_element(&sc2, &it, &gt);

        tmp3 = qtk_blas_matrix_new(c2->row,c2->col);
	memcpy(tmp3->m,c2->m,c2->row*c2->col*sizeof(float));
        qtk_sub_matrix_init2(&sc3, tmp3->m, c2->row, c2->col, c2->col);

        for (i = 0; i < sc3.row; i++) {
                wtk_tanh(sc3.f + sc3.stride * i, sc3.col);
        }
        qtk_sub_matrix_mul_element(&sh2, &ot, &sc3);

        qtk_blas_matrix_delete(tmp3);
}

qtk_torchnn_maxpool_t* qtk_torchnn_maxpool_new(void)
{
	qtk_torchnn_maxpool_t *maxpool = (qtk_torchnn_maxpool_t*)wtk_malloc(sizeof(qtk_torchnn_maxpool_t));

	return maxpool;
}

void qtk_torchnn_maxpool_delete(qtk_torchnn_maxpool_t *pool)
{

}

int qtk_torchnn_maxpool_write(void *mdl,FILE *f){
	int ret = 0;
	qtk_torchnn_maxpool_t *maxpool = (qtk_torchnn_maxpool_t*)mdl;
	ret = fwrite(&(maxpool->kernel_size),4,1,f);
	ret = fwrite(&(maxpool->stride),4,1,f);
	return ret;
}

qtk_torchnn_maxpool_t* qtk_torchnn_maxpool_read(wtk_source_t *src,wtk_strbuf_t *buf,int bin)
{
	qtk_torchnn_maxpool_t *maxpool = qtk_torchnn_maxpool_new();

	wtk_source_read_int(src, &(maxpool->kernel_size), 1, bin);
	wtk_source_read_int(src, &(maxpool->stride), 1, bin);
	wtk_source_read_string(src, buf);

	return maxpool;
}

float qtk_torchnn_maxpool_getmax(float floor, float *v, int len)
{
	int i;
	float max = floor;
	for(i = 0; i < len; i++)
	{
		max = max(max,*(v+i));
	}
	return max;
}

void qtk_torchnn_maxpool_cal(qtk_torchnn_maxpool_t *pool,qtk_blas_matrix_t *input,qtk_blas_matrix_t *output,int batch_row)
{
//TODO depends on the result of conv2d
	int i,j,k;
	float floor = FLT_MIN;
	float *p,*p1,*p2;
	//wtk_debug("%d %d %d %d\n",input->row,input->col,output->row,output->col);
	//wtk_debug("%d %d\n",batch_row,input->col);
	for(i = 0;i < output->row; i++)
	{
		p = input->m + i*input->col;
		for(j = 0; j <output->col; j++)
		{
			p1 = p + j*pool->stride;
			floor = FLT_MIN;
			for(k = 0; k<pool->kernel_size; k++)
			{
				p2 = p1 + batch_row*k*input->col;
				//wtk_debug("%d\n",batch_row*k*input->col+i*input->col+j*pool->stride);
				floor = qtk_torchnn_maxpool_getmax(floor,p2,pool->stride);
			}
			*(output->m+i*output->col+j) = floor;
		}
	}
//	qtk_blas_matrix_print(output);
//	exit(0);
}

qtk_torchnn_avpool_t* qtk_torchnn_avpool_new(void)
{
	qtk_torchnn_avpool_t *avpool = (qtk_torchnn_avpool_t*)wtk_malloc(sizeof(qtk_torchnn_avpool_t));

	return avpool;
}

void qtk_torchnn_avpool_delete(qtk_torchnn_avpool_t *pool)
{

}

int qtk_torchnn_avpool_write(void *mdl,FILE *f){
	int ret = 0;
	qtk_torchnn_avpool_t *avpool = (qtk_torchnn_avpool_t*)mdl;
	ret = fwrite(&(avpool->kernel_row),4,1,f);
	ret = fwrite(&(avpool->kernel_col),4,1,f);
	ret = fwrite(&(avpool->stride_row),4,1,f);
	ret = fwrite(&(avpool->stride_col),4,1,f);	
	return ret;
}

qtk_torchnn_avpool_t* qtk_torchnn_avpool_read(wtk_source_t *src,wtk_strbuf_t *buf,int bin)
{
	qtk_torchnn_avpool_t *avpool = qtk_torchnn_avpool_new();

	wtk_source_read_int(src, &(avpool->kernel_row), 1, bin);
	wtk_source_read_int(src, &(avpool->kernel_col), 1, bin);
	wtk_source_read_int(src, &(avpool->stride_row), 1, bin);
	wtk_source_read_int(src, &(avpool->stride_col), 1, bin);
	wtk_source_read_string(src, buf);

	return avpool;
}

void qtk_torchnn_avpool_cal(qtk_torchnn_avpool_t *pool,qtk_blas_matrix_t *input,qtk_blas_matrix_t *output)
{
	float *in;
	float *p = output->m;
	int i;//cnt = input->col/pool->kernel_col;

	for(i = 0; i < input->row; i++)
	{
		in = input->m + input->col*i;
		*p = (in[0]+in[1]+in[2])/3;
		p++;
	}
	//exit(0);
}

void qtk_torchnn_mean(qtk_blas_matrix_t *input,qtk_blas_matrix_t *output)//128 * 5,1 * 128
{
	float *p = output->m;
	float *in = input->m;
	int cnt = input->col;
	int i,j;
	//wtk_debug("%d %d %d %d\n",input->row,input->col,output->row,output->col);

	//qtk_blas_matrix_print(input);
	for(i=0;i<input->row;i++)
	{
		for(j=0;j<input->col;j++)
		{
			*p += *in;
			in++;
		}
		*p /= cnt;
		p++;
	}
	//qtk_blas_matrix_print(output);
}

qtk_torchnn_linear_t* qtk_torchnn_linear_new(void)
{
	qtk_torchnn_linear_t *linear = (qtk_torchnn_linear_t*)wtk_malloc(sizeof(qtk_torchnn_linear_t));

	linear->weight = NULL;
	linear->bias = NULL;

	return linear;
}

void qtk_torchnn_linear_delete(qtk_torchnn_linear_t *linear)
{
	qtk_blas_matrix_delete(linear->weight);
	if(linear->bias)
	{
		qtk_blas_matrix_delete(linear->bias);
	}
}

int qtk_torchnn_linear_write(void *mdl,FILE *f){
	int ret=0,len;
	qtk_torchnn_linear_t* linear = (qtk_torchnn_linear_t*)mdl;

	ret = fwrite(&(linear->weight->row),4,1,f);
	ret = fwrite(&(linear->weight->col),4,1,f);
	len = linear->weight->col * linear->weight->row;
	ret = fwrite((linear->weight->m),4,len,f);

	if(linear->bias){
		len = 1;
		ret = fwrite(&len,4,1,f);
		ret = fwrite(&(linear->bias->col),4,1,f);
		len = linear->bias->col * linear->bias->row;
		ret = fwrite((linear->bias->m),4,len,f);
	}else{
		len = 0;
		ret = fwrite(&len,4,1,f);
	}

	return ret;
}

qtk_torchnn_linear_t* qtk_torchnn_linear_read_bin(wtk_source_t *src,wtk_strbuf_t *buf,int bin)
{
	int ret = 0;
	int col=0, row=0;
	qtk_blas_matrix_t *m = 0;
	qtk_torchnn_linear_t *linear = qtk_torchnn_linear_new();

	ret = wtk_source_read_int_little(src, &row, 1, bin);
	ret = wtk_source_read_int_little(src, &col, 1, bin);
	//wtk_debug("%d %d\n",row,col);
	m=qtk_blas_matrix_new(row,col);
	//wtk_debug("%d %d\n",row,col);
	ret = wtk_source_read_float_little(src, m->m, row * col, bin);
	linear->weight=m;

	ret = wtk_source_read_int_little(src, &row, 1, bin);
	if (row == 1)
	{
		ret = wtk_source_read_int_little(src, &col, 1, bin);
		m=qtk_blas_matrix_new(1,col);
		ret = wtk_source_read_float_little(src, m->m, 1 * col, bin);
		linear->bias=m;
	}

	if(ret!=0)
	{
		wtk_debug("error\n");
	}

	return linear;
}

qtk_torchnn_linear_t* qtk_torchnn_linear_read(wtk_source_t *src,wtk_strbuf_t *buf,int bin)
{
	int ret = 0;
	int col=0, row=0;
	qtk_blas_matrix_t *m = 0;
	qtk_torchnn_linear_t *linear = qtk_torchnn_linear_new();

	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	ret = wtk_source_read_int(src, &row, 1, bin);
	ret = wtk_source_read_int(src, &col, 1, bin);
	//wtk_debug("%d %d\n",row,col);
	m=qtk_blas_matrix_new(row,col);
	//wtk_debug("%d %d\n",row,col);
	ret = wtk_source_read_float(src, m->m, row * col, bin);
	linear->weight=m;
	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (wtk_str_equal_s(buf->data, buf->pos, "<Bias>"))
	{
		ret = wtk_source_read_int(src, &col, 1, bin);
		//wtk_debug("%d\n",col);
		m=qtk_blas_matrix_new(1,col);
		ret = wtk_source_read_float(src, m->m, 1 * col, bin);
		linear->bias=m;
		wtk_source_read_string(src, buf);
		//wtk_debug("[%.*s]\n",buf->pos,buf->data);
		wtk_source_read_string(src, buf);
		//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	}

	if(ret!=0)
	{
		wtk_debug("error\n");
	}

	return linear;
}

void qtk_torchnn_linear_cal(qtk_torchnn_linear_t *linear,qtk_blas_matrix_t *input,qtk_blas_matrix_t *output)
{
	//wtk_debug("%d %d\n",input->row,input->col);
	//wtk_debug("%d %d\n",output->row,output->col);
	qtk_blas_matrix_mul(input,output,NULL,NULL,linear->weight,linear->bias);
}

qtk_torchnn_embed_t* qtk_torchnn_embed_new(void)
{
	qtk_torchnn_embed_t *embed = (qtk_torchnn_embed_t*)wtk_malloc(sizeof(qtk_torchnn_embed_t));
	embed->weight = NULL;

	return embed;
}

void qtk_torchnn_embed_delete(qtk_torchnn_embed_t *embed)
{
	qtk_blas_matrix_delete(embed->weight);
}

int qtk_torchnn_embed_write(void *mdl,FILE *f){
	int ret = 0,len;
	qtk_torchnn_embed_t* embed = (qtk_torchnn_embed_t*)mdl;	
	ret = fwrite(&(embed->weight->row),4,1,f);
	ret = fwrite(&(embed->weight->col),4,1,f);
	len = embed->weight->col * embed->weight->row;
	ret = fwrite((embed->weight->m),4,len,f);

	return ret;
}

qtk_torchnn_embed_t* qtk_torchnn_embed_read_bin(wtk_source_t *src,wtk_strbuf_t *buf,int bin)
{
	int ret = 0;
	int col, row;
	qtk_blas_matrix_t *m = 0;
	qtk_torchnn_embed_t *embed = qtk_torchnn_embed_new();

	ret = wtk_source_read_int_little(src, &row, 1, bin);
	ret = wtk_source_read_int_little(src, &col, 1, bin);
	m=qtk_blas_matrix_new(row,col);
	ret = wtk_source_read_float_little(src, m->m, row * col, bin);
	embed->weight=m;

	if(ret!=0)
	{
		wtk_debug("error\n");
	}
	return embed;
}

qtk_torchnn_embed_t* qtk_torchnn_embed_read(wtk_source_t *src,wtk_strbuf_t *buf,int bin)
{
	int ret = 0;
	int col, row;
	qtk_blas_matrix_t *m = 0;
	qtk_torchnn_embed_t *embed = qtk_torchnn_embed_new();

	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	ret = wtk_source_read_int(src, &row, 1, bin);
	ret = wtk_source_read_int(src, &col, 1, bin);
	m=qtk_blas_matrix_new(row,col);
	ret = wtk_source_read_float(src, m->m, row * col, bin);
	embed->weight=m;
	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if(ret!=0)
	{
		wtk_debug("error\n");
	}
	return embed;
}

void qtk_torchnn_embed_cal(qtk_torchnn_embed_t *embed,qtk_blas_matrix_t *output,int index)
{
	memcpy(output->m,embed->weight->m+index*embed->weight->col,embed->weight->col*sizeof(float));
}

qtk_torchnn_conv1d_t* qtk_torchnn_conv1d_new(int ker_row)
{
	qtk_torchnn_conv1d_t *conv1d = (qtk_torchnn_conv1d_t*)wtk_malloc(sizeof(qtk_torchnn_conv1d_t));

	conv1d->kernel_row = ker_row;
	conv1d->bias = NULL;
	conv1d->weight = NULL;

	return conv1d;
}

void qtk_torchnn_conv1d_delete(qtk_torchnn_conv1d_t *conv)
{
	qtk_blas_matrix_delete(conv->weight);
	qtk_blas_matrix_delete(conv->weight_t);
	if(conv->bias)
	{
		qtk_blas_matrix_delete(conv->bias);
	}
}

int qtk_torchnn_conv1d_write(void *mdl,FILE *f){
	int ret,len;
	qtk_torchnn_conv1d_t* conv1d = (qtk_torchnn_conv1d_t*)mdl;

	ret = fwrite(&(conv1d->i),4,1,f);
	ret = fwrite(&(conv1d->j),4,1,f);
	ret = fwrite(&(conv1d->kernel_row),4,1,f);

	ret = fwrite(&(conv1d->padding_row),4,1,f);
	ret = fwrite(&(conv1d->stride1),4,1,f);
	ret = fwrite(&(conv1d->dilation1),4,1,f);

	len = conv1d->weight_t->col * conv1d->weight_t->row;
	ret = fwrite((conv1d->weight_t->m),4,len,f);	

	if(conv1d->bias){
		len = 1;
		ret = fwrite(&len,4,1,f);
		ret = fwrite(&(conv1d->bias->col),4,1,f);
		len = conv1d->bias->col * conv1d->bias->row;
		ret = fwrite((conv1d->bias->m),4,len,f);
	}else{
		len = 0;
		ret = fwrite(&len,4,1,f);		
	}
	return ret;
}

qtk_torchnn_conv1d_t* qtk_torchnn_conv1d_read(wtk_source_t *src,wtk_strbuf_t *buf,int bin)
{
	int ret = 0;
	int col, row,i,j,kernel_row;
	qtk_blas_matrix_t *m = 0;
	qtk_torchnn_conv1d_t *conv1d;

	ret = wtk_source_read_string(src, buf);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<Shape>"))
	{
		ret = -1;
		goto end;
	}

	ret = wtk_source_read_int(src, &i, 1, bin);
	ret = wtk_source_read_int(src, &j, 1, bin);
	ret = wtk_source_read_int(src, &kernel_row, 1, bin);
	//ret = wtk_source_read_int(src, &kernel_col, 1, bin);


	col =  j * kernel_row;
	row = i;
	m = qtk_blas_matrix_new(row,col);

	conv1d = qtk_torchnn_conv1d_new(kernel_row);
	conv1d->i = i;
	conv1d->j = j;
	conv1d->weight = qtk_blas_matrix_new(col,row);

	ret = wtk_source_read_int(src, &(conv1d->padding_row), 1, bin);

	ret = wtk_source_read_int(src, &(conv1d->stride1), 1, bin);

	ret = wtk_source_read_int(src, &(conv1d->dilation1), 1, bin);

	ret = wtk_source_read_string(src, buf);//</Shape>
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	ret = wtk_source_read_float(src, m->m, row * col, bin);
	conv1d->weight_t=m;
	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	ret = wtk_source_read_string(src, buf);
	if (wtk_str_equal_s(buf->data, buf->pos, "<Bias>"))
	{
		m = qtk_blas_matrix_new(1,i);
		ret = wtk_source_read_float(src, m->m, i, bin);
		conv1d->bias=m;
		ret = wtk_source_read_string(src, buf);
		ret = wtk_source_read_string(src, buf);
	}

	qtk_blas_matrix_trans(conv1d->weight_t,conv1d->weight);

	end:
		if(ret==-1)
		{
			conv1d=NULL;
		}
		return conv1d;
}

//need input pad0 before this function
void qtk_torchnn_conv1d_cal(qtk_torchnn_conv1d_t *conv, qtk_blas_matrix_t *input, qtk_blas_matrix_t *output)
{
	//wtk_debug("heihei %d %d %d %d\n",input->row,input->col,output->row,output->col);
	int i,j,col = input->col;
	float *scale = conv->weight->m;
	float *p = input->m;
	float *p2 = output->m;

	for(i = 0; i < input->row; i++)
	{
		for(j = 0; j < col; j++)
		{
			p2[j] += (*p)*(*scale);
			p++;
			scale++;
		}
	}
}

qtk_torchnn_batch_t* qtk_torchnn_batch_new(void)
{
	qtk_torchnn_batch_t *batch;

	batch = (qtk_torchnn_batch_t*)wtk_malloc(sizeof(qtk_torchnn_batch_t));
	batch->offset = NULL;
	batch->scale = NULL;
	return batch;
}

void qtk_torchnn_batch_delete(qtk_torchnn_batch_t *batch)
{
	wtk_free(batch->weight);
	wtk_free(batch->bias);
	wtk_free(batch->mean);
	wtk_free(batch->var);

	qtk_blas_matrix_delete(batch->scale);
	if(batch->offset)
	{
		qtk_blas_matrix_delete(batch->offset);
	}
}

int qtk_torchnn_batch_write(void *mdl,FILE *f){
	int ret,len;
	qtk_torchnn_batch_t* batch = (qtk_torchnn_batch_t*)mdl;
	ret = fwrite(&(batch->scale->col),4,1,f);
	len = batch->scale->col;
	ret = fwrite((batch->weight),4,len,f);	
	ret = fwrite((batch->bias),4,len,f);	
	ret = fwrite((batch->mean),4,len,f);	
	ret = fwrite((batch->var),4,len,f);	
	return ret;
}

qtk_torchnn_batch_t* qtk_torchnn_batch_read_bin(wtk_source_t *src,wtk_strbuf_t *buf,int bin)
{
	int ret = 0;
	int row,i;
	qtk_blas_matrix_t *m = 0;
	qtk_torchnn_batch_t *batch;
	float *weight,*bias,*mean,*var;

	ret = wtk_source_read_int_little(src, &i, 1, bin);

	row = i;
	batch = qtk_torchnn_batch_new();
	//wtk_debug("%d\n",row);
	weight = (float*)wtk_malloc(sizeof(float)*row);
	bias = (float*)wtk_malloc(sizeof(float)*row);
	mean = (float*)wtk_malloc(sizeof(float)*row);
	var = (float*)wtk_malloc(sizeof(float)*row);

	ret = wtk_source_read_float_little(src, weight, row, bin);
	ret = wtk_source_read_float_little(src, bias, row, bin);
	ret = wtk_source_read_float_little(src, mean, row, bin);
	ret = wtk_source_read_float_little(src, var, row, bin);


	batch->weight = weight;
	batch->bias = bias;
	batch->mean = mean;
	batch->var = var;

	m = qtk_blas_matrix_new(1,row);
	batch->scale=m;
	m = qtk_blas_matrix_new(1,row);
	batch->offset=m;
	float eps = 0.00001;
	for(i = 0;i < row;i++)
	{
		batch->scale->m[i] = weight[i]/(sqrt(var[i]+eps));
		if(var[i] < eps)
		{
			batch->scale->m[i] = 0.0;
		}
		batch->offset->m[i] = -(mean[i]*weight[i]/(sqrt(var[i]+eps))) + bias[i];
		//wtk_debug("%f %f %f %f\n",weight[i],sqrt(var[i])+eps,batch->scale->m[i],batch->offset->m[i]);
	}

	if(ret==-1)
	{
		batch = NULL;
	}
	return batch;
}

qtk_torchnn_batch_t* qtk_torchnn_batch_read(wtk_source_t *src,wtk_strbuf_t *buf,int bin)
{
	int ret = 0;
	int row,i;
	qtk_blas_matrix_t *m = 0;
	qtk_torchnn_batch_t *batch;
	float *weight,*bias,*mean,*var;

	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<Weight>"))
	{
		ret = -1;
		goto end;
	}

	ret = wtk_source_read_int(src, &i, 1, bin);

	row = i;
	batch = qtk_torchnn_batch_new();
	//wtk_debug("%d\n",row);
	weight = (float*)wtk_malloc(sizeof(float)*row);
	bias = (float*)wtk_malloc(sizeof(float)*row);
	mean = (float*)wtk_malloc(sizeof(float)*row);
	var = (float*)wtk_malloc(sizeof(float)*row);

	ret = wtk_source_read_float(src, weight, row, bin);

	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	ret = wtk_source_read_string(src, buf);
	ret = wtk_source_read_float(src, bias, row, bin);
	ret = wtk_source_read_string(src, buf);

	ret = wtk_source_read_string(src, buf);
	ret = wtk_source_read_float(src, mean, row, bin);
	ret = wtk_source_read_string(src, buf);

	ret = wtk_source_read_string(src, buf);
	if (wtk_str_equal_s(buf->data, buf->pos, "<Vars>"))
	{
		ret = wtk_source_read_float(src, var, row, bin);

		ret = wtk_source_read_string(src, buf);
		ret = wtk_source_read_string(src, buf);
	}

	batch->weight = weight;
	batch->bias = bias;
	batch->mean = mean;
	batch->var = var;

	m = qtk_blas_matrix_new(1,row);
	batch->scale=m;
	m = qtk_blas_matrix_new(1,row);
	batch->offset=m;
	float eps = 0.00001;
	for(i = 0;i < row;i++)
	{
		batch->scale->m[i] = weight[i]/(sqrt(var[i]+eps));
		if(var[i] < eps)
		{
			batch->scale->m[i] = 0.0;
		}
		batch->offset->m[i] = -(mean[i]*weight[i]/(sqrt(var[i]+eps))) + bias[i];
		//wtk_debug("%f %f %f %f\n",weight[i],sqrt(var[i])+eps,batch->scale->m[i],batch->offset->m[i]);
	}


	end:
		if(ret==-1)
		{
			batch = NULL;
		}
		return batch;
}

void qtk_torchnn_batch_cal(qtk_torchnn_batch_t *batch, qtk_blas_matrix_t *in)
{
	int row = in->row;
	int col = in->col;
	int i,j;

	float *scale = batch->scale->m;
	float *offset = batch->offset->m;

	if(row > 1)//BATCHNORM 2d
	{
		for(i = 0;i < row; i++)
		{
			for(j = 0;j < col; j++)
			{
				in->m[j+i*col] = scale[i]*in->m[j+i*col] + offset[i];
			}
		}
	}else //BATCHNORM 1d
	{
		//qtk_blas_matrix_print(in);
		for(j = 0;j < col; j++)
		{
			in->m[j] = scale[j]*in->m[j] + offset[j];
		}
		//wtk_debug("batch1d\n");
		//qtk_blas_matrix_print(in);
	}
}
