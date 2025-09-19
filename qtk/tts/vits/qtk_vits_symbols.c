/*
 * qtk_vits_symbols.c
 *
 *  Created on: Aug 26, 2022
 *      Author: dm
 */
#include "qtk_vits_symbols.h"
// 中文音素表
// 声母：27
//char* shengmu[] = {"b", "p", "m", "f", "d", "t", "n", "l", "g", "k",
//				   "h", "j", "q", "x", "zh", "ch", "sh", "r", "z",
//				   "c", "s", "y", "w"};
//
//char* yunmu[] = {"a", "A", "o", "O", "e", "E", "i", "I", "u", "U", "v", "VV", "ii", "iii",
//          "er", "Er", "ei", "Ei", "ai", "Ai", "ou", "Ou", "ua", "Ua", "ia", "Ia",
//          "ve", "Ve", "ie", "Ie", "ao", "Ao", "uo", "Uo",
//          "iao", "Iao", "iou", "Iou", "io", "Io", "uei", "Uei", "uai", "Uai",
//          "vn", "Vn", "uen", "Uen", "in", "In", "en", "En", "van", "Van", "uan", "Uan",
//          "ian", "Ian", "an", "An", "iong", "Iong", "ong", "ueng", "Ueng", "ing", "Ing",
//          "eng", "Eng", "uang", "Uang", "iang", "Iang", "ang", "Ang"};
//
//char* shengdiao[] = {"1", "2", "3", "4", "5"};
//
//char* prosody[] = {"@", "#", "$"};
//
//char* word_bound[] = {"#S"};
//
//char* sil_phoneme[] = {"SIL", "sil", "sp1"};
//
//char* other_symb={"bos", "eos"};
//
//
//char** qtk_vits_symbols_new()
//{
//	qtk_vits_symbols_t* symbols;
//	int n, n2, i, k;
//
//	n = 0;
//
//	n += sizeof(shengmu)/sizeof(shengmu[0]);
//	n += sizeof(yunmu)/sizeof(yunmu[0]);
//	n += sizeof(shengdiao)/sizeof(shengdiao[0]);
//	n += sizeof(prosody)/sizeof(prosody[0]);
//	n += sizeof(word_bound)/sizeof(word_bound[0]);
//	n += sizeof(sil_phoneme)/sizeof(sil_phoneme[0]);
//	n += sizeof(other_symb)/sizeof(other_symb[0]);
//
//	symbols->symbols=(char**)wtk_malloc(sizeof(char*) * n);
//	k=0;
//	n2 = sizeof(shengmu)/sizeof(shengmu[0]);
//	for(i=0; i<n2; i++,k++)
//	{
//		symbols->symbols[k]=shengmu[i];
//	}
//	n2 = sizeof(yunmu)/sizeof(yunmu[0]);
//	for(i=0; i<n2; i++,k++)
//	{
//		symbols->symbols[k]=yunmu[i];
//	}
//	n2 = sizeof(shengdiao)/sizeof(shengdiao[0]);
//	for(i=0; i<n2; i++,k++)
//	{
//		symbols->symbols[k]=shengdiao[i];
//	}
//	n2 = sizeof(prosody)/sizeof(prosody[0]);
//	for(i=0; i<n2; i++,k++)
//	{
//		symbols->symbols[k]=prosody[i];
//	}
//	n2 = sizeof(word_bound)/sizeof(word_bound[0]);
//	for(i=0; i<n2; i++,k++)
//	{
//		symbols->symbols[k]=word_bound[i];
//	}
//	n2 = sizeof(sil_phoneme)/sizeof(sil_phoneme[0]);
//	for(i=0; i<n2; i++,k++)
//	{
//		symbols->symbols[k]=sil_phoneme[i];
//	}
//	n2 = sizeof(other_symb)/sizeof(other_symb[0]);
//	for(i=0; i<n2; i++,k++)
//	{
//		symbols->symbols[k]=other_symb[i];
//	}
//
//	return symbols;
//}
