#include "file.h"
#include "c3model.h"
// #include "comanche.h" // to dump comanche with correct obj offsets

int main(int argc, char *argv[])
{
#if 0
  com_debug_dump_obj();
  exit(0);
#endif

	uint64_t len;
	c3m_header_t *h = file_load(argv[1], &len);
  if(!h) exit(1);
  assert(h->off_eof == len);

#if 1
  // the first 6 might be a bounding box, like:
  // xmax xmin, ymax ymin, zmax zmin
  // only that it appears y is up (maybe obj -> blender flips axes)
  fprintf(stderr, "header bits:\n");
  fprintf(stderr, "%d ", h->dunno1);
  for(int k=0;k<6;k++)
    fprintf(stderr, "%d ", h->dunno2[k]);
  fprintf(stderr, "\n");
#endif

#if 1
  for(int k=0;k<6;k++)
    fprintf(stderr, "%d ", h->aabb[k]);
  fprintf(stderr, "\n");
  int32_t aabb[6] = {0};
  for(int v=0;v<c3m_num_vertices(h);v++)
  {
    // convert to weirdly flipped bounding box:
    c3m_vertex_t *v0 = c3m_get_vertices(h) + v;
    aabb[0] = aabb[0] > v0->pos[2] ? v0->pos[2] : aabb[0];
    aabb[1] = aabb[1] < v0->pos[2] ? v0->pos[2] : aabb[1];
    aabb[2] = aabb[2] > -v0->pos[1] ? -v0->pos[1] : aabb[2];
    aabb[3] = aabb[3] < -v0->pos[1] ? -v0->pos[1] : aabb[3];
    aabb[4] = aabb[4] > v0->pos[0] ? v0->pos[0] : aabb[4];
    aabb[5] = aabb[5] < v0->pos[0] ? v0->pos[0] : aabb[5];
  }
  for(int k=0;k<6;k++)
    fprintf(stderr, "%d ", aabb[k]);
  fprintf(stderr, " (<= real aabb)\n");
  for(int k=0;k<6;k++)
    fprintf(stderr, "%g ", aabb[k]/(float)0xffff);
  fprintf(stderr, " (<= real aabb in feet)\n");
#endif

  // unfortunately these are almost centered around 0
  // maybe the player.ai file and the comb_l.3do contain
  // offsets how to align these objects? at least it does
  // have exactly 23 normals.. hokum and apache don't seem to have it though.
  // (i think _l is just the lowres model)


#if 1 // DEBUG see material data
  for(int m=0;m<c3m_num_materials(h);m++)
  {
    c3m_material_t *m0 = c3m_get_materials(h) + m;
    fprintf(stderr, "%12s", m0->texname);
    for(int k=0;k<4;k++)
      fprintf(stderr, " %3u", m0->dunno[k]);
    fprintf(stderr, "\n");

    // texname seems to have leading zeroes/blanks
    // sometimes there is no texture.
    // dunno[0] is sometimes 2148009984 = 10000000000010000000100000000000b.
  }
#endif

#if 0 // DEBUG see triangle data
  for(int t=0;t<c3m_num_triangles(h);t++)
  {
    c3m_triangle_t *t0 = c3m_get_triangles(h) + t;
    fprintf(stderr, "%d %d\n", t0->dunno, t0->moff/sizeof(c3m_material_t));
    assert(t0->moff/sizeof(c3m_material_t) < c3m_num_materials(h));
  }
#endif

#if 0
  // fprintf(stderr, "file offsets: %u %u %u %u %u %u\n",
  //     h->off_mat, h->off_vtx, h->off_tri, h->off_nrm, h->offset4, h->off_eof);
  // fprintf(stderr, "block lengths: %d %d %d %d %d\n",
  //     h->off_vtx - h->off_mat, h->off_tri - h->off_vtx, h->off_nrm - h->off_tri,
  //     h->offset4 - h->off_nrm, h->off_eof - h->offset4);
  fprintf(stderr, "num mats %d num verts %d num tris %d num normals %d\n",
      c3m_num_materials(h),
      c3m_num_vertices(h),
      c3m_num_triangles(h),
      c3m_num_normals(h));
#endif

#if 0
  // DEBUG: see whether we got interesting data in the big vertices
  for(int v=0;v<c3m_num_vertices(h);v++)
  {
    c3m_vertex_t *v0 = c3m_get_vertices(h) + v;
    fprintf(stdout, "%d %d %d %d %d %d %d %d\n",
        // v0->pos[0],
        // v0->pos[1],
        // v0->pos[2],
        v0->dunno0[0],
        v0->dunno0[1],
        v0->dunno0[2],
        v0->dunno0[3],
        v0->dunno0[4],
        v0->st[0],
        v0->st[1],
        v0->dunno1);
  }
#endif

  if(argc > 2)
    c3m_dump_obj(h, argv[2]);

  free(h);

  exit(0);
}

