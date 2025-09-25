#include "qtk_ipu.h"
#include <bits/stdint-uintn.h>
#include <string.h>

int qtk_ipu_get_model_max_varible_size(char *model_path) {
    MI_U32 max_size = 0;
    MI_IPU_OfflineModelStaticInfo_t model_info;
    MI_S32 ret = MI_IPU_GetOfflineModeStaticInfo(NULL, model_path, &model_info);
    if (ret != MI_SUCCESS) {
        return -1;
    }
    max_size = model_info.u32VariableBufferSize;
    return max_size;
}
static int qtk_ipu_create_device(char *firmware_path, MI_U32 u32VarBufSize) {
    MI_IPU_DevAttr_t stDevAttr;
    stDevAttr.u32MaxVariableBufSize = u32VarBufSize;
    stDevAttr.u32YUV420_W_Pitch_Alignment = 16;
    stDevAttr.u32YUV420_H_Pitch_Alignment = 2;
    stDevAttr.u32XRGB_W_Pitch_Alignment = 16;
    MI_S32 ret = MI_IPU_CreateDevice(&stDevAttr, NULL, firmware_path, 0);
    if (ret != MI_SUCCESS) {
        wtk_debug("MI_IPU_CreateDevice failed: %d\n", ret);
        return -1;
    }
    return 0;
}

void qtk_ipu_device_new(char *firmware_path, char *argv[], int t) {
    int ret;
    MI_U32 varbufsize = 0;
    MI_U32 size;
    MI_SYS_Init(0);
    for (int i = 0; i < t; i++) {
        if (argv[i] != NULL){
            size = qtk_ipu_get_model_max_varible_size(argv[i]);
        }
        else
            continue;
        if (size < 0) {
            wtk_debug("--> get max varible size failed.\n");
            exit(1);
        }
        if (size > varbufsize)
            varbufsize = size;
    }
    ret = qtk_ipu_create_device(firmware_path, varbufsize);
    if (ret != 0) {
        wtk_debug("create ipu device failed.\n");
        exit(1);
    }
}

void qtk_ipu_channel_delete(qtk_ipu_t *ipu) {
    for (int i = 0; i < ipu->desc.u32InputTensorCount; i++) {
        if (ipu->input[i] != NULL)
            wtk_free(ipu->input[i]);
    }

    for (int i = 0; i < ipu->desc.u32OutputTensorCount; i++) {
        if (ipu->output[i] != NULL)
            wtk_free(ipu->output[i]);
    }
    MI_IPU_PutInputTensors(ipu->u32ChannelID, &ipu->InputTensorVector);
    MI_IPU_PutOutputTensors(ipu->u32ChannelID, &ipu->OutputTensorVector);
    MI_IPU_DestroyCHN(ipu->u32ChannelID);
    MI_SYS_Exit(0);
    wtk_free(ipu);
    ipu = NULL;
}

char* GetTensorTypeName(MI_IPU_ELEMENT_FORMAT eIPUFormat) 
{
    switch (eIPUFormat) {
        case MI_IPU_FORMAT_U8:
            return "UINT8";
        case MI_IPU_FORMAT_NV12:
            return "YUV_NV12";
        case MI_IPU_FORMAT_INT16:
            return "INT16";
        case MI_IPU_FORMAT_INT32:
            return "INT32";
        case MI_IPU_FORMAT_INT8:
            return "INT8";
        case MI_IPU_FORMAT_FP32:
            return "FLOAT32";
        case MI_IPU_FORMAT_UNKNOWN:
            return "UNKNOWN";
        case MI_IPU_FORMAT_ARGB8888:
            return "BGRA";
        case MI_IPU_FORMAT_ABGR8888:
            return "RGBA";
        case MI_IPU_FORMAT_GRAY:
            return "GRAY";
    }
    return "NOTYPE";
}
static void Show_Model_Info(MI_IPU_SubNet_InputOutputDesc_t desc) 
{
    for (MI_U32 idx = 0; idx < desc.u32InputTensorCount; idx++) {
        printf("Input(%d):\n",idx);
        printf("    name:\t%s\n", desc.astMI_InputTensorDescs[idx].name);
        printf("    dtype:\t%s\n",GetTensorTypeName(desc.astMI_InputTensorDescs[idx].eElmFormat));
        printf("    shape:\t[");
        for (MI_U32 jdx = 0; jdx < desc.astMI_InputTensorDescs[idx].u32TensorDim; jdx++) {
            printf("%d",desc.astMI_InputTensorDescs[idx].u32TensorShape[jdx]);
            if (jdx < desc.astMI_InputTensorDescs[idx].u32TensorDim - 1) {
                printf(", ");
            }
        }
        printf("]\n");
        printf("    size:\t%d\n",desc.astMI_InputTensorDescs[idx].s32AlignedBufSize);
        // if (desc.astMI_InputTensorDescs[idx].eLayoutType == E_IPU_LAYOUT_TYPE_NCHW) {
        //     printf("    layout:\t" << "NCHW" << std::endl;
        // }
        // if (desc.astMI_InputTensorDescs[idx].eElmFormat == MI_IPU_FORMAT_INT16) {
        //     std::cout << "    quantization:\t(" << desc.astMI_InputTensorDescs[idx].fScalar << ", ";
        //     std::cout << desc.astMI_InputTensorDescs[idx].s64ZeroPoint << ")" << std::endl;
        // }
    }
    for (MI_U32 idx = 0; idx < desc.u32OutputTensorCount; idx++) {
        printf("Output(%d):\n");
        printf("    name:\t%s\n",desc.astMI_OutputTensorDescs[idx].name);
        printf("    dtype:\t%s\n" ,GetTensorTypeName(desc.astMI_OutputTensorDescs[idx].eElmFormat));
        printf("    shape:\t[");
        for (MI_U32 jdx = 0; jdx < desc.astMI_OutputTensorDescs[idx].u32TensorDim; jdx++) {
            printf("%d",desc.astMI_OutputTensorDescs[idx].u32TensorShape[jdx]);
            if (jdx < desc.astMI_OutputTensorDescs[idx].u32TensorDim - 1) {
                printf(", ");
            }
        }
        printf("]");
        printf("    size:\t%d\n",desc.astMI_OutputTensorDescs[idx].s32AlignedBufSize);
        // if (desc.astMI_OutputTensorDescs[idx].eLayoutType == E_IPU_LAYOUT_TYPE_NCHW) {
        //     std::cout << "    layout:\t" << "NCHW" << std::endl;
        // }
        // if (desc.astMI_OutputTensorDescs[idx].eElmFormat == MI_IPU_FORMAT_INT16) {
        //     std::cout << "    quantization:\t(" << desc.astMI_OutputTensorDescs[idx].fScalar << ", ";
        //     std::cout << desc.astMI_OutputTensorDescs[idx].s64ZeroPoint << ")" << std::endl;
        // }
    }

}
qtk_ipu_t *qtk_ipu_channel_new(char *model) {
    qtk_ipu_t *ipu = (qtk_ipu_t *)wtk_malloc(sizeof(qtk_ipu_t));
    int ret;
    memset(ipu, 0, sizeof(qtk_ipu_t));
    ipu->stChnAttr.u32InputBufDepth = 1;
    ipu->stChnAttr.u32OutputBufDepth = 1;
    ret = MI_IPU_CreateCHN(&ipu->u32ChannelID, &ipu->stChnAttr, NULL, model);
    if (ret != MI_SUCCESS) {
        wtk_debug("MI_IPU_CreateCHN failed: %d\n", ret);
        qtk_ipu_channel_delete(ipu);
        ipu = NULL;
        return NULL;
    }
    ret = MI_IPU_GetInOutTensorDesc(ipu->u32ChannelID, &ipu->desc);
    if (ret != MI_SUCCESS) {
        wtk_debug("MI_IPU_GetInOutTensorDesc failed: %d\n", ret);
        qtk_ipu_channel_delete(ipu);
        ipu = NULL;
        return NULL;
    }
    ret = MI_IPU_GetInputTensors(ipu->u32ChannelID, &ipu->InputTensorVector);
    if (ret != MI_SUCCESS) {
        wtk_debug("MI_IPU_GetInputTensors failed: %d\n", ret);
        qtk_ipu_channel_delete(ipu);
        ipu = NULL;
        return NULL;
    }
    ret = MI_IPU_GetOutputTensors(ipu->u32ChannelID, &ipu->OutputTensorVector);
    if (ret != MI_SUCCESS) {
        wtk_debug("MI_IPU_GetOutputTensors failed: %d\n", ret);
        qtk_ipu_channel_delete(ipu);
        ipu = NULL;
        return NULL;
    }
    Show_Model_Info(ipu->desc);
    for (int i = 0; i < ipu->desc.u32InputTensorCount; i++) {
        ipu->input[i] = (float *)wtk_malloc(
            ipu->desc.astMI_InputTensorDescs[i].s32AlignedBufSize *
            sizeof(float));
        printf("inbuf %d %d\n",ipu->desc.astMI_InputTensorDescs[i].s32AlignedBufSize,ipu->desc.astMI_InputTensorDescs[i].u32BufSize);
        memset(ipu->input[i], 0,
               ipu->desc.astMI_InputTensorDescs[i].s32AlignedBufSize *
                   sizeof(float));
    }
    for (int i = 0; i < ipu->desc.u32OutputTensorCount; i++) {
        ipu->output[i] = (float *)wtk_malloc(
            ipu->desc.astMI_OutputTensorDescs[i].s32AlignedBufSize *
            sizeof(float));
        printf("outbuf %d\n",ipu->desc.astMI_OutputTensorDescs[i].s32AlignedBufSize);
        memset(ipu->output[i], 0,
               ipu->desc.astMI_OutputTensorDescs[i].s32AlignedBufSize *
                   sizeof(float));
    }
    return ipu;
}

void qtk_ipu_device_delete() { MI_IPU_DestroyDevice(); }

#ifdef USE_NEON
void __attribute__((noinline)) neon_memcpy(void *dest, const void *src, int n) {
    if (n & 63) {
        n = (n & -64) + 64;
    }
    asm("NEONCopyPLD11:\n"
        "   pld [r1, #0xC0]\n" //预取数据
        "   vldm r1!,{d0-d7}\n" //从参数一r0（src）加载8*8=64个单通道8位数据
        "   vstm r0!,{d0-d7}\n" //存储在目的地址r1（dst）中，同样是64个8位单通道8位数据
        "   subs r2,r2,#0x40\n" //循环跳转参数，每次减64，总共循环次数=row*col*4/64
        "   bgt NEONCopyPLD11\n" //以前这里是bge，有问题。现在改成bgt。
    );
}
#endif

float *qtk_ipu_single_invoke(qtk_ipu_t *ipu, uint8_t *data) {
#ifdef USE_NEON
    neon_memcpy(ipu->InputTensorVector.astArrayTensors[0].ptTensorData[0], data,
                ipu->desc.astMI_InputTensorDescs[0].s32AlignedBufSize);
#else
    memcpy(ipu->InputTensorVector.astArrayTensors[0].ptTensorData[0], data,
           ipu->desc.astMI_InputTensorDescs[0].s32AlignedBufSize);
#endif
    if (ipu->desc.astMI_InputTensorDescs[0].eElmFormat != MI_IPU_FORMAT_FP32)
        MI_SYS_FlushInvCache(
            ipu->InputTensorVector.astArrayTensors[0].ptTensorData[0],
            ipu->desc.astMI_InputTensorDescs[0].s32AlignedBufSize);

    if (MI_SUCCESS != MI_IPU_Invoke(ipu->u32ChannelID, &ipu->InputTensorVector,
                                    &ipu->OutputTensorVector)) {
        wtk_debug("<%d> error\n", __LINE__);
        return NULL;
    }

    return ipu->OutputTensorVector.astArrayTensors[0].ptTensorData[0];
}

void qtk_ipu_multiple_invoke(qtk_ipu_t *ipu, uint8_t *data) {
    for (int idx = 1; idx < ipu->desc.u32InputTensorCount; idx++) {
        memcpy(ipu->InputTensorVector.astArrayTensors[idx].ptTensorData[0],
               ipu->input[idx],
               ipu->desc.astMI_InputTensorDescs[idx].s32AlignedBufSize);
        if (ipu->desc.astMI_InputTensorDescs[idx].eElmFormat !=
            MI_IPU_FORMAT_FP32)
            MI_SYS_FlushInvCache(
                ipu->InputTensorVector.astArrayTensors[idx].ptTensorData[0],
                ipu->desc.astMI_InputTensorDescs[idx].s32AlignedBufSize);
    }
    memcpy(ipu->InputTensorVector.astArrayTensors[0].ptTensorData[0], data,
           ipu->desc.astMI_InputTensorDescs[0].s32AlignedBufSize);
    if (ipu->desc.astMI_InputTensorDescs[0].eElmFormat != MI_IPU_FORMAT_FP32)
        MI_SYS_FlushInvCache(
            ipu->InputTensorVector.astArrayTensors[0].ptTensorData[0],
            ipu->desc.astMI_InputTensorDescs[0].s32AlignedBufSize);
    if (MI_SUCCESS != MI_IPU_Invoke(ipu->u32ChannelID, &ipu->InputTensorVector,
                                    &ipu->OutputTensorVector)) {
        wtk_debug("<%d> error\n", __LINE__);
        return;
    }
    for (int idx = 1; idx < ipu->desc.u32OutputTensorCount; idx++) {
        memcpy(ipu->output[idx],
               ipu->OutputTensorVector.astArrayTensors[idx].ptTensorData[0],
               ipu->desc.astMI_OutputTensorDescs[idx].s32AlignedBufSize);
        memcpy(ipu->input[idx], ipu->output[idx],
               ipu->desc.astMI_OutputTensorDescs[idx].s32AlignedBufSize);
    }
    memcpy(ipu->output[0],ipu->OutputTensorVector.astArrayTensors[0].ptTensorData[0],
               ipu->desc.astMI_OutputTensorDescs[0].s32AlignedBufSize);
}
