#include "qtk_soundfield_syntheis.h"
#include "wtk/core/math/wtk_math.h"

#define WTK_EPSILON 1e-12f

static void complex_dump(wtk_complex_t *c, int len){
    // int i;
    // for(i = 0; i < len; i++){
    //     printf("%f %f\n", c[i].a, c[i].b);
    // }
    int i;
    for(i = 0; i < len; i++){
            printf("%.6g %.6gj\n", c[i].a, c[i].b);
    }
    printf("==============\n");
}

float* caculate_lambda2_vector_optimized(qtk_soundfield_syntheis_t *sos,float lambda2, int n_bin, int sample_rate) {
    float nyquist = sample_rate / 2.0;
    float* lambda2_vector = (float*)wtk_malloc(n_bin * sizeof(float));
    for (int i = 0; i < n_bin; i++) {
        float f = i * nyquist / (n_bin - 1);
        float weight;
        if (f < 31) weight = sos->cfg->lambda_vector[0];
        else if (f < 63) weight = sos->cfg->lambda_vector[1];
        else if (f < 125) weight = sos->cfg->lambda_vector[2];
        else if (f < 250) weight = sos->cfg->lambda_vector[3];
        else if (f < 500) weight = sos->cfg->lambda_vector[4];
        else if (f < 1000) weight = sos->cfg->lambda_vector[5];
        else if (f < 2000) weight = sos->cfg->lambda_vector[6];
        else weight = sos->cfg->lambda_vector[7];
        lambda2_vector[i] = lambda2 * weight;
    }
    return lambda2_vector;
}

void calculate_distance_frequency_weights(qtk_soundfield_syntheis_t* sos,int n_bin, int N, float sample_rate) {
    sos->weights = (float**)malloc(n_bin * sizeof(float*));
    float **weights = sos->weights;
    for (int i = 0; i < n_bin; i++) {
        weights[i] = (float*)malloc(N * sizeof(float));
    }

    float **positions = sos->cfg->positions;
    float *source_position= sos->cfg->source_position;
    float *reference_position= sos->cfg->reference_position;

    float* dist_to_source = (float*)malloc(N * sizeof(float)); ;
    for (int j = 0; j < N; j++) {
        float sum = 0.0;
        for (int k = 0; k < 3; k++) {
            sum += (positions[j][k] - source_position[k]) * (positions[j][k] - source_position[k]);
        }
        dist_to_source[j] = sqrt(sum);
    }

    float dist_ref_to_source = 0.0;
    for (int k = 0; k < 3; k++) {
        dist_ref_to_source += (reference_position[k] - source_position[k]) * (reference_position[k] - source_position[k]);
    }
    dist_ref_to_source = sqrt(dist_ref_to_source);

    float *delta_d = (float*)malloc(N * sizeof(float)); 
    for (int j = 0; j < N; j++) {
        delta_d[j] = dist_to_source[j] - dist_ref_to_source;
    }

    float *frequencies = (float*)malloc(n_bin * sizeof(float)); 
    for (int i = 0; i < n_bin; i++) {
        frequencies[i] = (i < n_bin/2) ? (i * sample_rate / n_bin) : (-(n_bin - i) * sample_rate / n_bin);
    }

    for (int i = 0; i < n_bin; i++) {
        float freq = fabs(frequencies[i]);
        for (int j = 0; j < N; j++) { 
            float position_weight;
            if (delta_d[j] < 0) {
                position_weight = 0.5;
            } else {
                position_weight = 1.0 + 1.0 * delta_d[j];
            }

            float freq_weight;
            if (freq < 31) freq_weight = sos->cfg->freq_weight[0];// 1e-2;
            else if (freq < 63) freq_weight = sos->cfg->freq_weight[1];
            else if (freq < 125) freq_weight = sos->cfg->freq_weight[2];
            else if (freq < 250) freq_weight = sos->cfg->freq_weight[3];
            else if (freq < 500) freq_weight = sos->cfg->freq_weight[4];
            else if (freq < 1000) freq_weight = sos->cfg->freq_weight[5];
            else if (freq < 2000) freq_weight = sos->cfg->freq_weight[6];
            else freq_weight = sos->cfg->freq_weight[7];

            weights[i][j] = position_weight * freq_weight;
        }
    }
    wtk_free(frequencies);
    wtk_free(dist_to_source);
    wtk_free(delta_d);
}


sfs_linear_conv_t* conv_new(int L1, int L2, int n) {
    sfs_linear_conv_t* ctx = (sfs_linear_conv_t*)wtk_malloc(sizeof(sfs_linear_conv_t));
    int i;

    ctx->n = n;
    ctx->L1 = L1;
    ctx->L2 = L2;
    ctx->L = L1 + L2;
    ctx->cache = (float**)wtk_malloc(sizeof(float*)*n);
    ctx->cache_x = (float**)wtk_malloc(sizeof(float*)*n);
    ctx->fft_n = ctx->L/2 + 1;
    ctx->fft_buf = (wtk_complex_t*)wtk_calloc(ctx->L/2 + 1, sizeof(wtk_complex_t));
    ctx->drft = wtk_drft_new(ctx->L);
    ctx->weight = (float**)wtk_malloc(n * sizeof(float*));
    ctx->WEIGHT = wtk_complex_new_p2_2(ctx->n, ctx->fft_n);
    ctx->output_buf = (float**)wtk_malloc(n * sizeof(float*));
    ctx->output = (float**)wtk_malloc(n * sizeof(float*));
    for(i = 0; i < n; i++){
        ctx->cache[i] = (float*)wtk_calloc(ctx->L2, sizeof(float));
        ctx->cache_x[i] = (float*)wtk_calloc(ctx->L, sizeof(float));
        ctx->weight[i] = (float*)wtk_calloc(ctx->L, sizeof(float));
        ctx->output_buf[i] = (float*)wtk_calloc(L1 + L2, sizeof(float));
        ctx->output[i] = ctx->output_buf[i] + L2;
    }
    return ctx;
}

void conv_delete(sfs_linear_conv_t *ctx){
    int i;

    for(i = 0; i < ctx->n; i++){
        wtk_free(ctx->cache[i]);
        wtk_free(ctx->cache_x[i]);
        wtk_free(ctx->output_buf[i]);
        if(ctx->weight){
            wtk_free(ctx->weight[i]);
        }
    }
    wtk_free(ctx->cache);
    wtk_free(ctx->cache_x);
    wtk_free(ctx->output_buf);
    if(ctx->weight){
        wtk_free(ctx->weight);
    }
    wtk_free(ctx->output);
    wtk_free(ctx->fft_buf);
    wtk_complex_delete_p2_2(ctx->WEIGHT);
    wtk_drft_delete(ctx->drft);
    wtk_free(ctx);
}

sfs_linear_conv_t *conv_load(wtk_strbuf_t* buf){
    sfs_linear_conv_t* ctx = (sfs_linear_conv_t*)wtk_malloc(sizeof(sfs_linear_conv_t));
    int i,j;
    int *data = (int*)buf->data;
    ctx->n = data[0];
    ctx->L1 = data[1];
    ctx->L2 = data[2];
    data = data + 3;

    ctx->L = ctx->L1 + ctx->L2;
    ctx->fft_n = ctx->L/2 + 1;
    ctx->weight = NULL;
    ctx->fft_n = ctx->L/2 + 1;

    float *weight_real = (float*)data;
    float *weight_imag = weight_real + ctx->n * (ctx->L/2 + 1);

    ctx->fft_buf = (wtk_complex_t*)wtk_calloc(ctx->L/2 + 1, sizeof(wtk_complex_t));
    ctx->drft = wtk_drft_new(ctx->L);

    ctx->cache = (float**)wtk_malloc(sizeof(float*)*ctx->n);
    ctx->cache_x = (float**)wtk_malloc(sizeof(float*)*ctx->n);
    ctx->WEIGHT = wtk_complex_new_p2_2(ctx->n, ctx->fft_n);
    ctx->output_buf = (float**)wtk_malloc(ctx->n * sizeof(float*));
    ctx->output = (float**)wtk_malloc(ctx->n * sizeof(float*));

    for(i = 0; i < ctx->n; i++){
        ctx->cache[i] = (float*)wtk_calloc(ctx->L2, sizeof(float));
        ctx->cache_x[i] = (float*)wtk_calloc(ctx->L, sizeof(float));
        for(j = 0; j < ctx->fft_n; j++,weight_real++,weight_imag++){
            ctx->WEIGHT[i][j].a = *weight_real;
            ctx->WEIGHT[i][j].b = *weight_imag;
        }
        ctx->output_buf[i] = (float*)wtk_calloc(ctx->L, sizeof(float));
        ctx->output[i] = ctx->output_buf[i] + ctx->L2;
    }
    return ctx;
}

void conv_reset_weight2(sfs_linear_conv_t* ctx, float* weight1,float *weight2) {
    int i;
    float *weight[2];
    weight[0] = weight1;
    weight[1] = weight2;
    for(i = 0; i < ctx->n; i++){
        memset(ctx->weight[i], 0, ctx->L * sizeof(float));
        memcpy(ctx->weight[i], weight[i], (ctx->L2 * sizeof(float)));//TODO
        wtk_drft_fft2_x(ctx->drft, ctx->weight[i], ctx->WEIGHT[i]);
        for(int j = 0; j < ctx->L/2 + 1; j++){
            ctx->WEIGHT[i][j].a *= ctx->L;
            ctx->WEIGHT[i][j].b *= ctx->L;
        }
    }
    //complex_dump(ctx->WEIGHT, ctx->L/2 + 1);
}

void conv_reset_weight(sfs_linear_conv_t* ctx, float* weight) {
    int i;
    for(i = 0; i < ctx->n; i++){
        memset(ctx->weight[i], 0, ctx->L * sizeof(float));
        memcpy(ctx->weight[i], weight + ctx->L2 * i, (ctx->L2 * sizeof(float)));//TODO
        wtk_drft_fft2_x(ctx->drft, ctx->weight[i], ctx->WEIGHT[i]);
        for(int j = 0; j < ctx->L/2 + 1; j++){
            ctx->WEIGHT[i][j].a *= ctx->L;
            ctx->WEIGHT[i][j].b *= ctx->L;
        }
    }
    //complex_dump(ctx->WEIGHT, ctx->L/2 + 1);
}

void conv_process(sfs_linear_conv_t* ctx, float* input) {
    float A, B, C;
    wtk_complex_t* p1, *p2;
    int i;

    for(i = 0; i < ctx->n; i++){
        memcpy(ctx->cache_x[i], ctx->cache[i], ctx->L2 * sizeof(float));
        memcpy(ctx->cache_x[i] + ctx->L2, input, ctx->L1 * sizeof(float));
        memcpy(ctx->cache[i], ctx->cache_x[i] + ctx->L1, ctx->L2 * sizeof(float));
        wtk_drft_fft2_x(ctx->drft, ctx->cache_x[i], ctx->fft_buf);
        for(int j = 0; j < ctx->L/2 + 1; j++){
            //ctx->fft_buf[i].a *= ctx->L;
            //ctx->fft_buf[i].b *= ctx->L;
            p1 = ctx->fft_buf + j;
            p2 = ctx->WEIGHT[i] + j;
            A = (p1->a + p1->b) * p2->a;
            B = (p2->a + p2->b) * p1->b;
            C = (p1->b - p1->a) * p2->b;
            ctx->fft_buf[j].a = (A - B) * ctx->L;
            ctx->fft_buf[j].b = (B - C) * ctx->L;
        }
        //complex_dump(ctx->WEIGHT[i],20);
        //complex_dump(ctx->fft_buf,20);
        wtk_drft_ifft2_x(ctx->drft, ctx->fft_buf, ctx->output_buf[i]);
        for(int j = ctx->L2; j < ctx->L; j++){
            ctx->output_buf[i][j] /= ctx->L;
        }
    }
}

// 矩阵乘法: C = A * B
wtk_complex_t** matrix_multiply(wtk_complex_t** A, wtk_complex_t** B, int m, int n, int k) {
    wtk_complex_t** C = wtk_complex_new_p2_2(m, n);
    if (!C) return NULL;

    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
            float real_sum = 0.0f;
            float imag_sum = 0.0f;
            for (int c = 0; c < k; c++) {
                real_sum += A[i][c].a * B[c][j].a - A[i][c].b * B[c][j].b;
                imag_sum += A[i][c].a * B[c][j].b + A[i][c].b * B[c][j].a;
            }
            C[i][j].a = real_sum;
            C[i][j].b = imag_sum;
        }
    }
    return C;
}

// 矩阵共轭转置
wtk_complex_t** matrix_hermitian(wtk_complex_t** A, int row, int col) {
    wtk_complex_t** result = wtk_complex_new_p2_2(col, row);
    if (!result) return NULL;

    for (int i = 0; i < row; i++) {
        for (int j = 0; j < col; j++) {
            result[j][i].a = A[i][j].a;
            result[j][i].b = -A[i][j].b;
        }
    }
    return result;
}

wtk_complex_t** matrix_add(wtk_complex_t** A, wtk_complex_t** B, int row, int col) {
    wtk_complex_t** C = wtk_complex_new_p2_2(row, col);
    if (!C) return NULL;

    wtk_complex_t *cpx = &(C[0][0]);
    wtk_complex_t *cpx1 = &(A[0][0]);
    wtk_complex_t *cpx2 = &(B[0][0]);
    size_t total_elements = row * col;

    for (size_t i = 0; i < total_elements; i++, cpx++, cpx1++, cpx2++) {
        cpx->a = cpx1->a + cpx2->a;
        cpx->b = cpx1->b + cpx2->b;
    }
    return C;
}

wtk_complex_t** matrix_scale(wtk_complex_t** A, float scalar, int row, int col) {
    wtk_complex_t** result = wtk_complex_new_p2_2(row, col);
    if (!result) return NULL;

    wtk_complex_t *cpx = &(result[0][0]);
    wtk_complex_t *cpx2 = &(A[0][0]);
    size_t total_elements = row * col;

    for (size_t i = 0; i < total_elements; i++, cpx++, cpx2++) {
        cpx->a = cpx2->a * scalar;
        cpx->b = cpx2->b * scalar;
    }
    return result;
}

// 创建单位矩阵
wtk_complex_t** identity_matrix(int size) {
    wtk_complex_t** I = wtk_complex_new_p2_2(size, size);
    if (!I) return NULL;

    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            I[i][j].a = 0.0f;
            I[i][j].b = 0.0f;
        }
    }

    for (int i = 0; i < size; i++) {
        I[i][i].a = 1.0f;
    }

    return I;
}

// LU分解 (高斯-约当消元法)
int matrix_lu_decomposition(wtk_complex_t** A, int n, wtk_complex_t*** L, wtk_complex_t*** U) {
    *L = wtk_complex_new_p2_2(n, n);
    *U = wtk_complex_new_p2_2(n, n);

    if (!*L || !*U) return 0;

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            (*L)[i][j].a = (i == j) ? 1.0f : 0.0f;
            (*L)[i][j].b = 0.0f;
            (*U)[i][j] = A[i][j];
        }
    }

    for (int k = 0; k < n; k++) {
        float divisor = (*U)[k][k].a * (*U)[k][k].a + (*U)[k][k].b * (*U)[k][k].b;
        if (fabs(divisor) < WTK_EPSILON) {
            return 0; // 矩阵奇异
        }

        for (int i = k + 1; i < n; i++) {
            float real_num = (*U)[i][k].a * (*U)[k][k].a + (*U)[i][k].b * (*U)[k][k].b;
            float imag_num = (*U)[i][k].b * (*U)[k][k].a - (*U)[i][k].a * (*U)[k][k].b;

            (*L)[i][k].a = real_num / divisor;
            (*L)[i][k].b = imag_num / divisor;

            for (int j = k; j < n; j++) {
                float real_part = (*L)[i][k].a * (*U)[k][j].a - (*L)[i][k].b * (*U)[k][j].b;
                float imag_part = (*L)[i][k].a * (*U)[k][j].b + (*L)[i][k].b * (*U)[k][j].a;

                (*U)[i][j].a -= real_part;
                (*U)[i][j].b -= imag_part;
            }

            (*U)[i][k].a = 0.0f;
            (*U)[i][k].b = 0.0f;
        }
    }

    return 1;
}

// 矩阵求逆 (基于LU分解)
wtk_complex_t** matrix_inverse(wtk_complex_t** A, int n) {
    wtk_complex_t **L = NULL, **U = NULL;
    if (!matrix_lu_decomposition(A, n, &L, &U)) {
        if (L) wtk_complex_delete_p2_2(L);
        if (U) wtk_complex_delete_p2_2(U);
        return NULL;
    }

    wtk_complex_t** inv = wtk_complex_new_p2_2(n, n);
    if (!inv) {
        wtk_complex_delete_p2_2(L);
        wtk_complex_delete_p2_2(U);
        return NULL;
    }

    for (int col = 0; col < n; col++) {
        wtk_complex_t y[n];
        for (int i = 0; i < n; i++) {
            y[i].a = (i == col) ? 1.0f : 0.0f;
            y[i].b = 0.0f;

            for (int j = 0; j < i; j++) {
                float real_part = L[i][j].a * y[j].a - L[i][j].b * y[j].b;
                float imag_part = L[i][j].a * y[j].b + L[i][j].b * y[j].a;

                y[i].a -= real_part;
                y[i].b -= imag_part;
            }
        }

        for (int i = n - 1; i >= 0; i--) {
            inv[i][col] = y[i];

            for (int j = i + 1; j < n; j++) {
                float real_part = U[i][j].a * inv[j][col].a - U[i][j].b * inv[j][col].b;
                float imag_part = U[i][j].a * inv[j][col].b + U[i][j].b * inv[j][col].a;

                inv[i][col].a -= real_part;
                inv[i][col].b -= imag_part;
            }

            float divisor = U[i][i].a * U[i][i].a + U[i][i].b * U[i][i].b;
            if (fabs(divisor) < WTK_EPSILON) {
                wtk_complex_delete_p2_2(L);
                wtk_complex_delete_p2_2(U);
                wtk_complex_delete_p2_2(inv);
                return NULL;
            }

            float real_temp = inv[i][col].a;
            float imag_temp = inv[i][col].b;

            inv[i][col].a = (real_temp * U[i][i].a + imag_temp * U[i][i].b) / divisor;
            inv[i][col].b = (imag_temp * U[i][i].a - real_temp * U[i][i].b) / divisor;
        }
    }

    wtk_complex_delete_p2_2(L);
    wtk_complex_delete_p2_2(U);

    return inv;
}

// 解线性方程组 A * x = b[2]
wtk_complex_t** solve_linear_system(wtk_complex_t** A, wtk_complex_t** b, int n) {
    wtk_complex_t** A_inv = matrix_inverse(A, n);

    if (A_inv) {
        wtk_complex_t** x = matrix_multiply(A_inv, b, n, 1, n);
        wtk_complex_delete_p2_2(A_inv);
        return x;
    }

    return NULL;
}

// 计算驱动函数权重
void caculate_driving_func_weight(qtk_soundfield_syntheis_t *sos, float sample_rate,
                                 wtk_complex_t** H_b,
                                 wtk_complex_t** Z_b,
                                 float** truncated_ir) {
    int nbin_input = sos->nbin / 2 + 1;  // 使用实际的频点数
    int N = sos->cfg->neval;              // 参考点数量
    int M = sos->cfg->N;                 // 通道数
    int N_TAP = sos->cfg->n_tap;
    *truncated_ir = (float*)malloc(M * N_TAP * sizeof(float));
    if (!*truncated_ir) return;
    memset(*truncated_ir, 0, M * N_TAP * sizeof(float));

    // 设置参考扬声器索引
    int ref_spk_index = sos->cfg->ref_spk_index;
    if (ref_spk_index >= M) ref_spk_index = M - 1;

    // 计算权重矩阵
    calculate_distance_frequency_weights(sos, nbin_input, N, sample_rate);
    float** weights = sos->weights;

    // 计算期望声压 P_des [nbin_input][N]
    wtk_complex_t** P_des = wtk_complex_new_p2_2(nbin_input, N);

    for (int bin_idx = 0; bin_idx < nbin_input; bin_idx++) {
        wtk_complex_t H_ref = Z_b[bin_idx][ref_spk_index];
        for (int n_idx = 0; n_idx < N; n_idx++) {
            P_des[bin_idx][n_idx] = H_ref;
        }
    }

    // 为每个频点计算最优权重
    wtk_complex_t** q_opt = wtk_complex_new_p2_2(nbin_input, M);

    // 计算频率向量
    float* freq_bins = (float*)malloc(nbin_input * sizeof(float));

    for (int i = 0; i < nbin_input; i++) {
        freq_bins[i] = i * (sample_rate / 2.0f) / (nbin_input - 1);
    }

    float lambda;
    float *lambda_vec = caculate_lambda2_vector_optimized(sos, sos->cfg->lambda,nbin_input,sos->cfg->fs);

    // 对每个频率bin进行处理
    for (int bin_idx = 0; bin_idx < nbin_input; bin_idx++) {
        // 检查是否超出截止频率
        if (freq_bins[bin_idx] > sos->cfg->cutoff_freq) {
            for (int m_idx = 0; m_idx < M; m_idx++) {
                q_opt[bin_idx][m_idx].a = 0.0f;
                q_opt[bin_idx][m_idx].b = 0.0f;
            }
            continue;
        }

        // 构建当前bin的系统矩阵 [N x M]
        wtk_complex_t** H_current = wtk_complex_new_p2_2(N, M);
        if (!H_current) continue;

        for (int n_idx = 0; n_idx < N; n_idx++) {
            for (int m_idx = 0; m_idx < M; m_idx++) {
                // 直接从H_b获取值，注意H_b的维度是[nbin_input][N][M]
                // 但在内存中是按行存储的，所以索引是 bin_idx * (N*M) + n_idx * M + m_idx
                int flat_idx = n_idx * M + m_idx;
                H_current[n_idx][m_idx] = H_b[bin_idx][flat_idx];
            }
        }

        // 应用权重
        for (int n_idx = 0; n_idx < N; n_idx++) {
            float weight = weights[bin_idx][n_idx];
            float sqrt_w = sqrtf(weight);

            // 加权H_current
            for (int m_idx = 0; m_idx < M; m_idx++) {
                H_current[n_idx][m_idx].a *= sqrt_w;
                H_current[n_idx][m_idx].b *= sqrt_w;
                //wtk_debug("%f %f\n",H_current[n_idx][m_idx].a,H_current[n_idx][m_idx].b);
            }
        }

        // 构建目标向量 [N x 1]
        wtk_complex_t** P_current = wtk_complex_new_p2_2(N, 1);
        if (!P_current) {
            wtk_complex_delete_p2_2(H_current);
            continue;
        }

        for (int n_idx = 0; n_idx < N; n_idx++) {
            float weight = weights[bin_idx][n_idx];
            float sqrt_w = sqrtf(weight);

            P_current[n_idx][0].a = sqrt_w * P_des[bin_idx][n_idx].a;
            P_current[n_idx][0].b = sqrt_w * P_des[bin_idx][n_idx].b;
            //wtk_debug("%f %f\n",P_current[n_idx][0].a,P_current[n_idx][0].b);
        }

        // 构建正则化矩阵
        wtk_complex_t** I = identity_matrix(M);
        if (!I) {
            wtk_complex_delete_p2_2(H_current);
            wtk_complex_delete_p2_2(P_current);
            continue;
        }
        lambda = lambda_vec[bin_idx];
        wtk_complex_t** lambdaI = matrix_scale(I, lambda, M, M);
        wtk_complex_delete_p2_2(I);

        if (!lambdaI) {
            wtk_complex_delete_p2_2(H_current);
            wtk_complex_delete_p2_2(P_current);
            continue;
        }

        // 计算系统矩阵
        wtk_complex_t** H_current_H = matrix_hermitian(H_current, N, M);
        wtk_complex_t** HTH = matrix_multiply(H_current_H, H_current, M, M, N);
        wtk_complex_t** Denominator = matrix_add(HTH, lambdaI, M, M);
        wtk_complex_delete_p2_2(HTH);
        wtk_complex_delete_p2_2(lambdaI);
        //wtk_debug("%f %f\n",Denominator[0][0].a,Denominator[0][0].b);
        //wtk_debug("%f %f\n",Denominator[0][1].a,Denominator[0][1].b);
        //wtk_debug("%f %f\n",Denominator[0][2].a,Denominator[0][2].b);
        // 计算分子
        wtk_complex_t** Numerator = matrix_multiply(H_current_H, P_current, M, 1, N);
        wtk_complex_delete_p2_2(H_current_H);
        wtk_complex_delete_p2_2(P_current);
        wtk_complex_delete_p2_2(H_current);
        // wtk_debug("%f %f\n",Numerator[0][0].a,Numerator[0][0].b);
        // wtk_debug("%f %f\n",Numerator[0][1].a,Numerator[0][1].b);
        // wtk_debug("%f %f\n",Numerator[0][2].a,Numerator[0][2].b);
        // wtk_debug("%f %f\n",Numerator[0][9].a,Numerator[0][9].b);
        // wtk_debug("%f %f\n",Numerator[0][10].a,Numerator[0][10].b);
        // wtk_debug("%f %f\n",Numerator[0][11].a,Numerator[0][11].b);
        if (!Numerator) {
            wtk_complex_delete_p2_2(Denominator);
            continue;
        }

        // 求解线性系统
        wtk_complex_t** q_bin = solve_linear_system(Denominator, Numerator, M);
        wtk_complex_delete_p2_2(Denominator);
        wtk_complex_delete_p2_2(Numerator);

        if (q_bin) {
            for (int m_idx = 0; m_idx < M; m_idx++) {
                q_opt[bin_idx][m_idx] = q_bin[m_idx][0];
                //wtk_debug("%f %f\n",q_opt[bin_idx][m_idx].a,q_opt[bin_idx][m_idx].b);
            }
            wtk_complex_delete_p2_2(q_bin);
            //exit(0);
        } else {
            for (int m_idx = 0; m_idx < M; m_idx++) {
                q_opt[bin_idx][m_idx].a = 0.0f;
                q_opt[bin_idx][m_idx].b = 0.0f;
            }
        }
    }

    // IFFT和脉冲响应截断
    for (int m = 0; m < M; m++) {
        // 提取当前通道的频域响应
        wtk_complex_t* channel_response = (wtk_complex_t*)malloc(nbin_input * sizeof(wtk_complex_t));
        if (!channel_response) continue;

        for (int bin_idx = 0; bin_idx < nbin_input; bin_idx++) {
            channel_response[bin_idx] = q_opt[bin_idx][m];
            //wtk_debug("%f %f\n",channel_response[bin_idx].a,channel_response[bin_idx].b);
        }

        int fft_size = (nbin_input - 1) * 2;
        wtk_drft_t *drft = wtk_drft_new2(fft_size);
        float *response = (float*)wtk_malloc(sizeof(float) * fft_size);
        wtk_drft_ifft22(drft, channel_response, response);
        int max_abs_idx = 0;
        float max_abs_val = 0.0f;
        for (int i = 0; i < fft_size; i++) {
            response[i] /= fft_size;
            float abs_val = fabs(response[i]);
            if (abs_val > max_abs_val) {
                max_abs_val = abs_val;
                max_abs_idx = i;
            }
        }
        // 截断脉冲响应
        int start_idx = max_abs_idx - N_TAP / 2;
        int end_idx = start_idx + N_TAP;
        //wtk_debug("%d %f\n",max_abs_idx,max_abs_val);
        //wtk_debug("%d\n",start_idx);
        if (start_idx < 0){
            start_idx = abs(start_idx);
            memcpy( (*truncated_ir) + m * N_TAP, response + fft_size - start_idx , sizeof(float) * start_idx);
            memcpy((*truncated_ir) + m * N_TAP + start_idx, response, sizeof(float) * end_idx);
        }else if (start_idx + N_TAP > fft_size){
            int idx = fft_size - start_idx;
            int cnt = end_idx - fft_size;
            memcpy( (*truncated_ir) + m * N_TAP, response + start_idx , sizeof(float) * idx);
            memcpy((*truncated_ir) + m * N_TAP + idx, response, sizeof(float)*cnt);
        }else{
            memcpy((*truncated_ir) + m * N_TAP, response + start_idx, sizeof(float) * N_TAP);
        }

        wtk_free(channel_response);
        wtk_free(response);
        wtk_drft_delete2(drft);
    }
    wtk_free(lambda_vec);
    wtk_free(freq_bins);
    wtk_complex_delete_p2_2(q_opt);
    wtk_complex_delete_p2_2(P_des);
}

// 计算Kaiser窗的beta参数
static float _calculate_kaiser_beta(float attenuation_db) {
    return 8.0f;
}

// 零阶修正贝塞尔函数
static float _i0(float x) {
    float sum = 1.0f;
    float term = 1.0f;
    float x_sq = x * x / 4.0f;

    for (int n = 1; n <= 20; n++) {
        term *= x_sq / (n * n);
        sum += term;
        if (term < sum * 1e-10f) break;
    }
    return sum;
}

// 生成Kaiser窗
static float* _kaiser_window(int N, float beta) {
    float* window = (float*)malloc(N * sizeof(float));
    if (!window) return NULL;

    float denominator = _i0(beta);
    int center = (N - 1) / 2;

    for (int n = 0; n < N; n++) {
        float relative_pos = (float)(n - center) / center;
        float numerator = 1.0f - relative_pos * relative_pos;
        if (numerator < 0) numerator = 0;
        window[n] = _i0(beta * sqrtf(numerator)) / denominator;
    }

    return window;
}

// 理想低通滤波器脉冲响应
static float* _ideal_lowpass(int N, float normalized_cutoff) {
    float* h = (float*)malloc(N * sizeof(float));
    if (!h) return NULL;

    int center = (N - 1) / 2;
    float omega_c = PI * normalized_cutoff; // 归一化角频率

    for (int n = 0; n < N; n++) {
        int n_centered = n - center;
        if (n_centered == 0) {
            h[n] = normalized_cutoff;
        } else {
            h[n] = sinf(omega_c * n_centered) / (PI * n_centered);
        }
    }

    return h;
}

// 设计FIR分频器
void design_fir_splitter(float F, float sample_rate,
                        float** lowpass, float** highpass,
                        int* filter_length) {
    float nyquist = sample_rate / 2.0f;

    // 计算归一化截止频率
    float normalized_cutoff = F / nyquist;

    // 计算滤波器长度
    float attenuation_db = 80.0f;
    float delta = powf(10.0f, -attenuation_db / 20.0f);
    float A = -20.0f * log10f(delta);

    int N;
    if (A > 50) {
        N = (int)((A - 7.95f) / (2.285f * 2 * PI * 0.2f * normalized_cutoff) + 1);
    } else {
        N = (int)(0.9222f / (2 * PI * 0.2f * normalized_cutoff) + 1);
    }

    // 确保N为奇数，以获得对称的滤波器响应
    if (N % 2 == 0) {
        N += 1;
    }

    *filter_length = N;

    // 生成Kaiser窗
    float beta = _calculate_kaiser_beta(attenuation_db);
    float* window = _kaiser_window(N, beta);

    // 生成理想低通滤波器
    float* ideal_lp = _ideal_lowpass(N, normalized_cutoff);

    // 设计低通滤波器
    *lowpass = (float*)malloc(N * sizeof(float));
    for (int i = 0; i < N; i++) {
        (*lowpass)[i] = ideal_lp[i] * window[i];
        //wtk_debug("%.5g\n",(*lowpass)[i]);
    }

    // 设计高通滤波器 (通过频谱反转)
    *highpass = (float*)malloc(N * sizeof(float));
    for (int i = 0; i < N; i++) {
        (*highpass)[i] = -(*lowpass)[i];
    }
    (*highpass)[N/2] += 1.0f; // 添加单位脉冲

    wtk_free(window);
    wtk_free(ideal_lp);
}

qtk_soundfield_syntheis_t *qtk_soundfield_syntheis_new(qtk_soundfield_syntheis_cfg_t *cfg){
    qtk_soundfield_syntheis_t *sos = wtk_malloc(sizeof(qtk_soundfield_syntheis_t));
    sos->cfg = cfg;
    int i;
    sos->input = wtk_strbuf_new(1024,1);
    sos->output_buf = wtk_calloc(cfg->N * cfg->hop_size, sizeof(float));
    sos->output = (short **)wtk_malloc(sizeof(short*) * cfg->N);
    if(cfg->nbin > 0){
        sos->nbin = cfg->nbin;
    }else{
        sos->nbin = cfg->rt60 * cfg->fs;
    }
    sos->nbin = sos->nbin >> 4 << 4;
    sos->drft = wtk_drft_new2(sos->nbin);
    for(i = 0; i < cfg->N; i++){
        sos->output[i] = (short *)wtk_malloc(sizeof(short) * cfg->hop_size);
    }

    if(cfg->lp){
        sos->spliter = conv_new(cfg->hop_size, cfg->lp->len, 2);
        conv_reset_weight2(sos->spliter,cfg->lp->p,cfg->hp->p);
    }

    if(cfg->weight_buf){
        sos->weight = conv_load(cfg->weight_buf);
        sos->Z_b = NULL;
        sos->H_b = NULL;
        sos->rir_bufs = NULL;
        sos->fft_buf = NULL;
    }else{
        int hop = sos->nbin/2 + 1;
        sos->eval_idx = 0;
        sos->ref_idx = 0;
        sos->Z_b = wtk_complex_new_p2_2(hop,cfg->N * cfg->nref);
        sos->H_b = wtk_complex_new_p2_2(hop,cfg->N * cfg->neval);
        sos->rir_bufs = wtk_strbufs_new(cfg->N);
        sos->fft_buf = (wtk_complex_t*)wtk_malloc(sizeof(wtk_complex_t) * hop);
    }
    return sos;
}

void set_Hb(qtk_soundfield_syntheis_t *sos, float *H, float *H2){
    int i,j;
    int hop = sos->nbin/2 + 1;
    for(i = 0; i < hop; i++){
        for(j = 0; j < sos->cfg->N*sos->cfg->neval;j++,H++,H2++){
            sos->H_b[i][j].a = *H;
            sos->H_b[i][j].b = *H2;
        }
    }
}

void set_Zb(qtk_soundfield_syntheis_t *sos, float *H, float *H2){
    int i,j;
    int hop = sos->nbin/2 + 1;
    for(i = 0; i < hop; i++){
        for(j = 0; j < sos->cfg->N*sos->cfg->nref;j++,H++,H2++){
            sos->Z_b[i][j].a = *H;
            sos->Z_b[i][j].b = *H2;
        }
    }
}

void qtk_soundfield_syntheis_delete(qtk_soundfield_syntheis_t *sos){
    int i;
    wtk_strbuf_delete(sos->input);
    wtk_free(sos->output_buf);
    for(i = 0; i < sos->cfg->N; i++){
        wtk_free(sos->output[i]);
    }
    conv_delete(sos->weight);
    conv_delete(sos->spliter);
    wtk_drft_delete2( sos->drft);
    wtk_free(sos->output);
    if(sos->Z_b){
        wtk_complex_delete_p2_2(sos->Z_b);
        wtk_complex_delete_p2_2(sos->H_b);
        wtk_strbufs_delete(sos->rir_bufs,sos->cfg->N);
        wtk_free(sos->fft_buf);
    }
    wtk_free(sos);
}

void qtk_soundfield_syntheis_reset(qtk_soundfield_syntheis_t *sos){

}

static void process_frame_(qtk_soundfield_syntheis_t *sos){
    float *data = (float*)sos->input->data,*frame_lowpass,*frame_highpass;
    int i,j;
    if(sos->cfg->bandsplit_on){
        //print_float(data,256);
        conv_process(sos->spliter,data);
        frame_lowpass = sos->spliter->output[0];
        frame_highpass = sos->spliter->output[1];
        //print_float(frame_highpass,256);
        //wtk_debug("%d\n",sos->spliter->L1);
        conv_process(sos->weight,frame_lowpass);
        for(i = 0; i < sos->weight->n; i++){
            for(j = 0; j < sos->weight->L1; j++){
                sos->weight->output[i][j] = (sos->weight->output[i][j] + frame_highpass[j]) * sos->cfg->scale;
                if(sos->weight->output[i][j] > 1){
                    sos->weight->output[i][j] = 0.9999;
                } else if (sos->weight->output[i][j] <= -1.0) {
                    sos->weight->output[i][j] = -1.0;
                }
                sos->output[i][j] = sos->weight->output[i][j] * 32768.0;
            }
        }
    }else{
        conv_process(sos->weight, data);
        for(i = 0; i < sos->weight->n; i++){
            for(j = 0; j < sos->weight->L1; j++){
                sos->weight->output[i][j] *= sos->cfg->scale;
                if(sos->weight->output[i][j] > 1){
                    sos->weight->output[i][j] = 0.9999;
                } else if (sos->weight->output[i][j] <= -1.0) {
                    sos->weight->output[i][j] = -1.0;
                }
                sos->output[i][j] = sos->weight->output[i][j] * 32768.0;
            }
        }
    }
}

void qtk_soundfield_syntheis_feed(qtk_soundfield_syntheis_t *sos, short *data, int len){
    wtk_strbuf_t *input = sos->input;
    int i;
    float fv;
    for(i = 0;i < len;++i)
    {
        fv = data[i]/32768.0;
        wtk_strbuf_push(input,(char *)(&fv),sizeof(float));
    }

    int wav_len = input->pos/sizeof(float);
    int fsize = sos->cfg->hop_size;
    while(wav_len > fsize){
        process_frame_(sos);

        if(sos->notify){
            sos->notify(sos->upval, sos->output, sos->cfg->hop_size);
        }

        wtk_strbuf_pop(input, NULL, sos->cfg->hop_size * sizeof(float));
        wav_len = input->pos/sizeof(float);
    }
}

void qtk_soundfield_syntheis_set_notify(qtk_soundfield_syntheis_t *sos, void *upval, qtk_soundfield_syntheis_notify_f notify){
    sos->upval = upval;
    sos->notify = notify;
}

void qtk_soundfield_syntheis_calc_weight(qtk_soundfield_syntheis_t *sos){
    float *truncated_ir;
    caculate_driving_func_weight(sos, sos->cfg->fs, sos->H_b, sos->Z_b, &truncated_ir);
    float **weights = sos->weights;
    for (int i = 0; i < sos->nbin / 2 + 1; i++) {
        wtk_free(weights[i]);
    }
    wtk_free(weights);

    sos->weight = conv_new(sos->cfg->hop_size, sos->cfg->n_tap, sos->cfg->N);
    conv_reset_weight(sos->weight, truncated_ir);
    wtk_free(truncated_ir);

    design_fir_splitter(sos->cfg->cutoff_freq, sos->cfg->fs,
                       &sos->taps_lowpass, &sos->taps_highpass,
                       &sos->filter_length);

    sos->spliter = conv_new(sos->cfg->hop_size, sos->filter_length, 2);
    conv_reset_weight2(sos->spliter, sos->taps_lowpass, sos->taps_highpass);
    wtk_free(sos->taps_highpass);
    wtk_free(sos->taps_lowpass);
}

void qtk_soundfield_syntheis_feed_rir(qtk_soundfield_syntheis_t *sos, short **rirs, int len, int eval){
    int i,j;
    float pv;

    if(len < sos->nbin){
        return;
    }

    wtk_strbufs_reset(sos->rir_bufs, sos->cfg->N);
    for(i = 0; i < sos->cfg->N; i++){
        for(j = 0; j < sos->nbin; j++){
            pv = rirs[i][j]/32768.0f;
            wtk_strbuf_push_float(sos->rir_bufs[i], &pv, 1);
        }
    }

    if(eval){
        for(i = 0; i < sos->cfg->N; i++){
            wtk_drft_fft22(sos->drft, (float*)sos->rir_bufs[i]->data,sos->fft_buf);
            for(j = 0; j < sos->nbin/2 + 1;j++){
                sos->H_b[j][sos->cfg->N * sos->eval_idx + i].a = sos->fft_buf[j].a * sos->nbin;
                sos->H_b[j][sos->cfg->N * sos->eval_idx + i].b = sos->fft_buf[j].b * sos->nbin;
            }
        }
        sos->eval_idx++;
    }else{
        for(i = 0; i < sos->cfg->N; i++){
            wtk_drft_fft22(sos->drft, (float*)sos->rir_bufs[i]->data,sos->fft_buf);
            for(j = 0; j < sos->nbin/2 + 1;j++){
                sos->Z_b[j][sos->cfg->N * sos->ref_idx + i].a = sos->fft_buf[j].a * sos->nbin;
                sos->Z_b[j][sos->cfg->N * sos->ref_idx + i].b = sos->fft_buf[j].b * sos->nbin;
            }
        }
        sos->ref_idx++;
    }
}