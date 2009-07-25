#include <math.h>

#include "sha123/tables.h"
#include "sha123/debug.h"

#ifndef M_PI
# define M_PI 3.14159265359
#endif

static int current_scaleval;

real sha123_muls[27][64];	/* sed by layer 1/2 */
real sha123_decwin[512 + 32];
static real cos64[16], cos32[8], cos16[4], cos8[2], cos4[1];
real * sha123_pnts[] = { cos64, cos32, cos16, cos8, cos4 };

static long intwinbase[] = {
  0, -1, -1, -1, -1, -1, -1, -2, -2, -2,
  -2, -3, -3, -4, -4, -5, -5, -6, -7, -7,
  -8, -9, -10, -11, -13, -14, -16, -17, -19, -21,
  -24, -26, -29, -31, -35, -38, -41, -45, -49, -53,
  -58, -63, -68, -73, -79, -85, -91, -97, -104, -111,
  -117, -125, -132, -139, -147, -154, -161, -169, -176, -183,
  -190, -196, -202, -208, -213, -218, -222, -225, -227, -228,
  -228, -227, -224, -221, -215, -208, -200, -189, -177, -163,
  -146, -127, -106, -83, -57, -29, 2, 36, 72, 111,
  153, 197, 244, 294, 347, 401, 459, 519, 581, 645,
  711, 779, 848, 919, 991, 1064, 1137, 1210, 1283, 1356,
  1428, 1498, 1567, 1634, 1698, 1759, 1817, 1870, 1919, 1962,
  2001, 2032, 2057, 2075, 2085, 2087, 2080, 2063, 2037, 2000,
  1952, 1893, 1822, 1739, 1644, 1535, 1414, 1280, 1131, 970,
  794, 605, 402, 185, -45, -288, -545, -814, -1095, -1388,
  -1692, -2006, -2330, -2663, -3004, -3351, -3705, -4063, -4425, -4788,
  -5153, -5517, -5879, -6237, -6589, -6935, -7271, -7597, -7910, -8209,
  -8491, -8755, -8998, -9219, -9416, -9585, -9727, -9838, -9916, -9959,
  -9966, -9935, -9863, -9750, -9592, -9389, -9139, -8840, -8492, -8092,
  -7640, -7134, -6574, -5959, -5288, -4561, -3776, -2935, -2037, -1082,
  -70, 998, 2122, 3300, 4533, 5818, 7154, 8540, 9975, 11455,
  12980, 14548, 16155, 17799, 19478, 21189, 22929, 24694, 26482, 28289,
  30112, 31947, 33791, 35640, 37489, 39336, 41176, 43006, 44821, 46617,
  48390, 50137, 51853, 53534, 55178, 56778, 58333, 59838, 61289, 62684,
  64019, 65290, 66494, 67629, 68692, 69679, 70590, 71420, 72169, 72835,
  73415, 73908, 74313, 74630, 74856, 74992, 75038
};

static void sha123_make_decode_tables_fpu(int scaleval)
{
  int i, j;
  real *table, *costab;
  double scale;

  /* Store current scale value */
  if (current_scaleval == scaleval) {
    sha123_debug(" -> already done\n");
    return;
  }
  current_scaleval = scaleval;

  /* Init cosinus tables */

  for (i = 0; i < 5; i++) {
    int kr = 0x10 >> i;
    int divv = 0x40 >> i;
    costab = sha123_pnts[i];
    for (j = 0; j < kr; j++) {
      costab[j] = 1.0 /
	(2.0 * cos(M_PI * ((double) j * 2.0 + 1.0) / (double) divv));
/*       sha123_debug("costab[%d,%d] = %.5f\n", i,j,costab[j]); */
    }
  }
  sha123_debug("   -> cosinus tables done\n");

#if 0
  /* Init decimation tables */
  table = sha123_decwin;
  scaleval = -scaleval;
  scale = (double) scaleval / 65536.0;
  for (i = 0, j = 0; i < 256; i++, j++, table += 32) {
    if (table < sha123_decwin + 512 + 16) {
      table[16] = table[0] =
	(double) intwinbase[j] * scale;
    }
    if ((i & 31) == 31)
      table -= 1023;
    if ((i & 63) == 63)
      scale = -scale;
  }
  
  for ( /* i=256 */ ; i < 512; i++, j--, table += 32) {
    if (table < sha123_decwin + 512 + 16) {
      table[16] = table[0] = (double) intwinbase[j] * scale;
    }
    if ((i & 31) == 31)
      table -= 1023;
    if ((i & 63) == 63)
      scale = -scale;
  }
#else
  table = sha123_decwin;
  scaleval = -scaleval;
  for (i = 0, j = 0; i < 256; i++, j++, table += 32) {
    if (table < sha123_decwin + 512 + 16) {
      table[16] = table[0] =
	(double) intwinbase[j] / 65536.0 * (double) scaleval;
    }
    if ((i & 31) == 31)
      table -= 1023;
    if ((i & 63) == 63)
      scaleval = -scaleval;
  }

  for ( /* i=256 */ ; i < 512; i++, j--, table += 32) {
    if (table < sha123_decwin + 512 + 16) {
      table[16] = table[0] = 
	(double) intwinbase[j] / 65536.0 * (double) scaleval;
    }
    if ((i & 31) == 31)
      table -= 1023;
    if ((i & 63) == 63)
      scaleval = -scaleval;
  }
#endif


/*   for (i=0;i<512+32; ++i) { */
/*     sha123_debug("decwin[%d] = %.5f\n", i, sha123_decwin[i]); */
/*   } */

  sha123_debug("   -> decimation tables done\n");
}

#ifdef USE_SIMD

gint16 sha123_decwins[(512 + 32) * 2];

static void sha123_make_decode_tables_mmx(long scaleval)
{
  int i, j, p, a;

  scaleval = -scaleval;
  a = 1;
  for (i = 0, j = 0, p = 0; i < 512; i++, j += a, p += 32) {
    if (p < 512 + 16) {
      int val = ((gint64)intwinbase[j] * scaleval) >> 17;
      
      val = CLAMP(val, -32767, 32767);
      if (p < 512) {
	int n = 1055 - p;
	sha123_decwins[n - 16] = val;
	sha123_decwins[n] = val;
      }
      if (!(p & 1))
	val = -val;
      sha123_decwins[p + 16] = val;
      sha123_decwins[p] = val;
    }
    if ((i & 31) == 31)
      p -= 1023;
    if ((i & 63) == 63)
      scaleval = -scaleval;
    if (i == 256)
      a = -1;
  }
}

#endif

int sha123_make_decode_tables(int scaleval)
{
  sha123_debug("make decode tables [%d]\n",scaleval);
  sha123_make_decode_tables_fpu(scaleval);
#ifdef USE_SIMD
  sha123_make_decode_tables_mmx(scaleval);
#endif
  sha123_debug("-> done\n");
  return 0;
}
