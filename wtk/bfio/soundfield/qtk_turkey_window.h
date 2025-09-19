#include "wtk/core/math/wtk_math.h"

// 检查是否全活动
int is_all_active(int* active, int n) {
    for (int i = 0; i < n; i++)
        if (!active[i]) return 0;
    return 1;
}

// 查找起始索引
int find_first_index(int* active, int n) {
    if ((active[0] && !active[n-1]) || is_all_active(active, n)) 
        return 0;
    
    for (int i = 0; i < n-1; i++) 
        if (!active[i] && active[i+1]) 
            return i+1;
    
    return 0;  // 默认值
}

void tukey(int* active, int n, float alpha, float *result) {
    memset(result, 0, sizeof(float) * n);
    // 计算活动点数
    int m = 0;
    for (int i = 0; i < n; i++) 
        if (active[i]) m++;
    
    // 获取连续活动块的起始索引
    int first_idx = find_first_index(active, n);
    
    // 创建活动索引映射
    int* idx_map = (int*)wtk_malloc(m * sizeof(int));
    for (int i = 0; i < m; i++) 
        idx_map[i] = (first_idx + i) % n;
    
    // 计算Tukey窗（长度m+2）
    float* tukey_win = (float*)wtk_malloc((m+2) * sizeof(float));
    for (int i = 0; i < m+2; i++) {
        float x = (float)i / (m+1);
        if (x < alpha/2) {
            tukey_win[i] = 0.5 * (1 + cos(2*M_PI/alpha * (x - alpha/2)));
        } else if (x >= 1 - alpha/2) {
            tukey_win[i] = 0.5 * (1 + cos(2*M_PI/alpha * (x - 1 + alpha/2)));
        } else {
            tukey_win[i] = 1.0;
        }
    }
    
    // 填充结果数组
    for (int i = 0; i < m; i++) 
        result[idx_map[i]] = tukey_win[i+1];  // 取[1:-1]
    
    // 释放临时内存
    free(idx_map);
    free(tukey_win);
}