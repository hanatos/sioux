#pragma once

#include <string.h>

static inline void mat4_set_zero(float *m)
{
  memset(m, 0, sizeof(float)*16);
}

static inline void mat4_set_identity(float *m)
{
  mat4_set_zero(m);
  m[0] = m[5] = m[10] = m[15] = 1.0f;
}

static inline void mat4_add(const float *a, const float *b, float *res)
{
  for(int k=0;k<16;k++) res[k] = a[k] + b[k];
}

static inline void mat4_sub(const float *a, const float *b, float *res)
{
  for(int k=0;k<16;k++) res[k] = a[k] - b[k];
}

static inline void mat4_mul(const float *a, const float *b, float *res)
{
  mat4_set_zero(res);
  for(int j=0;j<4;j++)
    for(int i=0;i<4;i++)
      for(int k=0;k<4;k++)
        res[i+4*j] += a[4*j+k] * b[4*k+i];
}

static inline void mat4_mulv(const float *a, const float *v, float *res)
{
  res[0] = res[1] = res[2] = res[3] = 0.0f;
  for(int j=0;j<4;j++)
    for(int i=0;i<4;i++)
        res[j] += a[4*j+i] * v[i];
}

// multiply the transpose of the matrix to the vector
static inline void mat4_tmulv(const float *a, const float *v, float *res)
{
  res[0] = res[1] = res[2] = res[3] = 0.0f;
  for(int j=0;j<4;j++)
    for(int i=0;i<4;i++)
        res[j] += a[4*i+j] * v[i];
}

static inline void mat4_transpose(const float *m, float *res)
{
  for(int i=0;i<4;i++) for(int k=0;k<4;k++) res[4*k+i] = m[4*i+k];
}

static inline float mat4_inv(const float *m, float *inv_out)
{
  float inv[16];

  inv[0] = m[5]  * m[10] * m[15] -
    m[5]  * m[11] * m[14] -
    m[9]  * m[6]  * m[15] +
    m[9]  * m[7]  * m[14] +
    m[13] * m[6]  * m[11] -
    m[13] * m[7]  * m[10];

  inv[4] = -m[4]  * m[10] * m[15] +
    m[4]  * m[11] * m[14] +
    m[8]  * m[6]  * m[15] -
    m[8]  * m[7]  * m[14] -
    m[12] * m[6]  * m[11] +
    m[12] * m[7]  * m[10];

  inv[8] = m[4]  * m[9] * m[15] -
    m[4]  * m[11] * m[13] -
    m[8]  * m[5] * m[15] +
    m[8]  * m[7] * m[13] +
    m[12] * m[5] * m[11] -
    m[12] * m[7] * m[9];

  inv[12] = -m[4]  * m[9] * m[14] +
    m[4]  * m[10] * m[13] +
    m[8]  * m[5] * m[14] -
    m[8]  * m[6] * m[13] -
    m[12] * m[5] * m[10] +
    m[12] * m[6] * m[9];
  inv[1] = -m[1]  * m[10] * m[15] +
    m[1]  * m[11] * m[14] +
    m[9]  * m[2] * m[15] -
    m[9]  * m[3] * m[14] -
    m[13] * m[2] * m[11] +
    m[13] * m[3] * m[10];

  inv[5] = m[0]  * m[10] * m[15] -
    m[0]  * m[11] * m[14] -
    m[8]  * m[2] * m[15] +
    m[8]  * m[3] * m[14] +
    m[12] * m[2] * m[11] -
    m[12] * m[3] * m[10];

  inv[9] = -m[0]  * m[9] * m[15] +
    m[0]  * m[11] * m[13] +
    m[8]  * m[1] * m[15] -
    m[8]  * m[3] * m[13] -
    m[12] * m[1] * m[11] +
    m[12] * m[3] * m[9];

  inv[13] = m[0]  * m[9] * m[14] -
    m[0]  * m[10] * m[13] -
    m[8]  * m[1] * m[14] +
    m[8]  * m[2] * m[13] +
    m[12] * m[1] * m[10] -
    m[12] * m[2] * m[9];

  inv[2] = m[1]  * m[6] * m[15] -
    m[1]  * m[7] * m[14] -
    m[5]  * m[2] * m[15] +
    m[5]  * m[3] * m[14] +
    m[13] * m[2] * m[7] -
    m[13] * m[3] * m[6];

  inv[6] = -m[0]  * m[6] * m[15] +
    m[0]  * m[7] * m[14] +
    m[4]  * m[2] * m[15] -
    m[4]  * m[3] * m[14] -
    m[12] * m[2] * m[7] +
    m[12] * m[3] * m[6];

  inv[10] = m[0]  * m[5] * m[15] -
    m[0]  * m[7] * m[13] -
    m[4]  * m[1] * m[15] +
    m[4]  * m[3] * m[13] +
    m[12] * m[1] * m[7] -
    m[12] * m[3] * m[5];

  inv[14] = -m[0]  * m[5] * m[14] +
    m[0]  * m[6] * m[13] +
    m[4]  * m[1] * m[14] -
    m[4]  * m[2] * m[13] -
    m[12] * m[1] * m[6] +
    m[12] * m[2] * m[5];

  inv[3] = -m[1] * m[6] * m[11] +
    m[1] * m[7] * m[10] +
    m[5] * m[2] * m[11] -
    m[5] * m[3] * m[10] -
    m[9] * m[2] * m[7] +
    m[9] * m[3] * m[6];

  inv[7] = m[0] * m[6] * m[11] -
    m[0] * m[7] * m[10] -
    m[4] * m[2] * m[11] +
    m[4] * m[3] * m[10] +
    m[8] * m[2] * m[7] -
    m[8] * m[3] * m[6];

  inv[11] = -m[0] * m[5] * m[11] +
    m[0] * m[7] * m[9] +
    m[4] * m[1] * m[11] -
    m[4] * m[3] * m[9] -
    m[8] * m[1] * m[7] +
    m[8] * m[3] * m[5];

  inv[15] = m[0] * m[5] * m[10] -
    m[0] * m[6] * m[9] -
    m[4] * m[1] * m[10] +
    m[4] * m[2] * m[9] +
    m[8] * m[1] * m[6] -
    m[8] * m[2] * m[5];

  float det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];

  if (det == 0)
    return 0;

  det = 1.0 / det;

  for (int i = 0; i < 16; i++)
    inv_out[i] = inv[i] * det;

  return det;
}

static inline void mat4_from_mat3(
    float *m, const float *mat3, const float *trans)
{
  m[ 0] = mat3[0]; m[ 1] = mat3[1]; m[ 2] = mat3[2]; m[ 3] = trans ? trans[0] : 0.0f;
  m[ 4] = mat3[3]; m[ 5] = mat3[4]; m[ 6] = mat3[5]; m[ 7] = trans ? trans[1] : 0.0f;
  m[ 8] = mat3[6]; m[ 9] = mat3[7]; m[10] = mat3[8]; m[11] = trans ? trans[2] : 0.0f;
  m[12] =       0; m[13] =       0; m[14] =       0; m[15] = 1.0f;
}

static inline void mat4_print(
    const float *m)
{
  fprintf(stdout, "%g %g %g %g\n", m[0], m[1], m[2], m[3]);
  fprintf(stdout, "%g %g %g %g\n", m[4], m[5], m[6], m[7]);
  fprintf(stdout, "%g %g %g %g\n", m[8], m[9], m[10], m[11]);
  fprintf(stdout, "%g %g %g %g\n", m[12], m[13], m[14], m[15]);
}
