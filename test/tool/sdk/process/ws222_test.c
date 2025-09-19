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

#include "sdk/dev/enc/EnDe.h"


// referrence
/*
#define SDA_HIGH	GPIO_WriteIO(1, WS222_SDA)
#define SDA_LOW		GPIO_WriteIO(0, WS222_SDA)
#define SCL_HIGH	GPIO_WriteIO(1, WS222_SCL)
#define SCL_LOW		GPIO_WriteIO(0, WS222_SCL)
#define SDA_IN		GPIO_InitIODir(0, WS222_SDA)
#define SDA_OUT		GPIO_InitIODir(1, WS222_SDA)
#define SCL_OUT		GPIO_InitIODir(1, WS222_SCL)
#define SDA_DETECT	GPIO_ReadIO(WS222_SDA)
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

#define  DEV_PATH  "/dev/ws222-iic2"


#define MSG(args...) printf(args) 
#define PRINTFMODE 0
 

int fd = -1;   // 文件描述符
char write_value[1];

void convertStrToUnChar(char* str, unsigned char* UnChar)  
{  
    int i = strlen(str), j = 0, counter = 0;  
    char c[2];  
    unsigned int bytes[2];  
  
    for (j = 0; j < i; j += 2)   
    {  
        if(0 == j % 2)  
        {  
            c[0] = str[j];  
            c[1] = str[j + 1];  
            sscanf(c, "%02x" , &bytes[0]);  
            UnChar[counter] = bytes[0];  
            counter++;  
        }  
    }  
    return;  
}  


//------------------------------   GPIO 操作 ----------------------

/* 挂载GPIO口*/
/*
int WS222_SetGpioInit(int pin)  
{  
    char buffer[64];  
    int len;  
    int fd;  
  
    fd = open("/sys/class/gpio/export", O_WRONLY);  
    if (fd < 0) 
    {  
        MSG("Failed to open export for writing!\n");  
        return(-1);  
    }  
  
    len = snprintf(buffer, sizeof(buffer), "%d", pin);  
    printf("%s,%d,%d\n",buffer,sizeof(buffer),len);
    if (write(fd, buffer, len) < 0) 
    {  
        MSG("Failed to export gpio!");  
        return -1;  
    }  
     
    close(fd);  
    return 0;  
}  
*/

/*设置GPIO方向： 1：out 0:in */
void GPIO_InitIODir(int dir, int pin_num)
{
 	
   // printf("GPIO_InitIODir before \n");

     if(dir ==1 && pin_num == 36 )  //WS222_SCL
     {
        ioctl(fd,SET_SCL_OUT,NULL);
     }
     else if(dir ==0 && pin_num == 37 )  //WS222_SDA
     {
        ioctl(fd,SET_SDA_IN,NULL);
     }	
     else if(dir ==1 && pin_num == 37 )  //WS222_SDA
     {
        ioctl(fd,SET_SDA_OUT,NULL);
     }	

  // printf("GPIO_InitIODir after \n");
}

/* set value: 0-->LOW, 1-->HIGH  */
void GPIO_WriteIO(int value, int pin_num)
{

   // write_value[0]=value;

   // printf("GPIO_WriteIO before \n");

    if(pin_num == 36 && value==0)  //WS222_SCL
    {
	    ioctl(fd,SCL_OUTPUT,IO_OUTPUT_LOW);
    }

    else if(pin_num == 36 && value==1)  //WS222_SCL
    {
	    ioctl(fd,SCL_OUTPUT,IO_OUTPUT_HIGH);
    }
    else if(pin_num == 37 && value==0)  //WS222_SDA
    {
	    ioctl(fd,SDA_OUTPUT,IO_OUTPUT_LOW);
    }
    else if(pin_num == 37 && value==1)  //WS222_SDA
    {
	    ioctl(fd,SDA_OUTPUT,IO_OUTPUT_HIGH);
    }

   //printf("GPIO_WriteIO after \n");

}

unsigned char GPIO_ReadIO(int pin_num)
{

    return  ioctl(fd,READ_SDA,NULL);

}

//-----------------------------------------

void WS222DelayMs(unsigned int data)
{
	unsigned int i;
	while(data--)
	{
		for(i=0;i<5000;i++){}  
	}
}

void WS222Delay(unsigned int data)
{
usleep(2);
}


#define WS222_SDA  (37)
#define WS222_SCL  (36)


#define SDA_HIGH	GPIO_WriteIO(1, WS222_SDA)
#define SDA_LOW		GPIO_WriteIO(0, WS222_SDA)
#define SCL_HIGH	GPIO_WriteIO(1, WS222_SCL)
#define SCL_LOW		GPIO_WriteIO(0, WS222_SCL)
#define SDA_IN		GPIO_InitIODir(0, WS222_SDA)
#define SDA_OUT		GPIO_InitIODir(1, WS222_SDA)
#define SCL_OUT		GPIO_InitIODir(1, WS222_SCL)
#define SDA_DETECT	GPIO_ReadIO(WS222_SDA)

#define I2C_DELAY	WS222Delay(2000) 
#define I2C_DELAY_LONG	WS222DelayMs(20)


#define ERROR_CODE_WRITE_ADDR 0xFF
#define ERROR_CODE_READ_ADDR  0XFF
#define ERROR_CODE_WRITE_DATA 0XFF
#define ERROR_CODE_TRUE	      0xFF


void WS222_i2c_start(void)
{
	//SDA_GPIO_M;
	//SCL_GPIO_M;

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

void WS222_i2c_stop(void)
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


unsigned char WS222_i2c_write_byte(unsigned char data)
{
	unsigned char i, ack;

	//printf("WS222_i2c_write_byte before \n");
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

	ack = SDA_DETECT;//GPIO_ReadIO(WS222_SDA); /// ack    

	//printf(" _i2c_write_byte, ack = %d \r\n", ack);

	//I2C_DELAY;

	//SCL_OUT;
	SCL_LOW;

	SDA_OUT;
	//SCL_OUT;
	return ack;
}


unsigned char WS222_i2c_read_byte_ack(void)
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
		data |= SDA_DETECT;//GPIO_ReadIO(WS222_SDA);
                I2C_DELAY;
		SCL_LOW;
		I2C_DELAY;
	}
	SDA_OUT;
	SDA_LOW;//GPIO_WriteIO(0, WS222_SDA);
	I2C_DELAY;
	SCL_HIGH;//GPIO_WriteIO(1, WS222_SCL);
	v_return = (unsigned char)data&0xFF;
	I2C_DELAY;
	SCL_LOW;//GPIO_WriteIO(0, WS222_SCL);
	I2C_DELAY;
	
	return v_return;
}


unsigned char WS222_i2c_read_byte_noack(void)
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
		data |= SDA_DETECT;//GPIO_ReadIO(WS222_SDA);
		SCL_LOW;
		I2C_DELAY;
	}
	SDA_OUT;
	SDA_HIGH;//GPIO_WriteIO(1, WS222_SDA);
	I2C_DELAY;
	SCL_HIGH;//GPIO_WriteIO(1, WS222_SCL);
	v_return = (unsigned char)data&0xFF;
	I2C_DELAY;
	SCL_LOW;//GPIO_WriteIO(0, WS222_SCL);
	I2C_DELAY;
	
	return v_return;
}

unsigned char WS222_i2c_write(unsigned char device_addr, unsigned char sub_addr, unsigned char *buff, int ByteNo)
{
	unsigned char i;

        //printf("WS222_i2c_start before \n");
	WS222_i2c_start();  // 最后状态 SCL_LOW  SDA_LOW
       // printf("WS222_i2c_start after \n");

        //printf("I2C_DELAY_LONG before \n");
	I2C_DELAY_LONG;
       // printf("I2C_DELAY_LONG after \n");


	if(WS222_i2c_write_byte(device_addr))   //&0xFF  
	{
		WS222_i2c_stop();
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

	if(WS222_i2c_write_byte(sub_addr)) 
	{
		WS222_i2c_stop();
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

		if(WS222_i2c_write_byte(buff[i])) 
		{

			WS222_i2c_stop();
			#if PRINTFMODE
			printf("\n\rWRITE I2C : Write Error - TX Data");
			#endif
			return ERROR_CODE_WRITE_DATA;
		}


	}


	I2C_DELAY;
	WS222_i2c_stop();
	I2C_DELAY_LONG;
	return ERROR_CODE_TRUE;
}

unsigned char WS222_i2c_read(unsigned char device_addr, unsigned char sub_addr, unsigned char *buff, int ByteNo)
{
	unsigned char i;

	#if PRINTFMODE
	printf("\r\n_i2c_read");
	#endif

	WS222_i2c_start();

	I2C_DELAY_LONG;

	if(WS222_i2c_write_byte(0xc0)) //0xc0 80
	{
		WS222_i2c_stop();
		#if PRINTFMODE
		printf("\n\r Read I2C Write Error - device Addr\r\n");
		#endif
		return ERROR_CODE_READ_ADDR;
	}
	if(WS222_i2c_write_byte(0xa0)) 
	{
		WS222_i2c_stop();
		#if PRINTFMODE
		printf("\n\rRead I2C Write Error - sub Addr \r\n");
		#endif
		return ERROR_CODE_READ_ADDR;
	}

	WS222_i2c_start();

	if(WS222_i2c_write_byte(0xc1)) //0xc1 81
	{
		WS222_i2c_stop();
		#if PRINTFMODE
		printf("\n\rRead I2C Write Error - sub Addr \r\n");
		#endif
		return ERROR_CODE_READ_ADDR;
	}

	for(i = 0; i<ByteNo; i++) 
	{
	     if(i<ByteNo-1)
		buff[i] = WS222_i2c_read_byte_ack();
	    else
		buff[i] = WS222_i2c_read_byte_noack();	
	}
	I2C_DELAY;
	I2C_DELAY_LONG;
	WS222_i2c_stop();
	I2C_DELAY_LONG;
	return ERROR_CODE_TRUE;
}



#define test_sub_addr 0xA0

char buffer [8]; //用于存放转换好的十六进制字符串，可根据需要定义长度

char * inttohex(int aa)
{
    sprintf(buffer, "%x", aa);
    return (buffer);
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

unsigned char Testws(int use_start)
{
	unsigned char error_code, i;
	unsigned char tx_data[8],rx_data[10],ex_data[8];
	unsigned char value=5;

	int j;
	char *result;
	char  tmp[2];
	srand((unsigned)time(NULL));
	printf("tx_data: \r\n");
	for (i = 0; i < 8; ++i){
		result = inttohex(rand());

		for(j = 0; j < 2; ++j){
			tmp[j] = result[j];
		}
		
		tx_data[i] = htoi(tmp);
		printf("tx_data[%d] = %d \r\n",i, tx_data[i]);//
	}

	error_code=0;

	// if(use_start)
	{
		
		// 先去除官方标准i2c

		if((access("/dev/i2c-0",F_OK))!= -1)
		{
	        system("rm /dev/i2c-0 &");  
		}
		WS222DelayMs(500);
		if((access("/dev/ws222-iic2",F_OK)) == -1)
		{
			system("insmod /data/iic2_drv.ko");
		}
		WS222DelayMs(500);

		// 再挂载在复用的模拟ws222-I2C-0
		fd = open(DEV_PATH,O_RDWR);
		if(-1 == fd){
		perror("open i2c error");
		return -1;
		}

	}

	printf("TestProtection begin\r\n");
	
	value=WS222_i2c_write(0xc0, test_sub_addr,tx_data , 8);//0XC0 0x80

	printf("value=%x\r\n",value);
	WS222DelayMs(500);


	WS222_i2c_read(0Xc1, test_sub_addr,rx_data, 10);
	printf("rx_data: \r\n");
	for(i = 0; i < 10; i++)
	{
		printf("rx_data[%d] = %d   \r\n",i, rx_data[i]);
	}
	
	EDesEn_Crypt(rx_data, ex_data);
	
	printf("ex_data: \r\n");
	for(i = 0; i < 8; i++)
	{
		printf("ex_data[%d] = %d \r\n",i, ex_data[i]);
	}
	for(i=0;i<8;i++)
	{
		if(tx_data[i]!=ex_data[i])
		error_code=1;
	}
	if(error_code)
	{
		printf(" EDesEn_Crypt failed \r\n");
		return 0;
	}
	else
	{
		printf(" EDesEn_Crypt pass \r\n");
		return 1;
	}


	
}

int main (int argc, char *argv[])
{
	int start=1;

	// start=atoi(argv[1]);

	Testws(start);
	
	return 0;
}



