#include "qtk_module_tool.h"

void qtk_module_replace_pwd(wtk_strbuf_t *dst,char *src,int len,wtk_string_t *pwd)
{
typedef enum{
	QTK_TOOL_NOPWD,
	QTK_TOOL_PWD_START,
	QTK_TOOL_PWD,
}qtk_module_pwd_state_t;
	qtk_module_pwd_state_t state;
	char *s,*e;
	char buf[16];
	int i=0;

	wtk_strbuf_reset(dst);
	s=src;e=src+len;
	state=QTK_TOOL_NOPWD;
	while(s<e)
	{
		switch(state)
		{
		case QTK_TOOL_NOPWD:
			if(*s=='$')
			{
				i=0;
				buf[i++]=*s;
				state=QTK_TOOL_PWD_START;
			}else{
				wtk_strbuf_push_c(dst,*s);
			}
			break;
		case QTK_TOOL_PWD_START:
			buf[i++]=*s;
			if(*s=='{')
			{
				state=QTK_TOOL_PWD;
			}else{
				wtk_strbuf_push(dst,buf,i);
				state=QTK_TOOL_NOPWD;
			}
			break;
		case QTK_TOOL_PWD:
			buf[i++]=*s;
			if(*s=='}' && strncmp(buf,"${pwd}",6)==0)
			{
				wtk_strbuf_push(dst,pwd->data,pwd->len);
				state=QTK_TOOL_NOPWD;
			}else{
				if(i>6)
				{
					state=QTK_TOOL_NOPWD;
					wtk_strbuf_push(dst,buf,i);
				}
			}
			break;
		}
		++s;
	}
	wtk_strbuf_push_c(dst,'\0');
}
