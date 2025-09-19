/*************************************************************************************************
*	Copyright (c) 2018, zhc
*	All rights reserved.
*
*	文件名称: zhc_uac_interface.h
*	摘    要: 
*	
*	当前版本: 1.0
*	作    者: 
*	完成日期: 
*
**************************************************************************************************/

#ifndef __CALLBACK_H__
#define __CALLBACK_H__
#ifdef __cplusplus
extern "C" {
#endif


//bit
#define BIT_PARAMETER_S16		2
#define BIT_PARAMETER_S24		3
#define BIT_PARAMETER_S32		4

//ch  相应数据位对应几通道
#define CHANNEL_PARAMETER_1		0x01
#define CHANNEL_PARAMETER_2		0x03
#define CHANNEL_PARAMETER_3		0x07
#define CHANNEL_PARAMETER_4		0x0f


//正常返回0 
int uac_init(void);


//参数1，采样率（16k，32k，48k）。
//参数2，位宽。
//参数3通道数
int uac_ctl(unsigned int rate,int bit,int ch);

#ifdef __cplusplus
}
#endif
#endif	// __CALLBACK_H__
