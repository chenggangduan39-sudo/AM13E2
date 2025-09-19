#include "wtk_gmmvad.h" 

// Spectrum Weighting
static const int16_t kSpectrumWeight[kNumChannels] = { 6, 8, 10, 12, 14, 16 };
static const int16_t kNoiseUpdateConst = 655; // Q15
static const int16_t kSpeechUpdateConst = 6554; // Q15
static const int16_t kBackEta = 154; // Q8
// Minimum difference between the two models, Q5
static const int16_t kMinimumDifference[kNumChannels] = {
    544, 544, 576, 576, 576, 576 };
// Upper limit of mean value for speech model, Q7
static const int16_t kMaximumSpeech[kNumChannels] = {
    11392, 11392, 11520, 11520, 11520, 11520 };
// Minimum value for mean value
static const int16_t kMinimumMean[kNumGaussians] = { 640, 768 };
// Upper limit of mean value for noise model, Q7
static const int16_t kMaximumNoise[kNumChannels] = {
    9216, 9088, 8960, 8832, 8704, 8576 };
// Start values for the Gaussian models, Q7
// Weights for the two Gaussians for the six channels (noise)
static const int16_t kNoiseDataWeights[kTableSize] = {
    34, 62, 72, 66, 53, 25, 94, 66, 56, 62, 75, 103 };
// Weights for the two Gaussians for the six channels (speech)
static const int16_t kSpeechDataWeights[kTableSize] = {
    48, 82, 45, 87, 50, 47, 80, 46, 83, 41, 78, 81 };
// Means for the two Gaussians for the six channels (noise)
static const int16_t kNoiseDataMeans[kTableSize] = {
    6738, 4892, 7065, 6715, 6771, 3369, 7646, 3863, 7820, 7266, 5020, 4362 };
// Means for the two Gaussians for the six channels (speech)
static const int16_t kSpeechDataMeans[kTableSize] = {
    8306, 10085, 10078, 11823, 11843, 6309, 9473, 9571, 10879, 7581, 8180, 7483
};
// Stds for the two Gaussians for the six channels (noise)
static const int16_t kNoiseDataStds[kTableSize] = {
    378, 1064, 493, 582, 688, 593, 474, 697, 475, 688, 421, 455 };
// Stds for the two Gaussians for the six channels (speech)
static const int16_t kSpeechDataStds[kTableSize] = {
    555, 505, 567, 524, 585, 1231, 509, 828, 492, 1540, 1079, 850 };

// Constants used in GmmProbability().
//
// Maximum number of counted speech (VAD = 1) frames in a row.
static const int16_t kMaxSpeechFrames = 6;
// Minimum standard deviation for both speech and noise.
static const int16_t kMinStd = 384;

// Constants in WebRtcVad_InitCore().
// Default aggressiveness mode.

// Constants used in WebRtcVad_set_mode_core().
//
// Thresholds for different frame lengths (10 ms, 20 ms and 30 ms).
//
// Mode 0, Quality.
static const int16_t kOverHangMax1Q[3] = { 8, 4, 3 };
static const int16_t kOverHangMax2Q[3] = { 14, 7, 5 };
static const int16_t kLocalThresholdQ[3] = { 24, 21, 24 };
static const int16_t kGlobalThresholdQ[3] = { 57, 48, 57 };
// Mode 1, Low bitrate.
static const int16_t kOverHangMax1LBR[3] = { 8, 4, 3 };
static const int16_t kOverHangMax2LBR[3] = { 14, 7, 5 };
static const int16_t kLocalThresholdLBR[3] = { 37, 32, 37 };
static const int16_t kGlobalThresholdLBR[3] = { 100, 80, 100 };
// Mode 2, Aggressive.
static const int16_t kOverHangMax1AGG[3] = { 6, 3, 2 };
static const int16_t kOverHangMax2AGG[3] = { 9, 5, 3 };
static const int16_t kLocalThresholdAGG[3] = { 82, 78, 82 };
static const int16_t kGlobalThresholdAGG[3] = { 285, 260, 285 };
// Mode 3, Very aggressive.
static const int16_t kOverHangMax1VAG[3] = { 6, 3, 2 };
static const int16_t kOverHangMax2VAG[3] = { 9, 5, 3 };
static const int16_t kLocalThresholdVAG[3] = { 94, 94, 94 };
static const int16_t kGlobalThresholdVAG[3] = { 1100, 1050, 1100 };

// Allpass filter coefficients, upper and lower, in Q13.
// Upper: 0.64, Lower: 0.17.
static const int16_t kAllPassCoefsQ13[2] = { 5243, 1392 };  // Q13.
static const int16_t kSmoothingDown = 6553;  // 0.2 in Q15.
static const int16_t kSmoothingUp = 32439;  // 0.99 in Q15.

// TODO(bjornv): Move this function to vad_filterbank.c.
// Downsampling filter based on splitting filter and allpass functions.
void wtk_downsampling(const int16_t* signal_in,
                            int16_t* signal_out,
                            int32_t* filter_state,
                            size_t in_length) {
  int16_t tmp16_1 = 0, tmp16_2 = 0;
  int32_t tmp32_1 = filter_state[0];
  int32_t tmp32_2 = filter_state[1];
  size_t n = 0;
  // Downsampling by 2 gives half length.
  size_t half_length = (in_length >> 1);

  // Filter coefficients in Q13, filter state in Q0.
  for (n = 0; n < half_length; n++) {
    // All-pass filtering upper branch.
    tmp16_1 = (int16_t) ((tmp32_1 >> 1) +
        ((kAllPassCoefsQ13[0] * *signal_in) >> 14));
    *signal_out = tmp16_1;
    tmp32_1 = (int32_t)(*signal_in++) - ((kAllPassCoefsQ13[0] * tmp16_1) >> 12);

    // All-pass filtering lower branch.
    tmp16_2 = (int16_t) ((tmp32_2 >> 1) +
        ((kAllPassCoefsQ13[1] * *signal_in) >> 14));
    *signal_out++ += tmp16_2;
    tmp32_2 = (int32_t)(*signal_in++) - ((kAllPassCoefsQ13[1] * tmp16_2) >> 12);
  }
  // Store the filter states.
  filter_state[0] = tmp32_1;
  filter_state[1] = tmp32_2;
}

wtk_gmmvad_t* wtk_gmmvad_new(wtk_gmmvad_cfg_t *cfg)
{
	wtk_gmmvad_t *v;

	v=(wtk_gmmvad_t*)wtk_malloc(sizeof(wtk_gmmvad_t));
	v->cfg=cfg;
	wtk_gmmvad_reset(v);
	return v;
}

void wtk_gmmvad_delete(wtk_gmmvad_t *v)
{
	wtk_free(v);
}

int wtk_gmmvad_set_mode(wtk_gmmvad_t *v,int mode)
{
	  int ret = 0;
	  switch (mode) {
	    case 0:
	      // Quality mode.
	      memcpy(v->over_hang_max_1, kOverHangMax1Q,
	             sizeof(v->over_hang_max_1));
	      memcpy(v->over_hang_max_2, kOverHangMax2Q,
	             sizeof(v->over_hang_max_2));
	      memcpy(v->individual, kLocalThresholdQ,
	             sizeof(v->individual));
	      memcpy(v->total, kGlobalThresholdQ,
	             sizeof(v->total));
	      break;
	    case 1:
	      // Low bitrate mode.
	      memcpy(v->over_hang_max_1, kOverHangMax1LBR,
	             sizeof(v->over_hang_max_1));
	      memcpy(v->over_hang_max_2, kOverHangMax2LBR,
	             sizeof(v->over_hang_max_2));
	      memcpy(v->individual, kLocalThresholdLBR,
	             sizeof(v->individual));
	      memcpy(v->total, kGlobalThresholdLBR,
	             sizeof(v->total));
	      break;
	    case 2:
	      // Aggressive mode.
	      memcpy(v->over_hang_max_1, kOverHangMax1AGG,
	             sizeof(v->over_hang_max_1));
	      memcpy(v->over_hang_max_2, kOverHangMax2AGG,
	             sizeof(v->over_hang_max_2));
	      memcpy(v->individual, kLocalThresholdAGG,
	             sizeof(v->individual));
	      memcpy(v->total, kGlobalThresholdAGG,
	             sizeof(v->total));
	      break;
	    case 3:
	      // Very aggressive mode.
	      memcpy(v->over_hang_max_1, kOverHangMax1VAG,
	             sizeof(v->over_hang_max_1));
	      memcpy(v->over_hang_max_2, kOverHangMax2VAG,
	             sizeof(v->over_hang_max_2));
	      memcpy(v->individual, kLocalThresholdVAG,
	             sizeof(v->individual));
	      memcpy(v->total, kGlobalThresholdVAG,
	             sizeof(v->total));
	      break;
	    default:
	      ret=-1;
	      break;
	  }

	  return ret;
}

void wtk_gmmvad_reset(wtk_gmmvad_t *v)
{
	int i;

	v->frame_counter=0;
	v->over_hang=0;
	v->num_of_speech=0;
	memset(v->downsampling_filter_state,0,sizeof(v->downsampling_filter_state));
	// Read initial PDF parameters.
	for(i=0;i<kTableSize;++i)
	{
	    v->noise_means[i] = kNoiseDataMeans[i];
	    v->speech_means[i] = kSpeechDataMeans[i];
	    v->noise_stds[i] = kNoiseDataStds[i];
	    v->speech_stds[i] = kSpeechDataStds[i];
	}
	// Initialize Index and Minimum value vectors.
	for(i = 0; i < 16 * kNumChannels; i++)
	{
		v->low_value_vector[i] = 10000;
		v->index_vector[i] = 0;
	}
	// Initialize splitting filter states.
	memset(v->upper_state, 0, sizeof(v->upper_state));
	memset(v->lower_state, 0, sizeof(v->lower_state));
	// Initialize high pass filter states.
	memset(v->hp_filter_state, 0, sizeof(v->hp_filter_state));
	  // Initialize mean value memory, for WebRtcVad_FindMinimum().
	  for (i = 0; i < kNumChannels; i++) {
	    v->mean_value[i] = 1600;
	  }
	  wtk_gmmvad_set_mode(v, v->cfg->mode);
}

// Constants used in LogOfEnergy().
static const int16_t kLogConst = 24660;  // 160*log10(2) in Q9.
static const int16_t kLogEnergyIntPart = 14336;  // 14 in Q10

// Coefficients used by HighPassFilter, Q14.
static const int16_t kHpZeroCoefs[3] = { 6631, -13262, 6631 };
static const int16_t kHpPoleCoefs[3] = { 16384, -7756, 5620 };

// Allpass filter coefficients, upper and lower, in Q15.
// Upper: 0.64, Lower: 0.17
static const int16_t kAllPassCoefsQ15[2] = { 20972, 5571 };

// Adjustment for division with two in SplitFilter.
static const int16_t kOffsetVector[6] = { 368, 368, 272, 176, 176, 176 };

// All pass filtering of |data_in|, used before splitting the signal into two
// frequency bands (low pass vs high pass).
// Note that |data_in| and |data_out| can NOT correspond to the same address.
//
// - data_in            [i]   : Input audio signal given in Q0.
// - data_length        [i]   : Length of input and output data.
// - filter_coefficient [i]   : Given in Q15.
// - filter_state       [i/o] : State of the filter given in Q(-1).
// - data_out           [o]   : Output audio signal given in Q(-1).
static void AllPassFilter(const int16_t* data_in, size_t data_length,
                          int16_t filter_coefficient, int16_t* filter_state,
                          int16_t* data_out) {
  // The filter can only cause overflow (in the w16 output variable)
  // if more than 4 consecutive input numbers are of maximum value and
  // has the the same sign as the impulse responses first taps.
  // First 6 taps of the impulse response:
  // 0.6399 0.5905 -0.3779 0.2418 -0.1547 0.0990

  size_t i;
  int16_t tmp16 = 0;
  int32_t tmp32 = 0;
  int32_t state32 = ((int32_t) (*filter_state) << 16);  // Q15

  for (i = 0; i < data_length; i++) {
    tmp32 = state32 + filter_coefficient * *data_in;
    tmp16 = (int16_t) (tmp32 >> 16);  // Q(-1)
    *data_out++ = tmp16;
    state32 = (*data_in << 14) - filter_coefficient * tmp16;  // Q14
    state32 <<= 1;  // Q15.
    data_in += 2;
  }

  *filter_state = (int16_t) (state32 >> 16);  // Q(-1)
}

static __inline int wtk_NormW32(int32_t a) {
  int zeros;

  if (a == 0) {
    return 0;
  }
  else if (a < 0) {
    a = ~a;
  }

  if (!(0xFFFF8000 & a)) {
    zeros = 16;
  } else {
    zeros = 0;
  }
  if (!(0xFF800000 & (a << zeros))) zeros += 8;
  if (!(0xF8000000 & (a << zeros))) zeros += 4;
  if (!(0xE0000000 & (a << zeros))) zeros += 2;
  if (!(0xC0000000 & (a << zeros))) zeros += 1;

  return zeros;
}

static __inline int wtk_NormU32(uint32_t a) {
  int zeros;

  if (a == 0) return 0;

  if (!(0xFFFF0000 & a)) {
    zeros = 16;
  } else {
    zeros = 0;
  }
  if (!(0xFF000000 & (a << zeros))) zeros += 8;
  if (!(0xF0000000 & (a << zeros))) zeros += 4;
  if (!(0xC0000000 & (a << zeros))) zeros += 2;
  if (!(0x80000000 & (a << zeros))) zeros += 1;

  return zeros;
}


// Splits |data_in| into |hp_data_out| and |lp_data_out| corresponding to
// an upper (high pass) part and a lower (low pass) part respectively.
//
// - data_in      [i]   : Input audio data to be split into two frequency bands.
// - data_length  [i]   : Length of |data_in|.
// - upper_state  [i/o] : State of the upper filter, given in Q(-1).
// - lower_state  [i/o] : State of the lower filter, given in Q(-1).
// - hp_data_out  [o]   : Output audio data of the upper half of the spectrum.
//                        The length is |data_length| / 2.
// - lp_data_out  [o]   : Output audio data of the lower half of the spectrum.
//                        The length is |data_length| / 2.
static void SplitFilter(const int16_t* data_in, size_t data_length,
                        int16_t* upper_state, int16_t* lower_state,
                        int16_t* hp_data_out, int16_t* lp_data_out) {
  size_t i;
  size_t half_length = data_length >> 1;  // Downsampling by 2.
  int16_t tmp_out;

  // All-pass filtering upper branch.
  AllPassFilter(&data_in[0], half_length, kAllPassCoefsQ15[0], upper_state,
                hp_data_out);

  // All-pass filtering lower branch.
  AllPassFilter(&data_in[1], half_length, kAllPassCoefsQ15[1], lower_state,
                lp_data_out);

  // Make LP and HP signals.
  for (i = 0; i < half_length; i++) {
    tmp_out = *hp_data_out;
    *hp_data_out++ -= *lp_data_out;
    *lp_data_out++ += tmp_out;
  }
}

static __inline int16_t wtk_GetSizeInBits(uint32_t n) {
  int bits;

  if (0xFFFF0000 & n) {
    bits = 16;
  } else {
    bits = 0;
  }
  if (0x0000FF00 & (n >> bits)) bits += 8;
  if (0x000000F0 & (n >> bits)) bits += 4;
  if (0x0000000C & (n >> bits)) bits += 2;
  if (0x00000002 & (n >> bits)) bits += 1;
  if (0x00000001 & (n >> bits)) bits += 1;

  return bits;
}

#define WEBRTC_SPL_MUL(a, b) \
    ((int32_t) ((int32_t)(a) * (int32_t)(b)))

int16_t wtk_GetScalingSquare(int16_t* in_vector,
                                   size_t in_vector_length,
                                   size_t times)
{
    int16_t nbits = wtk_GetSizeInBits((uint32_t)times);
    size_t i;
    int16_t smax = -1;
    int16_t sabs;
    int16_t *sptr = in_vector;
    int16_t t;
    size_t looptimes = in_vector_length;

    for (i = looptimes; i > 0; i--)
    {
        sabs = (*sptr > 0 ? *sptr : -*sptr);
        ++sptr;
        smax = (sabs > smax) ? sabs : smax;
    }
    t = wtk_NormW32(WEBRTC_SPL_MUL(smax, smax));

    if (smax == 0)
    {
        return 0; // Since norm(0) returns 0
    } else
    {
        return (t > nbits) ? 0 : nbits - t;
    }
}

int32_t wtk_Energy(int16_t* vector,
                         size_t vector_length,
                         int* scale_factor)
{
    int32_t en = 0;
    size_t i;
    int scaling =
        wtk_GetScalingSquare(vector, vector_length, vector_length);
    size_t looptimes = vector_length;
    int16_t *vectorptr = vector;

    for (i = 0; i < looptimes; i++)
    {
      en += (*vectorptr * *vectorptr) >> scaling;
      vectorptr++;
    }
    *scale_factor = scaling;

    return en;
}


// Calculates the energy of |data_in| in dB, and also updates an overall
// |total_energy| if necessary.
//
// - data_in      [i]   : Input audio data for energy calculation.
// - data_length  [i]   : Length of input data.
// - offset       [i]   : Offset value added to |log_energy|.
// - total_energy [i/o] : An external energy updated with the energy of
//                        |data_in|.
//                        NOTE: |total_energy| is only updated if
//                        |total_energy| <= |kMinEnergy|.
// - log_energy   [o]   : 10 * log10("energy of |data_in|") given in Q4.
static void LogOfEnergy(const int16_t* data_in, size_t data_length,
                        int16_t offset, int16_t* total_energy,
                        int16_t* log_energy) {
  // |tot_rshifts| accumulates the number of right shifts performed on |energy|.
  int tot_rshifts = 0;
  // The |energy| will be normalized to 15 bits. We use unsigned integer because
  // we eventually will mask out the fractional part.
  uint32_t energy = 0;

//  assert(data_in != NULL);
//  assert(data_length > 0);

  energy = (uint32_t) wtk_Energy((int16_t*) data_in, data_length,
                                       &tot_rshifts);

  if (energy != 0) {
    // By construction, normalizing to 15 bits is equivalent with 17 leading
    // zeros of an unsigned 32 bit value.
    int normalizing_rshifts = 17 - wtk_NormU32(energy);
    // In a 15 bit representation the leading bit is 2^14. log2(2^14) in Q10 is
    // (14 << 10), which is what we initialize |log2_energy| with. For a more
    // detailed derivations, see below.
    int16_t log2_energy = kLogEnergyIntPart;

    tot_rshifts += normalizing_rshifts;
    // Normalize |energy| to 15 bits.
    // |tot_rshifts| is now the total number of right shifts performed on
    // |energy| after normalization. This means that |energy| is in
    // Q(-tot_rshifts).
    if (normalizing_rshifts < 0) {
      energy <<= -normalizing_rshifts;
    } else {
      energy >>= normalizing_rshifts;
    }

    // Calculate the energy of |data_in| in dB, in Q4.
    //
    // 10 * log10("true energy") in Q4 = 2^4 * 10 * log10("true energy") =
    // 160 * log10(|energy| * 2^|tot_rshifts|) =
    // 160 * log10(2) * log2(|energy| * 2^|tot_rshifts|) =
    // 160 * log10(2) * (log2(|energy|) + log2(2^|tot_rshifts|)) =
    // (160 * log10(2)) * (log2(|energy|) + |tot_rshifts|) =
    // |kLogConst| * (|log2_energy| + |tot_rshifts|)
    //
    // We know by construction that |energy| is normalized to 15 bits. Hence,
    // |energy| = 2^14 + frac_Q15, where frac_Q15 is a fractional part in Q15.
    // Further, we'd like |log2_energy| in Q10
    // log2(|energy|) in Q10 = 2^10 * log2(2^14 + frac_Q15) =
    // 2^10 * log2(2^14 * (1 + frac_Q15 * 2^-14)) =
    // 2^10 * (14 + log2(1 + frac_Q15 * 2^-14)) ~=
    // (14 << 10) + 2^10 * (frac_Q15 * 2^-14) =
    // (14 << 10) + (frac_Q15 * 2^-4) = (14 << 10) + (frac_Q15 >> 4)
    //
    // Note that frac_Q15 = (|energy| & 0x00003FFF)

    // Calculate and add the fractional part to |log2_energy|.
    log2_energy += (int16_t) ((energy & 0x00003FFF) >> 4);

    // |kLogConst| is in Q9, |log2_energy| in Q10 and |tot_rshifts| in Q0.
    // Note that we in our derivation above have accounted for an output in Q4.
    *log_energy = (int16_t)(((kLogConst * log2_energy) >> 19) +
        ((tot_rshifts * kLogConst) >> 9));

    if (*log_energy < 0) {
      *log_energy = 0;
    }
  } else {
    *log_energy = offset;
    return;
  }

  *log_energy += offset;

  // Update the approximate |total_energy| with the energy of |data_in|, if
  // |total_energy| has not exceeded |kMinEnergy|. |total_energy| is used as an
  // energy indicator in WebRtcVad_GmmProbability() in vad_core.c.
  if (*total_energy <= kMinEnergy) {
    if (tot_rshifts >= 0) {
      // We know by construction that the |energy| > |kMinEnergy| in Q0, so add
      // an arbitrary value such that |total_energy| exceeds |kMinEnergy|.
      *total_energy += kMinEnergy + 1;
    } else {
      // By construction |energy| is represented by 15 bits, hence any number of
      // right shifted |energy| will fit in an int16_t. In addition, adding the
      // value to |total_energy| is wrap around safe as long as
      // |kMinEnergy| < 8192.
      *total_energy += (int16_t) (energy >> -tot_rshifts);  // Q0.
    }
  }
}

// High pass filtering, with a cut-off frequency at 80 Hz, if the |data_in| is
// sampled at 500 Hz.
//
// - data_in      [i]   : Input audio data sampled at 500 Hz.
// - data_length  [i]   : Length of input and output data.
// - filter_state [i/o] : State of the filter.
// - data_out     [o]   : Output audio data in the frequency interval
//                        80 - 250 Hz.
static void HighPassFilter(const int16_t* data_in, size_t data_length,
                           int16_t* filter_state, int16_t* data_out) {
  size_t i;
  const int16_t* in_ptr = data_in;
  int16_t* out_ptr = data_out;
  int32_t tmp32 = 0;


  // The sum of the absolute values of the impulse response:
  // The zero/pole-filter has a max amplification of a single sample of: 1.4546
  // Impulse response: 0.4047 -0.6179 -0.0266  0.1993  0.1035  -0.0194
  // The all-zero section has a max amplification of a single sample of: 1.6189
  // Impulse response: 0.4047 -0.8094  0.4047  0       0        0
  // The all-pole section has a max amplification of a single sample of: 1.9931
  // Impulse response: 1.0000  0.4734 -0.1189 -0.2187 -0.0627   0.04532

  for (i = 0; i < data_length; i++) {
    // All-zero section (filter coefficients in Q14).
    tmp32 = kHpZeroCoefs[0] * *in_ptr;
    tmp32 += kHpZeroCoefs[1] * filter_state[0];
    tmp32 += kHpZeroCoefs[2] * filter_state[1];
    filter_state[1] = filter_state[0];
    filter_state[0] = *in_ptr++;

    // All-pole section (filter coefficients in Q14).
    tmp32 -= kHpPoleCoefs[1] * filter_state[2];
    tmp32 -= kHpPoleCoefs[2] * filter_state[3];
    filter_state[3] = filter_state[2];
    filter_state[2] = (int16_t) (tmp32 >> 14);
    *out_ptr++ = filter_state[2];
  }
}


int16_t wtk_gmmvad_calc_feature(wtk_gmmvad_t* vad, const int16_t* data_in,
                                    size_t data_length, int16_t* features) {
  int16_t total_energy = 0;
  // We expect |data_length| to be 80, 160 or 240 samples, which corresponds to
  // 10, 20 or 30 ms in 8 kHz. Therefore, the intermediate downsampled data will
  // have at most 120 samples after the first split and at most 60 samples after
  // the second split.
  int16_t hp_120[120], lp_120[120];
  int16_t hp_60[60], lp_60[60];
  const size_t half_data_length = data_length >> 1;
  size_t length = half_data_length;  // |data_length| / 2, corresponds to
                                     // bandwidth = 2000 Hz after downsampling.

  // Initialize variables for the first SplitFilter().
  int frequency_band = 0;
  const int16_t* in_ptr = data_in;  // [0 - 4000] Hz.
  int16_t* hp_out_ptr = hp_120;  // [2000 - 4000] Hz.
  int16_t* lp_out_ptr = lp_120;  // [0 - 2000] Hz.

  //assert(data_length <= 240);
  //assert(4 < kNumChannels - 1);  // Checking maximum |frequency_band|.

  // Split at 2000 Hz and downsample.
  SplitFilter(in_ptr, data_length, &vad->upper_state[frequency_band],
              &vad->lower_state[frequency_band], hp_out_ptr, lp_out_ptr);

  // For the upper band (2000 Hz - 4000 Hz) split at 3000 Hz and downsample.
  frequency_band = 1;
  in_ptr = hp_120;  // [2000 - 4000] Hz.
  hp_out_ptr = hp_60;  // [3000 - 4000] Hz.
  lp_out_ptr = lp_60;  // [2000 - 3000] Hz.
  SplitFilter(in_ptr, length, &vad->upper_state[frequency_band],
              &vad->lower_state[frequency_band], hp_out_ptr, lp_out_ptr);

  // Energy in 3000 Hz - 4000 Hz.
  length >>= 1;  // |data_length| / 4 <=> bandwidth = 1000 Hz.

  LogOfEnergy(hp_60, length, kOffsetVector[5], &total_energy, &features[5]);

  // Energy in 2000 Hz - 3000 Hz.
  LogOfEnergy(lp_60, length, kOffsetVector[4], &total_energy, &features[4]);

  // For the lower band (0 Hz - 2000 Hz) split at 1000 Hz and downsample.
  frequency_band = 2;
  in_ptr = lp_120;  // [0 - 2000] Hz.
  hp_out_ptr = hp_60;  // [1000 - 2000] Hz.
  lp_out_ptr = lp_60;  // [0 - 1000] Hz.
  length = half_data_length;  // |data_length| / 2 <=> bandwidth = 2000 Hz.
  SplitFilter(in_ptr, length, &vad->upper_state[frequency_band],
              &vad->lower_state[frequency_band], hp_out_ptr, lp_out_ptr);

  // Energy in 1000 Hz - 2000 Hz.
  length >>= 1;  // |data_length| / 4 <=> bandwidth = 1000 Hz.
  LogOfEnergy(hp_60, length, kOffsetVector[3], &total_energy, &features[3]);

  // For the lower band (0 Hz - 1000 Hz) split at 500 Hz and downsample.
  frequency_band = 3;
  in_ptr = lp_60;  // [0 - 1000] Hz.
  hp_out_ptr = hp_120;  // [500 - 1000] Hz.
  lp_out_ptr = lp_120;  // [0 - 500] Hz.
  SplitFilter(in_ptr, length, &vad->upper_state[frequency_band],
              &vad->lower_state[frequency_band], hp_out_ptr, lp_out_ptr);

  // Energy in 500 Hz - 1000 Hz.
  length >>= 1;  // |data_length| / 8 <=> bandwidth = 500 Hz.
  LogOfEnergy(hp_120, length, kOffsetVector[2], &total_energy, &features[2]);

  // For the lower band (0 Hz - 500 Hz) split at 250 Hz and downsample.
  frequency_band = 4;
  in_ptr = lp_120;  // [0 - 500] Hz.
  hp_out_ptr = hp_60;  // [250 - 500] Hz.
  lp_out_ptr = lp_60;  // [0 - 250] Hz.
  SplitFilter(in_ptr, length, &vad->upper_state[frequency_band],
              &vad->lower_state[frequency_band], hp_out_ptr, lp_out_ptr);

  // Energy in 250 Hz - 500 Hz.
  length >>= 1;  // |data_length| / 16 <=> bandwidth = 250 Hz.
  LogOfEnergy(hp_60, length, kOffsetVector[1], &total_energy, &features[1]);

  // Remove 0 Hz - 80 Hz, by high pass filtering the lower band.
  HighPassFilter(lp_60, length, vad->hp_filter_state, hp_120);

  // Energy in 80 Hz - 250 Hz.
  LogOfEnergy(hp_120, length, kOffsetVector[0], &total_energy, &features[0]);

  return total_energy;
}

static const int32_t kCompVar = 22005;
static const int16_t kLog2Exp = 5909;  // log2(exp(1)) in Q12.

int32_t wtk_DivW32W16(int32_t num, int16_t den)
{
    // Guard against division with 0
    if (den != 0)
    {
        return (int32_t)(num / den);
    } else
    {
        return (int32_t)0x7FFFFFFF;
    }
}


// For a normal distribution, the probability of |input| is calculated and
// returned (in Q20). The formula for normal distributed probability is
//
// 1 / s * exp(-(x - m)^2 / (2 * s^2))
//
// where the parameters are given in the following Q domains:
// m = |mean| (Q7)
// s = |std| (Q7)
// x = |input| (Q4)
// in addition to the probability we output |delta| (in Q11) used when updating
// the noise/speech model.
int32_t wtk_GaussianProbability(int16_t input,
                                      int16_t mean,
                                      int16_t std,
                                      int16_t* delta) {
  int16_t tmp16, inv_std, inv_std2, exp_value = 0;
  int32_t tmp32;

  // Calculate |inv_std| = 1 / s, in Q10.
  // 131072 = 1 in Q17, and (|std| >> 1) is for rounding instead of truncation.
  // Q-domain: Q17 / Q7 = Q10.
  tmp32 = (int32_t) 131072 + (int32_t) (std >> 1);
  inv_std = (int16_t) wtk_DivW32W16(tmp32, std);

  // Calculate |inv_std2| = 1 / s^2, in Q14.
  tmp16 = (inv_std >> 2);  // Q10 -> Q8.
  // Q-domain: (Q8 * Q8) >> 2 = Q14.
  inv_std2 = (int16_t)((tmp16 * tmp16) >> 2);
  // TODO(bjornv): Investigate if changing to
  // inv_std2 = (int16_t)((inv_std * inv_std) >> 6);
  // gives better accuracy.

  tmp16 = (input << 3);  // Q4 -> Q7
  tmp16 = tmp16 - mean;  // Q7 - Q7 = Q7

  // To be used later, when updating noise/speech model.
  // |delta| = (x - m) / s^2, in Q11.
  // Q-domain: (Q14 * Q7) >> 10 = Q11.
  *delta = (int16_t)((inv_std2 * tmp16) >> 10);

  // Calculate the exponent |tmp32| = (x - m)^2 / (2 * s^2), in Q10. Replacing
  // division by two with one shift.
  // Q-domain: (Q11 * Q7) >> 8 = Q10.
  tmp32 = (*delta * tmp16) >> 9;

  // If the exponent is small enough to give a non-zero probability we calculate
  // |exp_value| ~= exp(-(x - m)^2 / (2 * s^2))
  //             ~= exp2(-log2(exp(1)) * |tmp32|).
  if (tmp32 < kCompVar) {
    // Calculate |tmp16| = log2(exp(1)) * |tmp32|, in Q10.
    // Q-domain: (Q12 * Q10) >> 12 = Q10.
    tmp16 = (int16_t)((kLog2Exp * tmp32) >> 12);
    tmp16 = -tmp16;
    exp_value = (0x0400 | (tmp16 & 0x03FF));
    tmp16 ^= 0xFFFF;
    tmp16 >>= 10;
    tmp16 += 1;
    // Get |exp_value| = exp(-|tmp32|) in Q10.
    exp_value >>= tmp16;
  }

  // Calculate and return (1 / s) * exp(-(x - m)^2 / (2 * s^2)), in Q20.
  // Q-domain: Q10 * Q10 = Q20.
  return inv_std * exp_value;
}



#define WEBRTC_SPL_WORD16_MAX       32767

// Inserts |feature_value| into |low_value_vector|, if it is one of the 16
// smallest values the last 100 frames. Then calculates and returns the median
// of the five smallest values.
int16_t wtk_gmmvad_FindMinimum(wtk_gmmvad_t* vad,
                              int16_t feature_value,
                              int channel) {
  int i = 0, j = 0;
  int position = -1;
  // Offset to beginning of the 16 minimum values in memory.
  const int offset = (channel << 4);
  int16_t current_median = 1600;
  int16_t alpha = 0;
  int32_t tmp32 = 0;
  // Pointer to memory for the 16 minimum values and the age of each value of
  // the |channel|.
  int16_t* age = &vad->index_vector[offset];
  int16_t* smallest_values = &vad->low_value_vector[offset];

  //assert(channel < kNumChannels);

  // Each value in |smallest_values| is getting 1 loop older. Update |age|, and
  // remove old values.
  for (i = 0; i < 16; i++) {
    if (age[i] != 100) {
      age[i]++;
    } else {
      // Too old value. Remove from memory and shift larger values downwards.
      for (j = i; j < 16; j++) {
        smallest_values[j] = smallest_values[j + 1];
        age[j] = age[j + 1];
      }
      age[15] = 101;
      smallest_values[15] = 10000;
    }
  }

  // Check if |feature_value| is smaller than any of the values in
  // |smallest_values|. If so, find the |position| where to insert the new value
  // (|feature_value|).
  if (feature_value < smallest_values[7]) {
    if (feature_value < smallest_values[3]) {
      if (feature_value < smallest_values[1]) {
        if (feature_value < smallest_values[0]) {
          position = 0;
        } else {
          position = 1;
        }
      } else if (feature_value < smallest_values[2]) {
        position = 2;
      } else {
        position = 3;
      }
    } else if (feature_value < smallest_values[5]) {
      if (feature_value < smallest_values[4]) {
        position = 4;
      } else {
        position = 5;
      }
    } else if (feature_value < smallest_values[6]) {
      position = 6;
    } else {
      position = 7;
    }
  } else if (feature_value < smallest_values[15]) {
    if (feature_value < smallest_values[11]) {
      if (feature_value < smallest_values[9]) {
        if (feature_value < smallest_values[8]) {
          position = 8;
        } else {
          position = 9;
        }
      } else if (feature_value < smallest_values[10]) {
        position = 10;
      } else {
        position = 11;
      }
    } else if (feature_value < smallest_values[13]) {
      if (feature_value < smallest_values[12]) {
        position = 12;
      } else {
        position = 13;
      }
    } else if (feature_value < smallest_values[14]) {
      position = 14;
    } else {
      position = 15;
    }
  }

  // If we have detected a new small value, insert it at the correct position
  // and shift larger values up.
  if (position > -1) {
    for (i = 15; i > position; i--) {
      smallest_values[i] = smallest_values[i - 1];
      age[i] = age[i - 1];
    }
    smallest_values[position] = feature_value;
    age[position] = 1;
  }

  // Get |current_median|.
  if (vad->frame_counter > 2) {
    current_median = smallest_values[2];
  } else if (vad->frame_counter > 0) {
    current_median = smallest_values[0];
  }

  // Smooth the median value.
  if (vad->frame_counter > 0) {
    if (current_median < vad->mean_value[channel]) {
      alpha = kSmoothingDown;  // 0.2 in Q15.
    } else {
      alpha = kSmoothingUp;  // 0.99 in Q15.
    }
  }
  tmp32 = (alpha + 1) * vad->mean_value[channel];
  tmp32 += (WEBRTC_SPL_WORD16_MAX - alpha) * current_median;
  tmp32 += 16384;
  vad->mean_value[channel] = (int16_t) (tmp32 >> 15);

  return vad->mean_value[channel];
}

// Calculates the weighted average w.r.t. number of Gaussians. The |data| are
// updated with an |offset| before averaging.
//
// - data     [i/o] : Data to average.
// - offset   [i]   : An offset added to |data|.
// - weights  [i]   : Weights used for averaging.
//
// returns          : The weighted average.
static int32_t WeightedAverage(int16_t* data, int16_t offset,
                               const int16_t* weights) {
  int k;
  int32_t weighted_average = 0;

  for (k = 0; k < kNumGaussians; k++) {
    data[k * kNumChannels] += offset;
    weighted_average += data[k * kNumChannels] * weights[k * kNumChannels];
  }
  return weighted_average;
}


// Calculates the probabilities for both speech and background noise using
// Gaussian Mixture Models (GMM). A hypothesis-test is performed to decide which
// type of signal is most probable.
//
// - self           [i/o] : Pointer to VAD instance
// - features       [i]   : Feature vector of length |kNumChannels|
//                          = log10(energy in frequency band)
// - total_power    [i]   : Total power in audio frame.
// - frame_length   [i]   : Number of input samples
//
// - returns              : the VAD decision (0 - noise, 1 - speech).
static int16_t wtk_gmmvad_calc_prob(wtk_gmmvad_t* vad, int16_t* features,
                              int16_t total_power, size_t frame_length) {
  int channel, k;
  int16_t feature_minimum;
  int16_t h0, h1;
  int16_t log_likelihood_ratio;
  int16_t vadflag = 0;
  int16_t shifts_h0, shifts_h1;
  int16_t tmp_s16, tmp1_s16, tmp2_s16;
  int16_t diff;
  int gaussian;
  int16_t nmk, nmk2, nmk3, smk, smk2, nsk, ssk;
  int16_t delt, ndelt;
  int16_t maxspe, maxmu;
  int16_t deltaN[kTableSize], deltaS[kTableSize];
  int16_t ngprvec[kTableSize] = { 0 };  // Conditional probability = 0.
  int16_t sgprvec[kTableSize] = { 0 };  // Conditional probability = 0.
  int32_t h0_test, h1_test;
  int32_t tmp1_s32, tmp2_s32;
  int32_t sum_log_likelihood_ratios = 0;
  int32_t noise_global_mean, speech_global_mean;
  int32_t noise_probability[kNumGaussians], speech_probability[kNumGaussians];
  int16_t overhead1, overhead2, individualTest, totalTest;

  // Set various thresholds based on frame lengths (80, 160 or 240 samples).
  if (frame_length == 80) {
    overhead1 = vad->over_hang_max_1[0];
    overhead2 = vad->over_hang_max_2[0];
    individualTest = vad->individual[0];
    totalTest = vad->total[0];
  } else if (frame_length == 160) {
    overhead1 = vad->over_hang_max_1[1];
    overhead2 = vad->over_hang_max_2[1];
    individualTest = vad->individual[1];
    totalTest = vad->total[1];
  } else {
    overhead1 = vad->over_hang_max_1[2];
    overhead2 = vad->over_hang_max_2[2];
    individualTest = vad->individual[2];
    totalTest = vad->total[2];
  }

  if (total_power > kMinEnergy) {
    // The signal power of current frame is large enough for processing. The
    // processing consists of two parts:
    // 1) Calculating the likelihood of speech and thereby a VAD decision.
    // 2) Updating the underlying model, w.r.t., the decision made.

    // The detection scheme is an LRT with hypothesis
    // H0: Noise
    // H1: Speech
    //
    // We combine a global LRT with local tests, for each frequency sub-band,
    // here defined as |channel|.
    for (channel = 0; channel < kNumChannels; channel++) {
      // For each channel we model the probability with a GMM consisting of
      // |kNumGaussians|, with different means and standard deviations depending
      // on H0 or H1.
      h0_test = 0;
      h1_test = 0;
      for (k = 0; k < kNumGaussians; k++) {
        gaussian = channel + k * kNumChannels;
        // Probability under H0, that is, probability of frame being noise.
        // Value given in Q27 = Q7 * Q20.
        tmp1_s32 = wtk_GaussianProbability(features[channel],
                                                 vad->noise_means[gaussian],
                                                 vad->noise_stds[gaussian],
                                                 &deltaN[gaussian]);
        noise_probability[k] = kNoiseDataWeights[gaussian] * tmp1_s32;
        h0_test += noise_probability[k];  // Q27

        // Probability under H1, that is, probability of frame being speech.
        // Value given in Q27 = Q7 * Q20.
        tmp1_s32 = wtk_GaussianProbability(features[channel],
                                                 vad->speech_means[gaussian],
                                                 vad->speech_stds[gaussian],
                                                 &deltaS[gaussian]);
        speech_probability[k] = kSpeechDataWeights[gaussian] * tmp1_s32;
        h1_test += speech_probability[k];  // Q27
      }

      // Calculate the log likelihood ratio: log2(Pr{X|H1} / Pr{X|H1}).
      // Approximation:
      // log2(Pr{X|H1} / Pr{X|H1}) = log2(Pr{X|H1}*2^Q) - log2(Pr{X|H1}*2^Q)
      //                           = log2(h1_test) - log2(h0_test)
      //                           = log2(2^(31-shifts_h1)*(1+b1))
      //                             - log2(2^(31-shifts_h0)*(1+b0))
      //                           = shifts_h0 - shifts_h1
      //                             + log2(1+b1) - log2(1+b0)
      //                          ~= shifts_h0 - shifts_h1
      //
      // Note that b0 and b1 are values less than 1, hence, 0 <= log2(1+b0) < 1.
      // Further, b0 and b1 are independent and on the average the two terms
      // cancel.
      shifts_h0 = wtk_NormW32(h0_test);
      shifts_h1 = wtk_NormW32(h1_test);
      if (h0_test == 0) {
        shifts_h0 = 31;
      }
      if (h1_test == 0) {
        shifts_h1 = 31;
      }
      log_likelihood_ratio = shifts_h0 - shifts_h1;

      // Update |sum_log_likelihood_ratios| with spectrum weighting. This is
      // used for the global VAD decision.
      sum_log_likelihood_ratios +=
          (int32_t) (log_likelihood_ratio * kSpectrumWeight[channel]);

      // Local VAD decision.
      if ((log_likelihood_ratio << 2) > individualTest) {
        vadflag = 1;
      }

      // TODO(bjornv): The conditional probabilities below are applied on the
      // hard coded number of Gaussians set to two. Find a way to generalize.
      // Calculate local noise probabilities used later when updating the GMM.
      h0 = (int16_t) (h0_test >> 12);  // Q15
      if (h0 > 0) {
        // High probability of noise. Assign conditional probabilities for each
        // Gaussian in the GMM.
        tmp1_s32 = (noise_probability[0] & 0xFFFFF000) << 2;  // Q29
        ngprvec[channel] = (int16_t) wtk_DivW32W16(tmp1_s32, h0);  // Q14
        ngprvec[channel + kNumChannels] = 16384 - ngprvec[channel];
      } else {
        // Low noise probability. Assign conditional probability 1 to the first
        // Gaussian and 0 to the rest (which is already set at initialization).
        ngprvec[channel] = 16384;
      }

      // Calculate local speech probabilities used later when updating the GMM.
      h1 = (int16_t) (h1_test >> 12);  // Q15
      if (h1 > 0) {
        // High probability of speech. Assign conditional probabilities for each
        // Gaussian in the GMM. Otherwise use the initialized values, i.e., 0.
        tmp1_s32 = (speech_probability[0] & 0xFFFFF000) << 2;  // Q29
        sgprvec[channel] = (int16_t) wtk_DivW32W16(tmp1_s32, h1);  // Q14
        sgprvec[channel + kNumChannels] = 16384 - sgprvec[channel];
      }
    }

    // Make a global VAD decision.
    vadflag |= (sum_log_likelihood_ratios >= totalTest);

    // Update the model parameters.
    maxspe = 12800;
    for (channel = 0; channel < kNumChannels; channel++) {

      // Get minimum value in past which is used for long term correction in Q4.
      feature_minimum = wtk_gmmvad_FindMinimum(vad, features[channel], channel);

      // Compute the "global" mean, that is the sum of the two means weighted.
      noise_global_mean = WeightedAverage(&vad->noise_means[channel], 0,
                                          &kNoiseDataWeights[channel]);
      tmp1_s16 = (int16_t) (noise_global_mean >> 6);  // Q8

      for (k = 0; k < kNumGaussians; k++) {
        gaussian = channel + k * kNumChannels;

        nmk = vad->noise_means[gaussian];
        smk = vad->speech_means[gaussian];
        nsk = vad->noise_stds[gaussian];
        ssk = vad->speech_stds[gaussian];

        // Update noise mean vector if the frame consists of noise only.
        nmk2 = nmk;
        if (!vadflag) {
          // deltaN = (x-mu)/sigma^2
          // ngprvec[k] = |noise_probability[k]| /
          //   (|noise_probability[0]| + |noise_probability[1]|)

          // (Q14 * Q11 >> 11) = Q14.
          delt = (int16_t)((ngprvec[gaussian] * deltaN[gaussian]) >> 11);
          // Q7 + (Q14 * Q15 >> 22) = Q7.
          nmk2 = nmk + (int16_t)((delt * kNoiseUpdateConst) >> 22);
        }

        // Long term correction of the noise mean.
        // Q8 - Q8 = Q8.
        ndelt = (feature_minimum << 4) - tmp1_s16;
        // Q7 + (Q8 * Q8) >> 9 = Q7.
        nmk3 = nmk2 + (int16_t)((ndelt * kBackEta) >> 9);

        // Control that the noise mean does not drift to much.
        tmp_s16 = (int16_t) ((k + 5) << 7);
        if (nmk3 < tmp_s16) {
          nmk3 = tmp_s16;
        }
        tmp_s16 = (int16_t) ((72 + k - channel) << 7);
        if (nmk3 > tmp_s16) {
          nmk3 = tmp_s16;
        }
        vad->noise_means[gaussian] = nmk3;

        if (vadflag) {
          // Update speech mean vector:
          // |deltaS| = (x-mu)/sigma^2
          // sgprvec[k] = |speech_probability[k]| /
          //   (|speech_probability[0]| + |speech_probability[1]|)

          // (Q14 * Q11) >> 11 = Q14.
          delt = (int16_t)((sgprvec[gaussian] * deltaS[gaussian]) >> 11);
          // Q14 * Q15 >> 21 = Q8.
          tmp_s16 = (int16_t)((delt * kSpeechUpdateConst) >> 21);
          // Q7 + (Q8 >> 1) = Q7. With rounding.
          smk2 = smk + ((tmp_s16 + 1) >> 1);

          // Control that the speech mean does not drift to much.
          maxmu = maxspe + 640;
          if (smk2 < kMinimumMean[k]) {
            smk2 = kMinimumMean[k];
          }
          if (smk2 > maxmu) {
            smk2 = maxmu;
          }
          vad->speech_means[gaussian] = smk2;  // Q7.

          // (Q7 >> 3) = Q4. With rounding.
          tmp_s16 = ((smk + 4) >> 3);

          tmp_s16 = features[channel] - tmp_s16;  // Q4
          // (Q11 * Q4 >> 3) = Q12.
          tmp1_s32 = (deltaS[gaussian] * tmp_s16) >> 3;
          tmp2_s32 = tmp1_s32 - 4096;
          tmp_s16 = sgprvec[gaussian] >> 2;
          // (Q14 >> 2) * Q12 = Q24.
          tmp1_s32 = tmp_s16 * tmp2_s32;

          tmp2_s32 = tmp1_s32 >> 4;  // Q20

          // 0.1 * Q20 / Q7 = Q13.
          if (tmp2_s32 > 0) {
            tmp_s16 = (int16_t) wtk_DivW32W16(tmp2_s32, ssk * 10);
          } else {
            tmp_s16 = (int16_t) wtk_DivW32W16(-tmp2_s32, ssk * 10);
            tmp_s16 = -tmp_s16;
          }
          // Divide by 4 giving an update factor of 0.025 (= 0.1 / 4).
          // Note that division by 4 equals shift by 2, hence,
          // (Q13 >> 8) = (Q13 >> 6) / 4 = Q7.
          tmp_s16 += 128;  // Rounding.
          ssk += (tmp_s16 >> 8);
          if (ssk < kMinStd) {
            ssk = kMinStd;
          }
          vad->speech_stds[gaussian] = ssk;
        } else {
          // Update GMM variance vectors.
          // deltaN * (features[channel] - nmk) - 1
          // Q4 - (Q7 >> 3) = Q4.
          tmp_s16 = features[channel] - (nmk >> 3);
          // (Q11 * Q4 >> 3) = Q12.
          tmp1_s32 = (deltaN[gaussian] * tmp_s16) >> 3;
          tmp1_s32 -= 4096;

          // (Q14 >> 2) * Q12 = Q24.
          tmp_s16 = (ngprvec[gaussian] + 2) >> 2;
          tmp2_s32 = tmp_s16 * tmp1_s32;
          // Q20  * approx 0.001 (2^-10=0.0009766), hence,
          // (Q24 >> 14) = (Q24 >> 4) / 2^10 = Q20.
          tmp1_s32 = tmp2_s32 >> 14;

          // Q20 / Q7 = Q13.
          if (tmp1_s32 > 0) {
            tmp_s16 = (int16_t) wtk_DivW32W16(tmp1_s32, nsk);
          } else {
            tmp_s16 = (int16_t) wtk_DivW32W16(-tmp1_s32, nsk);
            tmp_s16 = -tmp_s16;
          }
          tmp_s16 += 32;  // Rounding
          nsk += tmp_s16 >> 6;  // Q13 >> 6 = Q7.
          if (nsk < kMinStd) {
            nsk = kMinStd;
          }
          vad->noise_stds[gaussian] = nsk;
        }
      }

      // Separate models if they are too close.
      // |noise_global_mean| in Q14 (= Q7 * Q7).
      noise_global_mean = WeightedAverage(&vad->noise_means[channel], 0,
                                          &kNoiseDataWeights[channel]);

      // |speech_global_mean| in Q14 (= Q7 * Q7).
      speech_global_mean = WeightedAverage(&vad->speech_means[channel], 0,
                                           &kSpeechDataWeights[channel]);

      // |diff| = "global" speech mean - "global" noise mean.
      // (Q14 >> 9) - (Q14 >> 9) = Q5.
      diff = (int16_t) (speech_global_mean >> 9) -
          (int16_t) (noise_global_mean >> 9);
      if (diff < kMinimumDifference[channel]) {
        tmp_s16 = kMinimumDifference[channel] - diff;

        // |tmp1_s16| = ~0.8 * (kMinimumDifference - diff) in Q7.
        // |tmp2_s16| = ~0.2 * (kMinimumDifference - diff) in Q7.
        tmp1_s16 = (int16_t)((13 * tmp_s16) >> 2);
        tmp2_s16 = (int16_t)((3 * tmp_s16) >> 2);

        // Move Gaussian means for speech model by |tmp1_s16| and update
        // |speech_global_mean|. Note that |self->speech_means[channel]| is
        // changed after the call.
        speech_global_mean = WeightedAverage(&vad->speech_means[channel],
                                             tmp1_s16,
                                             &kSpeechDataWeights[channel]);

        // Move Gaussian means for noise model by -|tmp2_s16| and update
        // |noise_global_mean|. Note that |self->noise_means[channel]| is
        // changed after the call.
        noise_global_mean = WeightedAverage(&vad->noise_means[channel],
                                            -tmp2_s16,
                                            &kNoiseDataWeights[channel]);
      }

      // Control that the speech & noise means do not drift to much.
      maxspe = kMaximumSpeech[channel];
      tmp2_s16 = (int16_t) (speech_global_mean >> 7);
      if (tmp2_s16 > maxspe) {
        // Upper limit of speech model.
        tmp2_s16 -= maxspe;

        for (k = 0; k < kNumGaussians; k++) {
          vad->speech_means[channel + k * kNumChannels] -= tmp2_s16;
        }
      }

      tmp2_s16 = (int16_t) (noise_global_mean >> 7);
      if (tmp2_s16 > kMaximumNoise[channel]) {
        tmp2_s16 -= kMaximumNoise[channel];

        for (k = 0; k < kNumGaussians; k++) {
          vad->noise_means[channel + k * kNumChannels] -= tmp2_s16;
        }
      }
    }
    vad->frame_counter++;
  }

  // Smooth with respect to transition hysteresis.
  if (!vadflag) {
    if (vad->over_hang > 0) {
      vadflag = 2 + vad->over_hang;
      vad->over_hang--;
    }
    vad->num_of_speech = 0;
  } else {
    vad->num_of_speech++;
    if (vad->num_of_speech > kMaxSpeechFrames) {
      vad->num_of_speech = kMaxSpeechFrames;
      vad->over_hang = overhead2;
    } else {
      vad->over_hang = overhead1;
    }
  }
  return vadflag;
}

int wtk_gmmvad_process(wtk_gmmvad_t *v,short *sample,int len)
{
    int16_t feature_vector[kNumChannels], total_power;
    int vad;

   // print_short(sample,len);
	wtk_downsampling(sample,v->speechNB,v->downsampling_filter_state,len);
	len>>=1;
	total_power=wtk_gmmvad_calc_feature(v,v->speechNB,len,feature_vector);
	vad=wtk_gmmvad_calc_prob(v,feature_vector,total_power,len);
	return vad>0?1:0;
}
