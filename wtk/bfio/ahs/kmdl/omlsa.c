#include "omlsa.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

//# 定义expintpow_solution函数
float expintpow_solution(float v_subscript) {
  int vec = (int)(v_subscript * 100);
  if (vec < 1)
    vec = 1;
  else if (vec > 1500)
    vec = 1500;
  return (float)int_value[vec - 1];
}

//# 定义subexp_solution函数
float subexp_solution(float v_subscript) {
  int vec = (int)(v_subscript * 100);
  if (vec < 1)
    vec = 1;
  else if (vec > 1500)
    vec = 1500;
  return (float)expsub_value[vec - 1];
}

// 初始化compute_GH1
state_GH1 *compute_GH1_init(int frame_length) {
  state_GH1 *gh = (state_GH1 *)malloc(sizeof(state_GH1));
  gh->N_eff = frame_length / 2 + 1;
  gh->pr_snr = (float *)calloc(gh->N_eff, sizeof(float));
  for (int i = 0; i < gh->N_eff; i++) {
    gh->pr_snr[i] = 0;
  }

  gh->P_MIN = 0.0001;
  gh->pr_snr_parm = 0.95;

  gh->post_snr_tmp = (float *)calloc(gh->N_eff, sizeof(float));
  for (int i = 0; i < gh->N_eff; i++) {
    gh->post_snr_tmp[i] = 0;
  }

  gh->gh1 = (float *)calloc(gh->N_eff, sizeof(float));
  for (int i = 0; i < gh->N_eff; i++) {
    gh->gh1[i] = 0;
  }
  return gh;
}

// 计算GH1
void compute_GH1_call(state_GH1 *gh, const float *noise_est,
                      const float *amp_pr, float **gh1_out,
                      float **current_snr_out, float **v_int_out) {
  float *post_snr = (float *)malloc(gh->N_eff * sizeof(float));
  float *post_temp = (float *)malloc(gh->N_eff * sizeof(float));
  float *current_snr = (float *)malloc(gh->N_eff * sizeof(float));
  float *v_int = (float *)malloc(gh->N_eff * sizeof(float));
  float *integra = (float *)malloc(gh->N_eff * sizeof(float));

  // 计算post_snr
  for (int i = 0; i < gh->N_eff; i++) {
    float denom = noise_est[i] + 1e-7;
    post_snr[i] = pow(amp_pr[i] / denom, 2);
    if (post_snr[i] > 10000)
      post_snr[i] = 10000;
  }

  // 计算post_temp
  for (int i = 0; i < gh->N_eff; i++) {
    post_temp[i] = (post_snr[i] - 1 < 0) ? 0 : post_snr[i] - 1;
  }

  // 更新pr_snr
  for (int i = 0; i < gh->N_eff; i++) {
    gh->pr_snr[i] = pow(gh->gh1[i], 2) * gh->post_snr_tmp[i];
  }

  // 保存当前post_snr
  for (int i = 0; i < gh->N_eff; i++) {
    gh->post_snr_tmp[i] = post_snr[i];
  }

  // 计算current_snr
  for (int i = 0; i < gh->N_eff; i++) {
    current_snr[i] =
        gh->pr_snr_parm * gh->pr_snr[i] + (1 - gh->pr_snr_parm) * post_temp[i];
    if (current_snr[i] < gh->P_MIN)
      current_snr[i] = gh->P_MIN;
    if (current_snr[i] > 10000)
      current_snr[i] = 10000;
  }

  // 计算v_int
  for (int i = 0; i < gh->N_eff; i++) {
    v_int[i] = (current_snr[i] * post_snr[i]) / (1 + current_snr[i]);
    if (v_int[i] > 10000)
      v_int[i] = 10000;
    else if (v_int[i] < 0.01)
      v_int[i] = 0.01;
  }

  // 计算integra
  for (int i = 0; i < gh->N_eff; i++) {
    integra[i] = expintpow_solution(v_int[i]);
  }

  // 更新gh1
  for (int i = 0; i < gh->N_eff; i++) {
    gh->gh1[i] = current_snr[i] * (integra[i] / 16384) / (1 + current_snr[i]);
    if (gh->gh1[i] > 8)
      gh->gh1[i] = 8;
  }

  // 设置输出
  *gh1_out = gh->gh1;
  *current_snr_out = current_snr;
  *v_int_out = v_int;

  // 清理临时数组
  free(post_snr);
  free(post_temp);
  free(integra);
}

// 初始化omlsa_filtering
omlsa_filtering *omlsa_filtering_init(int frame_length) {
  omlsa_filtering *filt = (omlsa_filtering *)malloc(sizeof(omlsa_filtering));
  filt->N_eff = frame_length / 2 + 1;
  return filt;
}

// 计算滤波增益
float *omlsa_filtering_call(omlsa_filtering *filt, const float *v_int,
                            const float *q_est, const float *cur_snr,
                            const float *gh1) {
  float *integra = (float *)malloc(filt->N_eff * sizeof(float));
  float *arr_temp = (float *)malloc(filt->N_eff * sizeof(float));
  float *p_est = (float *)malloc(filt->N_eff * sizeof(float));
  float *g = (float *)malloc(filt->N_eff * sizeof(float));
  float NOISE_FACTOR = 0.001;

  // 计算integra
  for (int i = 0; i < filt->N_eff; i++) {
    integra[i] = subexp_solution(v_int[i]);
  }

  // 计算arr_temp
  for (int i = 0; i < filt->N_eff; i++) {
    float denom = 1 - q_est[i] + 1e-7;
    arr_temp[i] = 1 + ((1 + cur_snr[i]) * integra[i] * q_est[i]) / denom;
  }

  // 计算p_est
  for (int i = 0; i < filt->N_eff; i++) {
    p_est[i] = 1 / (arr_temp[i] + 1e-7);
    if (p_est[i] < 0.0001)
      p_est[i] = 0.0001;
    else if (p_est[i] > 1)
      p_est[i] = 1;
  }

  // 计算增益g
  for (int i = 0; i < filt->N_eff; i++) {
    float noise_fa = NOISE_FACTOR;
    g[i] = pow(gh1[i], p_est[i]) * pow(noise_fa, (1 - p_est[i]));
    if (g[i] > 1)
      g[i] = 1;
    else if (g[i] < 0)
      g[i] = 0;
  }

  free(integra);
  free(arr_temp);
  free(p_est);
  return g;
}

// 初始化OMLSA
OMLSA *OMLSA_init(int frame_length) {
  OMLSA *omlsa = (OMLSA *)malloc(sizeof(OMLSA));
  omlsa->gh1 = compute_GH1_init(frame_length);
  omlsa->filt = omlsa_filtering_init(frame_length);
  return omlsa;
}

// 执行OMLSA降噪
float *OMLSA_call(OMLSA *omlsa, const float *noise_est, const float *amp_pr,
                  const float *q_est) {
  float *gh1, *current_snr, *v_int;
  compute_GH1_call(omlsa->gh1, noise_est, amp_pr, &gh1, &current_snr, &v_int);

  float *g = omlsa_filtering_call(omlsa->filt, v_int, q_est, current_snr, gh1);

  free(current_snr);
  free(v_int);
  return g;
}

// 清理内存
void OMLSA_free(OMLSA *omlsa) {
  free(omlsa->gh1->pr_snr);
  free(omlsa->gh1->post_snr_tmp);
  free(omlsa->gh1->gh1);
  free(omlsa->gh1);
  free(omlsa->filt);
  free(omlsa);
}