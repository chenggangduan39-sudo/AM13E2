#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 
#include <assert.h>
#include <string.h>
#include <fcntl.h>
#include <sys/select.h>
#include "sdk/dev/hid/hidapi.h"
#include <errno.h>

int test_isRunning()
{
	int ret = 0;
	int count=0;
	FILE *fstream=NULL;

	char buff[1024] = {0};
	char input[1024] = {0};

	snprintf(input, 1024, "ps -ef | grep -w \"sound_uartup\" | wc -l");
	if(NULL==(fstream=popen(input, "r")))
	{
		fprintf(stderr,"execute command failed: %s", strerror(errno));
		return -1;
	}

	while(NULL!=fgets(buff, sizeof(buff), fstream))
	{
		count = atoi(buff);
		if(count > 2)
		{
			ret=1;
		}else
		{
			ret = 0;
		}
	}
	pclose(fstream);
	return ret;
}


#define MAX_STR 255
int main (int argc, char *argv[]) {
  int res;
  unsigned char buf[65];
  wchar_t wstr[MAX_STR];
  hid_device *handle;
  int i;
  int time_delay=500;

  if(argc < 2)
  {
    printf("./hid_test 1000");
    exit(0);
  }
  time_delay=atoi(argv[1]);

  res = hid_init();
  printf("hid init\n");
  int count=0;
  while (count < 10)
  {
    // handle = hid_open(0x2207, 0x0019, NULL);
    handle=hid_open_path("/dev/hidg0");
    printf("dev open\n");
    if(handle)
    {
      break;
    }
    count++;
    sleep(1);
  }
  
  if(handle)
  {
    res = hid_get_manufacturer_string(handle, wstr, MAX_STR);
    printf("Manufacturer String: %s\n", (char*)wstr);
    res = hid_get_product_string(handle, wstr, MAX_STR);
    printf("Product String: %s\n", (char*)wstr);
    res = hid_get_serial_number_string(handle, wstr, MAX_STR);
    printf("Serial Number String: (%d) %s\n", wstr[0], (char*)wstr);
    res = hid_get_indexed_string(handle, 1, wstr, MAX_STR);
    printf("Indexed String 1: %s\n", (char*)wstr);

    while(1)
    {
      memset(buf, 0, 65);
      res = hid_read_timeout(handle, buf, 8, time_delay);
      for(i=0;i<res;++i)
      {
        printf("=========>>%x\n",buf[i]);
      }
      if(buf[0] == 0x59)
      {
        if(buf[1] == 0x05)
        {
          system("killall sound_uartup");
          // if(test_isRunning() == 0)
          {
#ifdef USE_KTC3308
            system("/ktc/sound_uartup -c /oem/uart.cfg &");
#else
#ifdef USE_NEWAM32
            system("cp /oem/qdreamer/qsound/sound_uartup /tmp/");
            system("cp /oem/qdreamer/qsound/uart.cfg /tmp/");
	    system("sync");
            system("/tmp/sound_uartup /tmp/uart.cfg &");
#else
#if (defined  USE_AM32) || (defined  USE_802A) || (defined  USE_AM60)
            system("/oem/qdreamer/qsound/sound_uartup &");
#else
            printf("==========sound_uartup -c uart.cfg=========>>>>>>>>>>>>>>>>>>>\n");
            system("/oem/qdreamer/qsound/sound_uartup -c /oem/qdreamer/qsound/uart.cfg &");
#endif
#endif
#endif
          }
        }else if(buf[1] == 0x06)
        {
          printf("=========killall sound_uartup==========>>>>>>>>>>>>>>>>>>>\n");
          system("killall sound_uartup");
        }
      }
    }
    hid_close(handle);
  }else{
    printf("/dev/hidg0 open filed!\n");
  }
  res = hid_exit();
  return 0;
}
