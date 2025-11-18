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
static int is_one = 1;
static double starttime = 0.0;
static wtk_log_t *log_uevent = NULL;
#define USE_SOUNDRESET
#define USE_USBRESET

void qtk_mod_test_check_usb()
{
	FILE *sff;
	int ret;
	char buf2[1028]={0};

	while (1)
	{
		wtk_debug("======================>>>>>>>>>>fopen\n");
		// sff=fopen("/sys/class/udc/ff400000.usb/state","r");
		sff=fopen("/sys/devices/platform/ff400000.usb/udc/ff400000.usb/state","r");
		wtk_debug("======================>>>>>>>>>>fopen=%p\n",sff);
		if(sff)
		{
			wtk_debug("======================>>>>>>>>>>fread\n");
			ret = fread(buf2, 1, 10, sff);
			wtk_debug("======================>>>>>>>>>>fread ret=%d buf=[%s]\n",ret,buf2);
			if(ret > 0)
			{
				if(strncmp(buf2,"configured",strlen("configured")) == 0)
				{
					wtk_log_log0(log_uevent,"---------------->>>>>>>>>>>>>>>find configured!!!!exit");
					system("echo '[$(cat /proc/uptime | cut -d'\\'' ' -f1)s] find configured'");
					fclose(sff);
					break;
				}else{
					wtk_log_log0(log_uevent,"---------------->>>>>echo  disconnect!!!!!!!!");
					system("echo '[$(cat /proc/uptime | cut -d'\\'' ' -f1)s] echo  disconnect'");
					system("echo disconnect >/sys/devices/platform/ff400000.usb/udc/ff400000.usb/soft_connect");
					system("echo disconnect >/sys/devices/platform/ff400000.usb/udc/ff400000.usb/soft_connect");
					usleep(500*1000);

					wtk_log_log0(log_uevent,"---------------->>>>>echo connected!!!!!!!!");
					system("echo '[$(cat /proc/uptime | cut -d'\\'' ' -f1)s] echo connected'");
					system("echo connect >/sys/devices/platform/ff400000.usb/udc/ff400000.usb/soft_connect");
					system("echo connect >/sys/devices/platform/ff400000.usb/udc/ff400000.usb/soft_connect");
					sleep(4);
					system("sync");
				}
			}
			// fseek(sff,0,SEEK_SET);
			fclose(sff);
		}
	}
}

void qtk_mod_test_on_usb(void *ths, qtk_usb_uevent_state_t state, int sample_rate)
{
	char *rate = NULL;
	char *upresult;
	char tmpbuf[32] = {0};
	char buf[4096] = {0};
	int ulen, is_ok, ret;
	FILE *fp;
	FILE *fn;
	char *pv;
	int is_lineout = 0;
	fn = fopen("/oem/qdreamer/qsound/res/cfg","r");
	ret = fread(buf,1,sizeof(buf),fn);
	pv = strstr(buf,"use_mainlineout=");
	if (strstr(buf, "use_mainlineout=1") != NULL || strstr(buf, "use_wooflineout=1") != NULL || strstr(buf, "use_meetinglineout=1") != NULL || strstr(buf, "use_expandlineout=1") != NULL) {
    	is_lineout = 1;
	}
	fclose(fn);
	fn=NULL;

	switch (state)
	{
	case QTK_USB_STATE_PLAYER_START:
#ifdef USE_SOUNDRESET
		wtk_log_log0(log_uevent, "==============>>>>>>>>>>>>>>>>>>>>>QTK_USB_STATE_PLAYER_START");

		rate = wtk_itoa(sample_rate);  
		printf("==================>>>>>>>>>>>>>>start %d [%s]\n",sample_rate,rate);
		wtk_log_log(log_uevent, "==================>>>>>>>>>>>>>>start %d [%s]", sample_rate, rate);
		if(first && sample_rate)
		{
			first_rate = sample_rate;
			first = 0;
		}
		else
		{
			printf("first_rate = %d,rate = %d\n",first_rate,sample_rate);
			wtk_log_log(log_uevent, "first_rate = %d,rate = %d\n", first_rate, sample_rate);
			if(first_rate != sample_rate)
			{
				is_rate_change=1;//采样率改变
				first_rate = sample_rate;
			}else{
				is_rate_change=0;//采样率没有改变
			}
		}
		file_write_buf("/oem/qdreamer/qsound/filerate",rate, strlen(rate));
		ret = system("sync");
		if(ret < 0)
		{
			wtk_log_log(log_uevent, "===========>>>>>>sync error=%d\n",ret);
		}
		if(access("/oem/qdreamer/qsound/is_sound.txt",F_OK) == 0 && is_rate_change)
		{
			upresult=file_read_buf("/oem/qdreamer/qsound/is_sound.txt",&ulen);
			is_ok=atoi(upresult);
			if(is_ok)
			{
				wtk_log_log0(log_uevent, "===========>>>>>>restart ");
				ret = system("/oem/qdreamer/qsound/restart.sh");
				if(ret < 0)
				{
					wtk_log_log(log_uevent, "===========>>>>>>restart error=%d\n",ret);
				}
			}
			if(upresult)
			{
				wtk_free(upresult);
			}
		}
		if(rate)
		{
			wtk_free(rate);
		}
#endif
		break;
	case QTK_USB_STATE_PLAYER_STOP:
		printf("===============>>>>>>>>>>>>>>>>>>>>stop\n");
		// #endif
		wtk_log_log0(log_uevent, "==============>>>>>>>>>>>>>>>>>>>>>QTK_USB_STATE_SYSTEM_STOP");
		break;
	case QTK_USB_STATE_SYSTEM_REBOOT:
		// printf("==============>>>>>>>>>>>>>>>>>>>>>reboot -f\n");
		// printf("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<========reboot -f==========>>>>>>>>>>>>>>>>>>>>>>>>\n");
		wtk_log_log0(log_uevent, "==============>>>>>>>>>>>>>>>>>>>>>QTK_USB_STATE_SYSTEM_REBOOT");
	#if 1
		if(is_lineout){
			fp = fopen("/sys/class/gpio/export", "w");
			if (fp) {
				fprintf(fp, "%d", 53);
				fclose(fp);
			}
			fp = fopen("/sys/class/gpio/gpio53/direction", "w");
			if (fp) {
				fprintf(fp, "%s", "out");
				fclose(fp);
			}
			fp = fopen("/sys/class/gpio/gpio53/value", "w");
			if (fp) {
				fprintf(fp, "%d", 0);
				fclose(fp);
			}
		}
	#endif
		// ret = system("killall hid_test sound_uartup Qsound_3308");
		// if (ret < 0)
		// {
		// 	wtk_log_log(log_uevent, "===========>>>>>>kill hid_test sound_uartup Qsound_3308  error=%d\n", ret);
		// }
		// usleep(50 * 1000);
		// ret = system("echo none > /sys/kernel/config/usb_gadget/rockchip/UDC");
		// if (ret < 0)
		// {
		// 	wtk_log_log(log_uevent, "============>>>>>>>>>>echo none > /sys/kernel/config/usb_gadget/rockchip/UDC  error=%d\n", ret);
		// }
		// ret = system("sync");
		// if (ret < 0)
		// {
		// 	wtk_log_log(log_uevent, "===========>>>>>>sync error=%d\n", ret);
		// }
		// printf("+++/sys/kernel/config/usb_gadget/rockchip/UDC none+++\n");
		// wtk_log_log0(log_uevent, "+++/sys/kernel/config/usb_gadget/rockchip/UDC none+++");

		qtk_mod_test_check_usb();

		is_one = 1;
		// printf("=============kill_sound==>>>>>>>>>>>>>>>>>>>>stop\n");
		// system("/ktc/kill_sound.sh");
		// usleep(300*1000);
		// printf("===============/etc/init.d/S50usbdevice restart>>>>>>>reset uac\n");
		// system("/etc/init.d/S50usbdevice restart");
		break;
	case QTK_USB_STATE_PULL_UP:
		if(is_one == 1)
		{
			printf("=======QTK_USB_STATE_PULL_UP=======\n");
			wtk_log_log0(log_uevent,"=======QTK_USB_STATE_PULL_UP=======");
			///ktc/sound_uartup -c /ktc/uart.cfg
			// ret=system("/oem/qdreamer/qsound/sound_uartup -c /oem/uart.cfg &");
			// if(ret < 0)
			// {
			// 	wtk_log_log(log_uevent, "===========>>>>>>sound_uartup error=%d\n",ret);
			// }
			
			ret=system("/oem/qdreamer/qsound/restart.sh");
			// char *args[] = {"/oem/qdreamer/qsound/restart.sh", NULL};  // 用 NULL 结束参数列表
			// if (execvp(args[0], args) == -1) {
			// 	perror("execvp failed");  // 如果 execvp 执行失败，打印错误信息
			// }
			if(ret < 0)
			{
				wtk_log_log0(log_uevent, "===========>>>>>>restart error");
			}else{
				wtk_log_log0(log_uevent, "===========>>>>>>restart OK");
			}
			#if 1
			if(is_lineout){
				// usleep(750*1000);
				fp = fopen("/sys/class/gpio/export", "w");
				if (fp) {
					fprintf(fp, "%d", 53);
					fclose(fp);
				}
				fp = fopen("/sys/class/gpio/gpio53/direction", "w");
				if (fp) {
					fprintf(fp, "%s", "out");
					fclose(fp);
				}
				fp = fopen("/sys/class/gpio/gpio53/value", "w");
				if (fp) {
					fprintf(fp, "%d", 1);
					fclose(fp);
				}
			}
		#endif
		#if 0
			ret = system("/oem/qdreamer/qsound/hid_test 1000 &");
			if(ret < 0)
			{
				wtk_log_log(log_uevent, "===========>>>>>>hid_test error=%d\n",ret);
			}
		#endif
			printf("++++++++++++++++++++++++++++++++++++\n");
			wtk_log_log0(log_uevent,"++++++++++++++++++++++++++++++++++++");
			is_one=0;
		}
		break;
	default:
		break;
	}
}

void test(int argc, char **argv)
{
	qtk_usb_uevent_t *qu = NULL;
	log_uevent = wtk_log_new("/tmp/uevent.log");
	// log_uevent = NULL;
	
	qu = qtk_usb_uevent_new();
	qtk_usb_uevent_set_notify(qu, NULL, (qtk_usb_uevent_notify_f)qtk_mod_test_on_usb);

	qtk_mod_test_check_usb();

	while (1)
	{
		sleep(10);
	}
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
