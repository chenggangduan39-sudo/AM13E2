#include "wtk/core/cfg/wtk_main_cfg.h"
#include  "qtk/tts/parse/qtk_tts_parse.h"
#ifdef WIN32
#include <corecrt_io.h>
#include <fcntl.h>
#endif

static void print_usage(int argc, char**argv)
{
	printf("Usage:\n"
			"\t -use_bin is bin, 1->bin,default 0\n"
			"\t -c cfg file\n"
			"\t -s  text string\n"
			"\t -scp text list file\n");
}

#ifdef WIN32
int string_gdb2utf8(char* szGbk, char* szUtf8, int szUtf8_len)
{
    int n;
    n = MultiByteToWideChar(CP_ACP, 0, szGbk, -1, NULL, 0);
    WCHAR* szUtf16 = (WCHAR*)malloc(sizeof(WCHAR) * n);
    MultiByteToWideChar(CP_ACP, 0, szGbk, -1, szUtf16, n);
    n = WideCharToMultiByte(CP_UTF8, 0, szUtf16, -1, NULL, 0, NULL, NULL);
    if (n > szUtf8_len)
    {
        free(szUtf16);
        return -1;
    }
    n = WideCharToMultiByte(CP_UTF8, 0, szUtf16, -1, szUtf8, n, NULL, NULL);

    return n;
}

int string_utf8_from_utf16(WCHAR* utf16, UINT64 size)
{
    int len = WideCharToMultiByte(CP_UTF8, 0, utf16, -1, 0, 0, 0, 0);
    char* utf8 = (char*)malloc(len);
    WideCharToMultiByte(CP_UTF8, 0, utf16, -1, utf8, len, 0, 0);
    memcpy(utf16, utf8, len);
    free(utf8);
    printf("len len=%d\n", len);
    return len - 1;
}
#endif

int main(int argc, char **argv)
{
    int ret = 0, idx=0, idx2=0;
    qtk_tts_parse_cfg_t *parse_cfg = NULL;
    wtk_main_cfg_t *cfg = NULL;
    wtk_arg_t *arg = NULL;
    qtk_tts_parse_t *parse  = NULL;
    char *cfn = NULL;
    char *istr = NULL;
    char *scp = NULL;
    int use_bin=0;
    wtk_flist_it_t *it = NULL;
    int isgbk = 0;
    char* szUtf8 = NULL;

    arg = wtk_arg_new(argc,argv);
    wtk_arg_get_str_s(arg,"c",&cfn);
    wtk_arg_get_int_s(arg,"use_bin",&use_bin);
    wtk_arg_get_str_s(arg,"s",&istr);
    wtk_arg_get_str_s(arg,"scp",&scp);
    wtk_arg_get_int_s(arg, "gbk", &isgbk);

    if (NULL == cfn || ((NULL == istr && (NULL == scp))))
    {
        print_usage(argc, argv);
        exit(0);
    }
    if (use_bin)
    {
    	parse_cfg = qtk_tts_parse_cfg_new_bin(cfn, 0);
    }
    else
    {
        cfg = wtk_main_cfg_new_type(qtk_tts_parse_cfg,cfn);
        if (cfg)
        	parse_cfg = cfg->cfg;
    }
    if (!parse_cfg)
    {
    	printf("cfg res error\n");
    	return 0;
    }
    if (parse_cfg)
    {
    	parse = qtk_tts_parse_new(parse_cfg);
    	if (NULL==parse)
    	{
    		wtk_debug("parse gen failed\n");
    		print_usage(argc, argv);
    		exit(0);
    	}
    }
//    struct timeval start;
//    struct timeval end;


    if(scp == NULL){
#ifdef WIN32
        if (isgbk)
        {
            //char inbuf[1024];
            int n;
            //针对终端输入
            /* 将stdin的编码为utf-16 */
            //_setmode(_fileno(stdin), _O_U16TEXT);
            //WCHAR* r = _getws_s((WCHAR*)inbuf, 1024/sizeof(WCHAR) - 1);

            /* 将buf中的utf16转为utf-8 */
            //string_utf8_from_utf16((WCHAR*)inbuf, 1024);

            //针对命令行输入 
            n = (strlen(istr) / 2 + 1) * 3;
            szUtf8 = malloc(n);
            n = string_gdb2utf8(istr, szUtf8, n);
            //memcpy(inbuf, szUtf8, n);
            istr = szUtf8;
        }
        //system("CHCP 65001");
#endif
        ret=qtk_tts_parse_process(parse,istr,strlen(istr));
        if(isgbk && szUtf8)
            free(szUtf8);
        if(ret!=0){
        	printf("error: %s", istr);
        	goto end;
        }
        for (idx2=0; idx2 < parse->prosody->nids; idx2++)
        {
        	printf("%d %d %.*s\n", idx, idx2, parse->prosody->prosody_list[idx2]->len, parse->prosody->prosody_list[idx2]->data);
        }
    }else{
        it = wtk_flist_it_new(scp);
        idx=0;
        while(1){
            istr = wtk_flist_it_next(it);
            if(!istr)  break;
            //gettimeofday(&start,NULL);
            ret=qtk_tts_parse_process(parse,istr,strlen(istr));
            if (ret!=0)
            {
            	printf("error: %s", istr);
            	continue;
            }

            for (idx2=0; idx2 < parse->prosody->nids; idx2++)
            {
            	if (parse->prosody->prosody_list[idx2])
            		printf("%d %d %.*s\n", idx, idx2, parse->prosody->prosody_list[idx2]->len, parse->prosody->prosody_list[idx2]->data);
            }
            //gettimeofday(&end,NULL);
            //printf("use time %lf\n", (1000000*(end.tv_sec-start.tv_sec) + end.tv_usec-start.tv_usec) /1000000.0);
            idx++;
        }
    }
end:
	if(it)
		wtk_flist_it_delete(it);
    qtk_tts_parse_delete(parse);
    if(use_bin)
    	qtk_tts_parse_cfg_delete_bin(parse_cfg);
    else
    	wtk_main_cfg_delete(cfg);
    wtk_arg_delete(arg);
    return ret;
}
