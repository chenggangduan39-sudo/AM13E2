#include "wtk_nn_pad.h"

/* 
参考 np.pad
 */
void wtk_nn_pad_float( wtk_nn_pad_type_enum_t pad_type, float *src, int slen, float *dst, int dlen, int pad_len)
{
    int i, j;
    float
        *psrc_end=src+slen-1,
        *pdst_end=dst+dlen-1;

    assert( dlen == slen+pad_len*2);
    memcpy( dst+pad_len, src, sizeof(float)*slen);
    switch (pad_type)
    {
    case wtk_nn_pad_type_reflect:
        /* 围绕首末点各自对称
        arr1D=[5, 1, 2, 6, 3, 4]
        str(np.pad(arr1D, (2, 3), 'reflect'))=[2, 1, 5, 1, 2, 6, 3, 4, 3, 6, 2]
         */
        for (i=pad_len, j=0; i>0; --i, ++j)
        {
            dst[j]=src[i];
            pdst_end[-j]=psrc_end[-i];
        }
        break;
    case wtk_nn_pad_type_center:
        /* 末尾取值也是从0开始倒取,所以需要加1 */
        memset( dst, 0, pad_len*sizeof(float));
        memset( pdst_end-pad_len+1, 0, pad_len*sizeof(float));
        break;
    default:
        wtk_debug("未实现的填充类型: [%d] \n", pad_type);
        wtk_exit(1);
        break;
    }
}
