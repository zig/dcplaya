/* 
   This is an optimized DCT from Jeff Tsay's maplay 1.2+ package.
   Saved one multiplication by doing the 'twiddle factor' stuff
   together with the window mul. (MH)

   This uses Byeong Gi Lee's Fast Cosine Transform algorithm, but the
   9 point IDCT needs to be reduced further. Unfortunately, I don't
   know how to do that, because 9 is not an even number. - Jeff.

   ****************************************************************

   9 Point Inverse Discrete Cosine Transform

   This piece of code is Copyright 1997 Mikko Tommila and is freely usable
   by anybody. The algorithm itself is of course in the public domain.

   Again derived heuristically from the 9-point WFTA.

   The algorithm is optimized (?) for speed, not for small rounding errors or
   good readability.

   36 additions, 11 multiplications

   Again this is very likely sub-optimal.

   The code is optimized to use a minimum number of temporary variables,
   so it should compile quite well even on 8-register Intel x86 processors.
   This makes the code quite obfuscated and very difficult to understand.

   References:
   [1] S. Winograd: "On Computing the Discrete Fourier Transform",
   Mathematics of Computation, Volume 32, Number 141, January 1978,
   Pages 175-199
*/

#include "sha123/dct36.h"
#include "sha123/defs.h"

static const real COS6_1 = 0.866025403785; /* cos(M_PI / 6.0 * (double) 1); */
static const real COS6_2 = 0.5;            /* cos(M_PI / 6.0 * (double) 2); */

static const real cos9_0  = 0.939692620786;  /* cos(1.0 * M_PI / 9.0); */
static const real cos9_1  = -0.173648177673; /* cos(5.0 * M_PI / 9.0); */
static const real cos9_2  = -0.766044443118; /* cos(7.0 * M_PI / 9.0); */
static const real cos18_0 = 0.984807753012;  /* cos(1.0 * M_PI / 18.0); */
static const real cos18_1 = -0.342020143322; /* cos(11.0 * M_PI / 18.0); */
static const real cos18_2 = -0.642787609685; /* cos(13.0 * M_PI / 18.0); */

/* tfcos36[i] = 0.5 / cos(M_PI * (double) (i * 2 + 1) / 36.0); */
static const real tfcos36[9] = {
  0.50190991877167367985,
  0.51763809020504147895,
  0.55168895948124585527,
  0.61038729438072802935,
  0.70710678118654746172,
  0.87172339781054897223,
  1.18310079157624925550,
  1.93185165257813684647,
  5.73685662283492980862
};

/*------------------------------------------------------------------*/
/*                                                                  */
/*    Function: Calculation of the inverse MDCT                     */
/*                                                                  */
/*------------------------------------------------------------------*/

#define MACRO(v)						\
do {								\
	real tmpval;						\
								\
	tmpval = tmp[(v)] + tmp[17-(v)];			\
	out2[9+(v)] = tmpval * w[27+(v)];			\
	out2[8-(v)] = tmpval * w[26-(v)];			\
	tmpval = tmp[(v)] - tmp[17-(v)];			\
	ts[SBLIMIT*(8-(v))] = out1[8-(v)] + tmpval * w[8-(v)];	\
	ts[SBLIMIT*(9+(v))] = out1[9+(v)] + tmpval * w[9+(v)];	\
} while (0)

void sha123_dct36_std(real * inbuf,
		      real * o1, real * o2,
		      const real * wintab, real * tsbuf)
{
  real tmp[18];
  register real *in = inbuf;

  in[17] += in[16];
  in[16] += in[15];
  in[15] += in[14];
  in[14] += in[13];
  in[13] += in[12];
  in[12] += in[11];
  in[11] += in[10];
  in[10] += in[9];
  in[9] += in[8];
  in[8] += in[7];
  in[7] += in[6];
  in[6] += in[5];
  in[5] += in[4];
  in[4] += in[3];
  in[3] += in[2];
  in[2] += in[1];
  in[1] += in[0];

  in[17] += in[15];
  in[15] += in[13];
  in[13] += in[11];
  in[11] += in[9];
  in[9] += in[7];
  in[7] += in[5];
  in[5] += in[3];
  in[3] += in[1];


  {
    real t3;
    {
      real t0, t1, t2;

      t0 = COS6_2 * (in[8] + in[16] - in[4]);
      t1 = COS6_2 * in[12];

      t3 = in[0];
      t2 = t3 - t1 - t1;
      tmp[1] = tmp[7] = t2 - t0;
      tmp[4] = t2 + t0 + t0;
      t3 += t1;

      t2 = COS6_1 * (in[10] + in[14] - in[2]);
      tmp[1] -= t2;
      tmp[7] += t2;
    }
    {
      real t0, t1, t2;

      t0 = cos9_0 * (in[4] + in[8]);
      t1 = cos9_1 * (in[8] - in[16]);
      t2 = cos9_2 * (in[4] + in[16]);

      tmp[2] = tmp[6] = t3 - t0 - t2;
      tmp[0] = tmp[8] = t3 + t0 + t1;
      tmp[3] = tmp[5] = t3 - t1 + t2;
    }
  }
  {
    real t1, t2, t3;

    t1 = cos18_0 * (in[2] + in[10]);
    t2 = cos18_1 * (in[10] - in[14]);
    t3 = COS6_1 * in[6];

    {
      real t0 = t1 + t2 + t3;
      tmp[0] += t0;
      tmp[8] -= t0;
    }

    t2 -= t3;
    t1 -= t3;

    t3 = cos18_2 * (in[2] + in[14]);

    t1 += t3;
    tmp[3] += t1;
    tmp[5] -= t1;

    t2 -= t3;
    tmp[2] += t2;
    tmp[6] -= t2;
  }

  {
    real t0, t1, t2, t3, t4, t5, t6, t7;

    t1 = COS6_2 * in[13];
    t2 = COS6_2 * (in[9] + in[17] - in[5]);

    t3 = in[1] + t1;
    t4 = in[1] - t1 - t1;
    t5 = t4 - t2;

    t0 = cos9_0 * (in[5] + in[9]);
    t1 = cos9_1 * (in[9] - in[17]);

    tmp[13] = (t4 + t2 + t2) * tfcos36[17 - 13];
    t2 = cos9_2 * (in[5] + in[17]);

    t6 = t3 - t0 - t2;
    t0 += t3 + t1;
    t3 += t2 - t1;

    t2 = cos18_0 * (in[3] + in[11]);
    t4 = cos18_1 * (in[11] - in[15]);
    t7 = COS6_1 * in[7];

    t1 = t2 + t4 + t7;
    tmp[17] = (t0 + t1) * tfcos36[17 - 17];
    tmp[9] = (t0 - t1) * tfcos36[17 - 9];
    t1 = cos18_2 * (in[3] + in[15]);
    t2 += t1 - t7;

    tmp[14] = (t3 + t2) * tfcos36[17 - 14];
    t0 = COS6_1 * (in[11] + in[15] - in[3]);
    tmp[12] = (t3 - t2) * tfcos36[17 - 12];

    t4 -= t1 + t7;

    tmp[16] = (t5 - t0) * tfcos36[17 - 16];
    tmp[10] = (t5 + t0) * tfcos36[17 - 10];
    tmp[15] = (t6 + t4) * tfcos36[17 - 15];
    tmp[11] = (t6 - t4) * tfcos36[17 - 11];
  }

  {
    register real *out2 = o2;
    const register real *w = wintab;
    register real *out1 = o1;
    register real *ts = tsbuf;

    MACRO(0);
    MACRO(1);
    MACRO(2);
    MACRO(3);
    MACRO(4);
    MACRO(5);
    MACRO(6);
    MACRO(7);
    MACRO(8);
  }
}

sha123_dct_info_t sha123_dct36_std_info = {
  "FPU dct-36",
  (1<<DCT_MODE_SAFEST) | (1<<DCT_MODE_DEFAULT),
  0,
  0,
  sha123_dct36_std
};
