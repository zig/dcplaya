/* 2002/02/12 */

#ifndef _MATRIX_H_
#define _MATRIX_H_

#ifndef __MATRIX_H
typedef float matrix_t[4][4];
#endif

void MtxCopy(matrix_t m, matrix_t m2);
void MtxIdentity(matrix_t m);
void MtxMult(matrix_t m, matrix_t m2);
void MtxVectMult(float *v, const float *u, matrix_t m);
void MtxTranspose(matrix_t m);
void MtxScale(matrix_t m, const float s);
void MtxRotateX(matrix_t m, const float a);
void MtxRotateY(matrix_t m, const float a);
void MtxRotateZ(matrix_t m, const float a);
void MtxLookAt(matrix_t row, const float x, const float y, const float z);
void MtxProjection(matrix_t row, const float openAngle, const float fov,
                   const float aspectRatio, const float zFar);
void MtxFrustum(matrix_t row, const float left, const float right,
                              const float top, const float bottom,
                              const float zNear, const float zFar);
void CrossProduct(float *d, const float *v, const float * w);

#endif /* #ifndef _MATRIX_H_ */
