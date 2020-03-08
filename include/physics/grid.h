#pragma once

#include <stdint.h>

typedef struct sx_grid_t
{
  float aabb[6];
  // 2D is enough for our purposes, we'll do the fine checks anyways
  uint64_t *element;      // 6+6 morton + 32 entity id
  uint64_t  element_cnt;
  uint64_t  element_max;
}
sx_grid_t;

void sx_grid_init(sx_grid_t *g, int num_el);
void sx_grid_cleanup(sx_grid_t *g);
void sx_grid_build(sx_grid_t *g);
uint32_t sx_grid_query(sx_grid_t *g, float *aabb, uint32_t *collider, uint32_t collider_max);
void sx_grid_clear(sx_grid_t *g);
void sx_grid_add(sx_grid_t *g, const float *aabb, uint32_t eid);
