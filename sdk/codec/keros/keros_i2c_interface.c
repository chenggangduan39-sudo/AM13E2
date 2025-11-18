/*****************************************************************************
 * KEROS I2C Bus Driver
 *
 *
 * Copyright(C) CHIPSBRAIN GLOBAL CO., Ltd.
 * All rights reserved.
 *
 * File Name    : keros_i2c_interface.c
 * Author       : ARES HA
 *
 * Version      : V0.3
 * Date         : 2015.09.08
 * Description  : Keros I2C Bus Driver
 ****************************************************************************/
#define _SOURCE_I2CBUS_

/* USER INCLUDE BEGIN */

//#include "stm32l0xx_hal.h"


/* USER INCLUDE END  */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include "i2c-dev.h"

#include "keros_lib.h"
#include "keros_i2c_interface.h"

int qtk_i2c_index;

//void  OSTimeDly (__IO INT16U ticks);

//static uint8_t  	cBusEnable;
//static uint16_t  	iStretchTime;


//*************************************linux application interface*****************************************************

/*****************************************************************************
  ��ʱ������ms��
******************************************************************************/
void delay_ms(unsigned int i)  
{
    usleep(1000 * i);
}

/*****************************************************************************
  i2c������������1�����豸��ַ������2���Ĵ�����ַ������3����ȡ���ݻ�����������4����ȡ���ݴ�С
******************************************************************************/
unsigned char _i2c_read(unsigned char device_addr, unsigned char *sub_addr, unsigned char *buff, int ByteNo)
{
    int fd, ret;
    unsigned char buftmp[32];
    struct i2c_rdwr_ioctl_data i2c_data;
    // const char      *i2c_dev = "/dev/i2c-1";
    char i2c_dev[32]={0};
    snprintf(i2c_dev, sizeof(i2c_dev), "/dev/i2c-%d", qtk_i2c_index);
    //----------------------------------

    device_addr >>= 1;
    //init
    fd = open(i2c_dev, O_RDWR);
    if (fd<0)
    {
        printf("not have /dev/i2c-1 t\r\n");
        return -1;
    }

    i2c_data.nmsgs = 2;
    i2c_data.msgs = (struct i2c_msg *)malloc(i2c_data.nmsgs *sizeof(struct i2c_msg));
    if (i2c_data.msgs == NULL)
    {
        printf("malloc error");
        close(fd);
        return -1;
    }

    ioctl(fd, I2C_TIMEOUT, 1);
    ioctl(fd, I2C_RETRIES, 2);

    //write reg
    memset(buftmp, 0, 32);
    buftmp[0] = sub_addr[0];
    buftmp[1] = sub_addr[1];
    i2c_data.msgs[0].len = 2;    //0x4000
    i2c_data.msgs[0].addr = device_addr;
    i2c_data.msgs[0].flags = 0;     // 0: write 1:read
    i2c_data.msgs[0].buf = buftmp;

    //read data
    i2c_data.msgs[1].len = ByteNo;
    i2c_data.msgs[1].addr = device_addr;
    i2c_data.msgs[1].flags = 1;     // 0: write 1:read
    i2c_data.msgs[1].buf = buff;


    ret = ioctl(fd, I2C_RDWR, (unsigned long)&i2c_data);
    if (ret < 0)
    {
        printf("read data %x %x error\r\n", device_addr, sub_addr);
        close(fd);
        free(i2c_data.msgs);
        return 1;
    }
    free(i2c_data.msgs);
    close(fd);

#if 1
    int i;
    printf("i2c_read 0x%02x%02x:",buftmp[0],buftmp[1]);
    for (i = 0; i < ByteNo; i++)
    {
    printf(" 0x%02x",buff[i]);
    }
    printf("\n");
#endif

    return 0;
}

/*****************************************************************************
  i2cд����������1�����豸��ַ������2���Ĵ�����ַ������3��Ҫд������ݻ�����������4��д�����ݴ�С
******************************************************************************/
unsigned char _i2c_write(unsigned char device_addr, unsigned char *sub_addr, unsigned char *buff, int ByteNo)
{
    int fd, ret;
    unsigned char buftmp[32];
    struct i2c_rdwr_ioctl_data i2c_data;
    // const char      *i2c_dev = "/dev/i2c-1";
    char i2c_dev[32]={0};
    snprintf(i2c_dev, sizeof(i2c_dev), "/dev/i2c-%d", qtk_i2c_index);
    //----------------------------------

    device_addr >>= 1;
    //init
    fd = open(i2c_dev, O_RDWR);
    if (fd < 0)
    {
        printf("not have /dev/i2c-1\r\n");
        return -1;
    }

    i2c_data.nmsgs = 1;
    i2c_data.msgs = (struct i2c_msg *)malloc(i2c_data.nmsgs *sizeof(struct i2c_msg));
    if (i2c_data.msgs == NULL)
    {
        printf("malloc error");
        close(fd);
        return -1;
    }

    ioctl(fd, I2C_TIMEOUT, 1);
    ioctl(fd, I2C_RETRIES, 2);

    memset(buftmp, 0, 32);
    if(sub_addr==NULL){
	    memcpy(buftmp, buff, ByteNo);
		printf("power on buftmp=0x%x\n",buftmp[0]);
	    i2c_data.msgs[0].len = ByteNo + 1;;
	    i2c_data.msgs[0].addr = device_addr;
	    i2c_data.msgs[0].flags = 0;     // 0: write 1:read
	    i2c_data.msgs[0].buf = buftmp;

	}else{
   		 buftmp[0] = sub_addr[0];
   		 buftmp[1] = sub_addr[1];
  		 printf("write sub_addr=0x%02x%02x \n",buftmp[0],buftmp[1]);
   		 memcpy(buftmp + 2, buff, ByteNo);
		 i2c_data.msgs[0].len = ByteNo + 2;;
		 i2c_data.msgs[0].addr = device_addr;
		 i2c_data.msgs[0].flags = 0;     // 0: write 1:read
		 i2c_data.msgs[0].buf = buftmp;

    }
    ret = ioctl(fd, I2C_RDWR, (unsigned long)&i2c_data);
    if (ret < 0)
    {
        printf("write reg %x %x error\r\n", device_addr, sub_addr);
        close(fd);
        free(i2c_data.msgs);
        return 1;
    }
    free(i2c_data.msgs);
    close(fd);

#if 1
    int i;
    printf("i2c_write 0x%02x%02x:",buftmp[0],buftmp[1]);
    for(i=0; i<ByteNo; i++)
    {
    printf(" 0x%02x",buftmp[2+i]);
    }
    printf("\n");
#endif
    delay_ms(100);
    return 0;
}


