#include <stdio.h>  
#include <stdlib.h> 
#include "wtk/core/wtk_os.h"
#include "speex/speex_resampler.h"
#include "wtk/core/wtk_wavfile.h"

int findprocess(char *name)
{
	FILE *fbr;
	char readbuf[16]={0};
	int len;
	char tmpbuf[1024]={0};

	snprintf(tmpbuf, 1024,"ps -a | grep \"%s\" | awk '{if($5 != \"grep\" && $5 != \"sh\"){if(NR==1){print $1}}}'",name);
	// snprintf(tmpbuf, 1024,"ps -a | grep \"%s\" | awk '{print $5}'",argv[1]);
	// snprintf(tmpbuf, 1024,"ps -a | grep \"%s\"",argv[1]);
	fbr=popen(tmpbuf,"r");
	len = fread(readbuf, 1, 16, fbr);
	wtk_debug("============>>>>>>len=%d readbuf[%.*s]\n",len,len,readbuf);
	if(len > 0)
	{
		return 0;
	}else{
		return -1;
	}
}

char *testget_i2c_path(char *params)
{
	int len=strlen(params);
	int i=0,idxs=0,type=1;
	char *data;
	if(len <= 10){return NULL;}
	i=0;
	while (i<len)
	{
		data=params+i;
		switch (type)
		{
		case 1:
			if(strncmp(data, "i2c_path=", strlen("i2c_path=")) == 0)
			{
				idxs=i+8;
				type=2;
				i+=7;
			}
			break;
		case 2:
			if(strncmp(data, ";", strlen(";")) == 0)
			{
				char *path=wtk_malloc(i-idxs+1);
				memset(path, 0, i-idxs+1);
				printf("i=%d idxs=%d %.*s\n",i,idxs,i-idxs,params+idxs);
				memcpy(path, params+idxs, i-idxs);
				return path;
			}
			break;
		default:
			break;
		}
		i++;
	}
	
	return NULL;
}

void test(int argc, char **argv)
{
	char *runname;

	char *path;

	path = testget_i2c_path("a=0;b=c;i2c_path=abcd;");
	printf("path=%s\n",path);
	if(path != NULL)
		wtk_free(path);
	
	printf("===================================>>>>>>>>>>>>>>>>>\n");
	path = testget_i2c_path("a=0;b=c;i2c_path=abcd;iiii=9;asdfoifdsajidjapifjapdijp=0;");
	printf("path=%s\n",path);
	if(path != NULL)
		wtk_free(path);

	printf("===================================>>>>>>>>>>>>>>>>>\n");
	path = testget_i2c_path("a=0;b=c;iiii=9;asdfoifdsajidjapifjapdijp=0;");
	printf("path=%s\n",path);
	if(path != NULL)
		wtk_free(path);

	printf("===================================>>>>>>>>>>>>>>>>>\n");
	path = testget_i2c_path("a=0;b=c;jjjllkjk=0;");
	printf("path=%s\n",path);
	if(path != NULL)
		wtk_free(path);

#if 0
	if(argc <= 1)
	{
		printf("1 ---------> hid_test\n2 ---------> sound_uartup\n3 ---------> Qsound_3308\n");
		return -1;
	}
	if(strncmp("1", argv[1], 1) == 0 && strlen(argv[1]) == 1)
	{
		runname="hid_test";
	}else if(strncmp("2", argv[1], 1) == 0 && strlen(argv[1]) == 1)
	{
		runname="sound_uartup";
	}else if(strncmp("3", argv[1], 1) == 0 && strlen(argv[1]) == 1)
	{
		runname="Qsound_3308";
	}else{
		printf("1 ---------> hid_test\n2 ---------> sound_uartup\n3 ---------> Qsound_3308\n");
		return -1;
	}
	wtk_debug("==================+>>>>>>>>>>>>>>sssc=%d [%s] [%s]\n",argc,argv[1],runname);
	findprocess(runname);
#endif
}

void qtk_mod_bmc_find(char *data, int len)
{
	int i=0,j=0,k=0,pos=0;
	char c;
	char micgain[64]={0};
	char echogain[64]={0};
	char ndelay[64]={0};
	char tmpval[64]={0};

	char str[64]={0};
	snprintf(str, 64, "%f\n%f\n%f", 1.2, 2.1, 15.5);
	printf("cmd:%s\n",str);
	file_write_buf(data, str, strlen(str));
	system("sync");

	while(i<len)
	{
		c=data[i];
		if(c == '\n')
		{
			wtk_debug("data=%.*s\n", i-pos, data+pos);
			memcpy(tmpval, data+pos, i-pos);
			wtk_debug("==========>>>j=%d pos=%d data=%f\n",j , pos,atof(tmpval));
			// memcpy(ndelay, data+i+1, len-i-1);
			// wtk_debug("len=%d data[%d]=%s|%s|%s\n",len,i,data,gain,ndelay);
			pos=i;
			j++;
		}
		i++;
	}
	if(j==2)
	{
		memcpy(tmpval, data+pos, i-pos);
		wtk_debug("micgain=%f\n",atof(tmpval));
	}
}

void test_paser_file(char *filename)
{
	char *data=NULL;
	int len=0;

	data = file_read_buf(filename, &len);
	qtk_mod_bmc_find(data, len);
	wtk_free(data);
}

void test_resample(char *fn, char *ofn)
{
	SpeexResamplerState *in_resample;
	char *outresample=NULL;
	int inlen=0,outlen=0;
	int channel=1;
	outresample = (char *)wtk_malloc(1024*10);
	in_resample = speex_resampler_init(channel, 32000, 48000, SPEEX_RESAMPLER_QUALITY_DESKTOP, NULL);
	FILE *fp=fopen(ofn, "wb");

	char *data=NULL;
	int len=0;
	data = file_read_buf(fn, &len);
	char *ss=data;
	char *ee=data+len;
	int nx=0;
	int step=2048;

	while(ss<ee)
	{
		memset(outresample, 0, step*4);
		inlen=(step >> 1)/channel;
		outlen=inlen*6;

		wtk_debug("inlen=%d outlen=%d\n",inlen,outlen);
		speex_resampler_process_interleaved_int(in_resample,
									(spx_int16_t *)(ss), (spx_uint32_t *)(&inlen), 
									(spx_int16_t *)(outresample), (spx_uint32_t *)(&outlen));
		wtk_debug("inlen=%d outlen=%d/%d\n",inlen,outlen,outlen*2*channel);
		fwrite(outresample, 1, outlen*2*channel, fp);
		fflush(fp);
		ss+=step;
	}

	speex_resampler_destroy(in_resample);
	fclose(fp);
}


#include <speex/speex_resampler.h>
 
int resample_speex(int input_rate, int output_rate, int channels, void *input, int input_size, void *output, int *output_size) {
    int err;
    SpeexResamplerState *resampler = speex_resampler_init(channels, input_rate, output_rate, 3, &err);
    if (err != RESAMPLER_ERR_SUCCESS) {
        // 处理错误
        return -1;
    }
 
    // 假设input和output是已经分配好的输入和输出缓冲区
    // 它们的大小分别是input_size和*output_size
    int in_samples = input_size;
    int out_samples = *output_size;
    int in_idx = 0;
    int out_idx = 0;
 
    speex_resampler_process_interleaved_float(resampler, (const float *)input, &in_samples, (float *)output, &out_samples);
 
    *output_size = out_samples;
 
    speex_resampler_destroy(resampler);
    return 0;
}

static uint8_t crc8(const uint8_t *data, int len)
{
    unsigned crc = 0;
    int i, j;
    for (j = len; j; j--, data++)
    {
        crc ^= (*data << 8);
        for (i = 8; i; i--)
        {
            if (crc & 0x8000)
                crc ^= (0x1070 << 3);
            crc <<= 1;
        }
    }
    return (uint8_t)(crc >> 8);
}

void test_compress(char *fn, char *ofn)
{
	wtk_strbuf_t *sendbuf=NULL;
	int ret,pos,len;
	FILE *fp=NULL;

	sendbuf = wtk_strbuf_new(1024, 1.0);
	char *data=NULL;

	fp = fopen(ofn, "wb+");

	data = file_read_buf(fn, &len);
	pos=1024;
	char *ss=data+44;
	char *ee=data+len-44;
	char head[4]={0xEB,0x90,0xAA,0xAA};
	char end[4]={0xED,0x03,0xBF,0xBF};
	while(ss < ee)
	{
		wtk_strbuf_reset(sendbuf);
		wtk_strbuf_push(sendbuf, head, 4);
		short slen=pos;
		wtk_strbuf_push(sendbuf, (char *)&slen, 2);
		wtk_strbuf_push(sendbuf, ss, pos);
		short cc = crc8(ss, pos);
		wtk_strbuf_push(sendbuf, (char *)&cc, 2);
		wtk_strbuf_push(sendbuf, end, 4);
		// qtk_uart_write(uart, sendbuf->data, sendbuf->pos);
		ret = fwrite(sendbuf->data, 1, sendbuf->pos, fp);
		fflush(fp);
		wtk_debug("send data len:%d\n", sendbuf->pos);
		ss+=pos;
	}
	char tzdata[1024]={0};
	wtk_strbuf_reset(sendbuf);
	wtk_strbuf_push(sendbuf, head, 4);
	short tslen=1024;
	wtk_strbuf_push(sendbuf, (char *)&tslen, 2);
	wtk_strbuf_push(sendbuf, tzdata, 1024);
	short tcc = crc8(tzdata, 1024);
	wtk_strbuf_push(sendbuf, (char *)&tcc, 2);
	wtk_strbuf_push(sendbuf, end, 4);
	ret = fwrite(sendbuf->data, 1, sendbuf->pos, fp);
	fflush(fp);

end:
	if(sendbuf){
		wtk_strbuf_delete(sendbuf);
	}
	if(fp)
	{
		fclose(fp);
	}
}

typedef enum{
	test_hdgd_uart_RECV_START,
	test_hdgd_uart_RECV_TYPE,
	test_hdgd_uart_RECV_DATA,
	test_hdgd_uart_RECV_END,
}test_hdgd_uart_type;
#define RECV_HEAD_LEN 1024
char recv_buf[100];

int grep_start2(char *data, int len, int *alen)
{
	int pos=0;
	char head[4]={0xEB,0x90,0xAA,0xAA};
	while(pos < len)
	{
		if(memcmp(data+pos, head, 4) == 0)
		{
			short dlen;
			memcpy(&dlen, data+pos+4, 2);
			*alen=pos;
			return dlen;
		}
		pos++;
	}
	*alen=pos;
	return -1;
}

int grep_end(char *data, int len, int *alen)
{
	int pos=0;
	char end[4]={0xED,0x03,0xBF,0xBF};
	while(pos < len)
	{
		if(memcmp(data+pos, end, 4) == 0)
		{
			*alen=pos;
			return pos;
		}
		pos++;
	}
	*alen=pos;
	return -1;
}

void test_decompress(char *fn, char *ofn)
{
	wtk_strbuf_t *outbuf=NULL;
	wtk_strbuf_t *databuf=NULL;
	FILE *ofp=NULL;
	char *indata=NULL;
	int inlen=0;
	int ret,pos,alen, datalen=0;
	int recvheadlen=1036;
	
	indata = file_read_buf(fn, &inlen);
	outbuf=wtk_strbuf_new(1024, 1.0);
	databuf=wtk_strbuf_new(1024, 1.0);
	wtk_strbuf_reset(databuf);
	ofp=fopen(ofn, "wb+");
	test_hdgd_uart_type type=test_hdgd_uart_RECV_START;
	
	char *ss=indata;
	char *ee=indata+inlen;

	while(ss < ee)
	{
		memset(recv_buf, 0, sizeof(recv_buf));
		memcpy(recv_buf, ss, recvheadlen);
		ret = recvheadlen;
		if(ret > 0)
		{
			wtk_strbuf_push(outbuf, recv_buf, ret);
			switch (type)
			{
			case test_hdgd_uart_RECV_START:
				pos = grep_start2(outbuf->data, outbuf->pos, &alen);
				if(pos >= 0)
				{
					datalen = pos;
					if(outbuf->pos >= datalen+alen+6+6)
					{
						wtk_debug("databuf->pos=%d outbuf->pos=%d alen=%d pos=%d datalen=%d\n", databuf->pos, outbuf->pos, alen, pos, datalen);
						wtk_strbuf_push(databuf, outbuf->data+alen+6, datalen);
						wtk_strbuf_pop(outbuf, NULL, alen+datalen+6+6);
						datalen = 0;
						type=test_hdgd_uart_RECV_END;
					}else{
						wtk_debug("databuf->pos=%d outbuf->pos=%d alen=%d datalen=%d\n", databuf->pos, outbuf->pos, alen, datalen);
						wtk_strbuf_push(databuf, outbuf->data+alen+6, outbuf->pos-alen-6);
						datalen = datalen - (outbuf->pos-alen-6);
						wtk_strbuf_pop(outbuf, NULL, outbuf->pos);
						type=test_hdgd_uart_RECV_DATA;
					}
				}else{
					wtk_debug("============================>>>>>>pos=%d alen=%d\n", pos, alen);
					wtk_strbuf_pop(outbuf, NULL, alen-4);
				}
				break;
			case test_hdgd_uart_RECV_DATA:
				pos = grep_end(outbuf->data, outbuf->pos, &alen);
				if(pos >= 0)
				{
					wtk_debug("end=[");
					int i;
					for(i=0; i<outbuf->pos; ++i)
					{
						printf("%02X ", outbuf->data[i]);
					}
					printf("]\n");
					wtk_debug("==================>>>>>>>>>>>>pos=%d outbuf->pos=%d databuf->pos=%d datalen=%d alen=%d\n", pos, outbuf->pos, databuf->pos, datalen, alen);
					if(alen > 2 && datalen > 0)
					{
						wtk_strbuf_push(databuf, outbuf->data, alen-2);
					}
					wtk_strbuf_pop(outbuf, NULL, alen+4);
					type=test_hdgd_uart_RECV_END;
				}else{
					if(outbuf->pos >= datalen)
					{
						wtk_strbuf_push(databuf, outbuf->data, datalen);
						wtk_strbuf_pop(outbuf, NULL, datalen);
						datalen = 0;
					}else{
						wtk_strbuf_push(databuf, outbuf->data, outbuf->pos);
						datalen = datalen - outbuf->pos;
						wtk_strbuf_pop(outbuf, NULL, outbuf->pos);
					}
					wtk_debug("==================>>>>>>>>>>>>databuf->pos=%d outbuf->pos=%d datalen=%d\n",databuf->pos ,outbuf->pos, datalen);
				}
				break;
			default:
				break;
			}
			if(type == test_hdgd_uart_RECV_END)
			{
				wtk_debug("recv data len=%d outbuf->pos=%d\n", databuf->pos, outbuf->pos);
				fwrite(databuf->data, 1, databuf->pos, ofp);
				fflush(ofp);
				wtk_strbuf_reset(databuf);
				type=test_hdgd_uart_RECV_START;
			}
		}
		ss+=recvheadlen;
	}

end:
	if(ofp)
	{
		fclose(ofp);
	}
	if(outbuf)
	{
		wtk_strbuf_delete(outbuf);
	}
	if(databuf)
	{
		wtk_strbuf_delete(databuf);
	}
	if(indata)
	{
		wtk_free(indata);
	}
}

#include "wtk/core/rbin/wtk_rbin2.h"

char * test_get_authvalue(int *len)
{
    char avl[]={0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x00,0x00,0x05,0x00,0x00,0x00,0xD1,0xD0,0x9C,0x99,
0x98,0x19,0x00,0x00,0x00,0x17,0x00,0x00,0x00,0x8E,0x9B,0x8D,0x9A,0x9E,0x92,0x9A,
0x8D,0xA0,0x89,0xCE,0xD1,0xCF,0xA0,0xCD,0xCF,0xCD,0xCA,0xCF,0xCC,0xCE,0xCF,0xF5};
    wtk_rbin2_t *rbin;
    wtk_rbin2_item_t *item;
    char *cfg_fn="./cfg";
    int ret=0;

    rbin = wtk_rbin2_new_str(avl,sizeof(avl));

    item=wtk_rbin2_get2(rbin,cfg_fn,strlen(cfg_fn));
	if(!item)
	{
		wtk_debug("get %s failed\n",cfg_fn);
		ret=-1;goto end;
	}
    *len = item->data->len;

    wtk_rbin2_delete(rbin);
end:
    if(ret < 0)
    {
        *len=0;
        return NULL;
    }
    return item->data->data;
}

int Resample32bitTo16(char *in, char *out, int ilen)
{
	int count=ilen/4;
	int i;
	int j=0;
	for(i=0;i<count;++i){
		float tmp = ((float *)in)[i];
		short reout;
		if(tmp < -0.999999f){
			reout = -32766;
		}else if(tmp > 0.999999f){
			reout = 32767;
		}else{
			reout = (short)(tmp * 32767.0);
		}
		j=i%8;
		if(j==4){
			printf("=======###########===>");
		}
		wtk_debug("=====>>>>>>>>>>i=%d j=%d tmp=%f reout=%d\n",i ,j ,tmp, reout);
		((short *)out)[i]=reout;
	}
	return ilen/2;
}

int resample_32bit_to_16bit(char *fn)
{
	char *data;
	int len;
	data = file_read_buf(fn, &len);
	char *ss=data;
	// len = len -44;
	int pos=0;
	FILE *wf=fopen("./sample.pcm", "wb+");
	unsigned char *sample = (unsigned char*)calloc(1, 4+1);
	int step=32*64*8;
	int ret;
	char *out=(char *)wtk_malloc(step);
	int isample,itmp;
	float fsample;
	short tsample=0;

	wtk_debug("==========++>>>>>>>>len=%d %d/%d/%d/%d\n",len,sizeof(char),sizeof(short),sizeof(int),sizeof(float));

	while (pos < len)
	{
#if 1
		// sample=ss+pos;
		// sample[4]='\0';
		// float *sample32 = (float*)(sample);
		// short sample16 = (short)floor( (*sample32) * 32767 );
		// wtk_debug("pos=%d ==> tsample=%d fsample=%f\n",pos,sample16,*sample32);
		isample = ((int *)(ss+pos))[0];
		itmp = isample >> 16;
		if(itmp > 32767){
			itmp = 32767;
		}else if(itmp < -32766){
			itmp = -32766;
		}
		tsample = (short)(itmp);
		fsample = ((float *)(ss+pos))[0];
		wtk_debug("pos=%d/%f ==>isample=%d tsample=%d fsample=%f\n", pos, pos/(8*64.0), isample, tsample, fsample);

		// fwrite((char *)(&sample16), 2, 1, wf);
		fwrite((char *)(&tsample), 2, 1, wf);
		fflush(wf);
		pos+=4;
#else
		memset(out, 0, step);
		wtk_debug("========>>>>>>>>>>time=%f\n",pos/(8*64.0));
		ret = Resample32bitTo16(ss+pos, out, step);
		fwrite(out, ret, 1, wf);
		fflush(wf);
		pos+=step;
#endif
	}

	fclose(wf);
	wtk_free(data);
	wtk_free(out);
}

int main(int argc, char* argv[])  
{
	resample_32bit_to_16bit(argv[1]);
#if 0
	int avlen;
	char *avch;
	avch = test_get_authvalue(&avlen);
	printf("authvalue[%.*s]\n",avlen,avch);
#endif
#if 0
	int len=0;
	char *data=file_read_buf(argv[1], &len);

	int i=0;
	while(i<len)
	{
		printf("%d ",data[i]);
		if(i+1%16 == 0)
		{
			printf("\n");
		}
	}
#endif
#if 0
	// 使用示例
	int input_rate = 32000;
	int output_rate = 48000;
	int channels = 1; // 单通道
	void *input_buffer; // 输入音频数据
	int input_buffer_size; // 输入音频数据大小
	void *output_buffer; // 输出缓冲区
	int output_buffer_size; // 输出缓冲区大小

	// resample_speex(input_rate, output_rate, channels, input_buffer, input_buffer_size, output_buffer, &output_buffer_size);
	// test(argc, argv);
	// test_paser_file(argv[1]);
	test_resample(argv[1],argv[2]);
//#else
	int is_compress = 0;
	is_compress = atoi(argv[3]);

	if(is_compress==1)
	{
		test_compress(argv[1], argv[2]);
	}else if(is_compress==2){
		test_decompress(argv[1], argv[2]);
	}
#endif
	return 0;  
}
