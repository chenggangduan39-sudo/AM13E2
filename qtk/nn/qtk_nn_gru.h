#ifndef QBL_NN_QBL_NN_GRU_H
#define QBL_NN_QBL_NN_GRU_H
#pragma once
#include "qtk/core/qtk_binary.h"
#include "qtk/core/qtk_type.h"
#include "qtk/nn/qtk_nn_utils.h"
#include "qtk/nn/vm/qtk_nn_vm.h"
#include "qtk/numeric/qtk_numeric_type.h"
#ifdef __cplusplus
extern "C" {
#endif

#ifdef USE_NEON
#include "qtk/nn/neon_mathfun.h"
#include <arm_neon.h>

qtk_maybe_unused static void vm_GRU_(qtk_nn_vm_t *nv, uint8_t **instructions) {
    uint16_t x = qtk_littleEndian_uint16_from_bin(*instructions),
             w = qtk_littleEndian_uint16_from_bin(*instructions + 2),
             r = qtk_littleEndian_uint16_from_bin(*instructions + 4),
             b = qtk_littleEndian_uint16_from_bin(*instructions + 6),
             s1 = qtk_littleEndian_uint16_from_bin(*instructions + 10),

             y = qtk_littleEndian_uint16_from_bin(*instructions + 12),
             s2 = qtk_littleEndian_uint16_from_bin(*instructions + 14);
    qtk_numeric_data_t X, Y, W, R, B, S1, S2;
    uint32_t *in_shape;

    X.raw = qtk_nn_get_loc_from_repr(nv, x);
    W.raw = qtk_nn_get_loc_from_repr(nv, w);
    R.raw = qtk_nn_get_loc_from_repr(nv, r);
    B.raw = qtk_nn_get_loc_from_repr(nv, b);
    S1.raw = qtk_nn_get_loc_from_repr(nv, s1);
    Y.raw = qtk_nn_get_loc_from_repr(nv, y);
    S2.raw = qtk_nn_get_loc_from_repr(nv, s2);
    in_shape = qtk_nn_get_shape_from_repr(nv, x);

    *instructions += 16;
    int extra_index = qtk_littleEndian_uint16_from_bin(*instructions);
    *instructions += 2;
    uint8_t *extra = cast(uint8_t *, nv->extra) + extra_index;

    int hidden_size;
    memcpy(&hidden_size, extra, sizeof(int));
    int seq_len = in_shape[0];
    int input_dim = in_shape[2];

    int num_ouptut_blk = hidden_size >> 2;
    float *h = S1.f32;

    ensure_workspace_(nv, sizeof(float) * 2 * hidden_size);
    float *gates = nv->workspace.f32;

    for (int sl = 0; sl < seq_len; sl++) {
        float *z = X.f32 + sl * input_dim;
        float *y = Y.f32 + sl * hidden_size;
        float *gates_data = gates;
        int i = 0;

        for (; i < num_ouptut_blk; i++) {
            float *bias = B.f32 + i * 4 * 4;
            float *wz = W.f32 + (i * 12) * input_dim;
            float *wh = R.f32 + (i * 12) * hidden_size;

            float32x4_t sum_z = vld1q_f32(bias);
            float32x4_t sum_r = vld1q_f32(bias + 4);
            float32x4_t sum1 = vdupq_n_f32(.0f);
            float32x4_t sum2 = vdupq_n_f32(.0f);
            float32x4_t sum3 = vdupq_n_f32(.0f);
            float32x4_t sum4 = vdupq_n_f32(.0f);
            float32x4_t sum5 = vdupq_n_f32(.0f);
            float32x4_t sum6 = vdupq_n_f32(.0f);
            int j = 0;
            for (; j + 3 < input_dim; j += 4) {
                float32x4_t zj = vld1q_f32(z + j);

                float32x4_t wz_z = vld1q_f32(wz);
                float32x4_t wz_r = vld1q_f32(wz + 4);
                float32x4_t wz_z1 = vld1q_f32(wz + 8);
                float32x4_t wz_r1 = vld1q_f32(wz + 12);
                float32x4_t wz_z2 = vld1q_f32(wz + 16);
                float32x4_t wz_r2 = vld1q_f32(wz + 20);
                float32x4_t wz_z3 = vld1q_f32(wz + 24);
                float32x4_t wz_r3 = vld1q_f32(wz + 28);

                sum_z = vmlaq_lane_f32(sum_z, wz_z, vget_low_f32(zj), 0);
                sum_r = vmlaq_lane_f32(sum_r, wz_r, vget_low_f32(zj), 0);
                sum1 = vmlaq_lane_f32(sum1, wz_z1, vget_low_f32(zj), 1);
                sum2 = vmlaq_lane_f32(sum2, wz_r1, vget_low_f32(zj), 1);
                sum3 = vmlaq_lane_f32(sum3, wz_z2, vget_high_f32(zj), 0);
                sum4 = vmlaq_lane_f32(sum4, wz_r2, vget_high_f32(zj), 0);
                sum5 = vmlaq_lane_f32(sum5, wz_z3, vget_high_f32(zj), 1);
                sum6 = vmlaq_lane_f32(sum6, wz_r3, vget_high_f32(zj), 1);
                wz += 32;
            }

            for (; j < input_dim; j++) {
                float32x4_t zj = vdupq_n_f32(z[j]);
                float32x4_t wz_z = vld1q_f32(wz);
                float32x4_t wz_r = vld1q_f32(wz + 4);
                sum_z = vmlaq_f32(sum_z, wz_z, zj);
                sum_r = vmlaq_f32(sum_r, wz_r, zj);
                wz += 8;
            }

            j = 0;
            for (; j + 3 < hidden_size; j += 4) {
                float32x4_t hj = vld1q_f32(h + j);

                float32x4_t wh_z = vld1q_f32(wh);
                float32x4_t wh_r = vld1q_f32(wh + 4);
                float32x4_t wh_z1 = vld1q_f32(wh + 8);
                float32x4_t wh_r1 = vld1q_f32(wh + 12);
                float32x4_t wh_z2 = vld1q_f32(wh + 16);
                float32x4_t wh_r2 = vld1q_f32(wh + 20);
                float32x4_t wh_z3 = vld1q_f32(wh + 24);
                float32x4_t wh_r3 = vld1q_f32(wh + 28);

                sum_z = vmlaq_lane_f32(sum_z, wh_z, vget_low_f32(hj), 0);
                sum_r = vmlaq_lane_f32(sum_r, wh_r, vget_low_f32(hj), 0);
                sum1 = vmlaq_lane_f32(sum1, wh_z1, vget_low_f32(hj), 1);
                sum2 = vmlaq_lane_f32(sum2, wh_r1, vget_low_f32(hj), 1);
                sum3 = vmlaq_lane_f32(sum3, wh_z2, vget_high_f32(hj), 0);
                sum4 = vmlaq_lane_f32(sum4, wh_r2, vget_high_f32(hj), 0);
                sum5 = vmlaq_lane_f32(sum5, wh_z3, vget_high_f32(hj), 1);
                sum6 = vmlaq_lane_f32(sum6, wh_r3, vget_high_f32(hj), 1);

                wh += 32;
            }

            for (; j < hidden_size; j++) {
                float32x4_t hj = vdupq_n_f32(h[j]);
                float32x4_t wh_z = vld1q_f32(wh);
                float32x4_t wh_r = vld1q_f32(wh + 4);
                sum_z = vmlaq_f32(sum_z, wh_z, hj);
                sum_r = vmlaq_f32(sum_r, wh_r, hj);
                wh += 8;
            }

            sum_z = vaddq_f32(sum_z, sum1);
            sum_r = vaddq_f32(sum_r, sum2);
            sum3 = vaddq_f32(sum3, sum5);
            sum4 = vaddq_f32(sum4, sum6);
            sum_z = vaddq_f32(sum_z, sum3);
            sum_r = vaddq_f32(sum_r, sum4);

            sum_z = sigmoid_ps(sum_z);
            sum_r = sigmoid_ps(sum_r);

            float32x4_t sum_h = vld1q_f32(bias + 8);
            sum1 = vdupq_n_f32(.0f);
            sum2 = vdupq_n_f32(.0f);
            sum3 = vdupq_n_f32(.0f);

            j = 0;
            for (; j + 3 < hidden_size; j += 4) {
                float32x4_t hj = vld1q_f32(h + j);
                float32x4_t wh_h = vld1q_f32(wh);
                float32x4_t wh_h_1 = vld1q_f32(wh + 4);
                float32x4_t wh_h_2 = vld1q_f32(wh + 8);
                float32x4_t wh_h_3 = vld1q_f32(wh + 12);

                sum_h = vmlaq_lane_f32(sum_h, wh_h, vget_low_f32(hj), 0);
                sum1 = vmlaq_lane_f32(sum1, wh_h_1, vget_low_f32(hj), 1);
                sum2 = vmlaq_lane_f32(sum2, wh_h_2, vget_high_f32(hj), 0);
                sum3 = vmlaq_lane_f32(sum3, wh_h_3, vget_high_f32(hj), 1);

                wh += 16;
            }

            for (; j < hidden_size; j++) {
                float32x4_t hj = vld1q_f32(h + j);
                float32x4_t wh_h = vld1q_f32(wh);
                sum_h = vmlaq_f32(sum_h, wh_h, hj);
                wh += 4;
            }

            sum_h = vaddq_f32(sum_h, sum1);
            sum2 = vaddq_f32(sum2, sum3);
            sum_h = vaddq_f32(sum_h, sum2);

            sum_h = vmlaq_f32(vld1q_f32(bias + 12), sum_z, sum_h);
            sum1 = vdupq_n_f32(.0f);
            sum2 = vdupq_n_f32(.0f);
            sum3 = vdupq_n_f32(.0f);

            j = 0;
            for (; j + 3 < input_dim; j += 4) {
                float32x4_t zj = vld1q_f32(z + j);

                float32x4_t wz_h = vld1q_f32(wz);
                float32x4_t wz_h_1 = vld1q_f32(wz + 4);
                float32x4_t wz_h_2 = vld1q_f32(wz + 8);
                float32x4_t wz_h_3 = vld1q_f32(wz + 12);

                sum_h = vmlaq_lane_f32(sum_h, wz_h, vget_low_f32(zj), 0);
                sum1 = vmlaq_lane_f32(sum1, wz_h_1, vget_low_f32(zj), 1);
                sum2 = vmlaq_lane_f32(sum2, wz_h_2, vget_high_f32(zj), 0);
                sum3 = vmlaq_lane_f32(sum3, wz_h_3, vget_high_f32(zj), 1);

                wz += 16;
            }

            for (; j < input_dim; j++) {
                float32x4_t zj = vdupq_n_f32(z[j]);
                float32x4_t wz_h = vld1q_f32(wz);
                sum_h = vmlaq_f32(sum_h, wz_h, zj);
                wz += 4;
            }

            sum_h = vaddq_f32(sum_h, sum1);
            sum2 = vaddq_f32(sum2, sum3);
            sum_h = vaddq_f32(sum_h, sum2);

            sum_h = tanh_ps(sum_h);

            vst1q_f32(gates_data, sum_r);
            vst1q_f32(gates_data + 4, sum_h);
            gates_data += 8;
        }

        for (; i < hidden_size; i++) {
            // TODO
        }

        gates_data = gates;
        for (i = 0; i < num_ouptut_blk; i++) {
            float32x4_t sum_r = vld1q_f32(gates_data);
            float32x4_t sum_h = vld1q_f32(gates_data + 4);
            float32x4_t sum_new_h =
                vaddq_f32(vmulq_f32(vsubq_f32(vdupq_n_f32(1.f), sum_r), sum_h),
                          vmulq_f32(sum_r, vld1q_f32(h + i * 4)));
            vst1q_f32(y + i * 4, sum_new_h);
            gates_data += 8;
        }
        h = y;
    }
    memcpy(S2.raw, h, sizeof(float) * hidden_size);
}
#else
qtk_maybe_unused static void
gru_sigmoid_(float *m,int len)
{
    int i;

	for(i=0;i<len;++i)
	{
		*m=1.0/(1.0+expf(-*m));
		++m;
	}
}

qtk_maybe_unused static void
gru_tanh_(float *f,int len)
{
	float *p;
	float *e;
	float inv_expx,expx;
	p=f;e=p+len;
	while(p<e)
	{
		if(*p>0.0)
		{
			inv_expx=expf(-(*p));
			*p=-1.0+2.0/(1.0+inv_expx*inv_expx);
		}else
		{
			expx=expf(*p);
			*p=1.0-2.0/(1.0+expx*expx);
		}
		++p;
	}
}

qtk_maybe_unused static void
update_gru_layer_wb_rz_(float *it, float *data, const float *h_out, float *weii,
                        float *biasi, float *weih, float *biash, int input_idim,
                        int input_hdim, int gru_hidden) {
    float *weii1,*weih1;
    int i,j;
    int idim_1, hdim_1;
    idim_1 = (int)(input_idim >> 2) << 2;
    hdim_1 = (int)(input_hdim >> 2) << 2;

    float tmp1, tmp2, tmp3, tmp4, tmp5, tmp6;
    for(i=0;i<gru_hidden;++i,++it,++biasi,++biash)
    {   
        weii1 = weii + i * input_idim;
        weih1 = weih + i * input_hdim;
        tmp1=tmp2=tmp3=tmp4=tmp5=0;
        for(j=0;j<idim_1;j += 4)
        {   
            tmp1+=weii1[j]*data[j];
            tmp2+=weii1[j + 1]*data[j + 1];
            tmp3+=weii1[j + 2]*data[j + 2];
            tmp4+=weii1[j + 3]*data[j + 3];
        }
        tmp5 += (tmp1 + tmp2) + (tmp3 + tmp4);
        for(j=idim_1;j<input_idim;++j){
            tmp5+=weii1[j]*data[j];
        }

        tmp1=tmp2=tmp3=tmp4=tmp6=0;
        for(j=0;j<hdim_1;j += 4)
        {
            tmp1+=weih1[j]*h_out[j];
            tmp2+=weih1[j + 1]*h_out[j + 1];
            tmp3+=weih1[j + 2]*h_out[j + 2];
            tmp4+=weih1[j + 3]*h_out[j + 3];
        }
        tmp6 += (tmp1 + tmp2) + (tmp3 + tmp4);
        for(j=hdim_1;j<input_hdim;++j){
            tmp6+=weih1[j]*h_out[j];
        }

        *it+=(*biasi+*biash) + (tmp5 + tmp6);
    }
}

qtk_maybe_unused static void
update_gru_layer_wb_n_(float *nt, float *rt, float *data, const float *h_out,
                       float *weii, float *biasi, float *weih, float *biash,
                       int input_idim, int input_hdim, int gru_hidden) {
    float *weii1,*weih1;
    int i,j;
    float tmp;
    int idim_1, hdim_1;
    idim_1 = (int)(input_idim >> 2) << 2;
    hdim_1 = (int)(input_hdim >> 2) << 2;

    float tmp1, tmp2, tmp3, tmp4, tmp5;
    for(i=0;i<gru_hidden;++i,++nt,++biasi,++biash,++rt)
    {
        weii1 = weii + i * input_idim;
        weih1 = weih + i * input_hdim;
        tmp1=tmp2=tmp3=tmp4=tmp5=0;
        for(j=0;j<idim_1;j+=4)
        {
            tmp1+=weii1[j]*data[j];
            tmp2+=weii1[j + 1]*data[j + 1];
            tmp3+=weii1[j + 2]*data[j + 2];
            tmp4+=weii1[j + 3]*data[j + 3];
        }
        tmp5 += (tmp1 + tmp2) + (tmp3 + tmp4);
        for(j=idim_1;j<input_idim;++j){
            tmp5+=weii1[j]*data[j];
        }
        tmp=tmp1=tmp2=tmp3=tmp4=0;
        for(j=0;j<hdim_1;j+=4)
        {
            tmp1+=weih1[j]*h_out[j];
            tmp2+=weih1[j + 1]*h_out[j + 1];
            tmp3+=weih1[j + 2]*h_out[j + 2];
            tmp4+=weih1[j + 3]*h_out[j + 3];
        }
        tmp += *biash + (tmp1 + tmp2) + (tmp3 + tmp4);
        for(j=hdim_1;j<input_hdim;++j){
            tmp+=weih1[j]*h_out[j];
        }
        *nt+=*biasi + *rt*tmp + tmp5;
    }
}

qtk_maybe_unused static void vm_GRU_(qtk_nn_vm_t *nv, uint8_t **instructions) {
    uint16_t x = qtk_littleEndian_uint16_from_bin(*instructions),
             w = qtk_littleEndian_uint16_from_bin(*instructions + 2),
             r = qtk_littleEndian_uint16_from_bin(*instructions + 4),
             b = qtk_littleEndian_uint16_from_bin(*instructions + 6),
             s1 = qtk_littleEndian_uint16_from_bin(*instructions + 10),

             y = qtk_littleEndian_uint16_from_bin(*instructions + 12),
             s2 = qtk_littleEndian_uint16_from_bin(*instructions + 14);
    qtk_numeric_data_t X, Y, W, R, B, S1, S2;
    uint32_t *in_shape;

    X.raw = qtk_nn_get_loc_from_repr(nv, x);
    W.raw = qtk_nn_get_loc_from_repr(nv, w);
    R.raw = qtk_nn_get_loc_from_repr(nv, r);
    B.raw = qtk_nn_get_loc_from_repr(nv, b);
    S1.raw = qtk_nn_get_loc_from_repr(nv, s1);
    Y.raw = qtk_nn_get_loc_from_repr(nv, y);
    S2.raw = qtk_nn_get_loc_from_repr(nv, s2);
    in_shape = qtk_nn_get_shape_from_repr(nv, x);

    *instructions += 16;
    int extra_index = qtk_littleEndian_uint16_from_bin(*instructions);
    *instructions += 2;
    uint8_t *extra = cast(uint8_t *, nv->extra) + extra_index;

    int gru_hidden;
    memcpy(&gru_hidden,extra,sizeof(int));
    // print_float(X.f32,10);
    // printf("end\n");
    int seq_len = in_shape[0];
    int input_idim = in_shape[2];
    int input_hdim = in_shape[2];
    float *weih = R.f32;
    float *biash = B.f32 + 3 * gru_hidden;;
    float *biasi = B.f32;
    float *weii = W.f32;
    float *h_out = S2.f32;
    ensure_workspace_(nv, sizeof(float) * 3 * gru_hidden);
    memset(nv->workspace.f32, 0, sizeof(float) * 3 * gru_hidden);
    float* zt = nv->workspace.f32;
    float* rt = zt + gru_hidden;
    float* nt = rt + gru_hidden;

    const float *h_in = S1.f32;
    for (int sl = 0; sl < seq_len; sl++) {
        memset(nv->workspace.f32, 0, sizeof(float) * 3 * gru_hidden);
        update_gru_layer_wb_rz_(zt, X.f32 + input_idim * sl, h_in, weii, biasi,
                                weih, biash, input_idim, input_hdim,
                                gru_hidden);
        gru_sigmoid_(zt, gru_hidden);

        update_gru_layer_wb_rz_(
            rt, X.f32 + input_idim * sl, h_in, weii + gru_hidden * input_hdim,
            biasi + gru_hidden, weih + gru_hidden * input_hdim,
            biash + gru_hidden, input_idim, input_hdim, gru_hidden);
        gru_sigmoid_(rt, gru_hidden);

        update_gru_layer_wb_n_(
            nt, rt, X.f32 + input_idim * sl, h_in,
            weii + 2 * gru_hidden * input_hdim, biasi + 2 * gru_hidden,
            weih + 2 * gru_hidden * input_hdim, biash + 2 * gru_hidden,
            input_idim, input_hdim, gru_hidden);

        gru_tanh_(nt, gru_hidden);

        for (int i = 0; i < gru_hidden; ++i) {
            h_out[i] = (1 - zt[i]) * nt[i] + zt[i] * h_in[i];
        }
        memcpy(Y.f32 + gru_hidden * sl, h_out, gru_hidden * sizeof(float));
        h_in = h_out;
    }
}
#endif

qtk_maybe_unused static int vm_GRU_infer_shape_(qtk_nn_vm_t *nv,
                                                 uint8_t *instructions) {
    //TODO
    return 0;
}

#ifdef __cplusplus
};

#endif
#endif
