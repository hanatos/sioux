#pragma once

#include <string.h>

static inline void mat3_set_zero(float *m)
{
  memset(m, 0, sizeof(float)*9);
}

static inline void mat3_set_identity(float *m)
{
  mat3_set_zero(m);
  m[0] = m[4] = m[8] = 1.0f;
}

static inline void mat3_add(const float *a, const float *b, float *res)
{
  for(int k=0;k<9;k++) res[k] = a[k] + b[k];
}

static inline void mat3_sub(const float *a, const float *b, float *res)
{
  for(int k=0;k<9;k++) res[k] = a[k] - b[k];
}

static inline void mat3_mul(const float *a, const float *b, float *res)
{
  mat3_set_zero(res);
  for(int j=0;j<3;j++)
    for(int i=0;i<3;i++)
      for(int k=0;k<3;k++)
        res[i+3*j] += a[3*j+k] * b[3*k+i];
}

static inline void mat3_mulv(const float *a, const float *v, float *res)
{
  res[0] = res[1] = res[2] = 0.0f;
  for(int j=0;j<3;j++)
    for(int i=0;i<3;i++)
        res[j] += a[3*j+i] * v[i];
}

// multiply the transpose of the matrix to the vector
static inline void mat3_tmulv(const float *a, const float *v, float *res)
{
  res[0] = res[1] = res[2] = 0.0f;
  for(int j=0;j<3;j++)
    for(int i=0;i<3;i++)
        res[j] += a[3*i+j] * v[i];
}

static inline void mat3_transpose(const float *m, float *res)
{
  for(int i=0;i<3;i++) for(int k=0;k<3;k++) res[3*k+i] = m[3*i+k];
}

static inline float mat3_det(const float *a)
{
#define A(y, x) a[(y - 1) * 3 + (x - 1)]
  return
    A(1, 1) * (A(3, 3) * A(2, 2) - A(3, 2) * A(2, 3)) -
    A(2, 1) * (A(3, 3) * A(1, 2) - A(3, 2) * A(1, 3)) +
    A(3, 1) * (A(2, 3) * A(1, 2) - A(2, 2) * A(1, 3));
}

static inline float mat3_inv(const float *a, float *inv)
{
  const float det = mat3_det(a);
  if(!(det != 0.0f)) return 0.0f;
  const float invdet = 1.f / det;

  inv[3*0+0] =  invdet * (A(3, 3) * A(2, 2) - A(3, 2) * A(2, 3));
  inv[3*0+1] = -invdet * (A(3, 3) * A(1, 2) - A(3, 2) * A(1, 3));
  inv[3*0+2] =  invdet * (A(2, 3) * A(1, 2) - A(2, 2) * A(1, 3));

  inv[3*1+0] = -invdet * (A(3, 3) * A(2, 1) - A(3, 1) * A(2, 3));
  inv[3*1+1] =  invdet * (A(3, 3) * A(1, 1) - A(3, 1) * A(1, 3));
  inv[3*1+2] = -invdet * (A(2, 3) * A(1, 1) - A(2, 1) * A(1, 3));

  inv[3*2+0] =  invdet * (A(3, 2) * A(2, 1) - A(3, 1) * A(2, 2));
  inv[3*2+1] = -invdet * (A(3, 2) * A(1, 1) - A(3, 1) * A(1, 2));
  inv[3*2+2] =  invdet * (A(2, 2) * A(1, 1) - A(2, 1) * A(1, 2));
  return det;
#undef A
}

static inline float mat3_invert_sub2(const float *a, float *inv)
{
  const float det = a[0]*a[4] - a[1]*a[3];
  if(!(det != 0.0f)) return 0.0f;
  inv[0] = a[4]/det;
  inv[1] = -a[1]/det;
  inv[3] = -a[3]/det;
  inv[4] = a[0]/det;

  inv[2] = inv[5] = inv[6] = inv[7] = 0.0f;
  inv[8] = 1.0f;
  return det;
}
