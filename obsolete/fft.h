/***********************************************************************\
*	fft.h
*   Fast Fourier Transform C library - header file
*   Based on RECrandell's
*   dpwe 22jan90
*   08apr90 With experimental FFT2torl - converse of FFT2real
\***********************************************************************/

/*
    To call an FFT, one must first assign the complex exponential factors:
    A call such as
    e = AssignBasis(NULL, size)
    will set up the complex *e to be the array of cos, sin pairs corresponding
    to total number of complex data = size.   This call allocates the
    (cos, sin) array memory for you.  If you already have such memory
    allocated, pass the allocated pointer instead of NULL.
*/

#ifdef __cplusplus
extern "C"
{
#endif

/*** how can it be so hard to get PI *?***/
#ifndef PI
#ifdef M_PI
#define PI M_PI
#else
#define PI 3.1415926535
#endif
#endif

/*** yukky ansi bodge ***/
#ifndef FLOATARG
#ifdef __STDC__
#define FLOATARG double		/* some prototype checkers get funny about promotion */
#else
#define FLOATARG double
#endif				/* def __STDC__ */
#endif				/* def FLOATARG */

  typedef struct
  {
    float re, im;
  }
  complex;

  typedef struct lnode
  {
    struct lnode *next;
    long size;
    complex *table;
  }
  LNODE;

  int scancomplexdata (complex *);
  void putcomplexdata (complex *, long);
  void ShowCpx (complex *, long, char *);
  int PureReal (complex *, long);
  int IsPowerOfTwo (long);
  complex *FindTable (long);	/* search our list of existing LUT's */
  complex *AssignBasis (complex *, long);
  void reverseDig (complex *, long, int);
  void reverseDigpacked (complex *, long);
  void FFT2dimensional (complex *, long, long, complex *);
  void FFT2torl (complex *, long, int, float, complex *);
  void FFT2torlpacked (complex *, long, float, complex *);
  void ConjScale (complex *, long, float);
  void FFT2real (complex *, long, int, complex *);
  void FFT2realpacked (complex *, long, complex *);
  void Reals (complex *, long, int, int, complex *);
  void Realspacked (complex *, long, int, complex *);
  void FFT2 (complex *, long, int, complex *);
  void FFT2raw (complex *, long, int, int, complex *);
  void FFT2rawpacked (complex *, long, int, complex *);
  void FFTarb (complex *, complex *, long, complex *);
  void DFT (complex *, complex *, long, complex *);
  void cxmult (complex *, complex *, long);

#ifdef __cplusplus
}
#endif
