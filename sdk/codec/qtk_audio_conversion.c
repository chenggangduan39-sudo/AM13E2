#include "qtk_audio_conversion.h"

void qtk_data_change_vol(char *data, int bytes, float shift)
{
	short *ps, *pe;
	int num;

	ps = (short *)data;
	pe = (short *)(data + bytes);

	while(ps < pe){
		num = (*ps) *shift;
		if(num > 32767){
			*ps = 32767;
		}else if(num < -32768){
			*ps = -32768;
		}else{
			*ps = num;
		}
		++ps;
	}
}

void qtk_data_change_vol2(char *data, int bytes, float mshift, float sshift, int mic, int spk)
{
	short *ps, *pe;
	int num,i;

	ps = (short *)data;
	pe = (short *)(data + bytes);

	while(ps < pe){
		for(i=0;i<mic;++i)
		{
			num = (*ps) *mshift;
			if(num > 32767){
				*ps = 32767;
			}else if(num < -32768){
				*ps = -32768;
			}else{
				*ps = num;
			}
			++ps;
		}
		for(i=0;i<spk;++i)
		{
			num = (*ps) *sshift;
			if(num > 32767){
				*ps = 32767;
			}else if(num < -32768){
				*ps = -32768;
			}else{
				*ps = num;
			}
			++ps;
		}
	}
}