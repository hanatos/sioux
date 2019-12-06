#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

static inline void
sx_vid_gl_terrain_init(
    float    **out_v,
    uint32_t **out_idx,
    uint32_t  *out_num_v,
    uint32_t  *out_num_p)
{
  float size = 4096.0f; // 4km
  int wd = 129;
  // number of vertices
  int num = wd * wd;
  float *v = malloc(sizeof(float)*3*num);
  uint32_t *idx = malloc(sizeof(uint32_t)*3*2*num);

  // build a LOD structure from the camera coordinate at 0,0,0.
  // we'll refine this using tesselation shaders on the GPU later on.
  // FILE *f = fopen("debug.obj", "wb");

  int num_vtx = 0;
  int num_p = 0;

  // we'll build a super simple square regular grid, and warp it a little
  // afterwards. first the simple bits:
  for(int j=0;j<wd;j++)
  {
    for(int i=0;i<wd;i++)
    {
      float z = j/(wd-1.0f);
      float x = i/(wd-1.0f);
      float y = 0.0f;

      // normalise to -1,1:
      z = -1.0f + 2.0f*z;
      x = -1.0f + 2.0f*x;
      // "importance sampling":
      // rescale to what we think the radius should be
      float rad = sqrtf(z*z + x*x);
      float tgt = fmaxf(fabsf(x), fabsf(z));
      float scale = tgt / rad;
      // rescale square to circle:
      z *= scale;
      x *= scale;
      // stretch circle to have more detail at 0
      rad = sqrtf(z*z + x*x);
      rad *= rad;
      z *= rad * size;
      x *= rad * size;
      // emit vertex in worldspace
      v[3*num_vtx+0] = x;
      v[3*num_vtx+1] = y;
      v[3*num_vtx+2] = z;
      num_vtx++;
      // fprintf(f, "v %g %g %g\n", x, y, z);
      
      // emit two triangles
      if(i && j)
      {
        idx[3*num_p+0] = wd*(j-1) + i-1;
        idx[3*num_p+1] = wd*(j  ) + i  ;
        idx[3*num_p+2] = wd*(j  ) + i-1;
        // fprintf(f, "f %d %d %d\n",
        //     idx[3*num_p+0]+1,
        //     idx[3*num_p+1]+1,
        //     idx[3*num_p+2]+1);
        num_p++;
        idx[3*num_p+0] = wd*(j-1) + i-1;
        idx[3*num_p+1] = wd*(j-1) + i;
        idx[3*num_p+2] = wd*(j  ) + i;
        // fprintf(f, "f %d %d %d\n",
        //     idx[3*num_p+0]+1,
        //     idx[3*num_p+1]+1,
        //     idx[3*num_p+2]+1);
        num_p++;
      }
    }
  }
  *out_v = v;
  *out_idx = idx;
  *out_num_v = num_vtx;
  *out_num_p = num_p;
  // fclose(f);
}

// TODO: submit this geo as GL_PATCH (or let's try triangles first and see whether we can see something at all)
// TODO: need a camera-space vertex shader
// TODO: provide tessellation control and tessellation evaluation shaders
// TODO: add in some procedural detail?
