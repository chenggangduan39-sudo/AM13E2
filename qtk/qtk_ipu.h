#ifdef IPU_DEC
#ifndef F7288C73_E87F_4197_9820_C2383C8474E7
#define F7288C73_E87F_4197_9820_C2383C8474E7
#ifdef __cplusplus
extern "C" {
#endif
#include "mi_common_datatype.h"
#include "mi_ipu.h"
#include "mi_sys.h"
#include "mi_sys_datatype.h"
#include "wtk/core/wtk_alloc.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#define IPU_NUM 17

typedef struct qtk_ipu {
    MI_IPUChnAttr_t stChnAttr;
    MI_IPU_OfflineModelStaticInfo_t OfflineModelInfo;
    MI_U32 u32ChannelID;
    MI_IPU_SubNet_InputOutputDesc_t desc;
    MI_IPU_TensorVector_t InputTensorVector;
    MI_IPU_TensorVector_t OutputTensorVector;
    float *output[IPU_NUM];
    float *input[IPU_NUM];
} qtk_ipu_t;

//创建IPU设备 ipu_firmware:IPU文件路径 acc:模型路径数组  t:模型路径数组的个数
void qtk_ipu_device_new(char *ipu_firmware, char *argv[], int t);

//创建IPU通道 model:模型路径
qtk_ipu_t *qtk_ipu_channel_new(char *model);

//销毁IPU通道
void qtk_ipu_channel_delete(qtk_ipu_t *ipu);

//销毁IPU设备
void qtk_ipu_device_delete();

//单输入输出推理 data:图像数据
float *qtk_ipu_single_invoke(qtk_ipu_t *ipu, uint8_t *data);

//多输入输出推理  data:图像数据
void qtk_ipu_multiple_invoke(qtk_ipu_t *ipu, uint8_t *data);

#ifdef __cplusplus
};
#endif
#endif /* F7288C73_E87F_4197_9820_C2383C8474E7 */
#endif
