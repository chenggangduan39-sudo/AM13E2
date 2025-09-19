/*
 * qtk_vits_syn.c
 *
 *  Created on: Aug 26, 2022
 *      Author: dm
 */
#include "qtk_vits.h"

//valid example for the first method
//// [1, 102, 142]
//static float t_zp[]={
//#include "../../data/test/tmp.z_p.ctxt"
//};
//// [1, 1, 142]
//float t_ymask[]={
//#include "../../data/test/tmp.y_mask.ctxt"
//};

//valid example for the second method
//static float t_fea[]={
////#include "../../data/test/tmp.fea.ctxt"   //[1, 192, 138]
//#include "../../data/test/tmp.fea2.ctxt"    //[1, 192, 172]
//};

void qtk_vits_wsola_notity(void*, short*data,int len);
qtk_vits_t* qtk_vits_new(qtk_vits_cfg_t* cfg)
{
	qtk_vits_t* vits;
	int ret;

	vits = (qtk_vits_t*)wtk_calloc(1, sizeof(*vits));
	vits->cfg = cfg;
	vits->onnx = qtk_vits_onnx_new(&(cfg->onnx));
	if (cfg->dec_onnx.onnx_fn)
		vits->dec_onnx = qtk_vits_onnx_new(&(cfg->dec_onnx));
	if (vits->onnx==NULL)
	{
		ret=-1;
		goto end;
	}
	vits->audio_data = NULL;
	vits->audio_data_len = 0;
	vits->wsola = wtk_wsola_new(&(cfg->wsola));
	wtk_wsola_set_notify(vits->wsola, vits, qtk_vits_wsola_notity);
	vits->maxf = 32767.0f;
	ret=0;
end:
	if (ret!=0)
	{
		qtk_vits_delete(vits);
		vits=NULL;
	}
	return vits;
}

void qtk_vits_wsola_notity(void*thd, short*data,int len)
{
	qtk_vits_t *vits;
	int is_end;

	vits = (qtk_vits_t*)thd;
	if (len==0)
		is_end = 1;
	else
		is_end = 0;
    if (vits->notify)
    	vits->notify(vits->user_data,data,len,is_end);
}

int qtk_vits_convertwav(qtk_vits_t* vits, float* data, int len, int is_end)
{
    //float >> short data
    short *data_s=NULL;
    float maxf;
    int i = 0;

    if (len > 0)
    {
    	data_s = wtk_malloc(sizeof(short)*len);
    	maxf = 32767.0 / max(0.01, wtk_float_abs_max(data, len)) * 0.6;
    	if (maxf > vits->maxf)
    		maxf = vits->maxf;
    	else
    		vits->maxf = maxf;
        for(i = 0; i < len; ++i){
            data_s[i] = data[i]*maxf;
        }
    }
//    printf("maxf =%f\n", vits->maxf);
//    print_short(data_s, 3);
//    print_short(data_s+len-3, 3);
    if (vits->notify)
    	vits->notify(vits->user_data,data_s,len,is_end);
    if (data_s)
    	free(data_s);

    return 0;
}

float* qtk_vits_depadding(qtk_vits_t* vits, int num, float* data, int len, int idx, int *out_len)
{
	int frame_offset, l;
	float *ndata;

	ndata = wtk_malloc(len * sizeof(float));
	frame_offset = min(idx * vits->cfg->chunk_len, vits->cfg->chunk_pad);
//	printf("len %d chunk_len * upsample %d\n", len, vits->cfg->chunk_len * vits->cfg->upsample);
//	printf("frame_offset * upsample %d\n", frame_offset*vits->cfg->upsample);
//	printf("idx= %d num=%d\n", idx, num);
	l=len;
	if (vits->cfg->chunk_len * vits->cfg->upsample < l)
		l = vits->cfg->chunk_len * vits->cfg->upsample;

	if (idx == 0){
		memcpy(ndata, data, l * sizeof(float));
	}if (idx == num-1){
		l = len -(frame_offset * vits->cfg->upsample);
		if (l<=0)
			l = 0;
		else
			memcpy(ndata, data + frame_offset * vits->cfg->upsample, l *sizeof(float));
	}else{
		l = vits->cfg->chunk_len * vits->cfg->upsample;
		memcpy(ndata, data + frame_offset * vits->cfg->upsample, l*sizeof(float));
	}
	*out_len = l;
//	print_float(ndata, 3);
//	print_float(ndata+l-3, 3);
	return ndata;
}

/**
 * for the second method
 */
float* qtk_vits_onnx_dec(qtk_vits_t* vits)
{
	float* v, *fea;
	int64_t *shape_out=NULL, *shape_fea, *shape_t, size;
	float *fea_t;
	int m,n,base_fea, base_fea_t, step;
	int ret, idx, chunk_idx, len, chunk_pad, num;

    //fea
	shape_fea = qtk_vits_onnx_get_outshape(vits->onnx,0,&size);
	fea = qtk_vits_onnx_getout(vits->onnx,0);
	//debug
//	shape_fea[0] = 1;
//	shape_fea[1] = 192;
//	shape_fea[2] = 172; //138;
//	fea = t_fea;

    //split process
    fea_t = (float*)wtk_calloc(shape_fea[0]*shape_fea[1]*(vits->cfg->chunk_len+vits->cfg->chunk_pad + vits->cfg->chunk_pad), sizeof(float));
    shape_t = (int64_t*) wtk_calloc(3, sizeof(int64_t));
    for(step=0, chunk_idx=0; step < shape_fea[size-1]; step += vits->cfg->chunk_len, chunk_idx++)
    {
    	for(m=0; m < shape_fea[0]; m++)
    	{
    		for(n=0; n < shape_fea[1]; n++)
    		{
    			base_fea = m * shape_fea[1] * shape_fea[2] + n*shape_fea[2] + step;
    			if (step == 0)
    			{
    				if (step + vits->cfg->chunk_len >= shape_fea[size-1])
    					base_fea_t = m * shape_fea[1] * shape_fea[size-1] + n * shape_fea[size-1];
    				else
    				{
    					chunk_pad = min(vits->cfg->chunk_pad, shape_fea[size-1]-vits->cfg->chunk_len-step);
        				base_fea_t = m * shape_fea[1] * (vits->cfg->chunk_len + chunk_pad)
    					+ n*(vits->cfg->chunk_len + chunk_pad);         // only right margin
    				}
    			}else if(step + vits->cfg->chunk_len >= shape_fea[size-1])
    				base_fea_t = m * shape_fea[1] * (shape_fea[size-1]-step + vits->cfg->chunk_pad)
					+ n*(shape_fea[size-1]-step + vits->cfg->chunk_pad);          //only left margin
    			else
    			{
    				chunk_pad = min(vits->cfg->chunk_pad, shape_fea[size-1]-vits->cfg->chunk_len-step);
    				base_fea_t = m * shape_fea[1] * (vits->cfg->chunk_len + vits->cfg->chunk_pad + chunk_pad)
					+ n*(vits->cfg->chunk_len + vits->cfg->chunk_pad + chunk_pad);   //two margin
    			}

            	if (step > 0)
            	{
            		memcpy(fea_t + base_fea_t, fea + base_fea - vits->cfg->chunk_pad, vits->cfg->chunk_pad * sizeof(float));
            		base_fea_t += vits->cfg->chunk_pad;
            	}

            	if (step + vits->cfg->chunk_len >= shape_fea[size-1])
            	{
            		memcpy(fea_t + base_fea_t, fea + base_fea, (shape_fea[size-1] - step) * sizeof(float));
            		base_fea_t += shape_fea[size-1] - step;
            	}else
            	{
            		memcpy(fea_t + base_fea_t, fea + base_fea, vits->cfg->chunk_len * sizeof(float));
            		base_fea_t += vits->cfg->chunk_len;
            		chunk_pad = min(vits->cfg->chunk_pad, shape_fea[size-1]-vits->cfg->chunk_len-step);
            		memcpy(fea_t + base_fea_t, fea + base_fea + vits->cfg->chunk_len, chunk_pad * sizeof(float));
            	}
    		}
    	}
    	shape_t[0] = shape_fea[0];
    	shape_t[1] = shape_fea[1];
    	if (step==0)
    	{
        	if(step + vits->cfg->chunk_len >= shape_fea[size-1])
        	{
        		shape_t[2] = shape_fea[size-1];
        	}else
        		shape_t[2] = vits->cfg->chunk_len + min(vits->cfg->chunk_pad, shape_fea[size-1]-vits->cfg->chunk_len-step);
    	}else if (step + vits->cfg->chunk_len >= shape_fea[size-1])
    	{
    		shape_t[2] = (shape_fea[size-1] - step) + vits->cfg->chunk_pad;
    	}else
    		shape_t[2] = vits->cfg->chunk_len + vits->cfg->chunk_pad + min(vits->cfg->chunk_pad, shape_fea[size-1]-vits->cfg->chunk_len-step);
    	ret=qtk_vits_onnx_feed_inparam(vits->dec_onnx, fea_t, shape_t[0]*shape_t[1]*shape_t[2], shape_t, 3, 0, 0);
    	if (ret!=0)
    	{
    		wtk_debug("Error\n");
    		exit(0);
    	}

        //infer
        ret=qtk_vits_onnx_run(vits->dec_onnx);
        if (ret!=0){
        	qtk_vits_onnx_reset(vits->dec_onnx);
        	printf("Err: run-onnx\n");
        	goto end;
        }
        //output
        idx=0;
        shape_out = qtk_vits_onnx_get_outshape(vits->dec_onnx,idx,&size);
        v = qtk_vits_onnx_getout(vits->dec_onnx,idx);
        len = shape_out[2];
        num = (int)(1.0*shape_fea[size-1]/vits->cfg->chunk_len+ 1);
        if (num >= 1.0 * shape_fea[size-1]/vits->cfg->chunk_len + 1)
        	num = num -1;
    	v = qtk_vits_depadding(vits, num, v, shape_out[2], chunk_idx, &len);
        //direct connect
//    	printf("chunk_idx= %d len=%d\n", chunk_idx, len);
    	if (chunk_idx==num-1)
        	qtk_vits_convertwav(vits, v, len, 1);
    	else
        	qtk_vits_convertwav(vits, v, len, 0);
        free(shape_out);
        qtk_vits_onnx_reset(vits->dec_onnx);
        wtk_free(v);
    }
end:
	wtk_free(shape_t);
	wtk_free(fea_t);
	wtk_free(shape_fea);
    return 0;
}

/**
 * for the first method
 */
float* qtk_vits_onnx_decflow(qtk_vits_t* vits)
{
	float* v, *z_p, *y_mask;
	int64_t *shape_out=NULL, *shape_z_p, *shape_y_mask, *shape_t, size;
	float *z_p_t, *y_mask_t, maxf;
	int m,n,base_z_p, base_y_mask, base_z_p_t, base_y_mask_t, step;
	int ret, idx, i;

    //z_p
	shape_z_p = qtk_vits_onnx_get_outshape(vits->onnx,0,&size);
//	shape_z_p[0]=1;
//	shape_z_p[1]=192;
//	shape_z_p[2]=142;
	z_p = qtk_vits_onnx_getout(vits->onnx,0);
//	z_p = t_zp;

   	//y_mask
	shape_y_mask = qtk_vits_onnx_get_outshape(vits->onnx,1,&size);
    y_mask = qtk_vits_onnx_getout(vits->onnx,1);
//	shape_y_mask[0]=1;
//	shape_y_mask[1]=1;
//	shape_y_mask[2]=142;
//	y_mask=t_ymask;

    //split process
    z_p_t = (float*)wtk_calloc(shape_z_p[0]*shape_z_p[1]*(vits->cfg->chunk_len+vits->cfg->chunk_pad + vits->cfg->chunk_pad), sizeof(float));
    y_mask_t = (float*)wtk_calloc(shape_y_mask[0]*shape_y_mask[1]*(vits->cfg->chunk_len+vits->cfg->chunk_pad + vits->cfg->chunk_pad), sizeof(float));
    shape_t = (int64_t*) wtk_calloc(3, sizeof(int64_t));
    for(step=0; step < shape_z_p[size-1]; step += vits->cfg->chunk_len - vits->cfg->step)
    {
    	for(m=0; m < shape_z_p[0]; m++)
    	{
    		for(n=0; n < shape_z_p[1]; n++)
    		{
    			base_z_p = m * shape_z_p[1] * shape_z_p[2] + n*shape_z_p[2] + step;
    			if (step + (vits->cfg->chunk_len - vits->cfg->step) >= shape_z_p[size-1])
    				base_z_p_t = m * shape_z_p[1] * (shape_z_p[size-1]-step + vits->cfg->chunk_pad + vits->cfg->chunk_pad)
					+ n*(shape_z_p[size-1]-step + vits->cfg->chunk_pad + vits->cfg->chunk_pad);
    			else
    				base_z_p_t = m * shape_z_p[1] * (vits->cfg->chunk_len + vits->cfg->chunk_pad + vits->cfg->chunk_pad)
					+ n*(vits->cfg->chunk_len + vits->cfg->chunk_pad + vits->cfg->chunk_pad);
            	if (step==0)
            		memset(z_p_t, 0, vits->cfg->chunk_pad * sizeof(float));
            	else
            		memcpy(z_p_t + base_z_p_t, z_p + base_z_p - vits->cfg->chunk_pad, vits->cfg->chunk_pad * sizeof(float));

            	if (step + (vits->cfg->chunk_len - vits->cfg->step) >= shape_z_p[size-1])
            	{
            		memcpy(z_p_t + base_z_p_t + vits->cfg->chunk_pad, z_p + base_z_p, (shape_z_p[size-1] - step) * sizeof(float));
            		memset(z_p_t + base_z_p_t + vits->cfg->chunk_pad + (shape_z_p[size-1] - step), 0, vits->cfg->chunk_pad * sizeof(float));
            	}else
            	{
            		memcpy(z_p_t + base_z_p_t + vits->cfg->chunk_pad, z_p + base_z_p, vits->cfg->chunk_len * sizeof(float));
            		memcpy(z_p_t + base_z_p_t + vits->cfg->chunk_pad + vits->cfg->chunk_len, z_p + base_z_p + vits->cfg->chunk_len, vits->cfg->chunk_pad * sizeof(float));
            	}
    		}
    	}

    	for(m=0; m < shape_y_mask[0]; m++)
    	{
    		for(n=0; n < shape_y_mask[1]; n++)
    		{
    			base_y_mask = m * shape_y_mask[1] * shape_y_mask[2] + n*shape_y_mask[2] + step;
    			if (step + (vits->cfg->chunk_len - vits->cfg->step) >= shape_z_p[size-1])
    				base_y_mask_t = m * shape_y_mask[1] * (shape_z_p[size-1] - step + vits->cfg->chunk_pad + vits->cfg->chunk_pad)
					+ n*(shape_z_p[size-1] - step + vits->cfg->chunk_pad + vits->cfg->chunk_pad);
    			else
    				base_y_mask_t = m * shape_y_mask[1] * (vits->cfg->chunk_len + vits->cfg->chunk_pad + vits->cfg->chunk_pad)
					+ n*(vits->cfg->chunk_len + vits->cfg->chunk_pad + vits->cfg->chunk_pad);
            	if (step==0)
            		memset(y_mask_t, 0, vits->cfg->chunk_pad * sizeof(float));
            	else
            		memcpy(y_mask_t + base_y_mask_t, y_mask + base_y_mask - vits->cfg->chunk_pad, vits->cfg->chunk_pad * sizeof(float));

            	if (step + (vits->cfg->chunk_len - vits->cfg->step) >= shape_z_p[size-1])
            	{
            		memcpy(y_mask_t + base_y_mask_t + vits->cfg->chunk_pad, y_mask + base_y_mask, (shape_y_mask[size-1] - step) * sizeof(float));
            		memset(y_mask_t + base_y_mask_t + vits->cfg->chunk_pad + (shape_y_mask[size-1] - step), 0, vits->cfg->chunk_pad * sizeof(float));
            	}else
            	{
            		memcpy(y_mask_t + base_y_mask_t + vits->cfg->chunk_pad, y_mask + base_y_mask, vits->cfg->chunk_len * sizeof(float));
            		memcpy(y_mask_t + base_y_mask_t + vits->cfg->chunk_pad + vits->cfg->chunk_len, y_mask + base_y_mask + vits->cfg->chunk_len, vits->cfg->chunk_pad * sizeof(float));
            	}
    		}
    	}
    	//Note: last dim same in z_p and y_mask
    	shape_t[0] = shape_z_p[0];
    	shape_t[1] = shape_z_p[1];
    	if (step + (vits->cfg->chunk_len - vits->cfg->step) >= shape_z_p[size-1])
    	{
    		shape_t[2] = (shape_z_p[size-1] - step) + vits->cfg->chunk_pad + vits->cfg->chunk_pad;
    	}
    	else
    		shape_t[2] = vits->cfg->chunk_len + vits->cfg->chunk_pad + vits->cfg->chunk_pad;
//            	wtk_debug("in:%ld out:%ld shape_z_p[2]:%ld\n", vits->dec_onnx->num_in, vits->dec_onnx->num_out, shape_z_p[2]);
//            	wtk_debug("shape_t %ld,%ld,%ld\n", shape_t[0], shape_t[1], shape_t[2]);
//            	wtk_debug("len=%ld\n", shape_t[0]*shape_t[1]*shape_t[2]);
//            	print_float(z_p_t, 10);
    	ret=qtk_vits_onnx_feed_inparam(vits->dec_onnx, z_p_t, shape_t[0]*shape_t[1]*shape_t[2], shape_t, 3, 0, 0);
//            	wtk_debug("shape_t[2]=%ld\n", shape_t[2]);
    	if (ret!=0)
    	{
    		wtk_debug("Error\n");
    		exit(0);
    	}

    	shape_t[0] = shape_y_mask[0];
    	shape_t[1] = shape_y_mask[1];
//            	wtk_debug("shape_t %ld,%ld,%ld\n", shape_t[0], shape_t[1], shape_t[2]);
//            	wtk_debug("len=%ld\n", shape_t[0]*shape_t[1]*shape_t[2]);
//            	print_float(y_mask_t, 10);
        ret=qtk_vits_onnx_feed_inparam(vits->dec_onnx, y_mask_t, shape_t[0]*shape_t[1]*shape_t[2], shape_t, 3, 0, 1);
    	if (ret!=0)
    	{
    		wtk_debug("Error\n");
    		exit(0);
    	}
        //infer
        ret=qtk_vits_onnx_run(vits->dec_onnx);
        if (ret!=0){
        	qtk_vits_onnx_reset(vits->dec_onnx);
        	printf("Err: run-onnx\n");
        	goto end;
        }
        //output
        idx=0;
        shape_out = qtk_vits_onnx_get_outshape(vits->dec_onnx,idx,&size);
        v = qtk_vits_onnx_getout(vits->dec_onnx,idx);
//        wtk_debug("=========================step=%d shape_out: %ld %ld %ld\n", step, shape_out[0], shape_out[1], shape_out[size-1]);
//        print_float(v, 10);
//        exit(0);
    	//concatenate
    	vits->audio_data_len = shape_out[size-1];
    	vits->audio_data = wtk_malloc(sizeof(short) * vits->audio_data_len);
        //smooth connect
        maxf = 32766.0 / max(0.01, wtk_float_abs_max(v, vits->audio_data_len));
        for(i = 0; i < vits->audio_data_len; ++i){
        	vits->audio_data[i] = v[i]*maxf;
        }
        if (step + vits->cfg->chunk_len - vits->cfg->step >= shape_z_p[size-1])
            wtk_wsola_feed_flow(vits->wsola, vits->audio_data, vits->audio_data_len, 1);
        else
        	wtk_wsola_feed_flow(vits->wsola, vits->audio_data, vits->audio_data_len, 0);

        //direct connect
//        qtk_vits_convertwav(vits, v, shape_out[size-1], 0);
        free(shape_out);
        wtk_free(vits->audio_data);
        qtk_vits_onnx_reset(vits->dec_onnx);
    }
end:
	wtk_free(shape_t);
	wtk_free(z_p_t);
	wtk_free(y_mask_t);
	wtk_free(shape_z_p);
	wtk_free(shape_y_mask);
    return 0;
}

int qtk_vits_onnx_prefeed(qtk_vits_t* vits, wtk_veci_t **id_vec, int nid)
{
	wtk_veci_t *vec = NULL;
	int64_t *shape_in1=NULL;
	int64_t *shape_in2=NULL;
	int64_t *input1=NULL;
	int64_t *input2=NULL;
	int64_t *shape_out=NULL;
	int64_t size;
	float* v, *z_p, *y_mask;
	int ret=0, i, j, idx;

	//vits_syn
	shape_in1 = (int64_t*) wtk_malloc(sizeof(int64_t) * 2);
	shape_in2 = (int64_t*) wtk_malloc(sizeof(int64_t) * 1);
	for (i=0; i < 2; i++)
		shape_in1[i]=1;
	for (i=0; i < 1; i++)
		shape_in2[i]=1;

    for(i = 0; i < nid; i++){
        vec = id_vec[i];
        input1 = (int64_t*) wtk_malloc(sizeof(int64_t) * vec->len);
        for(j = 0; j < vec->len;++j){
        	input1[j] = vec->p[j];
        }
        //input info response to model
        //tst
        idx=0;
        shape_in1[2-1]=vec->len;
        qtk_vits_onnx_feed_inparam(vits->onnx, input1, vec->len, shape_in1, 2, 2, idx);
        //tst_length
        idx=1;
        input2 = (int64_t*) wtk_malloc(sizeof(int64_t) * 1);
        input2[1-1] = vec->len;
        qtk_vits_onnx_feed_inparam(vits->onnx, input2, 1, shape_in2, 1, 2, idx);

        //infer
        ret=qtk_vits_onnx_run(vits->onnx);
        if (ret!=0){
        	qtk_vits_onnx_reset(vits->onnx);
        	printf("Err: run-onnx\n");
        	goto end;
        }
        //output
        //z_p
//    	shape_z_p = qtk_vits_onnx_get_outshape(vits->onnx,0,&size);
        if (vits->cfg->use_stream)// && shape_z_p[size-1]/vits->cfg->chunk_len > vits->cfg->nt_thd)
        {
//        	qtk_vits_onnx_decflow(vits);
        	qtk_vits_onnx_dec(vits);
        }
        else{
        	//one method recreate value
        	//z_p
        	shape_out = qtk_vits_onnx_get_outshape(vits->onnx,0,&size);
        	z_p = qtk_vits_onnx_getout(vits->onnx,0);
        	qtk_vits_onnx_feed_inparam(vits->dec_onnx, z_p, shape_out[0]*shape_out[1]*shape_out[2],shape_out, size, 0, 0);
        	free(shape_out);
           	//y_mask
        	shape_out = qtk_vits_onnx_get_outshape(vits->onnx,1,&size);
            y_mask = qtk_vits_onnx_getout(vits->onnx,1);
            qtk_vits_onnx_feed_inparam(vits->dec_onnx, y_mask, shape_out[0]*shape_out[1]*shape_out[2], shape_out, size, 0, 1);

            free(shape_out);
	        //one method: direct give value, same above, but must see free mem.
//	        for(j=0; j < vits->dec_onnx->num_in; j++)
//	        {
//	        	vits->dec_onnx->in[j] = vits->onnx->out[j];
//	        }

	        //infer
	        ret=qtk_vits_onnx_run(vits->dec_onnx);
	        if (ret!=0){
	        	qtk_vits_onnx_reset(vits->dec_onnx);
	        	printf("Err: run-onnx\n");
	        	goto end;
	        }
	        //output
	        idx=0;
	        shape_out = qtk_vits_onnx_get_outshape(vits->dec_onnx,idx,&size);
	        v = qtk_vits_onnx_getout(vits->dec_onnx,idx);

	        if (i == nid-1)
	        	qtk_vits_convertwav(vits, v, shape_out[size-1], 1);
	        else
	        	qtk_vits_convertwav(vits, v, shape_out[size-1], 0);
	        free(shape_out);
	        qtk_vits_onnx_reset(vits->dec_onnx);
        }

        qtk_vits_onnx_reset(vits->onnx);
    	wtk_free(input1);
    	input1=NULL;
    	wtk_free(input2);
    	input2=NULL;
    }
end:
	if (input1)
		wtk_free(input1);
	if (input2)
		wtk_free(input2);
	if (shape_in1)
		wtk_free(shape_in1);
	if (shape_in2)
		wtk_free(shape_in2);

	return ret;
}

int qtk_vits_onnx_feed(qtk_vits_t* vits, wtk_veci_t **id_vec, int nid)
{
	wtk_veci_t *vec = NULL;
	int64_t *shape_in1=NULL;
	int64_t *shape_in2=NULL;
	int64_t *input1=NULL;
	int64_t *input2=NULL;
	int64_t *shape_out=NULL;
	int64_t size;
	float* v=NULL;
	int ret=0, i, j;

	//vits_syn
	shape_in1 = (int64_t*) wtk_malloc(sizeof(int64_t) * 2);
	shape_in2 = (int64_t*) wtk_malloc(sizeof(int64_t) * 1);
	for (i=0; i < 2; i++)
		shape_in1[i]=1;
	for (i=0; i < 1; i++)
		shape_in2[i]=1;

    for(i = 0; i < nid; i++){
        vec = id_vec[i];
        input1 = (int64_t*) wtk_malloc(sizeof(int64_t) * vec->len);
        for(j = 0; j < vec->len;++j){
        	input1[j] = vec->p[j];
        }
        //tst
        shape_in1[2-1]=vec->len;
        qtk_vits_onnx_feed_inparam(vits->onnx, input1, vec->len, shape_in1, 2, 2, 0);
        //tst_length
        input2 = (int64_t*) wtk_malloc(sizeof(int64_t) * 1);
        input2[1-1] = vec->len;
        qtk_vits_onnx_feed_inparam(vits->onnx, input2, 1, shape_in2, 1, 2, 1);

        ret=qtk_vits_onnx_run(vits->onnx);
        if (ret!=0){
        	qtk_vits_onnx_reset(vits->onnx);
        	printf("Err: run-onnx\n");
        	goto end;
        }
    	shape_out = qtk_vits_onnx_get_outshape(vits->onnx,0,&size);
        v = qtk_vits_onnx_getout(vits->onnx,0);
        if (i == nid-1)
        	qtk_vits_convertwav(vits, v, shape_out[size-1], 1);
        else
        	qtk_vits_convertwav(vits, v, shape_out[size-1], 0);
        qtk_vits_onnx_reset(vits->onnx);
    	wtk_free(input1);
    	wtk_free(input2);
    }
end:
	if (shape_in1)
		wtk_free(shape_in1);
	if (shape_in2)
		wtk_free(shape_in2);
	if (shape_out)
		wtk_free(shape_out);

	return ret;
}

int qtk_vits_feed(qtk_vits_t* vits, wtk_veci_t **id_vec, int nid)
{
	int ret=0;

	if (vits->dec_onnx)
		ret = qtk_vits_onnx_prefeed(vits, id_vec, nid);
	else
		ret = qtk_vits_onnx_feed(vits, id_vec, nid);

	return ret;
}
int qtk_vits_reset(qtk_vits_t* vits)
{
	qtk_vits_onnx_reset(vits->onnx);
	if (vits->dec_onnx)
		qtk_vits_onnx_reset(vits->dec_onnx);
	if (vits->wsola)
		wtk_wsola_reset(vits->wsola);
	vits->maxf = 32767.0f;
	return 0;
}

void qtk_vits_delete(qtk_vits_t* vits)
{
	if (vits->onnx)
		qtk_vits_onnx_delete(vits->onnx);
	if (vits->dec_onnx)
		qtk_vits_onnx_delete(vits->dec_onnx);
	if (vits->wsola)
		wtk_wsola_delete(vits->wsola);
	wtk_free(vits);
}

void qtk_vits_set_notify(qtk_vits_t* vits, qtk_vits_notify_f notify, void* user_data)
{
	vits->notify = notify;
	vits->user_data = user_data;
}
