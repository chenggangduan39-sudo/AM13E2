#include "qtk_enc_ver.h"

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdbool.h>
#include <linux/types.h> 
#include <sys/types.h> 
#include <sys/ioctl.h> 
#include <errno.h> 
#include <assert.h> 
#include <string.h> 
#include <linux/gpio.h>
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>

char wsbuffer [8]; //用于存放转换好的十六进制字符串，可根据需要定义长度

char * qtk_enc_inttohex(int aa)
{
    sprintf(wsbuffer, "%x", aa);
    return (wsbuffer);
}

int qtk_enc_tolower(int c)  
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

int qtk_enc_htoi(char s[])  
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
        if (qtk_enc_tolower(s[i]) > '9')  
        {  
            n = 16 * n + (10 + qtk_enc_tolower(s[i]) - 'a');  
        }  
        else  
        {  
            n = 16 * n + (qtk_enc_tolower(s[i]) - '0');  
        }  
    }  
    return n;  
}

#ifdef USE_ZHIWEI

#include "EnDe.h"
#include "i2cbusses.h"
#include "smbus.h"

#define I2C_SMBUS_BLOCK_MAX	32


#define tmp_sub_addr 0xA0

unsigned char tmp_dev_addr = 0xC0; //0xC4

#define ERROR_CODE_WRITE_ADDR -1
#define ERROR_CODE_WRITE_DATA -2
#define ERROR_CODE_TRUE -3
#define ERROR_CODE_READ_ADDR -4

#define dbg_print printf

void qtk_enc_WS222DelayMs(int data)
{
    usleep(data*1000);
}

unsigned char GetRandom(void)
{
	return(rand());
}

int qtk_enc_ver(char *i2cdev_path)
{
    int i, res = -1, file, i2cbus = 0;
    char filename[20];
    int daddress = 0xA0;
    int address = 0x60;
    int len = 8;
    int force = 1;
    char tx_block[8] = {}, ex_data[8];
    unsigned char rx_block[I2C_SMBUS_BLOCK_MAX];
    unsigned char error_code=0;



	int j;
	char *result;
	char  tmp[2];
	srand((unsigned)time(NULL));
	printf("tx_data: \r\n");
	for (i = 0; i < 8; ++i){
		result = qtk_enc_inttohex(rand());

		for(j = 0; j < 2; ++j){
			tmp[j] = result[j];
		}
		
		tx_block[i] = qtk_enc_htoi(tmp);
		printf("tx_data[%d] = %d \r\n",i, tx_block[i]);//
	}

	// for(i=8; i!=0; i--)
	// {
	// 	tx_block[8-i]=0xff;//GetRandom();
	// 	dbg_print("0x%x ", tx_block[8-i]);
	// }
    // dbg_print("\n");

    file = open_i2c_dev(i2cbus, filename, sizeof(filename), 0);
    set_slave_addr(file, address, force);
    res = i2c_smbus_write_i2c_block_data(file, daddress, len, tx_block);
	if (res < 0) {
		fprintf(stderr, "Error: Write failed\n");
		close(file);
	}
    qtk_enc_WS222DelayMs(500);
    res = i2c_smbus_read_i2c_block_data(file, daddress, 10, rx_block);
	if (res < 0) {
		fprintf(stderr, "Error: Read failed\n");
		close(file);
	}
	for(i = 0; i < 10; i++){
		dbg_print("0x%x ",rx_block[i]);
	}
    dbg_print("\n");

	EDesEn_Crypt(rx_block, ex_data);

	dbg_print("ex_data: \r\n");
	for(i = 0; i < 8; i++)
	{
		dbg_print("ex_data[%d] = %d \r\n",i, ex_data[i]);
	}
	for(i=0;i<8;i++){
		if(tx_block[i]!=ex_data[i])
		error_code=1;
	}
	if(error_code){
		dbg_print(" EDesEn_Crypt failed \r\n");
		return -1;
	} else{
		dbg_print(" EDesEn_Crypt pass \r\n");
		return 0;
	}
}
#else
#include "sdk/dev/enc/EnDe.h"
#include "wtk/core/wtk_type.h"
#include "wtk/core/wtk_os.h"


// referrence
/*
#define SDA_HIGH	QTK_GPIO_WriteIO(1, WS222_SDA)
#define SDA_LOW		QTK_GPIO_WriteIO(0, WS222_SDA)
#define SCL_HIGH	QTK_GPIO_WriteIO(1, WS222_SCL)
#define SCL_LOW		QTK_GPIO_WriteIO(0, WS222_SCL)
#define SDA_IN		QTK_GPIO_InitIODir(0, WS222_SDA)
#define SDA_OUT		QTK_GPIO_InitIODir(1, WS222_SDA)
#define SCL_OUT		QTK_GPIO_InitIODir(1, WS222_SCL)
#define SDA_DETECT	QTK_GPIO_ReadIO(WS222_SDA)
*/

#define  SET_SDA_IN     0x10001
#define  SET_SDA_OUT    0x10002
#define  SDA_OUTPUT     0x10003
#define  READ_SDA       0x10004
 
#define  SET_SCL_IN     0x20001
#define  SET_SCL_OUT    0x20002
#define  SCL_OUTPUT     0x20003

#define  IO_OUTPUT_HIGH 0x30001
#define  IO_OUTPUT_LOW  0x30002

#ifdef USE_SIZHENG
#define  DEV_PATH  "/dev/i2c-1"
#define  QTK_WS222_CHIP_ADR_BASE    0x60
#else
#ifdef USE_JINRUIXIAN
#define  DEV_PATH  "/dev/i2c-0"
#define  QTK_WS222_CHIP_ADR_BASE    0x60
#else
#ifdef USE_LIXUN
#define  DEV_PATH  "/dev/i2c-2"
#define  QTK_WS222_CHIP_ADR_BASE    0x60
#else
#ifdef USE_RUISHENGKE
#define  DEV_PATH  "/dev/i2c-2"
#define  QTK_WS222_CHIP_ADR_BASE    0x60
#else
#define  DEV_PATH  "/dev/ws222-iic2"
#endif
#endif
#endif
#endif

#define MSG(args...) printf(args) 
#define PRINTFMODE 0
 

int ii2c_fd = -1;   // 文件描述符
char write_value[1];

static int WS222_write(int fd, unsigned char slave_addr, unsigned char *value)
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

static int WS222_read(int fd, unsigned char slave_addr, unsigned char *value)
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

//------------------------------   GPIO 操作 ----------------------

/*设置GPIO方向： 1：out 0:in */
void QTK_GPIO_InitIODir(int dir, int pin_num)
{
 	
   // printf("QTK_GPIO_InitIODir before \n");

     if(dir ==1 && pin_num == 36 )  //WS222_SCL
     {
        ioctl(ii2c_fd,SET_SCL_OUT,NULL);
     }
     else if(dir ==0 && pin_num == 37 )  //WS222_SDA
     {
        ioctl(ii2c_fd,SET_SDA_IN,NULL);
     }	
     else if(dir ==1 && pin_num == 37 )  //WS222_SDA
     {
        ioctl(ii2c_fd,SET_SDA_OUT,NULL);
     }	

  // printf("QTK_GPIO_InitIODir after \n");
}

/* set value: 0-->LOW, 1-->HIGH  */
void QTK_GPIO_WriteIO(int value, int pin_num)
{

   // write_value[0]=value;

   // printf("QTK_GPIO_WriteIO before \n");

    if(pin_num == 36 && value==0)  //WS222_SCL
    {
	    ioctl(ii2c_fd,SCL_OUTPUT,IO_OUTPUT_LOW);
    }

    else if(pin_num == 36 && value==1)  //WS222_SCL
    {
	    ioctl(ii2c_fd,SCL_OUTPUT,IO_OUTPUT_HIGH);
    }
    else if(pin_num == 37 && value==0)  //WS222_SDA
    {
	    ioctl(ii2c_fd,SDA_OUTPUT,IO_OUTPUT_LOW);
    }
    else if(pin_num == 37 && value==1)  //WS222_SDA
    {
	    ioctl(ii2c_fd,SDA_OUTPUT,IO_OUTPUT_HIGH);
    }

   //printf("QTK_GPIO_WriteIO after \n");

}

unsigned char QTK_GPIO_ReadIO(int pin_num)
{

    return  ioctl(ii2c_fd,READ_SDA,NULL);

}

//-----------------------------------------

void qtk_enc_ws222_delay_ms(unsigned int data)
{
#if 1
	usleep(data*1000);
#else
	unsigned int i;
	while(data--)
	{
		for(i=0;i<5000;i++){}  
	}
#endif
}

void qtk_enc_ws222_delay(unsigned int data)
{
	usleep(2);
}


#define WS222_SDA  (37)
#define WS222_SCL  (36)


#define SDA_HIGH	QTK_GPIO_WriteIO(1, WS222_SDA)
#define SDA_LOW		QTK_GPIO_WriteIO(0, WS222_SDA)
#define SCL_HIGH	QTK_GPIO_WriteIO(1, WS222_SCL)
#define SCL_LOW		QTK_GPIO_WriteIO(0, WS222_SCL)
#define SDA_IN		QTK_GPIO_InitIODir(0, WS222_SDA)
#define SDA_OUT		QTK_GPIO_InitIODir(1, WS222_SDA)
#define SCL_OUT		QTK_GPIO_InitIODir(1, WS222_SCL)
#define SDA_DETECT	QTK_GPIO_ReadIO(WS222_SDA)

#define I2C_DELAY	qtk_enc_ws222_delay(2000) 
#define I2C_DELAY_LONG	qtk_enc_ws222_delay_ms(20)


#define ERROR_CODE_WRITE_ADDR 0xFF
#define ERROR_CODE_READ_ADDR  0XFF
#define ERROR_CODE_WRITE_DATA 0XFF
#define ERROR_CODE_TRUE	      0xFF


void qtk_enc_ws222_i2c_start(void)
{
	//SDA_QTK_GPIO_M;
	//SCL_QTK_GPIO_M;

	SDA_OUT;
	SCL_OUT;

	SDA_HIGH;	
	SCL_HIGH;	//just in case default output value is 0, to avoid unexcept falling edge while set I/O as output
	
	I2C_DELAY;

	SDA_OUT;
	SCL_OUT;

	SDA_HIGH;
	I2C_DELAY;
	SCL_HIGH;
	I2C_DELAY;
	I2C_DELAY;
	
	SDA_LOW;
	I2C_DELAY;
	I2C_DELAY;
	SCL_LOW;
	I2C_DELAY;
 

}

void qtk_enc_ws222_i2c_stop(void)
{

	SDA_OUT;
	SCL_OUT;
	
	SDA_LOW;
	I2C_DELAY;
	I2C_DELAY;
	SCL_HIGH;
	I2C_DELAY;
	I2C_DELAY;
	
	SDA_HIGH;
	I2C_DELAY;
	I2C_DELAY;
	I2C_DELAY;
	I2C_DELAY;
}


unsigned char qtk_enc_ws222_i2c_write_byte(unsigned char data)
{
	unsigned char i, ack;

	//printf("qtk_enc_ws222_i2c_write_byte before \n");
	//SDA_OUT;
	//SCL_OUT;
	I2C_DELAY; 

	for(i = 0; i< 8; i++)
	{
		if( (data << i) & 0x80) 
		{
			SDA_HIGH;
			I2C_DELAY;

		}
		else 
		{
			SDA_LOW;
			I2C_DELAY;

		}
		SCL_HIGH;
		I2C_DELAY;
		SCL_LOW;
		I2C_DELAY;
	}
	
	SDA_IN;
	//I2C_DELAY; 

	SCL_HIGH;
	I2C_DELAY;

	ack = SDA_DETECT;//QTK_GPIO_ReadIO(WS222_SDA); /// ack    

	//printf(" _i2c_write_byte, ack = %d \r\n", ack);

	//I2C_DELAY;

	//SCL_OUT;
	SCL_LOW;

	SDA_OUT;
	//SCL_OUT;
	return ack;
}


unsigned char qtk_enc_ws222_i2c_read_byte_ack(void)
{
	unsigned char i, data;
	unsigned char v_return;
	data = 0;

	SDA_IN;
	I2C_DELAY;   
	for(i = 0; i< 8; i++)
	{
		data <<= 1;
		I2C_DELAY;
		SCL_HIGH;
		I2C_DELAY;
		data |= SDA_DETECT;//QTK_GPIO_ReadIO(WS222_SDA);
                I2C_DELAY;
		SCL_LOW;
		I2C_DELAY;
	}
	SDA_OUT;
	SDA_LOW;//QTK_GPIO_WriteIO(0, WS222_SDA);
	I2C_DELAY;
	SCL_HIGH;//QTK_GPIO_WriteIO(1, WS222_SCL);
	v_return = (unsigned char)data&0xFF;
	I2C_DELAY;
	SCL_LOW;//QTK_GPIO_WriteIO(0, WS222_SCL);
	I2C_DELAY;
	
	return v_return;
}


unsigned char qtk_enc_ws222_i2c_read_byte_noack(void)
{
	unsigned char i, data;
	unsigned char v_return;
	data = 0;

	SDA_IN;
	I2C_DELAY;   
	for(i = 0; i< 8; i++)
	{
		data <<= 1;
		I2C_DELAY;
		SCL_HIGH;
		I2C_DELAY;
		data |= SDA_DETECT;//QTK_GPIO_ReadIO(WS222_SDA);
		SCL_LOW;
		I2C_DELAY;
	}
	SDA_OUT;
	SDA_HIGH;//QTK_GPIO_WriteIO(1, WS222_SDA);
	I2C_DELAY;
	SCL_HIGH;//QTK_GPIO_WriteIO(1, WS222_SCL);
	v_return = (unsigned char)data&0xFF;
	I2C_DELAY;
	SCL_LOW;//QTK_GPIO_WriteIO(0, WS222_SCL);
	I2C_DELAY;
	
	return v_return;
}

unsigned char qtk_enc_ws222_i2c_write(unsigned char device_addr, unsigned char sub_addr, unsigned char *buff, int ByteNo)
{
	unsigned char i;

        //printf("qtk_enc_ws222_i2c_start before \n");
	qtk_enc_ws222_i2c_start();  // 最后状态 SCL_LOW  SDA_LOW
       // printf("qtk_enc_ws222_i2c_start after \n");

        //printf("I2C_DELAY_LONG before \n");
	I2C_DELAY_LONG;
       // printf("I2C_DELAY_LONG after \n");


	if(qtk_enc_ws222_i2c_write_byte(device_addr))   //&0xFF  
	{
		qtk_enc_ws222_i2c_stop();
		#if PRINTFMODE
		printf("\n\rWRITE I2C : Write Error - Device Addr");
		#endif
		return ERROR_CODE_WRITE_ADDR;
	}
	else
	{
		#if PRINTFMODE
		printf("\n\rWRITE I2C : Write successful - Device Addr");
		#endif
	}

	if(qtk_enc_ws222_i2c_write_byte(sub_addr)) 
	{
		qtk_enc_ws222_i2c_stop();
		#if PRINTFMODE
		printf("\n\rWRITE I2C : Write Error - Sub Addr");
		#endif
		return ERROR_CODE_WRITE_ADDR;
	}

	else
	{
		#if PRINTFMODE
		printf("\n\rWRITE I2C : Write successful - Sub Addr ");
		#endif
	}


	for(i = 0; i<ByteNo; i++) 
	{	

		if(qtk_enc_ws222_i2c_write_byte(buff[i])) 
		{

			qtk_enc_ws222_i2c_stop();
			#if PRINTFMODE
			printf("\n\rWRITE I2C : Write Error - TX Data");
			#endif
			return ERROR_CODE_WRITE_DATA;
		}


	}


	I2C_DELAY;
	qtk_enc_ws222_i2c_stop();
	I2C_DELAY_LONG;
	return 0;
}

unsigned char qtk_enc_ws222_i2c_read(unsigned char device_addr, unsigned char sub_addr, unsigned char *buff, int ByteNo)
{
	unsigned char i;

	#if PRINTFMODE
	printf("\r\n_i2c_read");
	#endif

	qtk_enc_ws222_i2c_start();

	I2C_DELAY_LONG;

	if(qtk_enc_ws222_i2c_write_byte(0xc0)) //0xc0 80
	{
		qtk_enc_ws222_i2c_stop();
		#if PRINTFMODE
		printf("\n\r Read I2C Write Error - device Addr\r\n");
		#endif
		return ERROR_CODE_READ_ADDR;
	}
	if(qtk_enc_ws222_i2c_write_byte(0xa0)) 
	{
		qtk_enc_ws222_i2c_stop();
		#if PRINTFMODE
		printf("\n\rRead I2C Write Error - sub Addr \r\n");
		#endif
		return ERROR_CODE_READ_ADDR;
	}

	qtk_enc_ws222_i2c_start();

	if(qtk_enc_ws222_i2c_write_byte(0xc1)) //0xc1 81
	{
		qtk_enc_ws222_i2c_stop();
		#if PRINTFMODE
		printf("\n\rRead I2C Write Error - sub Addr \r\n");
		#endif
		return ERROR_CODE_READ_ADDR;
	}

	for(i = 0; i<ByteNo; i++) 
	{
	     if(i<ByteNo-1)
		buff[i] = qtk_enc_ws222_i2c_read_byte_ack();
	    else
		buff[i] = qtk_enc_ws222_i2c_read_byte_noack();	
	}
	I2C_DELAY;
	I2C_DELAY_LONG;
	qtk_enc_ws222_i2c_stop();
	I2C_DELAY_LONG;
	return 0;
}



#define test_sub_addr 0xA0

int qtk_enc_ver(char *i2cdev_path)
{
	unsigned char error_code, i;
	unsigned char tx_data[8]={0},rx_data[10]={0xa0},ex_data[8]={0xa0};
	unsigned char value=5;

	int j;
	char *result;
	char  tmp[2];
	srand((unsigned)time(NULL));
	// printf("tx_data: \r\n");
	for (i = 0; i < 8; ++i){
		result = qtk_enc_inttohex(rand());

		for(j = 0; j < 2; ++j){
			tmp[j] = result[j];
		}
		
		tx_data[i] = qtk_enc_htoi(tmp);
		// printf("tx_data[%d] = %d \r\n",i, tx_data[i]);//
	}

	error_code=0;

	// if(use_start)
	{
		
		// 先去除官方标准i2c
#ifndef USE_SIZHENG
#ifndef USE_JINRUIXIAN
#ifndef USE_LIXUN
#ifndef USE_RUISHENGKE
		if((access("/dev/i2c-0",F_OK))!= -1)
		{
	        system("rm /dev/i2c-0 &");  
		}
		qtk_enc_ws222_delay_ms(500);
		if((access("/dev/ws222-iic2",F_OK)) == -1)
		{
			system("insmod /data/iic2_drv.ko");
		}
		// double tm;
		// tm = time_get_ms();
		// qtk_enc_ws222_delay_ms(5000);
		// tm = time_get_ms() - tm;
		usleep(10*1000);
#endif
#endif
#endif
#endif
		// printf("dev name %s\n",DEV_PATH);
		// 再挂载在复用的模拟ws222-I2C-0
		if(i2cdev_path)
		{
			ii2c_fd = open(i2cdev_path,O_RDWR);
		}else{
			printf("IIC path set to empty. Change to default path.\n");
			ii2c_fd = open(DEV_PATH,O_RDWR);
		}
		if(-1 == ii2c_fd){
			perror("open i2c error");
			return -1;
		}
	}

	usleep(20*1000);
	// printf("TestProtection begin\r\n");
#if defined(USE_JINRUIXIAN) || defined(USE_RUISHENGKE) || defined(USE_LIXUN) || defined(USE_SIZHENG)
	value=WS222_write(ii2c_fd, QTK_WS222_CHIP_ADR_BASE,tx_data);//0XC0 0x80
#else
	value=qtk_enc_ws222_i2c_write(0xc0, test_sub_addr,tx_data , 8);//0XC0 0x80
#endif
	if(value!=0)
	{
		printf(" EDesEn_Crypt failed \r\n");
		if(ii2c_fd > 0)
		{
			close(ii2c_fd);
		}
		return -1;
	}
	// printf("value=%d\r\n",value);
	qtk_enc_ws222_delay_ms(500);

#if defined(USE_JINRUIXIAN) || defined(USE_RUISHENGKE) || defined(USE_LIXUN) || defined(USE_SIZHENG)
	value=WS222_read(ii2c_fd, QTK_WS222_CHIP_ADR_BASE, rx_data);
#else
	value=qtk_enc_ws222_i2c_read(0Xc1, test_sub_addr,rx_data, 10);
#endif
	if(value!=0)
	{
		printf(" EDesEn_Crypt failed \r\n");
		if(ii2c_fd > 0)
		{
			close(ii2c_fd);
		}
		return -1;
	}
	// printf("rx_data: \r\n");
	// for(i = 0; i < 10; i++)
	// {
	// 	printf("rx_data[%d] = %d   \r\n",i, rx_data[i]);
	// }
	
	EDesEn_Crypt(rx_data, ex_data);
	
	// printf("ex_data: \r\n");
	// for(i = 0; i < 8; i++)
	// {
	// 	printf("ex_data[%d] = %d \r\n",i, ex_data[i]);
	// }
	for(i=0;i<8;i++)
	{
		if(tx_data[i]!=ex_data[i])
		{
			error_code=1;
		}
	}
	if(ii2c_fd > 0)
	{
		close(ii2c_fd);
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

}
#endif
