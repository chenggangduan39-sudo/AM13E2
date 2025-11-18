#include <string.h>
#include <stdio.h>
#include "wavehdr.h"

void wavehdr_init(WaveHeader* header)
{
	if (header)
	{
		memcpy(header->riff_id, "RIFF", 4);
		memcpy(header->riff_type, "WAVE", 4);
		memcpy(header->fmt_id, "fmt ", 4);
		header->fmt_datasize = 16;
		header->fmt_compression_code = 1;
		memcpy(header->data_id, "data", 4);
	}
}

void wavehdr_set_fmt(WaveHeader* header, int channels, int sampleRate,
		int bytesPerSample)
{
	if (header)
	{
		header->fmt_channels = channels;
		header->fmt_sample_rate = sampleRate;
		header->fmt_block_align = channels * bytesPerSample;
		header->fmt_avg_bytes_per_sec = header->fmt_block_align * sampleRate;
		header->fmt_bit_per_sample = bytesPerSample << 3;
	}
}

void wavehdr_set_size(WaveHeader* header, int rawDataSize)
{
	if (header)
	{
		header->riff_datasize = rawDataSize + sizeof(WaveHeader) - 8;
		header->data_datasize = rawDataSize;///header->fmt_channels;
	}
}


void wavehdr_print(WaveHeader* header)
{
#ifndef HEXAGON
	printf("RIFF: %.*s\n",4,header->riff_id);
	printf("RIFF DATA_SIZE: %d\n",header->riff_datasize);
	printf("channels:\t%d\n", header->fmt_channels);
	printf("avg_bytes_per_sec:\t%d\n", header->fmt_avg_bytes_per_sec);
	printf("block align:\t%d\n", header->fmt_block_align);
	printf("sample rate:\t%d\n", header->fmt_sample_rate);
	printf("audio data size:\t%d\n", header->data_datasize);
	printf("fmt data size:\t%d\n",header->fmt_datasize);
#endif
}

void wave_write_file(char *fn,int rate,char *data,int bytes)
{
	WaveHeader hdr;
	FILE *f;

	wavehdr_init(&hdr);
	wavehdr_set_fmt(&hdr,1,rate,2);
	wavehdr_set_size(&hdr,bytes);
	f=fopen(fn,"wb");
	fwrite((void*)&hdr,1,sizeof(hdr),f);
	fwrite((void*)data,1,bytes,f);
	fclose(f);
}

float wtk_float_abs_max(float *a,int n);

void wave_write_file_int(char *fn,int rate,int *data,int len)
{
	short *pv;
	int i;
	float f=1.0/65536;

	pv=(short*)wtk_malloc(len*sizeof(short));
	for(i=0;i<len;++i)
	{
		pv[i]=data[i]*f;
	}
	wave_write_file(fn,rate,(char*)pv,len*2);
	wtk_free(pv);
}

void wave_write_file_float(char *fn,int rate,float *data,int len)
{
	short *pv;
	int i;
	float f;

	pv=(short*)wtk_malloc(len*sizeof(short));
	if(1)
	{
		f=wtk_float_abs_max(data,len);
	}else
	{
		f=1.0;
	}
	wtk_debug("fn=%s max=%f len=%d\n",fn,f,len);
	f=32000.0/f;
	for(i=0;i<len;++i)
	{
		pv[i]=data[i]*f;
	}
	wave_write_file(fn,rate,(char*)pv,len*2);
	wtk_free(pv);
}

#include "wtk/core/math/wtk_math.h"

void wave_write_file_float5(char *fn,int channel,int rate,float **data,int len)
{
	short **v;
	int i,j;
	float f=30000;

	v=(short**)wtk_calloc(channel,sizeof(short*));
	for(i=0;i<channel;++i)
	{
		v[i]=(short*)wtk_calloc(len,sizeof(short));
		//f=wtk_float_max(data[i],len);
		//wtk_debug("v[%d]=%f\n",i,f);
		for(j=0;j<len;++j)
		{
			v[i][j]=data[i][j]*f;
		}
	}
	wave_write_file5(fn,channel,rate,v,len);
	for(i=0;i<channel;++i)
	{
		wtk_free(v[i]);
	}
	wtk_free(v);
}

void wave_write_file_float32(char *fn,int channel,int rate,wtk_strbuf_t **bufs)
{
	wtk_strbuf_t **xbuf;
	int i,j,n;
	short v;
	float *fp;
	float f;

	xbuf=(wtk_strbuf_t**)wtk_calloc(channel,sizeof(wtk_strbuf_t*));
	for(i=0;i<channel;++i)
	{
		xbuf[i]=wtk_strbuf_new(1024,1);
	}
	for(i=0;i<channel;++i)
	{
		n=bufs[i]->pos/sizeof(float);
		fp=(float*)(bufs[i]->data);
		for(j=0;j<n;++j)
		{
			f=fp[j];
			if(f>0)
			{
				v=f+0.5;
			}else if(f<0)
			{
				v=f-0.5;
			}else
			{
				v=f;
			}
			wtk_strbuf_push(xbuf[i],(char*)&v,2);
		}
	}
	wave_write_file3(fn,channel,rate,xbuf);
	for(i=0;i<channel;++i)
	{
		wtk_strbuf_delete(xbuf[i]);
	}
	wtk_free(xbuf);
}

void wave_write_file_float3(char *fn,int channel,int rate,wtk_strbuf_t **bufs)
{
	wtk_strbuf_t **xbuf;
	int i,j,n;
	float mx;
	short v;
	float *fp;

	xbuf=(wtk_strbuf_t**)wtk_calloc(channel,sizeof(wtk_strbuf_t*));
	for(i=0;i<channel;++i)
	{
		xbuf[i]=wtk_strbuf_new(1024,1);
	}
	for(i=0;i<channel;++i)
	{
		n=bufs[i]->pos/sizeof(float);
		fp=(float*)(bufs[i]->data);
		mx=wtk_float_abs_max(fp,n);
		//f=1.0;
		wtk_debug("maxf=%f n=%d\n",mx,n);
		if(mx<0.1)
		{
			mx=0.1;
		}
		//mx=0.5;
		mx=32000.0/(mx*2);
		for(j=0;j<n;++j)
		{
			v=fp[j]*mx;
			wtk_strbuf_push(xbuf[i],(char*)&v,2);
		}
	}
	wave_write_file3(fn,channel,rate,xbuf);
	for(i=0;i<channel;++i)
	{
		wtk_strbuf_delete(xbuf[i]);
	}
	wtk_free(xbuf);
}

void wave_write_file_float4(char *fn,int channel,int rate,wtk_strbuf_t **bufs,wtk_strbuf_t **pad,int nx)
{
	wtk_strbuf_t **xbuf;
	int i,j,n;
	float mx;
	short v;
	float *fp;

	xbuf=(wtk_strbuf_t**)wtk_calloc(channel+nx,sizeof(wtk_strbuf_t*));
	for(i=0;i<(channel+nx);++i)
	{
		xbuf[i]=wtk_strbuf_new(1024,1);
	}
	for(i=0;i<channel;++i)
	{
		n=bufs[i]->pos/sizeof(float);
		fp=(float*)(bufs[i]->data);
		mx=wtk_float_abs_max(fp,n)*2;
		for(j=0;j<n;++j)
		{
			v=32700*fp[j]/mx;
			wtk_strbuf_push(xbuf[i],(char*)&v,2);
		}
	}
	for(i=0;i<nx;++i)
	{
		wtk_strbuf_push(xbuf[i+channel],pad[i]->data,pad[i]->pos);
	}
	wave_write_file3(fn,channel+nx,rate,xbuf);
	for(i=0;i<(channel+nx);++i)
	{
		wtk_strbuf_delete(xbuf[i]);
	}
	wtk_free(xbuf);
}

void wave_write_file_float2(char *fn,int rate,float *data,int len)
{
	short *pv;
	int i;
	float f,max;

	pv=(short*)wtk_malloc(len*sizeof(short));
	max=wtk_float_abs_max(data,len);
	//f=1.0;
	if(max<0.1)
	{
		max=0.1;
	}
	//max=0.5;
	f=32000.0/(max*1.2);
	wtk_debug("max=%f scale=%f\n",max,f);
	for(i=0;i<len;++i)
	{
		pv[i]=data[i]*f;
	}
	wave_write_file(fn,rate,(char*)pv,len*2);
	wtk_free(pv);
}

void wave_write_file_float22(char *fn,int rate,float *data,int len)
{
	short *pv;
	int i;
	float f;

	pv=(short*)wtk_malloc(len*sizeof(short));
	f=32000.0/(1);
	//wtk_debug("max=%f scale=%f\n",max,f);
	for(i=0;i<len;++i)
	{
		pv[i]=data[i]*f;
	}
	wave_write_file(fn,rate,(char*)pv,len*2);
	wtk_free(pv);
}

void wave_write_file2(char *fn,int channel,int rate,char *data,int bytes)
{
	WaveHeader hdr;
	FILE *f;

	wavehdr_init(&hdr);
	wavehdr_set_fmt(&hdr,channel,rate,2);
	wavehdr_set_size(&hdr,bytes);
	f=fopen(fn,"wb");
	fwrite((void*)&hdr,1,sizeof(hdr),f);
	fwrite((void*)data,1,bytes,f);
	fclose(f);
}

void wave_write_file22(char *fn,int channel,int rate,int bytes_per_sample,char *data,int bytes)
{
	WaveHeader hdr;
	FILE *f;

	wavehdr_init(&hdr);
	wavehdr_set_fmt(&hdr,channel,rate,bytes_per_sample);
	wavehdr_set_size(&hdr,bytes);
	//wtk_debug("fn=%s %d\n",fn,bytes_per_sample);
	f=fopen(fn,"wb");
	fwrite((void*)&hdr,1,sizeof(hdr),f);
	fwrite((void*)data,1,bytes,f);
	fclose(f);
}


void wave_write_file3(char *fn,int channel,int rate,wtk_strbuf_t **bufs)
{
	wtk_strbuf_t *output;
	int i,j;
	int len;

	output=wtk_strbuf_new(bufs[0]->pos*channel,1);
	len=bufs[0]->pos;
	for(i=1;i<channel;++i)
	{
		if(bufs[i]->pos<len)
		{
			len=bufs[i]->pos;
		}
	}
	len/=2;
	for(i=0;i<len;++i)
	{
		for(j=0;j<channel;++j)
		{
			wtk_strbuf_push(output,(bufs[j]->data+i*2),2);
		}
	}
	wave_write_file2(fn,channel,rate,output->data,output->pos);
	wtk_strbuf_delete(output);
}


void wave_write_file32(char *fn,int channel,int rate,wtk_strbuf_t **bufs,int skip)
{
	wtk_strbuf_t *output;
	int i,j;
	int len;

	output=wtk_strbuf_new(bufs[0]->pos*channel,1);
	len=bufs[0]->pos;
	for(i=1;i<channel;++i)
	{
		if(bufs[i]->pos<len)
		{
			len=bufs[i]->pos;
		}
	}
	len/=2;
	len-=skip;
	for(i=0;i<len;++i)
	{
		for(j=0;j<channel;++j)
		{
			wtk_strbuf_push(output,(bufs[j]->data+skip*2+i*2),2);
		}
	}
	wave_write_file2(fn,channel,rate,output->data,output->pos);
	wtk_strbuf_delete(output);
}

void wave_write_file4(char *fn,int channel,int rate,char **data,int len)
{
	wtk_strbuf_t *output;
	int i,j;

	output=wtk_strbuf_new(len*channel,1);
	len/=2;
	for(i=0;i<len;++i)
	{
		for(j=0;j<channel;++j)
		{
			wtk_strbuf_push(output,(data[j]+i*2),2);
		}
	}
	wave_write_file2(fn,channel,rate,output->data,output->pos);
	wtk_strbuf_delete(output);
}

void wave_write_file5(char *fn,int channel,int rate,short **data,int len)
{
	wtk_strbuf_t *output;
	int i,j;

	output=wtk_strbuf_new(len*channel,1);
	for(i=0;i<len;++i)
	{
		for(j=0;j<channel;++j)
		{
			wtk_strbuf_push(output,(char*)(data[j]+i),2);
		}
	}
	wave_write_file2(fn,channel,rate,output->data,output->pos);
	wtk_strbuf_delete(output);
}

void wtk_short_dc(short *in,int len)
{
	float f;
	int i;

	f=0;
	for(i=0;i<len;++i)
	{
		f+=in[i];
	}
	f/=len;
	for(i=0;i<len;++i)
	{
		in[i]-=f;
	}
}

#include <ctype.h>
int wave_head_size(char *fn)
{
#if 0
typedef struct wav_header
{
	char riff_id[4];	//riff chunk
	unsigned int riff_size;	//file size - 8
	char riff_fmt[4];

	char sub_id[4];		//fmt sub-chunk
	unsigned int sub_size;	//16 + extra format bytes
	unsigned short audio_fmt;	//1:pcm
	unsigned short channels;
	unsigned int sample_rate;
	unsigned int byte_rate;
	unsigned short block_align;
	unsigned short bits_per_sample;
#if 0
	unsigned short extra_size; //extra format bytes,optional
	/*extra data*/
#endif

#if 0
	char fact_id[4];	//fact chunk,optional
	unsigned int fact_size;
	/*fact data*/
#endif

	char data_id[4];	//data chunk
	unsigned int data_size;
	/*wav sample data*/
}wav_header_t;
#endif

	FILE *f;
	int ret=0;
	int size_until_fmt_chunk_size=16;
	int fmt_chunk_size;
	int dataID_dataLen_size=4+4;
	int size_until_other_chunk;
	int other_chunk_size;
	char *data=NULL;
	int flag=0;
	int i;


	f=fopen(fn,"r");
	if(!f){goto end;}

	//read fmt chunck size;4 bytes
	ret=fseek(f,16,SEEK_SET);
	if(ret==-1){goto end;}
	ret=fread(&fmt_chunk_size,1,4,f);
	if(ret!=4){ret=-1;goto end;}
	fseek(f,0,SEEK_SET);

	//read data_id or list_id or fact_id or other_id...; 4 bytes
	ret=fseek(f,(size_until_fmt_chunk_size + 4 + fmt_chunk_size),SEEK_SET);
	if(ret==-1){goto end;}
    data=malloc(4);
	ret=fread(data,1,4,f);
	if(ret!=4){ret=-1;goto end;}
	for(i=0;i<4;++i)
	{
		data[i]=(char)toupper(data[i]);
	}
	fseek(f,0,SEEK_SET);

	if(strncmp(data,"DATA",4)==0)
	{
		flag=0;
		ret=size_until_fmt_chunk_size + 4 + fmt_chunk_size + dataID_dataLen_size; 
	}else if(strncmp(data,"LIST",4)==0)
	{
		flag=1;
	}else
	{
		//need check !!!
		flag=1;
	}

	switch(flag)
	{
	case 0:
		break;
	case 1:
		//other data type
		size_until_other_chunk=size_until_fmt_chunk_size + 4 + fmt_chunk_size + 4;
		//read list chunck size;4 bytes
		ret=fseek(f,size_until_other_chunk,SEEK_SET);
		if(ret==-1){goto end;}
		ret=fread(&other_chunk_size,1,4,f);
		if(ret!=4){ret=-1;goto end;}
		fseek(f,0,SEEK_SET);

		ret=size_until_other_chunk + 4 + other_chunk_size + dataID_dataLen_size;
		break;
	default:
		break;
	}

end:
	if(data)
	{
		free(data);
	}

	if(f)
	{
		fclose(f);
	}
	return ret;
}
