#include "sdk/dev/enc/EnDe.h"
#include <stdio.h>
#include "wtk/core/wtk_type.h"

char tbuffer [8]; //用于存放转换好的十六进制字符串，可根据需要定义长度

char * inttohex(int aa)
{
    sprintf(tbuffer, "%x", aa);
    return (tbuffer);
}

int tolower(int c)  
{  
    if (c >= 'A' && c <= 'Z')  
    {  
        return c + 'a' - 'A';  
    }  
    else  
    {  
        return c;  
    }  
}

int htoi(char s[])  
{  
    int i;  
    int n = 0;  
    if (s[0] == '0' && (s[1]=='x' || s[1]=='X'))  
    {  
        i = 2;  
    }  
    else  
    {  
        i = 0;  
    }  
    for (; (s[i] >= '0' && s[i] <= '9') || (s[i] >= 'a' && s[i] <= 'z') || (s[i] >='A' && s[i] <= 'Z');++i)  
    {  
        if (tolower(s[i]) > '9')  
        {  
            n = 16 * n + (10 + tolower(s[i]) - 'a');  
        }  
        else  
        {  
            n = 16 * n + (tolower(s[i]) - '0');  
        }  
    }  
    return n;  
} 

#if 0
#include <linux/types.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>

static pthread_mutex_t stMutexI2CLock;
static long long time_delay = 20 * 1000;//20 * 1000;

#define WS222_DEV_NAME        "/dev/i2c-1"
#define WS222_CHIP_ADR_BASE    0x60//0xC0  	// ES7210 I2C ADDRESS 8位地址: 0x80  xC0 x60
#define WS222_CHIP_ADR_BASE1   0xC1//0xC1  	// ES7210 I2C ADDRESS 8位地址: 0x80  xC0 x60


/**********************************
	function: I2C写
	fd：设备结点句柄
	chipaddr：设备地址
	rwaddr：寄存器地址(8bit)
	wbuf：写数据buf地址
	size：数据长度
	返回值：	
		0：写入成功
		-1：写入失败
*************************************/
static int  i2c_write(int fd, unsigned char chipaddr, char rwaddr, unsigned char *wbuf, unsigned int size)
{
	int ret = -1;
	unsigned char *buf = (unsigned char *)malloc(size+2);
	if(buf == NULL){
		printf("malloc error\n");
		return -1;
	}
    struct i2c_rdwr_ioctl_data rdwr;
    struct i2c_msg msg;
	
    rdwr.msgs = &msg;
    rdwr.nmsgs =1;
    msg.addr = chipaddr;
    buf[0] = rwaddr;
    msg.buf = buf;
    msg.flags = 0;
    msg.len = size;   // wbuf长度"1" +  rwaddr 地址长度 "1"   size+

    memcpy(buf+1, wbuf, size); // buf[0] 是 地址  buf[1]是 wbuf数据 

    ret = ioctl(fd, I2C_RDWR, &rdwr);
    if (ret<0)
    {
		wtk_debug("#####################################>>>>>>>>>>>\n");
    	perror("i2c write:");
    }

    free(buf);
    return ret;
}


/***********************************************
	function: I2C读
	fd：设备结点句柄
	chipaddr：设备地址
	rwaddr：寄存器地址(16bit)
	rbuf：读数据buf地址
	size：数据长度
	返回值：	
		0：写入成功
		-1：写入失败
************************************************/

static int i2c_read(int fd, unsigned char chipaddr, unsigned char chipaddr2, char rwaddr, unsigned char *rbuf, unsigned int size)
{
	int ret = -1;
    struct i2c_rdwr_ioctl_data rdwr;
    struct i2c_msg msg[2];

    unsigned char wrbuf[1];

    wrbuf[0] = rwaddr;

    rdwr.msgs = msg;
    rdwr.nmsgs = 2;
    msg[0].addr = chipaddr;
    msg[0].buf = wrbuf;	//  rwaddr 
    msg[0].len = 1;	// rwaddr size 
    msg[0].flags = 0;

    msg[1].addr = chipaddr2;  
    msg[1].buf = rbuf;	
    msg[1].flags = I2C_M_RD;
    msg[1].len = size;
	
	ret = ioctl(fd, I2C_RDWR, &rdwr);
    if (ret<0)
    {
        perror("i2c read:");
    }

    return ret;
}



static int i2c_read_2(int fd, unsigned char chipaddr, char rwaddr, unsigned char *rbuf, unsigned int size)
{
	int ret = -1;
    struct i2c_rdwr_ioctl_data rdwr;
    struct i2c_msg msg[2];

    unsigned char wrbuf[1];

    wrbuf[0] = rwaddr;

    rdwr.msgs = msg;
    rdwr.nmsgs = 1;

    msg[0].addr = chipaddr;  
    msg[0].buf = rbuf;	
    msg[0].flags = I2C_M_RD;
    msg[0].len = size;
	
	ret = ioctl(fd, I2C_RDWR, &rdwr);
    if (ret<0)
    {
        perror("i2c read:");
    }

    return ret;
}


// WS222 写入接口
static int WS222_Write(int fd, unsigned char chipaddr, unsigned int rwaddr, unsigned char *wbuf, unsigned int size)
{
	int ret = -1;
	
	pthread_mutex_lock(&stMutexI2CLock);
	usleep(time_delay);
	ret = i2c_write(fd, chipaddr, rwaddr, wbuf, size);
	pthread_mutex_unlock(&stMutexI2CLock);

	if(ret == -1){
		return -1;
	}

	printf(" WS222 Wirte End \n");

	return ret;
}




//ES7210 读取接口
static int WS222_Read(int fd, unsigned char chipaddr, unsigned char chipaddr2, unsigned int rwaddr, unsigned char *rbuf, unsigned int size)
{
	int ret = -1;
	
	pthread_mutex_lock(&stMutexI2CLock);
	usleep(time_delay);
	ret = i2c_read(fd, chipaddr, chipaddr2, rwaddr, rbuf, size);
	pthread_mutex_unlock(&stMutexI2CLock);
	if(ret == -1){
		return -1;
	}
	
	//TCL_DBG(" WS222 Read End \n");
	return ret;
}


static int WS222_write_3(int fd, unsigned char slave_addr, unsigned char *value)
{
    struct i2c_rdwr_ioctl_data packets;
    struct i2c_msg messages[1];
    unsigned char outBuf[9] = {0xA0};
    memcpy(outBuf + 1, value, 8);
    messages[0].addr = slave_addr;
    messages[0].flags = 0;
    messages[0].len = 9;
    messages[0].buf = outBuf;

    /* Transfer the i2c packets to the kernel and verify it worked */
    packets.msgs = messages;
    packets.nmsgs = 1;
    if (ioctl(fd, I2C_RDWR, &packets) < 0)
    {
        perror("Unable to send data");
        return 1;
    }

    return 0;
}

static int WS222_read_3(int fd, unsigned char slave_addr, unsigned char *value)
{
    struct i2c_rdwr_ioctl_data packets;
    struct i2c_msg messages[2];

    /*
     * In order to read a register, we first do a "dummy write" by writing
     * 0 bytes to the register we want to read from.  This is similar to
     * the packet in set_i2c_register, except it‘s 1 byte rather than 2.
     */
    unsigned char outbuf = 0XA0;
    messages[0].addr = slave_addr;
    messages[0].flags = 0;
    messages[0].len = 1;
    messages[0].buf = &outbuf;

    /* The data will get returned in this structure */
    messages[1].addr = slave_addr;
    messages[1].flags = I2C_M_RD /* | I2C_M_NOSTART*/;
    messages[1].len = 10;
    messages[1].buf = value;

    /* Send the request to the kernel and get the result back */
    packets.msgs = messages;
    packets.nmsgs = 2;
    if (ioctl(fd, I2C_RDWR, &packets) < 0)
    {
        perror("Unable to send data");
        return 1;
    }
    return 0;
}

//------------------------------------------------------


static int WS222_Read_1(unsigned char chipaddr, unsigned int rwaddr, unsigned char *rbuf, unsigned int size)
{
	int ret = -1;
	int fd = open(WS222_DEV_NAME, O_RDWR); 
	if (fd < 0)
	{ 
		//TCL_ERR("can not open file %s\n", WS222_DEV_NAME); 
		perror("open error:");
		return -1; 
	} 
	
	pthread_mutex_lock(&stMutexI2CLock);
	usleep(time_delay);
	ret = i2c_read_2(fd, chipaddr, rwaddr, rbuf, size);
	pthread_mutex_unlock(&stMutexI2CLock);
	if(ret == -1){
		close(fd);
		return -1;
	}
	
	close(fd);
	//TCL_DBG(" WS222 Read End \n");
	return ret;
}



int main (int argc, char *argv[])
{
	
	int ret = -1;
	unsigned char uint8_value = 0, unit8_read_value[8];
	uint8_value = 0xFF;

	pthread_mutex_init(&stMutexI2CLock, NULL);

	unsigned char wbuf[8] = {0};		//8个ff为资料中验证所述
	unsigned char rbuf[10] = {0xa0};											//a0为ic偏移地址，即ic内部寄存器地址
	unsigned char pbuf[8] = {0xa0};											//a0为ic偏移地址，即ic内部寄存器地址
	
	int i,j,tmp1;
	char *result;
	char  tmp[2];


	srand((unsigned)time(NULL));
	for (i = 0; i < 8; ++i){
		result = inttohex(rand());

		for(j = 0; j < 2; ++j){
			tmp[j] = result[j];
		}
		
		wbuf[i] = htoi(tmp);
	}

	//----------------------打开设备--------------------------
	int fd = open(WS222_DEV_NAME, O_RDWR); 

	if (fd < 0)
	{ 
		printf("can not open file %s\n", WS222_DEV_NAME); 
		printf("open error:");
		return 0; 
	} 

	for(i=0;i<8;++i)
	{
		printf("wbuf[%d]=%d\n",i,wbuf[i]);
	}    
	// 
	usleep(time_delay);

	//
	// WS222_Write(WS222_CHIP_ADR_BASE, 0XA0, &uint8_value, 1);
	// ret = WS222_Write(fd, WS222_CHIP_ADR_BASE, 0XA0, wbuf, 8);
	ret = WS222_write_3(fd, WS222_CHIP_ADR_BASE, wbuf);
	printf("=============+>>>>>>>>>>>>write ret=%d\n",ret);
	if(ret < 0)
	{
		close(fd);
	}
		
	printf("read_value ");
	usleep(500*1000);


	// ret = WS222_Read(fd, WS222_CHIP_ADR_BASE, WS222_CHIP_ADR_BASE1, 0xA0, rbuf, 10);
	ret = WS222_read_3(fd, WS222_CHIP_ADR_BASE, rbuf);
	printf("=============+>>>>>>>>>>>>read ret=%d\n",ret);
	if(ret < 0)
	{
		close(fd);
	}

	for(int index=0; index<10;index++)
	{
	printf("0x%x ",rbuf[index]);
	}
	printf("\n");

	usleep(500*1000);
	
	EDesEn_Crypt(rbuf, pbuf);
	close(fd);

	printf("=============================================write:\n");
	//打印输出
	printf("wbuf:");
	for(int i = 0; i<9 ; i++)
	{
		printf("0x%x\t", wbuf[i]);
	}
	printf("\n");
	printf("=============================================read:\n");
	printf("rbuf:");
	for(int i = 0; i<10 ; i++)
	{
		printf("0x%x\t", rbuf[i]);
	} 
	printf("\n");
	printf("=============================================p:\n");
	printf("pbuf:");
	for(int i = 0; i<8 ; i++)
	{
		printf("0x%x\t", pbuf[i]);
	}

	int error_code = 0;
	for(i=0;i<8;i++)
	{
		if(wbuf[i]!=pbuf[i])
		{
			error_code=1;
		}
	}
	if(error_code)
	{
		printf(" EDesEn_Crypt failed \r\n");
		return -1;
	}
	else
	{
		printf(" EDesEn_Crypt pass \r\n");
		return 0;
	}

	//---------------------关闭设备--------------------------
end:
	if(ret == -1){
		close(fd);
		return 0;
	}
	close(fd);
	//TCL_DBG("OtpEepromDevInit \n");
	return 0;
}

#else
// int enc_ver() 
// {  
//         int fd,ret;  
// 		int error_code = 0;
// 		unsigned char wbuf[9] = {0xa0};		//a0为ic偏移地址，后接8个ff为资料中验证所述
// 		unsigned char rbuf[10] = {0xa0};											//a0为ic偏移地址，即ic内部寄存器地址
// 		unsigned char pbuf[8] = {0xa0};											//a0为ic偏移地址，即ic内部寄存器地址
// 		int i,j,tmp1;
// 		char *result;
// 		char  tmp[2];
//         char *devname="/dev/i2c-0";
// 		srand((unsigned)time(NULL));
// 		for (i = 0; i < 8; ++i){
// 			result = inttohex(rand());
// 			for(j = 0; j < 2; ++j){
// 				tmp[j] = result[j];
// 			}
// 			tmp1 = htoi(tmp);
// 			wbuf[i + 1] = tmp1;
// 		}
//         wtk_debug("============================+>>>>>>>>>>>>>\n");
//         fd=open(devname, O_RDWR);  												//接口地址
//         if(fd<0)
//         {  
//                 perror("open error");  
//         }
//         wtk_debug("============================+>>>>>>>>>>>>>fd=%d\n",fd);	
//         wtk_debug("============================+>>>>>>>>>>>>>\n");
// 		ret = ioctl(fd,I2C_TIMEOUT,2);													//超时时间  
//         wtk_debug("============================+>>>>>>>>>>>>>ret=%d\n",ret);
// 		ret = ioctl(fd,I2C_RETRIES,1);													//重复次数  
//         wtk_debug("============================+>>>>>>>>>>>>>ret=%d\n",ret);
// 		//设置ic的地址0xc0去除读写位为0x60
//         ret = ioctl(fd, I2C_SLAVE_FORCE, 0x60);
// 		if ( ret< 0)
// 		{ /* 设置芯片地址 */
// 			printf("oictl:set slave address failed\n");
// 			return -2;
// 		}
//         wtk_debug("============================+>>>>>>>>>>>>>%d\n",ret);
// 		ret = write(fd, wbuf, 9);
//         wtk_debug("============================+>>>>>>>>>>>>>ret=%d\n",ret);
// 		if(ret != 9)
// 		{
// 			printf("Error writing to the %s ret:%d\n",devname,ret);
// 			exit(1);
// 		}
//         wtk_debug("============================+>>>>>>>>>>>>>\n");
//         usleep(500*1000);															//ic中的demo（i2c_WS222_std.c）使用的是500ms，直接参考使用
// 		ret = read(fd, &rbuf, 10);
// 		if(ret <0)
// 		{
// 			printf("Error reading from the %s\n",devname);
// 			exit(1);
// 		}
// 		EDesEn_Crypt(rbuf, pbuf);
// 		close(fd);
// 		//打印输出
// 		for(int i = 0; i<9 ; i++)
// 		{
// 			printf("0x%x\t", wbuf[i]);
// 		}
// 		printf("rbuf:");
// 		for(int i = 0; i<10 ; i++)
// 		{
// 			printf("0x%x\t", rbuf[i]);
// 		}  
// 		for(int i = 0; i<8 ; i++)
// 		{
// 			printf("0x%x\t", pbuf[i]);
// 		}
// 		for(i=0;i<8;i++)
// 		{
// 			if(wbuf[i + 1]!=pbuf[i])
// 			{
// 				error_code=1;
// 			}
// 		}
// 		if(error_code)
// 		{
// 			printf(" EDesEn_Crypt failed \r\n");
// 			return -1;
// 		}
// 		else
// 		{
// 			printf(" EDesEn_Crypt pass \r\n");
// 			return 0;
// 		}
// }
// #include "sdk/dev/enc/qtk_enc_ver.h"
#include "sdk/codec/keros/qtk_keros.h"

int main(int argc,char *argv[])
{
	int ret = -1;
    // enc_ver();
	// qtk_enc_ver();

#ifdef USE_ENC8838
	char *params = "i2c_path=/dev/i2c-0;";
	char *i2cpath=NULL;
	i2cpath = qtk_session_get_i2c_path(params);
	ret = qtk_enc_ver(i2cpath);
	if(i2cpath != NULL)
	{
		free(i2cpath);
	}
	if(ret < 0)
	{
		goto end;
	}
end:
#endif

#ifdef USE_KEROS
	ret = qtk_check_keros(NULL);
	if(ret!=0)
	{
		wtk_debug("mod keros auth failed!");
		return NULL;
	}
#endif
    return 0;
}
#endif
