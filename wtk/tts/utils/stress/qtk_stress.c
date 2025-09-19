/*
 * qtk_stress.c
 *
 *  Created on: Mar 3, 2022
 *      Author: dm
 */
#include "wtk/core/wtk_str.h"
#include "qtk_stress.h"
#include "wtk/tts/utils/dtree/qtk_dtree.h"
#include "wtk/tts/utils/pos/qtk_nltkpos.h"
#include <stdlib.h>







/**
 * feats:
 *    forward position of current word in sentence,位置F
 *    backward position of current word in sentence,位置B
 *    forward position of current stress in word, 当前词重音音节的位置F
 *    backward position of current stress in word, 当前词重音音节的位置B
 *    forward position of current stress in phrase, 当前词重音音节在当前短语中的位置F
 *    backward position of current stress in phrase, 当前词重音音节在当前短语中的位置B
 *    number of stress before current stress in phrase, 当前词重音音节在当前短语中之前有多少重音音节  *
 *    number of stress behind current stress in phrase, 当前词重音音节在当前短语中之后有多少重音音节  *
 *    distance between current stress and prev stress, 当前词重音音节与上一个重音音节之间的距离  *
 *    distance between current stress and next stress, 当前词重音音节与下一个重音音节之间的距离  *
 *    pos of prev word, 上一个词的词性
 *    number of syllable for prev word, 上一个词有多少音节
 *    pos of current word, 当前词的词性
 *    number of syllable for current word, 当前词有多少音节
 *    forward position of current word in phrase, 当前词在当前短语中的位置F
 *    backward position of current word in phrase, 当前词在当前短语中的位置B
 *    number of content word before current word in phrase, 当前词在当前短语中之前有多少实义词
 *    number of content word behind current word in phrase, 当前词在当前短语中之后有多少实义词
 *    distance between current and prev content word, 当前词到上一个实义词的距离
 *    distance between current and next content word, 当前词到下一个实义词的距离
 *    pos of next word, 下一个词的词性
 *    number of syllable for next word, 下一个词有多少音节
 *    number of syllable for prev phrase, 当前词所在短语上一个短语有多少音节
 *    number of words for prev phrase, 当前词所在短语上一个短语有多少单词
 *    number of syllable for current phrase, 当前词所在短语有多少音节
 *    number of words for current phrase, 当前词所在短语有多少单词
 *    forward position of current word in phrase, 当前词处所在短语的位置F
 *    backward position of current word in phrase, 当前词处所在短语的位置B
 *    number of syllables in next phrase, 当前词所在短语下一个短语有多少音节
 *    number of words in next phrase, 当前词所在短语下一个短语有多少单词
 *    number of syllables in sentence, 当前词所在句子有多少音节
 *    number of words in sentence, 当前词所在句子有多少单词
 *    number of phrase in sentence, 当前词所在句子有多少短语
 */

//char *flab[]=
//{
//"x^x-pau+ay=l@1_1/A:0_0_0/B:x-x-x@x-x&x-x#x-x$x-x!x-x;x-x|x/C:1+0+1/D:0_0/E:x+x@x+x&x+x#x+x/F:content_1/G:0_0/H:x=x^0=2|L-L%/I:4=4/J:4+4-1",
//"x^pau-ay+l=ah@1_1/A:0_0_0/B:1-0-1@1-1&1-4#1-3$0-0!0-1;0-0|ay/C:1+0+3/D:0_0/E:content+1@1+4&1+3#0+1/F:content_1/G:0_0/H:4=4^1=1|L-L%/I:0=0/J:4+4-1",
//"pau^ay-l+ah=v@1_3/A:1_0_1/B:1-0-3@1-1&2-3#2-2$0-0!1-2;0-0|ah/C:0+0+2/D:content_1/E:content+1@2+3&2+2#1+2/F:det_1/G:0_0/H:4=4^1=1|L-L%/I:0=0/J:4+4-1",
//"ay^l-ah+v=dh@2_2/A:1_0_1/B:1-0-3@1-1&2-3#2-2$0-0!1-2;0-0|ah/C:0+0+2/D:content_1/E:content+1@2+3&2+2#1+2/F:det_1/G:0_0/H:4=4^1=1|L-L%/I:0=0/J:4+4-1",
//"l^ah-v+dh=ah@3_1/A:1_0_1/B:1-0-3@1-1&2-3#2-2$0-0!1-2;0-0|ah/C:0+0+2/D:content_1/E:content+1@2+3&2+2#1+2/F:det_1/G:0_0/H:4=4^1=1|L-L%/I:0=0/J:4+4-1",
//"ah^v-dh+ah=k@1_2/A:1_0_3/B:0-0-2@1-1&3-2#2-1$0-0!1-1;0-0|ah/C:1+0+3/D:content_1/E:det+1@3+2&2+1#1+1/F:content_1/G:0_0/H:4=4^1=1|L-L%/I:0=0/J:4+4-1",
//"v^dh-ah+k=ae@2_1/A:1_0_3/B:0-0-2@1-1&3-2#2-1$0-0!1-1;0-0|ah/C:1+0+3/D:content_1/E:det+1@3+2&2+1#1+1/F:content_1/G:0_0/H:4=4^1=1|L-L%/I:0=0/J:4+4-1",
//"dh^ah-k+ae=t@1_3/A:0_0_2/B:1-0-3@1-1&4-1#3-1$0-0!2-0;0-0|ae/C:0+0+0/D:det_1/E:content+1@4+1&3+1#2+0/F:0_0/G:0_0/H:4=4^1=1|L-L%/I:0=0/J:4+4-1",
//"ah^k-ae+t=pau@2_2/A:0_0_2/B:1-0-3@1-1&4-1#3-1$0-0!2-0;0-0|ae/C:0+0+0/D:det_1/E:content+1@4+1&3+1#2+0/F:0_0/G:0_0/H:4=4^1=1|L-L%/I:0=0/J:4+4-1",
//"k^ae-t+pau=x@3_1/A:0_0_2/B:1-0-3@1-1&4-1#3-1$0-0!2-0;0-0|ae/C:0+0+0/D:det_1/E:content+1@4+1&3+1#2+0/F:0_0/G:0_0/H:4=4^1=1|L-L%/I:0=0/J:4+4-1",
//"ae^t-pau+x=x@1_1/A:1_0_3/B:x-x-x@x-x&x-x#x-x$x-x!x-x;x-x|x/C:0+0+0/D:content_1/E:x+x@x+x&x+x#x+x/F:0_0/G:0_0/H:x=x^1=1|L-L%/I:0=0/J:4+4-1",
//};
// char *flab[]=
//{
//	'Surprisingly/B:2-3&2-22#1-16!0-3/D:0_0/E:content+4@1+16&1+13#0+1/F:content_1/G:0_0/H:23=16^1=1/I:0=0/J:23+16-1',
//};

qtk_stress_sntfeat_t* qtk_stress_sntfeat_new(int nwrds, int dim)
{
	qtk_stress_sntfeat_t* sf;
	short i;

	sf=wtk_malloc(sizeof(qtk_stress_sntfeat_t));
	sf->nwrds=nwrds;
	sf->dim=dim;
	sf->wv = wtk_malloc(sizeof(float*) * sf->nwrds);
	for(i=0; i<nwrds; i++)
	{
		sf->wv[i] = wtk_calloc(sf->dim, sizeof(float));
	}

	return sf;
}
void qtk_stress_sntfeat_delete(qtk_stress_sntfeat_t *sf)
{
	short i;

	for(i=0; i<sf->nwrds; i++)
	{
		wtk_free(sf->wv[i]);
	}
	wtk_free(sf->wv);
	wtk_free(sf);
}
/**
 * 关于addpos，使用外部的pos，对于首个或尾部单词，其上个词性或下个词性将不存在，这里直接使用flabel中默认的值
 * 关于重音B:x1 默认flabel生成并不会将0/?->1操作（研究上操作），因此要保持同步，这里将对单词音素没有stress=0时，默认将第一个音素作为lab.(直观上不会有问题)
 * 仅支持单词中最多一个重音音节
 */
qtk_stress_sntfeat_t* qtk_stress_sntfeat_build(char** wrds, int nwrds, int dim, char** labs, int nlab, char** addpos)
{
	qtk_stress_sntfeat_t *sf=NULL;
	float *wf;
	int i, labi, idx, ret=0;
	int posi_phn_insyl,posi_syl_inwrd,isstress, tmpi;
	char *ps, *pe;
	char *tarlab;

	sf=qtk_stress_sntfeat_new(nwrds, dim);
	for (i=0, labi=0; i < nwrds && labi < nlab; i++)
	{
		tarlab=NULL;
		isstress=0;
		idx=0;
		wf=sf->wv[i];
		// forward position of current word in sentence,位置F
		wf[idx++] = i;
		// backward position of current word in sentence,位置B
		wf[idx++] = nwrds-i-1;

		// 以下特征信息可以从flabel中获取，但前提需要做好单词与 flabel 的对应:
		// 通过当前音素是否pau、是否在音节末尾，音节是否在单词末尾，判断单词与 flabel 的对应边界,同时获取单词的需要特征值
		// 注: 这里没有直接通过重音扫描获取，然后跳出，主要考虑单词存在没有重音音节或多个重音音节的情况，从而出问题
		while(labi < nlab)
		{
			// 当前音素是否pau, 跳过
			ps = labs[labi];
			if ((ps=strchr(ps, '-'))!=NULL && (pe=strchr(ps, '+'))!=NULL)
			{
				ps++;
				if (wtk_data_cmp(ps, pe-ps, "pau", 3)==0)
				{
					labi++;
					continue;
				}
			}

			// /B:stress-b-nphn_insyl
			// is stress, 仅对单词的第一个重音作为重音特征考虑
			if ((ps = strchr(pe, 'B'))!=NULL && (ps = strchr(ps, ':'))!=NULL && (pe = strchr(ps, '-'))!=NULL)
			{
				if (NULL==tarlab)
					tarlab=labs[labi];
				ps++;
				tmpi=wtk_str_atoi(ps, pe-ps);
				if (0==isstress && 1==tmpi)
				{
					isstress=1;
					tarlab=labs[labi];
				}
			}

			//是否在音节末尾, if phn in tail of syllable,使用backward posi.
			//phn1^phn2-phn3+phn4@x1_x2/ ->x2
			ps = labs[labi];
			if ((ps = strchr(ps, '_'))!=NULL && (pe = strchr(ps, '/'))!=NULL)
			{
				ps++;
				posi_phn_insyl=wtk_str_atoi(ps, pe-ps);
			}

			//音节是否在单词末尾, if syllable in tail of word,使用backward posi.
			// /B:a-b-c@x1-x2& -> x2
			if ((ps = strchr(pe, '@'))!=NULL && (ps = strchr(ps, '-'))!=NULL && (pe = strchr(ps, '&'))!=NULL)
			{
				ps++;
				posi_syl_inwrd=wtk_str_atoi(ps, pe-ps);
			}

			if (posi_phn_insyl==1 && posi_syl_inwrd==1)
			{
				//单词的最后一个phone/segment片段
				//如果不存在，默认使用最后一个phn-lab
				if (NULL==tarlab)
					tarlab=labs[labi];
				labi++;
				break;
			}

			labi++;
		}

		if (labi >= nlab || NULL==tarlab)
		{
			wtk_debug("error\n");
			ret=-1; goto end;
		}

		// forward position of current stress in word, 当前词重音音节的位置F
		// backward position of current stress in word, 当前词重音音节的位置B
		// forward position of current stress in phrase, 当前词重音音节在当前短语中的位置F
		// backward position of current stress in phrase, 当前词重音音节在当前短语中的位置B
		// number of stress before current stress in phrase, 当前词重音音节在当前短语中之前有多少重音音节  *
		// number of stress behind current stress in phrase, 当前词重音音节在当前短语中之后有多少重音音节  *
		// distance between current stress and prev stress, 当前词重音音节与上一个重音音节之间的距离  *
		// distance between current stress and next stress, 当前词重音音节与下一个重音音节之间的距离  *
		// /B:a-b-c@x1-x2&x3-x4#x5-x6$x7-x8!x9-x10;x11-x12|x13/  x1->x4 x5 x6 x9 x10
		ps=tarlab;
		printf("tarlab=%s\n", tarlab);
		if ((ps = strchr(ps, 'B'))!=NULL && (ps = strchr(ps, '@'))!=NULL && (pe = strchr(ps, '-'))!=NULL)
		{
			ps++;
			wf[idx++] = wtk_str_atoi(ps, pe-ps);
			pe++;
		}
		ps=pe;
		if ((pe = strchr(ps, '&'))!=NULL)
		{
			wf[idx++] = wtk_str_atoi(ps, pe-ps);
			pe++;
		}
		ps=pe;
		if ((pe = strchr(ps, '-'))!=NULL)
		{
			wf[idx++] = wtk_str_atoi(ps, pe-ps);
			pe++;
		}
		ps=pe;
		if ((pe = strchr(ps, '#'))!=NULL)
		{
			wf[idx++] = wtk_str_atoi(ps, pe-ps);
			pe++;
		}
		ps=pe;
		if ((pe = strchr(ps, '-'))!=NULL)
		{
			wf[idx++] = wtk_str_atoi(ps, pe-ps);
			pe++;
		}
		ps=pe;
		if ((pe = strchr(ps, '$'))!=NULL)
		{
			wf[idx++] = wtk_str_atoi(ps, pe-ps);
			pe++;
		}
		ps=pe;
		if ((ps = strchr(ps, '!'))!=NULL && (pe = strchr(ps, '-'))!=NULL)
		{
			ps++;
			wf[idx++] = wtk_str_atoi(ps, pe-ps);
			pe++;
		}
		ps=pe;
		if ((pe = strchr(ps, '-'))!=NULL)
		{
			wf[idx++] = wtk_str_atoi(ps, pe-ps);
			pe++;
		}

		// pos of prev word, 上一个词的词性
		// number of syllable for prev word, 上一个词有多少音节
		// D:x1_x2/
		if ((ps = strchr(pe, 'D'))!=NULL && (ps = strchr(ps, ':'))!=NULL && (pe = strchr(ps, '_'))!=NULL)
		{
			ps++;
			if (i > 0)
			{
				if (addpos)
					wf[idx++] = qtk_nltkpos_getidx2(addpos[i-1], strlen(addpos[i-1]));          //using outer pos.
				else
					wf[idx++] = qtk_nltkpos_getidx2(ps, pe-ps);
			}else
				wf[idx++]=wtk_str_atoi(ps, pe-ps);
			pe++;
		}
		ps=pe;
		if ((pe = strchr(ps, '/'))!=NULL)
		{
			wf[idx++] = wtk_str_atoi(ps, pe-ps);
			pe++;
		}

		// E:x1+x2@x3+x4&x5+x6#x7+x8/  x1 x2
		// pos of current word, 当前词的词性
		if ((ps = strchr(pe, 'E'))!=NULL && (ps = strchr(ps, ':'))!=NULL && (pe = strchr(ps, '+'))!=NULL)
		{
			ps++;
			if(addpos)
				wf[idx++] = qtk_nltkpos_getidx2(addpos[i], strlen(addpos[i]));
			else
				wf[idx++] = qtk_nltkpos_getidx2(ps, pe-ps);
			pe++;
		}
		// number of syllable for current word, 当前词有多少音节
		ps=pe;
		if ((pe = strchr(ps, '@'))!=NULL)
		{
			wf[idx++] = wtk_str_atoi(ps, pe-ps);
			pe++;
		}
		// forward position of current word in phrase, 当前词在当前短语中的位置F
		ps=pe;
		if ((pe = strchr(ps, '+'))!=NULL)
		{
			wf[idx++] = wtk_str_atoi(ps, pe-ps);
			pe++;
		}
		// backward position of current word in phrase, 当前词在当前短语中的位置B
		ps=pe;
		if ((pe = strchr(ps, '&'))!=NULL)
		{
			wf[idx++] = wtk_str_atoi(ps, pe-ps);
			pe++;
		}
		// number of content word before current word in phrase, 当前词在当前短语中之前有多少实义词
		ps=pe;
		if ((pe = strchr(ps, '+'))!=NULL)
		{
			wf[idx++] = wtk_str_atoi(ps, pe-ps);
			pe++;
		}
		// number of content word behind current word in phrase, 当前词在当前短语中之后有多少实义词
		ps=pe;
		if ((pe = strchr(ps, '#'))!=NULL)
		{
			wf[idx++] = wtk_str_atoi(ps, pe-ps);
			pe++;
		}
		// distance between current and prev content word, 当前词到上一个实义词的距离
		ps=pe;
		if ((pe = strchr(ps, '+'))!=NULL)
		{
			wf[idx++] = wtk_str_atoi(ps, pe-ps);
			pe++;
		}
		// distance between current and next content word, 当前词到下一个实义词的距离
		ps=pe;
		if ((pe = strchr(ps, '/'))!=NULL)
		{
			wf[idx++] = wtk_str_atoi(ps, pe-ps);
			pe++;
		}

		// pos of next word, 下一个词的词性                          *
		// number of syllable for next word, 当前词下一个词有多少音节  *
		// F:x1_x2/
		if ((ps = strchr(pe, 'F'))!=NULL && (ps = strchr(ps, ':'))!=NULL && (pe = strchr(ps, '_'))!=NULL)
		{
			ps++;
			if (i < nwrds-1)
			{
				if (addpos)
					wf[idx++] = qtk_nltkpos_getidx2(addpos[i+1], strlen(addpos[i+1]));
				else
					wf[idx++] = qtk_nltkpos_getidx2(ps, pe-ps);
			}else
				wf[idx++] = wtk_str_atoi(ps, pe-ps);
			pe++;
		}
		ps=pe;
		if ((pe = strchr(ps, '/'))!=NULL)
		{
			wf[idx++] = wtk_str_atoi(ps, pe-ps);
			pe++;
		}

		// number of syllable for prev phrase, 当前词所在短语上一个短语有多少音节
		// number of words for prev phrase, 当前词所在短语上一个短语有多少单词
		// G:x1_x2/
		if ((ps = strchr(pe, 'G'))!=NULL && (ps = strchr(ps, ':'))!=NULL && (pe = strchr(ps, '_'))!=NULL)
		{
			ps++;
			wf[idx++] = wtk_str_atoi(ps, pe-ps);
			pe++;
		}
		ps=pe;
		if ((pe = strchr(ps, '/'))!=NULL)
		{
			wf[idx++] = wtk_str_atoi(ps, pe-ps);
			pe++;
		}

		// number of syllable for current phrase, 当前词所在短语有多少音节
		// number of words for current phrase, 当前词所在短语有多少单词
		// forward position of current word in phrase, 当前词处所在短语的位置F
		// backward position of current word in phrase, 当前词处所在短语的位置B
		// H:x1=x2@x3=x4|
		if ((ps = strchr(pe, 'H'))!=NULL && (ps = strchr(ps, ':'))!=NULL && (pe = strchr(ps, '='))!=NULL)
		{
			ps++;
			wf[idx++] = wtk_str_atoi(ps, pe-ps);
			pe++;
		}
		ps=pe;
		if ((pe = strchr(ps, '@'))!=NULL)
		{
			wf[idx++] = wtk_str_atoi(ps, pe-ps);
			pe++;
		}
		ps=pe;
		if ((pe = strchr(ps, '='))!=NULL)
		{
			wf[idx++] = wtk_str_atoi(ps, pe-ps);
			pe++;
		}
		ps=pe;
		if ((pe = strchr(ps, '|'))!=NULL)
		{
			wf[idx++] = wtk_str_atoi(ps, pe-ps);
			pe++;
		}

		//    number of syllables in next phrase, 当前词所在短语下一个短语有多少音节
		//    number of words in next phrase, 当前词所在短语下一个短语有多少单词
		//    I:x1=x2/
		if ((ps = strchr(pe, 'I'))!=NULL && (ps = strchr(ps, ':'))!=NULL && (pe = strchr(ps, '='))!=NULL)
		{
			ps++;
			wf[idx++] = wtk_str_atoi(ps, pe-ps);
			pe++;
		}
		ps=pe;
		if ((pe = strchr(ps, '/'))!=NULL)
		{
			wf[idx++] = wtk_str_atoi(ps, pe-ps);
			pe++;
		}
		// number of syllables in sentence, 当前词所在句子有多少音节
		// number of words in sentence, 当前词所在句子有多少单词
		// number of phrase in sentence, 当前词所在句子有多少短语
	    // J:x1+x2-x3
		if ((ps = strchr(pe, 'J'))!=NULL && (ps = strchr(ps, ':'))!=NULL && (pe = strchr(ps, '+'))!=NULL)
		{
			ps++;
			wf[idx++] = wtk_str_atoi(ps, pe-ps);
			pe++;
		}
		ps=pe;
		if ((pe = strchr(ps, '-'))!=NULL)
		{
			wf[idx++] = wtk_str_atoi(ps, pe-ps);
			pe++;
		}
		ps=pe;
		wf[idx++] = wtk_str_atoi(ps, tarlab+strlen(tarlab)-ps);
	}

end:
	if (ret!=0)
	{
		qtk_stress_sntfeat_delete(sf);
		sf=NULL;
	}
	return sf;
}

int qtk_stress_load(void *ths,wtk_source_t *src)
{
	float *thres, *values;
	short *feats;
	unsigned short nnode,nclass, *samples;
	int ret=0;
	qtk_dtree_t *dt;

	dt=(qtk_dtree_t*)ths;
	//default res file is little_endian, don't swap
	if (src->swap > 0)
		src->swap = 0;
	ret=wtk_source_read_ushort(src,&nnode,1,1);
	if(ret!=0){goto end;}
	//printf("num of nodes: %d\n", nnode);

	feats = wtk_malloc(sizeof(short) * nnode);
	ret=wtk_source_read_short(src,feats,nnode,1);
	if(ret!=0){goto end;}
	//print_short(feats, nnode);

	samples = wtk_malloc(sizeof(unsigned short) * nnode);
	ret=wtk_source_read_ushort(src,samples,nnode,1);
	if(ret!=0){goto end;}
	//print_short(samples, nnode);

	thres = wtk_malloc(sizeof(float) * nnode);
	ret=wtk_source_read_float(src,thres,nnode,1);
	if(ret!=0){goto end;}
	//print_float(thres, nnode);

	ret=wtk_source_read_ushort(src,&nclass,1,1);
	if(ret!=0){goto end;}

	values = wtk_malloc(sizeof(float) * nnode * nclass);
	ret=wtk_source_read_float(src,values,nnode * nclass,1);
	if(ret!=0){goto end;}
	// print_float(values, nnode*nclass);
	// build tree with DLA
	dt=qtk_dtree_build(dt, feats, samples, thres, values, nclass, nnode);
	//qtk_dtree_print(dt);
	//qtk_dtree_print_layer(dt);
	wtk_free(feats);
	wtk_free(samples);
	wtk_free(thres);
	wtk_free(values);
end:
	return ret;
}
