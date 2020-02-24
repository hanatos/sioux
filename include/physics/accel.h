/*
    This file is part of corona-13.

    corona-13 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    corona-13 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with corona-13. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once
#include <stdint.h>

typedef struct accel_t accel_t;
typedef accel_t sx_bvh_t;

// init new acceleration structure for given primitives (don't build yet)
accel_t* accel_init(uint64_t prim_cnt,
    void (*compute_boxes)(uint64_t b, uint64_t e, float *aabb));

// free memory
void accel_cleanup(accel_t *b);

// build acceleration structure and finish on return. also clean up, so this is
// only good for static objects.
void accel_build(accel_t *b, const char* filename);

// build acceleration structure in the background
void accel_build_async(accel_t *b);

// wait for build to finish. build can be triggered again.
void accel_build_wait(accel_t *b);

int accel_collide(accel_t *b, const float *aabb, uint64_t ignore, uint64_t *result, int result_cnt);

void accel_set_prim_cnt(accel_t *b, uint64_t prim_cnt);

#if 0
// intersect ray (closest point)
void accel_intersect(const accel_t *b, const ray_t *ray, hit_t *hit);

// test visibility up to max distance
int  accel_visible(const accel_t *b, const ray_t *ray, const float max_dist);

// find closest geo intersection point to world space point at ray with parameter centre:
// ray->pos + centre * ray->dir
void accel_closest(const accel_t *b, ray_t *ray, hit_t *hit, const float centre);

// return pointer to the 6-float minxyz-maxxyz aabb
const float *accel_aabb(const accel_t *b);
#endif
