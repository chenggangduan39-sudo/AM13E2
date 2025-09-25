#ifndef __SPLITF02_H__
#define __SPLITF02_H__
#ifdef __cplusplus
extern "C" {
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef struct _dLink2{
	double value;
	int idx;   // idx of the point in the original data array, also used to save windowSize in mHead
	struct _dLink2 *next;
}dLink2;

typedef struct _VoiceSeg2{
	int  st;
	int  end;
	int  dur;
    float  ave_engy;
    float  ave_f0;
	struct _VoiceSeg2 *next;
} VoiceSeg2;

/* find voiced segments */
VoiceSeg2* GetVoiceSegFromF02(double* f0, double* engy, int T, double minE);

void DiscardLowAveEngyRegion2(VoiceSeg2 *seghead, double *f0, double *engy, double fEngyRationThd);

void MergeShortVoiceRegion2(VoiceSeg2 *seghead, double *f0, double *engy, int n, int nMinVoiceDur, int nUvMergeThd);

/* smooth derivative and then smooth static again, only smooth non-zero region */
void SmoothF02(double *df0, double *f0, int T, VoiceSeg2 *seghead, int nPlsFltThd);

int NormF0Engy2(double *f0, double *engy, int T, VoiceSeg2 *seghead);

void FreeVoiceSeg2(VoiceSeg2 *oldseg);
#ifdef __cplusplus
}
#endif

#endif // __SPLITF0_H__
/* ------------------------------- end of split.h ------------------------------ */
