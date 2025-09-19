#include "wtk_preemph.h" 


void wtk_preemph_dc(float *mic,float *mem,int len)
{
	int i;
	float vin,vout;
	float radius=0.982;
	float den2=radius*radius+0.7*(1-radius)*(1-radius);

	for(i=0;i<len;++i)
	{
		vin=mic[i];
		vout=mem[0]+vin;
		mic[i]=radius*vout;
		mem[0]=mem[1]+2*(-vin+mic[i]);
		mem[1]=vin-den2*vout;
	}
}

float wtk_preemph_asis(float *mic,int len,float memD)
{
	int i;
	float tmp;
	float preemph=0.9;

	for(i=0;i<len;++i)
	{
		tmp=mic[i]-preemph*memD;
		memD=mic[i];
		mic[i]=tmp;
	}
	return memD;
}

float wtk_preemph_asis2(float *mic,int len,float memX)
{
	int i;
	float tmp;
	float preemph=0.9;

	for(i=0;i<len;++i)
	{
		tmp=mic[i]+preemph*memX;
        mic[i]=tmp;
		memX=mic[i];
	}
	return memX;
}
