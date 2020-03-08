#include "physics/grid.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// regular hashed grid for collision detection.
// we usually have well < 1000 objects to collide.
// put only collidable into grid (not missiles)
// query only stuff that does something (moving objects crash into static ones,
// missiles make stuff blow up etc)

// use 1024^2 tiled morton codes to derive buckets?
// fill list by adding morton codes for objects + entity id
// sort list
// query: bisect list with our own morton codes (we expect 4 per entity)
// our hashed grid will be 16x16 meters per cell, such that a comanche
// with 12m rotor diameter usually occupies 4.
// that makes 1024/64 so 6 bits per dimension.

int compare_element(const void *ai, const void *bi)
{
  uint64_t a = ((uint64_t*)ai)[0];
  uint64_t b = ((uint64_t*)bi)[0];
  return (a>>32) - (b>>32); // compare morton code portion only
}

// https://stackoverflow.com/questions/30539347/2d-morton-code-encode-decode-64bits
static inline uint32_t
xy_to_morton(uint32_t x, uint32_t y)
{
  // return _pdep_u32(x, 0x55555555) | _pdep_u32(y,0xaaaaaaaa); // returns 64 bits
  // traditional version, also just for 32 bits
  x = (x | (x << 8)) & 0x00FF00FFu;
  x = (x | (x << 4)) & 0x0F0F0F0Fu;
  x = (x | (x << 2)) & 0x33333333u;
  x = (x | (x << 1)) & 0x55555555u;

  y = (y | (y << 8)) & 0x00FF00FFu;
  y = (y | (y << 4)) & 0x0F0F0F0Fu;
  y = (y | (y << 2)) & 0x33333333u;
  y = (y | (y << 1)) & 0x55555555u;

  return x | (y << 1);
}

#if 0
static inline void
morton_to_xy(uint64_t m, uint32_t *x, uint32_t *y)
{
  *x = _pext_u64(m, 0x5555555555555555);
  *y = _pext_u64(m, 0xaaaaaaaaaaaaaaaa);
}
#endif

void
sx_grid_init(sx_grid_t *g, int num_el)
{
  memset(g, 0, sizeof(*g));
  // alloc list, cnt = 0;
  g->element_max = num_el;
  g->element = malloc(sizeof(g->element[0])*num_el);
  g->element_cnt = 0;
}

void
sx_grid_cleanup(sx_grid_t *g)
{
  free(g->element);
  memset(g, 0, sizeof(*g));
}

void
sx_grid_build(sx_grid_t *g)
{
  // sort list
  // TODO: use our faster parallel radix sort (if worth it)
  qsort(g->element, g->element_cnt, sizeof(g->element[0]), compare_element);
}

static inline int
bisect(sx_grid_t *g, uint32_t m)
{
  int a = 0, b = g->element_cnt;
  while(b - a > 1)
  {
    // we want a to point to the first element in the list with el[a]==m, if any.
    // so during bisection, we only allow [b]==m, and after that we'll step a one further.
    int c = (a + b)/2;
    if((g->element[c] >> 32) >= m)
      b = c;
    else
      a = c;
  }
  a++;
  return a;
}

static inline uint32_t
dedup(uint32_t *coll, uint32_t cnt)
{ // n^2 deduplication, this sucks.
  for(int i=0;i<cnt;i++)
    for(int j=i+1;j<cnt;j++)
      if(coll[i] == coll[j])
        coll[j--] = coll[--cnt]; // remove j
  return cnt;
}

static inline uint32_t
query(sx_grid_t *g, uint32_t m, uint32_t *coll, uint32_t cnt, uint32_t max_cnt)
{
  int beg = bisect(g, m);
  while(beg < g->element_cnt)
  {
    coll[cnt++] = g->element[beg++] & 0xffffffffu; // keep entity index
    if((g->element[beg]>>32) != m) break;          // not this cell any more
  }
  return dedup(coll, cnt);
}

uint32_t
sx_grid_query(sx_grid_t *g, float *aabb, uint32_t *coll, uint32_t collider_max)
{
  // TODO: compute aabb in integers
  // TODO: compute morton codes, bisect list
  uint32_t cnt = 0;
  uint32_t m[4];
  for(int c=0;c<4;c++)
    m[c] = xy_to_morton(
        aabb[0 + ((c&1)?3:0)]/64.0f,
        aabb[2 + ((c&2)?3:0)]/64.0f);
  // query a couple of times if necessary
  int num_queries = dedup(m, 4);
  // fprintf(stderr, "querying morton %u %u %u %u num %d\n", m[0], m[1], m[2], m[3], num_queries);
  for(int i=0;i<num_queries;i++)
    cnt = query(g, m[i], coll, cnt, collider_max);
  // fprintf(stderr, "colliding %u %u %u %u %u %u max %u\n", coll[0], coll[1], coll[2], coll[3], coll[4], coll[5], cnt);
  return cnt;
}

void sx_grid_clear(sx_grid_t *g)
{
  // set count to zero
  g->element_cnt = 0;
}

void 
sx_grid_add(sx_grid_t *g, const float *aabb, uint32_t eid)
{
  // TODO: actually would need to rasterise aabb into int grid,
  //       i.e. walk between min and max int bounds
  uint32_t m[4];
  for(int c=0;c<4;c++)
    m[c] = xy_to_morton(
        aabb[0 + ((c&1)?3:0)]/64.0f,
        aabb[2 + ((c&2)?3:0)]/64.0f);

  g->element[g->element_cnt++] = ((uint64_t)m[0]<<32) | eid;
  if(m[0] != m[1])
    g->element[g->element_cnt++] = ((uint64_t)m[1]<<32) | eid;
  if(m[0] != m[2] && m[1] != m[2])
    g->element[g->element_cnt++] = ((uint64_t)m[2]<<32) | eid;
  if(m[0] != m[3] && m[1] != m[3] && m[2] != m[3])
    g->element[g->element_cnt++] = ((uint64_t)m[3]<<32) | eid;
}
