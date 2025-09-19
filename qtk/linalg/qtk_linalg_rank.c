#include "qtk_linalg_rank.h"
#include <stdio.h>
//row 行   col列
int qtk_linalg_rank(float *matrix, const int row, const int col,
                    float *echelon_matrix, int *total, float *original_matrix) {
    int i,j,none_zero = 0,result = 0;
    
    for(i = 0;i < row;i ++)
        for(j = 0;j < col;j ++)
            echelon_matrix[i * col + j] = qtk_linalg_standard_echelon(
                matrix, row, col, i, j, total, original_matrix);

    for(i = 0;i < row;i ++)
    {
        for(j = 0;j < col;j ++)
            if(echelon_matrix[i*col+j] != 0)
            {
                none_zero = 1;
                break;
            }
        if(none_zero == 1)
            result ++;
        none_zero = 0;
    }
    return result;
}

float qtk_linalg_standard_echelon(float *matrix, const int row, const int col,
                                  int x, int y, int *total,
                                  float *original_matrix) {
    int i,j,k,l;
    float times,temp,result = 0;
    
    for(i = 0;i < row;i ++)
        for(j = 0;j < col;j ++)
            original_matrix[i*col+j] = matrix[i*col+j];
    for(i = 0;i < row - 1;i ++)
        for(k = i + 1;k < row ; k ++)
        {
            j = 0;
            while(matrix[i*col+j] == 0)
                j ++;
            if(matrix[i*col+j] != 0)
            {
                times = matrix[k*col+j] / matrix[i*col+j];
                for(j = 0;j < col;j ++)
                    matrix[k*col+j] -= matrix[i*col+j] * times;
            }
        }
    for(i = 0;i < row;i ++)
    {
        j = 0;
        while(matrix[i*col+j] == 0){
            j++;
            if (j==col-1){
                break;
            }
        }
        if(matrix[i*col+j] != 0)
        {
            times = matrix[i*col+j];
            for(j = 0;j < col;j ++)
                matrix[i*col+j] /= times;
        }
    }
    for(i = 0;i < row;i ++)
        for(j = 0;j < col;j ++)
            if(matrix[i*col+j] == 0)
                total[i] ++;
            else
                break;
    for(l = row - 1;l > 0;l --)
        for(i = 0;i < l;i ++)
            if(total[l] < total[i])
                for(j = 0;j < col;j ++)
                {
                    temp = matrix[l*col+j];
                    matrix[l*col+j] = matrix[i*col+j];
                    matrix[i*col+j] = temp;
                }
    for(i = 0;i < row;i ++)
    {
        j = 0;
        while (matrix[i * col + j] == 0) {
            j ++;
            if (j == row - 1)
                break;
        }

        if(matrix[i*col+j] != 0)
            for(k = 0;k < i;k ++)
            {
                times = matrix[k*col+j] / matrix[i*col+j];
                for(l = 0;l < col;l ++)
                    matrix[k*col+l] -= times * matrix[i*col+l];
            }
    }
    result = matrix[x*col+y];
    for(i = 0;i < row;i ++)
        for(j = 0;j < col;j ++)
            matrix[i*col+j] = original_matrix[i*col+j];
    if (fabs(result) <= 1e-6)
        result = 0;
    return result;
}
