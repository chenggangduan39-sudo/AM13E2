#include "extrPFfeat.h"
#define MINSIL  20   // unit: energy density
#define ENGYRATIOTHRD 0.2F
#define MINVOICEDUR 10 // 1 point = 5ms, so the dur at least 50ms, here for chars/words
#define UVMERGETHRD 10 // 1 point = 5ms, so the gap is at most 50ms, here for chars/words
#define MAXENGYPEAK 5 // maximum number of peaks in a voiced region
#define INTERVALSCALE 0.18F // given 1 point = 5ms, normal duration is 50-60, scale to 10
#define PULSETHRESHOD 10  // if abs diff > PULSETHRESHOD, consider as pulse
#define DEWEIGHTINGFACTOR  0.5  // deweighting factor to smooth the probability value
#define POLYNOMAIL_POST_KAPPA 1 // scale for confidence calculation
#define PF_FEATURE_DIM 4
#define MINF0 50
#define MAXF0 550
#define PF_LZERO -20

#ifdef FALSE 
#undef FALSE
#endif

#ifdef TRUE 
#undef TRUE
#endif

typedef enum
{
	FALSE = 0, TRUE = 1,
} Boolean;

typedef float  LogFloat;   /* types just to signal log values */
typedef double LogDouble;
typedef wtk_vector_t* Vector;
typedef wtk_double_matrix_t* DMatrix;
typedef wtk_double_vector_t* DVector;
#define CreateDMatrix(h,n1,n2) wtk_double_matrix_new_h(h,n1,n2)
#define CreateDVector(h,n) wtk_double_vector_newh(h,n);
#define FreeMatrix(a,b)
#define FreeDMatrix(a,b)
#define VectorSize(v) wtk_vector_size(v)
#define CreateVector(h,s) wtk_vector_new_h(h,s)
#define FreeVector(h,v)
#define ZeroVector(v) wtk_vector_zero(v)

#define sgn(x)  ((x) >= 0 ? 1 : -1)
#define MACHEPS 2.22045e-16

void HholdVec(wtk_double_vector_t* tmp, int i0, int size,
                     double *beta, double *newval)
{
   int i;
   double norm = 0.0;

   for (i = i0; i <= size; i++) {
      norm += tmp[i]*tmp[i];
   }
   norm = sqrt(norm);

   if ( norm <= 0.0 ) {
      *beta = 0.0;
   }
   else {
      *beta = 1.0/(norm * (norm+fabs(tmp[i0])));
      if ( tmp[i0] > 0.0 )
         *newval = -norm;
      else
         *newval = norm;
      tmp[i0] -= *newval;
   }

}


void CopyDColumn(wtk_double_matrix_t *M, int k, wtk_double_vector_t *v)
{
   int i, size;

   size = wtk_vector_size(v);
   for (i = 1; i <= size; i++)
   {
      v[i] = M[i][k];
   }
}

/* copy a row from a matrix into  a vector */
void CopyDRow(wtk_double_matrix_t *M, int k, wtk_double_vector_t *v)
{
   int i, size;
   wtk_double_vector_t *w;

   size = wtk_vector_size(v);
   w = M[k];

   for (i = 1; i <= size; i++)
   {
      v[i] = w[i];
   }
}

/* HholdTrRows -- transform a matrix by a Householder vector by rows
   starting at row i0 from column j0 -- in-situ */
void HholdTrRows(wtk_double_matrix_t *M, int i0, int j0, wtk_double_vector_t* hh, double beta)
{
   double ip, scale;
   int i, j;
   int m,n;

   m = wtk_matrix_rows(M);
   n = wtk_matrix_cols(M);

   if ( beta != 0.0 ) {
      /* for each row ... */
      for ( i = i0; i <= m; i++ )
         {
            /* compute inner product */
            /* ip = __ip__(&(M->me[i][j0]),&(hh->ve[j0]),(int)(M->n-j0));*/
            ip = 0.0;
            for ( j = j0; j <= n; j++ )
               ip += M[i][j]*hh[j];
            scale = beta*ip;
            if ( scale == 0.0 )
               continue;
            /* __mltadd__(&(M->me[i][j0]),&(hh->ve[j0]),-scale,
               (int)(M->n-j0)); */
            for ( j = j0; j <= n; j++ )
               M[i][j] -= scale*hh[j];
         }
   }
}

/* HholdTrCols -- transform a matrix by a Householder vector by columns
   starting at row i0 from column j0 -- in-situ */
static void HholdTrCols(wtk_double_matrix_t* M, int i0, int j0,
		wtk_double_vector_t* hh, double beta, wtk_double_vector_t* w)
{
   int i, j;
   int n;

   n = wtk_matrix_rows(M);
   wtk_double_vector_zero(w);

   if ( beta != 0.0 ) {

      for ( i = i0; i <= n; i++ )
         if ( hh[i] != 0.0 )
            for ( j = j0; j <= n; j++ )
               w[j] += M[i][j]*hh[i];

      for ( i = i0; i <= n; i++ )
         if ( hh[i] != 0.0 )
            for ( j = j0; j <= n; j++ )
               M[i][j] -= w[j]*beta*hh[i];

   }
}

static void BiFactor(wtk_heap_t *heap,wtk_double_matrix_t* A, wtk_double_matrix_t* U, wtk_double_matrix_t *V)
{
   int n, k;
   wtk_double_vector_t *tmp1, *tmp2, *tmp3;
   double beta;

   n = wtk_matrix_rows(A);

   tmp1 = wtk_double_vector_newh(heap, n);
   tmp2 = wtk_double_vector_newh(heap, n);
   tmp3 = wtk_double_vector_newh(heap, n);

   for ( k = 1; k <= n; k++ )
   {
      CopyDColumn(A,k,tmp1);
      HholdVec(tmp1,k,n,&beta,&(A[k][k]));
      HholdTrCols(A,k,k+1,tmp1,beta,tmp3);
      if ( U )
         HholdTrCols(U,k,1,tmp1,beta,tmp3);
      if ( k+1 > n )
         continue;
      CopyDRow(A,k,tmp2);
      HholdVec(tmp2,k+1,n,&beta,&(A[k][k+1]));
      HholdTrRows(A,k+1,k+1,tmp2,beta);
      if ( V )
         HholdTrCols(V,k+1,1,tmp2,beta,tmp3);
   }

   //FreeDVector(&gstack, tmp1);
}

static void Givens(double x, double y, double *c, double *s)
{
   double norm;

   norm = sqrt(x*x+y*y);
   if ( norm == 0.0 ) {
      *c = 1.0;
      *s = 0.0;
   }       /* identity */
   else {
      *c = x/norm;
      *s = y/norm;
   }
}

static void RotRows(wtk_double_matrix_t* M, int i, int k,
                    double c, double s)
{
   int   j, n;
   double temp;

   n = wtk_matrix_rows(M);

   /*
   if (i > n || k > n)
      HError(1, "RotRows: Index tooo big i=%d k=%d\n", i, k);
   */

   for ( j=1; j<=n; j++ ) {
      temp = c*M[i][j] + s*M[k][j];
      M[k][j] = -s*M[i][j] + c*M[k][j];
      M[i][j] = temp;
   }

}

/* FixSVD -- fix minor details about SVD make singular values non-negative
   -- sort singular values in decreasing order */
static void FixSVD(wtk_double_vector_t *d, wtk_double_matrix_t* U, wtk_double_matrix_t* V)
{

   int  i, j, n,rows;

   n = wtk_vector_size(d);
   rows=wtk_matrix_rows(U);

   /* make singular values non-negative */
   for (i = 1; i <= n; i++) {
      if ( d[i] < 0.0 ) {
         d[i] = - d[i];
         for ( j = 1; j <= rows; j++ )
            U[i][j] = - U[i][j];
      }
   }

   return;

#if 0 /* #### ge: what is this code after return supposed to do here? */
   {
      int  k, l, r, stack[MAX_STACK], sp;
      double tmp, v;

   /* sort singular values */
   sp = -1;
   l = 1;
   r = n;
   for ( ; ; ) {
      while ( r >= l ) {
         /* i = partition(d->ve,l,r) */
         v = d[r];
         i = l-1;
         j = r;
         for ( ; ; ) {
            /* inequalities are "backwards" for **decreasing** order */
            while ( d[++i] > v );
            while ( d[--j] < v );
            if ( i >= j )
               break;
            /* swap entries in d->ve */
            tmp = d[i];
            d[i] = d[j];
            d[j] = tmp;
            /* swap rows of U & V as well */
            for ( k = 1; k <= DVectorSize(U[1]); k++ ) {
               tmp = U[i][k];
               U[i][k] = U[j][k];
               U[j][k] = tmp;
            }
            for ( k = 1; k <= DVectorSize(V[1]); k++ ) {
               tmp = V[i][k];
               V[i][k] = V[j][k];
               V[j][k] = tmp;
            }
         }
         tmp = d[i];
         d[i] = d[r];
         d[r] = tmp;
         for ( k = 1; k <= DVectorSize(U[1]); k++ ) {
            tmp = U[i][k];
            U[i][k] = U[r][k];
            U[r][k] = tmp;
         }
         for ( k = 1; k <= DVectorSize(V[1]); k++ ) {
            tmp = V[i][k];
            V[i][k] = V[r][k];
            V[r][k] = tmp;
         }
         /* end i = partition(...) */
         if ( i - l > r - i ) {
            stack[++sp] = l;
            stack[++sp] = i-1;
            l = i+1;
         }
         else {
            stack[++sp] = i+1;
            stack[++sp] = r;
            r = i-1;
         }
      }
      if ( sp < 0 )
         break;
      r = stack[sp--];
      l = stack[sp--];
   }
   }
#endif
}


static void BiSVD(wtk_double_vector_t* d, wtk_double_vector_t* f, wtk_double_matrix_t* U, wtk_double_matrix_t* V)
{
   int i, j, n;
   int i_min, i_max, split;
   double c, s, shift, size, z;
   double d_tmp, diff, t11, t12, t22;

   /*
   if ( ! d || ! f )
      HError(1,"BiSVD: Vectors are null!");
   if ( DVectorSize(d) != DVectorSize(f) + 1 )
      HError(1, "BiSVD: Error with the vector sizes!");
   */
   n = wtk_vector_size(d);
   /*
   if ( ( U && DVectorSize(U[1]) < n ) || ( V && NumDRows(V) < n ) )
      HError(1, "BiSVD: Error Matrix sizes!");
   if ( ( U && NumDRows(U) != DVectorSize(U[1])) ||
        ( V && NumDRows(V) != DVectorSize(V[1])) )
      HError(1, "BiSVD: One of the matrices must be square");
	*/
   if ( n == 1 )
      return;

   s = 0.0;
   for ( i = 1; i <= n; i++)
      s += d[i]*d[i];
   size = sqrt(s);
   s = 0.0;
   for ( i = 1; i < n; i++)
      s += f[i]*f[i];
   size += sqrt(s);
   s = 0.0;

   i_min = 1;
   while ( i_min <= n ) {   /* outer while loop */
      /* find i_max to suit;
         submatrix i_min..i_max should be irreducible */
      i_max = n;
      for ( i = i_min; i < n; i++ )
         if ( d[i] == 0.0 || f[i] == 0.0 ) {
            i_max = i;
            if ( f[i] != 0.0 ) {
               /* have to ``chase'' f[i] element out of matrix */
               z = f[i];
               f[i] = 0.0;
               for ( j = i; j < n && z != 0.0; j++ ) {
                  Givens(d[j+1],z, &c, &s);
                  s = -s;
                  d[j+1] =  c*d[j+1] - s*z;
                  if ( j+1 < n ) {
                     z      = s*f[j+1];
                     f[j+1] = c*f[j+1];
                  }
                  RotRows(U,i,j+1,c,s);
               }
            }
            break;
         }


      if ( i_max <= i_min ) {
         i_min = i_max + 1;
         continue;
      }

      split = FALSE;
      while ( ! split ) {
         /* compute shift */
         t11 = d[i_max-1]*d[i_max-1] +
            (i_max > i_min+1 ? f[i_max-2]*f[i_max-2] : 0.0);
         t12 = d[i_max-1]*f[i_max-1];
         t22 = d[i_max]*d[i_max] + f[i_max-1]*f[i_max-1];
         /* use e-val of [[t11,t12],[t12,t22]] matrix
            closest to t22 */
         diff = (t11-t22)/2;
         shift = t22 - t12*t12/(diff +
                                sgn(diff)*sqrt(diff*diff+t12*t12));

         /* initial Givens' rotation */
         Givens(d[i_min]*d[i_min]-shift,
                d[i_min]*f[i_min], &c, &s);

         /* do initial Givens' rotations */
         d_tmp      = c*d[i_min] + s*f[i_min];
         f[i_min]   = c*f[i_min] - s*d[i_min];
         d[i_min]   = d_tmp;
         z          = s*d[i_min+1];
         d[i_min+1] = c*d[i_min+1];
         RotRows(V,i_min,i_min+1,c,s);

         /* 2nd Givens' rotation */
         Givens(d[i_min],z, &c, &s);
         d[i_min]   = c*d[i_min] + s*z;
         d_tmp      = c*d[i_min+1] - s*f[i_min];
         f[i_min]   = s*d[i_min+1] + c*f[i_min];
         d[i_min+1] = d_tmp;
         if ( i_min+1 < i_max ) {
            z          = s*f[i_min+1];
            f[i_min+1] = c*f[i_min+1];
         }
         RotRows(U,i_min,i_min+1,c,s);

         for ( i = i_min+1; i < i_max; i++ ) {
            /* get Givens' rotation for zeroing z */
            Givens(f[i-1],z, &c, &s);
            f[i-1] = c*f[i-1] + s*z;
            d_tmp  = c*d[i] + s*f[i];
            f[i]   = c*f[i] - s*d[i];
            d[i]   = d_tmp;
            z      = s*d[i+1];
            d[i+1] = c*d[i+1];
            RotRows(V,i,i+1,c,s);

            /* get 2nd Givens' rotation */
            Givens(d[i],z, &c, &s);
            d[i]   = c*d[i] + s*z;
            d_tmp  = c*d[i+1] - s*f[i];
            f[i]   = c*f[i] + s*d[i+1];
            d[i+1] = d_tmp;
            if ( i+1 < i_max ) {
               z      = s*f[i+1];
               f[i+1] = c*f[i+1];
            }
            RotRows(U,i,i+1,c,s);
         }
         /* should matrix be split? */
         for ( i = i_min; i < i_max; i++ )
            if ( fabs(f[i]) <
                 MACHEPS*(fabs(d[i])+fabs(d[i+1])) )
               {
                  split = TRUE;
                  f[i] = 0.0;
               }
            else if ( fabs(d[i]) < MACHEPS*size )
               {
                  split = TRUE;
                  d[i] = 0.0;
               }
      }
   }
}

void SVD(wtk_heap_t *heap,wtk_double_matrix_t* A, wtk_double_matrix_t* U, wtk_double_matrix_t* V, wtk_double_vector_t* d)
{
   wtk_double_vector_t* f=NULL;
   int i, n;
   wtk_double_matrix_t* A_tmp;


   n = wtk_matrix_rows(A);
   A_tmp = wtk_double_matrix_new_h(heap, n, n);
   wtk_double_matrix_cpy(A, A_tmp);
   wtk_double_matrix_init_identity(U);
   wtk_double_matrix_init_identity(V);
   f=wtk_double_vector_newh(heap,n-1);

   BiFactor(heap,A_tmp,U,V);
   for ( i = 1; i <= n; i++ ) {
      d[i] = A_tmp[i][i];
      if ( i+1 <= n )
         f[i] = A_tmp[i][i+1];
   }

   BiSVD(d,f,U,V);
   FixSVD(d,U,V);
  // FreeDMatrix(&gstack, A_tmp);
}

void InvSVD(wtk_heap_t *heap,wtk_double_matrix_t *A, wtk_double_matrix_t *U, wtk_double_vector_t* W, wtk_double_matrix_t* V, wtk_double_matrix_t* Result)
{
   int m, n, i, j, k;
   double wmax, wmin;
   wtk_double_matrix_t *tmp1;

   m = wtk_matrix_rows(U);
   n = wtk_vector_size(U[1]);


   SVD(heap,A, U, V, W);
   /* NOTE U and V actually now hold U' and V' ! */

   tmp1 = wtk_double_matrix_new_h(heap,m, n);

   wmax = 0.0;
   for (k = 1; k <= n; k ++)
      if (W[k] > wmax)
         wmax = W[k];
   wmin = wmax * 1.0e-8;
   for (k = 1; k <= n; k ++)
      if (W[k] < wmin) {
         /* A component of the diag matrix 'w' of the SVD of 'a'
            was smaller than 1.0e-6 and consequently set to zero. */
         W[k] = 0.0;
      }
   /* tmp1 will be the product of matrix v and the diagonal
      matrix of singular values stored in vector w. tmp1 is then
      multiplied by the transpose of matrix u to produce the
      inverse which is returned */
   for (j = 1; j <= m; j++)
      for (k = 1; k <= n; k ++)
         if (W[k] > 0.0)
            /* Only non-zero elements of diag matrix w are
               used to compute the inverse. */
            tmp1[j][k] = V[k][j] / W[k];
         else
            tmp1[j][k] = 0.0;

   wtk_double_matrix_zero(Result);
   for (i=1;i<=m;i++)
      for (j=1;j<=m;j++)
         for (k=1;k<=n;k++)
            Result[i][j] += tmp1[i][k] * U[k][j];
  // FreeDMatrix(&gstack,tmp1);
}

/* ----------------------------------------------------------------------------- */
/*           Link operation for voice segment               */
/* ------------------------------------------------------------------------------*/
/*create new empty struct to save voice segmentation information*/
VoiceSeg * NewVoiceSeg(void)
{
	VoiceSeg *newseg;

	newseg = (VoiceSeg *)malloc(sizeof(VoiceSeg));
	newseg->st = 0;
	newseg->end = 0;
	newseg->dur = 0;
        newseg->ave_f0 = 0;
        newseg->ave_engy = 0.0;
	newseg->next = NULL;
	return newseg;
}

/* Recursively free all the links after oldseg (incl. oldseg) */
void FreeVoiceSeg(VoiceSeg *oldseg)
{
	VoiceSeg* current = oldseg;
	VoiceSeg* next = current->next;
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
VoiceSeg* GetVoiceSeg(ToneFeat *tFeat, float fMinEngy)
{
   VoiceSeg *seghead, *segcur;
   Boolean voiced;
   int t, nSeg, minEngy;
   int T=tFeat->nFrm;
   /* floor silence first */
   minEngy = INT32_MAX; /* just a big number */

   /* find the minimum non-zero value */
   for(t=0;t<T;t++){
      if(tFeat->engy[t]>0 && minEngy>tFeat->engy[t]) minEngy = (int)tFeat->engy[t];
   }
   /* calibrate energy and floor sil first */
   for(t=0;t<T;t++)
      tFeat->engy[t] -= minEngy;


   /* preprocessing: remove head and tail f0 values with f0<MINF0 or engy<MINENGY */
   for(t=0; t<T; t++){
      if(tFeat->f0[t]<MINF0||tFeat->f0[t]>MAXF0|| tFeat->engy[t]<fMinEngy)
         tFeat->f0[t] = 0.0;
      else
         break;
   }
   for(t=T-1; t>0; t--){
      if(tFeat->f0[t]<MINF0||tFeat->f0[t]>MAXF0|| tFeat->engy[t]<fMinEngy)
         tFeat->f0[t] = 0.0;
      else
         break;
   }

   /* find remained voiced segments */
   seghead = NewVoiceSeg();

   /* get boundary of each voiced/uv region, always uv|v|uv|v... */
   nSeg = 0; segcur = seghead; voiced = FALSE;
   for (t = 0; t < T; t++) {
      if (tFeat->f0[t] > 0 && !voiced) {
         nSeg++;
         segcur->next = NewVoiceSeg(); // add new seg and jump to the new seg
         segcur = segcur->next; 
         segcur->st = t;
         voiced = TRUE;
      } else if (tFeat->f0[t] == 0 && voiced){
         segcur->end = t - 1;
         segcur->dur = segcur->end - segcur->st + 1;
         if( segcur->dur < 2 )
            printf("Error: too short seg %d\n",segcur->dur);
         voiced = FALSE;
      }
      //wtk_debug("cur dur=%d\n",segcur->dur);
   }

   if(voiced) {
      segcur->end = T - 1;
      segcur->dur = segcur->end - segcur->st + 1;
   } 
   seghead->dur = nSeg;  /* steal a space to save the number of segs */

   return seghead;
} 

/* Delete the next voice seg structure */
void DelNextVoiceSeg(VoiceSeg *curseg, ToneFeat *tFeat)
{
	VoiceSeg *tmp;
	int i=0;
	if(!curseg->next){
		fprintf(stderr, "Error: can not delete null\n");
		exit(1);
	}
	tmp = curseg->next;

	//discard the short voiced segment and set the value of f0 as zero, add by hlw at 2009.5.4
	for(i=tmp->st;i<=tmp->end;i++)
            tFeat->f0[i]=0.0;

	curseg->next = curseg->next->next;

	free(tmp);

	return;
}

/*discard these segmentation whose average energy is lower than the energy of master segmentation */
void DiscardLowAveEngySeg(VoiceSeg *seghead, ToneFeat *tFeat, float fEngyRationThd)
{
   VoiceSeg *ptr;
   double master_engy=0,tmp_engy;
   int t, maxdur;

   /* calculate average energy for each voiced region and find the master engery */
   /* master engery is the average energy of the longest voiced segment */
   maxdur = 0;
   for(ptr=seghead->next;ptr;ptr=ptr->next){
      tmp_engy = 0.0;
      for(t=ptr->st;t<=ptr->end;t++) tmp_engy += tFeat->engy[t];
      ptr->ave_engy = (float)tmp_engy/(float)(ptr->dur);
      if(maxdur < ptr->dur){
         maxdur = ptr->dur;
         master_engy = ptr->ave_engy;
      }
   }

   /* discard low average energy regions */
   for(ptr=seghead;ptr && ptr->next;ptr=ptr->next){
      /* ptr->next is to be checked and deleted */
      if(ptr->next->ave_engy < fEngyRationThd*master_engy){
         DelNextVoiceSeg(ptr, tFeat);
         seghead->dur--;
      }
   }

   return;
}

/* delete isolate shortest voiced segments first */
Boolean DeleteShortestSeg(VoiceSeg *seghead, ToneFeat *tFeat,int nMinVoiceDur)
{
    VoiceSeg *ptr, *nxshortest;
   /* delete isolate short voiced segments first */
    nxshortest = seghead;
    for(ptr=seghead;ptr->next;ptr=ptr->next)
	if(ptr->next->dur < nxshortest->next->dur)
            nxshortest = ptr;
    if(nxshortest->next->dur <nMinVoiceDur ){
        DelNextVoiceSeg(nxshortest, tFeat);
        seghead->dur--;
        return TRUE;
    } else
        return FALSE;
}

/*discard these isolated short voiced segments whose length is lower than the given threshold */
void DiscardShortSeg(VoiceSeg *seghead, ToneFeat *tFeat,  int tgtNum, int nMinVoiceDur)
{
    if(seghead->dur > tgtNum){
		fprintf(stderr, "WARNING: After preprocessing, still %d chars detected, inconsistent with %d target chars! ", seghead->dur, tgtNum);//You need to check whether you input the wrong numChar! As a hack, shorter segments are deleted to give you result\n", seghead->dur, tgtNum);
		while(seghead->dur > tgtNum)
                    if(!DeleteShortestSeg(seghead, tFeat, nMinVoiceDur))
                            return;
	} else if(seghead->dur == tgtNum)
		return;
}

/*merge two voice segmentations using linear  interpolation*/
VoiceSeg * MergeVoiceSeg(VoiceSeg *prev, VoiceSeg *follow, ToneFeat *tFeat)
{

	double scope, tmp_engy;
	int t;

	// interpolate f0 values - linear 
	scope = (tFeat->f0[follow->st] - tFeat->f0[prev->end])/(double)(follow->st-prev->end);
	for(t=prev->end+1;t<follow->st;t++)
		tFeat->f0[t] = tFeat->f0[prev->end] + scope*(t-prev->end);

	// merge voice seg structure and save in *prev
	tmp_engy = 0.0;
	for(t=prev->end+1;t<follow->st;t++) tmp_engy += tFeat->engy[t];
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

/*merge two voice segmentation with the shortest interval/gap until the number of voice segmentation is less than or equal with the syllable number*/
void MergeNearVoiceSeg(VoiceSeg *seghead, ToneFeat *tFeat, int tgtNum, int nUvMergeThd)
{

   VoiceSeg *ptr,*pre_ptr, *min_ptr;
   int dur_nuv, mindur;
   
   /* merge segments if there are still too many voiced segs */
    while( seghead->dur > tgtNum ){
        /*find minmum duration	between two voiced segs*/		
	mindur=INT32_MAX;
	min_ptr=NULL;
	for(ptr=seghead->next->next,pre_ptr=seghead->next;ptr;ptr=ptr->next,pre_ptr=pre_ptr->next){
		dur_nuv = ptr->st - pre_ptr->end;
		if(mindur>dur_nuv){
			mindur=dur_nuv;
			min_ptr=pre_ptr;
		}
	}
	/*merge the two voiced segs*/
	//if(min_ptr&&mindur <= 2*nUvMergeThd){
		min_ptr = MergeVoiceSeg(min_ptr,min_ptr->next,tFeat);
          	seghead->dur--;
	//}			
    }

   /* merge these closest segments */
   for(ptr=seghead->next;ptr;ptr=ptr->next){
       
         /* possible candidate to merge, check the previous/next unvoiced seg */
         /* merge this segment with the neighbour if it is close to any of them */
         /* get unvoiced seg dur */
         if(ptr->next) dur_nuv = ptr->next->st - ptr->end-1;
         else dur_nuv = nUvMergeThd + 1;

         /* unvoiced seg dur must be greater than UVMERGETHRD */
         if(dur_nuv <= nUvMergeThd){
            ptr = MergeVoiceSeg(ptr,ptr->next,tFeat);
            seghead->dur--;
         }         
   }

    return;
}

/*-------------------------------------------------------------------------------------------*/
/*   build voiced segment struct for F0 smoothing                */
/*-------------------------------------------------------------------------------------------*/
VoiceSeg *CheckVoiceSeg(VoiceSeg *head)
{
    if(head->dur<1)
    {
        FreeVoiceSeg(head);
        head=0;
    }
    return head;
}

/*create voiced segment struct for F0 smoothing*/
VoiceSeg * CreateVoiceSeg(ToneFeat *tFeat,int nTone, F0ProcPara F0ProcPrm)
{
    VoiceSeg *head=NULL;
    /* 1. construct voice segments */
    head = GetVoiceSeg(tFeat, F0ProcPrm.minEngThd);

    /*check whether the F0 frame number is valid*/
    if(CheckVoiceSeg(head)==NULL) return NULL;
   /* 2. discard voiced regins with very low average energy */
   DiscardLowAveEngySeg(head,tFeat,F0ProcPrm.engRatioThd);
   if(CheckVoiceSeg(head)==NULL) return NULL;
   /* 3. discard voiced regins with very short length */
   DiscardShortSeg(head,tFeat,nTone, F0ProcPrm.minVoiceLen);
   if(CheckVoiceSeg(head)==NULL) return NULL;
   /* 4. merge  very close voiced regions and 
      delete isolated short segs after merging, result should be only 1 seg */
   MergeNearVoiceSeg(head,tFeat,nTone, F0ProcPrm.minUnvoiceLen);
   return head;
}



/* ----------------------------------------------------------------------------- */
/*                 filter design  for Smoothing                 */
/* ---------------------------------------------------------------------------- */
/* ---------------- Average Filter ------------------------ */
/* calulate average for points from stidx to endidx with windowSize neighbouring points */
void AveFilter(float *data, int stidx, int endidx, int windowSize)
{
    float *tmp, tmpvalue;
    int hfWnd, i, j;

    if(endidx-stidx<5) return;
	if(windowSize<2 || windowSize>128){
		fprintf(stderr, "Wrong window size %d\n",windowSize);
		exit(1);
	}
	hfWnd = (int)(windowSize/2);

	/* get tmp array, extend boundary using edge values */
	tmp = (float *)malloc((endidx-stidx+1+windowSize)*sizeof(float));
	memcpy(tmp+hfWnd,data+stidx,(endidx-stidx+1)*sizeof(float));
	for(i=0;i<hfWnd;i++) tmp[i] = data[stidx];
	for(i=endidx-stidx+hfWnd+1;i<=endidx-stidx+windowSize;i++) tmp[i] = data[endidx];

	/* do smoothing */
	for(i=0;i<endidx-stidx+1;i++){
		tmpvalue = 0.0;
		for(j=0;j<windowSize;j++) tmpvalue += tmp[i+j];
		data[stidx+i] = tmpvalue/(float)windowSize;
	}

	free(tmp);

	return;
}

/* -----------------  Median Filter ----------------- */
/* Median filter implemented using link list */
dLink *InitialiseMedianFilter(dLink *head, float *data, int stidx)
{

	int i, size;
	dLink *ptrTmp, *ptr;

	size = head->idx;

	/* initialise */
	for(i=stidx;i<stidx+size;i++){
		ptrTmp = (dLink *)malloc(sizeof(dLink));
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

dLink *UpdateMedianFilter(dLink *head, int delIdx, int addIdx, float *data)
{

	dLink *ptr, *takeOut;
	int i, hfWnd;

	/* take out link with delIdx */
	ptr = head;
	while(ptr->next && ptr->next->idx != delIdx)
		ptr = ptr->next;
	if(!ptr->next){
		fprintf(stderr, "Error: idx %d not in median filter",delIdx);
		exit(1);
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

void FreeMedianFilter(dLink *head)
{
	dLink *ptr, *toFree;
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
void MedianFilter(float *data, int stidx, int endidx, int windowSize)
{
	float *tmp;
	dLink *mHead, *mMedian;
	int hfWnd, i, j;

        if(endidx-stidx<5) return;
	if(windowSize<2 || windowSize>128){
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
	tmp = (float *)malloc((endidx-stidx+2-windowSize)*sizeof(float));
	mHead = (dLink *)malloc(sizeof(dLink));
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
void PulseFilter(float *data, int stidx, int endidx, int windowSize, int nPlsFltThd)
{
    float tmpvalue, avediff;
    float *tmp;
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
    tmp = (float *)malloc((endidx-stidx+1)*sizeof(float));
    avediff = 0.0;numPoints = 0;
    for(i=stidx;i<=endidx;i++){
    	tmp[i-stidx] = data[i];
        if(fabs(data[i]) <= nPlsFltThd){ // get average value of small values
    	    avediff += data[i];
            numPoints++;
        }
    }
    
    //add by hlwang, otherwis, it seems there are some expection to happen in the future because numPoints may be zero by hlw.
    if(numPoints>0)
	avediff /= (float)numPoints; 

    /* detect pulse and do average for pulse point */
    for(i=stidx;i<=endidx;i++){
    	if( fabs(data[i]) >= nPlsFltThd ){
        	tmpvalue = 0.0; 
            /* find the neighbouring windowSize points and do average */
            /* 1. search left side within range windowSize */
            cnt = i-1; numPoints = 0;
            //while(cnt >= stidx && cnt > i-windowSize && numPoints < hfWnd){
            while(cnt >= stidx && cnt > i-hfWnd && numPoints < hfWnd){     //changed by hlw       
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
            } else if (numPoints < 2){
            	tmp[i-stidx] = avediff;
            } else
            	tmp[i-stidx] = tmpvalue/(float)numPoints;
		}
	}

    //add by hlw, to reduce the influence of pre-position
    for(i=stidx;i<=endidx;i++){
	data[i]=tmp[i-stidx];
        //printf("%d %f\n",i,data[i]);
    }
   free(tmp);

    return;
}


/* ----------------------------------------------------------------------------- */
/*          Smooth and Normalisation                        */
/* ---------------------------------------------------------------------------- */

/* smooth derivative and then smooth static again, only smooth voiced segments */
/* modify boundary if necessary */
void SmoothF0(ToneFeat *tFeat,VoiceSeg *seghead, F0ProcPara F0ProcPrm)
{
    VoiceSeg *segcur;
    int t;
    float lastsum=0, nowsum=0;
    int T=tFeat->nFrm;
    float *df0=(float *)calloc(T,sizeof(float));   /* get differential of F0, df0(t) = f0(t+1) - f0(t) */

   /* smooth f0 in the voiced segment */
   segcur = seghead;
   for(segcur = seghead->next;segcur;segcur = segcur->next){
      // do median average first to smooth the curve
      MedianFilter(tFeat->f0,segcur->st,segcur->end,5);
      for(t=segcur->st; t<segcur->end; t++){
         df0[t] = tFeat->f0[t+1] - tFeat->f0[t];
         //printf("%d %f %f\n",t,tFeat->f0[t],df0[t]);
         }
   }
   
   /* smooth df0 first: use 5 neighborhood points to do median and average */
   for(segcur = seghead->next;segcur;segcur = segcur->next){
      /* remove pulses */
      PulseFilter(df0,segcur->st,segcur->end-1,5, F0ProcPrm.plsFilterThd);
      /* further smooth derivatives */
      AveFilter(df0,segcur->st,segcur->end-1,5);
   }

   /*TODO:
   //find the most stable point as start point to restore F0 */

    /* resume f0 from smoothed df0 and smooth f0 again using 3 neighbouring points */
    for(segcur = seghead->next;segcur;segcur = segcur->next){
        //computer original F0 average
	lastsum=0.0;      	
	for(t=segcur->st;t<=segcur->end;t++)
            lastsum=lastsum+tFeat->f0[t];


      	//Taking 1st point as the baseline is not reliable, this should be modified later!
	nowsum=tFeat->f0[segcur->st];   
	for(t=segcur->st+1; t<=segcur->end; t++){
            tFeat->f0[t] = tFeat->f0[t-1] + df0[t-1];
                nowsum=nowsum+tFeat->f0[t];
	}
        

	//parallelly move F0 to original position
	lastsum=(lastsum-nowsum)/(segcur->end-segcur->st+1);
	for(t=segcur->st;t<=segcur->end;t++)
		tFeat->f0[t]=tFeat->f0[t]+lastsum;
	
      	AveFilter(tFeat->f0,segcur->st,segcur->end,3);
    }
    free(df0);
    return;
}
//normalize the F0 sequence
void NormalizeF0(ToneFeat *tFeat,VoiceSeg *seghead)
{
    int i,sig,nSum;
    double ave_f0=0.0;
    VoiceSeg *segcur;
    for(i=0;i<tFeat->nFrm;i++)
    {
        sig=(tFeat->f0[i]< 0.0)?-1:1;
        if(tFeat->f0[i]!=0.0) tFeat->f0[i]=sig*log(fabs(tFeat->f0[i]));
    }
    
     /* calculate average f0 and engy of voiced segments */
    nSum=0;
    for(segcur = seghead->next; segcur; segcur = segcur->next){
        for(i=segcur->st; i<=segcur->end; i++)
            ave_f0 += tFeat->f0[i];
       	nSum=nSum+segcur->end-segcur->st+1;
    }
    ave_f0 = ave_f0/nSum;

    /* normalise by dividing the average (can be done in LOG domain later) */
    for(i=0; i<tFeat->nFrm; i++)
        if(tFeat->f0[i]!=0.0)
            tFeat->f0[i] -= ave_f0;    
}
/* ----------------------------------------------------------------------------- */
/*          Extract Polynomial fitting feature              */
/* ---------------------------------------------------------------------------- */
/*calculate coefficients of 2nd order polynomial*/
 void CalPoly2Coef(wtk_heap_t *heap,VoiceSeg *seg, Poly2Stat *stat, int doLogNrm)
{
	DMatrix invA, U, V, A; // matrix for MLS
	DVector W;
	Vector a,b;
	size_t i,j;

	A = CreateDMatrix(heap,3,3);
	invA = CreateDMatrix(heap,3,3);
	U = CreateDMatrix(heap,3,3);
	V = CreateDMatrix(heap,3,3);
	W = CreateDVector(heap,3);
	a = CreateVector(heap,3);
	b = CreateVector(heap,3);
        
	// fill in the symetric matrix and bias
	A[1][1] = stat->matx[0]; A[2][1] = A[1][2] = stat->matx[1]; A[3][1] = A[1][3] = stat->matx[2];
	A[2][2] = A[3][1]; A[3][2] = A[2][3] = stat->matx[3]; A[3][3] = stat->matx[4];

	b[1] = stat->vecxy[0]; b[2] = stat->vecxy[1]; b[3] = stat->vecxy[2];

	// a=A^{-1}*b
	InvSVD(heap,A, U, W, V, invA);
	ZeroVector(a);
	for(i=1;i<=3;i++)
		for(j=1;j<=3;j++)
			a[i] += (float)invA[i][j] * b[j];

	// copy the coefficients
	seg->PFfeat.a[0] = a[1]; seg->PFfeat.a[1] = a[2]; seg->PFfeat.a[2] = a[3];
	// use the variance as the last feature as tone 1 is sensitive to this feature
    // note that boundary points are deweighted
        if(doLogNrm)
	{
                if(stat->sqy == 0.0f)
                    seg->PFfeat.logres = PF_LZERO;
                else
                    seg->PFfeat.logres = (float)log(stat->sqy / (float)stat->matx[4]);
        }
        else
        {
               /*modified by hlwang, at 2010.0428*/
                if(stat->sqy == 0.0f)
                    seg->PFfeat.logres = 0.0;
                else
                    seg->PFfeat.logres = (float)(stat->sqy / (float)stat->matx[4]);
        }

	FreeDMatrix(&gstack, A);
	return;
}

/*Extract Polynomial fitting feature from coefficients of 2nd order polynomial */
void ExtractPolyFea(wtk_heap_t *heap,VoiceSeg *seghead, ToneFeat *tFeat, int doLogNrm)
{
	VoiceSeg *ptr;
	Poly2Stat *stat;
	double tmpmean;
	float x,y, wgt, ratio;
	size_t t,segcnt;

	stat = (Poly2Stat *)malloc(sizeof(Poly2Stat));
	segcnt = 0;

	for(ptr=seghead->next;ptr;ptr=ptr->next){
		// calculate mean of f0
		tmpmean = 0.0;
		for(t=ptr->st;t<=ptr->end;t++)
			tmpmean += tFeat->f0[t];
		tmpmean /= (double)(ptr->dur);
		
		//sanity check for debug
        if(ptr->dur != ptr->end - ptr->st +1){
           fprintf(stderr, "Error: segment duration %d is not consistent with st(%d)/end(%d) time.\n",ptr->dur, ptr->st, ptr->end );
           exit(1);
        }
        /*
        // use a pre-defined window to weight the data
        //       _________       1.0
        //      /         \
        // ____/           \____ 0.0
        //    0 0.14    0.8 1.0
        //
        // wgt = n/dur*7.0   n/dur<=0.14
        // wgt = 1.0       0.14<n/dur<0.8
        // wgt = 1.0-(1.0-n/dur)*5.0    0.8<=n/dur<1.0
        //
		// accumulate stat based on normalised f0
        */
		stat->sqy = stat->vecxy[0] = stat->vecxy[1] = stat->vecxy[2] = 0.0;
		stat->matx[0] = stat->matx[1] = stat->matx[2] = stat->matx[3] = stat->matx[4] = 0.0;
		for(t=ptr->st;t<=ptr->end;t++){
			// f0=f0-tmpmean, and t= (t-meanT)*itvScale
			// itvScale is to change dynamic range by scaling the interval between t and (t-1)
			x = (float)(t - (float)(ptr->st+ptr->end)/2.0)*INTERVALSCALE;
			y = (float)(tFeat->f0[t] - tmpmean);

			//if(debug==100 || debug==101 || debug==103 ) printf("NormalisedSeg %d %f %f\n",ptr->st,x,y);
			
			// find corresponding weight
            ratio = (float)(t-ptr->st+1)/(float)ptr->dur;
            if( ratio <= 0.14 )
               wgt = ratio*7.0f;
            else if ( ratio >= 0.8 )
               wgt = 1.0f-(1.0f-ratio)*5.0f;
            else
               wgt = 1.0;

			stat->sqy += wgt * y*y;
			stat->vecxy[0] += wgt * x*x*y; stat->vecxy[1] += wgt * x*y; stat->vecxy[2] += wgt * y; 
			stat->matx[0] += wgt * x*x*x*x;stat->matx[1] += wgt * x*x*x;
			stat->matx[2] += wgt * x*x;stat->matx[3] += wgt * x; stat->matx[4] += wgt;          
		}

		// calculate coefficients
		CalPoly2Coef(heap,ptr,stat, doLogNrm);

		segcnt++;

	}

	free(stat);

	return;
}

/*-------------------------------------------------------------------------------------------------*/
VoiceSeg *ExtractPolyFitFeat(wtk_heap_t *heap,ToneFeat *tFeat,int nTone, F0ProcPara F0ProcPrm)
{
    VoiceSeg *seghead=NULL;
    if((seghead=CreateVoiceSeg(tFeat,nTone, F0ProcPrm))==NULL)
    {
            printf("ERROR: can't find valid voiced segment!\n");
            return NULL;
    }
    SmoothF0(tFeat,seghead, F0ProcPrm);
    NormalizeF0(tFeat,seghead);
    ExtractPolyFea(heap,seghead, tFeat, F0ProcPrm.doLogNrm);
    return seghead;
}

/*----------------------------------------------------------------------------------------------------*/
/*exchange the high byte and low byte of 16bit data*/
void Swap16 ( short *Short )
{
        *Short = ((*Short & 0x00ff) << 8) | ((*Short & 0xff00) >> 8);
}

/*exchange the high byte and low byte of 32bit data*/
void Swap32 (int* Long )
{
        *Long = ((*Long&0x000000ffL)<<24 ) | ((*Long&0x0000ff00L)<<8 )| 
                ((*Long&0x00ff0000L)>>8 )| ((*Long&0xff000000L)>>24 );
}


/*exchange the high byte and low byte*/
void swap_float_ptr(void* mfcc_ptr,int size)
{
	char *ptr;
    char temp;
    int i;

    ptr=(char*)mfcc_ptr;

	for(i=0;i<size;i++) {
		temp=ptr[0];
        ptr[0]=ptr[3];
        ptr[3]=temp;
        temp=ptr[1];
        ptr[1]=ptr[2];
        ptr[2]=temp;
        ptr+=4;
	}
    return;
}
