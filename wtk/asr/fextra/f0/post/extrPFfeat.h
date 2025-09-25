/* 
 * File:   getPFfeat.h
 * Author: hlw74
 *
 * Created on April 13, 2010, 11:55 AM
 */

#ifndef _GETPFFEAT_H
#define	_GETPFFEAT_H

#ifdef	__cplusplus
extern "C" {
#endif

 #include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#ifdef WIN32
#include <limits.h>
#define INT32_MAX  INT_MAX   
#else
#include <stdint.h>
#endif
#include "wtk/core/math/wtk_math.h"

    
typedef struct _ToneFeat{
        int nFrm;
	float *f0;
	float *engy;
}ToneFeat;

typedef struct _F0ProcPara{
    float minEngThd;
    float plsFilterThd;
    int minVoiceLen;
    int minUnvoiceLen;
    float engRatioThd;
    int sylNum;
    int doLogNrm;
}F0ProcPara;

typedef struct{
   int nSamples;
   int nSampPeriod;
   short nSampSize;
   short nParmKind;
}HTKHead;

typedef struct _dLink{
	float value;
	int idx;   // idx of the point in the original data array, also used to save windowSize in mHead
	struct _dLink *next;
}dLink;

typedef struct _Poly2Stat{
	float sqy;         // f0 value square:    \sum_{i}{y_i}
	float vecxy[3];    // vecx = [x^2 x 1]:  \sum_{i}{y_i*vecx_i}
	float matx[5];     // stat 4 symetric matrix: [x^4 x^3 x^2 x 1]   \sum_{i}{vecx_i*vecx_i^T}
} Poly2Stat;

typedef struct _PolyFeat{
	float a[3];   // coefficients of 2nd polynomial: y=a[0]*x^2+a[1]*x+a[2]
	float logres;    // residue after poly fit
} PolyFeat;

typedef struct _VoiceSeg{
	int  st;
	int  end;
	int  dur;
    float  ave_engy;
    float  ave_f0;
struct _VoiceSeg *next;
    PolyFeat PFfeat;
} VoiceSeg;


ToneFeat *loadF0(char *inF0fn);
int getFileLineNum(FILE *fp);

void SmoothF0(ToneFeat *tFeat,VoiceSeg *seghead, F0ProcPara F0ProcPrm);
void ExtractPolyFea(wtk_heap_t *heap,VoiceSeg *seghead, ToneFeat *tFeat, int doLogNrm);
VoiceSeg *ExtractPolyFitFeat(wtk_heap_t *heap,ToneFeat *tFeat,int nTone, F0ProcPara F0ProcPrm);

/*--------------------------------------------------------------------------------*/
/*              create voiced segment                              */
/*-------------------------------------------------------------------------------*/
/*create struct to save voice segmentation information*/
VoiceSeg *NewVoiceSeg(void);

/*create stable voice segmentation struct*/
VoiceSeg * CreateVoiceSeg(ToneFeat *tFeat,int nTone,F0ProcPara F0ProcPrm);

/* Get voiced segments, normally should be 1 but sometimes can be over 1 */
VoiceSeg* GetVoiceSeg(ToneFeat *tFeat, float fMinEngy);

/*merge two voice segmentations using linear  interpolation*/
VoiceSeg * MergeVoiceSeg(VoiceSeg *prev, VoiceSeg *follow, ToneFeat *tFeat);

/*merge two voice segmentation with the shortest interval/gap until the number of voice segmentation is less than or equal with the syllable number*/
void MergeNearVoiceSeg(VoiceSeg *seghead, ToneFeat *tFeat, int tgtNum, int nUvMergeThd);
       
/*discard these segmentation whose average energy is lower than the energy of master segmentation */
void DiscardLowAveEngyRegion(VoiceSeg *seghead, ToneFeat *tFeat, float fEngyRationThd);

/*discard these isolated short voiced segments whose length is lower than the given threshold */
void DiscardShortSeg(VoiceSeg *seghead, ToneFeat *tFeat, int nTone, int nMinVoiceDur);

/* Delete the next voice seg structure */
void DelNextVoiceSeg(VoiceSeg *curseg, ToneFeat *tFeat);

/* Recursively free all the links after oldseg (incl. oldseg) */
void FreeVoiceSeg(VoiceSeg *oldseg);

/*----------------------------------------------------------------------------------------*/
/*                                Smoothing F0                                 */
/*----------------------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------------------*/
void swap_float_ptr(void* mfcc_ptr,int size);
void Swap32 (int* Long );
void Swap16 ( short *Short );

#ifdef	__cplusplus
}
#endif

#endif	/* _GETPFFEAT_H */

