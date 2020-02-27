#pragma once
// test obb vs obb via separating axis theorem.
// after a compact answer here:
// https://gamedev.stackexchange.com/questions/112883/simple-3d-obb-collision-directx9-c

typedef struct sx_obb_t
{
  float pos[3], x[3], y[3], z[3], hsize[3];
}
sx_obb_t;

static inline void sx_obb_get(
    sx_obb_t *obb,
    const sx_entity_t *e,
    uint32_t g,
    uint32_t o)
{
  const uint32_t oi = e->objectid;
  const float *b = sx.assets.object[oi].geo_aabb[g];
  obb[0] = (sx_obb_t) {
    .pos={.5f*(b[3]+b[0]), .5f*(b[4]+b[1]), .5f*(b[5]+b[2])},
    .x={1,0,0},
    .y={0,1,0},
    .z={0,0,1},
    .hsize={.5f*(b[3]-b[0]), .5f*(b[4]-b[1]), .5f*(b[5]-b[2])}};
  // compute offset to center of aabb and transform to world space:
  if(o != -1)
    for(int k=0;k<3;k++)
      obb[0].pos[k] += sx.assets.object[oi].geo_off[g][3*o+k];
  quat_transform(&e->body.q, obb[0].pos);
  for(int k=0;k<3;k++) obb[0].pos[k] += e->body.c[k];
  // transform obb axes to world space, too:
  quat_transform(&e->body.q, obb[0].x);
  quat_transform(&e->body.q, obb[0].y);
  quat_transform(&e->body.q, obb[0].z);
}

static inline int
sx_collision_test_plane(
    float *rpos,    // relative position, i.e. box2.pos - box1.pos
    float *n,       // normal of separating plane
    sx_obb_t *box1,
    sx_obb_t *box2)
{
  return fabsf(dot(rpos, n) >
      fabsf(dot(box1->x, n)*box1->hsize[0]) +
      fabsf(dot(box1->y, n)*box1->hsize[1]) +
      fabsf(dot(box1->z, n)*box1->hsize[2]) +
      fabsf(dot(box2->x, n)*box2->hsize[0]) +
      fabsf(dot(box2->y, n)*box2->hsize[1]) +
      fabsf(dot(box2->z, n)*box2->hsize[2]));
}

static inline int
sx_collision_test_obb_obb(
    sx_obb_t *box1,
    sx_obb_t *box2)
{
  float n[3], rpos[] = {
    box2->pos[0] - box1->pos[0],
    box2->pos[1] - box1->pos[1],
    box2->pos[2] - box1->pos[2]};
  if(sx_collision_test_plane(rpos, box1->x, box1, box2)) return 0;
  if(sx_collision_test_plane(rpos, box1->y, box1, box2)) return 0;
  if(sx_collision_test_plane(rpos, box1->z, box1, box2)) return 0;
  if(sx_collision_test_plane(rpos, box2->x, box1, box2)) return 0;
  if(sx_collision_test_plane(rpos, box2->y, box1, box2)) return 0;
  if(sx_collision_test_plane(rpos, box2->z, box1, box2)) return 0;
  cross(box1->x, box2->x, n); if(sx_collision_test_plane(rpos, n, box1, box2)) return 0;
  cross(box1->x, box2->y, n); if(sx_collision_test_plane(rpos, n, box1, box2)) return 0;
  cross(box1->x, box2->z, n); if(sx_collision_test_plane(rpos, n, box1, box2)) return 0;
  cross(box1->y, box2->x, n); if(sx_collision_test_plane(rpos, n, box1, box2)) return 0;
  cross(box1->y, box2->y, n); if(sx_collision_test_plane(rpos, n, box1, box2)) return 0;
  cross(box1->y, box2->z, n); if(sx_collision_test_plane(rpos, n, box1, box2)) return 0;
  cross(box1->z, box2->x, n); if(sx_collision_test_plane(rpos, n, box1, box2)) return 0;
  cross(box1->z, box2->y, n); if(sx_collision_test_plane(rpos, n, box1, box2)) return 0;
  cross(box1->z, box2->z, n); if(sx_collision_test_plane(rpos, n, box1, box2)) return 0;
  return 1;
}
