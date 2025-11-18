#include "qtk_verify.h"

#define QTK_VERIFY_CACHE_PATH "/etc/.qdreamer_verify_info"

static int qtk_verify_get_dll_path(char *path,int bytes)
{
	extern char** environ;
	FILE *fp = NULL;
	char **env = environ;
	char *tmp = NULL;
	int ret;

	while(*env != NULL) {
		if(strncmp("LD_LIBRARY_PATH",*env,sizeof("LD_LIBRARY_PATH")-1) == 0) {
			tmp = (*env)+sizeof("LD_LIBRARY_PATH");
			break;
		}
		++env;
	}
	if(!tmp) {
		ret = -1;
		goto end;
	}

	ret = snprintf(path,bytes,"%.*s/libqvoice.so",(int)strlen(tmp),tmp);
	wtk_debug("path:%s\n",path);
end:
	if(fp) {
		fclose(fp);
	}
	return ret;
}

/*
 * -1 failed;0 first run;1 already run
 */
static int qtk_verify_check_dll()
{
	FILE *fp = NULL;
	char path[256];
	int ret;
	char ch;

	ret = qtk_verify_get_dll_path(path,256);
	if(ret == -1) {
		wtk_debug("exit tag\n");
		goto end;
	}

	fp = fopen(path,"rb");
	if(!fp) {
		wtk_debug("exit tag\n");
		ret = -1;
		goto end;
	}
	fseek(fp,-1,SEEK_END);
	ret = fread((char*)(&ch),1,sizeof(char),fp);
	if(ret != 1) {
		wtk_debug("exit tag\n");
		ret = -1;
		goto end;
	}

	if(ch == 1) {
		ret = 1;
	} else {
		ret = 0;
	}

end:
	if(fp) {
		fclose(fp);
	}
	return ret;
}

static int qtk_verify_write_info(char *usrid,int bytes)
{
	FILE *fp = NULL,*fp2 = NULL;
	char path[256];
	int ret;
	char ch;

	fp = fopen(QTK_VERIFY_CACHE_PATH,"w");
	if(!fp) {
		ret = -1;
		goto end;
	}
	ret = fwrite(usrid,1,bytes,fp);
	if(ret <= 0) {
		ret = -1;
		goto end;
	}


	ret = qtk_verify_get_dll_path(path,256);
	if(ret == -1) {
		goto end;
	}
	fp2 = fopen(path,"a+");
	if(!fp) {
		ret = -1;
		goto end;
	}
	ch = 1;
	fwrite((char*)(&ch),1,sizeof(char),fp2);

	ret = 0;
end:
	if(fp) {
		fclose(fp);
	}
	if(fp2) {
		fclose(fp2);
	}
	return ret;
}

static int qtk_verify_check_info(char *usrid,int bytes)
{
	FILE *fp = NULL;
	char buf[64];
	int ret;

	fp = fopen(QTK_VERIFY_CACHE_PATH,"r");
	if(!fp) {
		ret = -1;
		goto end;
	}

	ret = fread(buf,1,64,fp);
	if(ret <= 0) {
		ret = -1;
		goto end;
	}

	if(strncmp(usrid,buf,bytes) != 0) {
		ret = -1;
		goto end;
	}

	ret = 0;
end:
	return ret;
}

int qtk_verify_proc(char *usrid,int bytes)
{
	int ret;

	ret = qtk_verify_check_dll();
	wtk_debug("============>ret = %d\n",ret);
	if(ret == 0) {
		ret = qtk_verify_write_info(usrid,bytes);
	} else if(ret == 1) {
		ret = qtk_verify_check_info(usrid,bytes);
	}

	return ret;
}

#ifndef _WIN32
int qtk_verify_proc_limitdays(int limitdays)
{
#define QTK_VERIFY_YEAR 118
#define QTK_VERIFY_MONTH 4
#define QTK_VERIFY_DAY 2
	time_t timep;
	struct tm *p;
	int delta = 0;

	time(&timep);
	p = gmtime(&timep);
	delta = (p->tm_year - QTK_VERIFY_YEAR) * 365 + (p->tm_mon - QTK_VERIFY_MONTH) * 30 + (p->tm_mday - QTK_VERIFY_DAY);
	return delta >limitdays ? -1 : 0;
}
#endif

