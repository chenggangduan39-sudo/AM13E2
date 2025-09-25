#include "wtk/core/rbin/wtk_rbin2.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_opt.h"
#include <sys/types.h>
#include <dirent.h>


void test_rbin_extract(wtk_arg_t *arg)
{
	wtk_rbin2_t *rb;
	int ret;
	char *dn;

	rb=wtk_rbin2_new();
	ret=wtk_arg_get_str_s(arg,"f",&dn);
	if(ret!=0){goto end;}
	ret=wtk_rbin2_read(rb,dn);
	if(ret!=0){goto end;}
	ret=wtk_arg_get_str_s(arg,"d",&dn);
	if(ret!=0){goto end;}
	wtk_rbin2_extract(rb,dn);
end:

	wtk_rbin2_delete(rb);
	return;
}

void zip_add_dir(wtk_rbin2_t *rb,char *dn,char *pre)
{
	DIR *dir=NULL;
	struct dirent *ent;
	wtk_strbuf_t *buf,*buf2;
	wtk_string_t name;

	buf=wtk_strbuf_new(256,1);
	buf2=wtk_strbuf_new(256,1);
	dir=opendir(dn);
	while(1)
	{
		ent=readdir(dir);
		if(!ent){break;}
		if(ent->d_name[0]=='.' || ent->d_name[strlen(ent->d_name)-1]=='~')
		{
			continue;
		}
		//wtk_debug("%d:%s\n",ent->d_type,ent->d_name);
		wtk_strbuf_reset(buf);
		wtk_strbuf_push(buf,dn,strlen(dn));
		if(buf->data[buf->pos-1]!='/')
		{
			wtk_strbuf_push_s(buf,"/");
		}
		wtk_strbuf_push(buf,ent->d_name,strlen(ent->d_name));
		wtk_strbuf_push_c(buf,0);
		if(ent->d_type==8)
		{
			if(pre)
			{
				wtk_strbuf_reset(buf2);
				wtk_strbuf_push(buf2,pre,strlen(pre));
				wtk_strbuf_push_c(buf2,'/');
				wtk_strbuf_push(buf2,ent->d_name,strlen(ent->d_name));
				wtk_string_set(&(name),buf2->data,buf2->pos);
			}else
			{
				wtk_string_set(&(name),ent->d_name,strlen(ent->d_name));
			}
			wtk_rbin2_add(rb,&name,buf->data);
		}else if(ent->d_type==4)
		{
			if(pre)
			{
				wtk_strbuf_reset(buf2);
				wtk_strbuf_push(buf2,pre,strlen(pre));
				wtk_strbuf_push_c(buf2,'/');
				wtk_strbuf_push(buf2,ent->d_name,strlen(ent->d_name));
				wtk_strbuf_push_c(buf2,0);
				zip_add_dir(rb,buf->data,buf2->data);
			}else
			{
				zip_add_dir(rb,buf->data,ent->d_name);
			}
		}
	}
	wtk_strbuf_delete(buf2);
	wtk_strbuf_delete(buf);
	closedir(dir);
}

void test_rbin_zip(wtk_arg_t *arg)
{
	wtk_rbin2_t *rb;
	char *dn;
	int ret=-1;

	rb=wtk_rbin2_new();
	ret=wtk_arg_get_str_s(arg,"d",&dn);
	if(ret!=0){goto end;}
	zip_add_dir(rb,dn,NULL);
	ret=wtk_arg_get_str_s(arg,"f",&dn);
	if(ret!=0){goto end;}
	wtk_debug("write [%s]\n",dn);
	wtk_rbin2_write(rb,dn);
end:
	if(ret!=0)
	{
		wtk_debug("write failed\n");
	}
	//wtk_debug("=============end ======\n");
	wtk_rbin2_delete(rb);
	return;
}

static void print_usage()
{
	printf("rbin\n");
	printf("\t-x extract\n");
	printf("\t-z zip\n ");
	printf("\t-d directory to compress or extract to\n");
	printf("\t-f file to compre to or extract\n");
}


int main(int argc,char **argv)
{
	wtk_arg_t *arg;

	arg=wtk_arg_new(argc,argv);
	if(!arg){goto end;}
	if(wtk_arg_exist_s(arg,"x"))
	{
		test_rbin_extract(arg);
	}else if(wtk_arg_exist_s(arg,"z"))
	{
		test_rbin_zip(arg);
	}else
	{
		print_usage();
	}
end:
	if(arg)
	{
		wtk_arg_delete(arg);
	}
	return 0;
}
