/* 2002/02/12 */

#include <dc/fmath.h>
#include "matrix.h"

#define Cos fcos
#define Sin fsin
#define Inv(a) (1.0f/(a))

void MtxCopy(matrix_t m, matrix_t m2)
{
  int i;
  for (i=0; i<4; ++i) {
    m[i][0] = m2[i][0];
    m[i][1] = m2[i][1];
    m[i][2] = m2[i][2];
    m[i][3] = m2[i][3];
  }
}

void MtxCopyFlexible(float *d, const float *s, int nline, int ncol,
					 int dsize, int ssize)
{
  if (nline > 0 && ncol > 0) {
	if ( ncol*sizeof(float) == dsize && dsize == ssize) {
	  unsigned int words = (unsigned int)dsize / sizeof(float);
	  do {
		*d++ = *s++;
	  } while(--words);
	}

	do  {
	  int i;
	  for (i=0; i<ncol; ++i) {
		d[i] = s[i];
	  }
	  d = (float *) ((char *)d + dsize);
	  s = (const float *) ((const char *)s + ssize);
	} while (--nline);
  }
}

void MtxIdentity(matrix_t m)
{
  int i,j;
  for (i=0; i<4; ++i) for (j=0; j<4; ++j) m[i][j] = (float)(i==j);
}

void MtxMult(matrix_t m, matrix_t m2)
{
  int i,k;
  for (i=0; i<4; ++i) {
    const float x = m[i][0];
    const float y = m[i][1];
    const float z = m[i][2];
    const float w = m[i][3];
    for (k=0; k<4; ++k) {
      m[i][k] = x * m2[0][k] + y * m2[1][k] + z * m2[2][k] + w * m2[3][k];
    }
  }
}

void MtxMult3(matrix_t d, matrix_t m, matrix_t m2)
{
  int i,k;
  for (i=0; i<4; ++i) {
    const float x = m[i][0];
    const float y = m[i][1];
    const float z = m[i][2];
    const float w = m[i][3];
    for (k=0; k<4; ++k) {
      d[i][k] = x * m2[0][k] + y * m2[1][k] + z * m2[2][k] + w * m2[3][k];
    }
  }
}

void MtxVectMult(float *v, const float *u, matrix_t m)
{
  const float x=u[0],y=u[1],z=u[2],w=u[3];
  v[0] = x * m[0][0] + y * m[1][0] + z * m[2][0] + w * m[3][0];
  v[1] = x * m[0][1] + y * m[1][1] + z * m[2][1] + w * m[3][1];
  v[2] = x * m[0][2] + y * m[1][2] + z * m[2][2] + w * m[3][2];
  v[3] = x * m[0][3] + y * m[1][3] + z * m[2][3] + w * m[3][3];
}

void MtxVectorsMult(float *v, const float *u, matrix_t m, int nmemb,
					int sizev, int sizeu)
{
  while (nmemb--) {
	const float x=u[0],y=u[1],z=u[2],w=u[3];
	v[0] = x * m[0][0] + y * m[1][0] + z * m[2][0] + w * m[3][0];
	v[1] = x * m[0][1] + y * m[1][1] + z * m[2][1] + w * m[3][1];
	v[2] = x * m[0][2] + y * m[1][2] + z * m[2][2] + w * m[3][2];
	v[3] = x * m[0][3] + y * m[1][3] + z * m[2][3] + w * m[3][3];

	v = (float *)((char *)v + sizev);
	u = (const float *)((const char *)u + sizeu);
  }
}


void MtxScale(matrix_t m, const float s)
{
  int i,j;
  matrix_t sm;
  for (i=0; i<4; ++i) for (j=0; j<4; ++j) sm[i][j] = (i==j) ? s : 0;
  MtxMult(m,sm);
}

void MtxTranspose(matrix_t m)
{
  int i, j;
  for (i=0; i<4; ++i) {
    for (j=i+1;j<4; ++j) {
      float tmp = m[i][j];
      m[i][j] = m[j][i];
      m[j][i] = tmp;
    }
  }
}

void MtxRotateX(matrix_t m, const float a)
{
  const float ca = Cos(a);
  const float sa = Sin(a);
  matrix_t m2;
  MtxIdentity(m2);
  m2[1][1] = m2[2][2] = ca;
  m2[1][2] = sa;
  m2[2][1] = -sa;
  MtxMult(m,m2);
}

void MtxRotateY(matrix_t m, const float a)
{
  const float ca = Cos(a);
  const float sa = Sin(a);
  matrix_t m2;
  MtxIdentity(m2);
  m2[0][0] = m2[2][2] = ca;
  m2[2][0] = sa;
  m2[0][2] = -sa;
  MtxMult(m,m2);
}

void MtxRotateZ(matrix_t m, const float a)
{
  const float ca = Cos(a);
  const float sa = Sin(a);
  matrix_t m2;
  MtxIdentity(m2);
  m2[0][0] = m2[1][1] = ca;
  m2[0][1] = sa;
  m2[1][0] = -sa;
  MtxMult(m,m2);
}

static int IsNearZero(const float v)
{
  const float e = 1E-6f;
  return (v > -e && v < e);
}

void MtxLookAt(matrix_t row, const float x, const float y, const float z)
{
  if(IsNearZero(x) && IsNearZero(y) && IsNearZero(z)) {
    MtxIdentity(row);
    return;
  } else {
    const float x2 = x*x;
    const float y2 = y*y;
    const float z2 = z*z;
    const float nxz = fsqrt(x2+z2);
    const float oonxz = 1.0f / nxz;
    const float oonxyz= frsqrt(x2+y2+z2);
    {
      const float tmp = oonxz*oonxyz;
      row[0][1] = -y*x*tmp;
      row[1][1] = nxz*oonxyz;
      row[2][1] = -y*z*tmp;
      row[3][1] = 0;
    }
    
    row[0][2] = x * oonxyz;
    row[1][2] = y * oonxyz;
    row[2][2] = z * oonxyz;
    row[3][2] = 0;
    
    {
      float tmp = (nxz + y2*oonxz) * oonxyz;
      row[0][0] =  row[2][2] * tmp;
      row[2][0] = -row[0][2] * tmp;
      row[1][0] =
	row[3][0] = 0;
    }
    
    row[0][3] =
      row[1][3] =
      row[2][3] = 0;
    row[3][3] = 1;
  }
}


static void SetProjection(matrix_t row, const float w, const float h,
						  const float q, const float zNear)
{
  row[0][0] = w;
  row[1][1] = h;
  row[2][2] = q;
  row[3][2] = -q * zNear;
  row[2][3] = 1.0f;
  row[0][1] = row[0][2] = row[0][3] =
    row[1][0] = row[1][2] = row[1][3] =
    row[2][0] = row[2][1] =
    row[3][0] = row[3][1] = row[3][3] = 0.0f;
}

void MtxProjection(matrix_t row, const float openAngle, const float fov,
				   const float aspectRatio, const float zFar)
{
  const float halfFov = 0.5f * fov;
  const float halfAngle = 0.5f * openAngle;
  const float cosOverSin = Cos(halfAngle) / Sin(halfAngle);
  const float zNear = halfFov * cosOverSin;
  const float w = cosOverSin;
  const float h = w * aspectRatio;
  const float q = zFar / (zFar - zNear);

  SetProjection(row, w, h, q, zNear);
}

void MtxProjection2(matrix_t row, const float openAngle,
					const float aspectRatio,
					const float zNear, const float zFar)
{
  const float halfAngle = 0.5f * openAngle;
  const float cosOverSin = Cos(halfAngle) / Sin(halfAngle);
  const float w = cosOverSin;
  const float h = w * aspectRatio;
  const float q = zFar / (zFar - zNear);

/*   row[0][0] = w; */
/*   row[1][1] = h; */
/*   row[2][2] = (zFar+zNear)/(zNear-zFar); */
/*   row[3][2] = 2*zFar*zNear/(zNear-zFar); */
/*   row[2][3] = -1.0f; */
/*   row[3][3] = 1.0f; */
/*   row[0][1] = row[0][2] = row[0][3] = */
/*     row[1][0] = row[1][2] = row[1][3] = */
/*     row[2][0] = row[2][1] = */
/*     row[3][0] = row[3][1] = 0.0f; */

  SetProjection(row, w, h, q, zNear);
}

void MtxFrustum(matrix_t row, const float left, const float right,
		const float top, const float bottom,
		const float zNear, const float zFar)
{

  const float twoNear = 2.0f * zNear;
  const float ooRightMinusLeft = Inv(right - left);
  const float ooTopMinusBottom = Inv(top - bottom);
  const float ooFarMinusNear   = Inv(zFar - zNear);

  row[0][0] = twoNear * ooRightMinusLeft;
  row[1][1] = twoNear * ooTopMinusBottom;
  row[3][2] = twoNear * zFar * ooFarMinusNear;

  row[2][0] = (right + left) * ooRightMinusLeft;
  row[2][1] = (top + bottom) * ooTopMinusBottom;
  row[2][2] = (zFar + zNear) * ooFarMinusNear;

  row[0][1] = row[0][2] = row[0][3] =
    row[1][0] = row[1][2] = row[1][3] =
    row[3][0] = row[3][1] = row[3][3] = 0.0f;
  row[2][3] = -1.0f;
}


void CrossProduct(float *d, const float *v, const float * w)
{
  d[0] = v[1] * w[2] - v[2] * w[1];
  d[1] = v[2] * w[0] - v[0] * w[2];
  d[2] = v[0] * w[1] - v[1] * w[0];
}
