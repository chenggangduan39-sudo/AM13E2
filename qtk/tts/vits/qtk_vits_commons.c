/*
 * qtk_vits_commons.c
 *
 *  Created on: Sep 2, 2022
 *      Author: dm
 */

#include "qtk_vits_commons.h"

int qtk_vits_comm_seq_mask(int len, int max_len, int* v)
{
	int i;

	for(i=0; i < len; i++)
	{
		if(i < max_len)
			*(v+i) = 1;
		else
			*(v+i) = 0;
	}

	return 0;
}


