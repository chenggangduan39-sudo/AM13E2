#include <stdio.h>
#include <stdlib.h>
#include "sdk/codec/qtk_usb_uevent.h"
#include "sdk/dev/uart/qtk_uart_cfg.h"
#include "sdk/dev/uart/qtk_uart.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/wtk_strbuf.h"
#include "wtk/os/wtk_log.h"

int qtk_findprocess(char *name)
{
	FILE *fbr;
	char readbuf[16] = {0};
	int len;
	char tmpbuf[1024] = {0};

	snprintf(tmpbuf, 1024, "ps -a | grep \"%s\" | awk '{if($5 != \"grep\" && $5 != \"sh\"){if(NR==1){print $1}}}'", name);
	// snprintf(tmpbuf, 1024,"ps -a | grep \"%s\" | awk '{print $5}'",argv[1]);
	// snprintf(tmpbuf, 1024,"ps -a | grep \"%s\"",argv[1]);
	fbr = popen(tmpbuf, "r");
	len = fread(readbuf, 1, 16, fbr);
	wtk_debug("============>>>>>>len=%d readbuf[%.*s]\n", len, len, readbuf);
	if (len > 0)
	{
		return 0;
	}
	else
	{
		return -1;
	}
}

static int first = 1;
static int first_rate = 48000;
static int is_rate_change = 1;
static int is_one = 0;
static double starttime = 0.0;
static wtk_log_t *log_uevent;
#define USE_SOUNDRESET
#define USE_USBRESET

void qtk_mod_test_on_usb(void *ths, qtk_usb_uevent_state_t state, int sample_rate)
{
	char *rate = NULL;
	char *upresult;
	char tmpbuf[32] = {0};
	int ulen, is_ok, ret;
	switch (state)
	{
	case QTK_USB_STATE_PLAYER_START:
#ifdef USE_SOUNDRESET
		rate = wtk_itoa(sample_rate);
		printf("==================>>>>>>>>>>>>>>start %d [%s]\n", sample_rate, rate);
#if defined(USE_802A) || defined(USE_BMC) || defined(USE_AM32)
		printf("=====>>>>>first_rate=%d rate=%d\n", first_rate, sample_rate);
		// if (first && sample_rate)
		// {
		// 	first_rate = sample_rate;
		// 	first = 0;
		// }
		// else
		// {
		// 	printf("first_rate=%d rate=%d is_rate_change=%d\n", first_rate, sample_rate, is_rate_change);
		// 	if (first_rate != sample_rate)
		// 	{
		// 		is_rate_change = 1;
		// 		first_rate = sample_rate;
		// 	}
		// 	else
		// 	{
		// 		is_rate_change = 0;
		// 	}
		// }
		if (sample_rate)
		{
			first_rate = sample_rate;
		}
		file_write_buf("/oem/qdreamer/qsound/filerate", rate, strlen(rate));
#else
		file_write_buf("/oem/filerate", rate, strlen(rate));
#endif
		system("sync");
#ifdef USE_LIXUN
		system("/oem/sdk_demo/restart.sh");
#else
#if defined(USE_802A) || defined(USE_BMC) || defined(USE_AM32) || defined(USE_AM60)
		// if(access("/oem/qdreamer/qsound/is_sound.txt",F_OK) == 0 && is_rate_change)
		if (access("/oem/qdreamer/qsound/is_sound.txt", F_OK) == 0)
		{
			upresult = file_read_buf("/oem/qdreamer/qsound/is_sound.txt", &ulen);
#else
		if (access("/oem/is_sound.txt", F_OK) == 0)
		{
			upresult = file_read_buf("/oem/is_sound.txt", &ulen);
#endif
			is_ok = atoi(upresult);
			if (is_ok)
			{
#ifdef USE_KTC3308
				system("/ktc/restart.sh");
#else
				system("/oem/qdreamer/qsound/restart.sh");
#endif
			}
			if (upresult)
			{
				wtk_free(upresult);
			}
		}
#endif
		if (rate)
		{
			wtk_free(rate);
		}
#else
		rate = wtk_itoa(sample_rate);
		printf("==================>>>>>>>>>>>>>>start %d [%s]\n", sample_rate, rate);
		wtk_log_log(log_uevent, "==================>>>>>>>>>>>>>>start %d [%s]", sample_rate, rate);
#if defined(USE_802A) || defined(USE_BMC) || defined(USE_AM32)
		if (first && sample_rate)
		{
			first_rate = sample_rate;
			first = 0;
		}
		else
		{
			printf("first_rate = %d,rate = %d\n", first_rate, sample_rate);
			wtk_log_log(log_uevent, "first_rate = %d,rate = %d\n", first_rate, sample_rate);
			if (first_rate != sample_rate)
			{
				is_rate_change = 1; // 采样率改变
				first_rate = sample_rate;
			}
			else
			{
				is_rate_change = 0; // 采样率没有改变
			}
		}
		file_write_buf("/oem/qdreamer/qsound/filerate", rate, strlen(rate));
#else
		file_write_buf("/oem/filerate", rate, strlen(rate));
#endif
		ret = system("sync");
		if (ret < 0)
		{
			wtk_log_log(log_uevent, "===========>>>>>>sync error=%d\n", ret);
		}
#if defined(USE_802A) || defined(USE_BMC) || defined(USE_AM32) || defined(USE_AM60)
		if (access("/oem/qdreamer/qsound/is_sound.txt", F_OK) == 0 && is_rate_change)
		{
			upresult = file_read_buf("/oem/qdreamer/qsound/is_sound.txt", &ulen);
#else
		if (access("/oem/is_sound.txt", F_OK) == 0 && is_rate_change)
		{
			upresult = file_read_buf("/oem/is_sound.txt", &ulen);
#endif
			is_ok = atoi(upresult);
			if (is_ok)
			{
#ifdef USE_KTC3308
				ret = system("/ktc/restart.sh");
#else
				ret = system("/oem/qdreamer/qsound/restart.sh");
#endif
				if (ret < 0)
				{
					wtk_log_log(log_uevent, "===========>>>>>>restart error=%d\n", ret);
				}
			}
			if (upresult)
			{
				wtk_free(upresult);
			}
		}
		if (rate)
		{
			wtk_free(rate);
		}
#endif
		break;
	case QTK_USB_STATE_PLAYER_STOP:
		printf("===============>>>>>>>>>>>>>>>>>>>>stop\n");
#ifdef USE_SOUNDRESET
#ifndef USE_BMC
#ifndef USE_802A
// #ifndef USE_AM32
#ifndef USE_AM60
#ifdef USE_KTC3308
		system("/ktc/kill_sound.sh");
#else
#ifdef USE_LIXUN
		system("/oem/sdk_demo/kill_sound.sh");
#else
		system("/oem/qdreamer/qsound/kill_sound.sh");
#endif
#endif
#endif
#endif
#endif
#endif
		// #endif
		break;
	case QTK_USB_STATE_SYSTEM_REBOOT:
		printf("==============>>>>>>>>>>>>>>>>>>>>>reboot -f\n");
		printf("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<========reboot -f==========>>>>>>>>>>>>>>>>>>>>>>>>\n");
#ifdef USE_USBRESET
		wtk_log_log0(log_uevent, "==============>>>>>>>>>>>>>>>>>>>>>QTK_USB_STATE_SYSTEM_REBOOT");
		ret = system("killall hid_test sound_uartup Qsound_3308");
		if (ret < 0)
		{
			wtk_log_log(log_uevent, "===========>>>>>>kill hid_test sound_uartup Qsound_3308  error=%d\n", ret);
		}
		usleep(50 * 1000);
		// if(time_get_ms() > 50000)
		// {
		ret = system("echo none > /sys/kernel/config/usb_gadget/rockchip/UDC");
		if (ret < 0)
		{
			wtk_log_log(log_uevent, "============>>>>>>>>>>echo none > /sys/kernel/config/usb_gadget/rockchip/UDC  error=%d\n", ret);
		}
		ret = system("sync");
		if (ret < 0)
		{
			wtk_log_log(log_uevent, "===========>>>>>>sync error=%d\n", ret);
		}
		printf("+++/sys/kernel/config/usb_gadget/rockchip/UDC none+++\n");
		wtk_log_log0(log_uevent, "+++/sys/kernel/config/usb_gadget/rockchip/UDC none+++");
		is_one = 1;
#else
#if 0
		is_ok = qtk_findprocess("hid_test");
		wtk_debug("hid_test ====>>> ret=%d\n",is_ok);
		if(is_ok > 0)
		{
			snprintf(tmpbuf,32,"kill -9 %d",is_ok);
			system(tmpbuf);
		}
		is_ok = qtk_findprocess("sound_uartup");
		wtk_debug("sound_uartup ====>>> ret=%d\n",is_ok);
		if(is_ok > 0)
		{
			snprintf(tmpbuf,32,"kill -9 %d",is_ok);
			system(tmpbuf);
		}
#else
		system("killall hid_test sound_uartup");
#endif
#ifdef USE_KTC3308
		system("/ktc/kill_sound.sh");
#else
#ifdef USE_LIXUN
		system("/oem/sdk_demo/kill_sound.sh");
#else
		system("/oem/qdreamer/qsound/kill_sound.sh");
#endif
#endif
		system("reboot -f");
#endif
		// printf("=============kill_sound==>>>>>>>>>>>>>>>>>>>>stop\n");
		// system("/ktc/kill_sound.sh");
		// usleep(300*1000);
		// printf("===============/etc/init.d/S50usbdevice restart>>>>>>>reset uac\n");
		// system("/etc/init.d/S50usbdevice restart");
		break;
	case QTK_USB_STATE_PULL_UP:
#ifdef USE_USBRESET
		if (is_one == 1)
		{
			printf("=======QTK_USB_STATE_PULL_UP=======\n");
			wtk_log_log0(log_uevent, "=======QTK_USB_STATE_PULL_UP=======");
#ifdef USE_KTC3308
			/// ktc/sound_uartup -c /ktc/uart.cfg
			ret = system("/ktc/sound_uartup -c /oem/uart.cfg &");
			if (ret < 0)
			{
				wtk_log_log(log_uevent, "===========>>>>>>sound_uartup error=%d\n", ret);
			}
			ret = system("/ktc/restart.sh");
			if (ret < 0)
			{
				wtk_log_log(log_uevent, "===========>>>>>>restart error=%d\n", ret);
			}
			ret = system("/ktc/hid_test 1000 &");
			if (ret < 0)
			{
				wtk_log_log(log_uevent, "===========>>>>>>hid_test error=%d\n", ret);
			}
#else
#if (defined USE_AM32) || (defined USE_802A) || (defined USE_AM60)
			ret = system("/oem/qdreamer/qsound/sound_uartup &");
#else
			ret = system("/oem/qdreamer/qsound/sound_uartup -c /oem/qdreamer/qsound/uart.cfg &");
#endif
			if (ret < 0)
			{
				wtk_log_log(log_uevent, "===========>>>>>>sound_uartup error=%d\n", ret);
			}
			ret = system("/oem/qdreamer/qsound/restart.sh");
			if (ret < 0)
			{
				wtk_log_log(log_uevent, "===========>>>>>>restart error=%d\n", ret);
			}
			ret = system("/oem/qdreamer/qsound/hid_test 1000 &");
			if (ret < 0)
			{
				wtk_log_log(log_uevent, "===========>>>>>>hid_test error=%d\n", ret);
			}
#endif
			printf("++++++++++++++++++++++++++++++++++++\n");
			wtk_log_log0(log_uevent, "++++++++++++++++++++++++++++++++++++");
			is_one = 0;
		}
#endif
		break;
	default:
		break;
	}
}

void test(int argc, char **argv)
{
	qtk_usb_uevent_t *qu = NULL;
	// log_uevent = wtk_log_new("/tmp/uevent.log");
	log_uevent = NULL;
#if 0
	wtk_main_cfg_t *main_cfg=NULL;
	qtk_uart_cfg_t *cfg=NULL;
	qtk_uart_t *uart=NULL;
	
	main_cfg = wtk_main_cfg_new_type(qtk_uart_cfg, "./uart.cfg");
	if(!main_cfg)
	{
		wtk_debug("uart cfg new faild\n");
		goto end;
	}
	cfg=(qtk_uart_cfg_t *)(main_cfg->cfg);
	uart = qtk_uart_new(cfg);
#endif

	qu = qtk_usb_uevent_new();
	qtk_usb_uevent_set_notify(qu, NULL, (qtk_usb_uevent_notify_f)qtk_mod_test_on_usb);

#if 0
#ifndef USE_USBRESET
	FILE *sff=NULL;
	char buf[16]={0};
	int ret=-1;
	FILE *flog=NULL;

	flog = fopen("/tmp/ulog.txt", "wb+");
#endif

#ifndef USE_USBRESET
	int mcount=0;
	while (1)
	{
#if defined(USE_802A) && defined(USE_BMC)
		sleep(3);
#endif
		wtk_debug("======================>>>>>>>>>>fopen\n");
		sff=fopen("/sys/class/udc/ff400000.usb/state","r");
		wtk_debug("======================>>>>>>>>>>fopen=%p\n",sff);
		if(sff)
		{
			wtk_debug("======================>>>>>>>>>>fread\n");
			ret = fread(buf, 1, 10, sff);
			wtk_debug("======================>>>>>>>>>>fread ret=%d buf=[%s]\n",ret,buf);
			// printf("==================>>>>>>>>>>>ret=%d\n",ret);
			fwrite("==========================>>>>111111\n", strlen("==========================>>>>111111\n"), 1, flog);
			fwrite(buf, ret, 1, flog);
			fflush(flog);
			if(ret > 0)
			{
				printf("=================+>>>>>>>>>>>>>>>>>[%.*s]\n",ret,buf);
				if(strncmp(buf,"configured",strlen("configured")) == 0)
				{
					fclose(sff);
					break;
				}
			}
			fseek(sff,0,SEEK_SET);

			fclose(sff);
			mcount++;
			wtk_debug("======================>>>>>>>>>>mount=%d\n",mcount);
			if(mcount == 9)
			{
				system("cp /tmp/ulog.txt /oem");
				system("sync");
				printf("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<========reboot -f==========>>>>>>>>>>>>>>>>>>>>>>>>\n");
#if 0
				char tmpbuf[32]={0};
				ret = qtk_findprocess("hid_test");
				wtk_debug("hid_test ====>>> ret=%d\n",ret);
				if(ret > 0)
				{
					snprintf(tmpbuf,32,"kill -9 %d",ret);
					system(tmpbuf);
				}
				ret = qtk_findprocess("sound_uartup");
				wtk_debug("sound_uartup ====>>> ret=%d\n",ret);
				if(ret > 0)
				{
					snprintf(tmpbuf,32,"kill -9 %d",ret);
					system(tmpbuf);
				}
#else
				system("killall hid_test sound_uartup");
#endif
#ifdef USE_KTC3308
				system("/ktc/kill_sound.sh");
#else
#ifdef USE_LIXUN
				system("/oem/sdk_demo/kill_sound.sh");
#else
				system("/oem/qdreamer/qsound/kill_sound.sh");
#endif
#endif
				system("reboot -f");
			}
			wtk_debug("======================>>>>>>>>>>sleep\n");
			sleep(2);
			wtk_debug("======================>>>>>>>>>>sleep22222222222222\n");
			continue;
		}
	}
	wtk_debug("=======================>>>>>>>>>>>>>>>>>>start check ok<<<<<<<<<<<<<<<<<<==================\n");

	fwrite("==========================>>>>2222222222222\n", strlen("==========================>>>>2222222222222\n"), 1, flog);

	fwrite("==========================>>>>333333333333333\n", strlen("==========================>>>>333333333333333\n"), 1, flog);
	fflush(flog);
	while(1)
	{
#if 0
		int len=11,ret;
		char *data=wtk_malloc(len);
		ret = qtk_uart_read(uart, data, len);
		if(ret == len)
		{
			int i;
			printf("readbuf==>[");
			for(i=0;i<len;++i)
			{
				printf("0x%02x ",data[i]);
			}
			printf("]\n");
		}
#else
		// sleep(3600);
		sleep(3);
		
		sff=fopen("/sys/class/udc/ff400000.usb/state","r");
		if(sff)
		{
			ret = fread(buf, 1, 9, sff);
			// printf("==================>>>>>>>>>>>ret=%d\n",ret);
			// fwrite("==========================>>>>vvvvvvvvvvvvvvvvvvvv\n", strlen("==========================>>>>vvvvvvvvvvvvvvvvvvvv\n"), 1, flog);
			// fwrite(buf, ret, 1, flog);
			// fflush(flog);

			if(ret > 0)
			{
				// printf("=================+>>>>>>>>>>>>>>>>>[%.*s]\n",ret,buf);
				if(strncmp(buf,"not",strlen("not")) == 0)
				{
					fseek(sff,0,SEEK_SET);
					ret = fread(buf, 1, strlen("not attached"), sff);
					if(ret > 0)
					{
						printf("not attached===================>>>>>>>>>>>>>>>>>\n");
						if(strncmp(buf,"not attached",strlen("not attached")) == 0)
						{
							printf("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<========reboot -f==========>>>>>>>>>>>>>>>>>>>>>>>>\n");
#if 0
							char tmpbuf[32]={0};
							ret = qtk_findprocess("hid_test");
							wtk_debug("hid_test ====>>> ret=%d\n",ret);
							if(ret > 0)
							{
								snprintf(tmpbuf,32,"kill -9 %d",ret);
								system(tmpbuf);
							}
							ret = qtk_findprocess("sound_uartup");
							wtk_debug("sound_uartup ====>>> ret=%d\n",ret);
							if(ret > 0)
							{
								snprintf(tmpbuf,32,"kill -9 %d",ret);
								system(tmpbuf);
							}
#else
							system("killall hid_test sound_uartup");
#endif
#ifdef USE_KTC3308
							system("/ktc/kill_sound.sh");
#else
#ifdef USE_LIXUN
							system("/oem/sdk_demo/kill_sound.sh");
#else
							system("/oem/qdreamer/qsound/kill_sound.sh");
#endif
#endif
							system("reboot -f");
						}
					}
				}
			}
			fseek(sff,0,SEEK_SET);
			if(sff)
			{
				fclose(sff);
			}
		}else{
			sleep(3);
		}
#endif
	}
	fclose(flog);
#endif
#else
	while (1)
	{
		sleep(10);
	}
#endif

#if 0
end:
	if(main_cfg)
	{
		wtk_main_cfg_delete(main_cfg);
	}
	if(uart)
	{
		qtk_uart_delete(uart);
	}
#endif
	if (qu)
	{
		qtk_usb_uevent_delete(qu);
	}
}

int main(int argc, char *argv[])
{
	test(argc, argv);
	return 0;
}
