#include "wtk_fixpoint.h"

fixed32 fe_log(float32 x)
{
    if (x <= 0) {
        return MIN_FIXLOG;
    }
    else {
        return FLOAT2FIX(log(x));
    }
}

/* Table of log2(x/128)*(1<<DEFAULT_RADIX) */
/* perl -e 'for (0..128) {my $x = 1 + $_/128; $y = 1 + ($_ + 1.) / 128;
                  print "    (uint32)(", (log($x) + log($y))/2/log(2)," *(1<<DEFAULT_RADIX)),\n"}' */
static uint32 logtable[] = {
    (uint32)(0.00561362771162706*(1<<DEFAULT_RADIX)),
    (uint32)(0.0167975342258543*(1<<DEFAULT_RADIX)),
    (uint32)(0.0278954072829524*(1<<DEFAULT_RADIX)),
    (uint32)(0.0389085604479519*(1<<DEFAULT_RADIX)),
    (uint32)(0.0498382774298215*(1<<DEFAULT_RADIX)),
    (uint32)(0.060685812979481*(1<<DEFAULT_RADIX)),
    (uint32)(0.0714523937543017*(1<<DEFAULT_RADIX)),
    (uint32)(0.0821392191505851*(1<<DEFAULT_RADIX)),
    (uint32)(0.0927474621054331*(1<<DEFAULT_RADIX)),
    (uint32)(0.103278269869348*(1<<DEFAULT_RADIX)),
    (uint32)(0.113732764750838*(1<<DEFAULT_RADIX)),
    (uint32)(0.124112044834237*(1<<DEFAULT_RADIX)),
    (uint32)(0.13441718467188*(1<<DEFAULT_RADIX)),
    (uint32)(0.144649235951738*(1<<DEFAULT_RADIX)),
    (uint32)(0.154809228141536*(1<<DEFAULT_RADIX)),
    (uint32)(0.164898169110351*(1<<DEFAULT_RADIX)),
    (uint32)(0.174917045728623*(1<<DEFAULT_RADIX)),
    (uint32)(0.184866824447476*(1<<DEFAULT_RADIX)),
    (uint32)(0.194748451858191*(1<<DEFAULT_RADIX)),
    (uint32)(0.204562855232657*(1<<DEFAULT_RADIX)),
    (uint32)(0.214310943045556*(1<<DEFAULT_RADIX)),
    (uint32)(0.223993605479021*(1<<DEFAULT_RADIX)),
    (uint32)(0.23361171491048*(1<<DEFAULT_RADIX)),
    (uint32)(0.243166126384332*(1<<DEFAULT_RADIX)),
    (uint32)(0.252657678068119*(1<<DEFAULT_RADIX)),
    (uint32)(0.262087191693777*(1<<DEFAULT_RADIX)),
    (uint32)(0.271455472984569*(1<<DEFAULT_RADIX)),
    (uint32)(0.280763312068243*(1<<DEFAULT_RADIX)),
    (uint32)(0.290011483876938*(1<<DEFAULT_RADIX)),
    (uint32)(0.299200748534365*(1<<DEFAULT_RADIX)),
    (uint32)(0.308331851730729*(1<<DEFAULT_RADIX)),
    (uint32)(0.317405525085859*(1<<DEFAULT_RADIX)),
    (uint32)(0.32642248650099*(1<<DEFAULT_RADIX)),
    (uint32)(0.335383440499621*(1<<DEFAULT_RADIX)),
    (uint32)(0.344289078557851*(1<<DEFAULT_RADIX)),
    (uint32)(0.353140079424581*(1<<DEFAULT_RADIX)),
    (uint32)(0.36193710943195*(1<<DEFAULT_RADIX)),
    (uint32)(0.37068082279637*(1<<DEFAULT_RADIX)),
    (uint32)(0.379371861910488*(1<<DEFAULT_RADIX)),
    (uint32)(0.388010857626406*(1<<DEFAULT_RADIX)),
    (uint32)(0.396598429530472*(1<<DEFAULT_RADIX)),
    (uint32)(0.405135186209943*(1<<DEFAULT_RADIX)),
    (uint32)(0.4136217255118*(1<<DEFAULT_RADIX)),
    (uint32)(0.422058634793998*(1<<DEFAULT_RADIX)),
    (uint32)(0.430446491169411*(1<<DEFAULT_RADIX)),
    (uint32)(0.438785861742727*(1<<DEFAULT_RADIX)),
    (uint32)(0.447077303840529*(1<<DEFAULT_RADIX)),
    (uint32)(0.455321365234813*(1<<DEFAULT_RADIX)),
    (uint32)(0.463518584360147*(1<<DEFAULT_RADIX)),
    (uint32)(0.471669490524698*(1<<DEFAULT_RADIX)),
    (uint32)(0.479774604115327*(1<<DEFAULT_RADIX)),
    (uint32)(0.487834436796966*(1<<DEFAULT_RADIX)),
    (uint32)(0.49584949170644*(1<<DEFAULT_RADIX)),
    (uint32)(0.503820263640951*(1<<DEFAULT_RADIX)),
    (uint32)(0.511747239241369*(1<<DEFAULT_RADIX)),
    (uint32)(0.519630897170528*(1<<DEFAULT_RADIX)),
    (uint32)(0.527471708286662*(1<<DEFAULT_RADIX)),
    (uint32)(0.535270135812172*(1<<DEFAULT_RADIX)),
    (uint32)(0.543026635497834*(1<<DEFAULT_RADIX)),
    (uint32)(0.550741655782637*(1<<DEFAULT_RADIX)),
    (uint32)(0.558415637949355*(1<<DEFAULT_RADIX)),
    (uint32)(0.56604901627601*(1<<DEFAULT_RADIX)),
    (uint32)(0.573642218183348*(1<<DEFAULT_RADIX)),
    (uint32)(0.581195664378452*(1<<DEFAULT_RADIX)),
    (uint32)(0.588709768994618*(1<<DEFAULT_RADIX)),
    (uint32)(0.596184939727604*(1<<DEFAULT_RADIX)),
    (uint32)(0.603621577968369*(1<<DEFAULT_RADIX)),
    (uint32)(0.61102007893241*(1<<DEFAULT_RADIX)),
    (uint32)(0.618380831785792*(1<<DEFAULT_RADIX)),
    (uint32)(0.625704219767993*(1<<DEFAULT_RADIX)),
    (uint32)(0.632990620311629*(1<<DEFAULT_RADIX)),
    (uint32)(0.640240405159187*(1<<DEFAULT_RADIX)),
    (uint32)(0.647453940476827*(1<<DEFAULT_RADIX)),
    (uint32)(0.654631586965362*(1<<DEFAULT_RADIX)),
    (uint32)(0.661773699968486*(1<<DEFAULT_RADIX)),
    (uint32)(0.668880629578336*(1<<DEFAULT_RADIX)),
    (uint32)(0.675952720738471*(1<<DEFAULT_RADIX)),
    (uint32)(0.682990313344332*(1<<DEFAULT_RADIX)),
    (uint32)(0.689993742341272*(1<<DEFAULT_RADIX)),
    (uint32)(0.696963337820209*(1<<DEFAULT_RADIX)),
    (uint32)(0.703899425110987*(1<<DEFAULT_RADIX)),
    (uint32)(0.710802324873503*(1<<DEFAULT_RADIX)),
    (uint32)(0.717672353186654*(1<<DEFAULT_RADIX)),
    (uint32)(0.724509821635192*(1<<DEFAULT_RADIX)),
    (uint32)(0.731315037394519*(1<<DEFAULT_RADIX)),
    (uint32)(0.738088303313493*(1<<DEFAULT_RADIX)),
    (uint32)(0.744829917995304*(1<<DEFAULT_RADIX)),
    (uint32)(0.751540175876464*(1<<DEFAULT_RADIX)),
    (uint32)(0.758219367303974*(1<<DEFAULT_RADIX)),
    (uint32)(0.764867778610703*(1<<DEFAULT_RADIX)),
    (uint32)(0.77148569218905*(1<<DEFAULT_RADIX)),
    (uint32)(0.778073386562917*(1<<DEFAULT_RADIX)),
    (uint32)(0.784631136458046*(1<<DEFAULT_RADIX)),
    (uint32)(0.791159212870769*(1<<DEFAULT_RADIX)),
    (uint32)(0.797657883135205*(1<<DEFAULT_RADIX)),
    (uint32)(0.804127410988954*(1<<DEFAULT_RADIX)),
    (uint32)(0.810568056637321*(1<<DEFAULT_RADIX)),
    (uint32)(0.816980076816112*(1<<DEFAULT_RADIX)),
    (uint32)(0.823363724853051*(1<<DEFAULT_RADIX)),
    (uint32)(0.829719250727828*(1<<DEFAULT_RADIX)),
    (uint32)(0.836046901130843*(1<<DEFAULT_RADIX)),
    (uint32)(0.84234691952066*(1<<DEFAULT_RADIX)),
    (uint32)(0.848619546180216*(1<<DEFAULT_RADIX)),
    (uint32)(0.854865018271815*(1<<DEFAULT_RADIX)),
    (uint32)(0.861083569890926*(1<<DEFAULT_RADIX)),
    (uint32)(0.867275432118842*(1<<DEFAULT_RADIX)),
    (uint32)(0.873440833074202*(1<<DEFAULT_RADIX)),
    (uint32)(0.879579997963421*(1<<DEFAULT_RADIX)),
    (uint32)(0.88569314913005*(1<<DEFAULT_RADIX)),
    (uint32)(0.891780506103101*(1<<DEFAULT_RADIX)),
    (uint32)(0.897842285644346*(1<<DEFAULT_RADIX)),
    (uint32)(0.903878701794633*(1<<DEFAULT_RADIX)),
    (uint32)(0.90988996591924*(1<<DEFAULT_RADIX)),
    (uint32)(0.915876286752278*(1<<DEFAULT_RADIX)),
    (uint32)(0.921837870440188*(1<<DEFAULT_RADIX)),
    (uint32)(0.927774920584334*(1<<DEFAULT_RADIX)),
    (uint32)(0.933687638282728*(1<<DEFAULT_RADIX)),
    (uint32)(0.939576222170905*(1<<DEFAULT_RADIX)),
    (uint32)(0.945440868461959*(1<<DEFAULT_RADIX)),
    (uint32)(0.951281770985776*(1<<DEFAULT_RADIX)),
    (uint32)(0.957099121227478*(1<<DEFAULT_RADIX)),
    (uint32)(0.962893108365084*(1<<DEFAULT_RADIX)),
    (uint32)(0.968663919306429*(1<<DEFAULT_RADIX)),
    (uint32)(0.974411738725344*(1<<DEFAULT_RADIX)),
    (uint32)(0.980136749097113*(1<<DEFAULT_RADIX)),
    (uint32)(0.985839130733238*(1<<DEFAULT_RADIX)),
    (uint32)(0.991519061815512*(1<<DEFAULT_RADIX)),
    (uint32)(0.997176718429429*(1<<DEFAULT_RADIX)),
    (uint32)(1.00281227459694*(1<<DEFAULT_RADIX)),
};

/**
 * Take base-2 logarithm of an integer, yielding a fixedpoint number
 * with DEFAULT_RADIX as radix point.
 */
int32
fixlog2(uint32 x)
{
    uint32 y;

    if (x == 0)
        return MIN_FIXLOG2;

    /* Get the exponent. */
#if ((defined(__ARM_ARCH_5__) || defined(__ARM_ARCH_5T__) || \
      defined(__ARM_ARCH_5TE__) || defined(__ARM_ARCH_7A__)) && !defined(__thumb__))
  __asm__("clz %0, %1\n": "=r"(y):"r"(x));
    x <<= y;
    y = 31 - y;
#elif defined(__ppc__)
  __asm__("cntlzw %0, %1\n": "=r"(y):"r"(x));
    x <<= y;
    y = 31 - y;
#elif __GNUC__ >= 4
    y = __builtin_clz(x);
    x <<= y;
    y = (31 - y);
#else
    for (y = 31; y > 0; --y) {
        if (x & 0x80000000)
            break;
        x <<= 1;
    }
#endif
    y <<= DEFAULT_RADIX;
    /* Do a table lookup for the MSB of the mantissa. */
    x = (x >> 24) & 0x7f;
    return y + logtable[x];
}


/**
 * Take natural logarithm of an integer, yielding a fixedpoint number
 * with DEFAULT_RADIX as radix point.
 */
int fixlog(uint32 x)
{
    int32 y;
    y = fixlog2(x);
    return FIXMUL(y, FIXLN_2);
}

/* Internal log-addition table for natural log with radix point at 8
 * bits.  Each entry is 256 * log(1 + e^{-n/256}).  This is used in the
 * log-add computation:
 *
 * e^z = e^x + e^y
 * e^z = e^x(1 + e^{y-x})     = e^y(1 + e^{x-y})
 * z   = x + log(1 + e^{y-x}) = y + log(1 + e^{x-y})
 *
 * So when y > x, z = y + logadd_table[-(x-y)]
 *    when x > y, z = x + logadd_table[-(y-x)]
 */
static const unsigned char fe_logadd_table[] = {
    177, 177, 176, 176, 175, 175, 174, 174, 173, 173,
    172, 172, 172, 171, 171, 170, 170, 169, 169, 168,
    168, 167, 167, 166, 166, 165, 165, 164, 164, 163,
    163, 162, 162, 161, 161, 161, 160, 160, 159, 159,
    158, 158, 157, 157, 156, 156, 155, 155, 155, 154,
    154, 153, 153, 152, 152, 151, 151, 151, 150, 150,
    149, 149, 148, 148, 147, 147, 147, 146, 146, 145,
    145, 144, 144, 144, 143, 143, 142, 142, 141, 141,
    141, 140, 140, 139, 139, 138, 138, 138, 137, 137,
    136, 136, 136, 135, 135, 134, 134, 134, 133, 133,
    132, 132, 131, 131, 131, 130, 130, 129, 129, 129,
    128, 128, 128, 127, 127, 126, 126, 126, 125, 125,
    124, 124, 124, 123, 123, 123, 122, 122, 121, 121,
    121, 120, 120, 119, 119, 119, 118, 118, 118, 117,
    117, 117, 116, 116, 115, 115, 115, 114, 114, 114,
    113, 113, 113, 112, 112, 112, 111, 111, 110, 110,
    110, 109, 109, 109, 108, 108, 108, 107, 107, 107,
    106, 106, 106, 105, 105, 105, 104, 104, 104, 103,
    103, 103, 102, 102, 102, 101, 101, 101, 100, 100,
    100, 99, 99, 99, 98, 98, 98, 97, 97, 97,
    96, 96, 96, 96, 95, 95, 95, 94, 94, 94,
    93, 93, 93, 92, 92, 92, 92, 91, 91, 91,
    90, 90, 90, 89, 89, 89, 89, 88, 88, 88,
    87, 87, 87, 87, 86, 86, 86, 85, 85, 85,
    85, 84, 84, 84, 83, 83, 83, 83, 82, 82,
    82, 82, 81, 81, 81, 80, 80, 80, 80, 79,
    79, 79, 79, 78, 78, 78, 78, 77, 77, 77,
    77, 76, 76, 76, 75, 75, 75, 75, 74, 74,
    74, 74, 73, 73, 73, 73, 72, 72, 72, 72,
    71, 71, 71, 71, 71, 70, 70, 70, 70, 69,
    69, 69, 69, 68, 68, 68, 68, 67, 67, 67,
    67, 67, 66, 66, 66, 66, 65, 65, 65, 65,
    64, 64, 64, 64, 64, 63, 63, 63, 63, 63,
    62, 62, 62, 62, 61, 61, 61, 61, 61, 60,
    60, 60, 60, 60, 59, 59, 59, 59, 59, 58,
    58, 58, 58, 58, 57, 57, 57, 57, 57, 56,
    56, 56, 56, 56, 55, 55, 55, 55, 55, 54,
    54, 54, 54, 54, 53, 53, 53, 53, 53, 52,
    52, 52, 52, 52, 52, 51, 51, 51, 51, 51,
    50, 50, 50, 50, 50, 50, 49, 49, 49, 49,
    49, 49, 48, 48, 48, 48, 48, 48, 47, 47,
    47, 47, 47, 47, 46, 46, 46, 46, 46, 46,
    45, 45, 45, 45, 45, 45, 44, 44, 44, 44,
    44, 44, 43, 43, 43, 43, 43, 43, 43, 42,
    42, 42, 42, 42, 42, 41, 41, 41, 41, 41,
    41, 41, 40, 40, 40, 40, 40, 40, 40, 39,
    39, 39, 39, 39, 39, 39, 38, 38, 38, 38,
    38, 38, 38, 37, 37, 37, 37, 37, 37, 37,
    37, 36, 36, 36, 36, 36, 36, 36, 35, 35,
    35, 35, 35, 35, 35, 35, 34, 34, 34, 34,
    34, 34, 34, 34, 33, 33, 33, 33, 33, 33,
    33, 33, 32, 32, 32, 32, 32, 32, 32, 32,
    32, 31, 31, 31, 31, 31, 31, 31, 31, 31,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 29,
    29, 29, 29, 29, 29, 29, 29, 29, 28, 28,
    28, 28, 28, 28, 28, 28, 28, 28, 27, 27,
    27, 27, 27, 27, 27, 27, 27, 27, 26, 26,
    26, 26, 26, 26, 26, 26, 26, 26, 25, 25,
    25, 25, 25, 25, 25, 25, 25, 25, 25, 24,
    24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
    23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
    23, 23, 22, 22, 22, 22, 22, 22, 22, 22,
    22, 22, 22, 22, 21, 21, 21, 21, 21, 21,
    21, 21, 21, 21, 21, 21, 21, 20, 20, 20,
    20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
    19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
    19, 19, 19, 19, 18, 18, 18, 18, 18, 18,
    18, 18, 18, 18, 18, 18, 18, 18, 18, 17,
    17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
    17, 17, 17, 17, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 14, 14,
    14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
    14, 14, 14, 14, 14, 14, 14, 13, 13, 13,
    13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
    13, 13, 13, 13, 13, 13, 13, 12, 12, 12,
    12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
    12, 12, 12, 12, 12, 12, 12, 12, 12, 11,
    11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
    11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
    11, 11, 11, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 9,
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
    9, 9, 9, 9, 9, 9, 9, 9, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 5, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 0
};

static const int fe_logadd_table_size =
    sizeof(fe_logadd_table) / sizeof(fe_logadd_table[0]);

fixed32 fe_log_add(fixed32 x, fixed32 y)
{
    fixed32 d, r;

    if (x > y) {
        d = (x - y) >> (DEFAULT_RADIX - 8);
        r = x;
    }
    else {
        d = (y - x) >> (DEFAULT_RADIX - 8);
        r = y;
    }

    if (r <= MIN_FIXLOG)
    {
    	return MIN_FIXLOG;
    }else if (d > fe_logadd_table_size - 1)
    {
        return r;
    }else if(d<0)
    {
    	return r;
    }else
    {
    	//wtk_debug("d=%d %f/%f\n",d,FIX2FLOAT(x),FIX2FLOAT(y));
        r += ((fixed32) fe_logadd_table[d] << (DEFAULT_RADIX - 8));
/*        printf("%d - %d = %d | %f - %f = %f | %f - %f = %f\n",
               x, y, r, FIX2FLOAT(x), FIX2FLOAT(y), FIX2FLOAT(r),
               exp(FIX2FLOAT(x)), exp(FIX2FLOAT(y)), exp(FIX2FLOAT(r)));
*/
        return r;
    }
}

int* wtk_fix_float2int(float *v,int n)
{
	int *fix;
	int i;

	fix=(int*)wtk_calloc(n,sizeof(int));
	for(i=0;i<n;++i)
	{
		fix[i]=FLOAT2FIX(v[i]);
	}
	return fix;
}


void wtk_fix_print_float(int *v,int len)
{
	int i;

	wtk_debug("================================\n");
	for(i=0;i<len;++i)
	{
		wtk_debug("v[%d]=%f\n",i,FIX2FLOAT(v[i]));
		//wtk_debug("v[%d]=%d/%f\n",i,v[i],FIX2FLOAT(v[i]));
	}
}

void wtk_fix_print(int *v,int len,int shift)
{
	int i;
	float f;

	wtk_debug("================================\n");
	f=0;
	if(shift>0)
	{
		for(i=0;i<len;++i)
		{
			f+=FIX2FLOAT_ANY(v[i],shift);
			wtk_debug("v[%d]=%d/%.7f\n",i,v[i],FIX2FLOAT_ANY(v[i],shift));
			//wtk_debug("v[%d]=%d/%f\n",i,v[i],FIX2FLOAT(v[i]));
		}
	}else
	{
		shift=-shift;
		for(i=0;i<len;++i)
		{
			f+=FIX2FLOAT_ANY2(v[i],shift);
			wtk_debug("v[%d]=%d/%f\n",i,v[i],FIX2FLOAT_ANY2(v[i],shift));
			//wtk_debug("v[%d]=%d/%f\n",i,v[i],FIX2FLOAT(v[i]));
		}
	}
	wtk_debug("============== tot=%f ===============\n",f);
}

void wtk_fix_print_short(short *v,int len,int shift)
{
	int i;
	float f;

	wtk_debug("================================\n");
	f=0;
	if(shift>0)
	{
		for(i=0;i<len;++i)
		{
			f+=FIX2FLOAT_ANY(v[i],shift);
			wtk_debug("v[%d]=%d/%.7f\n",i,v[i],FIX2FLOAT_ANY(v[i],shift));
			//wtk_debug("v[%d]=%d/%f\n",i,v[i],FIX2FLOAT(v[i]));
		}
	}else
	{
		shift=-shift;
		for(i=0;i<len;++i)
		{
			f+=FIX2FLOAT_ANY2(v[i],shift);
			wtk_debug("v[%d]=%d/%f\n",i,v[i],FIX2FLOAT_ANY2(v[i],shift));
			//wtk_debug("v[%d]=%d/%f\n",i,v[i],FIX2FLOAT(v[i]));
		}
	}
	wtk_debug("============== tot=%f ===============\n",f);
}


float wtk_fix_sum(int *v,int len,int shift)
{
	int i;
	float f;

	f=0;
	for(i=0;i<len;++i)
	{
		f+=FIX2FLOAT_ANY(v[i],shift);
	}
	return f;
}


short wtk_flt_ilog(unsigned int x)
{
   int r=0;

   if (x>=(int)65536)
   {
      x >>= 16;
      r += 16;
   }
   if (x>=256)
   {
      x >>= 8;
      r += 8;
   }
   if (x>=16)
   {
      x >>= 4;
      r += 4;
   }
   if (x>=4)
   {
      x >>= 2;
      r += 2;
   }
   if (x>=2)
   {
      r += 1;
   }
   return r;
}

short wtk_flt_ilog2(int64 x)
{
   int r=0;

   if(x>=131072)
   {
	   x>>=32;
	   r+=32;
   }
   if (x>=(int)65536)
   {
      x >>= 16;
      r += 16;
   }
   if (x>=256)
   {
      x >>= 8;
      r += 8;
   }
   if (x>=16)
   {
      x >>= 4;
      r += 4;
   }
   if (x>=4)
   {
      x >>= 2;
      r += 2;
   }
   if (x>=2)
   {
      r += 1;
   }
   return r;
}



#define ABS(x) ((x) < 0 ? (-(x)) : (x))      /**< Absolute integer value. */
#define ABS16(x) ((x) < 0 ? (-(x)) : (x))    /**< Absolute 16-bit value.  */
#define MIN16(a,b) ((a) < (b) ? (a) : (b))   /**< Maximum 16-bit value.   */
#define MAX16(a,b) ((a) > (b) ? (a) : (b))   /**< Maximum 16-bit value.   */
#define ABS32(x) ((x) < 0 ? (-(x)) : (x))    /**< Absolute 32-bit value.  */
#define MIN32(a,b) ((a) < (b) ? (a) : (b))   /**< Maximum 32-bit value.   */
#define MAX32(a,b) ((a) > (b) ? (a) : (b))   /**< Maximum 32-bit value.   */
#define NEG16(x) (-(x))
#define NEG32(x) (-(x))
#define EXTRACT16(x) ((short)(x))
#define EXTEND32(x) ((int)(x))
#define EXTEND64(x) ((int64)(x))
#define SHR16(a,shift) ((a) >> (shift))
#define SHL16(a,shift) ((a) << (shift))
#define SHR32(a,shift) ((a) >> (shift))
#define SHL32(a,shift) ((a) << (shift))
#define PSHR16(a,shift) (SHR16((a)+((1<<((shift))>>1)),shift))
#define PSHR32(a,shift) (SHR32((a)+((EXTEND32(1)<<((shift))>>1)),shift))
#define VSHR32(a, shift) (((shift)>0) ? SHR32(a, shift) : SHL32(a, -(shift)))
#define SATURATE16(x,a) (((x)>(a) ? (a) : (x)<-(a) ? -(a) : (x)))
#define SATURATE32(x,a) (((x)>(a) ? (a) : (x)<-(a) ? -(a) : (x)))
#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif
#define DIV32_16(a,b) ((short)(((int)(a))/((short)(b))))

wtk_flt_t wtk_flt2(int64 x)
{
   int e=0;
   int sign=0;
   wtk_flt_t r;

   if (x<0)
   {
	  sign = 1;
	  x = -x;
   }
   if (x==0)
   {
	  r.m=r.e=0;
	  return r;
   }
   e = wtk_flt_ilog2(x)-14;
   x = VSHR32(x, e);
   if (sign)
   {
	  r.m = -x;
	  r.e = e;
	  return r;
   }
   else
   {
	  r.m = x;
	  r.e = e;
	  return r;
   }
}

int64 wtk_flt_extract64(wtk_flt_t a)
{
   if (a.e<0)
   {
      return (EXTEND64(a.m)+(EXTEND64(1)<<(-a.e-1)))>>-a.e;
   }else
   {
      return EXTEND64(a.m)<<a.e;
   }
}

int wtk_flt_toi(wtk_flt_t a)
{
	if(a.e<0)
	{
		return a.m/(((long long)1)<<(-a.e));
	}else
	{
		return a.m*(((long long)1)<<a.e);
	}
}

int wtk_flt_toi2(wtk_flt_t a,int shift)
{
	int df;

	df=a.e-shift;
	if(df>31 || df<-31)
	{
		return  df>=0?(a.m<<df):(a.m/(((long long)1)<<(-df)));
	}
	return  df>=0?(a.m<<df):(a.m/(1<<(-df)));
}



float wtk_flt_tof(wtk_flt_t a,int shift)
{
	int df;

	df=a.e-shift;
	if(df<0)
	{
		return a.m*1.0/(((long long)1)<<(-df));
	}else
	{
		return a.m*1.0*(((long long)1)<<df);
	}
}

void wtk_flt_print(wtk_flt_t flt)
{
	int64 v;

	v=wtk_flt_extract64(flt);
	wtk_debug("v=%lld m=%d e=%d\n",v,flt.m,flt.e);
}

void wtk_flt_print2(wtk_flt_t flt,int shift)
{
	float f;
	int df;

	df=flt.e-shift;
	if(df<0)
	{
		f=(flt.m*1.0)/(((long long)1)<<(-df));
		//wtk_debug("f=%f df=%d %d\n",f,df,1<<(df));
	}else
	{
		f=(flt.m*1.0)*(((long long)1)<<df);
	}
	//f=(flt.m*1.0)/(1<<(flt.e+shift));
	wtk_debug("v=%f(m=%d e=%d shift=%d df=%d)\n",f,flt.m,flt.e,shift,df);
}

wtk_flt_t wtk_flt3(int shift)
{
	wtk_flt_t r;

	r.e=shift-14;
	r.m=1<<(shift-r.e);
	return r;
}

wtk_flt_t wtk_flt(int x)
{
   int e=0;
   int sign=0;
   wtk_flt_t r;

   if (x<0)
   {
      sign = 1;
      x = -x;
   }
   if (x==0)
   {
	  r.m=r.e=0;
      return r;
   }
   e = wtk_flt_ilog(ABS32(x))-14;
   x = VSHR32(x, e);
   if (sign)
   {
      r.m = -x;
      r.e = e;
      return r;
   }
   else
   {
      r.m = x;
      r.e = e;
      return r;
   }
}


short wtk_flt_ilogl(wtk_int64_t x)
{
   int r=0;

   if (x>=(wtk_int64_t)4294967296)
   {
      x >>= 32;
      r += 32;
   }
   if (x>=(int)65536)
   {
      x >>= 16;
      r += 16;
   }
   if (x>=256)
   {
      x >>= 8;
      r += 8;
   }
   if (x>=16)
   {
      x >>= 4;
      r += 4;
   }
   if (x>=4)
   {
      x >>= 2;
      r += 2;
   }
   if (x>=2)
   {
      r += 1;
   }
   return r;
}

wtk_flt_t wtk_fltf(wtk_int64_t x)
{
   int e=0;
   int sign=0;
   wtk_flt_t r;

   if (x<0)
   {
      sign = 1;
      x = -x;
   }
   if (x==0)
   {
	  r.m=r.e=0;
      return r;
   }
   e = wtk_flt_ilogl(x>0?x:-x)-14;
   x=(e>0)?(x>>e):(x<<(-e));
   //x = VSHR32(x, e);
   if (sign)
   {
      r.m = -x;
      r.e = e;
      return r;
   }
   else
   {
      r.m = x;
      r.e = e;
      return r;
   }
}



int wtk_flt_extract32(wtk_flt_t a)
{
   if (a.e<0)
   {
      return (EXTEND32(a.m)+(EXTEND32(1)<<(-a.e-1)))>>-a.e;
   }else
   {
      return EXTEND32(a.m)<<a.e;
   }
}

short wtk_flt_extract16(wtk_flt_t a)
{
	   if (a.e<0)
	   {
	      return EXTRACT16((EXTEND32(a.m)+(EXTEND32(1)<<(-a.e-1)))>>-a.e);
	   }else
	   {
	      return a.m<<a.e;
	   }
}

wtk_flt_t wtk_flt_shl(wtk_flt_t a,int b)
{
	wtk_flt_t r;

	r.m = a.m;
	r.e = a.e+b;
	return r;
}

int wtk_flt_lt(wtk_flt_t a,wtk_flt_t b)
{
   if (a.m==0)
   {
      return b.m>0;
   }
   else if (b.m==0)
   {
      return a.m<0;
   }
   if ((a).e > (b).e)
   {
      return ((a).m>>1) < ((b).m>>MIN(15,(a).e-(b).e+1));
   }else
   {
      return ((b).m>>1) > ((a).m>>MIN(15,(b).e-(a).e+1));
   }
}

int wtk_flt_gt(wtk_flt_t a,wtk_flt_t b)
{
	return wtk_flt_lt(b,a);
}


wtk_flt_t wtk_flt_add(wtk_flt_t a,wtk_flt_t b)
{
	wtk_flt_t r;

   if (a.m==0)
      return b;
   else if (b.m==0)
      return a;
   if ((a).e > (b).e)
   {
      r.m = ((a).m>>1) + ((b).m>>MIN(15,(a).e-(b).e+1));
      r.e = (a).e+1;
   }
   else
   {
      r.m = ((b).m>>1) + ((a).m>>MIN(15,(b).e-(a).e+1));
      r.e = (b).e+1;
   }
   if (r.m>0)
   {
      if (r.m<16384)
      {
         r.m<<=1;
         r.e-=1;
      }
   } else {
      if (r.m>-16384)
      {
         r.m<<=1;
         r.e-=1;
      }
   }
   /*printf ("%f + %f = %f\n", REALFLOAT(a), REALFLOAT(b), REALFLOAT(r));*/
   return r;
}

wtk_flt_t wtk_flt_sub(wtk_flt_t a,wtk_flt_t b)
{
	wtk_flt_t r;
   if (a.m==0)
      return b;
   else if (b.m==0)
      return a;
   if ((a).e > (b).e)
   {
      r.m = ((a).m>>1) - ((b).m>>MIN(15,(a).e-(b).e+1));
      r.e = (a).e+1;
   }
   else
   {
      r.m = ((a).m>>MIN(15,(b).e-(a).e+1)) - ((b).m>>1);
      r.e = (b).e+1;
   }
   if (r.m>0)
   {
      if (r.m<16384)
      {
         r.m<<=1;
         r.e-=1;
      }
   } else {
      if (r.m>-16384)
      {
         r.m<<=1;
         r.e-=1;
      }
   }
   /*printf ("%f + %f = %f\n", REALFLOAT(a), REALFLOAT(b), REALFLOAT(r));*/
   return r;
}

wtk_flt_t wtk_flt_mul(wtk_flt_t a, wtk_flt_t b)
{
	wtk_flt_t r;

	r.m = (short)((((int)a.m)*(b.m))>>15);
	//r.m = (short)((int)(a).m*(b).m>>15);
	r.e = a.e+b.e+15;
	if (r.m>0)
	{
		if (r.m<16384)
		{
			r.m<<=1;
			r.e-=1;
		}
	} else
	{
		if (r.m>-16384)
		{
			r.m<<=1;
			r.e-=1;
		}
	}
	/*printf ("%f * %f = %f\n", REALFLOAT(a), REALFLOAT(b), REALFLOAT(r));*/
	return r;
}

wtk_flt_t wtk_flt_mul2(wtk_flt_t a, int b,int shift)
{
	a=wtk_flt_mul(a,wtk_flt(b));
	if(a.e>=shift)
	{
		a.e-=shift;
		return a;
	}else
	{
		return wtk_flt_divu(a,wtk_flt3(shift));
	}
}

wtk_flt_t wtk_flt_mul3(wtk_flt_t a,wtk_flt_t b,int shift)
{
	a=wtk_flt_mul(a,b);
	//wtk_flt_print(a);
	if(a.e>=shift || shift<=0)
	{
		a.e-=shift;
		return a;
	}else
	{
		return wtk_flt_divu(a,wtk_flt3(shift));
	}
}

wtk_flt_t wtk_flt_square2(int a)
{
	wtk_flt_t flt;

	flt=wtk_flt(a);
	return wtk_flt_mul(flt,flt);
}

wtk_flt_t wtk_flt_square3(wtk_flt_t a)
{
	return wtk_flt_mul(a,a);
}

wtk_flt_t wtk_flt_square(int a,int shift)
{
	wtk_flt_t flt1;

	flt1=wtk_flt(a);
	flt1=wtk_flt_mul(flt1,flt1);
	if(flt1.e>=shift)
	{
		flt1.e-=shift;
		return flt1;
	}else
	{
		return wtk_flt_divu(flt1,wtk_flt3(shift));
	}
}

wtk_flt_t wtk_flt_mul_int(int a,int b,int shift)
{
	wtk_flt_t flt1,flt2;

	flt1=wtk_flt(a);
	if(a==b)
	{
		flt2=flt1;
	}else
	{
		flt2=wtk_flt(b);
	}
	if(flt1.e+flt2.e>=shift)
	{
		a=flt1.e-shift;
		if(a<0)
		{
			flt1.e=0;
			flt2.e+=a;
		}else
		{
			flt1.e=a;
		}
		return wtk_flt_mul(flt1,flt2);
	}
	//wtk_debug("%d/%d/%d\n",flt1.e,flt2.e,shift);
	flt1=wtk_flt_mul(flt1,flt2);
	if(flt1.e>=shift)
	{
		flt1.e-=shift;
		return flt1;
	}else
	{
		return wtk_flt_divu(flt1,wtk_flt3(shift));
	}
}


short wtk_fix_sqrt(int x);

wtk_flt_t wtk_flt_sqrt(wtk_flt_t a)
{
	wtk_flt_t r;
   int m;

   m = SHL32(EXTEND64(a.m), 14);
   r.e = a.e - 14;
   if (r.e & 1)
   {
      r.e -= 1;
      m <<= 1;
   }
   r.e >>= 1;
   r.m = wtk_fix_sqrt(m);
   return r;
}

static const wtk_flt_t FLOAT_ONE = {16384,-14};
static const wtk_flt_t FLOAT_ZERO = {0,0};

/* Do NOT attempt to divide by a negative number */
wtk_flt_t wtk_flt_divu(wtk_flt_t a, wtk_flt_t b)
{
   int e=0;
   int num;
   wtk_flt_t r;

   if (b.m<=0)
   {
      return FLOAT_ONE;
   }
   num = a.m;
   a.m = ABS16(a.m);
   while (a.m >= b.m)
   {
      e++;
      a.m >>= 1;
   }
   num = num << (15-e);
   r.m = DIV32_16(num,b.m);
   r.e = a.e-b.e-15+e;
   return r;
}

/* Do NOT attempt to divide by a negative number */
wtk_flt_t wtk_flt_div32(int a,int b)
{
   int e0=0,e=0;
   wtk_flt_t r;

   if (a==0)
   {
      return FLOAT_ZERO;
   }
   if (b>32767)
   {
      e0 = wtk_flt_ilog(b)-14;
      b = VSHR32(b, e0);
      e0 = -e0;
   }
   e = wtk_flt_ilog(ABS32(a))-wtk_flt_ilog(b-1)-15;
   a = VSHR32(a, e);
   if (ABS32(a)>=SHL32(EXTEND32(b-1),15))
   {
      a >>= 1;
      e++;
   }
   e += e0;
   r.m = DIV32_16(a,b);
   r.e = e;
   return r;
}



int wtk_fix_mul(int a,int b,int shift)
{
	wtk_flt_t flt;

	flt=wtk_flt_mul(wtk_flt(a),wtk_flt(b));
	flt=wtk_flt_divu(flt,wtk_flt3(shift));
	return  wtk_flt_extract32(flt);
}

wtk_fixvec_t* wtk_fixvec_new(int len)
{
	wtk_fixvec_t *vec;

	vec=(wtk_fixvec_t*)wtk_malloc(sizeof(wtk_fixvec_t));
	vec->len=len;
	vec->shift=0;
	vec->pv=(int*)wtk_calloc(len,sizeof(int));
	return  vec;
}

void wtk_fixvec_delete(wtk_fixvec_t *v)
{
	wtk_free(v->pv);
	wtk_free(v);
}


wtk_fixvec_t* wtk_fixvec_new2(float *pf,int len)
{
	int i;
	wtk_fixvec_t *vec;
	int v=pow(2,15)-1;
	float max,min;
	float f;
	int shift;
	int *iv;

	vec=wtk_fixvec_new(len);
	max=wtk_float_max(pf,len);
	min=wtk_float_min(pf,len);
	//wtk_debug("max=%f min=%f\n",max,min);
	if(max<-min)
	{
		max=-min;
	}
	f=v*1.0/max;
	shift=wtk_flt_ilogl(f);
	if(shift>15)
	{
		shift=15;
	}
	//wtk_debug("f=%f shift=%d\n",f,shift);
	iv=vec->pv;
	vec->shift=shift;
	for(i=0;i<len;++i)
	{
		iv[i]=FLOAT2FIX_ANY(pf[i],shift);
		//wtk_debug("v[%d]=%d/%f/%f\n",i,iv[i],FIX2FLOAT_ANY(iv[i],shift),pf[i]);
	}
	return vec;
}

wtk_fixvec_t* wtk_fixvec_new3(float *pf,int len,int shiftx)
{
	int i;
	wtk_fixvec_t *vec;
	int v=pow(2,20)-1;
	float max,min;
	float f;
	int shift;
	int *iv;

	vec=wtk_fixvec_new(len);
	max=wtk_float_max(pf,len);
	min=wtk_float_min(pf,len);
	//wtk_debug("max=%f min=%f v=%d\n",max,min,v);
	if(max<-min)
	{
		max=-min;
	}
	f=v*1.0/max;
	//wtk_debug("f=%f\n",f);
	shift=wtk_flt_ilogl(f);
	if(shift>shiftx)
	{
		shift=shiftx;
	}else if(shift<shiftx)
	{
		wtk_debug("shift=%d/%d\n",shift,shiftx);
		exit(0);
	}
	//wtk_debug("f=%f shift=%d\n",f,shift);
	iv=vec->pv;
	vec->shift=shift;
	for(i=0;i<len;++i)
	{
		iv[i]=FLOAT2FIX_ANY(pf[i],shift);
		//wtk_debug("v[%d]=%d/%f/%f\n",i,iv[i],FIX2FLOAT_ANY(iv[i],shift),pf[i]);
	}
	return vec;
}


void wtk_fixvecc_delete(wtk_fixvecc_t *v)
{
	wtk_free(v->pv);
	wtk_free(v);
}


int wtk_fixvecc_bytes(wtk_fixvecc_t *v)
{
	return sizeof(wtk_fixvecc_t)+sizeof(char)*v->len;
}

wtk_fixvecc_t* wtk_fixvecc_new(int len)
{
	wtk_fixvecc_t *v;

	v=(wtk_fixvecc_t*)wtk_malloc(sizeof(wtk_fixvecc_t));
	v->pv=(unsigned char*)wtk_calloc(len,sizeof(char));
	v->len=len;
	v->shift=0;
	return v;
}

wtk_fixvecc_t* wtk_fixvecc_new2(float *pf,int len)
{
	wtk_fixvecc_t *v;
	float max,min;
	int shift;
	int i;
	unsigned char *sv;
	float f;
	int vt,vmin;

	v=(wtk_fixvecc_t*)wtk_malloc(sizeof(wtk_fixvecc_t));
	//v->shift=shift;
	sv=v->pv=(unsigned char*)wtk_calloc(len,sizeof(char));
	if(len==1)
	{
		max=fabs(pf[0])+0.5;
		min=0;
	}else
	{
		min=wtk_float_min(pf,len);
		max=wtk_float_max2(pf,len,-min);
	}
	f=pow(2,8)*1.0/max;
	shift=log2(f);
	if(shift<0)
	{
		print_float(pf,len);
		wtk_debug("min=%f max=%f f=%f shift=%d\n",min,max,f,shift);
		exit(0);
	}
	v->shift=shift;

	f=min*(1<<shift);
	vmin=(int)((f>0)?(f+0.5):(f-0.5));
	v->min=vmin;
	if(fabs((double)(vmin-v->min))>1)
	{
		exit(0);
	}
	for(i=0;i<len;++i)
	{
		f=pf[i]*(1<<shift)-vmin;
		vt=(int)((f>0)?(f+0.5):(f-0.5));
		// wtk_debug("v=%d f=%f %f\n",vt,f,pf[i]*(1<<shift));
		if(vt>256)
		{
			//wtk_debug("v=%d\n",v);
			vt=256;
		}else if(vt<0)
		{
			wtk_debug("v=%d is negative\n",vt);
			exit(0);
		}
		sv[i]=vt;//(int)((f>0)?(f+0.5):(f-0.5));
		//wtk_debug("v[%d]=%f/%f\n",i,FIX2FLOAT_ANY(sv[i],shift)+FIX2FLOAT_ANY(vmin,shift),pf[i]);
		if(fabs((double)(vt-sv[i]))>1)
		{
			wtk_debug("vt=%d si=%d\n",vt,sv[i]);
			exit(0);
		}
	}
	//wtk_debug("vec=[%f,%f] shift=%d\n",wtk_float_min(pf,len),wtk_float_max(pf,len),shift);
	//exit(0);
	v->len=len;
	return v;
}

int wtk_fixmatc_bytes(wtk_fixmatc_t *m)
{
	return sizeof(wtk_fixmatc_t)+m->row+m->row*m->col+m->row;
}


wtk_fixmatc_t* wtk_fixmatc_new2(int row,int col)
{
	wtk_fixmatc_t *m;

	m=(wtk_fixmatc_t*)wtk_malloc(sizeof(wtk_fixmatc_t));
	m->row=row;
	m->col=col;
	m->pv=(unsigned char*)wtk_calloc(row*col,sizeof(char));
	m->min=(short*)wtk_calloc(row,sizeof(short));
	m->shift=(unsigned char*)wtk_malloc(row);
	return m;
}

void wtk_fixmatc_delete(wtk_fixmatc_t *m)
{
	wtk_free(m->min);
	wtk_free(m->pv);
	wtk_free(m->shift);
	wtk_free(m);
}



wtk_fixmatc_t* wtk_fixmatc_new(float *pf,int row,int col)
{
	wtk_fixmatc_t *m;
	int i,j;
	float f;
	unsigned char *cv;
	int shift;
	int v;
	float xmin,xmax2;
	int vmin;

	m=(wtk_fixmatc_t*)wtk_malloc(sizeof(wtk_fixmatc_t));
	m->row=row;
	m->col=col;
	//m->pv=(char*)wtk_malloc(row*col);
	m->pv=(unsigned char*)wtk_calloc(row*col,sizeof(char));
	m->min=(short*)wtk_calloc(row,sizeof(short));
	m->shift=(unsigned char*)wtk_malloc(row);
	cv=m->pv;
	for(i=0;i<row;++i)
	{
		if(col==1)
		{
			xmax2=fabs(pf[0])+0.5;
			xmin=pf[0];
		}else
		{
			xmin=wtk_float_min(pf,col);
			xmax2=wtk_float_max2(pf,col,-xmin);
		}
		f=pow(2,8)*1.0/xmax2;
		shift=log2(f);
		//wtk_debug("shift=%d\n",shift);
		//shift+=5;
		//wtk_debug("mat[%d]=[%f,%f] shift=%d\n",i,wtk_float_min(pf,col),wtk_float_max(pf,col),shift);
		if(shift<0)
		{
			print_float(pf,col);
			wtk_debug("min=%f max=%f f=%f shift=%d\n",xmin,xmax2,f,shift);
			exit(0);
		}
		m->shift[i]=shift;
		//wtk_debug("v[%d]=%f/%f shift=%d\n",i,xmax2,wtk_float_max(pf,col),m->shift[i]);
		f=xmin*(1<<shift);
		vmin=(int)((f>0)?(f+0.5):(f-0.5));
		m->min[i]=vmin;
		if(fabs((double)(vmin-m->min[i]))>1)
		{
			wtk_debug("%d/%d\n",vmin,m->min[i]);
			exit(0);
		}
		//wtk_debug("vmin=%d\n",vmin);
		for(j=0;j<col;++j)
		{
			f=pf[j]*(1<<shift)-vmin;
			v=(int)((f>0)?(f+0.5):(f-0.5));
			//wtk_debug("v=%d f=%f %f\n",v,f,pf[j]*(1<<shift));
			if(v>255)
			{
				//wtk_debug("v=%d\n",v);
				v=255;
			}else if(v<0)
			{
				wtk_debug("v=%d is negative\n",v);
				exit(0);
			}
			cv[j]=v;
			//wtk_debug("v[%d/%d]=%d/%d %f/%f v=%d\n",i,j,cv[j],vmin,FIX2FLOAT_ANY(cv[j],shift)+FIX2FLOAT_ANY(vmin,shift),pf[j],v);
			if(fabs((double)(v-cv[j]))>1)
			{
				wtk_debug("v=%d/%d\n",v,cv[j]);
				exit(0);
			}
		}
		//wtk_debug("v[%d]=[%f,%f] shift=%d\n",i,wtk_float_min(pf,col),wtk_float_max(pf,col),shift);
//		if(i>0)
//		{
//			exit(0);
//		}
		//exit(0);
		pf+=col;
		cv+=col;
		//tk_debug("vcnt=%d/%d\n",vcnt,col);
	}
	//exit(0);
	return m;
}

wtk_fixvecs_t* wtk_fixvecs_new(float *pf,int len,int min_shift)
{
	wtk_fixvecs_t *v;
	float max;
	int shift;
	int i,t;
	short *sv;
	float f;

	v=(wtk_fixvecs_t*)wtk_malloc(sizeof(wtk_fixvecs_t));
	//v->shift=shift;
	sv=v->pv=(short*)wtk_calloc(len,sizeof(short));
	max=wtk_float_abs_max(pf,len);
	f=pow(2,15)*1.0/max;
	shift=log2(f);
	//shift=10;
	//wtk_debug("max=%f shift=%d\n",max,shift);
	if(shift>min_shift)
	{
		shift=min_shift;
	}
	if(shift<0)
	{
		exit(0);
	}
	v->shift=shift;
	//wtk_debug("shift=%d\n",shift);
	for(i=0;i<len;++i)
	{
		f=pf[i]*(1<<shift);
		t=(int)((f>0)?(f+0.5):(f-0.5));
		if(t>32767)
		{
			wtk_debug("v=%d\n",t);
			exit(0);
		}else if(t<-32768)
		{
			wtk_debug("v=%d\n",t);
			exit(0);
		}
		sv[i]=t;
		//wtk_debug("v[%d]=%f/%f\n",i,FIX2FLOAT_ANY(sv[i],shift),pf[i]);
	}
	//exit(0);
	v->len=len;
	return v;
}



wtk_fixmats_t* wtk_fixmats_new(float *pf,int row,int col,int min_shift)
{
	wtk_fixmats_t *m;
	short *sv;
	int i,j;
	float xmax,f;
	int shift;
	//static int ki=0;
	int v;

	//++ki;
	m=(wtk_fixmats_t*)wtk_malloc(sizeof(wtk_fixmats_t));
	m->row=row;
	m->col=col;
	m->pv=sv=(short*)wtk_calloc(row*col,sizeof(short));
	m->shift=(unsigned char*)wtk_malloc(row);
	for(i=0;i<row;++i)
	{
//		xmin=wtk_float_min(pf,col);
//		xmax=wtk_float_max2(pf,col,-xmin);
		xmax=wtk_float_abs_max(pf,col);
		f=pow(2,15)*1.0/xmax;
		shift=log2(f);
		//wtk_debug("max=%f/%f shift=%d/%f\n",xmax,xmin,shift,log2(pow(2,15)*1.0/xf));
		if(shift<0)
		{
			exit(0);
		}
		if(shift>min_shift)
		{
			shift=min_shift;
		}
		m->shift[i]=shift;
		//wtk_debug("v[%d]=%f shift=%d\n",i,max,m->shift[i]);
		for(j=0;j<col;++j)
		{
			f=pf[j]*(1<<shift);
			v=(int)((f>0)?(f+0.5):(f-0.5));
			if(v>32767)
			{
				wtk_debug("v=%d\n",v);
				exit(0);
			}else if(v<-32768)
			{
				wtk_debug("v=%d\n",v);
				exit(0);
			}
			sv[j]=v;
			//wtk_debug("v[%d/%d]=%f/%f\n",i,j,FIX2FLOAT_ANY(cv[j],shift),pf[j]);
		}
		pf+=col;
		sv+=col;
		//wtk_debug("v[%d]=%d\n",i,shift);
	}
	//wtk_debug("min=%d max=%d\n",xmin,xmax);
	//exit(0);
	return m;
}

//#define MIN_SHIFT 15
//#define WORD_BIT 15
//#define MIN_SHIFT 18

int wtk_fixvecs_bytes(wtk_fixvecs_t *vec)
{
	int bytes;

	bytes=sizeof(wtk_fixvecs_t);
	bytes+=vec->len*sizeof(short);
	return bytes;
}

wtk_fixvecs_t* wtk_fixvecs_new2(int len)
{
	wtk_fixvecs_t *vec;

	vec=(wtk_fixvecs_t*)wtk_malloc(sizeof(wtk_fixvecs_t));
	vec->len=len;
	vec->shift=0;
	vec->pv=(short*)wtk_calloc(len,sizeof(short));
	return vec;
}

void wtk_fixvecs_delete(wtk_fixvecs_t *v)
{
	wtk_free(v->pv);
	wtk_free(v);
}




wtk_fixmats_t* wtk_fixmats_new2(int row,int col)
{
	wtk_fixmats_t *m;

	m=(wtk_fixmats_t*)wtk_malloc(sizeof(wtk_fixmats_t));
	m->row=row;
	m->col=col;
	m->pv=(short*)wtk_calloc(row*col,sizeof(short));
	m->shift=(unsigned char*)wtk_malloc(row);
	return m;
}

int wtk_fixmats_bytes(wtk_fixmats_t *m)
{
	int bytes;

	bytes=sizeof(wtk_fixmats_t);
	bytes+=m->row*m->col*sizeof(short);
	bytes+=m->row;
	return bytes;
}

void wtk_fixmats_delete(wtk_fixmats_t* m)
{
	wtk_free(m->pv);
	wtk_free(m->shift);
	wtk_free(m);
}





/*
 * 0 7
 */
int wtk_fltchar_base(float f)
{
	int r;
	int nx=1;

	if(f>=(1<<(nx+7*WTK_FLTCHAR_STEP)))
	{
		r=7;
	}else if(f>=(1<<(nx+6*WTK_FLTCHAR_STEP)))
	{
		r=6;
	}else if(f>=(1<<(nx+5*WTK_FLTCHAR_STEP)))
	{
		r=5;
	}else if(f>=(1<<(nx+4*WTK_FLTCHAR_STEP)))
	{
		r=4;
	}else if(f>=(1<<(nx+3*WTK_FLTCHAR_STEP)))
	{
		r=3;
	}else if(f>=(1<<(nx+2*WTK_FLTCHAR_STEP)))
	{
		r=2;
	}else if(f>=(1<<(1*WTK_FLTCHAR_STEP+nx)))
	{
		r=1;
	}else
	{
		r=0;
	}
	return r;
}

void wtk_fltchar_print(wtk_fltchar_t c)
{
	float f;

	f=(c.m<<(c.e*WTK_FLTCHAR_STEP))*1.0/(1<<(6*WTK_FLTCHAR_STEP));
	wtk_debug("%f(%d+%d)\n",f,c.m,c.e);
}


wtk_fltchar_t wtk_fltchar_init(float f)
{
	wtk_fltchar_t c;
	int v;
	int sign;

	//0.001 => 10
	if(f<0)
	{
		sign=1;
		f=-f;
	}else
	{
		sign=0;
	}
	v=f*(1<<(6*WTK_FLTCHAR_STEP));
	//wtk_debug("v=%d f=%f\n",v,FIX2FLOAT_ANY(v,6*WTK_FLTCHAR_STEP));
	c.e=wtk_fltchar_base(v);
	//wtk_debug("e=%d v=%d\n",c.e,v>>(c.e*WTK_FLTCHAR_STEP));
	if(sign)
	{
		//c.m=-(v>>(c.e*WTK_FLTCHAR_STEP));
		c.m=-(v-(1<<(c.e*WTK_FLTCHAR_STEP-1)))>>(c.e*WTK_FLTCHAR_STEP);
	}else
	{
		c.m=(v+(1<<(c.e*WTK_FLTCHAR_STEP-1)))>>(c.e*WTK_FLTCHAR_STEP);
		//c.m=(v)>>(c.e*WTK_FLTCHAR_STEP);
	}
	//wtk_debug("c=%d+%d\n",c.e,c.m);
	return c;
}




wtk_fltchar_t wtk_fltchar_mul(wtk_fltchar_t a,wtk_fltchar_t b)
{
	wtk_fltchar_t c;
	int v,v2;

	//-16 15
	v=a.e+b.e-6;
	v2=((int)(a.m)*(b.m));
	//wtk_debug("%d+%d\n",v2,v);
	if(v2>15)
	{
		do
		{
			v2>>=WTK_FLTCHAR_STEP;
			++v;
		}while(v2>15);
	}else if(v2<-16)
	{
		do
		{
			v2>>=WTK_FLTCHAR_STEP;
			++v;
		}while(v2<-16);
	}
	//wtk_debug("%d+%d\n",v2,v);
	//wtk_debug("%d+%d\n",v2,v);
	c.m=v2;
	c.e=v;
	return c;
}


#include "wtk/core/wtk_os.h"

void wtk_fltchar_test(void)
{
	wtk_fltchar_t c;
	wtk_fltchar_t c1;
	wtk_fltchar_t c2;
	float f1,f2,f;
	int i;
	double t;
	int v;

	if(0)
	{
		for(i=-100;i<=100;++i)
		{
			wtk_debug("i=%d\n",i);
			c1=wtk_fltchar_init(i);
			wtk_fltchar_print(c1);

		}
		exit(0);
	}
	f1=0.001;
	f2=8.0;

	f1=10;
	f2=12;

	c1=wtk_fltchar_init(f1);
	c2=wtk_fltchar_init(f2);
	c=wtk_fltchar_mul(c1,c2);
	wtk_fltchar_print(c1);
	wtk_fltchar_print(c2);
	wtk_fltchar_print(c);
	//exit(0);

	t=time_get_ms();
	for(i=0;i<100000;++i)
	{
		f=f1*f2;
	}
	t=time_get_ms()-t;
	wtk_debug("time=%f f=%f\n",t,f);
	t=time_get_ms();
	for(i=0;i<100000;++i)
	{
		WTK_FLTCHAR_MUL(c,c1,c2,v);
		//c=wtk_fltchar_mul(c1,c2);
	}
	t=time_get_ms()-t;
	wtk_debug("time=%f\n",t);
	wtk_fltchar_print(c);
	exit(0);

	c=wtk_fltchar_init(-16);
	wtk_fltchar_print(c);

	c=wtk_fltchar_init(15);
	wtk_fltchar_print(c);

	c=wtk_fltchar_init(8);
	wtk_fltchar_print(c);

	c=wtk_fltchar_init(0.5);
	wtk_fltchar_print(c);

	c=wtk_fltchar_init(0);
	wtk_fltchar_print(c);

	c=wtk_fltchar_init(-0.5);
	wtk_fltchar_print(c);

	c=wtk_fltchar_init(0.1);
	wtk_fltchar_print(c);

	c=wtk_fltchar_init(0.01);
	wtk_fltchar_print(c);

	c=wtk_fltchar_init(0.001);
	wtk_fltchar_print(c);

	c=wtk_fltchar_init(0.0001);
	wtk_fltchar_print(c);
	exit(0);
}


#ifdef USE_AUX
void wtk_fix_test_mul(void)
{
	__m128i x,y,z,tot;
	char *c,*b,*d;
	int i;
	int n=16;
	short *sv;
	int iter=1000000;
	double t;
	float f;

	c=aligned_alloc(64,n);
	b=aligned_alloc(64,n);
	d=aligned_alloc(64,n);
	for(i=0;i<16;i+=2)
	{
		c[i]=i;
		c[i+1]=0;
		b[i]=3;
		b[i+1]=0;
	}
//	for(i=0;i<16;++i)
//	{
//		wtk_debug("v[%d]=%d/%d\n",i,c[i],b[i]);
//	}
//	for(i=0;i<8;++i)
//	{
//		wtk_debug("xv[%d]=%d\n",i,c[i*2]);
//	}
	if(1)
	{
		float xv[32];
		float a[8]={0,1,0};
		float b[8]={0,1,0};

		t=time_get_ms();
		f=0;
		for(i=0;i<iter;++i)
		{
			xv[0]=a[0]*b[0];
			xv[1]=a[1]*b[1];
			xv[2]=a[2]*b[2];
			xv[3]=a[3]*b[3];
			xv[4]=a[4]*b[4];
			xv[5]=a[5]*b[5];
			xv[6]=a[6]*b[6];
			xv[7]=a[7]*b[7];
			f+=xv[0]+xv[1]+xv[2]+xv[3]+xv[4]+xv[5]+xv[6]+xv[7];
		}
		t=time_get_ms()-t;
		wtk_debug("time float=%f\n",t);
		wtk_debug("xv[%d]=%f f=%f\n",1,xv[1],f);
	}
	if(1)
	{
		short xv[32];
		short *sv1,*sv2;

		sv1=(short*)c;
		sv2=(short*)b;
		t=time_get_ms();
		f=0;
		for(i=0;i<iter;++i)
		{
			xv[0]=sv1[0]*sv2[0];
			xv[1]=sv1[1]*sv2[1];
			xv[2]=sv1[2]*sv2[2];
			xv[3]=sv1[3]*sv2[3];
			xv[4]=sv1[4]*sv2[4];
			xv[5]=sv1[5]*sv2[5];
			xv[6]=sv1[6]*sv2[6];
			xv[7]=sv1[7]*sv2[7];
			f+=xv[0]+xv[1]+xv[2]+xv[3]+xv[4]+xv[5]+xv[6]+xv[7];
		}
		//f+=xv[0]+xv[1]+xv[2]+xv[3]+xv[4]+xv[5]+xv[6]+xv[7];
		t=time_get_ms()-t;
		wtk_debug("time short=%f\n",t);
		wtk_debug("xv[%d]=%d f=%f\n",1,xv[1],f);
	}
	t=time_get_ms();
	f=0;
	sv=(short*)d;
	//_mm_store_si128((__m128i*)d,y);
	//wtk_debug("xsv1[%d]=%d\n",1,sv[1]);
	wtk_debug("xsv2[%d]=%d\n",1,sv[1]);

	sv=(short*)(&z);
	tot=_mm_setzero_si128();
	for(i=0;i<iter;++i)
	{
		//x=_mm_set_epi16(c[0],c[2],c[4],c[6],c[8],c[10],c[12],c[14]);
		//y=_mm_set_epi16(b[0],b[2],b[4],b[6],b[8],b[10],b[12],b[14]);
		//_mm_store_si128((__m128i*)d,y);
		//_mm_store_si128((__m128i*)d,y);
		x=_mm_load_si128((__m128i*)c);
		y=_mm_load_si128((__m128i*)b);
		z=_mm_mullo_epi16(x,y);
		tot=_mm_hadd_epi16(tot,z);
		//f+=sv[0]+sv[1]+sv[2]+sv[3]+sv[4]+sv[5]+sv[6]+sv[7];
	}
	t=time_get_ms()-t;
	wtk_debug("time mm=%f f=%f\n",t,f);
	//sv=(short*)d;
	sv=(short*)(&tot);
	for(i=0;i<8;++i)
	{
		wtk_debug("r[%d]=%d\n",i,sv[i]);
	}
	exit(0);
}

void wtk_fix_test_mul2()
{
	__m128i x,y;
	char *c,*b;
	int i;
	int n=16;
	double t;
	char v[32];
	int iter=10000000;
	int j;

	c=aligned_alloc(64,n);
	b=aligned_alloc(64,n);
	for(i=0;i<n;++i)
	{
		c[i]=i;
	}
	if(1)
	{
		t=time_get_ms();
		for(i=0;i<iter;++i)
		{
			for(j=0;j<16;++j)
			{
				v[j]=c[j]+c[j];
			}
		}
		t=time_get_ms()-t;
		wtk_debug("v[%d]=%d\n",1,v[1]);
		wtk_debug("time=%f\n",t);
	}
	if(1)
	{
		t=time_get_ms();
		for(i=0;i<iter;++i)
		{
			v[0]=c[0]+c[0];
			v[1]=c[1]+c[1];
			v[2]=c[2]+c[2];
			v[3]=c[3]+c[3];

			v[4]=c[4]+c[4];
			v[5]=c[5]+c[5];
			v[6]=c[6]+c[6];
			v[7]=c[7]+c[7];

			v[8]=c[8]+c[8];
			v[9]=c[9]+c[9];
			v[10]=c[10]+c[10];
			v[11]=c[11]+c[11];

			v[12]=c[12]+c[12];
			v[13]=c[13]+c[13];
			v[14]=c[14]+c[14];
			v[15]=c[15]+c[15];
		}
		t=time_get_ms()-t;
		wtk_debug("v[%d]=%d\n",1,v[1]);
		wtk_debug("time=%f\n",t);
	}
	t=time_get_ms();
	for(i=0;i<iter;++i)
	{
		x=_mm_load_si128((__m128i*)c);
		y=_mm_add_epi8(x,x);
		_mm_store_si128((__m128i*)b,y);
	}
	t=time_get_ms()-t;
	wtk_debug("time=%f\n",t);
	for(i=0;i<16;++i)
	{
		wtk_debug("v[%d]=%d\n",i,b[i]);
	}
	exit(0);
}
#endif
