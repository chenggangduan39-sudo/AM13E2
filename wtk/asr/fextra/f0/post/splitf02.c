#include "splitf02.h"
#include "stdio.h"
#define exit(x) return
/* parameters */

#define MINF0  50    // minimum f0
#define MINENGY 200   // minimum engy
#define MINVOICEDUR 10 // 1 point = 5ms, so the dur at least 75ms,
#define UVMERGETHRD 15 // 1 point = 5ms, so the gap is at most 50ms,
#define PULSETHRESHOD 10  // if abs diff > PULSETHRESHOD, consider as pulse
#define ENGYRATIOTHRD 0.2 //energy percentage ratio
#define VcdGblMean 9.992222e-01  //global mean of voiced segment for generating noise in unvoiced segment
#define VcdGblVar 3.979555e-02   //global variance of voiced segment for generating noise in unvoiced segment
#define NEGTIVEINFINITE -1.0e+10  //define negtive infinite

/*
#define MINF0  50    // minimum f0
#define MINENGY 100   // minimum engy
#define MINVOICEDUR 10 // 1 point = 5ms, so the dur at least 75ms,
#define UVMERGETHRD 5 // 1 point = 5ms, so the gap is at most 50ms,
#define PULSETHRESHOD 10  // if abs diff > PULSETHRESHOD, consider as pulse
#define ENGYRATIOTHRD 0.2 //energy percentage ratio
#define VcdGblMean 9.992222e-01  //global mean of voiced segment for generating noise in unvoiced segment
#define VcdGblVar 3.979555e-02   //global variance of voiced segment for generating noise in unvoiced segment
#define NEGTIVEINFINITE -1.0e+10  //define negtive infinite
*/

typedef struct{
   int nSamples;
   int nSampPeriod;
   short nSampSize;
   short nParmKind;
}HTKHead;

typedef enum
{
	FALSE=0,
	TRUE=1,
}Boolean;

/* -------------------------------------------------------- */
/*                 Filter for Smoothing                     */
/* -------------------------------------------------------- */

/* ---------------- Average Filter ------------------------ */
/* calulate average for points from stidx to endidx with windowSize neighbouring points */
static void AveFilter(double *data, int stidx, int endidx, int windowSize)
{
	double *tmp, tmpvalue;
	int hfWnd, i, j;

    if(endidx-stidx<5) return;
	if(windowSize<2 || windowSize>100){
		fprintf(stderr, "Wrong window size %d\n",windowSize);
		exit(1);
	}
	hfWnd = (int)(windowSize/2);

	/* get tmp array, extend boundary using edge values */
	tmp = (double *)malloc((endidx-stidx+1+windowSize)*sizeof(double));
	memcpy(tmp+hfWnd,data+stidx,(endidx-stidx+1)*sizeof(double));
	for(i=0;i<hfWnd;i++) tmp[i] = data[stidx];
	for(i=endidx-stidx+hfWnd+1;i<=endidx-stidx+windowSize;i++) tmp[i] = data[endidx];

	/* do smoothing */
	for(i=0;i<endidx-stidx+1;i++){
		tmpvalue = 0.0;
		for(j=0;j<windowSize;j++) tmpvalue += tmp[i+j];
		data[stidx+i] = tmpvalue/(double)windowSize;
	}

	free(tmp);

	return;

}

/* -----------------  Median Filter ----------------- */
/* Median filter implemented using link list */
static dLink2 *InitialiseMedianFilter(dLink2 *head, double *data, int stidx)
{

	int i, size;
	dLink2 *ptrTmp, *ptr;

	size = head->idx;

	/* initialise */
	for(i=stidx;i<stidx+size;i++){
		ptrTmp = (dLink2 *)malloc(sizeof(dLink2));
		ptrTmp->value = data[i];
		ptrTmp->idx = i;
		/* find the position to insert, ascending order */
		ptr = head;
		while(ptr->next && ptr->next->value < ptrTmp->value)
			ptr = ptr->next;
		ptrTmp->next = ptr->next;
		ptr->next = ptrTmp;
	}

	/* find median */
	size = (int)(size/2);
	ptr = head->next;
	for(i=0;i<size;i++)
		ptr = ptr->next;

	return ptr;
}

static dLink2 *UpdateMedianFilter(dLink2 *head, int delIdx, int addIdx, double *data)
{

	dLink2 *ptr, *takeOut;
	int i, hfWnd;

	/* take out link with delIdx */
	ptr = head;
	while(ptr->next && ptr->next->idx != delIdx)
		ptr = ptr->next;
	if(!ptr->next){
		fprintf(stderr, "Error: idx %d not in median filter",delIdx);
		//exit(1);
		return 0;
	}
	takeOut = ptr->next;
	ptr->next = ptr->next->next;

	/* add the new data in - median filter in ascending order */
	takeOut->idx = addIdx;
	takeOut->value = data[addIdx];
	ptr = head;
	while(ptr->next && ptr->next->value < takeOut->value)
		ptr = ptr->next;
	takeOut->next = ptr->next;
	ptr->next = takeOut;

	/* find median */
	hfWnd = (int)(head->idx/2); // head->idx is actually windowSize
	ptr = head->next;
	for(i=0;i<hfWnd;i++)
		ptr = ptr->next;

	return ptr;

}

static void FreeMedianFilter(dLink2 *head)
{
	dLink2 *ptr, *toFree;
	int size, i;

	ptr = head;
	size = head->idx;
	i = 0;
	while(ptr){
		toFree = ptr;
		ptr = ptr->next;
		free(toFree);
		i++;
	}

	if(i!=size+1){
		fprintf(stderr, "Error: free %d elements, but size+head is %d\n",i,size+1);
	}

	return;
}

/* calulate median value for points from stidx to endidx with windowSize neighbouring points */
static void MedianFilter(double *data, int stidx, int endidx, int windowSize)
{
	double *tmp;
	dLink2 *mHead, *mMedian;
	int hfWnd, i, j;

    if(endidx-stidx<5) return;
	if(windowSize<2 || windowSize>100){
		fprintf(stderr, "Wrong window size %d in segment %d to %d\n",windowSize,stidx,endidx);
		exit(1);
	}
	if(windowSize>endidx-stidx+1) return;
	i = windowSize%2;
	if(i==0){
		fprintf(stderr, "Wrong median filter window size %d. Must be odd!\n",windowSize);
		exit(1);
	}
	hfWnd = (int)(windowSize/2);

	/* get tmp array, only smooth those points away from boundaries */
	/*                                                             */
	/* |--- hfWnd ---|---- data to be filtered ----|--- hfWnd ---| */
	/*                                                             */
	tmp = (double *)malloc((endidx-stidx+2-windowSize)*sizeof(double));
	mHead = (dLink2 *)malloc(sizeof(dLink2));
	mHead->idx = windowSize;
	mHead->value = -1000.0;
	mHead->next = NULL;

	/* do median smoothing */
	mMedian = InitialiseMedianFilter(mHead,data,stidx);
	tmp[0] = mMedian->value;
	for(i=stidx+hfWnd+1;i<endidx-hfWnd+1;i++){
		mMedian = UpdateMedianFilter(mHead,i-hfWnd-1,i+hfWnd,data); 
		tmp[i-stidx-hfWnd] = mMedian->value;
	}

	/* copy boundary values to head and tail points and update data */
	for(i=stidx;i<=endidx;i++){
		j = i-stidx-hfWnd;
		if( j < 0 )
			data[i] = tmp[0];
		else if ( j > endidx-stidx+1-windowSize )
			data[i] = tmp[endidx-stidx+1-windowSize];
		else
            data[i] = tmp[j]; 
	}

	FreeMedianFilter(mHead);

	free(tmp);

	return;

}


/* ---------------- Pulse Filter ------------------------ */
/* remove pulses for points from stidx to endidx.
   For the points whose abs gradient value is equal or greater than PULSETHRESHOD, use the
   average of the neighbourhood points whose abs gradient value are less than PULSETHRESHOD.
   The actual widowSize is the number of points to do average */
static void PulseFilter(double *data, int stidx, int endidx, int windowSize, int nPlsFltThd)
{
	double tmpvalue, avediff;
	double *tmp;
	int hfWnd, i, cnt, numPoints;
              
    if(endidx-stidx<5) return;
	if(windowSize<2 || windowSize>100){
		fprintf(stderr, "Wrong window size %d in segment %d to %d\n",windowSize,stidx,endidx);
		exit(1);
	}
	if(windowSize>endidx-stidx+1) return;
	i = windowSize%2;
	if(i==0){
		fprintf(stderr, "Wrong median filter window size %d. Must be odd!\n",windowSize);
		exit(1);
	}
	hfWnd = (int)(windowSize/2);

    /* reserve value and calculate ave gradient based on reliable points */
    tmp = (double *)malloc((endidx-stidx+1)*sizeof(double));
    avediff = 0.0;numPoints = 0;
    for(i=stidx;i<=endidx;i++){
    	tmp[i-stidx] = data[i];
        if(fabs(data[i]) <= nPlsFltThd){ // get average value of small values
    	    avediff += data[i];
            numPoints++;
        }
    }
	if(numPoints>0) //add by hlwang, otherwis, it seems there are some expection to happen in the future because numPoints may be zero by hlw.
		avediff /= (double)numPoints; 

	/* detect pulse and do average for pulse point */
    for(i=stidx;i<=endidx;i++){
    	if( fabs(data[i]) >= nPlsFltThd ){
        	tmpvalue = 0.0; 
            /* find the neighbouring windowSize points and do average */
            /* 1. search left side within range windowSize */
            cnt = i-1; numPoints = 0;
            while(cnt >= stidx && cnt > i-windowSize && numPoints < hfWnd){
            //while(cnt >= stidx && cnt > i-windowSize && numPoints < windowSize){            
        	    /* look for non-pulse points */
                if( fabs(data[cnt]) < nPlsFltThd/2 ){ // use a more strict criterion to select smoothing points
        	        tmpvalue += data[cnt];
                    numPoints++;
                }
                cnt--;
            }
            /* 2. search right side within range windowSize */
            cnt = i+1;
            while(cnt <= endidx && cnt < i+windowSize && numPoints<windowSize){
        	    /* look for non-pulse points */
                if( fabs(data[cnt]) < nPlsFltThd/2 ){
        	        tmpvalue += data[cnt];
        	        numPoints++;
                }
                cnt++;
            }
            /* 3. calculate average */
            if(numPoints == 0){
            	printf("Warning: no non-pulse neighbouring points at position %d\n",i );
                tmp[i-stidx] = avediff;
				//data[i] = avediff;
            } else if (numPoints < 2){
            	tmp[i-stidx] = avediff;
				//data[i] = avediff;
            } else
            	tmp[i-stidx] = tmpvalue/(double)numPoints;
				//data[i] = tmpvalue/(double)numPoints;
		}
	}

	//add by hlw, to reduce the influence of pre-position
    for(i=stidx;i<=endidx;i++){
		data[i]=tmp[i-stidx];
	}
   	free(tmp);

	return;
}


/* -------------------------------------------------------- */
/*           Link operation for voice segment               */
/* -------------------------------------------------------- */

VoiceSeg2 * CreateVoiceSeg2(void)
{
	VoiceSeg2 *newseg;

	newseg = (VoiceSeg2 *)malloc(sizeof(VoiceSeg2));
	newseg->st = 0;
	newseg->end = 0;
	newseg->dur = 0;
        newseg->ave_f0 = 0;
        newseg->ave_engy = 0.0;
	newseg->next = NULL;
	return newseg;
}

/* Recursively free all the links after oldseg (incl. oldseg) */
void FreeVoiceSeg2(VoiceSeg2 *oldseg)
{
	VoiceSeg2* current = oldseg;
	VoiceSeg2* next = current->next;
	while (current->next != NULL)
	{
		free(current);
		current = next;
		next = current->next;
	}
	free(current);

	return;
}

/* Get voiced segments, normally should be 1 but sometimes can be over 1 */
VoiceSeg2* GetVoiceSegFromF02(double* f0, double* engy, int T, double fMinEngy)
{
   VoiceSeg2 *seghead, *segcur;
   Boolean voiced;
   int t, nSeg, min0;

   /* floor silence first */
   min0 = 32700; /* just a big number */
   /* find the minimum non-zero value */
   for(t=0;t<T;t++){
      if(engy[t]>0 && min0>engy[t]) min0 = (int)engy[t];
   }
   /* calibrate energy and floor sil first */
   for(t=0;t<T;t++)
      engy[t] -= min0;


   /* preprocessing: remove head and tail f0 values with f0<MINF0 or engy<MINENGY */
   for(t=0;t<T;t++){
      if(f0[t]<MINF0 || engy[t]<fMinEngy)
         f0[t] = 0.0;
      else
         break;
   }
   for(t=T-1;t>0;t--){
      if(f0[t]<MINF0 || engy[t]<fMinEngy)
         f0[t] = 0.0;
      else
         break;
   }

   /* preprocessing: remove the region of f0 values with f0<MINF0 or engy<MINENGY */
   /*for(t=0;t<T;t++){
      if(f0[t]<MINF0 || engy[t]<fMinEngy)
         f0[t] = 0.0;
   }*/

   /* find remained voiced segments */
   seghead = CreateVoiceSeg2();

   /* get boundary of each voiced/uv region, always uv|v|uv|v... */
   nSeg = 0; segcur = seghead; voiced = FALSE;
   for (t = 0; t < T; t++) {
      if (f0[t] > 0 && !voiced) {
         nSeg++;
         segcur->next = CreateVoiceSeg2(); // add new seg and jump to the new seg
         segcur = segcur->next; 
         segcur->st = t;
         voiced = TRUE;
      } else if (f0[t] == 0 && voiced){
         segcur->end = t - 1;
         segcur->dur = segcur->end - segcur->st + 1;
         if( segcur->dur < 2 )
            printf("Error: too short seg %d\n",segcur->dur);
         voiced = FALSE;
      }
   }

   if(voiced) {
      segcur->end = T - 1;
      segcur->dur = segcur->end - segcur->st + 1;
   } 
   seghead->dur = nSeg;  /* steal a space to save the number of segs */

   return seghead;
} 


/* Delete the next voice seg structure */
void DelNextVoiceSeg2(VoiceSeg2 *curseg, double *f0)
{

	VoiceSeg2 *tmp;
	int i=0;
	if(!curseg->next){
		fprintf(stderr, "Error: can not delete null\n");
		exit(1);
	}
	tmp = curseg->next;

	//discard the short voiced segment and set the value of f0 as zero, add by hlw at 2009.5.4
	for(i=tmp->st;i<=tmp->end;i++)
		f0[i]=0.0;

	curseg->next = curseg->next->next;

	free(tmp);

	return;
}


void DiscardLowAveEngyRegion2(VoiceSeg2 *seghead, double *f0, double *engy, double fEngyRationThd)
{
   VoiceSeg2 *ptr;
   double master_engy=0,tmp_engy;
   int t, maxdur;

   /* calculate average energy for each voiced region and find the master engery */
   /* master engery is the average energy of the longest voiced segment */
   maxdur = 0;
   for(ptr=seghead->next;ptr;ptr=ptr->next){
      tmp_engy = 0.0;
      for(t=ptr->st;t<=ptr->end;t++) tmp_engy += engy[t];
      ptr->ave_engy = (float)tmp_engy/(float)(ptr->dur);
      if(maxdur < ptr->dur){
         maxdur = ptr->dur;
         master_engy = ptr->ave_engy;
      }
   }

   /* discard low average energy regions */
   for(ptr=seghead;ptr && ptr->next;){
      /* ptr->next is to be checked and deleted */
      if(ptr->next->ave_engy < fEngyRationThd*master_engy){
         for(t=ptr->next->st;t<=ptr->next->end;t++) f0[t] = 0.0;
         DelNextVoiceSeg2(ptr, f0);
         seghead->dur--;
      }
       else
        ptr=ptr->next;
   }

   return;
}

VoiceSeg2 * MergeVoiceSeg2(VoiceSeg2 *prev, VoiceSeg2 *follow, double *f0, double *engy)
{

	double scope, tmp_engy;
	int t;

	// interpolate f0 values - linear 
	scope = (f0[follow->st] - f0[prev->end])/(double)(follow->st-prev->end);
	for(t=prev->end+1;t<follow->st;t++)
		f0[t] = f0[prev->end] + scope*(t-prev->end);

	// merge voice seg structure and save in *prev
	tmp_engy = 0.0;
	for(t=prev->end+1;t<follow->st;t++) tmp_engy += engy[t];
	tmp_engy += prev->dur*prev->ave_engy;
	tmp_engy += follow->dur*follow->ave_engy;
	prev->ave_engy = (float)tmp_engy/(float)(follow->end-prev->st+1);
	prev->next = follow->next;
	prev->end = follow->end;
	prev->dur = follow->end - prev->st + 1;

	// release follow
	free(follow);

	return prev;
}


void MergeShortVoiceRegion2(VoiceSeg2 *seghead, double *f0, double *engy, int tgtNum, int nMinVoiceDur, int nUvMergeThd)
{

   VoiceSeg2 *ptr,*pre_ptr, *min_ptr;
   int  dur_nuv, mindur;

   /* delete isolate short voiced segments first */
   for(ptr=seghead;ptr->next;){
      if(ptr->next->dur < nMinVoiceDur){
         DelNextVoiceSeg2(ptr, f0);
         seghead->dur--;
      } else
         ptr = ptr->next;
   }


   
   /* merge segments if there are still too many voiced segs */
	//tgtNum = 1;
    while( seghead->dur > tgtNum ){
		/*find minmum duration	between two voiced segs*/		
		mindur=100000000;
		min_ptr=NULL;
		for(ptr=seghead->next->next,pre_ptr=seghead->next;ptr;ptr=ptr->next,pre_ptr=pre_ptr->next){
			dur_nuv = ptr->st - pre_ptr->end;
			if(mindur>dur_nuv){
				mindur=dur_nuv;
				min_ptr=pre_ptr;
			}
		}
		/*merge the two voiced segs*/
		if(min_ptr){
			min_ptr = MergeVoiceSeg2(min_ptr,min_ptr->next,f0,engy);
           	seghead->dur--;
		}			
	}  

   /* merge these closest segments */
   for(ptr=seghead->next,pre_ptr=seghead;ptr;ptr=ptr->next,pre_ptr=pre_ptr->next){
       
         /* possible candidate to merge, check the previous/next unvoiced seg */
         /* merge this segment with the neighbour if it is close to any of them */
         /* previous unvoiced seg dur */
         //if(pre_ptr != seghead) dur_puv = ptr->st - pre_ptr->end;
         //else dur_puv = nUvMergeThd + 1;
         /* next unvoiced seg dur */
         if(ptr->next) dur_nuv = ptr->next->st - ptr->end-1;
         else dur_nuv = nUvMergeThd + 1;

         /* unvoiced seg dur must be greater than UVMERGETHRD */
         //if(dur_puv <= nUvMergeThd){
         //   ptr = MergeVoiceSeg2(pre_ptr,ptr,f0,engy);
         //   seghead->dur--;
         //}
         if(dur_nuv <= nUvMergeThd){
            ptr = MergeVoiceSeg2(ptr,ptr->next,f0,engy);
            seghead->dur--;
         }         
   }

   return;
}


/* -------------------------------------------------------- */
/*          Smooth and Normalisation                        */
/* -------------------------------------------------------- */

/* smooth derivative and then smooth static again, only smooth voiced segments */
/* modify boundary if necessary */
void SmoothF02(double *df0, double *f0, int T, VoiceSeg2 *seghead, int nPlsFltThd)
{
   	VoiceSeg2 *segcur;
   	int t;
	double lastsum=0, nowsum=0;

   /* get differential of F0, df0(t) = f0(t+1) - f0(t) */
   /* note that the last elem does not have df0 value */
   for(t=0;t<T;t++)
      df0[t] = 0.0;

   /* smooth f0 in the voiced segment */
   segcur = seghead;
   for(segcur = seghead->next;segcur;segcur = segcur->next){
      // do median average first to smooth the curve
      MedianFilter(f0,segcur->st,segcur->end,5);
      for(t=segcur->st;t<segcur->end;t++)
         df0[t] = f0[t+1] - f0[t];
   }
   //---------------------------------------------------------------------------
   //find the most stable segment 
   //for(segcur = seghead->next;segcur;segcur = segcur->next){
   //   for(t=segcur->st;t<segcur->end;t++)
   //      df0[t] = f0[t+1] - f0[t];
   //}	

   /*for(t=0;t<T;t++)
      printf("org:%f\n",df0[t]);*/

   /* smooth df0 first: use 5 neighborhood points to do median and average */
   for(segcur = seghead->next;segcur;segcur = segcur->next){
      /* remove pulses */
      PulseFilter(df0,segcur->st,segcur->end-1,5, nPlsFltThd);
   	  /*for(t=0;t<T;t++)
      printf("pul:%f\n",df0[t]);*/
      /* further smooth derivatives */
      AveFilter(df0,segcur->st,segcur->end-1,5);
   }
   /*for(t=0;t<T;t++)
      printf("ave:%f\n",df0[t]);
	*/
   /* resume f0 from smoothed df0 and smooth f0 again using 3 neighbouring points */
   	for(segcur = seghead->next;segcur;segcur = segcur->next){
		
		//computer original F0 average
		lastsum=0.0;      	
		for(t=segcur->st;t<=segcur->end;t++)
			lastsum=lastsum+f0[t];

      	// Taking 1st point as the baseline is not reliable, this should be modified later!
		nowsum=f0[segcur->st];   
		for(t=segcur->st+1;t<=segcur->end;t++){
			f0[t] = f0[t-1] + df0[t-1];
			nowsum=nowsum+f0[t];
		}

		//parallelly move F0 to original position
		lastsum=(lastsum-nowsum)/(segcur->end-segcur->st+1);
		for(t=segcur->st;t<=segcur->end;t++)
			f0[t]=f0[t]+lastsum;
	  	//
      	AveFilter(f0,segcur->st,segcur->end,3);
   	}

}

//void NormF0Engy(double *f0, double *engy, double *inf0bak, int T, VoiceSeg2 *seghead)
int NormF0Engy2(double *f0, double *engy, int T, VoiceSeg2 *seghead)
{
   	VoiceSeg2 *segcur;
   	double ave_f0, ave_engy;
   	int t, nSum;
	int sig=0, sig1;


   	nSum = 0;
	ave_f0 = 0.0;
	ave_engy = 0.0;

	/*take the logarithm for F0 and Energy*/
	for(t=0;t<T;t++)
		if(f0[t]< 0.0){
			sig=1;
			break;
		}
   	for(t=0;t<T;t++){
		sig1=(f0[t]< 0.0)?-1:1;
		if(f0[t]!=0.0)
			f0[t]=sig1*log(fabs(f0[t]));
		if(engy[t]>=1)
			engy[t]=log(engy[t]);
		else
			engy[t]=0.0;
       	
	}
	
	/*for(t=0;t<T;t++){
		if(engy[t]>0){
         		ave_engy += engy[t];
			m++;
		}
	}
	ave_engy=ave_engy/m;*/
   	/* calculate average f0 and engy of voiced segments */
   	for(segcur = seghead->next;segcur;segcur = segcur->next){
      		for(t=segcur->st;t<segcur->end;t++)
      		{
         		ave_f0 += f0[t];
         		ave_engy += engy[t];
         		nSum++;
         	}
       		//nSum=nSum+segcur->end-segcur->st+1;
   	}
   	ave_f0 = ave_f0/nSum;
	ave_engy=ave_engy/nSum;
   	/* normalise by dividing the average (can be done in LOG domain later) */
   	for(t=0;t<T;t++){
		if(f0[t]!=0.0)
      		f0[t] -= ave_f0;
		else
			f0[t]=NEGTIVEINFINITE;
      	engy[t] -= ave_engy;
   	}

   	return sig;


}



/* ------------------------------- end of split.c ------------------------------ */
