
/* 
 * Mpeg Layer-3 audio decoder 
 * --------------------------
 * copyright (c) 1995-1999 by Michael Hipp.
 * All rights reserved. See also 'README'
 *
 * Optimize-TODO: put short bands into the band-field without the stride of
 * 3 reals
 * Length-optimze: unify long and short band code where it is possible
 */

#include <math.h>

#include "sha123/debug.h"
#include "sha123/sha123.h"
#include "sha123/huffman.h"
#include "sha123/bitstream.h"
#include "sha123/dct12.h"
#include "sha123/dct36.h"

static real ispow[8207];
static real aa_ca[8], aa_cs[8];
static real COS1[12][6];
static real win[4][36];
static real win1[4][36];
static real gainpow2[256 + 118 + 4];
/* static real COS9[9]; */

/* static real COS6_1, COS6_2; */
/* real tfcos36[9]; */
/* static real tfcos12[3]; */
/* #define NEW_DCT9 */
/* #ifdef NEW_DCT9 */
/* static real cos9[3], cos18[3]; */
/* #endif */

#define DCT36 sha123_dct36

struct bandInfoStruct
{
  int longIdx[23];
  int longDiff[22];
  int shortIdx[14];
  int shortDiff[13];
};

struct gr_info_s
{
  int scfsi;
  unsigned part2_3_length;
  unsigned big_values;
  unsigned scalefac_compress;
  unsigned block_type;
  unsigned mixed_block_flag;
  unsigned table_select[3];
  unsigned subblock_gain[3];
  unsigned maxband[3];
  unsigned maxbandl;
  unsigned maxb;
  unsigned region1start;
  unsigned region2start;
  unsigned preflag;
  unsigned scalefac_scale;
  unsigned count1table_select;
  real *full_gain[3];
  real *pow2gain;
};

struct III_sideinfo
{
  unsigned main_data_begin;
  unsigned private_bits;
  struct {
    struct gr_info_s gr[2];
  }
  ch[2];
};

/* Used by the getbits macros */

const struct bandInfoStruct bandInfo[9] = {
  /* MPEG 1.0 */
  { {0,4,8,12,16,20,24,30,36,44,52,62,74,90,110,134,162,196,238,288,
     342,418,576},
    {4,4,4,4,4,4,6,6,8, 8,10,12,16,20,24,28,34,42,50,54, 76,158},
    {0,4*3,8*3,12*3,16*3,22*3,30*3,40*3,52*3,66*3, 84*3,106*3,136*3,192*3},
    {4,4,4,4,6,8,10,12,14,18,22,30,56} } ,

  { {0,4,8,12,16,20,24,30,36,42,50,60,72, 88,106,128,156,190,230,276,330,
     384,576},
    {4,4,4,4,4,4,6,6,6, 8,10,12,16,18,22,28,34,40,46,54, 54,192},
    {0,4*3,8*3,12*3,16*3,22*3,28*3,38*3,50*3,64*3, 80*3,100*3,126*3,192*3},
    {4,4,4,4,6,6,10,12,14,16,20,26,66} } ,

  { {0,4,8,12,16,20,24,30,36,44,54,66,82,102,126,156,194,240,296,364,448,
     550,576} ,
    {4,4,4,4,4,4,6,6,8,10,12,16,20,24,30,38,46,56,68,84,102, 26} ,
    {0,4*3,8*3,12*3,16*3,22*3,30*3,42*3,58*3,78*3,104*3,138*3,180*3,192*3} ,
    {4,4,4,4,6,8,12,16,20,26,34,42,12} }  ,

  /* MPEG 2.0 */
  { {0,6,12,18,24,30,36,44,54,66,80,96,116,140,168,200,238,284,336,396,464,
     522,576},
    {6,6,6,6,6,6,8,10,12,14,16,20,24,28,32,38,46,52,60,68,58,54 } ,
    {0,4*3,8*3,12*3,18*3,24*3,32*3,42*3,56*3,74*3,100*3,132*3,174*3,192*3} ,
    {4,4,4,6,6,8,10,14,18,26,32,42,18 } } ,
  /* changed 19th value from 330 to 332 */
  { {0,6,12,18,24,30,36,44,54,66,80,96,114,136,162,194,232,278,332,
     394,464,540,576},
    {6,6,6,6,6,6,8,10,12,14,16,18,22,26,32,38,46,54,62,70,76,36 } ,
    {0,4*3,8*3,12*3,18*3,26*3,36*3,48*3,62*3,80*3,104*3,136*3,180*3,192*3} ,
    {4,4,4,6,8,10,12,14,18,24,32,44,12 } } ,

  { {0,6,12,18,24,30,36,44,54,66,80,96,116,140,168,200,238,284,336,396,
     464,522,576},
    {6,6,6,6,6,6,8,10,12,14,16,20,24,28,32,38,46,52,60,68,58,54 },
    {0,4*3,8*3,12*3,18*3,26*3,36*3,48*3,62*3,80*3,104*3,134*3,174*3,192*3},
    {4,4,4,6,8,10,12,14,18,24,30,40,18 } } ,
  /* MPEG 2.5 */
  { {0,6,12,18,24,30,36,44,54,66,80,96,116,140,168,200,238,284,336,396,
     464,522,576} ,
    {6,6,6,6,6,6,8,10,12,14,16,20,24,28,32,38,46,52,60,68,58,54},
    {0,12,24,36,54,78,108,144,186,240,312,402,522,576},
    {4,4,4,6,8,10,12,14,18,24,30,40,18} },
  { {0,6,12,18,24,30,36,44,54,66,80,96,116,140,168,200,238,284,336,
     396,464,522,576} ,
    {6,6,6,6,6,6,8,10,12,14,16,20,24,28,32,38,46,52,60,68,58,54},
    {0,12,24,36,54,78,108,144,186,240,312,402,522,576},
    {4,4,4,6,8,10,12,14,18,24,30,40,18} },
  { {0,12,24,36,48,60,72,88,108,132,160,192,232,280,336,400,476,566,568,
     570,572,574,576},
    {12,12,12,12,12,12,16,20,24,28,32,40,48,56,64,76,90,2,2,2,2,2},
    {0, 24, 48, 72,108,156,216,288,372,480,486,492,498,576},
    {8,8,8,12,16,20,24,28,36,2,2,2,26} } ,
};

static int mapbuf0[9][152];
static int mapbuf1[9][156];
static int mapbuf2[9][44];
static int *map[9][3];
static int *mapend[9][3];

static unsigned int n_slen2[512]; /* MPEG 2.0 slen for 'normal' mode */
static unsigned int i_slen2[256]; /* MPEG 2.0 slen for intensity stereo */

static real tan1_1[16], tan2_1[16], tan1_2[16], tan2_2[16];
static real pow1_1[2][16], pow2_1[2][16], pow1_2[2][16], pow2_2[2][16];

void sha123_init_layer3_frame(sha123_t * sha123,
			      int sfreq, int down_sample_sblimit)
{
  int i;
  const struct bandInfoStruct * bi;

  if (sfreq == sha123->limits.frq &&
      down_sample_sblimit == sha123->limits.downsample) {
    return;
  }  
  sha123_debug("Init layer III limits (f:%d ds:%d) old:(f:%d ds:%d)\n",
	       sfreq, down_sample_sblimit,
	       sha123->limits.frq, sha123->limits.downsample);

  sha123->limits.frq = sfreq;
  sha123->limits.downsample = down_sample_sblimit;

  bi = bandInfo + sfreq;

  for (i = 0; i < 23; i++) {
    int v = (bi->longIdx[i] - 1 + 8) / 18 + 1;
    sha123->limits.Long[i] = (v > down_sample_sblimit)
      ? down_sample_sblimit : v;

/*     sha123_debug("limits.Long[%d] = %d,%d\n", i, sha123->limits.Long[i],v); */
  }

  for (i = 0; i < 14; i++) {
    int v = (bi->shortIdx[i] - 1) / 18 + 1;
    sha123->limits.Short[i] = (v > down_sample_sblimit)
      ? down_sample_sblimit : v;

/*     sha123_debug("limits.Short[%d] = %d,%d\n", i, sha123->limits.Short[i],v); */

  }
}

/* 
 * init tables for layer-3 
 */
void sha123_init_layer3(void)
{
  int i, j, k, l;

  sha123_debug("Init layer III\n");

  for (i = -256; i < 118 + 4; i++) {
    gainpow2[i + 256] = pow((double) 2.0, -0.25 * (double) (i + 210));
/*     sha123_debug("gainpow2[%d] = %.3f\n", i + 256, gainpow2[i + 256]); */
  }

  for (i = 0; i < 8207; i++) {
    ispow[i] = pow((double) i, (double) 4.0 / 3.0);
/*     sha123_debug("ispow[%d] = %.3f\n", i, ispow[i]); */
  }

  for (i = 0; i < 8; i++) {
    static double Ci[8] =
      {-0.6, -0.535, -0.33, -0.185, -0.095, -0.041, -0.0142, -0.0037};
    double sq = sqrt(1.0 + Ci[i] * Ci[i]);
    aa_cs[i] = 1.0 / sq;
    aa_ca[i] = Ci[i] / sq;
/*     sha123_debug("aa_cs,aa_ca[%d] = %.3f, %.3f\n", i, aa_cs[i],aa_ca[i]); */
  }

  for (i = 0; i < 18; i++) {
    win[0][i] = win[1][i] =
      0.5 * sin(M_PI / 72.0 * (double) (2 * (i + 0) + 1))
      / cos(M_PI * (double) (2 * (i + 0) + 19) / 72.0);
    win[0][i + 18] = win[3][i + 18] =
      0.5 * sin(M_PI / 72.0 * (double) (2 * (i + 18) + 1))
      / cos(M_PI * (double) (2 * (i + 18) + 19) / 72.0);
  }
  for (i = 0; i < 6; i++) {
    win[1][i + 18] = 0.5 / cos(M_PI * (double) (2 * (i + 18) + 19) / 72.0);
    win[3][i + 12] = 0.5 / cos(M_PI * (double) (2 * (i + 12) + 19) / 72.0);
    win[1][i + 24] = 0.5 * sin(M_PI / 24.0 * (double) (2 * i + 13))
      / cos(M_PI * (double) (2 * (i + 24) + 19) / 72.0);
    win[1][i + 30] = win[3][i] = 0.0;
    win[3][i + 6] = 0.5 * sin(M_PI / 24.0 * (double) (2 * i + 1))
      / cos(M_PI * (double) (2 * (i + 6) + 19) / 72.0);
  }

/*   for (i = 0; i < 9; i++) */
/*     COS9[i] = cos(M_PI / 18.0 * (double) i); */

/*   for (i = 0; i < 9; i++) { */
/*     double tt = 0.5 / cos(M_PI * (double) (i * 2 + 1) / 36.0); */
/*     tfcos36[i] = 0.5 / cos(M_PI * (double) (i * 2 + 1) / 36.0); */
/*   } */
/*   for (i = 0; i < 3; i++) */
/*     tfcos12[i] = 0.5 / cos(M_PI * (double) (i * 2 + 1) / 12.0); */

/*   COS6_1 = cos(M_PI / 6.0 * (double) 1); */
/*   COS6_2 = cos(M_PI / 6.0 * (double) 2); */

/* #ifdef NEW_DCT9 */
/*   cos9[0] = cos(1.0 * M_PI / 9.0); */
/*   cos9[1] = cos(5.0 * M_PI / 9.0); */
/*   cos9[2] = cos(7.0 * M_PI / 9.0); */
/*   cos18[0] = cos(1.0 * M_PI / 18.0); */
/*   cos18[1] = cos(11.0 * M_PI / 18.0); */
/*   cos18[2] = cos(13.0 * M_PI / 18.0); */
/* #endif */

  for (i = 0; i < 12; i++) {
    win[2][i] = 0.5 * sin(M_PI / 24.0 * (double) (2 * i + 1))
      / cos(M_PI * (double) (2 * i + 7) / 24.0);
    for (j = 0; j < 6; j++)
      COS1[i][j] = cos(M_PI / 24.0 * (double) ((2 * i + 7) * (2 * j + 1)));
  }
  
  for (j = 0; j < 4; j++) {
    static int len[4] = { 36, 36, 12, 36 };
    
    for (i = 0; i < len[j]; i += 2)
      win1[j][i] = +win[j][i];
    for (i = 1; i < len[j]; i += 2)
      win1[j][i] = -win[j][i];
  }

  for (i = 0; i < 16; i++) {
    double t;

    switch (i) {
    case 15:
    case 9:
    case 3:
      t = 1;
      tan1_1[i] = 0.5;
      tan2_1[i] = 0.5;
      break;
    case 6:
    case 12:
      t = -1;
      tan1_1[i] = 1;
      tan2_1[i] = -1;
      break;
    default:
      t = tan((double) i * M_PI / 12.0);
      tan1_1[i] = t / (1.0 + t);
      tan2_1[i] = 1.0 / (1.0 + t);
    }
    
    tan1_2[i] = M_SQRT2 * tan1_1[i]; // t / (1.0 + t);
    tan2_2[i] = M_SQRT2 * tan2_1[i]; // / (1.0 + t);

    for (j = 0; j < 2; j++) {
      double base = pow(2.0, -0.25 * (j + 1.0));
      double p1 = 1.0, p2 = 1.0;
      
      if (i > 0) {
	if (i & 1)
	  p1 = pow(base, (i + 1.0) * 0.5);
	else
	  p2 = pow(base, i * 0.5);
      }
      pow1_1[j][i] = p1;
      pow2_1[j][i] = p2;
      pow1_2[j][i] = M_SQRT2 * p1;
      pow2_2[j][i] = M_SQRT2 * p2;
    }
  }

  for (j = 0; j < 9; j++) {
    const struct bandInfoStruct *bi = &bandInfo[j];
    int *mp;
    int cb, lwin;
    const int *bdf;
    
    mp = map[j][0] = mapbuf0[j];
    bdf = bi->longDiff;
    for (i = 0, cb = 0; cb < 8; cb++, i += *bdf++) {
      *mp++ = (*bdf) >> 1;
      *mp++ = i;
      *mp++ = 3;
      *mp++ = cb;
    }
    bdf = bi->shortDiff + 3;
    for (cb = 3; cb < 13; cb++) {
      int l = (*bdf++) >> 1;
      
      for (lwin = 0; lwin < 3; lwin++) {
	*mp++ = l;
	*mp++ = i + lwin;
	*mp++ = lwin;
	*mp++ = cb;
      }
      i += 6 * l;
    }
    mapend[j][0] = mp;
    
    mp = map[j][1] = mapbuf1[j];
    bdf = bi->shortDiff + 0;
    for (i = 0, cb = 0; cb < 13; cb++) {
      int l = (*bdf++) >> 1;
      
      for (lwin = 0; lwin < 3; lwin++) {
	*mp++ = l;
	*mp++ = i + lwin;
	*mp++ = lwin;
	*mp++ = cb;
      }
      i += 6 * l;
    }
    mapend[j][1] = mp;

    mp = map[j][2] = mapbuf2[j];
    bdf = bi->longDiff;
    for (cb = 0; cb < 22; cb++) {
      *mp++ = (*bdf++) >> 1;
      *mp++ = cb;
    }
    mapend[j][2] = mp;
    
  }
  
  for (i = 0; i < 5; i++) {
    for (j = 0; j < 6; j++) {
      for (k = 0; k < 6; k++) {
	int n = k + j * 6 + i * 36;

	i_slen2[n] = i | (j << 3) | (k << 6) | (3 << 12);
      }
    }
  }
  for (i = 0; i < 4; i++) {
    for (j = 0; j < 4; j++) {
      for (k = 0; k < 4; k++) {
	int n = k + j * 4 + i * 16;

	i_slen2[n + 180] = i | (j << 3) | (k << 6) | (4 << 12);
      }
    }
  }
  for (i = 0; i < 4; i++) {
    for (j = 0; j < 3; j++) {
      int n = j + i * 3;

      i_slen2[n + 244] = i | (j << 3) | (5 << 12);
      n_slen2[n + 500] = i | (j << 3) | (2 << 12) | (1 << 15);
    }
  }

  for (i = 0; i < 5; i++) {
    for (j = 0; j < 5; j++) {
      for (k = 0; k < 4; k++) {
	for (l = 0; l < 4; l++) {
	  int n = l + k * 4 + j * 16 + i * 80;

	  n_slen2[n] = i | (j << 3) | (k << 6) | (l << 9) | (0 << 12);
	}
      }
    }
  }
  for (i = 0; i < 5; i++) {
    for (j = 0; j < 5; j++) {
      for (k = 0; k < 4; k++) {
	int n = k + j * 4 + i * 20;

	n_slen2[n + 400] = i | (j << 3) | (k << 6) | (1 << 12);
      }
    }
  }

  sha123_debug("-> layer III initialized\n");
}


/*
 * read additional side information (for MPEG 1 and MPEG 2)
 */
static int III_get_side_info(sha123_t * sha123,
			     struct III_sideinfo *si,
			     int stereo, int ms_stereo,
			     int sfreq, int single, int lsf
			     )
{
  bsi_t * bsi = &sha123->bsi;
  int ch, gr;
  int powdiff = (single == 3) ? 4 : 0;

  typedef struct {
    int granules;
    int reservoir;
    int private[2];
    int scalefac_compress;
  } III_frame_header_t;

  static const III_frame_header_t tabs[2] = {
    /* LSF 0 */
    {2, 9, {5, 3}, 4},
    /* LSF 1 */
    {1, 8, {1, 2}, 9}
  };
  const III_frame_header_t * tab = tabs + lsf;

  if (!sha123->frame.header.not_protected) {
    /* Skip CRC */
    bsi_skipbytes(bsi,2);
  }

  /* Get bit reservoir size : bitstream is aligned, just read 1 byte. */
  ch = bsi_getbyte(bsi);
  if (!lsf) {
    /* Add 9th bit for LSF 0. */
    ch = (ch << 1) | bsi_get1bit(bsi);
  }
  si->main_data_begin = ch;

  /* Get private bits : depends on stereo */
  si->private_bits = bsi_getbits_fast(bsi,tab->private[stereo-1]);

  /* Get scfsi for LSF 0. */
  if (!lsf) {
    si->ch[0].gr[0].scfsi = -1;
    if (stereo == 1) {
      si->ch[0].gr[1].scfsi = bsi_getbits_fast(bsi, 4);
    } else {
      register unsigned int scfsi;
      si->ch[1].gr[0].scfsi = -1;
      scfsi = bsi_getbits_fast(bsi, 8);
      si->ch[1].gr[1].scfsi = scfsi & 0xF;
      si->ch[0].gr[1].scfsi = scfsi >> 4;
    }
  }

  for (gr = 0; gr < tab->granules; gr++) {
    for (ch = 0; ch < stereo; ch++) {
      register struct gr_info_s *gr_info = &(si->ch[ch].gr[gr]);
      register unsigned int v;

      /* Get part2_3_length and big value */
      v = bsi_getbits_long(bsi, 12 + 9);
      gr_info->part2_3_length = v >> 9;
      v &= 0x1ff;
      gr_info->big_values = v;
      if (v > 288) {
	sha123_debug("III : big_value [%d] > 288\n", v);
	sha123_set_error(sha123, "III : big_values > 288");
	return -1;
      }

      /* Get power2gain */
      gr_info->pow2gain =
	gainpow2 + 256 - bsi_getbits_fast(bsi,8) + powdiff + (ms_stereo << 1);

      /* Get scale factor compression */
      gr_info->scalefac_compress = bsi_getbits(bsi,tab->scalefac_compress);

      /* Get window flag and data. */
      v = bsi_getbits_long(bsi, 1 + 22);
      if (v & (1<<22)) {
	/* Window switch 1 */

	/* Get full gain table. */
	gr_info->full_gain[2] = gr_info->pow2gain + ((v&0007) << 3);
	gr_info->full_gain[1] = gr_info->pow2gain + ((v&0070));
	gr_info->full_gain[0] = gr_info->pow2gain + ((v&0700) >> 3);
	v >>= 9;

	/*
	 * table_select[2] not needed, because
	 * there is no region2, but to satisfy
	 * some verifications tools we set it
	 * either.
	 */
	gr_info->table_select[2] = 0;
	gr_info->table_select[1] = v & 0x1f;
	v >>= 5; 
	gr_info->table_select[0] = v & 0x1f;
	v >>= 5; 
	gr_info->mixed_block_flag = v & 1;
	v = (v >> 1) & 3;
	gr_info->block_type = v;
	if (!v) {
	  sha123_debug("III : block type [0] and window flags [1]\n");
	  sha123_set_error(sha123, "III : invalid block-type for window mode");
	  return -1;
	}

	/* region_count/start parameters are implicit in this case. */
	if (v == 2 || !lsf) {
	  gr_info->region1start = 36 >> 1;
	} else {
	  /* check this again for 2.5 and sfreq=8 */
	  gr_info->region1start = 27 << (sfreq == 8);
	}
	gr_info->region2start = 576 >> 1;
      } else {
	/* Window switch 0 */
	int r0c, r1c;
	
	r1c = (v & 7) + 1;
	v >>= 3;
	r0c = (v & 15) + 1;
	v >>= 4;

	gr_info->region1start = bandInfo[sfreq].longIdx[r0c] >> 1;
	r0c += r1c;
	gr_info->region2start =
	  ((r0c > 22) ? 576 : bandInfo[sfreq].longIdx[r0c]) >> 1;

	gr_info->table_select[2] = v & 0x1F; v >>= 5;
	gr_info->table_select[1] = v & 0x1F; v >>= 5;
	gr_info->table_select[0] = v & 0x1F;

	gr_info->block_type = 0;
	gr_info->mixed_block_flag = 0;

      }

      v = bsi_getbits_fast(bsi, 3-lsf);
      gr_info->count1table_select = v & 1;
      v >>= 1;
      gr_info->scalefac_scale = v & 1;
      v >>= 1;
      /* Don't need it for LSF 1 (computed in III_get_scale_factors_2()) */
      gr_info->preflag = v & 1;
    }
  }

#ifdef SHA123_PARANO
  if (bsi->idx != 0 || bsi->ptr - bsi->buffer != sha123->frame.info.ssize) {
    sha123_debug("III side info : bsi : %d != 0, %d != %d \n", bsi->idx,
		 bsi->ptr - bsi->buffer, sha123->frame.info.ssize);
    sha123_set_error(sha123, "III : get side info internal error");
    return -1;
  }
#endif

  return 0;
}

/* Read scalefactors for LSF-0 frame 
 */
static int III_get_scale_factors_1(sha123_t * sha123,
				   int * scf,
				   struct gr_info_s *gr_info)
{
  bsi_t * bsi = &sha123->bsi;
  static const unsigned char slen[2][16] = {
    {0, 0, 0, 0, 3, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4},
    {0, 1, 2, 3, 0, 1, 2, 3, 1, 2, 3, 1, 2, 3, 2, 3}
  };
  int numbits;
  int num0 = slen[0][gr_info->scalefac_compress];
  int num1 = slen[1][gr_info->scalefac_compress];

  if (gr_info->block_type == 2) {
    int n = 18 - gr_info->mixed_block_flag;
    numbits = (num0 + num1) * 18 - (-(n==17) & num0);

    bsi_collectbits_fast(bsi, num0, scf, n);
    scf += n;
    bsi_collectbits_fast(bsi, num1, scf, 18);
    scf += 18;
    *scf++ = 0;
    *scf++ = 0;
    *scf++ = 0;	/* short[13][0..2] = 0 */

  } else {
    int scfsi = gr_info->scfsi;

    if (scfsi < 0) {		/* scfsi < 0 => granule == 0 */
      bsi_collectbits_fast(bsi, num0, scf, 11);
      scf += 11;
      bsi_collectbits_fast(bsi, num1, scf, 10);
      scf += 10;
      numbits = (num0 + num1) * 10 + num0;
      *scf++ = 0;
    } else {
      numbits = 0;
      if (!(scfsi & 0x8)) {
	bsi_collectbits_fast(bsi, num0, scf, 6);
	numbits += num0 * 6;
      }
      scf += 6;

      if (!(scfsi & 0x4)) {
	bsi_collectbits_fast(bsi, num0, scf, 5);
	numbits += num0 * 5;
      }
      scf += 5;

      if (!(scfsi & 0x2)) {
	bsi_collectbits_fast(bsi, num1, scf, 5);
	numbits += num1 * 5;
      }
      scf += 5;

      if (!(scfsi & 0x1)) {
	bsi_collectbits_fast(bsi, num1, scf, 5);
	numbits += num1 * 5;
      }
      scf += 5;

      *scf++ = 0;	/* no l[21] in original sources */
    }
  }
  return numbits;
}

/* Read scalefactors for LSF-1 frame 
 */
static int III_get_scale_factors_2(sha123_t * sha123,
				   int *scf,
				   struct gr_info_s *gr_info, int i_stereo)
{
  bsi_t * bsi = &sha123->bsi;
  unsigned char *pnt;
  int i, n = 0, numbits = 0;
  unsigned int slen;

  static unsigned char stab[3][6][4] =
    {
      {{6, 5, 5, 5}, {6, 5, 7, 3}, {11, 10, 0, 0},
       {7, 7, 7, 0}, {6, 6, 6, 3}, {8, 8, 5, 0}},
      {{9, 9, 9, 9}, {9, 9, 12, 6}, {18, 18, 0, 0},
       {12, 12, 12, 0}, {12, 9, 9, 6}, {15, 12, 9, 0}},
      {{6, 9, 9, 9}, {6, 9, 12, 6}, {15, 18, 0, 0},
       {6, 15, 12, 0}, {6, 12, 9, 6}, {6, 18, 9, 0}}
    };

  /* i_stereo AND second channel -> sha123_do_layer3() checks this */
  if (i_stereo) {
    slen = i_slen2[gr_info->scalefac_compress >> 1];
  } else {
    slen = n_slen2[gr_info->scalefac_compress];
  }
  gr_info->preflag = (slen >> 15) & 0x1;

  n = (gr_info->block_type == 2);
  n += (n & gr_info->mixed_block_flag);

  pnt = stab[n][(slen >> 12) & 0x7];

  for (i = 0; i < 4; i++) {
    int n = (int)pnt[i];
    int num = slen & 0x7;
    slen >>= 3;
    bsi_collectbits_fast(bsi, num, scf, n);
    scf += n;
    numbits += n * num;
  }

  n = (n << 1) + 1;
  for (i = 0; i < n; i++)
    *scf++ = 0;

  return numbits;
}

static int pretab1[22] =
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 3, 3, 3, 2, 0};
static int pretab2[22] =
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

/*
 * Dequantize samples (includes huffman decoding)
 */
/* 24 is enough because tab13 has max. a 19 bit huffvector */
#define BITSHIFT ((sizeof(long) - 1) << 3)
#define SIGNSHIFT ((sizeof(long) << 3) - 1)


#define REFRESH_MASK()							\
while(num < BITSHIFT) {							\
	mask |= ((unsigned long)bsi_getbyte(bsi)) << (BITSHIFT - num);	\
	num += 8;							\
	part2remain -= 8;						\
}

static int III_dequantize_sample(real xr[SBLIMIT][SSLIMIT], int *scf,
				 struct gr_info_s *gr_info, int sfreq, int part2bits, sha123_t * sha123)
{
  bsi_t * bsi = &sha123->bsi;
  int shift = 1 + gr_info->scalefac_scale;
  real *xrpnt = (real *) xr;
  int l[3], l3;
  int part2remain = gr_info->part2_3_length - part2bits;
  int *me;

  int num = bsi_getbitoffset(bsi);
  long mask;
  /* we must split this, because for num==0 the shift is undefined if you do it in one step */
  mask = ((unsigned long) bsi_getbits(bsi,num)) << BITSHIFT;
  mask <<= 8 - num;
  part2remain -= num;

  {
    int bv = gr_info->big_values;
    int region1 = gr_info->region1start;
    int region2 = gr_info->region2start;

    l3 = ((576 >> 1) - bv) >> 1;
    /*
     * we may lose the 'odd' bit here !! 
     * check this later again 
     */
    if (bv <= region1) {
      l[0] = bv;
      l[1] = 0;
      l[2] = 0;
    } else {
      l[0] = region1;
      if (bv <= region2) {
	l[1] = bv - region1;
	l[2] = 0;
      } else {
	l[1] = region2 - region1;
	l[2] = bv - region2;
      }
    }
  }

  if (gr_info->block_type == 2) {
    /*
     * decoding with short or mixed mode BandIndex table 
     */
    int i, max[4];
    int step = 0, lwin = 3, cb = 0;
    register real v = 0.0;
    register int *m, mc;

    {
      int mbf = -gr_info->mixed_block_flag; /* 0/-1 */
      int v = (3 & mbf) - 1; /* -1/2 */

      max[0] = max[1] = max[2] = v;
      max[3] = -1;

      ++mbf; /* 1/0 */
      m = map[sfreq][mbf];
      me = mapend[sfreq][mbf];
    }

#if 0
    if (gr_info->mixed_block_flag) {
      max[3] = -1;
      max[0] = max[1] = max[2] = 2;
      m = map[sfreq][0];
      me = mapend[sfreq][0];
    } else {
      max[0] = max[1] = max[2] = max[3] = -1;
      /* max[3] not really needed in this case */
      m = map[sfreq][1];
      me = mapend[sfreq][1];
    }
#endif

    mc = 0;
    for (i = 0; i < 2; i++) {
      int lp = l[i];
      const struct newhuff *h = ht + gr_info->table_select[i];

      for (; lp; lp--, mc--) {
	register int x, y;
	if ((!mc)) {
	  mc = *m++;
	  xrpnt = ((real *) xr) + (*m++);
	  lwin = *m++;
	  cb = *m++;
	  if (lwin == 3) {
	    v = gr_info->pow2gain[(*scf++) << shift];
	    step = 1;
	  } else {
	    v = gr_info->full_gain[lwin][(*scf++) << shift];
	    step = 3;
	  }
	}

	{
	  const register short *val = h->table;

	  REFRESH_MASK();
	  while ((y = *val++) < 0) {
	    val -= y & ((long)mask >> SIGNSHIFT);
/* 	    if (mask < 0) */
/* 	      val -= y; */
	    num--;
	    mask <<= 1;
	  }
	  x = y >> 4;
	  y &= 0xf;
	}

	{
	  real w;

	  if (!x) {
	    w = 0;
	  } else {
	    max[lwin] = cb;

	    if (x == 15 && h->linbits) {
	      REFRESH_MASK();
	      x += ((unsigned long) mask) >> (BITSHIFT + 8 - h->linbits);
	      num -= h->linbits;
	      mask <<= h->linbits;
	    }
	    --num;
	    w = v * ispow[x];
	    if (mask < 0) w = -w;
	    mask <<= 1;
	  }
	  *xrpnt = w;
	  xrpnt += step;
	}

	{
	  real w;

	  if (!y) {
	    w = 0;
	  } else {
	    max[lwin] = cb;

	    if (y == 15 && h->linbits) {
	      REFRESH_MASK();
	      y += ((unsigned long) mask) >> (BITSHIFT + 8 - h->linbits);
	      num -= h->linbits;
	      mask <<= h->linbits;
	    }
	    --num;
	    w = v * ispow[y];
	    if (mask < 0) w = -w;
	    mask <<= 1;
	  }
	  *xrpnt = w;
	  xrpnt += step;
	}

      }
    }
    
    for (; l3 && (part2remain + num > 0); l3--) {
      const struct newhuff *h = htc + gr_info->count1table_select;
      const register short *val = h->table;
      register short a;
      
      REFRESH_MASK();
      while ((a = *val++) < 0) {
	if (mask < 0)
	  val -= a;
	num--;
	mask <<= 1;
      }
      if (part2remain + num <= 0) {
	num -= part2remain + num;
	break;
      }

      for (i = 0; i < 4; i++) {
	if (!(i & 1)) {
	  if (!mc) {
	    mc = *m++;
	    xrpnt =	((real *) xr) + (*m++);
	    lwin = *m++;
	    cb = *m++;
	    if (lwin == 3) {
	      v = gr_info->pow2gain[(*scf++) << shift];
	      step = 1;
	    } else {
	      v = gr_info->full_gain[lwin][(*scf++) << shift];
	      step = 3;
	    }
	  }
	  mc--;
	}
	if ((a & (0x8 >> i))) {
	  max[lwin] = cb;
	  if (part2remain + num <= 0) {
	    break;
	  }
	  *xrpnt = (mask < 0) ? -v : v;
	  num--;
	  mask <<= 1;
	} else {
	  *xrpnt = 0.0;
	}
	xrpnt += step;
      }
    }

    if (lwin < 3) {
      /* short band? */
      while (1) {
	/* HACK Prevent overflowing the xr buffer */
	if (mc * 6 > &xr[SBLIMIT][SSLIMIT] - xrpnt) {
	  sha123_debug("III : xr buffer overflow (%d > %d)\n",
		       mc * 6, &xr[SBLIMIT][SSLIMIT]-xrpnt);
	  sha123_set_error(sha123, "III : xr buffer overflow");
	  return -1;
	}
					
	for (; mc > 0; mc--) {
	  *xrpnt = 0.0;
	  xrpnt += 3;	/* short band -> step=3 */
	  *xrpnt = 0.0;
	  xrpnt += 3;
	}
	if (m >= me)
	  break;
	mc = *m++;
	xrpnt = ((real *) xr) + *m++;
	if (*m++ == 0)
	  break; /* optimize: field will be set to zero at the end of the function */
	m++;	/* cb */
      }
    }

    gr_info->maxband[0] = max[0] + 1;
    gr_info->maxband[1] = max[1] + 1;
    gr_info->maxband[2] = max[2] + 1;
    gr_info->maxbandl   = max[3] + 1;

    {
      int rmax = max[0] > max[1] ? max[0] : max[1];
      
      rmax = (rmax > max[2] ? rmax : max[2]) + 1;
      gr_info->maxb =
	rmax ? sha123->limits.Short[rmax] : sha123->limits.Long[max[3] + 1];
    }

  } else {
    /*
     * decoding with 'long' BandIndex table (block_type != 2)
     */
    int *pretab = gr_info->preflag ? pretab1 : pretab2;
    int i, max = -1;
    int cb = 0;
    int *m = map[sfreq][2];
    register real v = 0.0;
    int mc = 0;

    /*
     * long hash table values
     */
    for (i = 0; i < 3; i++) {
      int lp = l[i];
      const struct newhuff *h = ht + gr_info->table_select[i];

      for (; lp; lp--, mc--) {
	int x, y;

	if (!mc) {
	  mc = *m++;
	  cb = *m++;
	  /* if (cb == 21) */
	  /*   v = 0.0; */
	  /* else */
	  v = gr_info->pow2gain[((*scf++) + (*pretab++)) << shift];
	  
	}
	{
	  const register short *val = h->table;
	  REFRESH_MASK();
	  while ((y = *val++) < 0) {
	    val -= y & ((long)mask >> SIGNSHIFT);
	    num--;
	    mask <<= 1;
	  }
	  x = y >> 4;
	  y &= 0xf;
	}

	{
	  real w;
	  
	  if (!x) {
	    w = 0;
	  } else {
	    max = cb;
	    if (x == 15 && h->linbits) {
	      REFRESH_MASK();
	      x += ((unsigned long) mask) >> (BITSHIFT + 8 - h->linbits);
	      num -= h->linbits;
	      mask <<= h->linbits;
	    }
	    --num;
	    w = ispow[x] * v;
	    if (mask < 0) w = -w;
	    mask <<= 1;
	  }
	  *xrpnt++ = w;
	}

	{
	  real w;
	  if (!y) {
	    w = 0;
	  } else {
	    max = cb;
	    if (y == 15 && h->linbits) {
	      REFRESH_MASK();
	      y += ((unsigned long) mask) >> (BITSHIFT + 8 - h->linbits);
	      num -= h->linbits;
	      mask <<= h->linbits;
	    }
	    --num;
	    w = ispow[y] * v;
	    if (mask < 0) w = -w;
	    mask <<= 1;
	  }
	  *xrpnt++ = w;
	}
      }
    }

    /*
     * short (count1table) values
     */
    for (; l3 && (part2remain + num > 0); l3--) {
      const struct newhuff *h = htc + gr_info->count1table_select;
      const register short *val = h->table;
      register short a;

      REFRESH_MASK();
      while ((a = *val++) < 0) {
	val -= a & ((long)mask >> SIGNSHIFT);
	num--;
	mask <<= 1;
      }
      if (part2remain + num <= 0) {
	num -= part2remain + num;
	break;
      }

      for (i = 0; i < 4; i++) {
	if (!(i & 1)) {
	  if (!mc) {
	    mc = *m++;
	    cb = *m++;
	    /* if (cb == 21) */
	    /*  v = 0.0; */
	    /* else */
	    v = gr_info->pow2gain[((*scf++) + (*pretab++)) << shift];
	  }
	  mc--;
	}
	if ((a & (0x8 >> i))) {
	  max = cb;
	  if (part2remain + num <= 0) {
	    break;
	  }
	  *xrpnt++ = (mask < 0) ? -v : v;
	  num--;
	  mask <<= 1;
	} else {
	  *xrpnt++ = 0.0;
	}
      }
    }
    gr_info->maxbandl = max + 1;
    gr_info->maxb = sha123->limits.Long[gr_info->maxbandl];
  }

  part2remain += num;
  bsi_backbits(bsi,num);
  num = 0;

  while (xrpnt < &xr[SBLIMIT][0])
    *xrpnt++ = 0.0;

  if (part2remain < 0) {
    sha123_debug("III : III_dequantize_sample part2remain [%d] < 0\n",
		 part2remain);
    sha123_set_error(sha123, "III : negative number of bit to skip");
    return -1;
  }

  /* Dismiss stuffing Bits */
  bsi_skipbits(bsi, part2remain);

  return 0;
}

/* 
 * III_stereo: calculate real channel values for Joint-I-Stereo-mode
 */
static void III_i_stereo(real xr_buf[2][SBLIMIT][SSLIMIT], int *scalefac, struct gr_info_s *gr_info, int sfreq, int ms_stereo, int lsf)
{
  real(*xr)[SBLIMIT * SSLIMIT] = (real(*)[SBLIMIT * SSLIMIT]) xr_buf;
  const struct bandInfoStruct *bi = &bandInfo[sfreq];

  const real *tab1, *tab2;

  int tab;
  static const real *tabs[3][2][2] = {
    {{tan1_1, tan2_1}, {tan1_2, tan2_2}},
    {{pow1_1[0], pow2_1[0]}, {pow1_2[0], pow2_2[0]}},
    {{pow1_1[1], pow2_1[1]}, {pow1_2[1], pow2_2[1]}}
  };

  tab = lsf + (gr_info->scalefac_compress & lsf);
  tab1 = tabs[tab][ms_stereo][0];
  tab2 = tabs[tab][ms_stereo][1];

  if (gr_info->block_type == 2)
    {
      int lwin, do_l = 0;

      if (gr_info->mixed_block_flag)
	do_l = 1;

      for (lwin = 0; lwin < 3; lwin++)
	{	/* process each window */
	  /* get first band with zero values */
	  int is_p, sb, idx, sfb = gr_info->maxband[lwin];	/* sfb is minimal 3 for mixed mode */

	  if (sfb > 3)
	    do_l = 0;

	  for (; sfb < 12; sfb++)
	    {
	      is_p = scalefac[sfb * 3 + lwin - gr_info->mixed_block_flag];	/* scale: 0-15 */
	      if (is_p != 7)
		{
		  real t1, t2;

		  sb = bi->shortDiff[sfb];
		  idx = bi->shortIdx[sfb] + lwin;
		  t1 = tab1[is_p];
		  t2 = tab2[is_p];
		  for (; sb > 0; sb--, idx += 3)
		    {
		      real v = xr[0][idx];

		      xr[0][idx] = v * t1;
		      xr[1][idx] = v * t2;
		    }
		}
	    }

#if 1
	  /* in the original: copy 10 to 11 , here: copy 11 to 12 
	     maybe still wrong??? (copy 12 to 13?) */
	  is_p = scalefac[11 * 3 + lwin - gr_info->mixed_block_flag];	/* scale: 0-15 */
	  sb = bi->shortDiff[12];
	  idx = bi->shortIdx[12] + lwin;
#else
	  is_p = scalefac[10 * 3 + lwin - gr_info->mixed_block_flag];	/* scale: 0-15 */
	  sb = bi->shortDiff[11];
	  idx = bi->shortIdx[11] + lwin;
#endif
	  if (is_p != 7)
	    {
	      real t1, t2;
	      t1 = tab1[is_p];
	      t2 = tab2[is_p];
	      for (; sb > 0; sb--, idx += 3)
		{
		  real v = xr[0][idx];
		  xr[0][idx] = v * t1;
		  xr[1][idx] = v * t2;
		}
	    }
	}		/* end for(lwin; .. ; . ) */

      /* also check l-part, if ALL bands in the three windows are 'empty'
       * and mode = mixed_mode 
       */
      if (do_l)
	{
	  int sfb = gr_info->maxbandl;
	  int idx = bi->longIdx[sfb];

	  for (; sfb < 8; sfb++)
	    {
	      int sb = bi->longDiff[sfb];
	      int is_p = scalefac[sfb];	/* scale: 0-15 */

	      if (is_p != 7)
		{
		  real t1, t2;

		  t1 = tab1[is_p];
		  t2 = tab2[is_p];
		  for (; sb > 0; sb--, idx++)
		    {
		      real v = xr[0][idx];

		      xr[0][idx] = v * t1;
		      xr[1][idx] = v * t2;
		    }
		}
	      else
		idx += sb;
	    }
	}
    }
  else
    {			/* ((gr_info->block_type != 2)) */
      int sfb = gr_info->maxbandl;
      int is_p, idx = bi->longIdx[sfb];

      /* hmm ... maybe the maxbandl stuff for i-stereo is buggy? */
      if (sfb <= 21)
	{
	  for (; sfb < 21; sfb++)
	    {
	      int sb = bi->longDiff[sfb];

	      is_p = scalefac[sfb];	/* scale: 0-15 */
	      if (is_p != 7)
		{
		  real t1, t2;
		  t1 = tab1[is_p];
		  t2 = tab2[is_p];
		  for (; sb > 0; sb--, idx++)
		    {
		      real v = xr[0][idx];
		      xr[0][idx] = v * t1;
		      xr[1][idx] = v * t2;
		    }
		}
	      else
		idx += sb;
	    }

	  is_p = scalefac[20];
	  if (is_p != 7)
	    {		/* copy l-band 20 to l-band 21 */
	      int sb;
	      real t1 = tab1[is_p], t2 = tab2[is_p];

	      for (sb = bi->longDiff[21]; sb > 0; sb--, idx++)
		{
		  real v = xr[0][idx];

		  xr[0][idx] = v * t1;
		  xr[1][idx] = v * t2;
		}
	    }
	}
    }		/* ... */
}

static void III_antialias(real xr[SBLIMIT][SSLIMIT], struct gr_info_s *gr_info)
{
  int sblim;

  if (gr_info->block_type == 2)
    {
      if (!gr_info->mixed_block_flag)
	return;
      sblim = 1;
    }
  else
    {
      sblim = gr_info->maxb - 1;
    }

  /* 31 alias-reduction operations between each pair of sub-bands */
  /* with 8 butterflies between each pair                         */

  {
    int sb;
    real *xr1 = (real *) xr[1];

    if (sblim < 1 || sblim > SBLIMIT)
      return;
		
    for (sb = sblim; sb; sb--, xr1 += 10)
      {
	int ss;
	real *cs = aa_cs, *ca = aa_ca;
	real *xr2 = xr1;

	for (ss = 7; ss >= 0; ss--)
	  {	/* upper and lower butterfly inputs */
	    register real bu = *--xr2, bd = *xr1;

	    *xr2 = (bu * (*cs)) - (bd * (*ca));
	    *xr1++ = (bd * (*cs++)) + (bu * (*ca++));
	  }
      }
  }
}


/*
 * III_hybrid
 */
static void III_hybrid(real fsIn[SBLIMIT][SSLIMIT],
		       real tsOut[SSLIMIT][SBLIMIT], int ch,
		       struct gr_info_s *gr_info, sha123_t *sha123)
{
  static real block[2][2][SBLIMIT * SSLIMIT] = { {{0,}} };
  static int blc[2] = { 0, 0 };

  real *tspnt = (real *) tsOut;
  real *rawout1, *rawout2;
  int bt, sb = 0;

  {
    int b = blc[ch];
    rawout1 = block[b][ch];
    b = -b + 1;
    rawout2 = block[b][ch];
    blc[ch] = b;
  }

  if (gr_info->mixed_block_flag) {
    sb = 2;
    DCT36(fsIn[0],rawout1,rawout2,win[0],tspnt);
    DCT36(fsIn[1],rawout1+18,rawout2+18,win1[0],tspnt+1);
    rawout1 += 36;
    rawout2 += 36;
    tspnt += 2;
  }

  bt = gr_info->block_type;
  if (bt == 2) {
    for (; sb < gr_info->maxb; sb += 2, tspnt += 2, rawout1 += 36, rawout2 += 36) {
      sha123_dct12(fsIn[sb], rawout1, rawout2, win[2], tspnt);
      sha123_dct12(fsIn[sb + 1], rawout1 + 18, rawout2 + 18, win1[2], tspnt + 1);
    }
  } else {
    for (; sb < gr_info->maxb; sb += 2, tspnt += 2, rawout1 += 36, rawout2 += 36)
	{
	  DCT36(fsIn[sb], rawout1, rawout2, win[bt], tspnt);
	  DCT36(fsIn[sb+1], rawout1+18, rawout2+18, win1[bt], tspnt+1);
	}
  }

  for (; sb < SBLIMIT; sb++, tspnt++) {
    int i;
    for (i = 0; i < SSLIMIT; i++) {
      tspnt[i * SBLIMIT] = *rawout1++;
      *rawout2++ = 0.0;
    }
  }
}

int sha123_do_layer3(sha123_t * sha123)
{
  sha123_frame_t * fr = &sha123->frame;
  int gr, ch, ss;
  int scalefacs[2][39];	/* max 39 for short[13][3] mode, mixed: 38, long: 22 */
  struct III_sideinfo sideinfo;
  int stereo = 1 << fr->info.log2chan;
  int single = -1;
  int ms_stereo, i_stereo;
  int sfreq = fr->info.sampling_idx;
  int stereo1, granules;

  sha123_init_layer3_frame(sha123,sfreq,SBLIMIT);

  bsi_set(&sha123->bsi, fr->sideinfo, fr->info.ssize);

  if (stereo == 1) {			
      /* stream is mono */
      stereo1 = 1;
      single = 0;
  } else if (single >= 0) {
    /* stream is stereo, but force to mono */
    stereo1 = 1;
  } else {
    stereo1 = 2;
  }

  if (fr->header.mode == MPG_MD_JOINT_STEREO) {
    ms_stereo = (fr->header.mode_ext & 0x2) >> 1;
    i_stereo = fr->header.mode_ext & 0x1;
  } else {
    ms_stereo = i_stereo = 0;
  }

  granules = fr->info.lsf ? 1 : 2;
  
  if (III_get_side_info(sha123,
			&sideinfo,
			stereo, ms_stereo,
			sfreq,single, fr->info.lsf)) {
    return -1;
  }

  bsi_set(&sha123->bsi,
	  fr->buffer.data - sideinfo.main_data_begin,
	  fr->buffer.size + sideinfo.main_data_begin);

  for (gr = 0; gr < granules; gr++) {
    real hybridIn[2][SBLIMIT][SSLIMIT];
    real hybridOut[2][SSLIMIT][SBLIMIT];

    {
      struct gr_info_s *gr_info = &(sideinfo.ch[0].gr[gr]);
      long part2bits;

      part2bits = (fr->info.lsf)
	? III_get_scale_factors_2(sha123, scalefacs[0], gr_info, 0)
	: III_get_scale_factors_1(sha123, scalefacs[0], gr_info);

      if (III_dequantize_sample(hybridIn[0], scalefacs[0], gr_info, sfreq, part2bits, sha123))
	return -1;
    }

    if (stereo == 2) {
      struct gr_info_s *gr_info = &(sideinfo.ch[1].gr[gr]);
      long part2bits;

      part2bits = (fr->info.lsf)
	? III_get_scale_factors_2(sha123, scalefacs[1], gr_info, i_stereo)
	: III_get_scale_factors_1(sha123, scalefacs[1], gr_info);

      if (III_dequantize_sample(hybridIn[1], scalefacs[1], gr_info, sfreq, part2bits, sha123))
	return -1;

      if (ms_stereo) {
	int i;
	int maxb = sideinfo.ch[0].gr[gr].maxb;

	if (sideinfo.ch[1].gr[gr].maxb > maxb) {
	  maxb = sideinfo.ch[1].gr[gr].maxb;
	}

	for (i = 0; i < SSLIMIT * maxb; i++) {
	  real tmp0 = ((real *) hybridIn[0])[i];
	  real tmp1 = ((real *) hybridIn[1])[i];
	  ((real *) hybridIn[0])[i] = tmp0 + tmp1;
	  ((real *) hybridIn[1])[i] = tmp0 - tmp1;
	}
      }

      if (i_stereo)
	III_i_stereo(hybridIn, scalefacs[1], gr_info, sfreq, ms_stereo, fr->info.lsf);


      if (ms_stereo || i_stereo || (single == 3)) {
	if (gr_info->maxb > sideinfo.ch[0].gr[gr].maxb)
	  sideinfo.ch[0].gr[gr].maxb = gr_info->maxb;
	else
	  gr_info->maxb = sideinfo.ch[0].gr[gr].maxb;
      }

      switch (single) {
      case 3: {
	register int i;
	register real *in0 = (real *) hybridIn[0],
	  *in1 = (real *) hybridIn[1];
	for (i = 0; i < SSLIMIT * gr_info->maxb; i++, in0++)
	  *in0 = (*in0 + *in1++);	/* *0.5 done by pow-scale */
      } break;
      case 1: {
	register int i;
	register real *in0 = (real *) hybridIn[0],
	  *in1 = (real *) hybridIn[1];
	for (i = 0; i < SSLIMIT * gr_info->maxb; i++)
	  *in0++ = *in1++;
      } break;
      }
    }
    /*       if (sha123_info->eq_active) */
    /* 	{ */
    /* 	  int i, sb; */

    /* 	  if (single < 0) */
    /* 	    { */
    /* 	      for (sb = 0, i = 0; sb < SBLIMIT; sb++) */
    /* 		{ */
    /* 		  for (ss = 0; ss < SSLIMIT; ss++) */
    /* 		    { */
    /* 		      hybridIn[0][sb][ss] *= sha123_info->eq_mul[i]; */
    /* 		      hybridIn[1][sb][ss] *= sha123_info->eq_mul[i++]; */
    /* 		    } */
    /* 		} */
    /* 	    } */
    /* 	  else */
    /* 	    { */
    /* 	      for (sb = 0, i = 0; sb < SBLIMIT; sb++) */
    /* 		{ */
    /* 		  for (ss = 0; ss < SSLIMIT; ss++) */
    /* 		    hybridIn[0][sb][ss] *= sha123_info->eq_mul[i++]; */
    /* 		} */
    /* 	    } */
    /* 	} */

#ifdef USE_SIMD
    if (fr->synth_type == SYNTH_MMX && single < 0) {
      int i, sb;
      for (sb = 0, i = 0; sb < SBLIMIT; sb++) {
	for (ss = 0; ss < SSLIMIT; ss++) {
	  hybridIn[0][sb][ss] *= 16384.0;
	  hybridIn[1][sb][ss] *= 16384.0;
	}
      }
    }
#endif
    for (ch = 0; ch < stereo1; ch++) {
      struct gr_info_s *gr_info = &(sideinfo.ch[ch].gr[gr]);
      III_antialias(hybridIn[ch], gr_info);
      if (gr_info->maxb < 1 || gr_info->maxb > SBLIMIT) {
	sha123_debug("III : max band 1 < %d < %d\n", gr_info->maxb, SBLIMIT);
	sha123_set_error(sha123, "III : max band out of range");
	return -1;
      }
      III_hybrid(hybridIn[ch], hybridOut[ch], ch, gr_info, sha123);
    }

    for (ss = 0; ss < SSLIMIT; ss++) {
      if (single >= 0) {
	fr->synth_mn(hybridOut[0][ss],
		     sha123->pcm_sample, &sha123->pcm_point);
      } else {
	int p1 = sha123->pcm_point;
	fr->synth_st(hybridOut[0][ss], 0, sha123->pcm_sample, &p1);
	fr->synth_st(hybridOut[1][ss], 1, sha123->pcm_sample, &sha123->pcm_point);
      }
    }
  }
  
  return 0;
}
