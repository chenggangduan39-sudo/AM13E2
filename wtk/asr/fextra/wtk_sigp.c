#ifdef USE_BLAS
#include "GotoBLAS2/common.h"
#include "GotoBLAS2/cblas.h"
#endif
#include "wtk_sigp.h"
#include "wtk/core/math/wtk_math.h"
#define Mel(k,fres) (1127*log(1+(k-1)*fres))

static void FBank2ASpec (wtk_vector_t *fbank, wtk_vector_t* as, wtk_vector_t* eql,
		float compressFact,FBankInfo *fb)
{
   const float melfloor = 1.0;
   int i;

   for (i=1; i<=fb->numChans; i++)
   {
      if (fbank[i] < melfloor)
      {
    	  fbank[i] = melfloor;
      }
      as[i+1] = fbank[i] * eql[i]; /* Apply equal-loudness curve */
      as[i+1] = pow((double) as[i+1], (double) compressFact);
   }
   as[1] = as[2];  /* Duplicate values at either end */
   as[fb->numChans+2] = as[fb->numChans+1];
}

/* Matrix IDFT converts from auditory spectrum into autocorrelation values */
static float MatrixIDFT(wtk_vector_t *as, wtk_vector_t *ac, wtk_double_matrix_t* cm)
{
   double acc;
   float E=0.0f;
   int nAuto, nFreq;
   int i, j;

   nFreq = wtk_vector_size(as);
   nAuto = wtk_vector_size(ac);
   for (i=0; i<nAuto; i++)
   {
      acc = cm[i+1][1] * (double)as[1];
      for (j=1; j<nFreq; j++)
      {
         acc += cm[i+1][j+1] * (double)as[j+1];
      }
      if (i>0)
      {
         ac[i] = (float)(acc / (double)(2.0 * (nFreq-1)));
      }
      else
      {
         E = (float)(acc / (double)(2.0 * (nFreq-1)));
      }
   }
   return E; /* Return zero'th auto value separately */
}


/* Durbins recursion to get LP coeffs for auto values */
static float  Durbin(wtk_vector_t* k, wtk_vector_t* thisA,wtk_vector_t* r,wtk_vector_t *newA, float E, int order)
{
   float ki;         /* Current Reflection Coefficient */
   int i,j;

   for (i=1;i<=order;i++)
   {
      ki = r[i];              /* Calc next reflection coef */
      for (j=1;j<i;j++)
      {
         ki = ki + thisA[j] * r[i - j];
      }
      ki = ki / E;
      if (k!=NULL) k[i] = ki;
      E *= 1 - ki*ki;         /* Update Error */
      newA[i] = -ki;          /* Calc new filter coef */
      for (j=1;j<i;j++)
      {
         newA[j] = thisA[j] - ki * thisA[i - j];
      }
      for (j=1;j<=i;j++)
      {
         thisA[j] = newA[j];
      }
   }
   return (E);
}

static void LPC2Cepstrum(wtk_vector_t *a, wtk_vector_t *c)
{
   int i,n,p;
   float sum;

   p=wtk_vector_size(c);
   for (n=1;n<=p;n++)
   {
      sum = 0.0;
      for (i=1;i<n;i++)
      {
         sum = sum + (n - i) * a[i] * c[n - i];
      }
      c[n] = -(a[n] + sum / n);
   }
}

static void ASpec2LPCep (wtk_vector_t* as, wtk_vector_t* ac, wtk_vector_t *acb,wtk_vector_t* lp, wtk_vector_t* c, wtk_double_matrix_t* cm)
{
   float lpcGain, E;

   // Do IDFT to get autocorrelation values
   E = MatrixIDFT(as, ac, cm);
   lp[wtk_vector_size(lp)] = 0.0;    // init to make Purify et al. happy

   // do Durbin recursion to get predictor coefficients
   lpcGain = Durbin(NULL,lp,ac,acb,E,wtk_vector_size(ac)-1);
   LPC2Cepstrum(lp,c);
   c[wtk_vector_size(c)] = (float) -log((double) 1.0/lpcGain); // value forms C0
}


/* EXPORT->FBank2MFCC: compute first n cepstral coeff */
static void  FBank2MFCC(wtk_vector_t* fbank, wtk_vector_t* c, int n)
{
   int j,k,numChan;
   float mfnorm,pi_factor,x;

   numChan = wtk_vector_size(fbank);
   mfnorm = sqrt(2.0/(float)numChan);
   pi_factor = PI/(float)numChan;
   for (j=1; j<=n; j++)
   {
      c[j] = 0.0; x = (float)j * pi_factor;
      for (k=1; k<=numChan; k++)
      {
         c[j] += fbank[k] * cos(x*(k-0.5));
      }
      c[j] *= mfnorm;
   }
}

/* EXPORT->FBank2C0: return zero'th cepstral coefficient */
static float FBank2C0(wtk_vector_t *v)
{
   int numChan;
   float mfnorm,sum;

   numChan = wtk_vector_size(v);
   mfnorm = sqrt(2.0/(float)numChan);
   sum = 0.0;
   wtk_vector_do_p(v,sum+=,);
   return sum * mfnorm;
}


void wtk_compute_power_spec(wtk_vector_t *v)
{
	int n=256;//128 for 8k
	float first_energy=v[0]*v[0];
	float last_energy=v[1]*v[1];
	float real,im;
	int i;
	for(i=1;i<n;i++)
	{
		real=v[i*2];
		im=v[i*2+1];
		v[i]=real*real+im*im;
	}
//	wtk_debug("%d\n",n);
	v[0]=first_energy;
	v[n]=last_energy;
}

static float mel_bank_16k[499]={0.404428,0.503977,0.496023,0.456236,0.543764,0.448956,0.551044,0.479132,0.520868,0.544077,0.455923,0.641381,0.358619,0.768881,0.231119,0.924625,0.106839,0.0753749,0.893161,0.313911,0.686089,0.544379,0.455621,0.7969,0.0702425,0.2031,0.929757,0.36328,0.63672,0.674974,0.00436526,0.325026,0.995635,0.350564,0.649436,0.712752,0.0901616,0.287248,0.909838,0.482087,0.517913,0.887862,0.306873,0.112138,0.693127,0.738548,0.182334,0.261452,0.817666,0.637739,0.104279,0.362261,0.895721,0.58151,0.069012,0.41849,0.930988,0.566388,0.0732714,0.433612,0.926729,0.589298,0.114145,0.410702,0.885855,0.647491,0.189037,0.352509,0.810963,0.738503,0.29562,0.261497,0.70438,0.860131,0.431788,0.0103721,0.139869,0.568212,0.989628,0.595653,0.187424,0.404347,0.812576,0.785486,0.389642,0.214514,0.610358,0.999723,0.615544,0.236939,0.00027689,0.384456,0.763061,0.863756,0.495834,0.133029,0.136244,0.504166,0.866971,0.775202,0.422215,0.073941,0.224798,0.577785,0.926059,0.730255,0.391039,0.0561804,0.269745,0.608961,0.94382,0.725562,0.399089,0.0766449,0.274438,0.600911,0.923355,0.758139,0.443476,0.132565,0.241861,0.556524,0.867435,0.825313,0.521638,0.221458,0.174687,0.478362,0.778542,0.924697,0.63127,0.341109,0.0541377,0.0753028,0.36873,0.658891,0.945862,0.770294,0.489498,0.211697,0.229706,0.510502,0.788303,0.936827,0.664813,0.395617,0.129166,0.0631733,0.335187,0.604383,0.870834,0.865409,0.604296,0.345765,0.0897774,0.134591,0.395704,0.654235,0.910223,0.836268,0.585207,0.33654,0.0902181,0.163732,0.414793,0.66346,0.909782,0.846199,0.604443,0.36491,0.12755,0.153801,0.395557,0.63509,0.87245,0.892335,0.659222,0.428174,0.199152,0.107665,0.340778,0.571826,0.800848,0.972125,0.747055,0.523916,0.302664,0.0832733,0.0278755,0.252945,0.476084,0.697336,0.916727,0.865711,0.649953,0.435962,0.223714,0.0131777,0.134289,0.350047,0.564038,0.776286,0.986822,0.804326,0.597133,0.391576,0.187624,0.195674,0.402867,0.608424,0.812376,0.985254,0.784444,0.585168,0.3874,0.191122,0.0147458,0.215556,0.414832,0.6126,0.808878,0.996307,0.802942,0.610995,0.420452,0.231288,0.0434887,0.00369281,0.197058,0.389005,0.579548,0.768712,0.956511,0.857034,0.671896,0.488068,0.305524,0.124251,0.142966,0.328104,0.511932,0.694476,0.875749,0.944226,0.765434,0.587862,0.411494,0.236307,0.0622835,0.0557737,0.234566,0.412138,0.588506,0.763693,0.937716,0.889419,0.717692,0.547094,0.377592,0.209193,0.0418724,0.110581,0.282308,0.452906,0.622408,0.790807,0.958128,0.875619,0.710417,0.546255,0.383121,0.220998,0.0598762,0.124381,0.289583,0.453745,0.616879,0.779001,0.940124,0.899743,0.740589,0.5824,0.425172,0.268876,0.113513,0.100257,0.259411,0.4176,0.574828,0.731124,0.886487,0.959065,0.805538,0.652903,0.50115,0.350284,0.200283,0.0511344,0.0409346,0.194462,0.347097,0.49885,0.649716,0.799717,0.948866,0.90284,0.755382,0.60875,0.462948,0.317948,0.173751,0.0303448,0.0971599,0.244618,0.39125,0.537052,0.682052,0.826249,0.969655,0.887724,0.745871,0.604798,0.464476,0.324912,0.186088,0.0479925,0.112276,0.254129,0.395202,0.535524,0.675088,0.813912,0.952007,0.910632,0.773984,0.638053,0.502828,0.368293,0.234459,0.101303,0.089368,0.226016,0.361947,0.497172,0.631707,0.765541,0.898697,0.968825,0.837008,0.705864,0.57537,0.445526,0.316326,0.187765,0.0598364,0.0311755,0.162992,0.294136,0.42463,0.554474,0.683674,0.812235,0.940164,0.932529,0.805837,0.679762,0.55429,0.429418,0.305145,0.18146,0.0583562,0.067471,0.194163,0.320238,0.44571,0.570582,0.694855,0.81854,0.941644,0.935829,0.813885,0.6925,0.571675,0.451409,0.331702,0.212538,0.0939107,0.0641705,0.186115,0.3075,0.428325,0.548591,0.668298,0.787462,0.906089,0.975831,0.858271,0.741248,0.624739,0.50875,0.393281,0.278315,0.163857,0.0498969,0.0241686,0.141729,0.258752,0.375261,0.49125,0.606719,0.721685,0.836143,0.950103,0.936434,0.823463,0.710978,0.598974,0.48745,0.3764,0.265825,0.155714,0.0460653,0.0635659,0.176537,0.289022,0.401026,0.51255,0.6236,0.734175,0.844287,0.953935,0.93688,0.828141,0.71986,0.612025,0.504637,0.397683,0.291165,0.185087,0.0794338,0.0631198,0.171859,0.28014,0.387975,0.495364,0.602317,0.708835,0.814913,0.920566,0.97421,0.86941,0.765028,0.661059,0.557508,0.454358,0.351627,0.249291,0.14735,0.0458167,0.0257903,0.13059,0.234972,0.338941,0.442492,0.545642,0.648373,0.750709,0.85265,0.954183,0.944667,0.843913,0.743548,0.643568,0.543961,0.444743,0.345899,0.247427,0.149318,0.0515865,0.055333,0.156087,0.256452,0.356432,0.456039,0.555257,0.654101,0.752573,0.850682,0.948414,0.954217,0.857209,0.760563,0.664273,0.568333,0.472749,0.37751,0.282621,0.188077,0.093866};
static int mel_offset_16k[64]={1,2,3,4,5,6,7,8,9,11,12,13,15,16,18,19,21,22,24,26,28,30,32,34,36,38,41,43,45,48,51,54,57,60,63,66,70,73,77,81,85,89,93,98,103,107,112,118,123,129,135,141,147,154,161,168,175,183,191,199,208,217,226,236};

static int mel_cut_16k[64]={2,2,2,2,2,2,2,3,3,2,
3,3,3,3,3,3,3,4,4,4,
4,4,4,4,5,5,4,5,6,6,
6,6,6,6,7,7,7,8,8,8,
8,9,10,9,9,11,11,11,12,12,
12,13,14,14,14,15,16,16,17,18,
18,19,20,20};

static int mel_cnt_16k[64]={0,2,4,6,8,10,12,14,17,20,22,
25,28,31,34,37,40,43,47,51,55,
59,63,67,71,76,81,85,90,96,102,
108,114,120,126,133,140,147,155,163,171,
179,188,198,207,216,227,238,249,261,273,
285,298,312,326,340,355,371,387,404,422,
440,459,479
};


static int mel_cut_16k2[40]={3,3,3,3,4,4,5,5,4,5,6,6,6,6,7,8,8,8,9,10,10,10,11,12,13,14,14,15,17,18,18,19,21,22,24,25,26,29,30,31};
static float mel_bank_16k2[492]={0.255101,0.943645,0.395472,0.604528,0.76011,0.148375,0.23989,0.851625,0.558571,0.441429,0.989179,0.438832,0.0108214,0.561168,0.906302,0.390467,0.0936983,0.609533,0.890312,0.404915,0.109687,0.595085,0.933428,0.475075,0.0291449,0.0665716,0.524925,0.970855,0.594983,0.171984,0.405017,0.828016,0.759586,0.357274,0.240414,0.642726,0.964563,0.581008,0.206189,0.0354369,0.418992,0.793811,0.839719,0.481236,0.130394,0.160281,0.518764,0.869606,0.78688,0.45039,0.120644,0.21312,0.54961,0.879356,0.797376,0.480336,0.169293,0.202624,0.519664,0.830707,0.864018,0.564306,0.269954,0.135982,0.435694,0.730046,0.980776,0.696592,0.417235,0.142542,0.0192244,0.303408,0.582765,0.857458,0.872357,0.606541,0.344948,0.0874499,0.127643,0.393459,0.655052,0.91255,0.83392,0.584235,0.338287,0.095958,0.16608,0.415765,0.661713,0.904042,0.857146,0.621753,0.389679,0.160833,0.142854,0.378247,0.610321,0.839167,0.935127,0.712474,0.492793,0.276006,0.0620395,0.0648732,0.287527,0.507207,0.723994,0.93796,0.85082,0.642276,0.436347,0.232958,0.0320544,0.14918,0.357724,0.563653,0.767042,0.967946,0.833575,0.637461,0.443656,0.252107,0.0627631,0.166425,0.362539,0.556344,0.747893,0.937237,0.875575,0.69049,0.507466,0.326453,0.147413,0.124425,0.30951,0.492534,0.673547,0.852587,0.970296,0.795069,0.621689,0.450111,0.28031,0.112242,0.0297036,0.204931,0.378311,0.549889,0.71969,0.887758,0.945872,0.781169,0.618096,0.456627,0.296721,0.13836,0.0541284,0.218831,0.381904,0.543373,0.703279,0.86164,0.981508,0.826136,0.672216,0.519723,0.368634,0.218914,0.0705477,0.0184919,0.173864,0.327784,0.480277,0.631366,0.781086,0.929452,0.923507,0.777769,0.633309,0.490107,0.34814,0.207391,0.0678316,0.076493,0.222231,0.366691,0.509893,0.65186,0.792609,0.932168,0.929447,0.792215,0.656122,0.521143,0.387263,0.254464,0.122727,0.0705531,0.207785,0.343878,0.478857,0.612737,0.745536,0.877273,0.992035,0.862376,0.73373,0.606081,0.479416,0.353719,0.228973,0.105167,0.00796457,0.137624,0.26627,0.393919,0.520584,0.646281,0.771027,0.894833,0.982283,0.860314,0.73924,0.619051,0.499732,0.381274,0.263664,0.146884,0.0309316,0.0177167,0.139686,0.26076,0.380949,0.500268,0.618726,0.736336,0.853116,0.969068,0.915788,0.801447,0.687894,0.575117,0.463111,0.351863,0.241361,0.131593,0.0225553,0.0842117,0.198553,0.312106,0.424883,0.536889,0.648137,0.758639,0.868407,0.977445,0.914234,0.806626,0.699709,0.593488,0.487947,0.38308,0.278876,0.175327,0.0724279,0.0857657,0.193374,0.30029,0.406512,0.512053,0.61692,0.721124,0.824673,0.927572,0.970166,0.868536,0.767529,0.667138,0.567358,0.468183,0.369596,0.271597,0.174176,0.077336,0.0298337,0.131464,0.232471,0.332862,0.432642,0.531817,0.630404,0.728403,0.825824,0.922664,0.981059,0.885338,0.790177,0.695561,0.601483,0.507943,0.414931,0.322439,0.230472,0.13901,0.0480547,0.018941,0.114662,0.209823,0.304439,0.398517,0.492057,0.585069,0.677561,0.769528,0.86099,0.951945,0.957598,0.867637,0.778161,0.689177,0.600667,0.512634,0.425068,0.337962,0.25132,0.165126,0.0793855,0.0424017,0.132363,0.221839,0.310823,0.399333,0.487366,0.574932,0.662038,0.74868,0.834874,0.920614,0.99409,0.90923,0.824812,0.740821,0.657258,0.574112,0.49139,0.409078,0.327176,0.245681,0.164588,0.0838944,0.00359288,0.00590971,0.0907701,0.175188,0.259179,0.342742,0.425888,0.50861,0.590922,0.672823,0.754319,0.835412,0.916106,0.996407,0.92368,0.844155,0.765012,0.686247,0.607859,0.529843,0.452193,0.374907,0.297988,0.221422,0.145209,0.0693483,0.0763201,0.155845,0.234988,0.313753,0.392141,0.470157,0.547807,0.625093,0.702012,0.778578,0.854791,0.930652,0.993841,0.918675,0.843849,0.769368,0.695215,0.6214,0.54791,0.474748,0.401914,0.329397,0.257201,0.185318,0.11375,0.0424908,0.00615922,0.0813245,0.156151,0.230632,0.304785,0.3786,0.45209,0.525252,0.598086,0.670603,0.742799,0.814682,0.88625,0.957509,0.971539,0.900889,0.830543,0.760496,0.690749,0.621293,0.552131,0.48326,0.414671,0.346371,0.278352,0.210615,0.143152,0.0759637,0.00905348,0.0284614,0.0991107,0.169457,0.239504,0.309251,0.378706,0.447869,0.51674,0.585329,0.653629,0.721648,0.789385,0.856848,0.924036,0.990947,0.942411,0.876039,0.809934,0.744093,0.678512,0.613195,0.548131,0.483331,0.418781,0.35448,0.290435,0.226633,0.16308,0.0997737,0.0367094,0.0575894,0.123961,0.190066,0.255907,0.321488,0.386805,0.451869,0.516669,0.581219,0.645521,0.709565,0.773367,0.83692,0.900226,0.963291,0.97388,0.911297,0.848949,0.786836,0.724951,0.663305,0.601888,0.540698,0.479737,0.419,0.358484,0.298192,0.238118,0.178265,0.11863,0.0592043};
static int mel_offset_16k2[40]={1,3,4,6,7,9,11,13,16,18,20,23,26,29,32,35,39,43,47,51,56,61,66,71,77,83,90,97,104,112,121,130,139,149,160,171,184,196,210,225};
static int mel_cnt_16k2[40]={0,3,6,9,12,16,20,25,30,34,39,45,51,57,63,70,78,86,94,103,113,123,133,144,156,169,183,197,212,229,247,265,284,305,327,351,376,402,431,461};

double melb[]=
{
#include "melbank"
};

/* EXPORT->Wave2FBank:  Perform filterbank analysis on speech s */
static void Wave2Fbank(wtk_sigp_t *s,wtk_vector_t *v,wtk_vector_t *fbank,FBankInfo *info)
{
	float melfloor=1.0;
	int k,bin,i,j;
	float t1,t2;
	float ek;
	float *p;
	wtk_short_vector_t *lochan=info->loChan;
	wtk_vector_t *loWt=info->loWt;
	int khi=info->khi;
	int use_pow=info->usePower;
	int numChans=info->numChans;
	int nbin=s->rfft->win;
//       wtk_debug("use_power %d\n",use_pow);
//       wtk_debug("log fbank %d\n",info->takeLogs);
	//apply fft
//	for(k=1;k<=info->frameSize;++k)
//	{
//		info->x[k]=v[k];
//	}
	memcpy(info->x+1,v+1,info->frameSize*sizeof(float));
//	for(k=info->frameSize+1;k<=info->fftN;++k)
//	{
//		info->x[k]=0.0;
//	}
	memset(info->x+info->frameSize+1,0,(info->fftN-info->frameSize)*sizeof(float));
	if(!s->povey)
	{
		if(1)
		{
			p=s->rfft_fft;
			wtk_rfft_process_fft(s->rfft,p,info->x+1);
			//wtk_rfft_print_fft(t,rfft->len);
			//wtk_vector_do_p(fbank,,=0.0);
			memset(fbank+1,0,s->cfg->NUMCHNAS*sizeof(float));
			for(k=info->klo,i=k-1,j=k-1+s->rfft->win;k<=khi;++k,++i,++j)
			{
				t1=p[i];t2=p[j];
				//wtk_debug("t1[%d]=%f/%f\n",k,t1,t2);
				//exit(0);
				if(use_pow)
				{
					ek=t1*t1+t2*t2;
				}else
				{
					ek=sqrt(t1*t1+t2*t2);
				}
	//			bin=info->loChan[k];
	//			t1=info->loWt[k]*ek;
				bin=lochan[k];
				t1=loWt[k]*ek;
				//wtk_debug("t1=%f,w=%f,ek=%f\n",t1,info->loWt[k],ek);
				if(bin>0)
				{
					fbank[bin]+=t1;
				}
				if(bin<numChans)
				{
					fbank[bin+1]+=ek-t1;
				}
			}
		}else
		{
            wtk_realft(info->x);
            wtk_vector_do_p(fbank,,=0.0);
            //memset(fbank+1,0,wtk_vector_size(fbank)*sizeof(float));
            for(k=info->klo;k<=info->khi;++k)
            {
                t1=info->x[2*k-1];t2=info->x[2*k];
                //wtk_debug("t1[%d]=%f/%f\n",k,t1,t2);
                if(info->usePower)
                {
                    ek=t1*t1+t2*t2;
                }else
                {
                    ek=sqrt(t1*t1+t2*t2);
                }
                bin=info->loChan[k];
                t1=info->loWt[k]*ek;
                //wtk_debug("t1=%f,w=%f,ek=%f\n",t1,info->loWt[k],ek);
                if(bin>0)
                {
                    fbank[bin]+=t1;
                }
                if(bin<info->numChans)
                {
                    fbank[bin+1]+=ek-t1;
                }
            }
		}
	}else
	{
		if(s->cfg->NUMCHNAS == 64){
			p=s->rfft_fft;
			wtk_vector_do_p(fbank,,=0.0);
			wtk_rfft_process_fft(s->rfft,p,info->x+1);
			//wtk_vector_print(info->x);
			//print_double(dx,512);
			//wtk_compute_power_spec(info->x+1);
			p[0]*=p[0];
			p[nbin]*=p[nbin];

			for(i=1,j=i+nbin;i<nbin;++i,++j)
			{
				p[i]=p[i]*p[i]+p[j]*p[j];
			}
			//print_double(dp,512);
			//wtk_debug("1112 %f %f\n",loWt[1],loWt[2]);
			double *mb = melb;
			for(i=0;i<257;i++)
			{
				for(j=0;j<64;j++)
				{
					fbank[j+1] += *mb*p[i];
//					if(j==0)
//					{
//						wtk_debug("%f %f\n",*mb,dp[i]);
//					}
					mb++;
				}
			}
//			for(j=1;j<65;j++)
//			{
//				if(fbank[j] < 1.0)
//				{
//					fbank[j] = 1.0;
//				}
//				fbank[j] = log(fbank[j]);
//			}
		}else
		{
			wtk_realft(info->x);
			wtk_compute_power_spec(info->x+1);
			int kk;
			int moffset,mcnt;
			if(s->cfg->NUMCHNAS == 64)
			{
				for(kk=0;kk<64;kk++)
				{
					moffset=mel_offset_16k[kk];
					mcnt=mel_cnt_16k[kk];
	#ifdef USE_BLAS
					fbank[kk+1]=cblas_sdot(mel_cut_16k[kk],mel_bank_16k+mcnt,1,info->x+moffset+1,1);
	#else
					int j;
					for(j=0;j<mel_cut_16k[kk];j++)
					{
						fbank[kk+1]+=*(mel_bank_16k+mcnt+j)*(*(info->x+moffset+1+j));
					}
	#endif
				}
			}else
			{
	            for(kk=0;kk<40;kk++)
	            {
	                moffset=mel_offset_16k2[kk];
	                mcnt=mel_cnt_16k2[kk];
	#ifdef USE_BLAS
	                fbank[kk+1]=cblas_sdot(mel_cut_16k2[kk],mel_bank_16k2+mcnt,1,info->x+moffset+1,1);
	#else
					int j;
					for(j=0;j<mel_cut_16k2[kk];j++)
					{
						fbank[kk+1]+=*(mel_bank_16k2+mcnt+j)*(*(info->x+moffset+1+j));
					}
	#endif
	            }
			}
		}
	}

	//double d1=1.0,d2=1.0;
	if(info->takeLogs)
	{
		for(bin=1;bin<=info->numChans;++bin)
		{
			t1=fbank[bin];
			if(t1<melfloor){t1=melfloor;}
			fbank[bin]=log(t1);
		}
	}
//	if(info->numChans >60)
//	{
//		wtk_vector_print(fbank);
//	}
//		exit(0);
//	{
//		static int ki=0;
//		++ki;
//		for(bin=1;bin<=info->numChans;++bin)
//		{
//			t1=fbank[bin];
//			wtk_debug("v[%d/%d]=%f\n",ki,bin,t1);
//		}
//	}

}

static float WarpFreq(float fcl, float fcu, float freq, float minFreq, float maxFreq , float alpha)
{
   if (alpha == 1.0)
   {
	  return freq;
   }else
   {
	  float scale = 1.0 / alpha;
	  float cu = fcu * 2 / (1 + scale);
	  float cl = fcl * 2 / (1 + scale);
	  float au = (maxFreq - cu * scale) / (maxFreq - cu);
	  float al = (cl * scale - minFreq) / (cl - minFreq);
	  if (freq > cu)
		 return  au * (freq - cu) + scale * cu ;
	  else if (freq < cl)
		 return al * (freq - minFreq) + minFreq ;
	  else
		 return scale * freq ;
   }
}

void FBankInfo_print(FBankInfo* fb)
{
	printf("usePower: %d\n",fb->usePower);
}


void wtk_parm_init_fbankinfo(FBankInfo* fb,wtk_fextra_cfg_t* cfg)
{
	int Nby2,maxChan,k,chan;
	float t,mlo,mhi,ms,alpha,melk;

	fb->frameSize=cfg->frame_size;
	fb->numChans=cfg->NUMCHNAS;
	fb->sampPeriod=cfg->src_sample_rate;
	fb->usePower=cfg->USEPOWER;
	//fb->takeLogs=cfg->use_mfcc;
	//(btgt == PLP) ? FALSE : btgt != MELSPEC,
	fb->takeLogs=(cfg->base_kind==PLP)?0:(cfg->base_kind!=MELSPEC);
	fb->fftN=2;
	k=cfg->frame_size;
	while(k>fb->fftN)
	{
		fb->fftN*=2;
	}
	if(cfg->DOUBLEFFT)
	{
		fb->fftN*=2;
	}
	fb->fres=1.0E7/(fb->sampPeriod*fb->fftN*700.0);
	Nby2=fb->fftN/2;
	fb->klo=2;fb->khi=Nby2;
	mlo=0;mhi=Mel(Nby2+1,fb->fres);
	t=cfg->LOFREQ;
	if(t>=0.0)
	{
		mlo=1127*log(1+t/700.0);
		fb->klo=(int)(t*fb->sampPeriod*1.0E-7*fb->fftN+2.5);
		if(fb->klo<2){fb->klo=2;}
	}
	t=cfg->HIFREQ;
	if(t>=0.0)
	{
		mhi=1127*log(1+t/700.0);
		fb->khi=(int)(t*fb->sampPeriod*1.0E-7*fb->fftN+0.5);
		if(fb->khi>Nby2){fb->khi=Nby2;}
	}
	maxChan=fb->numChans+1;
	fb->cf=wtk_vector_new(maxChan);
	ms=mhi-mlo;
	alpha=cfg->WARPFREQ;
	if(alpha==1.0)
	{
#ifdef USE_STEP
		float sf=mlo;
		float mf=ms/maxChan;
#endif
		for(chan=1;chan<=maxChan;++chan)
		{
#ifdef USE_STEP
			sf+=mf;
			fb->cf[chan]=sf;
#else
			fb->cf[chan] = ((float)chan/(float)maxChan)*ms + mlo;
#endif
		}
	}else
	{
		float minFreq=700.0*(exp(mlo/1127.0)-1.0);
		float maxFreq=700.0*(exp(mhi/1127.0)-1.0);
		float mf,cf,wf,sf;
		float warpLowcut=cfg->WARPLCUTOFF;
		float warpUpCut=cfg->WARPUCUTOFF;

		sf=mlo;mf=ms/maxChan;
		for(chan=1;chan<=maxChan;++chan)
		{
			sf+=mf;
			cf=700*(exp(sf/1127.0)-1.0);
			wf=WarpFreq(warpLowcut,warpUpCut,cf,minFreq,maxFreq,alpha);
			fb->cf[chan]=1127.0*log(1.0+wf/700.0);
		}
	}
	//wtk_vector_print(fb->cf);
	fb->loChan=wtk_short_vector_new(Nby2);
	/* Create loChan map, loChan[fftindex] -> lower channel index */
	for(k=1,chan=1;k<=Nby2;++k)
	{
		if(k<fb->klo || k>fb->khi)
		{
			fb->loChan[k]=-1;
		}else
		{
			melk=Mel(k,fb->fres);
			while(fb->cf[chan]<melk && chan<=maxChan)
			{
				++chan;
			}
			fb->loChan[k]=chan-1;
		}
	}
	//wtk_short_vector_print(fb->loChan);
	fb->loWt=wtk_vector_new(Nby2);
	for(k=1;k<=Nby2;++k)
	{
		chan=fb->loChan[k];
		if(k<fb->klo || k>fb->khi)
		{
			fb->loWt[k]=0;
		}else
		{
			if(chan>0)
			{
				fb->loWt[k]=(fb->cf[chan+1]-Mel(k,fb->fres))/(fb->cf[chan+1]-fb->cf[chan]);
			}else
			{
				fb->loWt[k]=(fb->cf[1]-Mel(k,fb->fres))/(fb->cf[1]-mlo);
			}
		}
	}
	fb->x=wtk_vector_new(fb->fftN);
}

void wtk_param_clean_fbankinfo(FBankInfo* fb)
{
	wtk_vector_delete(fb->x);
	wtk_vector_delete(fb->loWt);
	wtk_vector_delete(fb->cf);
	wtk_free(fb->loChan);
}

void wtk_sigp_init_plp(FBankInfo* fb, int lpcOrder, wtk_vector_t* eql,wtk_double_matrix_t*cm)
{
   int i,j;
   double baseAngle;
   float f_hz_mid, fsub, fsq;
   int  nAuto, nFreq;

   /* Create the equal-loudness curve */
   for (i=1; i<=fb->numChans; i++)
   {
	  f_hz_mid = 700*(exp(fb->cf[i]/1127)-1); /* Mel to Hz conversion */
	  fsq = (f_hz_mid * f_hz_mid);
	  fsub = fsq / (fsq + 1.6e5);
	  eql[i] = fsub * fsub * ((fsq + 1.44e6)  /(fsq + 9.61e6));
   }
   /* Builds up matrix of cosines for IDFT */
   nAuto = lpcOrder+1;
   nFreq = fb->numChans+2;
   baseAngle =  PI / (double)(nFreq - 1);
   for (i=0; i<nAuto; i++)
   {
	  cm[i+1][1] = 1.0;
	  for (j=1; j<(nFreq-1); j++)
	  {
		 cm[i+1][j+1] = 2.0 * cos(baseAngle * (double)i * (double)j);
	  }
	  cm[i+1][nFreq] = cos(baseAngle * (double)i * (double)(nFreq-1));
   }
}


int wtk_sigp_init(wtk_sigp_t *s,wtk_fextra_cfg_t *cfg)
{
	memset(s,0,sizeof(*s));
	s->ham=0;
    s->povey=0;
	s->cfg=cfg;
	wtk_parm_init_fbankinfo(&(s->fbInfo),cfg);
	s->fbank=wtk_vector_new(cfg->NUMCHNAS);
	s->rfft=wtk_rfft_new(s->fbInfo.fftN/2);
	s->rfft_fft=(float*)wtk_malloc(s->rfft->len*sizeof(float));
	switch(cfg->base_kind)
	{
	case FBANK:
		break;
	case MFCC:
		s->c=wtk_vector_new(cfg->NUMCEPS+1);
		FBank2MFCC(s->fbank,s->c,cfg->NUMCEPS);
		break;
	//case PLP:
	default:
		s->c=wtk_vector_new(cfg->NUMCEPS+1);
		s->as=wtk_vector_new(cfg->NUMCHNAS+2);
		s->eql=wtk_vector_new(cfg->NUMCHNAS);
		s->ac=wtk_vector_new(cfg->LPCORDER+1);
		s->acb=wtk_vector_new(cfg->LPCORDER);
		s->lp=wtk_vector_new(cfg->LPCORDER+1);
		s->cm=wtk_double_matrix_new(cfg->LPCORDER+1,cfg->NUMCHNAS+2);
		wtk_sigp_init_plp(&(s->fbInfo),cfg->LPCORDER,s->eql,s->cm);
		break;
		/*
	default:
		wtk_debug("unsported tgt type[%d]\n",cfg->base_kind);
		return -1;
		break;
		*/
	}
	return 0;
}

int wtk_sigp_clean(wtk_sigp_t *s)
{
	wtk_rfft_delete(s->rfft);
	wtk_free(s->rfft_fft);
	wtk_param_clean_fbankinfo(&(s->fbInfo));
	//wtk_heap_delete(s->heap);
	wtk_vector_delete(s->fbank);
	if(s->ham)
	{
		wtk_vector_delete(s->ham);
	}
    if(s->povey)
    {   
        wtk_vector_delete(s->povey);
    }
	if(s->cepWin)
	{
		wtk_vector_delete(s->cepWin);
	}
	wtk_free(s->c);
	if(s->cm){wtk_free(s->cm);}
	if(s->as){wtk_free(s->as);}
	if(s->eql){wtk_free(s->eql);}
	if(s->ac){wtk_free(s->ac);}
	if(s->acb){wtk_free(s->acb);}
	if(s->lp){wtk_free(s->lp);}
	return 0;
}

void wtk_sigp_ham(wtk_sigp_t *sigp,wtk_vector_t *v)
{
	int frame_size;

	frame_size=wtk_vector_size(v);
	if(!sigp->ham || wtk_vector_size(sigp->ham)<frame_size)
	{
		if(sigp->ham)
		{
			wtk_vector_delete(sigp->ham);
		}
		sigp->ham=wtk_math_create_ham_window(frame_size);
	}
	wtk_vector_do_i(v,,*=sigp->ham[i]);
}

void wtk_sigp_povey(wtk_sigp_t *sigp,wtk_vector_t *v)
{
    int frame_size;

    frame_size=wtk_vector_size(v);
    if(!sigp->povey || wtk_vector_size(sigp->povey)<frame_size)
    {
        if(sigp->povey)
        {   
            wtk_vector_delete(sigp->povey);
        }
        sigp->povey=wtk_math_create_povey_window(frame_size);
    }
    wtk_vector_do_i(v,,*=sigp->povey[i]);
}

void wtk_sigp_create_cepwin(wtk_sigp_t* s,int cepLiftering,int count)
{
	int i;
	float a,Lby2;
	wtk_vector_t *cepWin=s->cepWin;

	if(!cepWin || wtk_vector_size(cepWin)<count)
	{
		if(s->cepWin)
		{
			wtk_vector_delete(s->cepWin);
		}
		s->cepWin=cepWin=wtk_vector_new(count);
	}
	a=PI/cepLiftering;
	Lby2=cepLiftering/2.0;
	for(i=1;i<=count;++i)
	{
		cepWin[i]=1.0+Lby2*sin(i*a);
	}
	s->cepWinL=cepLiftering;
	s->cepWinSize=count;
}

void wtk_sigp_WeightCepstrum(wtk_sigp_t *s,wtk_vector_t *c)
{
	wtk_vector_t *cepWin;
	int start,count,cepLiftering;
	int i,j;

	start=1;count=s->cfg->NUMCEPS;cepLiftering=s->cfg->CEPLIFTER;
	if(s->cepWinL != cepLiftering || count > s->cepWinSize)
	{
		wtk_sigp_create_cepwin(s,cepLiftering,count);
	}
	cepWin=s->cepWin;
	for(i=1,j=start;i<=count;++i,++j)
	{
		c[j]*=cepWin[i];
	}
}


float RandUniform(int *seed)
{
	return (wtk_math_rand_r((unsigned int*)seed)+1.0)/(2147483647.0+2.0);
}

float wtk_rand_gauss(int *seed)
{
	return sqrtf(-2.0*logf(RandUniform(seed)))*cosf(2*M_PI*RandUniform(seed));
}

float wtk_sigp_pre(wtk_sigp_t *s,wtk_vector_t* v)
{
	wtk_fextra_cfg_t *cfg=s->cfg;
	float energy=0;
	cfg=s->cfg;
	//if(cfg->dither)
//	{
//		int seed=rand()+27437;
//		int n=wtk_vector_size(v);
//		//wtk_debug("%d\n",n);
//		int i;
//		for(i=1;i<=n;i++)
//		{
//			v[i]+=wtk_rand_gauss(&seed);
//		}
//	}
	if(cfg->ZMEANSOURCE)
	{
		wtk_vector_zero_mean_frame(v);
	}
	if(cfg->ENERGY && cfg->RAWENERGY)
	{
		wtk_vector_do_i(v,energy+=,*v[i]);
	}
	if(cfg->PREMCOEF)
	{
		wtk_vector_pre_emphasise(v,cfg->PREMCOEF);
	}
    if(cfg->USEPOVEY)
    {    
         wtk_sigp_povey(s,v);
    }else if(cfg->USEHAMMING)
    {    
         wtk_sigp_ham(s,v);
    }
	if(cfg->ENERGY && !cfg->RAWENERGY)
	{
		wtk_vector_do_i(v,energy+=,*v[i]);
	}
	return energy;
}

int wtk_sigp_procss(wtk_sigp_t *s,wtk_vector_t *v,float *feature)
{
	wtk_fextra_cfg_t *cfg=s->cfg;
	int i;
	float t,energy;
	int mfcc=0;
	int fbank=0;
	wtk_vector_t *b;
	int bsize;

//wtk_vector_print(v);
	//mfcc=cfg->use_mfcc;
	energy=wtk_sigp_pre(s,v);
	Wave2Fbank(s,v, s->fbank, &(s->fbInfo));
//wtk_debug("sip\n");
//wtk_vector_print(v);
	switch(cfg->base_kind)
	{
	case FBANK:
		b=s->fbank;
		fbank=1;
		bsize=cfg->NUMCHNAS;
		break;
	case MFCC:
		mfcc=1;
		FBank2MFCC(s->fbank,s->c,cfg->NUMCEPS);
		b=s->c;
		bsize=cfg->NUMCEPS;
		break;
	//case PLP:
	default:
		FBank2ASpec(s->fbank,s->as,s->eql,cfg->COMPRESSFACT,&(s->fbInfo));
		ASpec2LPCep(s->as,s->ac,s->acb,s->lp,s->c,s->cm);
		b=s->c;
		bsize=cfg->NUMCEPS;
		break;
		/*
	default:
		wtk_debug("unsported tgt type[%d]\n",cfg->base_kind);
		return -1;
		break;
		*/
	}
	if(fbank==0)
	{
		if(cfg->CEPLIFTER>0)
		{
			wtk_sigp_WeightCepstrum(s,s->c);//,s->c,1,s->numCepCoef,s->cepLifter);
		}
	}
	t=cfg->CEPSCALE;
	for(i=1;i<=bsize;++i)
	{	
		*feature++=b[i]*t;
	}

	if(cfg->Zero)
	{
		if(mfcc)
		{
			*feature++ = FBank2C0(s->fbank) * t;
		}else
		{
			*feature++ = b[bsize+1] * t;
		}
	}
	if(cfg->ENERGY)
	{
		*feature++ = (energy<MINLARG) ? LZERO : log(energy);
		//wtk_debug("e=%f,loge=%f\n",energy,log(energy));
	}
	return 0;
	//return s->mfcc?wtk_sigp_procss_mfcc(s,v,feature):wtk_sigp_procss_plp(s,v,feature);
}


