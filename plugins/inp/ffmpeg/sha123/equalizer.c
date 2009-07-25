#include <math.h>
#include "sha123/equalizer.h"

static int init_spline(real *x, real *y, unsigned int n, real *y2)
{
  int i, k;
  real p, qn, sig, un, *u = 0;
  real tmp[64];

  if (n > sizeof(tmp)/sizeof(*tmp)) {
    return -1;
  }

  y2[0] = u[0] = 0.0;
  for (i = 1; i < n - 1; i++) {
    sig = ((real) x[i] - x[i - 1]) / ((real) x[i + 1] - x[i - 1]);
    p = sig * y2[i - 1] + 2.0;
    y2[i] = (sig - 1.0) / p;
    u[i] = (((real) y[i + 1] - y[i]) / (x[i + 1] - x[i])) -
      (((real) y[i] - y[i - 1]) / (x[i] - x[i - 1]));
    u[i] = (6.0 * u[i] / (x[i + 1] - x[i - 1]) - sig * u[i - 1]) / p;
  }
  qn = un = 0.0;

  y2[n - 1] = (un - qn * u[n - 2]) / (qn * y2[n - 2] + 1.0);
  for (k = n - 2; k >= 0; k--) {
    y2[k] = y2[k] * y2[k + 1] + u[k];
  }
  return 0;
}

static real eval_spline(real xa[], real ya[], real y2a[], int n, real x)
{
  int klo, khi, k;
  real h, b, a;

  klo = 0;
  khi = n - 1;
  while (khi - klo > 1) {
    k = (khi + klo) >> 1;
    if (xa[k] > x)
      khi = k;
    else
      klo = k;
  }
  h = xa[khi] - xa[klo];
  a = (xa[khi] - x) / h;
  b = (x - xa[klo]) / h;
  return (a * ya[klo] + b * ya[khi] + ((a * a * a - a) * y2a[klo] +
				       (b * b * b - b) * y2a[khi])
	  * (h * h) / 6.0);
}

int sha123_set_eq(sha123_equalizer_t * equa, int on, real preamp, real *b)
{
  real x[] =
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}, yf[10], val, band[10];
  int bands[] =
    {0, 4, 8, 16, 26, 78, 157, 313, 366, 418};
  int i, j;

  equa->active = on;
  if (equa->active) {
    for (i = 0; i < 10; i++) {
      band[i] = b[i] + preamp;
    }
    if (init_spline(x, band, 10, yf)) {
      equa->active = 0;
      return -1;
    }
      
    for (i = 0; i < 9; i++) {
      for (j = bands[i]; j < bands[i + 1]; j++) {
	val = eval_spline(x, band, yf, 10, i +
			  ((real) (j - bands[i])
			   * (1.0 / (bands[i + 1] - bands[i]))));
	equa->mul[j] = pow(2, val / 10.0);
      }
    }
    for (i = bands[9]; i < 576; i++) {
      equa->mul[i] = equa->mul[bands[9] - 1];
    }
  }
  return 0;
}
