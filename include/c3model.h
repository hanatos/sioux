#pragma once
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>

#include "util.h" // convert feet to m

// novalogic's comanche 3 .3do model file format
typedef struct c3m_header_t
{
  char magic[4];    // 0
  uint32_t dunno0;  // version == 257 ?   // 4
  char filename[8]; // 8
  uint32_t dunno1;  // weird large number on world space range
  int32_t  aabb[6]; // zmin+1 zmax+1 -ymin -ymax+1 xmin xmax where the +-1 is probably some rounding on their side
  uint32_t dunno2[6];// 16 [4] may be 21 normal 2 don't render? [5] seems to be 0
  uint32_t off_mat;  // 68 (== 92, directly after header for null) // materials with texture names
  uint32_t off_vtx;  // 72 // vertex coordinates. int32_t stride 11, first three int are world space. [9],[10] might be texture st coordinates
  uint32_t off_tri;  // 76 // vertex indices. int32_t stride 8. [0]=0 [1]? [2,3,4] offset into vertex buffer [5,6,7] offset into normal buffer
  uint32_t off_nrm;  // 80 // normals? int32_t 4 strided are on unit sphere in signed int16_t range, fourth component seems to be 0 all the time
  uint32_t offset4;  // 84 // ?? something optional, only seen it zero sized so far
  uint32_t off_eof;  // 88 end of file offset
  // size: 92
  // it follows a buffer of int32_t sized numbers
}
c3m_header_t;

typedef struct c3m_material_t
{
  // TODO: check cmch_bod, it should have glass window materials
  char texname[12];
  uint32_t dunno[4];
  // 0 8 8 {0,128}
  // r g b a ?
  // ? ? ? ?
  // 0 0 0 0 or 0 2 127 0
}
c3m_material_t;

typedef struct c3m_vertex_t
{
  int32_t pos[3];
  int32_t dunno0[5]; // seems to be 0
  int32_t st[2];     // texture coordinates, i believe 0xffff means 1.0f. sometimes we get -2097120000 2097185535 (transparency flags?)
  int32_t dunno1;    // seems to be 0
}
c3m_vertex_t;

typedef struct c3m_triangle_t
{
  int32_t dunno;    // seems to be zero
  int32_t moff;     // material offset
  int32_t voff[3];
  int32_t noff[3];
}
c3m_triangle_t;

// seriously? this is the most terrible way to store normals
// i have ever seen, by far:
typedef struct c3m_normal_t
{
  int32_t normal[3];
  int32_t dunno;  // i think this is 0
}
c3m_normal_t;

// meta struct for mental sanity
typedef struct c3m_tri_t
{
  float v[3][3];
  float n[3][3];
  float st[3][2];
  // int material etc?
}
c3m_tri_t;


// animation data, .kda file
typedef struct c3m_kda_t
{
  // 16-byte header?
  char samp[4];      // =SAMP
  uint32_t len;      // length after header in bytes
  uint32_t dunno[2]; // lots of zeros. maybe flags?
  int8_t delta[];    // from 0x10 to end of file? sometimes 2 strided, sometimes 1d animation curve
}
c3m_kda_t;




static inline void
c3m_get_aabb(const c3m_header_t *h, float *aabb)
{
  aabb[0] =  ft2m(h->aabb[4]/(float)0xffff);
  aabb[1] = -ft2m(h->aabb[3]/(float)0xffff);
  aabb[2] =  ft2m(h->aabb[0]/(float)0xffff);
  aabb[3] =  ft2m(h->aabb[5]/(float)0xffff);
  aabb[4] = -ft2m(h->aabb[2]/(float)0xffff);
  aabb[5] =  ft2m(h->aabb[1]/(float)0xffff);
}

static inline int
c3m_num_materials(const c3m_header_t *h)
{
  return (h->off_vtx - h->off_mat)/sizeof(c3m_material_t);
}

static inline int
c3m_num_vertices(const c3m_header_t *h)
{
  return (h->off_tri - h->off_vtx)/sizeof(c3m_vertex_t);
}

static inline int
c3m_num_triangles(const c3m_header_t *h)
{
  return (h->off_nrm - h->off_tri)/sizeof(c3m_triangle_t);
}

static inline int
c3m_num_normals(const c3m_header_t *h)
{
  return (h->offset4 - h->off_nrm)/sizeof(c3m_normal_t);
}

static inline c3m_material_t*
c3m_get_materials(c3m_header_t *h)
{
  return (c3m_material_t *)((uint8_t*)h + h->off_mat);
}

static inline c3m_vertex_t*
c3m_get_vertices(c3m_header_t *h)
{
  return (c3m_vertex_t *)((uint8_t*)h + h->off_vtx);
}

static inline c3m_triangle_t*
c3m_get_triangles(c3m_header_t *h)
{
  return (c3m_triangle_t *)((uint8_t*)h + h->off_tri);
}

static inline c3m_normal_t*
c3m_get_normals(c3m_header_t *h)
{
  return (c3m_normal_t *)((uint8_t*)h + h->off_nrm);
}

static inline void
c3m_pack_tri(c3m_header_t *h, c3m_tri_t *t, int i)
{
  assert(i < c3m_num_triangles(h));
  c3m_triangle_t *tri = c3m_get_triangles(h) + i;

  assert(tri->voff[0]/sizeof(c3m_vertex_t) < c3m_num_vertices(h));
  c3m_vertex_t *v0 = c3m_get_vertices(h) + tri->voff[0]/sizeof(c3m_vertex_t);
  assert(tri->voff[1]/sizeof(c3m_vertex_t) < c3m_num_vertices(h));
  c3m_vertex_t *v1 = c3m_get_vertices(h) + tri->voff[1]/sizeof(c3m_vertex_t);
  assert(tri->voff[2]/sizeof(c3m_vertex_t) < c3m_num_vertices(h));
  c3m_vertex_t *v2 = c3m_get_vertices(h) + tri->voff[2]/sizeof(c3m_vertex_t);

  assert(tri->noff[0]/sizeof(c3m_normal_t) < c3m_num_normals(h));
  c3m_normal_t *n0 = c3m_get_normals(h) + tri->noff[0]/sizeof(c3m_normal_t);
  assert(tri->noff[1]/sizeof(c3m_normal_t) < c3m_num_normals(h));
  c3m_normal_t *n1 = c3m_get_normals(h) + tri->noff[1]/sizeof(c3m_normal_t);
  assert(tri->noff[2]/sizeof(c3m_normal_t) < c3m_num_normals(h));
  c3m_normal_t *n2 = c3m_get_normals(h) + tri->noff[2]/sizeof(c3m_normal_t);

  for(int k=0;k<3;k++) t->v[0][k] = v0->pos[k] / 1e6f;
  for(int k=0;k<3;k++) t->n[0][k] = n0->normal[k] / ((float)0xffff);
  t->st[0][0] = v0->st[0]/(float)0xffff;
  t->st[0][1] = v0->st[1]/(float)0xffff;
  for(int k=0;k<3;k++) t->v[1][k] = v1->pos[k] / 1e6f;
  for(int k=0;k<3;k++) t->n[1][k] = n1->normal[k] / ((float)0xffff);
  t->st[1][0] = v1->st[0]/(float)0xffff;
  t->st[1][1] = v1->st[1]/(float)0xffff;
  for(int k=0;k<3;k++) t->v[2][k] = v2->pos[k] / 1e6f;
  for(int k=0;k<3;k++) t->n[2][k] = n2->normal[k] / ((float)0xffff);
  t->st[2][0] = v2->st[0]/(float)0xffff;
  t->st[2][1] = v2->st[1]/(float)0xffff;
}

static inline void
c3m_to_float_arrays(
    c3m_header_t *h,
    float    **v,
    float    **n,
    float    **uv,
    uint32_t **i,
    uint32_t *num_v,
    uint32_t *num_n,
    uint32_t *num_uv,
    uint32_t *num_i)
{
  // vertices and uvs always come in lockstep. unfortunately
  // we also got triangles that refer to the same vertex but to
  // a different normal. in such a case, we'll need to duplicate
  // the vertex.
  // we'll assume no more than 6x the vertices:
  uint32_t num_vertices = c3m_num_vertices(h);
  const int num_tris = c3m_num_triangles(h);
  uint32_t hard_length = num_tris * 6;
  if(hard_length < num_vertices) hard_length = 2*num_vertices;
  uint32_t num_dup_vertices = num_vertices;
  uint32_t *dedup_v = malloc(sizeof(uint32_t)*hard_length);
  memset(dedup_v, -1, sizeof(uint32_t)*hard_length);
  // also need to copy the triangle array for this:
  c3m_triangle_t *t2 = malloc(sizeof(c3m_triangle_t)*num_tris);
  memcpy(t2, c3m_get_triangles(h), sizeof(c3m_triangle_t)*num_tris);

  *v  = malloc(sizeof(float)*3*hard_length);
  *uv = malloc(sizeof(float)*3*hard_length);
  *n  = malloc(sizeof(float)*3*hard_length);
  *i  = malloc(sizeof(uint32_t)*3*num_tris);
  *num_i = 3*num_tris;

#if 0
  int32_t texslot[128];
  int32_t slot = 1;
  const int num_mat = c3m_num_materials(h);
  for(int m=0;m<num_mat;m++)
  {
    // TODO: encode weird flags for window glass and glossiness etc?
    c3m_material_t *mt = c3m_get_materials(h)+m;
    if(mt->texname[0])
      texslot[m] = slot++;
    else
      texslot[m] = 255;
  }
#endif

  // now go through all triangles
  for(int t=0;t<num_tris;t++)
  {
    c3m_triangle_t *tri = t2+t;
    for(int k=0;k<3;k++)
    {
      uint32_t vi = tri->voff[k]/sizeof(c3m_vertex_t);
      uint32_t ni = tri->noff[k]/sizeof(c3m_normal_t);
      uint32_t mi = tri->moff/sizeof(c3m_material_t);
      c3m_vertex_t *v0 = c3m_get_vertices(h) + vi;
      c3m_normal_t *n0 = c3m_get_normals(h) + ni;
      int write = 1;
      if(dedup_v[vi] == (ni | (mi<<16))) write = 0; // all good already
      if(dedup_v[vi] == -1)
        dedup_v[vi] = (ni | (mi<<16)); // unused slot in base array
      else if(dedup_v[vi] != (ni | (mi<<16)))
      { // need to find new slot
        vi = num_dup_vertices++;
        assert(num_dup_vertices <= hard_length);
        dedup_v[vi] = (ni | (mi<<16));
      }
      // write triangle indices (new vertex index)
      (*i)[3*t+k] = vi;
      // still write vertex below
      if(!write) continue;
      // if(dedup_v[vi] == ni) continue; // all good already
      // also write new or base slot:
      (*v)[3*vi+0] = ft2m(v0->pos[0]/(float)0xffff);
      (*v)[3*vi+1] = ft2m(v0->pos[1]/(float)0xffff);
      (*v)[3*vi+2] = ft2m(v0->pos[2]/(float)0xffff);
      (*uv)[3*vi+0] = v0->st[0]/(float)0xffff;
      (*uv)[3*vi+1] = v0->st[1]/(float)0xffff;
      (*uv)[3*vi+2] = mi;
      // XXX may be flipped in weird ways
      (*n)[3*vi+0] = n0->normal[0]/(float)0xffff;
      (*n)[3*vi+1] = n0->normal[1]/(float)0xffff;
      (*n)[3*vi+2] = n0->normal[2]/(float)0xffff;
    }
  }
  *num_v = *num_uv = *num_n = num_dup_vertices;

  free(dedup_v);
  free(t2);
}

// dump the loaded model as wavefront obj file.
// includes texture st and normals, splits by material.
// also writes a basic .mtl file with texture assignments.
static inline int
c3m_dump_obj(c3m_header_t *h, const char *filename)
{
  FILE *f = fopen(filename, "wb");
  if(!f) return 1;
  for(int v=0;v<c3m_num_vertices(h);v++)
  {
    c3m_vertex_t *v0 = c3m_get_vertices(h) + v;
    fprintf(f, "v %g %g %g\n",
        v0->pos[0]/(float)0xffff, v0->pos[1]/(float)0xffff, v0->pos[2]/(float)0xffff);
    // XXX TODO: these coordinates are /sometimes/ flipped, but sometimes also not?
    // XXX body and blade rotor are flipped, tail seems not so much or different?
    fprintf(f, "vt %g %g\n",
        1.0f-v0->st[0]/(float)0xffff, 1.0f-v0->st[1]/(float)0xffff);
  }
  for(int n=0;n<c3m_num_normals(h);n++)
  {
    c3m_normal_t *n0 = c3m_get_normals(h) + n;
    fprintf(f, "vn %g %g %g\n",
        n0->normal[0]/(float)0xffff, n0->normal[1]/(float)0xffff, n0->normal[2]/(float)0xffff);
  }

  char mtlname[1024];
  strncpy(mtlname, filename, sizeof(mtlname)-1);
  char *c = mtlname+strlen(mtlname);
  while(*c != '.' && c > mtlname) c--;
  *c++ = '.'; *c++ = 'm'; *c++ = 't'; *c++ = 'l'; *c++ = 0;
  FILE *mtl = fopen(mtlname, "wb");
  if(!mtl) return 1;
  // separate by material (most stupid way of doing it):
  for(int m=0;m<c3m_num_materials(h);m++)
  {
    char mat[1024];
    char *tex = c3m_get_materials(h)[m].texname;
    int len = strlen(tex);
    c = tex + len;
    while(*c != '.' && c > tex) c--;
    if(c == tex) sprintf(tex, "%04d", m);
    else sprintf(c, ".png");
    // { *c++ = '.'; *c++ = 'p'; *c++ = 'n'; *c++ = 'g'; *c++ = 0; }
    c = tex;
    for(int i=0;i<len;i++) c[i] = tolower(c[i]);
    snprintf(mat, sizeof(mat), "%s_%s", filename, tex);
    fprintf(f, "usemtl %s\n", mat);
    fprintf(f, "o %s\n", mat);
    // also write material file:
    fprintf(mtl, "newmtl %s\n"
        "Ns 0\n"
        "Ka 0.000000 0.000000 0.000000\n"
        "Kd 0.800000 0.800000 0.800000\n"
        "Ks 0.000000 0.000000 0.000000\n"
        "Ni 1.000000\n"
        "d 1.000000\n"
        "illum 1\n"
        "map_Kd %s\n",
        mat, tex);
    for(int t=0;t<c3m_num_triangles(h);t++)
    {
      c3m_triangle_t *tri = c3m_get_triangles(h) + t;
      int mind = tri->moff/sizeof(c3m_material_t);
      if(mind == m)
        fprintf(f, "f %lu/%lu/%lu %lu/%lu/%lu %lu/%lu/%lu\n",
          1+tri->voff[0]/sizeof(c3m_vertex_t),
          1+tri->voff[0]/sizeof(c3m_vertex_t),
          1+tri->noff[0]/sizeof(c3m_normal_t),
          1+tri->voff[1]/sizeof(c3m_vertex_t),
          1+tri->voff[1]/sizeof(c3m_vertex_t),
          1+tri->noff[1]/sizeof(c3m_normal_t),
          1+tri->voff[2]/sizeof(c3m_vertex_t),
          1+tri->voff[2]/sizeof(c3m_vertex_t),
          1+tri->noff[2]/sizeof(c3m_normal_t));
    }
  }
  fclose(f);
  fclose(mtl);
  return 0;
}
