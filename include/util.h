/*
    This file is part of corona-13.
    copyright (c) 2004-2017 johannes hanika

    corona-14 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    corona-14: radiata is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with corona-14.  If not, see <http://www.gnu.org/licenses/>.
*/
#pragma once

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <sched.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <fcntl.h>

// minimal math:
#ifndef M_PI
  # define M_E            2.7182818284590452354   /* e */
  # define M_LOG2E        1.4426950408889634074   /* log_2 e */
  # define M_LOG10E       0.43429448190325182765  /* log_10 e */
  # define M_LN2          0.69314718055994530942  /* log_e 2 */
  # define M_LN10         2.30258509299404568402  /* log_e 10 */
  # define M_PI           3.14159265358979323846  /* pi */
  # define M_PI_2         1.57079632679489661923  /* pi/2 */
  # define M_PI_4         0.78539816339744830962  /* pi/4 */
  # define M_1_PI         0.31830988618379067154  /* 1/pi */
  # define M_2_PI         0.63661977236758134308  /* 2/pi */
  # define M_2_SQRTPI     1.12837916709551257390  /* 2/sqrt(pi) */
  # define M_SQRT2        1.41421356237309504880  /* sqrt(2) */
  # define M_SQRT1_2      0.70710678118654752440  /* 1/sqrt(2) */
#endif

// these work for float, double, int ;)
#define cross(v1, v2, res) \
  (res)[0] = (v1)[1]*(v2)[2] - (v2)[1]*(v1)[2];\
  (res)[1] = (v1)[2]*(v2)[0] - (v2)[2]*(v1)[0];\
  (res)[2] = (v1)[0]*(v2)[1] - (v2)[0]*(v1)[1]

#define dot(u, v) ((u)[0]*(v)[0] + (u)[1]*(v)[1] + (u)[2]*(v)[2])

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define CLAMP(a, m, M) MIN(MAX(a, m), M)

static inline void normalise(float *f)
{
  const float len = 1.0f/sqrtf(dot(f, f));
  for(int k=0;k<3;k++) f[k] *= len;
}

static inline void get_perpendicular(const float* n, float* res)
{
  if(fabsf(n[1]) < 0.5)
  {
    float up[3] = {0, 1, 0};
    cross(n, up, res);
  }
  else
  {
    float rg[3] = {1, 0, 0};
    cross(n, rg, res);
  }
}

static inline void get_onb(const float *n, float *u, float *v)
{
  get_perpendicular(n, u);
  normalise(u);
  cross(n, u, v);
  // now v should be normalised already (n and u were perpendicular and normalised)
}

static inline float tofloat(uint32_t i)
{
  union { uint32_t i; float f; } u = {.i=i};
  return u.f;
}

static inline uint32_t touint(float f)
{
  union { float f; uint32_t i; } u = {.f=f};
  return u.i;
}

static inline double tofloat64(uint64_t i)
{
  union { uint64_t i; double f; } u = {.i=i};
  return u.f;
}

static inline uint64_t touint64(double f)
{
  union { double f; uint64_t i; } u = {.f=f};
  return u.i;
}

static inline float ft2m(float f)
{ // convert feet to meters
  return 0.3048f * f;
}

static inline float m2ft(float f)
{ // convert meters to feet
  return f/0.3048f;
}

static inline float ms2knots(float v)
{ // nautical miles per hour
  return 60.0f*60.0f/1852.0f * v;
}

// tiny encryption algorithm random numbers
static inline void
encrypt_tea(uint32_t arg[], float rand[])
{
  const uint32_t key[] = {
    0xa341316c, 0xc8013ea4, 0xad90777d, 0x7e95761e
  };
  uint32_t v0 = arg[0], v1 = arg[1];
  uint32_t sum = 0;
  uint32_t delta = 0x9e3779b9;

  for(int i = 0; i < 16; i++) {
    sum += delta;
    v0 += ((v1 << 4) + key[0]) ^ (v1 + sum) ^ ((v1 >> 5) + key[1]);
    v1 += ((v0 << 4) + key[2]) ^ (v0 + sum) ^ ((v0 >> 5) + key[3]);
  }
  rand[0] = v0/(0xffffffffu+1.0f);
  rand[1] = v1/(0xffffffffu+1.0f);
}

